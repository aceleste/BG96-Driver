#include "BG96TLSSocket.h"
#include "BG96.h"
#include "rtos.h"
#include "Callback.h"

static Thread timeout_thread;

void timeout_task(void * tls_socket);

BG96TLSSocket::BG96TLSSocket(BG96* bg96driver) 
{
    bg96 = bg96driver;
    pdp_ctx = 1;
    sslctx_id = 0;
    client_id = 0;
    timeout = BG96TLSSOCKET_DEFAULT_TO;
    timeout_ovf = false;
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

nsapi_error_t BG96TLSSocket::set_client_cert_key(const char * client_cert_pem, const char * client_private_key_pem)
{
    nsapi_error_t rc = NSAPI_ERROR_OK;
    if (client_cert_pem != NULL) {
        rc = set_cert_pem(client_cert_pem);
    }
    if (rc == NSAPI_ERROR_OK && client_private_key_pem !=NULL) {
        rc = set_privkey_pem(client_private_key_pem);
    } 
    return rc;
}

nsapi_error_t BG96TLSSocket::set_cert_pem(const char * client_cert_pem)
{
    nsapi_error_t rc = NSAPI_ERROR_DEVICE_ERROR;
    static char cert_pem_filename[80];
    memset(cert_pem_filename, 0, 80);
    if (client_cert_pem == NULL) {
        printf("BG96TLSSocket: error - invalid client certificate.\r\n");
        return rc;
    }

    if ( bg96->send_file(client_cert_pem, "clientcert.pem", true) ) {
        strcpy(cert_pem_filename, "clientcert.pem");
    } else {
        printf("BG96TLSSocket: Error transferring client certificate file to modem.\r\n");
        rc = NSAPI_ERROR_DEVICE_ERROR;
        return rc;
    }

    if ( configure_client_cert_path(cert_pem_filename) ){
        rc = NSAPI_ERROR_OK;
    } else {
        printf("BG96TSLSocket: Error while configuring client cert path in TLS Socket.\r\n");
        rc = NSAPI_ERROR_DEVICE_ERROR;
    }
    return rc;
}

nsapi_error_t BG96TLSSocket::set_privkey_pem(const char * client_private_key_pem)
{
    nsapi_error_t rc = NSAPI_ERROR_DEVICE_ERROR;
    static char privkey_pem_filename[80];
    memset(privkey_pem_filename, 0, 80);
    if (client_private_key_pem == NULL) {
        printf("BG96TLSSocket: error - invalid client key.\r\n");
        return rc;
    }

    if ( bg96->send_file(client_private_key_pem, "privkey.pem", true) ) {
        strcpy(privkey_pem_filename, "privkey.pem");
    } else {
        printf("BG96TLSSocket: Error transferring private key file to modem.\r\n");
        rc = NSAPI_ERROR_DEVICE_ERROR;
        return rc;
    }

    if ( configure_client_cert_path(privkey_pem_filename) ){
        rc = NSAPI_ERROR_OK;
    } else {
        printf("BG96TSLSocket: Error while configuring private key path in TLS Socket.\r\n");
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

int BG96TLSSocket::configure_client_cert_path(const char* path)
{
    int rc = 0;
    if (bg96->configure_client_cert_path(path, sslctx_id)) {
        rc = 1;
    } else {
        rc = 0;
    }
    return rc;
}

int BG96TLSSocket::configure_privkey_path(const char* path)
{
    int rc = 0;
    if (bg96->configure_privkey_path(path, sslctx_id)) {
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
        char errstring[80];
        bg96->getError(errstring);
        printf("BG96TLSSocket %s opening TLS Socket\r\n", errstring);
        rc = NSAPI_ERROR_DEVICE_ERROR;
    }
    return rc;
}

bool BG96TLSSocket::is_connected()
{
    return bg96->ssl_client_status(client_id);
}

nsapi_error_t BG96TLSSocket::send(const void * data, nsapi_size_t size)
{
    int rc = -1;
    rc = bg96->sslsend(client_id, data, size, this->timeout);
    if ( rc > 0 ) {
        return rc;
    } else {
        return NSAPI_ERROR_TIMEOUT; // Not necessary the issue, but no way to find out.
    };
}

nsapi_error_t BG96TLSSocket::recv(void * buffer, nsapi_size_t size)
{
    int cnt = -1;
    timeout_thread.start(callback(timeout_task, this));
    while (!this->timeout_ovf && cnt < 0) {
        cnt = bg96->sslrecv(this->client_id, buffer, size);
    }
    timeout_thread.terminate();
    if (cnt == -1 && this->timeout_ovf) return NSAPI_ERROR_TIMEOUT;
    return cnt;
}

void timeout_task(void * arg)
{
    BG96TLSSocket* tls_socket = (BG96TLSSocket *)arg;
    tls_socket->set_timeout_ovf(false);
    while(!tls_socket->get_timeout_ovf()) {
        Thread::wait(tls_socket->get_timeout());
        tls_socket->set_timeout_ovf(true);
    }
}

nsapi_error_t   BG96TLSSocket::close()
{
    return bg96->sslclose(this->client_id);
}




