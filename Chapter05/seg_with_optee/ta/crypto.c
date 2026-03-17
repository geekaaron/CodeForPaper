#include <nonce.h>
#include <game.h>
#include <global.h>
#include <seg_with_optee_ta.h>
#include <crypto.h>

#define OBJ_SIZE 15
#define AES256_KEY_BIT_SIZE     256
#define AES256_KEY_BYTE_SIZE    (AES256_KEY_BIT_SIZE / 8)

TEE_ObjectHandle key_handle = TEE_HANDLE_NULL;
TEE_ObjectHandle peer_key_handle = TEE_HANDLE_NULL;

TEE_OperationHandle sign_op_handle = TEE_HANDLE_NULL;
TEE_OperationHandle peer_verify_op_handle = TEE_HANDLE_NULL;

uint8_t peer_public_key_x[ECDH256_KEY_BYTE_SIZE];
uint8_t peer_public_key_y[ECDH256_KEY_BYTE_SIZE];

uint8_t ra_hash[HASH_SIZE] = {0x71, 0xbb, 0xb3, 0x50, 0xaa, 0x73, 0xe9, 0x65,
                              0xcf, 0x72, 0x7a, 0xd7, 0x30, 0x51, 0xba, 0x7d,
                              0x60, 0xdc, 0x29, 0x83, 0x26, 0xc2, 0x41, 0x9b,
                              0x1b, 0x3c, 0x9b, 0x8f, 0x5f, 0xd6, 0x1c, 0x13};
uint32_t ra_hash_size = HASH_SIZE;

uint8_t ciphertext_hash[HASH_SIZE];
uint32_t ciphertext_hash_size = HASH_SIZE;

uint8_t ecdhkey_hash[HASH_SIZE];
uint32_t ecdhkey_hash_size = HASH_SIZE;

uint8_t shared_key[AES256_KEY_BYTE_SIZE];
uint32_t shared_key_size = AES256_KEY_BYTE_SIZE;

