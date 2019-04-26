/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 *
 */

#include "MIAS.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//#define USE_GAT_RESPONSE

static uint8_t AID[] = {0xA0, 0x00, 0x00, 0x00, 0x18, 0x80, 0x00, 0x00, 0x00, 0x06, 0x62, 0x41, 0x51};

/* CONTAINERS_INFO file is organized into 11 byte long records. Each record has the following structure:
 *   Off 0   - non-zero indicates a valid record
 *   Off 4-5 - 0x0000 for decryption key pair or non-zero size in bits for signature key pair (big endian)
 *   Off 6-7 - size in bits for decription key pair (bif endian)
 *   Other bytes are reserved/not used here.
 */
static uint8_t CONTAINERS_INFO_EF[] = {0x00, 0x02};

/* FILE_DIR file is organized into 21 byte long records. Each record has the following structure:
 *   Off 0-1 - file ID
 *   Off 2-3 - file size
 *   Off 4-11 - File name, padded to the left.
 *   Off 12-19 - Directory name, padded to the left. We are interested only in files under "mscp".
 *   Other bytes are reserved/not used here.
 */

static uint8_t FILE_DIR_EF[] = {0x01, 0x01};

MIAS::MIAS(void) : Applet(AID, sizeof(AID)) {
  _keypairs_num = -1;

  _hashAlgo = 0;

  _signAlgo = 0;
  _signKey  = 0;

  _decryptKey  = 0;
  _decryptAlgo = 0;
}

MIAS::~MIAS(void) {
}

/** PRIVATE *******************************************************************/

bool MIAS::mseSetBeforeHash(uint8_t algorithm) {
  uint8_t data[3];

  data[0] = 0x80;
  data[1] = 0x01;
  data[2] = algorithm;

  if (transmit(0x00, SCIns::ManageSecurityEnvironment, SCP1::MSECompDecInt | SCP1::MSESet, SCP2::MSETemplateHashCode,
               data, sizeof(data))) {
    if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
      return true;
    }
  }

  return false;
}

bool MIAS::psoHashInternally(uint8_t algorithm, const uint8_t* data, uint16_t dataLen) {
  uint8_t block_size;
  uint16_t i, l;

  if ((algorithm == ALGO_SHA1) || (algorithm == ALGO_SHA224) || (algorithm == ALGO_SHA256)) {
    block_size = 64;
  } else if ((algorithm == ALGO_SHA384) || (algorithm == ALGO_SHA512)) {
    block_size = 128;
  } else {
    return false;
  }

  for (i = 0; i < dataLen; i += l) {
    if ((l = dataLen - i) > block_size) {
      l = block_size;
    }

    if (!transmit(0x00, SCIns::PerformSecurityOperation, SCP1::PSOHashCode, SCP2::PSOPlain, &data[i], l)) {
      return false;
    }

    if (getStatusWord() != (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
      return false;
    }
  }

  return true;
}

bool MIAS::psoHashInternallyFinal(uint8_t* hash, uint16_t* hashLen) {
  uint8_t data[2];

  data[0] = static_cast<uint8_t>(SCTag::PSOHashInt);
  data[1] = 0x00;

  *hashLen = 0;

  if (transmit(0x00, SCIns::PerformSecurityOperation, SCP1::PSOHashCode, SCP2::PSOTemplateHash, data, sizeof(data),
               0x00)) {
    if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
      *hashLen = getResponse(hash);
      return true;
    }
  }

  return false;
}

bool MIAS::psoHashExternally(uint8_t algorithm, const uint8_t* hash, uint16_t hashLen) {
  uint8_t data[66];

  data[0] = static_cast<uint8_t>(SCTag::PSOHashExt);
  switch (algorithm) {
    case ALGO_SHA1:
      data[1] = 20;
      break;

    case ALGO_SHA224:
      data[1] = 28;

      break;
    case ALGO_SHA256:
      data[1] = 32;

      break;
    case ALGO_SHA384:
      data[1] = 48;

      break;
    case ALGO_SHA512:
      data[1] = 64;
      break;

    default:
      return false;
  }
  memcpy(&data[2], hash, data[1]);

  if (transmit(0x00, SCIns::PerformSecurityOperation, SCP1::PSOHashCode, SCP2::PSOTemplateHash, data, 2 + data[1],
               0x00)) {
    if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
      return true;
    }
  }

  return false;
}

