/*
 *  Copyright (c) 2017 Gemalto Limited. All Rights Reserved
 *  This software is the confidential and proprietary information of GEMALTO.
 *  
 *  GEMALTO MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF 
 *  THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 *  TO THE IMPLIED WARRANTIES OR MERCHANTABILITY, FITNESS FOR A
 *  PARTICULAR PURPOSE, OR NON-INFRINGEMENT. GEMALTO SHALL NOT BE
 *  LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS RESULT OF USING,
 *  MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.

 *  THIS SOFTWARE IS NOT DESIGNED OR INTENDED FOR USE OR RESALE AS ON-LINE
 *  CONTROL EQUIPMENT IN HAZARDOUS ENVIRONMENTS REQUIRING FAIL-SAFE
 *  PERFORMANCE, SUCH AS IN THE OPERATION OF NUCLEAR FACILITIES, AIRCRAFT
 *  NAVIGATION OR COMMUNICATION SYSTEMS, AIR TRAFFIC CONTROL, DIRECT LIFE
 *  SUPPORT MACHINES, OR WEAPONS SYSTEMS, IN WHICH THE FAILURE OF THE
 *  SOFTWARE COULD LEAD DIRECTLY TO DEATH, PERSONAL INJURY, OR SEVERE
 *  PHYSICAL OR ENVIRONMENTAL DAMAGE ("HIGH RISK ACTIVITIES"). GEMALTO
 *  SPECIFICALLY DISCLAIMS ANY EXPRESS OR IMPLIED WARRANTY OF FTNESS FOR
 *  HIGH RISK ACTIVITIES;
 *
 */

#include "MIAS.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//#define USE_GAT_RESPONSE

static uint8_t AID[] = { 0xA0, 0x00, 0x00, 0x00, 0x18, 0x80, 0x00, 0x00, 0x00, 0x06, 0x62, 0x41, 0x51 };

MIAS::MIAS(void) : Applet(AID, sizeof(AID)) {
	_keypairs = NULL;
	
	_hashAlgo = 0;

	_signAlgo = 0;
	_signKey = 0;

	_decryptKey = 0;
	_decryptAlgo = 0;
}
  
MIAS::~MIAS(void) {
	void* ptr;
	
	while(_keypairs != NULL) {
		ptr = _keypairs->next;
		free(_keypairs);
		_keypairs = (mias_key_pair_t*) ptr;
	}
}

/** PRIVATE *******************************************************************/

bool MIAS::mseSetBeforeHash(uint8_t algorithm) {
	uint8_t data[3];
		
	data[0] = 0x80;
	data[1] = 0x01;
	data[2] = algorithm;
	
	if(transmit(0x00, 0x22, 0x41, 0xAA, data, sizeof(data))) {
		if(getStatusWord() == 0x9000) {
			return true;
		}
	}
	
	return false;
}

bool MIAS::psoHashInternally(uint8_t algorithm, uint8_t* data, uint16_t dataLen) {
	uint8_t block_size;
	uint16_t i, l;
	
	if((algorithm == ALGO_SHA1) || (algorithm == ALGO_SHA224) || (algorithm == ALGO_SHA256)) {
		block_size = 64;
	}
	else if((algorithm == ALGO_SHA384) || (algorithm == ALGO_SHA512)) {
		block_size = 128;
	}
	else {
		return false;
	}
	
	for(i=0; i<dataLen; i += l) {
		if((l = dataLen - i) > block_size) {
			l = block_size;
		}

		if(!transmit(0x00, 0x2A, 0x90, 0x80, &data[i], l)) {
			return false;
		}
			
		if(getStatusWord() != 0x9000) {
			return false;
		}
	}
	
	return true;
}

