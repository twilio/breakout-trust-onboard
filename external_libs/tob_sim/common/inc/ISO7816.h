/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#ifndef __ISO7816_H__
#define __ISO7816_H__

#ifdef __cplusplus
/* Instruction codes with P1, P2 and tag values defined in ISO/IEC 7816*/

enum class SCIns : uint8_t {
  Verify                    = 0x20,
  ManageSecurityEnvironment = 0x22,
  ChangeReferenceData       = 0x24,
  PerformSecurityOperation  = 0x2A,
  ManageChannel             = 0x70,
  Select                    = 0xA4,
  ReadBinary                = 0xB0,
  Envelope                  = 0xC2,
};

enum class SCP1 : uint8_t {
  VERIFYReserved               = 0x00,
  MSEMessagingCommand          = 0x10,
  MSEMessagingResponse         = 0x20,
  MSECompDecInt                = 0x40,
  MSEVerEncExt                 = 0x80,
  MSESet                       = 0x01,
  MSEStore                     = 0xF2,
  MSERestore                   = 0xF3,
  MSEErase                     = 0xF4,
  CHANGEREFERENCEDATAOldAndNew = 0x00,
  CHANGEREFERENCEDATAOnlyNew   = 0x01,
  PSOPlain                     = 0x80,
  PSOPadding                   = 0x86,
  PSOHashCode                  = 0x90,
  PSOSignature                 = 0x9E,
  MANAGECHANNELOpen            = 0x00,
  MANAGECHANNELClose           = 0x80,
  SELECTByDFName               = 0x04,
  SELECTByPathFromMF           = 0x08,
  SELECTByPathFromCurrent      = 0x09,
  ENVELOPEReserved             = 0x00,
};

enum class SCP2 : uint8_t {
  BasicSecurityMFKey           = 0x00,
  BasicSecurityDFKey           = 0x80,
  SELECTFirstOrOnly            = 0x00,
  SELECTLast                   = 0x01,
  SELECTNext                   = 0x02,
  SELECTPrevious               = 0x03,
  SELECTFCITemplate            = 0x00,
  SELECTFCPTemplate            = 0x04,
  SELECTFMDTemplate            = 0x08,
  SELECTProprietary            = 0x0C,
  MSETemplateHashCode          = 0xAA,
  MSETemplateSignature         = 0xB6,
  MSETemplateConfidentiality   = 0xB8,
  PSOPlain                     = 0x80,
  PSOPadding                   = 0x86,
  PSOSignatureInput            = 0x9A,
  PSOTemplateHash              = 0xA0,
  MANAGECHANNELAllocateChannel = 0x00,
  ENVELOPEReserved             = 0x00,
};

enum class SCTag : uint8_t {
  FileControlInfoFCP = 0x62,
  FileControlInfoFMD = 0x64,
  FileControlInfoFCI = 0x6F,

  FCPFileSizeWithoutInfo       = 0x80,
  FCPFileSizeWithInfo          = 0x81,
  FCPFileDescriptorByte        = 0x82,
  FCPFileID                    = 0x83,
  FCPLifeCycleStatus           = 0x8A,
  FCPSecurityAttributeExpanded = 0x8B,
  FCPSecurityAttributeCompact  = 0x8C,

  PSOPaddingProprietary1 = 0x81,
  PSOHashInt             = 0x80,
  PSOHashExt             = 0x90,

  MSEAlgReference = 0x80,
  MSEPublicKey    = 0x84,
  MSEPrivateKey   = 0x92,
};

enum class SCSW1 : uint16_t {
  OKNoQualification = 0x9000,
  OKLengthInSW2     = 0x6100,
};

enum class SCSW2 : uint8_t { OKNoQualification = 0x00 };

/* Common operators to keep casting directive proliferation under control */
static inline SCIns operator|(SCIns ins, uint8_t channel) {
  return static_cast<SCIns>(static_cast<uint8_t>(ins) | channel);
}

static inline uint16_t operator|(SCSW1 sw1, SCSW2 sw2) {
  return static_cast<uint16_t>(sw1) | static_cast<uint8_t>(sw2);
}

static inline SCP1 operator|(SCP1 l, SCP1 r) {
  return static_cast<SCP1>(static_cast<uint8_t>(l) | static_cast<uint8_t>(r));
}

static inline SCP2 operator|(SCP2 l, SCP2 r) {
  return static_cast<SCP2>(static_cast<uint8_t>(l) | static_cast<uint8_t>(r));
}

static inline SCP2 operator|(SCP2 l, int r) {
  return static_cast<SCP2>(static_cast<uint8_t>(l) | r);
}

static inline bool operator==(int l, SCSW1 r) {
  return l == static_cast<int>(r);
}

static inline bool operator==(uint8_t l, SCTag r) {
  return l == static_cast<uint8_t>(r);
}

#endif  // __cplusplus

#endif  // __ISO7816_H__
