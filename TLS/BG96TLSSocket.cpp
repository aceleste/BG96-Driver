#include "BG96TLSSocket.h"
#include "BG96.h"

BG96TLSSocket::BG96TLSSocket(BG96* bg96driver) 
{
    bg96 = bg96driver;
    pdp_ctx = 1;
    sslctx_id = 0;
    client_id = 0;
}

nsapi_error_t BG96TLSSocket::set_root_ca_cert(const char* cacert)
{
    nsapi_error_t rc = NSAPI_ERROR_DEVICE_ERROR;
    static char ca_filename[80];
    memset(ca_filename, 0, 80);
    if (cacert == NULL) {
        printf("BG96TLSSocket: error - invalid CA certificate.\r\n");
        return rc;
    }

    if ( bg96->send_file(cacert, "cacert.pem", true) ) {
        strcpy(ca_filename, "cacert.pem");
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

int BG96TLSSocket::configure_cacert_path(const char* path)
{
    int rc = 0;
    if (bg96->configure_cacert_path(path, sslctx_id)) {
        rc = 1;
    } else {
        rc = 0;
    }
    return rc;
}

nsapi_error_t BG96TLSSocket::connect(const char* hostname, int port)
{
    int rc = NSAPI_ERROR_OK;
    // if (!bg96->isConnected()) {
    //     printf("BG96TLSSocket: Should connect to APN first.\r\n");
    //     return NSAPI_ERROR_DEVICE_ERROR;
    // }
    // wait(4);
    if (bg96->sslopen(hostname, port, pdp_ctx, client_id, sslctx_id)) {
        rc = NSAPI_ERROR_OK;
        printf("\r\n\r\n\r\nBG96TLSSocket: Successfully opened TLS connection to %s\r\n", hostname);
    } else {
        printf("BG96TLSSocket: Error opening TLS Socket\r\n");
        rc = NSAPI_ERROR_DEVICE_ERROR;
    }
    return rc;
}