bool MIAS::mseSetBeforeSignature(uint8_t algorithm, uint8_t key) {
  uint8_t data[6];

  data[0] = static_cast<uint8_t>(SCTag::MSEAlgReference);
  data[1] = 0x01;
  data[2] = algorithm;
  data[3] = static_cast<uint8_t>(SCTag::MSEPublicKey);
  data[4] = 0x01;
  data[5] = key;

  if (transmit(0x00, SCIns::ManageSecurityEnvironment, SCP1::MSECompDecInt | SCP1::MSESet, SCP2::MSETemplateSignature,
               data, sizeof(data))) {
    if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
      return true;
    }
  }

  return false;
}

bool MIAS::psoComputeDigitalSignature(uint8_t* signature, uint16_t* signatureLen) {
  uint16_t len;

  *signatureLen = 0;

  if (transmit(0x00, SCIns::PerformSecurityOperation, SCP1::PSOSignature, SCP2::PSOSignatureInput, 0x00)) {
#ifdef USE_GAT_RESPONSE
    if (_isBasic) {
#endif
      while (getResponseLength()) {
        len = getResponse(signature);
        signature += len;
        *signatureLen += len;
        if ((getStatusWord() & 0xFF00) == SCSW1::OKLengthInSW2) {
          // Transmit GET Response
          if (transmit(0x00, SCIns::Envelope, SCP1::ENVELOPEReserved, SCP2::ENVELOPEReserved,
                       getStatusWord() & 0x00FF)) {
            continue;
          }
        } else if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
          return true;
        }
        return false;
      }
#ifdef USE_GAT_RESPONSE
    } else {
      // GAT format: [DATA1 DATA2 ... ][GAT SW1 SW2][90 00]
      while ((getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) &&
             (_seiface->_apduResponseLen >= 4)) {
        // Convert ApduResponse in order to have GAT status word as regular status word
        _seiface->_apduResponseLen -= 2;

        len = getResponse(signature);
        signature += len;
        *signatureLen += len;
        if ((getStatusWord() & 0xFF00) == SCSW1::OKLengthinSW2) {
          // Transmit GAT Response
          if (transmit(0x00, SCIns::Envelope, SCP1::ENVELOPEReserved, SCP2::ENVELOPEReserved,
                       getStatusWord() & 0x00FF)) {
            continue;
          }
        } else if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
          return true;
        }
        return false;
      }
    }
#endif
  }

  return false;
}

bool MIAS::mseSetBeforeDecrypt(uint8_t algorithm, uint8_t key) {
  uint8_t data[6];

  data[0] = static_cast<uint8_t>(SCTag::MSEAlgReference);
  data[1] = 0x01;
  data[2] = algorithm;
  data[3] = static_cast<uint8_t>(SCTag::MSEPublicKey);
  data[4] = 0x01;
  data[5] = key;

  if (transmit(0x00, SCIns::ManageSecurityEnvironment, SCP1::MSECompDecInt | SCP1::MSESet,
               SCP2::MSETemplateConfidentiality, data, sizeof(data))) {
    if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
      return true;
    }
  }

  return false;
}

