#include "BG96MQTTClient.h"
#include "BG96.h"
#include "BG96TLSSocket.h"
#include "nsapi_types.h"

#define RETURN_RC_IF_NEG(A,B)   A = B; \
                                if (A < 0) return A

BG96MQTTClient::BG96MQTTClient(BG96* bg96, BG96TLSSocket* tls)
{
    _bg96 = bg96;
    _tls  = tls;
    _ctx.pdp_ctx  = DEFAULT_PDP;
    _ctx.ssl_ctx  = 0;
    _ctx.mqtt_ctx = 0;
}

BG96MQTTClient::~BG96MQTTClient()
{

}

nsapi_error_t BG96MQTTClient::configure_pdp_context(BG96_PDP_Ctx* pdp_ctx)
{
    return _bg96->configure_pdp_context(pdp_ctx);
}

nsapi_error_t BG96MQTTClient::open(MQTTNetwork_Ctx* network_ctx)
{
    if (network_ctx == NULL) return NSAPI_ERROR_DEVICE_ERROR;

    if (_ctx.options == NULL) return NSAPI_ERROR_DEVICE_ERROR;

    if (_ctx.options->sslenable) {

        _tls->set_root_ca_cert(network_ctx->ca_cert.payload);
        _tls->set_client_cert_key(network_ctx->client_cert.payload,
                                  network_ctx->client_key.payload);
    }


    if (!_bg96.isConnected()) _bg96.connect(_ctx.pdp_ctx); //Check is needed or 

    _bg96.mqtt_open(network_ctx->hostname.payload, network_ctx->port); 
    
}

nsapi_error_t BG96MQTTClient::close()
{
    return _bg96.mqtt_close();
}

nsapi_error_t BG96MQTTClient::configure_mqtt(MQTTClientOptions* options)
{
    int rc=-1;
    if (options == NULL) return NSAPI_ERROR_DEVICE_ERROR;
    RETURN_RC_IF_NEG(rc, configure_mqtt_version(options->version));
    RETURN_RC_IF_NEG(rc, configure_mqtt_pdpcid(_ctx.pdp_ctx_id)); //TODO: replace value par MACRO in every place where pdp id is required
    RETURN_RC_IF_NEG(rc, cconfigure_mqtt_will(options->will_fg,
                                             options->will_qos,
                                             options->will_retain,
                                             options->will_topic.payload,
                                             options->will_msg.payload));
    RETURN_RC_IF_NEG(rc, configure_mqtt_timeout(options->timeout,
                                                options->retries,
                                                options->timeout_notice));
    RETURN_RC_IF_NEG(rc, configure_mqtt_session(options->cleansession));
    RETURN_RC_IF_NEG(rc, configure_mqtt_keepalive(options->keepalive));
    RETURN_RC_IF_NET(rc, configure_mqtt_sslenable(options->sslenable));
    return NSAPI_ERROR_OK;
}

nsapi_error_t BG96MQTTClient::configure_mqtt_version(int version)
{
    char cmd[80];
    sprintf(cmd, "AT+QMTCFG=\"version\",%d,%d",_ctx->mqtt_ctx_id,version, BG96_AT_TIMEOUT);
    return _bg96->send_generic_cmd(cmd);
}

nsapi_error_t BG96MQTTClient::configure_mqtt_pdpcid(int pdp_id)
{
    char cmd[80];
    sprintf(cmd, "AT+QMTCFG=\"pdpcid\",%d,%d",_ctx->mqtt_ctx_id, pdp_id, BG96_AT_TIMEOUT);
    return _bg96->send_generic_cmd(cmd);
}

nsapi_error_t BG96MQTTClient::configure_mqtt_will(  int will_fg,
                                                    int will_qos,
                                                    int will_retain,
                                                    const char* will_topic,
                                                    const char* will_msg)
{
    char cmd[256];
    sprintf(cmd, "AT+QMTCFG=\"will\",%d,%d,%d,%d,%s,%s",_ctx->mqtt_ctx_id, will_fg,
                                                                          will_qos,
                                                                          will_retain,
                                                                          will_topic,
                                                                          will_msg, BG96_AT_TIMEOUT);
    return _bg96->send_generic_cmd(cmd);
}

nsapi_error_t BG96MQTTClient::configure_mqtt_timeout(int timeout, int retries,int timeout_notice)
{
    char cmd[80];
    sprintf(cmd, "AT+QMTCFG=\"timeout\",%d,%d,%d,%d",_ctx->mqtt_ctx_id, timeout, retries, timeout_notice, BG96_AT_TIMEOUT);
    return _bg96->send_generic_cmd(cmd);
}

nsapi_error_t BG96MQTTClient::configure_mqtt_session(int cleansession)
{
    char cmd[80];
    sprintf(cmd, "AT+QMTCFG=\"session\",%d,%d",_ctx->mqtt_ctx_id, cleansession, BG96_AT_TIMEOUT);
    return _bg96->send_generic_cmd(cmd);
}

nsapi_error_t BG96MQTTClient::configure_mqtt_keepalive(int keepalive)
{
    char cmd[80];
    sprintf(cmd, "AT+QMTCFG=\"keepalive\",%d,%d",_ctx->mqtt_ctx_id, keepalive, BG96_AT_TIMEOUT);
    return _bg96->send_generic_cmd(cmd);
}

nsapi_error_t BG96MQTTClient::configure_mqtt_sslenable(int sslenable)
{
    char cmd[80];
    sprintf(cmd, "AT+QMTCFG=\"ssl\",%d,%d,%d", _ctx->mqtt_ctx_id, sslenable, _ctx.ssl_ctx_id, BG96_AT_TIMEOUT);
    return _bg96->send_generic_cmd(cmd);
}

nsapi_error_t BG96MQTTClient::connect(MQTTConnect_Ctx* ctx)
{
    int rc=-1;
    ConnectResult result;
    rc = _bg96->mqtt_connect(_ctx.mqtt_ctx_id, ctx->client_id.payload,
                                               ctx->username.payload,
                                               ctx->password.payload,
                                               result);
    if (result.result == 0) return NSAPI_ERROR_OK;
    switch (result.rc) {
        case BG96_MQTT_CLIENT_CONNECT_ERROR_BAD_CREDENTIALS:
            rc = NSAPI_ERROR_AUTH_FAILURE;
#if defined(MQTT_DEBUG)
            printf("BG96MQTTClient: Connect error. Bad credentials.\r\n");
#endif
            break;
        case BG96_MQTT_CLIENT_CONNECT_ERROR_IDENTIFIER_REJECTED:
            rc = NSAPI_ERROR_AUTH_FAILURE;
#if defined(MQTT_DEBUG)
            printf("BG96MQTTClient: Connect error. Identifier rejected.\r\n");
#endif
            break;
        case BG96_MQTT_CLIENT_CONNECT_ERROR_SERVER_UNAVAILABLE:
            rc = NSAPI_ERROR_CONNECTION_TIMEOUT;
#if defined(MQTT_DEBUG)
            printf("BG96MQTTClient: Connect error. Server not available.\r\n");
#endif
            break;
        case BG96_MQTT_CLIENT_CONNECT_ERROR_UNNACCEPTED_PROTOCOL:
            rc = NSAPI_ERROR_UNSUPPORTED;
#if defined(MQTT_DEBUG)
            printf("BG96MQTTClient: Connect error. Protocol not accepted.\r\n");
#endif
            break;
    }
    return rc;
}

nsapi_error_t disconnect()
{
   return _bg96->mqtt_disconnect(_ctx.mqtt_ctx_id);
}

