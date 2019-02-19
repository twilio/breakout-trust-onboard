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

#include <stdio.h>
#include <string.h>

#include "MF.h"
#include "MIAS.h"

#include "BreakoutTrustOnboardSDK.h"
#include "GenericModem.h"

#include "SEInterface.h"

static MF* _mf;
static MIAS* _mias;
static GenericModem* _modem = nullptr;

#define USE_BASIC_CHANNEL false

typedef struct {
	char*  pin;
	mias_key_pair_t* kp;
} mias_key_t;

static void hex_string_2_bytes_array(uint8_t* hexstr, uint16_t hexstrLen, uint8_t* bytes, uint16_t* bytesLen) {
	uint8_t d;
	uint16_t i, j;

	*bytesLen = 0;
	for(i = 0; i < hexstrLen; *bytesLen += 1) {
		d = 0;
		for(j = i + 2; i < j; i++) {
			d <<= 4;
			if((hexstr[i] >= '0') && (hexstr[i] <= '9')) {
				d |= hexstr[i] - '0';
			}
			else if((hexstr[i] >= 'a') && (hexstr[i] <= 'f')) {
				d |= hexstr[i] - 'a' + 10;
			}
			else if((hexstr[i] >= 'A') && (hexstr[i] <= 'F')) {
				d |= hexstr[i] - 'A' + 10;
			}
		}
		*bytes = d;
		bytes++;
	}
}

static int se_read_ef(uint8_t* efname, uint16_t efnamelen, char** data, int* data_size, const char* pin) {
	int ret;
	uint16_t size;

	if (pin == NULL) {
		return ERR_SE_EF_VERIFY_PIN_ERROR;
	}

	*data_size = -1;
	
	#ifdef __cplusplus
	if(_mf->select(USE_BASIC_CHANNEL)) {
		if(_mf->verifyPin((uint8_t*) pin, strlen(pin))) {
			if(_mf->readEF(efname, efnamelen, (uint8_t**) data, &size)) {
				*data_size = size & 0x0000FFFF;
				ret = 0;
			}
			else {
				ret = ERR_SE_EF_READ_OBJECT_ERROR;
			}
		}
		else {
			ret = ERR_SE_EF_VERIFY_PIN_ERROR;
		}
	}
	_mf->deselect();
	#else
	if(Applet_select((Applet*) _mf, USE_BASIC_CHANNEL)) {
		if(MF_verify_pin(_mf, (uint8_t*) pin, strlen(pin))) {
			if(MF_read_ef(_mf, efname, efnamelen, (uint8_t**) data, &size)) {
				*data_size = size & 0x0000FFFF;
				ret = 0;
			}
			else {
				ret = ERR_SE_EF_READ_OBJECT_ERROR;
			}
		}
		else {
			ret = ERR_SE_EF_VERIFY_PIN_ERROR;
		}
	}
	Applet_deselect((Applet*) _mf);	
	#endif
	
	return ret;
}

static int se_read_object(const char* path, char** obj, int* size, const char* pin) {
	int ret;
	uint8_t* efname;
	uint16_t efname_len;
		
	if (path == NULL || pin == NULL) {
		return ERR_SE_EF_INVALID_NAME_ERROR;
	}

	// Name is expected to be of even size (hex string name)
	if(strlen(path) & 1) {
		return ERR_SE_EF_INVALID_NAME_ERROR;
	}
	
	efname_len = (strlen(path) / 2);
	efname = (uint8_t*) malloc(efname_len * sizeof(uint8_t));
	hex_string_2_bytes_array((uint8_t*) path, strlen(path), efname, (uint16_t*) &efname_len);
	ret = se_read_ef(efname, efname_len, obj, size, pin);
	free(efname);
	
	return ret;
}

static int se_p11_read_object(const char* label, char** obj, int* size, const char* pin) {
	int ret;

	if (label == NULL || pin == NULL) {
		return ERR_SE_MIAS_READ_OBJECT_ERROR;
	}

	#ifdef __cplusplus
	if(_mias->select(USE_BASIC_CHANNEL)) {
		if(_mias->verifyPin((uint8_t*) pin, strlen(pin))) {
			if(_mias->p11GetObjectByLabel((uint8_t*) label, strlen(label), (uint8_t**) obj, (uint16_t*) size)) {
				*size = *size & 0x0000FFFF;
				ret = 0;
			}
			else {
				ret = ERR_SE_MIAS_READ_OBJECT_ERROR;
			}
		}
		else {
			ret = ERR_SE_MIAS_READ_OBJECT_ERROR;
		}
	}
	_mias->deselect();
	#else
	if(Applet_select((Applet*) _mias, USE_BASIC_CHANNEL)) {
		if(MIAS_verify_pin(_mias, (uint8_t*) pin, strlen(pin))) {
			if(MIAS_p11_get_object_by_label(_mias, (uint8_t*) label, strlen(label), (uint8_t**) obj, (uint16_t*) size)) {
				*size = *size & 0x0000FFFF;
				ret = 0;
			}
			else {
				ret = ERR_SE_MIAS_READ_OBJECT_ERROR;
			}
		}
		else {
			ret = ERR_SE_MIAS_READ_OBJECT_ERROR;
		}
	}
	Applet_deselect((Applet*) _mf);	
	#endif
	
	return ret;
}