bool MIAS::psoDecipher(const uint8_t* data, uint16_t dataLen, uint8_t* plain, uint16_t* plainLen) {
  uint8_t buf[255];

  if (dataLen < 255) {
    buf[0] = static_cast<uint8_t>(SCTag::PSOPaddingProprietary1);
    memcpy(&buf[1], data, dataLen);

    if (!transmit(0x00, SCIns::PerformSecurityOperation, SCP1::PSOPlain, SCP2::PSOPadding, buf, dataLen + 1)) {
      return false;
    }
  } else {
    buf[0] = static_cast<uint8_t>(SCTag::PSOPaddingProprietary1);
    memcpy(&buf[1], data, 0xFE);
    data += 0xFE;
    dataLen -= 0xFE;

    if (!transmit(0x10, SCIns::PerformSecurityOperation, SCP1::PSOPlain, SCP2::PSOPadding, buf, 0xFF)) {
      return false;
    }

    if (getStatusWord() != (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
      return false;
    }

    if (!transmit(0x00, SCIns::PerformSecurityOperation, SCP1::PSOPlain, SCP2::PSOPadding, data, dataLen)) {
      return false;
    }
  }

  if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
    *plainLen = getResponse(plain);
    return true;
  }

  return false;
}

bool MIAS::listKeyPairs(void) {
  uint16_t size = 0;
  mias_key_pair_t* ptr;

  if (_keypairs_num != -1) {
    return true;
  }

  if (transmit(0x00, SCIns::Select, SCP1::SELECTByPathFromMF, SCP2::SELECTFirstOrOnly | SCP2::SELECTFCPTemplate,
               CONTAINERS_INFO_EF, sizeof(CONTAINERS_INFO_EF), 0x15)) {
    if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
      if (getResponseLength() > 2) {
        if (_seiface->_apduResponse[0] == SCTag::FileControlInfoFCP) {
          uint8_t i;
          uint8_t len;
          SCTag t;
          uint8_t l;
          uint16_t offset;

          len = _seiface->_apduResponse[1];

          for (i = 2, len += 2; i < len;) {
            t = static_cast<SCTag>(_seiface->_apduResponse[i]);
            l = _seiface->_apduResponse[i + 1];

            switch (t) {
              case SCTag::FCPFileSizeWithInfo:
                size = (_seiface->_apduResponse[i + 2] << 8) | _seiface->_apduResponse[i + 3];
                break;
            }

            i += 2 + l;
          }


          for (i = 0, offset = 0; offset < size; i++, offset += 0x0B) {
            if (transmit(0x00, SCIns::ReadBinary, (offset >> 8) & 0xFF, offset & 0xFF, 0x0B)) {
              if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
                if (_seiface->_apduResponse[0]) {
                  if (_keypairs_num >= key_pool_size - 1) break;

                  ++_keypairs_num;
                  ptr                 = &_keypairs[_keypairs_num];
                  ptr->pub_file_id[0] = 0;
                  ptr->pub_file_id[1] = 0;

                  ptr->flags = SIGNATURE_KEY_PAIR_FLAG;
                  if ((ptr->size_in_bits = (_seiface->_apduResponse[4] << 8) | _seiface->_apduResponse[5]) == 0) {
                    ptr->flags |= DECRYPTION_KEY_PAIR_FLAG;
                    ptr->size_in_bits = (_seiface->_apduResponse[6] << 8) | _seiface->_apduResponse[7];
                  }

                  // RSA 1024-bits exchange keys
                  if ((ptr->size_in_bits == 0x400) &&
                      ((ptr->flags & (SIGNATURE_KEY_PAIR_FLAG | DECRYPTION_KEY_PAIR_FLAG)) ==
                       (SIGNATURE_KEY_PAIR_FLAG | DECRYPTION_KEY_PAIR_FLAG))) {
                    ptr->kid = 0x10 | (i + 1);
                    ptr->flags |= RSA_KEY_PAIR_FLAG;
                  }

                  // RSA 1024-bits signature keys
                  else if ((ptr->size_in_bits == 0x400) &&
                           ((ptr->flags & (SIGNATURE_KEY_PAIR_FLAG | DECRYPTION_KEY_PAIR_FLAG)) ==
                            SIGNATURE_KEY_PAIR_FLAG)) {
                    ptr->kid = 0x20 | (i + 1);
                    ptr->flags |= RSA_KEY_PAIR_FLAG;
                  }

                  // RSA 2048-bits exchange keys
                  else if ((ptr->size_in_bits == 0x800) &&
                           ((ptr->flags & (SIGNATURE_KEY_PAIR_FLAG | DECRYPTION_KEY_PAIR_FLAG)) ==
                            (SIGNATURE_KEY_PAIR_FLAG | DECRYPTION_KEY_PAIR_FLAG))) {
                    ptr->kid = 0x30 | (i + 1);
                    ptr->flags |= RSA_KEY_PAIR_FLAG;
                  }

                  // RSA 2048-bits signature keys
                  else if ((ptr->size_in_bits == 0x800) &&
                           ((ptr->flags & (SIGNATURE_KEY_PAIR_FLAG | DECRYPTION_KEY_PAIR_FLAG)) ==
                            SIGNATURE_KEY_PAIR_FLAG)) {
                    ptr->kid = 0x40 | (i + 1);
                    ptr->flags |= RSA_KEY_PAIR_FLAG;
                  }

                  ptr->has_cert = false;
                }
              } else {
                break;
              }
            }
          }
          ++_keypairs_num;

          if (transmit(0x00, SCIns::Select, SCP1::SELECTByPathFromMF, SCP2::SELECTFCPTemplate | SCP2::SELECTFirstOrOnly,
                       FILE_DIR_EF, sizeof(FILE_DIR_EF), 0x15)) {
            if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
              if (transmit(0x00, SCIns::ReadBinary, 0x00, 0x00, 0x01)) {
                if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
                  size = _seiface->_apduResponse[0];

                  for (i = 0, offset = 1; i < size; i++, offset += 0x15) {
                    if (transmit(0x00, SCIns::ReadBinary, (offset >> 8) & 0xFF, offset & 0xFF, 0x15)) {
                      if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
                        /* We are interested in files under "mcsp" directory with names following the patterns
                         * "kxc??" (decryption keys) and "ksc??" (signature keys).
                         * ?? is a decimal number for key ID, which is in turn the index in CONTAINER_INFO counting from
                         * 1 */

                        if ((_seiface->_apduResponse[12] == 'm') && (_seiface->_apduResponse[13] == 's') &&
                            (_seiface->_apduResponse[14] == 'c') && (_seiface->_apduResponse[15] == 'p')) {
                          if ((_seiface->_apduResponse[4] == 'k') && (_seiface->_apduResponse[5] == 'x') &&
                              (_seiface->_apduResponse[6] == 'c')) {
                            for (int j = 0; j < _keypairs_num; ++j) {
                              ptr = &_keypairs[j];
                              if (((ptr->kid & 0x0F) - 1) ==
                                  (((_seiface->_apduResponse[7] - '0') * 10) + (_seiface->_apduResponse[8] - '0'))) {
                                ptr->pub_file_id[0] = _seiface->_apduResponse[0];
                                ptr->pub_file_id[1] = _seiface->_apduResponse[1];
                                ptr->has_cert       = true;
                                break;
                              }
                            }
                          }
                          // ksc file is for signature keys
                          else if ((_seiface->_apduResponse[4] == 'k') && (_seiface->_apduResponse[5] == 's') &&
                                   (_seiface->_apduResponse[6] == 'c')) {
                            for (int j = 0; j < _keypairs_num; ++j) {
                              ptr = &_keypairs[j];
                              if (((ptr->kid & 0x0F) - 1) ==
                                  (((_seiface->_apduResponse[7] - '0') * 10) + (_seiface->_apduResponse[8] - '0'))) {
                                ptr->pub_file_id[0] = _seiface->_apduResponse[0];
                                ptr->pub_file_id[1] = _seiface->_apduResponse[1];
                                ptr->has_cert       = true;
                                break;
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
          return true;
        }
      }
    }
  }

  return false;
}

bool MIAS::getKeyPairByContainerId(uint8_t containter_id, mias_key_pair_t** kp) {
  listKeyPairs();


  for (int i = 0; i < _keypairs_num; i++) {
    if (((_keypairs[i].kid & 0x0F) - 1) == containter_id) {
      *kp = &_keypairs[i];
      return true;
    }
  }

  return false;
}

bool MIAS::getCertificateByContainerId(uint8_t container_id, uint8_t* cert, uint16_t* certLen) {
  uint8_t i;
  uint8_t len;
  SCTag t;
  uint8_t l;
  uint16_t offset;
  uint16_t ef_size;
  mias_key_pair_t* kp;

  if (getKeyPairByContainerId(container_id, &kp)) {
    if (kp->has_cert) {
      if (transmit(0x00, SCIns::Select, SCP1::SELECTByPathFromMF, SCP2::SELECTFCPTemplate | SCP2::SELECTFirstOrOnly,
                   kp->pub_file_id, sizeof(kp->pub_file_id), 0x1C)) {
        if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
          if (_seiface->_apduResponse[0] == SCTag::FileControlInfoFCP) {
            len = _seiface->_apduResponse[1];

            for (i = 2, len += 2; i < len;) {
              t = static_cast<SCTag>(_seiface->_apduResponse[i]);
              l = _seiface->_apduResponse[i + 1];

              switch (t) {
                case SCTag::FCPFileSizeWithInfo:
                  ef_size = (_seiface->_apduResponse[i + 2] << 8) | _seiface->_apduResponse[i + 3];
                  break;
              }

              i += 2 + l;
            }
          }

          if (ef_size) {
            *certLen = ef_size;
            if (cert == NULL) {
              return true;
            }

            for (offset = 0; offset < ef_size;) {
              len = 0xEE;
              if ((offset + len) > ef_size) {
                len = ef_size - offset;
              }

              if (transmit(0x00, SCIns::ReadBinary, (offset >> 8) & 0xFF, offset & 0xFF, len)) {
                if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
                  len = getResponse(&(cert[offset]));
                }
              }

              offset += len;
            }

            if ((cert[0] == 0x01) && (cert[1] == 0x00)) {
              // Compressed
              // ToDo: uncompressed using zlib
              // printf("ERROR: Compressed certificate!");
              return false;
            }

            return true;
          }
        }
      }
    }
  }

  return false;
}

/** PUBLIC ********************************************************************/

bool MIAS::verifyPin(uint8_t* pin, uint16_t pinLen) {
  if (transmit(0x00, SCIns::Verify, SCP1::VERIFYReserved, SCP2::BasicSecurityDFKey | 0x01, pin, pinLen)) {
    if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
      return true;
    }
  }
  return false;
}

bool MIAS::changePin(uint8_t* oldPin, uint16_t oldPinLen, uint8_t* newPin, uint16_t newPinLen) {
  uint8_t data[1 + (8 * 2)];
  uint16_t dataLen;

  dataLen = 1 + oldPinLen + newPinLen;

  if (dataLen > sizeof(data)) {
    return false;
  }

  data[0] = (uint8_t)oldPinLen;
  memcpy(&data[1], oldPin, oldPinLen);
  memcpy(&data[oldPinLen], newPin, newPinLen);

  if (transmit(0x80, SCIns::ChangeReferenceData, SCP1::CHANGEREFERENCEDATAOldAndNew, SCP2::BasicSecurityDFKey | 0x01,
               data, dataLen)) {
    if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
      return true;
    }
  }
  return false;
}

bool MIAS::p11GetObjectByLabel(uint8_t* label, uint16_t labelLen, uint8_t* object, uint16_t* objectLen) {
  uint8_t record[0x15];
  uint16_t i, offset, len, size, trimPos, trimLen;
  mias_file_t* nfile = NULL;

  *objectLen = 0;
  _files_num = 0;

  if (transmit(0x00, SCIns::Select, SCP1::SELECTByPathFromMF, SCP2::SELECTFCPTemplate | SCP2::SELECTFirstOrOnly,
               FILE_DIR_EF, sizeof(FILE_DIR_EF))) {
    if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
      uint8_t nbOfFiles;

      if (transmit(0x00, SCIns::ReadBinary, 0x00, 0x00, 0x01)) {
        if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
          if (getResponse(&nbOfFiles) == 1) {
            for (i = 0; i < nbOfFiles; i++) {
              offset = 1 + (i * 0x15);

              if (transmit(0x00, SCIns::ReadBinary, offset >> 8, offset, 0x15)) {
                if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
                  if (getResponse(record) == 0x15) {
                    nfile       = &_files[_files_num];
                    nfile->efid = (record[0] << 8) | record[1];
                    nfile->size = (record[2] << 8) | record[3];
                    memcpy(nfile->dir, &record[12], 8);
                    memcpy(nfile->name, &record[4], 8);
                    ++_files_num;
                  }
                }
              }
            }

            for (int j = 0; j < _files_num; ++j) {
              if ((strcmp((const char*)_files[j].dir, "p11") == 0) &&
                  ((memcmp((const char*)_files[j].name, "pubdat", 6) == 0) ||
                   ((memcmp((const char*)_files[j].name, "pridat", 6) == 0)))) {
                uint8_t file_id[2];
                file_id[0] = _files[j].efid >> 8;
                file_id[1] = _files[j].efid;

                if (transmit(0x00, SCIns::Select, SCP1::SELECTByPathFromMF,
                             SCP2::SELECTFCPTemplate | SCP2::SELECTFirstOrOnly, file_id, sizeof(file_id))) {
                  if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
                    offset = 16;

                    // CKO_DATA file, contains L-V pairs in the following order:
                    // label, CKA_APPLICATION, CKA_OBJECT_ID, CKA_VALUE
                    if (transmit(0x00, SCIns::ReadBinary, offset >> 8, offset, 0x01)) {
                      if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
                        if (getResponse(record) == 0x01) {
                          offset++;
                          if (transmit(0x00, SCIns::ReadBinary, offset >> 8, offset, record[0])) {
                            if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
                              if (getResponseLength() == record[0]) {
                                getResponse(record);
                                record[getResponseLength()] = '\0';
                                // Ignore padding spaces from record on SIM
                                trimPos = getResponseLength() - 1;
                                while (trimPos >= 0 && record[trimPos] == ' ') {
                                  trimPos--;
                                }
                                trimLen = trimPos + 1;
                                // Check if we have a record that matches the label
                                if ((labelLen == trimLen) && (memcmp((void*)label, (void*)record, labelLen) == 0)) {
                                  offset += getResponseLength();

                                  // Skip CKA_APPLICATION
                                  if (transmit(0x00, SCIns::ReadBinary, offset >> 8, offset, 0x01)) {
                                    if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
                                      if (getResponse(record) == 0x01) {
                                        offset += 1 + record[0];

                                        // Skip CKA_OBJECT_ID
                                        if (transmit(0x00, SCIns::ReadBinary, offset >> 8, offset, 0x01)) {
                                          if (getStatusWord() ==
                                              (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
                                            if (getResponse(record) == 0x01) {
                                              offset += 1 + record[0];

                                              // Read CKA_VALUE
                                              if (transmit(0x00, SCIns::ReadBinary, offset >> 8, offset, 0x05)) {
                                                if (getStatusWord() ==
                                                    (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
                                                  if (getResponse(record) == 0x05) {
                                                    if (record[0] < 0x80) {
                                                      size = record[0];
                                                    } else {
                                                      for (i = 0; i < (record[0] & 0x0F); i++, offset++) {
                                                        size <<= 8;
                                                        size |= record[1 + i];
                                                      }
                                                    }

                                                    offset++;

                                                    *objectLen = size;
                                                    if (object == NULL) {
                                                      return true;
                                                    }

                                                    for (i = 0; i < size;) {
                                                      len = 0xEE;
                                                      if ((i + len) > size) {
                                                        len = size - i;
                                                      }

                                                      if (transmit(0x00, SCIns::ReadBinary, offset >> 8, offset, len)) {
                                                        if (getStatusWord() ==
                                                            (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
                                                          len = getResponse(&(object[i]));
                                                        }
                                                      }

                                                      offset += len;
                                                      i += len;
                                                    }

                                                    return true;
                                                  }
                                                }
                                              }
                                            }
                                          }
                                        }
                                      }
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  return false;
}

bool MIAS::hashInit(uint8_t algorithm) {
  _hashAlgo = algorithm;
  return mseSetBeforeHash(_hashAlgo);
}

bool MIAS::hashUpdate(const uint8_t* data, uint16_t dataLen) {
  return psoHashInternally(_hashAlgo, data, dataLen);
}

bool MIAS::hashFinal(uint8_t* hash, uint16_t* hashLen) {
  return psoHashInternallyFinal(hash, hashLen);
}

bool MIAS::signInit(uint8_t algorithm, uint8_t key) {
  _signAlgo = algorithm;
  _signKey  = key;
  return mseSetBeforeSignature(_signAlgo, _signKey);
}

bool MIAS::signFinal(const uint8_t* hash, uint16_t hashLen, uint8_t* signature, uint16_t* signatureLen) {
  if (psoHashExternally(_signAlgo & 0xF0, hash, hashLen)) {
    if (psoComputeDigitalSignature(signature, signatureLen)) {
      return true;
    }
  }
  return false;
}

bool MIAS::decryptInit(uint8_t algorithm, uint8_t key) {
  _decryptAlgo = algorithm;
  _decryptKey  = key;
  return mseSetBeforeDecrypt(_decryptAlgo, _decryptKey);
}

bool MIAS::decryptFinal(const uint8_t* data, uint16_t dataLen, uint8_t* plain, uint16_t* plainLen) {
  return psoDecipher(data, dataLen, plain, plainLen);
}

/** C Accessors	***************************************************************/

extern "C" MIAS* MIAS_create(void) {
  return new MIAS();
}

extern "C" void MIAS_destroy(MIAS* mias) {
  delete mias;
}

extern "C" bool MIAS_verify_pin(MIAS* mias, uint8_t* pin, uint16_t pin_len) {
  return mias->verifyPin(pin, pin_len);
}

extern "C" bool MIAS_change_pin(MIAS* mias, uint8_t* old_pin, uint16_t old_pin_len, uint8_t* new_pin,
                                uint16_t new_pin_len) {
  return mias->changePin(old_pin, old_pin_len, new_pin, new_pin_len);
}

extern "C" bool MIAS_get_key_pair_by_container_id(MIAS* mias, uint8_t container_id, mias_key_pair_t** kp) {
  return mias->getKeyPairByContainerId(container_id, kp);
}

extern "C" bool MIAS_get_certificate_by_container_id(MIAS* mias, uint8_t container_id, uint8_t* cert,
                                                     uint16_t* cert_len) {
  return mias->getCertificateByContainerId(container_id, cert, cert_len);
}

extern "C" bool MIAS_p11_get_object_by_label(MIAS* mias, uint8_t* label, uint16_t label_len, uint8_t* object,
                                             uint16_t* object_len) {
  return mias->p11GetObjectByLabel(label, label_len, object, object_len);
}

extern "C" bool MIAS_hash_init(MIAS* mias, uint8_t algorithm) {
  return mias->hashInit(algorithm);
}

extern "C" bool MIAS_hash_update(MIAS* mias, uint8_t* data, uint16_t data_len) {
  return mias->hashUpdate(data, data_len);
}

extern "C" bool MIAS_hash_final(MIAS* mias, uint8_t* hash, uint16_t* hash_len) {
  return mias->hashFinal(hash, hash_len);
}

extern "C" bool MIAS_sign_init(MIAS* mias, uint8_t algorithm, uint8_t key) {
  return mias->signInit(algorithm, key);
}

extern "C" bool MIAS_sign_final(MIAS* mias, uint8_t* hash, uint16_t hash_len, uint8_t* signature,
                                uint16_t* signature_len) {
  return mias->signFinal(hash, hash_len, signature, signature_len);
}

extern "C" bool MIAS_decrypt_init(MIAS* mias, uint8_t algorithm, uint8_t key) {
  return mias->decryptInit(algorithm, key);
}

extern "C" bool MIAS_decrypt_final(MIAS* mias, uint8_t* data, uint16_t data_len, uint8_t* plain, uint16_t* plain_len) {
  return mias->decryptFinal(data, data_len, plain, plain_len);
}
