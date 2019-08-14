/**
 * @file BG96TLSSocket.h
 * @author Alain CELESTE (alain.celeste@polaris-innovation.com)
 * @brief 
 * @version 0.1
 * @date 2019-08-14
 * 
 * @copyright Copyright (c) 2019 Polaris Innovation
 * 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 */

#ifndef __BG96TLSSOCKET_H__
#define __BG96TLSSOCKET_H__
#include "BG96.h"
#include "nsapi_types.h"

#define BG96TLSSOCKET_SSLVERSION_SSL3_0     0
#define BG96TLSSOCKET_SSLVERSION_TLS1_0     1
#define BG96TLSSOCKET_SSLVERSION_TLS1_1     2
#define BG96TLSSOCKET_SSLVERSION_TLS1_2     3
#define BG96TLSSOCKET_SSLVERSION_ALL        4       //THIS IS THE DEFAULT
#define BG96TLSSOCKET_DEFAULT_TO            3000    //3 seconds

class BG96TLSSocket 
{
public:
    BG96TLSSocket(BG96 * bg96driver);
    ~BG96TLSSocket() {};

    void            set_timeout(int timeout)    {this->timeout = timeout;};
    int             get_timeout()               {return this->timeout;};
    bool            get_timeout_ovf()           {return this->timeout_ovf;};
    void            set_timeout_ovf(bool ovf)   {this->timeout_ovf = ovf;};
    void            set_socket_id(int socket_id){this->sslctx_id = socket_id;};
    nsapi_error_t   recv(void * buffer, nsapi_size_t size);
    nsapi_error_t   send(const void * data, nsapi_size_t size);
    nsapi_error_t   set_root_ca_cert(const char * root_ca_pem);
    nsapi_error_t   set_client_cert_key(const char * client_cert_pem, const char * client_private_key_pem);
    nsapi_error_t   connect(const char* hostname, int port);
    bool            is_connected();
    nsapi_error_t   close();

private:
    int     set_cert_pem(const char * client_cert_pem);
    int     set_privkey_pem(const char * client_private_key_pem);
    int     configure_cacert_path(const char* path);
    int     configure_client_cert_path(const char* path);
    int     configure_privkey_path(const char* path);
    int     configure_sslversion(int version);
//    nsapi_error_t configure_ciphersuite(BG96TLSSocket_cipher_suite_t suite);
    int     configure_seclevel(int seclevel);
    int     configure_ignorelocaltime(bool ignorelocaltime);
    int     configure_negotiatetime(int negotiatetime);


    BG96*   bg96;
    int     sslctx_id;
    int     client_id;
    int     pdp_ctx;
    int     timeout;
    bool    timeout_ovf;
};

#endif //__BG96TLSSOCKET_H__