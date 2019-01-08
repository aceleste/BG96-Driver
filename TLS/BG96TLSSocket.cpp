#include "BG96TLSSocket.h"
#include "BG96.h"
#include "rtos.h"

static Thread timeout_thread;

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

void BG96TLSSocket::set_timeout(int timeout)
{
    this->timeout = timeout; 
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

nsapi_error_t BG96TLSSocket::send(const void * data, nsapi_size_t size)
{
    if ( bg96->sslsend(client_id, data, size, this->timeout) ) {
        return NSAPI_ERROR_OK;
    } else {
        return NSAPI_ERROR_TIMEOUT; // Not necessary the issue, but no way to find out.
    };
}
void BG96TLSSocket::recv_timeout()
{
    this->timeout_ovf = false;
    while(!this->timeout_ovf) {
        Thread::wait(this->timeout);
        this->timeout_ovf = true;
    }
}

nsapi_error_t BG96TLSSocket::recv(void * buffer, nsapi_size_t size)
{
    int cnt = -1;
    timeout_thread.start(this, recv_timeout);
    while (!this->timeout_ovf && cnt < 0) {
        cnt = bg96->sslrecv(this->client_id, buffer, size);
    }
    timeout_thread.terminate();
    if (cnt == -1 && this->timeout_ovf) return NSAPI_ERROR_TIMEOUT;
    return cnt;
}