TEE_Result ecdsa_keygen(uint32_t param_types, TEE_Param params[4])
{
    const uint32_t exp_param_types =
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
                        TEE_PARAM_TYPE_MEMREF_OUTPUT,
                        TEE_PARAM_TYPE_NONE,
                        TEE_PARAM_TYPE_NONE);

    TEE_ObjectHandle object1;
    TEE_ObjectHandle object2;
    TEE_ObjectHandle object3;
    TEE_Attribute attrs[1];
    TEE_Result res;

    uint32_t obj_data_flag;

    char obj_id1[] = "EcdsaPublicKeyX";
    char obj_id2[] = "EcdsaPublicKeyY";
    char obj_id3[] = "EcdsaPrivateKey";

    uint8_t public_key_x[ECDSA256_KEY_BYTE_SIZE];
    uint8_t public_key_y[ECDSA256_KEY_BYTE_SIZE];
    uint8_t private_key[ECDSA256_KEY_BYTE_SIZE];

    uint32_t key_size = ECDSA256_KEY_BYTE_SIZE;

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    // generate ecdsa key pair
    res = TEE_AllocateTransientObject(TEE_TYPE_ECDSA_KEYPAIR,
                                      ECDSA256_KEY_BIT_SIZE, &key_handle);
    if (res != TEE_SUCCESS) return res;

    TEE_InitValueAttribute(&attrs[0], TEE_ATTR_ECC_CURVE, TEE_ECC_CURVE_NIST_P256, 0);
    res = TEE_GenerateKey(key_handle, ECDSA256_KEY_BIT_SIZE, attrs, 1);
    if (res != TEE_SUCCESS) return res;

    // store ecdsa key pair
    obj_data_flag = TEE_DATA_FLAG_ACCESS_READ |
                    TEE_DATA_FLAG_ACCESS_WRITE |
                    TEE_DATA_FLAG_ACCESS_WRITE_META |
                    TEE_DATA_FLAG_OVERWRITE;

    // store public key x
    res = TEE_GetObjectBufferAttribute(key_handle,
                                       TEE_ATTR_ECC_PUBLIC_VALUE_X,
                                       public_key_x, &key_size);
    if (res != TEE_SUCCESS) return res;

    res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE,
                                     obj_id1, OBJ_SIZE,
                                     obj_data_flag,
                                     TEE_HANDLE_NULL,
                                     NULL, 0, &object1);
    if (res != TEE_SUCCESS) return res;

    res = TEE_WriteObjectData(object1, public_key_x, key_size);
    if (res != TEE_SUCCESS) TEE_CloseAndDeletePersistentObject1(object1);
    else TEE_CloseObject(object1);

    // store public key y
    res = TEE_GetObjectBufferAttribute(key_handle,
                                       TEE_ATTR_ECC_PUBLIC_VALUE_Y,
                                       public_key_y, &key_size);
    if (res != TEE_SUCCESS) return res;

    res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE,
                                     obj_id2, OBJ_SIZE,
                                     obj_data_flag,
                                     TEE_HANDLE_NULL,
                                     NULL, 0, &object2);
    if (res != TEE_SUCCESS) return res;

    res = TEE_WriteObjectData(object2, public_key_y, ECDSA256_KEY_BYTE_SIZE);
    if (res != TEE_SUCCESS) TEE_CloseAndDeletePersistentObject1(object2);
    else TEE_CloseObject(object2);

    // store private key
    res = TEE_GetObjectBufferAttribute(key_handle,
                                       TEE_ATTR_ECC_PRIVATE_VALUE,
                                       private_key, &key_size);
    if (res != TEE_SUCCESS) return res;

    res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE,
                                     obj_id3, OBJ_SIZE,
                                     obj_data_flag,
                                     TEE_HANDLE_NULL,
                                     NULL, 0, &object3);
    if (res != TEE_SUCCESS) return res;

    res = TEE_WriteObjectData(object3, private_key, ECDSA256_KEY_BYTE_SIZE);
    if (res != TEE_SUCCESS) TEE_CloseAndDeletePersistentObject1(object3);
    else TEE_CloseObject(object3);

    TEE_MemMove(params[0].memref.buffer, public_key_x, params[0].memref.size);
    TEE_MemMove(params[1].memref.buffer, public_key_y, params[1].memref.size);

    return TEE_SUCCESS;
}

