#ifndef TA_SEG_WITH_OPTEE_H
#define TA_SEG_WITH_OPTEE_H

/*
 * This UUID is generated with uuidgen
 * the ITU-T UUID generator at http://www.itu.int/ITU-T/asn1/uuid.html
 */
#define TA_SEG_WITH_OPTEE_UUID \
	{ 0xfaaaf200, 0x2450, 0x11e4, \
		{ 0xab, 0xe2, 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b} }

#define PER_PRICE_STR_SIZE		32

#define PROVIDER_PARAMS_NUM     4
#define REQUESTER_PARAMS_NUM    3

#define HASH_SIZE 32

#define TA_VERIFY_RA        0
#define TA_VERIFY_CIPHER    1
#define TA_VERIFY_ECDHKEY	2

#define AES128_BLOCK_BYTE_SIZE  16

#define ECDSA256_KEY_BIT_SIZE   256
#define ECDSA256_KEY_BYTE_SIZE  (ECDSA256_KEY_BIT_SIZE / 8)
#define ECDH256_KEY_BIT_SIZE    256
#define ECDH256_KEY_BYTE_SIZE   (ECDH256_KEY_BIT_SIZE / 8)

/* The function IDs implemented in this TA */
#define TA_SEG_WITH_OPTEE_CMD_GET_RA_PUBKEY			0
#define TA_SEG_WITH_OPTEE_CMD_ECDSA_KEYGEN			1
#define TA_SEG_WITH_OPTEE_CMD_ECDH_DERIVE_KEY		2
#define TA_SEG_WITH_OPTEE_CMD_AES_DECRYPT			3
#define TA_SEG_WITH_OPTEE_CMD_GET_PARAMS_INFO		4
#define TA_SEG_WITH_OPTEE_CMD_BUILD_KEY_HANDLE		5
#define TA_SEG_WITH_OPTEE_CMD_VERIFY_SIGNATURE		6
#define TA_SEG_WITH_OPTEE_CMD_REMOTE_ATTESTATION	7
#define TA_SEG_WITH_OPTEE_CMD_START_PRICING_GAME	8

#endif /*TA_AES_TEST_H*/
