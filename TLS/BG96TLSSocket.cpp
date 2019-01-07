#include "BG96TLSSocket.h"
#include "BG96.h"

BG96TLSSocket::BG96TLSSocket(BG96* bg96driver) 
{
    bg96 = bg96driver;
    sslctx_id = 0;
}

nsapi_error_t BG96TLSSocket::set_root_ca_cert(const char* cacert)
{
    nsapi_error_t rc = NSAPI_ERROR_DEVICE_ERROR;
    static char * ca_filename;
    if (cacert == NULL) {
        printf("BG96TLSSocket: error - invalid CA certificate.\r\n");
        return rc;
    }

    if ( bg96->send_file(cacert, "cacert.pem") ) {
        ca_filename = "cacert.pem";
    } else {
        printf("BG96TLSSocket: Error transferring CA certificate file to modem.\r\n");
        rc = NSAPI_ERROR_DEVICE_ERROR;
        return rc;
    }

    if ( configure_cacert_path(ca_filename) ){
        rc = NSAPI_ERROR_OK;
    } else {
        printf("BG96TSLSocket: Error while configuring CA path in TLS Socket.\r\n");
        rc = NSAPI_ERROR_DEVICE_ERROR;
    }
    return rc;
}

int configure_cacert_path(const char* path)
{
    int rc = 0;
    if (bg96->configure_cacert_path(sslctx_id, path)) {
        rc = 1;
    } else {
        rc = 0;
    }
    return rc;
}