TEE_Result build_key_handle(uint32_t param_types, TEE_Param params[4])
{
    const uint32_t exp_param_types =
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                        TEE_PARAM_TYPE_MEMREF_INPUT,
                        TEE_PARAM_TYPE_NONE,
                        TEE_PARAM_TYPE_NONE);
    TEE_Attribute attrs[4];
    TEE_Result res;

    TEE_ObjectHandle object;
    TEE_ObjectInfo object_info;
    uint32_t read_bytes;

    char obj_id1[] = "EcdsaPublicKeyX";
    char obj_id2[] = "EcdsaPublicKeyY";
    char obj_id3[] = "EcdsaPrivateKey";

    uint8_t public_key_x[ECDSA256_KEY_BYTE_SIZE];
    uint8_t public_key_y[ECDSA256_KEY_BYTE_SIZE];
    uint8_t private_key[ECDSA256_KEY_BYTE_SIZE];

    uint32_t key_size = ECDSA256_KEY_BYTE_SIZE;

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    // read public key x
    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
                                   obj_id1, OBJ_SIZE,
                                   TEE_DATA_FLAG_ACCESS_READ |
                                   TEE_DATA_FLAG_SHARE_READ,
                                   &object);
    if (res != TEE_SUCCESS) return res;

    res = TEE_GetObjectInfo1(object, &object_info);
    if (res != TEE_SUCCESS) return res;

    if (object_info.dataSize > key_size) return TEE_ERROR_SHORT_BUFFER;

    res = TEE_ReadObjectData(object, public_key_x, object_info.dataSize,
                             &read_bytes);
    if (res != TEE_SUCCESS || read_bytes != object_info.dataSize) return res;

    // read public key y
    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
                                   obj_id2, OBJ_SIZE,
                                   TEE_DATA_FLAG_ACCESS_READ |
                                   TEE_DATA_FLAG_SHARE_READ,
                                   &object);
    if (res != TEE_SUCCESS) return res;

    res = TEE_GetObjectInfo1(object, &object_info);
    if (res != TEE_SUCCESS) return res;

    if (object_info.dataSize > key_size) return TEE_ERROR_SHORT_BUFFER;

    res = TEE_ReadObjectData(object, public_key_y, object_info.dataSize,
                             &read_bytes);
    if (res != TEE_SUCCESS || read_bytes != object_info.dataSize) return res;

    // read private key
    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
                                   obj_id3, OBJ_SIZE,
                                   TEE_DATA_FLAG_ACCESS_READ |
                                   TEE_DATA_FLAG_SHARE_READ,
                                   &object);
    if (res != TEE_SUCCESS) return res;

    res = TEE_GetObjectInfo1(object, &object_info);
    if (res != TEE_SUCCESS) return res;

    if (object_info.dataSize > key_size) return TEE_ERROR_SHORT_BUFFER;

    res = TEE_ReadObjectData(object, private_key, object_info.dataSize,
                             &read_bytes);
    if (res != TEE_SUCCESS || read_bytes != object_info.dataSize) return res;

    // build self key handle
    res = TEE_AllocateTransientObject(TEE_TYPE_ECDSA_KEYPAIR,
                                      ECDSA256_KEY_BIT_SIZE, &key_handle);
    if (res != TEE_SUCCESS) return res;

    TEE_InitValueAttribute(&attrs[0], TEE_ATTR_ECC_CURVE, TEE_ECC_CURVE_NIST_P256, 0);
    TEE_InitRefAttribute(&attrs[1], TEE_ATTR_ECC_PUBLIC_VALUE_X,
                         public_key_x, key_size);
    TEE_InitRefAttribute(&attrs[2], TEE_ATTR_ECC_PUBLIC_VALUE_Y,
                         public_key_y, key_size);
    TEE_InitRefAttribute(&attrs[3], TEE_ATTR_ECC_PRIVATE_VALUE,
                         private_key, key_size);

    res = TEE_PopulateTransientObject(key_handle, attrs, 4);
    if (res != TEE_SUCCESS) return res;

    // build peer key handle
    res = TEE_AllocateTransientObject(TEE_TYPE_ECDSA_PUBLIC_KEY,
                                      ECDSA256_KEY_BIT_SIZE, &peer_key_handle);
    if (res != TEE_SUCCESS) return res;

    TEE_InitValueAttribute(&attrs[0], TEE_ATTR_ECC_CURVE, TEE_ECC_CURVE_NIST_P256, 0);
    TEE_InitRefAttribute(&attrs[1], TEE_ATTR_ECC_PUBLIC_VALUE_X,
                         params[0].memref.buffer, params[0].memref.size);
    TEE_InitRefAttribute(&attrs[2], TEE_ATTR_ECC_PUBLIC_VALUE_Y,
                         params[1].memref.buffer, params[1].memref.size);

    res = TEE_PopulateTransientObject(peer_key_handle, attrs, 3);
    if (res != TEE_SUCCESS) return res;

    // build self sign op hanle
    res = TEE_AllocateOperation(&sign_op_handle, TEE_ALG_ECDSA_P256,
                                TEE_MODE_SIGN, ECDSA256_KEY_BIT_SIZE);
    if (res != TEE_SUCCESS) return res;

    res = TEE_SetOperationKey(sign_op_handle, key_handle);
    if (res != TEE_SUCCESS) return res;

    // verify key handle
    res = TEE_AllocateOperation(&peer_verify_op_handle, TEE_ALG_ECDSA_P256,
                                TEE_MODE_VERIFY, ECDSA256_KEY_BIT_SIZE);
    if (res != TEE_SUCCESS) return res;

    res = TEE_SetOperationKey(peer_verify_op_handle, peer_key_handle);
    if (res != TEE_SUCCESS) return res;

    return TEE_SUCCESS;
}