bool MIAS::psoHashInternallyFinal(uint8_t* hash, uint16_t* hashLen) {
	uint8_t data[2];
	
	data[0] = 0x80;
	data[1] = 0x00;
	
	*hashLen = 0;
	
	if(transmit(0x00, 0x2A, 0x90, 0xA0, data, sizeof(data), 0x00)) {
		if(getStatusWord() == 0x9000) {
			*hashLen = getResponse(hash);
			return true;
		}
	}
	
	return false;
}

bool MIAS::psoHashExternally(uint8_t algorithm, uint8_t* hash, uint16_t hashLen) {
	uint8_t data[66];
	
	data[0] = 0x90;
	switch(algorithm) {
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
	
	if(transmit(0x00, 0x2A, 0x90, 0xA0, data, 2 + data[1], 0x00)) {
		if(getStatusWord() == 0x9000) {
			return true;
		}
	}
	
	return false;
}

bool MIAS::mseSetBeforeSignature(uint8_t algorithm, uint8_t key) {
	uint8_t data[6];
	
	data[0] = 0x80;
	data[1] = 0x01;
	data[2] = algorithm;
	data[3] = 0x84;
	data[4] = 0x01;
	data[5] = key;
	
	if(transmit(0x00, 0x22, 0x41, 0xB6, data, sizeof(data))) {
		if(getStatusWord() == 0x9000) {
			return true;
		}
	}
	
	return false;
}

bool MIAS::psoComputeDigitalSignature(uint8_t* signature, uint16_t* signatureLen) {
	uint16_t len;
	
	*signatureLen = 0;
	
	if(transmit(0x00, 0x2A, 0x9E, 0x9A, 0x00)) {
		#ifdef USE_GAT_RESPONSE
		if(_isBasic) {
		#endif
			while(getResponseLength()) {
				len = getResponse(signature);
				signature += len;
				*signatureLen += len;
				if((getStatusWord() & 0xFF00) == 0x6100) {
					// Transmit GET Response
					if(transmit(0x00, 0xC0, 0x00, 0x00, getStatusWord() & 0x00FF)) {
						continue;
					}
				}
				else if(getStatusWord() == 0x9000) {
					return true;
				}
				return false;
			}
		#ifdef USE_GAT_RESPONSE
		}
		else {
			// GAT format: [DATA1 DATA2 ... ][GAT SW1 SW2][90 00]
			while((getStatusWord() == 0x9000) && (_seiface->_apduResponseLen >= 4)) {
				// Convert ApduResponse in order to have GAT status word as regular status word
				_seiface->_apduResponseLen -= 2;
				
				len = getResponse(signature);
				signature += len;
				*signatureLen += len;
				if((getStatusWord() & 0xFF00) == 0x6100) {
					// Transmit GAT Response
					if(transmit(0x00, 0xC2, 0x00, 0x00, getStatusWord() & 0x00FF)) {
						continue;
					}
				}
				else if(getStatusWord() == 0x9000) {
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
	
	data[0] = 0x80;
	data[1] = 0x01;
	data[2] = algorithm;
	data[3] = 0x84;
	data[4] = 0x01;
	data[5] = key;
	
	if(transmit(0x00, 0x22, 0x41, 0xB8, data, sizeof(data))) {
		if(getStatusWord() == 0x9000) {
			return true;
		}
	}
	
	return false;
}

bool MIAS::psoDecipher(uint8_t* data, uint16_t dataLen, uint8_t* plain, uint16_t* plainLen) {	
	uint8_t buf[255];
	
	if(dataLen < 255) {
		buf[0] = 0x81;
		memcpy(&buf[1], data, dataLen);
		
		if(!transmit(0x00, 0x2A, 0x80, 0x86, buf, dataLen + 1)) {
			return false;
		}
	}
	else {
		buf[0] = 0x81;
		memcpy(&buf[1], data, 0xFE);
		data += 0xFE;
		dataLen -= 0xFE;
		
		if(!transmit(0x10, 0x2A, 0x80, 0x86, buf, 0xFF)) {
			return false;
		}
		
		if(getStatusWord() != 0x9000) {
			return false;
		}
		
		if(!transmit(0x00, 0x2A, 0x80, 0x86, data, dataLen)) {
			return false;
		}
	}
	
	if(getStatusWord() == 0x9000) {
		*plainLen = getResponse(plain);
		return true;
	}
	
	return false;
}

#define TAG_FILE_SIZE   0x81
#define TAG_FDB         0x82
#define TAG_FILE_ID     0x83
#define TAG_LCS         0x8A
#define TAG_CT_SEC_ATTR 0x8C

bool MIAS::listKeyPairs(void) {
	uint8_t CONTAINERS_INFO_EF[] = { 0x00, 0x02 };
	uint8_t FILE_DIR_EF[] = { 0x01, 0x01 };
	uint16_t size = 0;
	mias_key_pair_t* ptr;
	
	if(_keypairs != NULL) {
		return true;
	}
	
	if(transmit(0x00, 0xA4, 0x08, 0x04, CONTAINERS_INFO_EF, sizeof(CONTAINERS_INFO_EF), 0x15)) {
		if(getStatusWord() == 0x9000) {
			if(getResponseLength() > 2) {
				if(_seiface->_apduResponse[0] == 0x62) {
					uint8_t i, len;
					uint8_t t, l;
					uint16_t offset;
					
					len = _seiface->_apduResponse[1];
					
					for(i = 2, len += 2; i < len;) {
						t = _seiface->_apduResponse[i];
						l = _seiface->_apduResponse[i + 1];

						switch(t) {
							case TAG_FILE_SIZE:
								size = (_seiface->_apduResponse[i + 2] << 8) | _seiface->_apduResponse[i + 3];
								break;
						}
						
						i += 2 + l;
					}
					
					for(i = 0, offset = 0; offset < size; i++, offset += 0x0B) {
						if(transmit(0x00, 0xB0, (offset >> 8) & 0xFF, offset & 0xFF, 0x0B)) {
							if(getStatusWord() == 0x9000) {
								if(_seiface->_apduResponse[0]) {	
									ptr = (mias_key_pair_t*) malloc(sizeof(mias_key_pair_t));
									ptr->pub_file_id[0] = 0;
									ptr->pub_file_id[1] = 0;
									
									ptr->flags = SIGNATURE_KEY_PAIR_FLAG;									
									if((ptr->size_in_bits = (_seiface->_apduResponse[4] << 8) | _seiface->_apduResponse[5]) == 0) {
										ptr->flags |= DECRYPTION_KEY_PAIR_FLAG;
										ptr->size_in_bits = (_seiface->_apduResponse[6] << 8) | _seiface->_apduResponse[7];
									}
									
									// RSA 1024-bits exchange keys
									if((ptr->size_in_bits == 0x400) && ((ptr->flags & (SIGNATURE_KEY_PAIR_FLAG | DECRYPTION_KEY_PAIR_FLAG)) ==  (SIGNATURE_KEY_PAIR_FLAG | DECRYPTION_KEY_PAIR_FLAG))) {
										ptr->kid = 0x10 | (i + 1);
										ptr->flags |= RSA_KEY_PAIR_FLAG;
									}
									
									// RSA 1024-bits signature keys
									else if((ptr->size_in_bits == 0x400) && ((ptr->flags & (SIGNATURE_KEY_PAIR_FLAG | DECRYPTION_KEY_PAIR_FLAG)) ==  SIGNATURE_KEY_PAIR_FLAG)) {
										ptr->kid = 0x20 | (i + 1);
										ptr->flags |= RSA_KEY_PAIR_FLAG;
									}
									
									// RSA 2048-bits exchange keys
									else if((ptr->size_in_bits == 0x800) && ((ptr->flags & (SIGNATURE_KEY_PAIR_FLAG | DECRYPTION_KEY_PAIR_FLAG)) ==  (SIGNATURE_KEY_PAIR_FLAG | DECRYPTION_KEY_PAIR_FLAG))) {
										ptr->kid = 0x30 | (i + 1);
										ptr->flags |= RSA_KEY_PAIR_FLAG;
									}
									
									// RSA 2048-bits signature keys
									else if((ptr->size_in_bits == 0x800) && ((ptr->flags & (SIGNATURE_KEY_PAIR_FLAG | DECRYPTION_KEY_PAIR_FLAG)) ==  SIGNATURE_KEY_PAIR_FLAG)) {
										ptr->kid = 0x40 | (i + 1);
										ptr->flags |= RSA_KEY_PAIR_FLAG;
									}
									
									ptr->has_cert = false;
									
									ptr->next = _keypairs;
									_keypairs = ptr;
								}
							}
							else {
								break;
							}
						}
					}
									
					if(transmit(0x00, 0xA4, 0x08, 0x04, FILE_DIR_EF, sizeof(FILE_DIR_EF), 0x15)) {
						if(getStatusWord() == 0x9000) {
							if(transmit(0x00, 0xB0, 0x00, 0x00, 0x01)) {
								if(getStatusWord() == 0x9000) {
									size = _seiface->_apduResponse[0];
									
									for(i = 0, offset = 1; i < size; i++, offset += 0x15) {
										if(transmit(0x00, 0xB0, (offset >> 8) & 0xFF, offset & 0xFF, 0x15)) {
											if(getStatusWord() == 0x9000) {
												if((_seiface->_apduResponse[12] == 'm') && (_seiface->_apduResponse[13] == 's') && (_seiface->_apduResponse[14] == 'c') && (_seiface->_apduResponse[15] == 'p')) {
													/*
													{
														uint16_t j;
														
														printf("\r\nFile: ");
														for(j=4; (j<12) && _seiface->_apduResponse[j]; j++) {
															printf("%c", _seiface->_apduResponse[j]);
														}
														printf(" (%02d%02d) size = %dbytes\r\n\r\n", _seiface->_apduResponse[0], _seiface->_apduResponse[1], (_seiface->_apduResponse[2] << 8) | _seiface->_apduResponse[3]);
													}
													*/
													
													// priprk is for key pair only (no certificate)
													/*
													if((_seiface->_apduResponse[4] == 'p') && (_seiface->_apduResponse[5] == 'r') && (_seiface->_apduResponse[6] == 'i') && 
														(_seiface->_apduResponse[7] == 'p') && (_seiface->_apduResponse[8] == 'r') && (_seiface->_apduResponse[9] == 'k')) {

														ptr = _keypairs;
													
														while(ptr != NULL) {
															if(((ptr->kid & 0x0F) - 1) == (((_seiface->_apduResponse[10] - '0') * 10) + (_seiface->_apduResponse[11] - '0'))) {
																ptr->pub_file_id[0] = _seiface->_apduResponse[0];
																ptr->pub_file_id[1] = _seiface->_apduResponse[1];
																ptr->has_cert = false;
																break;
															}
															ptr = ptr->next;
														}
													}
													*/

													// kxc file is for exchange keys
													/*else*/ if((_seiface->_apduResponse[4] == 'k') && (_seiface->_apduResponse[5] == 'x') && (_seiface->_apduResponse[6] == 'c')) {
														ptr = _keypairs;
													
														while(ptr != NULL) {
															if(((ptr->kid & 0x0F) - 1) == (((_seiface->_apduResponse[7] - '0') * 10) + (_seiface->_apduResponse[8] - '0'))) {
																ptr->pub_file_id[0] = _seiface->_apduResponse[0];
																ptr->pub_file_id[1] = _seiface->_apduResponse[1];
																ptr->has_cert = true;
																break;
															}
															ptr = ptr->next;
														}
													}
													// ksc file is for signature keys
													else if((_seiface->_apduResponse[4] == 'k') && (_seiface->_apduResponse[5] == 's') && (_seiface->_apduResponse[6] == 'c')) {
														ptr = _keypairs;
													
														while(ptr != NULL) {
															if(((ptr->kid & 0x0F) - 1) == (((_seiface->_apduResponse[7] - '0') * 10) + (_seiface->_apduResponse[8] - '0'))) {
																ptr->pub_file_id[0] = _seiface->_apduResponse[0];
																ptr->pub_file_id[1] = _seiface->_apduResponse[1];
																ptr->has_cert = true;
																break;
															}
															ptr = ptr->next;
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
					
					// DEBUG
					/*
					{
						ptr = _keypairs;
											
						printf("\r\n---\r\n");
						while(ptr != NULL) {
							printf("KID: %02X %d\r\n", ptr->kid,  ptr->has_cert);
							ptr = ptr->next;
						}
						printf("---\r\n\r\n");
					}
					*/
					// -----
					return true;
				}
			}
		}
	}
	
	return false;
}

bool MIAS::getKeyPairByContainerId(uint8_t containter_id, mias_key_pair_t** kp) {												
	listKeyPairs();
	
	*kp = _keypairs;
	
	while(*kp != NULL) {
		if((((*kp)->kid & 0x0F) - 1) == containter_id) {
			return true;
		}
		*kp = (*kp)->next;
	}
	
	return false;
}

bool MIAS::getCertificateByContainerId(uint8_t container_id, uint8_t** cert, uint16_t* certLen) {
	uint8_t i, len;
	uint8_t t, l;
	uint16_t offset;
	uint16_t ef_size;
	mias_key_pair_t* kp;
	
	if(getKeyPairByContainerId(container_id, &kp)) {
		if(kp->has_cert) {
			if(transmit(0x00, 0xA4, 0x08, 0x04, kp->pub_file_id, sizeof(kp->pub_file_id), 0x1C)) {
				if(getStatusWord() == 0x9000) {
					if(_seiface->_apduResponse[0] == 0x62) {
						len = _seiface->_apduResponse[1];
						
						for(i = 2, len += 2; i < len;) {
							t = _seiface->_apduResponse[i];
							l = _seiface->_apduResponse[i + 1];

							switch(t) {
								case TAG_FILE_SIZE:
									ef_size = (_seiface->_apduResponse[i + 2] << 8) | _seiface->_apduResponse[i + 3];
									break;
							}
							
							i += 2 + l;
						}
					}
					
					if(ef_size) {
						*certLen = ef_size + 1;
						*cert = (uint8_t*) malloc((*certLen) * sizeof(uint8_t));
						
						for(offset = 0; offset < ef_size;) {
							len = 0xEE;
							if((offset + len) > ef_size) {
								len = ef_size - offset;
							}
							
							if(transmit(0x00, 0xB0, (offset >> 8) & 0xFF, offset & 0xFF, len)) {
								if(getStatusWord() == 0x9000) {
									len = getResponse(&((*cert)[offset]));
								}
							}
							
							offset += len;
						}
						(*cert)[offset] = '\0';
						
						if(((*cert)[0] == 0x01) && ((*cert)[1] == 0x00)) {
							// Compressed
							// ToDo: uncompressed using zlib
							//printf("ERROR: Compressed certificate!");
							free(*cert);
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
	if(transmit(0x00, 0x20, 0x00, 0x81, pin, pinLen)) {
		if(getStatusWord() == 0x9000) {
			return true;
		}
	}
	return false;
}

bool MIAS::changePin(uint8_t* oldPin, uint16_t oldPinLen, uint8_t* newPin, uint16_t newPinLen) {
	uint8_t data[1 + (8 * 2)];
	uint16_t dataLen;
	
	dataLen = 1 + oldPinLen + newPinLen;
	
	if(dataLen > sizeof(data)) {
		return false;
	}
	
	data[0] = (uint8_t) oldPinLen;
	memcpy(&data[1], oldPin, oldPinLen);
	memcpy(&data[oldPinLen], newPin, newPinLen);
	
	if(transmit(0x80, 0x24, 0x00, 0x81, data, dataLen)) {
		if(getStatusWord() == 0x9000) {
			return true;
		}
	}
	return false;
}

bool MIAS::p11GetObjectByLabel(uint8_t* label, uint16_t labelLen, uint8_t** object, uint16_t* objectLen) {
	uint8_t  data[2];
	uint8_t  record[0x15];
	uint16_t i, offset, len, size;
	mias_file_t* file = NULL;
	mias_file_t* nfile = NULL;

	*objectLen = 0;

	// FILE_DIR_EF
	data[0] = 0x01;
	data[1] = 0x01;

	if(transmit(0x00, 0xA4, 0x08, 0x04, data, sizeof(data))) {
		if(getStatusWord() == 0x9000) {
			uint8_t nbOfFiles;

			if(transmit(0x00, 0xB0, 0x00, 0x00, 0x01)) {
				if(getStatusWord() == 0x9000) {
					if(getResponse(&nbOfFiles) == 1) {

						for(i = 0; i < nbOfFiles; i++) {
							offset =  1 + (i * 0x15);

							if(transmit(0x00, 0xB0, offset >> 8, offset, 0x15)) {
								if(getStatusWord() == 0x9000) {
									if(getResponse(record) == 0x15) {
										nfile = (mias_file_t*) calloc(1, sizeof(mias_file_t));
										nfile->efid = (record[0] << 8) | record[1];
										nfile->size = (record[2] << 8) | record[3];
										memcpy(nfile->dir, &record[12], 8);
										memcpy(nfile->name, &record[4], 8);

										/*
										printf("dir: ");
										for(int ii=0; ii<8; ii++) {
											printf("%c", nfile->dir[ii]);
										}
										printf("    ");

										printf("name: ");
										for(int ii=0; ii<8; ii++) {
											printf("%c", nfile->name[ii]);
										}
										printf("    ");

										printf("efid: %04X\n", nfile->efid);
										fflush(stdout);
										*/
										nfile->next = file;
										file = nfile;
									}
								}
							}
						}

						while(file != NULL) {
							if((strcmp((const char*) file->dir, "p11") == 0) && ((memcmp((const char*) file->name, "pubdat", 6) == 0) || ((memcmp((const char*) file->name, "pridat", 6) == 0)))) {
								//printf("name: %s\n", file->name);
								//printf("efid: %04X\n", file->efid);

								data[0] = file->efid >> 8;
								data[1] = file->efid;

								if(transmit(0x00, 0xA4, 0x08, 0x04, data, sizeof(data))) {
									if(getStatusWord() == 0x9000) {

										offset = 16;

										if(transmit(0x00, 0xB0, offset >> 8, offset, 0x01)) {
											if(getStatusWord() == 0x9000) {
												if(getResponse(record) == 0x01) {
													offset++;
													if(transmit(0x00, 0xB0, offset >> 8, offset, record[0])) {
														if(getStatusWord() == 0x9000) {
															if(getResponseLength() == record[0]) {
																getResponse(record);
																record[getResponseLength()] = '\0';
																//printf("%s\n", record);
																if((labelLen == getResponseLength()) && (memcmp((void*) label, (void*) record, labelLen) == 0)) {

																	offset += labelLen;

																	// Skip CKA_APPLICATION
																	if(transmit(0x00, 0xB0, offset >> 8, offset, 0x01)) {
																		if(getStatusWord() == 0x9000) {
																			if(getResponse(record) == 0x01) {
																				offset += 1 + record[0];

																				// Skip CKA_OBJECT_ID
																				if(transmit(0x00, 0xB0, offset >> 8, offset, 0x01)) {
																					if(getStatusWord() == 0x9000) {
																						if(getResponse(record) == 0x01) {
																							offset += 1 + record[0];

																							// Read CKA_VALUE
																							if(transmit(0x00, 0xB0, offset >> 8, offset, 0x05)) {
																								if(getStatusWord() == 0x9000) {
																									if(getResponse(record) == 0x05) {
																										if(record[0] < 0x80) {
																											size = record[0];
																										}
																										else {
																											for(i=0; i<(record[0] & 0x0F); i++, offset++) {
																												size <<= 8;
																												size |= record[1 + i];
																											}
																										}

																										offset++;

																										//printf("CKA_VALUE size = %02X\n", size);

																										// Clear the files to free some space
																										while(file != NULL) {
																											nfile = file->next;
																											free(file);
																											file = nfile;
																										}
																										// ----

																										*objectLen = size + 1;
																										*object = (uint8_t*) malloc((*objectLen) * sizeof(uint8_t));
																										for(i=0; i<size;) {
																											len = 0xEE;
																											if((i + len) > size) {
																												len = size - i;
																											}

																											if(transmit(0x00, 0xB0, offset >> 8, offset, len)) {
																												if(getStatusWord() == 0x9000) {
																													len = getResponse(&(*object)[i]);
																												}
																											}

																											offset += len;
																											i += len;
																										}
																										(*object)[i] = '\0';

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
							nfile = file->next;
							free(file);
							file = nfile;
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

bool MIAS::hashUpdate(uint8_t* data, uint16_t dataLen) {
	return psoHashInternally(_hashAlgo, data, dataLen);
}

bool MIAS::hashFinal(uint8_t* hash, uint16_t* hashLen) {
	return psoHashInternallyFinal(hash, hashLen);
}

bool MIAS::signInit(uint8_t algorithm, uint8_t key) {
	_signAlgo = algorithm;
	_signKey = key;
	return mseSetBeforeSignature(_signAlgo, _signKey);
}

bool MIAS::signFinal(uint8_t* hash, uint16_t hashLen, uint8_t* signature, uint16_t* signatureLen) {
	if(psoHashExternally(_signAlgo & 0xF0, hash, hashLen)) {
		if(psoComputeDigitalSignature(signature, signatureLen)) {
			return true;
		}
	}	
	return false;
}

bool MIAS::decryptInit(uint8_t algorithm, uint8_t key) {
	_decryptAlgo = algorithm;
	_decryptKey = key;
	return mseSetBeforeDecrypt(_decryptAlgo, _decryptKey);
}

bool MIAS::decryptFinal(uint8_t* data, uint16_t dataLen, uint8_t* plain, uint16_t* plainLen) {
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

extern "C" bool MIAS_change_pin(MIAS* mias, uint8_t* old_pin, uint16_t old_pin_len, uint8_t* new_pin, uint16_t new_pin_len) {
	return mias->changePin(old_pin, old_pin_len, new_pin, new_pin_len);
}

extern "C" bool MIAS_get_key_pair_by_container_id(MIAS* mias, uint8_t container_id, mias_key_pair_t** kp) {
	return mias->getKeyPairByContainerId(container_id, kp);
}

extern "C" bool MIAS_get_certificate_by_container_id(MIAS* mias, uint8_t container_id, uint8_t** cert, uint16_t* cert_len) {
	return mias->getCertificateByContainerId(container_id, cert, cert_len);
}

extern "C" bool MIAS_p11_get_object_by_label(MIAS* mias, uint8_t* label, uint16_t label_len, uint8_t** object, uint16_t* object_len) {
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

extern "C" bool MIAS_sign_final(MIAS* mias, uint8_t* hash, uint16_t hash_len, uint8_t* signature, uint16_t* signature_len) {
	return mias->signFinal(hash, hash_len, signature, signature_len);
}

extern "C" bool MIAS_decrypt_init(MIAS* mias, uint8_t algorithm, uint8_t key) {
	return mias->decryptInit(algorithm, key);
}

extern "C" bool MIAS_decrypt_final(MIAS* mias, uint8_t* data, uint16_t data_len, uint8_t* plain, uint16_t* plain_len) {
	return mias->decryptFinal(data, data_len, plain, plain_len);
}
