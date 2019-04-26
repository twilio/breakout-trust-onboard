# Library for Trust Onboard 

## Contents
1. [Overview](#overview)
1. [MF class](#mf)
1. [MIAS class](#mias)
    1. [Extracting available certificate and private key](#extracting-available-certificate-and-private-key)
    1. [Using signing certificate and private key](#using-signing-certificate-and-private-key)
1. [SEInterface](#seinterface)
    1. [GenericModem](#genericmodem)
    1. [PcscSEInterface](#pcscseinterface)

## Overview

The library implements an interface to the Trust Onboard SIM (smart card) to use it as a security element. Two major parts of the library's API are `MF` and `MIAS` classes. `SEInterface` abstract class lets the user implement their own interfaces to the smart card. Some useful implementations of `SEInterface` can be found in `platform` directory.

Trust Onboard card contains two pairs of credentials: one pair is called **available** certificate and private key, while the other is called **signing** certificate and private key. The difference between the two is that **available** private key can be read out of the element and used as a normal file, while **signing** private key never leaves the smart card, and requires `MIAS` operations described below in [MIAS section](#MIAS) to work.

## MF
MF applet and the class that represents it, provides an access to the file tree on the smart card. The underlying interface is as described in [ISO/IEC 7816-4](https://www.iso.org/standard/54550.html), that is a tree of files with master file (MF) at its root and elementary files (EF) at leaves.

To start using the applet, it should first be initialized with an instance of `SEInterface` (once) and the selected (each time it is used) as in:

```
  auto mf = new MF();
  mf->init(se_interface);

  mf->select();

  // do MF operations
  ...
  // end of MF operations

  other_applet->select();

  // do something else
  ...
  // switch back to MF
  mf->select();
```

The main opertaion `MF` class supports is to reading out elementary file contents.

```
    #define PATH "7FAA6F01"
    uint8_t* buf;
    uint16_t read_len;

    mf->select();
    mf->verifyPin("0000", 4);
    if(!mf->readEF(PATH, 8, &buf, &read_len)) {
      // Error
    } else {
      // Do something with the data
      free(buf);
    }
```

The first parameter is a file path as specified in *Selection by path* in 5.3.1.1 of [ISO/IEC 7816-4](https://www.iso.org/standard/54550.html) encoded as a hex string, the second is the length of this string, the last two give a pointer to allocate the buffer and a variable to store the actual amount of data read out.

You might have noticed the `verifyPin` function. It's purpose as the name implies is to unlock the applet before it can be used. The second parameter is the length of pin string.

There are two convenience functions to read out the **available** certificate and private key without having to deal with file paths. These are:

```
    bool MF::readCertificate(uint8_t** data, uint16_t dataLen);
    bool MF::readPrivateKey(uint8_t** data, uint16_t dataLen);
```

## MIAS
MIAS applet implements security operations defined in [ISO/IEC 7816-8](https://www.iso.org/standard/66092.html). **Available** key pair can also be extracted from it, and it supports operations for TLS authentication with the **signing** key pair as well.

For all operations listed below, the MIAS applet should be selected and unlocked the same way as was described for MF applet (the PIN value may be different for both), calls to `select()` and `verifyPin` are assumed and omitted in the code samples.

### Extracting available certificate and private key
Both can be extracted as:

```
    uint8_t *cert;
    uint16_t cert_len;

    uint8_t *pkey;
    uint16_t pkey_len;

    mias->p11GetObjectByLabel("CERT_TYPE_A", 11, &cert, &cert_len);
    mias->p11GetObjectByLabel("PRIV_TYPE_A", 11, &pkey, &pkey_len);

    // do something with both
    ...
    // done
    free(cert);
    free(pkey);
```

### Using signing certificate and private key
Usage of signing certificate and private key is somewhat more complicated and depends on the TLS library in use. In general you first need to extract the certificate and get the handle for your private key. With this handle you can call respective functions to sign and decrypt data.

```
    uint8_t *cert;
    uint16_t cert_len;
    mias_key_pair_t *key;

    mias->getCertificateByContainerId(0, &cert, &cert_len);

    // Store/register the certificate
    ...
    // done

    free(cert);

    mias->getKeyPairByContainerId(0, &key);

    // some data to sign arrived
    mias->signInit(RSA_WITH_PKCS1_PADDING, key->kid);

    uint8_t sig[4096];
    uint16_t sig_len;
    mias->signFinal(data, data_len, sig, &sig_len);

    // do something with the signature
    ...
    // done

    // Data to decrypt arrived
    uint8_t plaintext[MAX_PLAINTEXT];
    uint16_t plaintext_len;

    decryptInit(ALGO_SHA256_WITH_RSA_PKCS1_PADDING, key->kid);
    decryptFinal(ciphertext, ciphertext_len, plaintext, &plaintext_len);
``` 

At the time of writing only container with ID 0 existed and only `ALGO_SHA256_WITH_RSA_PKCS1_PADDING` was supported.

## SEInterface
`SEInterface` is an abstract class representing an interface to security element. The interface should be able to transmit command-response pairs as described in Chapter 12 of [ISO/IEC 7816-4](https://www.iso.org/standard/38770.html)

An implementation of `SEInterface` should define three methods (not including the destructor):

```
    virtual bool open();
    virtual void close();
    virtual bool transmitApdu(const uint8_t* apdu, uint16_t apduLen, uint8_t* response, uint16_t* responseLen);
```

`open()` and `close()` are used to open (possibly unsuccessfully) and close the interface respectively.

`transmitApdu()` blockingly sends a command APDU and returns the response back. The operation can fail in which case `false` should be returned.

An instance of SEInterface implementation could be then passed to an `Applet`'s `init()` functions.

```
    MySEInterface iface;
    MIAS mias;

    if (!iface.open()) {
      return -1;
    }

    if (!mias.init(&iface)) {
      return -1;
    }
```

Two interface implementations are already provided: `GenericModem` for a baseline AT serial modem and `PcscSEInterface` using PCSCLite to connect to the smart card with a card reader.

### GenericModem

Can be instantiated as:

```
    GenericModem modem("/dev/ttyUSB0");
```

substituting path to your serial device. The serial interface should be connected to a AT modem capable of processing `AT+CSIM` command according to section 8.17 of [ETSI TS 127 007](https://www.etsi.org/deliver/etsi_ts/127000_127099/127007/13.04.00_60/ts_127007v130400p.pdf).

### PcscSEInterface

Requires libpcsclite to be installed. Instantiated as:

```
    PcscSEInterface pcsc(0);
```

taking the index of a card reader in the list as the argument. It is recommended to always have only one reader connected as set it to `0` in order not to rely on the ordering PCSCLite output.