TEE_Result verify_signature(uint32_t param_types, TEE_Param params[4])
{
    const uint32_t exp_param_types =
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,    // mode
                        TEE_PARAM_TYPE_MEMREF_INPUT,   // nocne
                        TEE_PARAM_TYPE_MEMREF_INPUT,   // signature
                        TEE_PARAM_TYPE_MEMREF_INPUT);  // [ciphertext or ecdh public key]

    TEE_OperationHandle digest_op_handle = TEE_HANDLE_NULL;
    TEE_Result res;
    uint8_t *hash = NULL;

    uint8_t nonced_buf[NONCE_SIZE + HASH_SIZE];
    uint8_t nonced_hash[HASH_SIZE];
    uint32_t nonced_hash_size = HASH_SIZE;

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    if (params[0].value.a == TA_VERIFY_RA) hash = ra_hash;
    else if (params[0].value.a == TA_VERIFY_CIPHER) {
        res = TEE_AllocateOperation(&digest_op_handle,
                                    TEE_ALG_SHA256, TEE_MODE_DIGEST, 0);
        if (res != TEE_SUCCESS) return res;

        res = TEE_DigestDoFinal(digest_op_handle,
                                params[3].memref.buffer, params[3].memref.size,
                                ciphertext_hash, &ciphertext_hash_size);
        if (res != TEE_SUCCESS) return res;

        hash = ciphertext_hash;

        TEE_FreeOperation(digest_op_handle);
        digest_op_handle = TEE_HANDLE_NULL;
    } else if (params[0].value.a == TA_VERIFY_ECDHKEY) {
        TEE_MemMove(peer_public_key_x, params[3].memref.buffer, ECDH256_KEY_BYTE_SIZE);
        TEE_MemMove(peer_public_key_y, params[3].memref.buffer + ECDH256_KEY_BYTE_SIZE, ECDH256_KEY_BYTE_SIZE);

        res = TEE_AllocateOperation(&digest_op_handle,
                                    TEE_ALG_SHA256, TEE_MODE_DIGEST, 0);
        if (res != TEE_SUCCESS) return res;

        res = TEE_DigestDoFinal(digest_op_handle,
                                params[3].memref.buffer, params[3].memref.size,
                                ecdhkey_hash, &ecdhkey_hash_size);
        if (res != TEE_SUCCESS) return res;

        hash = ecdhkey_hash;

        TEE_FreeOperation(digest_op_handle);
        digest_op_handle = TEE_HANDLE_NULL;
    }
    else return TEE_ERROR_BAD_PARAMETERS;

    if (is_nonce_used(nonces, nonces_count, params[1].memref.buffer))
        return TEE_ERROR_BAD_PARAMETERS;

    TEE_MemMove(nonces[nonces_count++],
                params[1].memref.buffer, params[1].memref.size);

    TEE_MemMove(nonced_buf, params[1].memref.buffer, params[1].memref.size);
    TEE_MemMove(nonced_buf + params[1].memref.size, hash, HASH_SIZE);

    res = TEE_AllocateOperation(&digest_op_handle,
                                TEE_ALG_SHA256, TEE_MODE_DIGEST, 0);
    if (res != TEE_SUCCESS) return res;

    res = TEE_DigestDoFinal(digest_op_handle,
                            nonced_buf, NONCE_SIZE + HASH_SIZE,
                            nonced_hash, &nonced_hash_size);
    if (res != TEE_SUCCESS) return res;

    return TEE_AsymmetricVerifyDigest(peer_verify_op_handle, NULL, 0,
                                      nonced_hash, nonced_hash_size,
                                      params[2].memref.buffer,
                                      params[2].memref.size);
}