static int mias_pk_rsa_alt_decrypt(void* ctx, int mode, size_t* olen, const unsigned char* input, unsigned char* output, size_t output_max_len) {
	int ret;
	
	#ifdef __cplusplus
	if(_mias->select(USE_BASIC_CHANNEL)) {
		if(_mias->verifyPin((uint8_t*) ((mias_key_t*) ctx)->pin, strlen(((mias_key_t*) ctx)->pin))) {
			if(_mias->decryptInit(ALGO_RSA_PKCS1_PADDING, ((mias_key_t*) ctx)->kp->kid)) {						
				if(_mias->decryptFinal((uint8_t*) input, ((mias_key_t*) ctx)->kp->size_in_bits / 8, output, (uint16_t*) &olen)) {
						ret = 0;
				}
				else {
					ret = ERR_SE_MIAS_IO_ERROR;
				}
			}
			else {
				ret = ERR_SE_MIAS_IO_ERROR;
			}
		}
		else {
			ret = ERR_SE_MIAS_VERIFY_PIN_ERROR;
		}
	}
	else {
		ret = ERR_SE_MIAS_SELECT_ERROR;
	}
	_mias->deselect();
	#else
	if(Applet_select((Applet*) _mias, USE_BASIC_CHANNEL)) {
		if(MIAS_verify_pin(_mias, (uint8_t*) ((mias_key_t*) ctx)->pin, strlen(((mias_key_t*) ctx)->pin))) {
			if(MIAS_decrypt_init(_mias, ALGO_RSA_PKCS1_PADDING, ((mias_key_t*) ctx)->kp->kid)) {						
				if(MIAS_decrypt_final(_mias, (uint8_t*) input, ((mias_key_t*) ctx)->kp->size_in_bits / 8, output, (uint16_t*) &olen)) {
						ret = 0;
				}
				else {
					ret = ERR_SE_MIAS_IO_ERROR;
				}
			}
			else {
				ret = ERR_SE_MIAS_IO_ERROR;
			}
		}
		else {
			ret = ERR_SE_MIAS_VERIFY_PIN_ERROR;
		}
	}
	else {
		ret = ERR_SE_MIAS_SELECT_ERROR;
	}
	Applet_deselect((Applet*) _mias);
	#endif
	
	return ret;
}

/*
// Used for mbedtls_md_type mapping to mias_alg
// FIXME: implement
static int mbedtls_mias_pk_rsa_alt_sign(void* ctx, int (*f_rng)(void*, unsigned char*, size_t), void* p_rng, int mode, mbedtls_md_type_t md_alg, unsigned int hashlen, const unsigned char* hash, unsigned char* sig) {
	char mias_alg;

	switch (md_alg) {
		case MBEDTLS_MD_SHA1:
			mias_alg = ALGO_SHA1_WITH_RSA_PKCS1_PADDING;
			break;

		case MBEDTLS_MD_SHA224:
			mias_alg = ALGO_SHA224_WITH_RSA_PKCS1_PADDING;
			break;

		case MBEDTLS_MD_SHA256:
			mias_alg = ALGO_SHA256_WITH_RSA_PKCS1_PADDING;
			break;

		case MBEDTLS_MD_SHA384:
			mias_alg = ALGO_SHA384_WITH_RSA_PKCS1_PADDING;
			break;

		case MBEDTLS_MD_SHA512:
			mias_alg = ALGO_SHA512_WITH_RSA_PKCS1_PADDING;
			break;

		default:
			return MBEDTLS_ERR_MD_FEATURE_UNAVAILABLE;
	}

 	return mias_pk_rsa_alt_sign(ctx, f_rng, p_rng, mode, mias_alg, hashlen, hash, sig);
}
*/

