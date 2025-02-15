/*
 * Copyright (C) 2011-2020 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _SAMPLE_RA_MSG_
#define _SAMPLE_RA_MSG_

#include <stdint.h>

#include "ecp.h"
#include "sgx_quote.h"
#include "sgx_qve_header.h"
#include "sgx_ql_quote.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* Enum for all possible message types between the ISV app and
 * the ISV SP. Requests and responses in the remote attestation
 * sample.
 */
typedef enum _ra_msg_type_t
{
     TYPE_RA_MSG0,
     TYPE_RA_MSG1,
     TYPE_RA_MSG2,
     TYPE_RA_MSG3,
     TYPE_RA_ATT_RESULT,
}ra_msg_type_t;

typedef enum {
    SP_OK,
    SP_UNSUPPORTED_EXTENDED_EPID_GROUP,
    SP_INTEGRITY_FAILED,
    SP_QUOTE_VERIFICATION_FAILED,
    SP_IAS_FAILED,
    SP_INTERNAL_ERROR,
    SP_PROTOCOL_ERROR,
    SP_QUOTE_VERSION_ERROR,
} sp_ra_msg_status_t;

// These status should align with the definition in IAS API spec(rev 0.6)
#define ISVSVN_SIZE         2
#define PSDA_SVN_SIZE       4
#define GID_SIZE            4
#define PSVN_SIZE           18

#define SAMPLE_REPORT_DATA_SIZE         64
#define SAMPLE_CPUSVN_SIZE      16
#define SAMPLE_SP_TAG_SIZE      16
#define SAMPLE_SP_IV_SIZE       12

#ifndef SAMPLE_FEBITSIZE
    #define SAMPLE_FEBITSIZE                    256
#endif

#define SAMPLE_ECP_KEY_SIZE                     (SAMPLE_FEBITSIZE/8)

#define SAMPLE_HASH_SIZE    32  // SHA256
#define SAMPLE_MAC_SIZE     16  // Message Authentication Code

/*Key Derivation Function ID : 0x0001  AES-CMAC Entropy Extraction and Key Expansion*/
const uint16_t SAMPLE_AES_CMAC_KDF_ID = 0x0001;

#define SAMPLE_NISTP256_KEY_SIZE    (SAMPLE_FEBITSIZE/ 8 /sizeof(uint32_t))

#define SAMPLE_SP_TAG_SIZE          16

#define SAMPLE_QUOTE_UNLINKABLE_SIGNATURE 0
#define SAMPLE_QUOTE_LINKABLE_SIGNATURE   1

#pragma pack(push, 1)

typedef uint8_t sample_epid_group_id_t[4];
typedef uint8_t sample_report_data_t[SAMPLE_REPORT_DATA_SIZE];
typedef uint8_t sample_mac_t[SAMPLE_MAC_SIZE];

typedef struct sample_ec_pub_t
{
    uint8_t gx[SAMPLE_ECP_KEY_SIZE];
    uint8_t gy[SAMPLE_ECP_KEY_SIZE];
} sample_ec_pub_t;

typedef struct sample_ec_sign256_t
{
    uint32_t x[SAMPLE_NISTP256_KEY_SIZE];
    uint32_t y[SAMPLE_NISTP256_KEY_SIZE];
} sample_ec_sign256_t;

typedef struct sample_spid_t
{
    uint8_t                 id[16];
} sample_spid_t;

typedef struct sp_aes_gcm_data_t {
    uint32_t        payload_size;       /*  0: Size of the payload which is*/
                                        /*     encrypted*/
    uint8_t         reserved[12];       /*  4: Reserved bits*/
    uint8_t         payload_tag[SAMPLE_SP_TAG_SIZE];
                                        /* 16: AES-GMAC of the plain text,*/
                                        /*     payload, and the sizes*/
    uint8_t         payload[];          /* 32: Ciphertext of the payload*/
                                        /*     followed by the plain text*/
} sp_aes_gcm_data_t;

typedef struct ias_platform_info_blob_t
{
    sgx_quote_nonce_t nonce;
    sgx_ql_qv_result_t quote_verification_result;
    sgx_ql_qe_report_info_t qve_report_info;
} ias_platform_info_blob_t;

/*fixed length to align with internal structure*/
typedef struct sample_ps_sec_prop_desc_t
{
    uint8_t  sample_ps_sec_prop_desc[256];
} sample_ps_sec_prop_desc_t;


typedef struct sample_ra_msg1_t
{
    sample_ec_pub_t             g_a;        /* the Endian-ness of Ga is
                                                 Little-Endian*/
    sample_epid_group_id_t      gid;        /* the Endian-ness of GID is
                                                 Little-Endian*/
} sample_ra_msg1_t;

typedef struct sample_ra_msg2_t
{
    sample_ec_pub_t             g_b;        /* the Endian-ness of Gb is
                                                  Little-Endian*/
    sample_spid_t               spid;       /* In little endian*/
    uint16_t                    quote_type; /* unlinkable Quote(0) or linkable Quote(0) in little endian*/
    uint16_t                    kdf_id;     /* key derivation function id in little endian.
                                             0x0001 for AES-CMAC Entropy Extraction and Key Derivation */
    sample_ec_sign256_t         sign_gb_ga; /* In little endian*/
    sample_mac_t                mac;        /* mac_smk(g_b||spid||quote_type||
                                                       sign_gb_ga)*/
    uint32_t                    sig_rl_size;
    uint8_t                     sig_rl[];
} sample_ra_msg2_t;

typedef struct sample_ra_msg3_t
{
    sample_mac_t                mac;           /* mac_smk(g_a||ps_sec_prop||quote)*/
    sample_ec_pub_t             g_a;           /* the Endian-ness of Ga is*/
                                               /*  Little-Endian*/
    sample_ps_sec_prop_desc_t   ps_sec_prop;
    uint8_t                     quote[];
} sample_ra_msg3_t;


typedef struct sample_ra_att_result_msg_t {
    ias_platform_info_blob_t    platform_info_blob;
    sample_mac_t                mac;    /* mac_smk(attestation_status)*/
    sp_aes_gcm_data_t           secret;
} sample_ra_att_result_msg_t;

typedef struct _ra_samp_request_header_t{
    uint8_t  type;     /* set to one of ra_msg_type_t*/
    uint32_t size;     /*size of request body*/
    uint8_t  align[3];
    uint8_t body[];
}ra_samp_request_header_t;

typedef struct _ra_samp_response_header_t{
    uint8_t  type;      /* set to one of ra_msg_type_t*/
    uint8_t  status[2];
    uint32_t size;      /*size of the response body*/
    uint8_t  align[1];
    uint8_t  body[];
}ra_samp_response_header_t;

// This is a context data structure used on SP side
typedef struct _sp_db_item_t
{
    sample_ec_pub_t             g_a;
    sample_ec_pub_t             g_b;
    sample_ec_key_128bit_t      vk_key;// Shared secret key for the REPORT_DATA
    sample_ec_key_128bit_t      mk_key;// Shared secret key for generating MAC's
    sample_ec_key_128bit_t      sk_key;// Shared secret key for encryption
    sample_ec_key_128bit_t      smk_key;// Used only for SIGMA protocol
    sample_ec_priv_t            b;
    sample_ps_sec_prop_desc_t   ps_sec_prop;
}sp_db_item_t;

#pragma pack(pop)

#ifdef  __cplusplus
}
#endif

#endif