TEE_Result ecdh_derive_key(uint32_t param_types, TEE_Param params[4])
{
    const uint32_t exp_param_types =
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
                        TEE_PARAM_TYPE_MEMREF_OUTPUT,
                        TEE_PARAM_TYPE_MEMREF_OUTPUT,
                        TEE_PARAM_TYPE_NONE);
    TEE_Attribute attrs[2];
    TEE_Result res;

    TEE_ObjectHandle ecdh_key_handle = TEE_HANDLE_NULL;
    TEE_ObjectHandle shared_key_handle = TEE_HANDLE_NULL;
    TEE_OperationHandle derive_op_handle = TEE_HANDLE_NULL;
    TEE_OperationHandle digest_op_handle = TEE_HANDLE_NULL;

    uint8_t nonced_buf[NONCE_SIZE + HASH_SIZE];
    uint8_t nonced_hash[HASH_SIZE];
    uint32_t nonced_hash_size = HASH_SIZE;

    uint8_t public_key_x[ECDH256_KEY_BYTE_SIZE];
    uint8_t public_key_y[ECDH256_KEY_BYTE_SIZE];

    uint32_t public_key_x_size = ECDH256_KEY_BYTE_SIZE;
    uint32_t public_key_y_size = ECDH256_KEY_BYTE_SIZE;

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    // generate ecdh key
    res = TEE_AllocateTransientObject(TEE_TYPE_ECDH_KEYPAIR,
                                      ECDH256_KEY_BIT_SIZE, &ecdh_key_handle);
    if (res != TEE_SUCCESS) return res;

    TEE_InitValueAttribute(&attrs[0], TEE_ATTR_ECC_CURVE, TEE_ECC_CURVE_NIST_P256, 0);
    res = TEE_GenerateKey(ecdh_key_handle, ECDH256_KEY_BIT_SIZE, attrs, 1);
    if (res != TEE_SUCCESS) return res;

    // extract public keys
    res = TEE_GetObjectBufferAttribute(ecdh_key_handle,
                                       TEE_ATTR_ECC_PUBLIC_VALUE_X,
                                       public_key_x, &public_key_x_size);
    if (res != TEE_SUCCESS) return res;

    res = TEE_GetObjectBufferAttribute(ecdh_key_handle,
                                       TEE_ATTR_ECC_PUBLIC_VALUE_Y,
                                       public_key_y, &public_key_y_size);
    if (res != TEE_SUCCESS) return res;

    TEE_MemMove(params[0].memref.buffer, public_key_x, public_key_x_size);
    TEE_MemMove(params[0].memref.buffer + public_key_x_size, public_key_y, public_key_y_size);
    params[0].memref.size = public_key_x_size + public_key_y_size;

    // derive shared key
    res = TEE_AllocateOperation(&derive_op_handle,
                                TEE_ALG_ECDH_DERIVE_SHARED_SECRET,
                                TEE_MODE_DERIVE, ECDH256_KEY_BIT_SIZE);
    if (res != TEE_SUCCESS) return res;

    res = TEE_SetOperationKey(derive_op_handle, ecdh_key_handle);
    if (res != TEE_SUCCESS) return res;

    res = TEE_AllocateTransientObject(TEE_TYPE_GENERIC_SECRET,
                                      ECDH256_KEY_BIT_SIZE, &shared_key_handle);
    if (res != TEE_SUCCESS) return res;

    TEE_InitRefAttribute(&attrs[0], TEE_ATTR_ECC_PUBLIC_VALUE_X,
                         peer_public_key_x, ECDH256_KEY_BYTE_SIZE);
    TEE_InitRefAttribute(&attrs[1], TEE_ATTR_ECC_PUBLIC_VALUE_Y,
                         peer_public_key_y, ECDH256_KEY_BYTE_SIZE);

    TEE_DeriveKey(derive_op_handle, attrs, 2, shared_key_handle);

    res = TEE_GetObjectBufferAttribute(shared_key_handle,
                                       TEE_ATTR_SECRET_VALUE,
                                       shared_key, &shared_key_size);
    if (res != TEE_SUCCESS) return res;

    // sign
    do {
        TEE_GenerateRandom(params[1].memref.buffer, NONCE_SIZE);
    } while(is_nonce_used(nonces, nonces_count, params[1].memref.buffer));

    TEE_MemMove(nonces[nonces_count++], params[1].memref.buffer, NONCE_SIZE);
    params[1].memref.size = NONCE_SIZE;

    res = TEE_AllocateOperation(&digest_op_handle,
                                TEE_ALG_SHA256, TEE_MODE_DIGEST, 0);
    if (res != TEE_SUCCESS) return res;

    res = TEE_DigestDoFinal(digest_op_handle,
                            params[0].memref.buffer, params[0].memref.size,
                            nonced_hash, &nonced_hash_size);
    if (res != TEE_SUCCESS) return res;
    TEE_FreeOperation(digest_op_handle);
    digest_op_handle = TEE_HANDLE_NULL;

    TEE_MemMove(nonced_buf, params[1].memref.buffer, NONCE_SIZE);
    TEE_MemMove(nonced_buf + NONCE_SIZE, nonced_hash, HASH_SIZE);

    res = TEE_AllocateOperation(&digest_op_handle,
                                TEE_ALG_SHA256, TEE_MODE_DIGEST, 0);
    if (res != TEE_SUCCESS) return res;

    res = TEE_DigestDoFinal(digest_op_handle,
                            nonced_buf, NONCE_SIZE + HASH_SIZE,
                            nonced_hash, &nonced_hash_size);
    if (res != TEE_SUCCESS) return res;

    res = TEE_AsymmetricSignDigest(sign_op_handle, NULL, 0,
                                   nonced_hash, nonced_hash_size,
                                   params[2].memref.buffer,
                                   &params[2].memref.size);
    if (res != TEE_SUCCESS) return res;

    TEE_FreeTransientObject(ecdh_key_handle);
    TEE_FreeTransientObject(shared_key_handle);
    TEE_FreeOperation(derive_op_handle);
    TEE_FreeOperation(digest_op_handle);

    return res;
}