static int mias_pk_rsa_alt_sign(void* ctx, int (*f_rng)(void*, unsigned char*, size_t), void* p_rng, int mode, char mias_alg, unsigned int hashlen, const unsigned char* hash, unsigned char* sig) {
	int ret;
	uint16_t sig_size;

	#ifdef __cplusplus
	if(_mias->select(USE_BASIC_CHANNEL)) {
		if(_mias->verifyPin((uint8_t*) ((mias_key_t*) ctx)->pin, strlen(((mias_key_t*) ctx)->pin))) {
			if(_mias->signInit(mias_alg, ((mias_key_t*) ctx)->kp->kid)) {						
				if(_mias->signFinal((uint8_t*) hash, hashlen, sig, &sig_size)) {
					ret = 0;
				}
				else {
					ret = ERR_SE_MIAS_IO_ERROR;
				}
			}
			else {
				ret = ERR_SE_MIAS_IO_ERROR;
			}
		}
		else {
			ret = ERR_SE_MIAS_VERIFY_PIN_ERROR;
		}
	}
	else {
		ret = ERR_SE_MIAS_SELECT_ERROR;
	}
	_mias->deselect();
	#else
	if(Applet_select((Applet*) _mias, USE_BASIC_CHANNEL)) {
		if(MIAS_verify_pin(_mias, (uint8_t*) ((mias_key_t*) ctx)->pin, strlen(((mias_key_t*) ctx)->pin))) {
			if(MIAS_sign_init(_mias, mias_alg, ((mias_key_t*) ctx)->kp->kid)) {						
				if(MIAS_sign_final(_mias, (uint8_t*) hash, hashlen, sig, &sig_size)) {
					ret = 0;
				}
				else {
					ret = ERR_SE_MIAS_IO_ERROR;
				}
			}
			else {
				ret = ERR_SE_MIAS_IO_ERROR;
			}
		}
		else {
			ret = ERR_SE_MIAS_VERIFY_PIN_ERROR;
		}
	}
	else {
		ret = ERR_SE_MIAS_SELECT_ERROR;
	}
	Applet_deselect((Applet*) _mias);
	#endif
	
	return ret;
}

static size_t mias_pk_rsa_alt_key_len(void* ctx) {
	return (((mias_key_t*) ctx)->kp->size_in_bits / 8);
}

int tob_se_init_with_interface(SEInterface *seiface) {
	#ifdef __cplusplus
	_mias = new MIAS();
	_mias->init(seiface);
	
	_mf = new MF();
	_mf->init(seiface);
	#else
	_mias = MIAS_create();
	Applet_init((Applet*) _mias, seiface);
	
	_mf = MF_create();
	Applet_init((Applet*) _mf, seiface);
	#endif
	return 0;
}

int tob_se_init(const char *device) {
  if (_modem != nullptr) {
		printf("Modem already setup, skipping initialization.\n");
    return 0;
  }

  _modem = new GenericModem(device);

	if(!_modem->open()) {
		printf("Error modem not found!\n");
    free(_modem);
    _modem = nullptr;
		return -1;
	}
  return tob_se_init_with_interface(_modem);
}

int tob_x509_crt_extract_se(uint8_t *cert, int *cert_size, const char *path, const char *pin) {
	int ret = ERR_SE_BAD_KEY_NAME_ERROR;
	
	// Read Certificate from EF
	if(memcmp(path, SE_EF_KEY_NAME_PREFIX, strlen(SE_EF_KEY_NAME_PREFIX)) == 0) {
		int obj_size;
		char* obj = NULL;
		
		// Remove prefix from key path
		path += strlen(SE_EF_KEY_NAME_PREFIX);
	
		if((ret = se_read_object(path, &obj, &obj_size, pin)) == 0) {
      memcpy(cert, obj, obj_size); // TODO: re-evaluate this
      cert[obj_size] = '\0';
      *cert_size = obj_size;
		}

		if (obj != NULL) {
			free(obj);
		}
	}

	// Read Certificate from MIAS P11 Data object
	else if(memcmp(path, SE_MIAS_P11_KEY_NAME_PREFIX, strlen(SE_MIAS_P11_KEY_NAME_PREFIX)) == 0) {
		int obj_size;
		char* obj = NULL;
		
		// Remove prefix from key path
		path += strlen(SE_MIAS_P11_KEY_NAME_PREFIX);
		
		if((ret = se_p11_read_object(path, &obj, &obj_size, pin)) == 0) {
      memcpy(cert, obj, obj_size); // TODO: re-evaluate this
      cert[obj_size] = '\0';
      *cert_size = obj_size;
		}

		if (obj != NULL) {
			free(obj);
		}
	}
	
	// Read Certificate from MIAS
	else if(memcmp(path, SE_MIAS_KEY_NAME_PREFIX, strlen(SE_MIAS_KEY_NAME_PREFIX)) == 0) {
		uint8_t cid;
		
		// Remove prefix from key path
		path += strlen(SE_MIAS_KEY_NAME_PREFIX);
			
		cid = 0;
		while(*path) {
			cid *= 10;
			cid += *path - '0';
			path++;
		}
		
		#ifdef __cplusplus
		if(_mias->select(USE_BASIC_CHANNEL)) {
			char* obj;
			int obj_size;
			
			if(_mias->getCertificateByContainerId(cid, (uint8_t**) &obj, (uint16_t*) &obj_size)) {
				obj_size &= 0x0000FFFF;
        memcpy(cert, obj, obj_size); // TODO: re-evaluate this
        cert[obj_size] = '\0';
        *cert_size = obj_size;
				free(obj);
			}
		}
		_mias->deselect();		
		#else
		if(Applet_select((Applet*) _mias, USE_BASIC_CHANNEL)) {
			char* obj;
			int obj_size;
			
			if(MIAS_get_certificate_by_container_id(_mias, cid, (uint8_t**) &obj, (uint16_t*) &obj_size)) {
				obj_size &= 0x0000FFFF;
				if (obj_size > 0) {
					memcpy(cert, obj, obj_size); // TODO: re-evaluate this
					cert[obj_size] = '\0';
					*cert_size = obj_size;
					free(obj);
				}
			}
		}
		Applet_deselect((Applet*) _mias);
		#endif
	}

	return ret;
}

