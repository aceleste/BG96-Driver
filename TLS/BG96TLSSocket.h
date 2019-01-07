#ifndef __BG96TLSSOCKET_H__
#define __BG96TLSSOCKET_H__
#include "BG96.h"
#include "nsapi_types.h"

#define BG96TLSSOCKET_SSLVERSION_SSL3_0     0
#define BG96TLSSOCKET_SSLVERSION_TLS1_0     1
#define BG96TLSSOCKET_SSLVERSION_TLS1_1     2
#define BG96TLSSOCKET_SSLVERSION_TLS1_2     3
#define BG96TLSSOCKET_SSLVERSION_ALL        4 //THIS IS THE DEFAULT

class BG96TLSSocket 
{
public:
    BG96TLSSocket(BG96 * bg96driver);
    ~BG96TLSSocket() {};

    void set_timeout(int timeout);
    nsapi_error_t recv(void * buffer, nsapi_size_t size);
    nsapi_error_t send(const void * data, nsapi_size_t size);
    nsapi_error_t set_root_ca_cert(const char * root_ca_pem);
    nsapi_error_t set_client_cert_key(const char * client_cert_pem, const char * client_private_key_pem);
    nsapi_error_t connect(const char* hostname, int port);
    nsapi_error_t close();
private:
    int configure_cacert_path(const char* path);
    int configure_client_cert_path(const char* path);
    int configure_client_key_path(const char* path);
    int configure_sslversion(int version);
//    nsapi_error_t configure_ciphersuite(BG96TLSSocket_cipher_suite_t suite);
    int configure_seclevel(int seclevel);
    int configure_ignorelocaltime(bool ignorelocaltime);
    int configure_negotiatetime(int negotiatetime);

    BG96* bg96;
    int sslctx_id;
    int client_id;
    int pdp_ctx;
};

#endif //__BG96TLSSOCKET_H__