TEE_Result aes_decrypt(uint32_t param_types, TEE_Param params[4])
{
    const uint32_t exp_param_types =
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                        TEE_PARAM_TYPE_MEMREF_INPUT,
                        TEE_PARAM_TYPE_VALUE_INPUT,
                        TEE_PARAM_TYPE_VALUE_INPUT);
    TEE_Attribute attr;
    TEE_Result res;

    TEE_ObjectHandle aes_key_handle = TEE_HANDLE_NULL;
    TEE_OperationHandle aes_op_handle = TEE_HANDLE_NULL;

    uint8_t *buffer = NULL;
    uint32_t size = params[1].memref.size;

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    res = TEE_AllocateOperation(&aes_op_handle, TEE_ALG_AES_CBC_NOPAD,
                                TEE_MODE_DECRYPT, AES256_KEY_BIT_SIZE);
    if (res != TEE_SUCCESS) goto ERROR;

    res = TEE_AllocateTransientObject(TEE_TYPE_AES, AES256_KEY_BIT_SIZE, &aes_key_handle);
    if (res != TEE_SUCCESS) goto ERROR;

    TEE_InitRefAttribute(&attr, TEE_ATTR_SECRET_VALUE, shared_key, AES256_KEY_BYTE_SIZE);

    res = TEE_PopulateTransientObject(aes_key_handle, &attr, 1);
    if (res != TEE_SUCCESS) goto ERROR;

    res = TEE_SetOperationKey(aes_op_handle, aes_key_handle);
    if (res != TEE_SUCCESS) goto ERROR;

    TEE_CipherInit(aes_op_handle, params[0].memref.buffer, params[0].memref.size);

    buffer = TEE_Malloc(size, 0);

    res = TEE_CipherUpdate(aes_op_handle,
                           params[1].memref.buffer, params[1].memref.size,
                           buffer, &size);
    if (res != TEE_SUCCESS) goto ERROR;

    // load plaintext to structs
    res = load_game_params(buffer, params[2].value.a, params[3].value.a);

ERROR:
    if (buffer != NULL) TEE_Free(buffer);
    return res;
}