int tob_pk_extract_se(uint8_t *pk, int *pk_size, const char *path, const char *pin) {
	int ret = ERR_SE_BAD_KEY_NAME_ERROR;

	// Read PKey from EF
	if(memcmp(path, SE_EF_KEY_NAME_PREFIX, strlen(SE_EF_KEY_NAME_PREFIX)) == 0) {
		int obj_size;
		char* obj = NULL;
		
		// Remove prefix from key path
		path += strlen(SE_EF_KEY_NAME_PREFIX);
	
		if((ret = se_read_object(path, &obj, &obj_size, pin)) == 0) {
      memcpy(pk, obj, obj_size); // TODO: re-evaluate this
      pk[obj_size] = '\0';
      *pk_size = obj_size;
		}

		if (obj != NULL) {
			free(obj);
		}
	}

	// Read PKey from MIAS P11 Data object
	else if(memcmp(path, SE_MIAS_P11_KEY_NAME_PREFIX, strlen(SE_MIAS_P11_KEY_NAME_PREFIX)) == 0) {
		int obj_size;
		char* obj = NULL;
		
		// Remove prefix from key path
		path += strlen(SE_MIAS_P11_KEY_NAME_PREFIX);
		
		if((ret = se_p11_read_object(path, &obj, &obj_size, pin)) == 0) {
      memcpy(pk, obj, obj_size); // TODO: re-evaluate this
      pk[obj_size] = '\0';
      *pk_size = obj_size;
		}

		if (obj != NULL) {
			free(obj);
		}
	}

/*
// FIXME: implement for both openssl, mbedtls, and maybe wolfssl?
	// Read PKey from MIAS
	else if(memcmp(path, SE_MIAS_KEY_NAME_PREFIX, strlen(SE_MIAS_KEY_NAME_PREFIX)) == 0) {
		uint8_t cid;
		mias_key_t* mias_key;
		
		// Remove prefix from key path
		path += strlen(SE_MIAS_KEY_NAME_PREFIX);
		
		// NOTE: for unknown reason a call to mbedtls_pk_parse_key need to be done even if useless in that case,
		//       otherwise a compil error will occur!
		// ToDo: Investigate this to avoid calling mbedtls_pk_parse_key.
		mbedtls_pk_parse_key(pk, NULL, 0, NULL, 0);
		
		mias_key = (mias_key_t*) malloc(sizeof(mias_key_t));
		mias_key->pin = (char*) malloc((strlen(pin) + 1) * sizeof(char));
		memcpy(mias_key->pin, pin, strlen(pin) + 1);
		
		cid = 0;
		while(*path) {
			cid *= 10;
			cid += *path - '0';
			path++;
		}
		
		#ifdef __cplusplus
		if(_mias->select(USE_BASIC_CHANNEL)) {
			_mias->getKeyPairByContainerId(cid, &mias_key->kp);
		}
		_mias->deselect();
		#else
		if(Applet_select((Applet*) _mias, USE_BASIC_CHANNEL)) {
			MIAS_get_key_pair_by_container_id(_mias, cid, &mias_key->kp);
		}
		Applet_deselect((Applet*) _mias);
		#endif
				
		if(mias_key->kp) {
			return mbedtls_pk_setup_rsa_alt(pk, mias_key, mias_pk_rsa_alt_decrypt, mias_pk_rsa_alt_sign, mias_pk_rsa_alt_key_len);
		}
		else {
			free(mias_key);
		}
	}
*/
	
	return ret;
}

