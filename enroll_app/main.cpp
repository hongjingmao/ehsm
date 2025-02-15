/*
 * Copyright (C) 2021-2022 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
 *      distribution.
 *   3. Neither the name of Intel Corporation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY CLEANUP OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

using namespace std;

#include <jsoncpp/json/json.h>
#include <sys/time.h>
#include <fstream>
#include <string>
#include <getopt.h>

#include "ulog_utils.h"
#include "rest_utils.h"
#include "enroll_msg.h"

int main(int argc, char *argv[])
{
    enroll_status_t ret = ENL_OK;
    RetJsonObj retJsonObj;

    if (initLogger(NULL) < 0)
    {
        ret = ENL_CONFIG_INVALID;
        return ret;
    }
    log_i("ehsm-kms enroll app start.");

    std::string msg0_str;
    std::string msg2_str;
    std::string att_result_msg_str;
    uint8_t *apikey = nullptr;
    bool cert_verify = true;

    log_d("=> reading ehsm_kms_url .....");
    static const char* _sopts = "hna:";
    static const struct option _lopts[] = {{"help", no_argument, NULL, 'h'},
                                          {"insecure", no_argument, NULL, 'n'},
                                          {"ehsm_kms_url", required_argument, NULL, 'a'},
                                          {0}};
    std::string ehsm_kms_url;
    int opt;
    int oidx = 0;
    do {
        opt = getopt_long(argc, argv, _sopts, _lopts, &oidx);
        switch (opt) {
            case 'a':
                ehsm_kms_url = strdup(optarg);
                break;
            case 'n':
                cert_verify = false;
                break;
            default:
		if (ehsm_kms_url.empty()) {
                    log_i("\nusage: ehsm-kms_enroll_app -a https://1.2.3.4:9000/ehsm/ [-n]");
                    log_i("\n -n Optional only for informal certificates.\n\n");
                    ret = ENL_CONFIG_INVALID;
                    goto OUT;
                }
        }
    } while (opt != -1);

    log_d("ehsm_kms_url : %s", ehsm_kms_url.c_str());

    log_i("First handle:  send msg0 and get msg1.");
    ret = ra_get_msg0(&msg0_str);
    log_d("msg0 : \n%s", msg0_str.c_str());

    log_d("post RA_HANDSHAKE_MSG0.....");
    post_KMS(ehsm_kms_url + "?Action=RA_HANDSHAKE_MSG0", msg0_str, &retJsonObj, cert_verify);
    if (!retJsonObj.isSuccess())
    {
        log_e("NAPI Exception: %s", retJsonObj.getMessage().c_str());
        ret = ENL_NAPI_EXCEPTION;
        goto OUT;
    }
    log_d("post success msg1 : \n%s", retJsonObj.toString().c_str());
    log_i("First handle success.");

    log_i("Second handle:  send msg2 and get msg3.");
    ret = ra_proc_msg1_get_msg2(retJsonObj, &msg2_str);
    if (ret != ENL_OK || msg2_str.empty())
    {
        log_e("ra_proc_msg1_get_msg2 failed. error code [%d]\n", ret);
        goto OUT;
    }
    log_d("msg2 : \n%s", msg2_str.c_str());

    post_KMS(ehsm_kms_url + "?Action=RA_HANDSHAKE_MSG2", msg2_str, &retJsonObj, cert_verify);
    if (!retJsonObj.isSuccess())
    {
        log_e("NAPI Exception: %s", retJsonObj.getMessage().c_str());
        ret = ENL_NAPI_EXCEPTION;
        goto OUT;
    }
    log_d("post success msg3 : \n%s", retJsonObj.toString().c_str());
    log_i("Second handle success.");

    log_i("Third handle:  send att_result_msg and get ciphertext of the APP ID and API Key.");
    ret = ra_proc_msg3_get_att_result_msg(retJsonObj, &att_result_msg_str);
    if (ret != ENL_OK || att_result_msg_str.empty())
    {
        log_e("ra_proc_msg3_get_att_result_msg failed. error code [%d]\n", ret);
        ret = ENL_NAPI_EXCEPTION;
        goto OUT;
    }
    log_d("att_result_msg : \n%s", att_result_msg_str.c_str());

    post_KMS(ehsm_kms_url + "?Action=RA_GET_API_KEY", att_result_msg_str, &retJsonObj, cert_verify);
    if (!retJsonObj.isSuccess())
    {
        log_e("NAPI Exception: %s", retJsonObj.getMessage().c_str());
        ret = ENL_NAPI_EXCEPTION;
        goto OUT;
    }
    log_d("post success apikey_result_msg : \n%s", retJsonObj.toString().c_str());

    apikey = (uint8_t *)calloc(EH_API_KEY_SIZE + 1, sizeof(uint8_t));
    if (apikey == NULL)
    {
        ret = ENL_DEVICE_MEMORY_FAILED;
        goto OUT;
    }
    ret = ra_proc_apikey_result_msg_get_apikey(retJsonObj, apikey);
    if (ret != ENL_OK)
    {
        log_e("ra_proc_cipherapikey_result_msg_get_apikey failed. error code [%d]\n", ret);
        goto OUT;
    }

    log_i("\nappid: %s\n", retJsonObj.readData_cstr("appid"));
    log_i("\napikey: %s\n\n", apikey);

    log_i("decrypt APP ID and API Key success.");
    log_i("Third handle success.");

OUT:
    log_i("ehsm-kms enroll app end.");
    logger_shutDown();
    if (apikey)
        explicit_bzero(apikey, EH_API_KEY_SIZE+1);
    return ret;
}
