/**
 * @file BG96MQTTClient.cpp
 * @author Alain CELESTE (alain.celeste@polaris-innovation.com)
 * @brief 
 * @version 0.1
 * @date 2019-08-14
 * 
 * @copyright Copyright (c) 2019
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

#include "BG96MQTTClient.h"
#include "BG96.h"
#include "BG96TLSSocket.h"
#include "Thread.h"
#include "cmsis_os2.h"
#include "nsapi_types.h"

#define RETURN_RC_IF_NEG(A,B)   A = B; \
                                if (A < 0) return A

BG96MQTTClient::BG96MQTTClient(BG96* bg96, BG96TLSSocket* tls) //mqtt_thread(osPriorityNormal,2048,NULL,NULL)
{
    _bg96 = bg96;
    _tls  = tls;
    _tls->set_socket_id(2);
    _ctx.pdp_ctx_id  = DEFAULT_PDP;
    _ctx.ssl_ctx_id  = 2;
    _ctx.mqtt_ctx_id = 0;
    _sublist = NULL;
    _nmid = 1;
    _mqtt_thread = NULL;
    _running = false;
}

BG96MQTTClient::~BG96MQTTClient()
{
    stopRunning();
//    if (_sublist != NULL) freeSublist(); //maybe a good thing to record it and reactivate it on reconnection instead.
}

bool BG96MQTTClient::startMQTTClient()
{
   return _bg96->startup();
}

void BG96MQTTClient::stopMQTTClient()
{
    disconnect();
    _bg96->disconnect();
    _bg96->powerDown();
}

nsapi_error_t BG96MQTTClient::configure_pdp_context(BG96_PDP_Ctx* pdp_ctx)
{
    return _bg96->configure_pdp_context(pdp_ctx);
}

nsapi_error_t BG96MQTTClient::open(MQTTNetwork_Ctx* network_ctx)
{
    int rc=-1;
    char cmd[40];
    if (network_ctx == NULL) return NSAPI_ERROR_DEVICE_ERROR;

    if (_ctx.options == NULL) return NSAPI_ERROR_DEVICE_ERROR;

    if (_ctx.options->sslenable > 0) {
        sprintf(cmd, "AT+QSSLCFG=\"sslversion\",%d,4",_ctx.ssl_ctx_id); //Quick turn around. TODO: Use a specific API.
        _bg96->send_generic_cmd(cmd,BG96_AT_TIMEOUT);
        sprintf(cmd, "AT+QSSLCFG=\"seclevel\",%d,1",_ctx.ssl_ctx_id); //Quick turn around. TODO: Use a specific API.
        _bg96->send_generic_cmd(cmd,BG96_AT_TIMEOUT);       
        _tls->set_root_ca_cert(network_ctx->ca_cert.payload);
        _tls->set_client_cert_key(network_ctx->client_cert.payload,
                                  network_ctx->client_key.payload);

    }


    // if (!_bg96->isConnected()) _bg96->connect(_ctx.pdp_ctx_id); //Check is needed or 

    rc = _bg96->mqtt_open(network_ctx->hostname.payload, network_ctx->port); 
    switch(rc) {
        case 0:
            debug("Successfully opened MQTT Socket to %s:%d\r\n", network_ctx->hostname.payload, network_ctx->port);    
            return NSAPI_ERROR_OK;
        case BG96_MQTT_NETWORK_ERROR_WRONG_PARAMETER:
            debug("BG96MQTTClient: Error opening network socket. Wrong parameter.\r\n");
            return NSAPI_ERROR_DEVICE_ERROR;
        case BG96_MQTT_NETWORK_ERROR_MQTT_OCCUPIED:
            debug("BG96MQTTClient: Error opening network socket. MQTT occupied.\r\n");      
            return NSAPI_ERROR_DEVICE_ERROR;
        case BG96_MQTT_NETWORK_ERROR_PDP_ACTIVATION_ERROR:
            debug("BG96MQTTClient: Error opening network socket. Failed to activate PDP.\r\n");         
            return NSAPI_ERROR_DEVICE_ERROR;
        case BG96_MQTT_NETWORK_ERROR_FAIL_DOMAIN_NAME_PARSING:
            debug("BG96MQTTClient: Error opening network socket. Failed to parse domain name.\r\n");            
            return NSAPI_ERROR_DEVICE_ERROR;
        case BG96_MQTT_NETWORK_ERROR_NETWORK_DISCONNECTED:
            debug("BG96MQTTClient: Error opening network socket. Network disconnection.\r\n");            
            return NSAPI_ERROR_DEVICE_ERROR;
        default:
            return NSAPI_ERROR_DEVICE_ERROR;
    }
    
}

nsapi_error_t BG96MQTTClient::close()
{
    return _bg96->mqtt_close();
}

nsapi_error_t BG96MQTTClient::configure_mqtt(MQTTClientOptions* options)
{
    int rc=-1;
    if (options == NULL) return NSAPI_ERROR_DEVICE_ERROR;
    RETURN_RC_IF_NEG(rc, configure_mqtt_version(options->version));
    RETURN_RC_IF_NEG(rc, configure_mqtt_pdpcid(_ctx.pdp_ctx_id)); //TODO: replace value par MACRO in every place where pdp id is required
    RETURN_RC_IF_NEG(rc, configure_mqtt_will(options->will_fg,
                                             options->will_qos,
                                             options->will_retain,
                                             options->will_topic.payload,
                                             options->will_msg.payload));
    RETURN_RC_IF_NEG(rc, configure_mqtt_timeout(options->timeout,
                                                options->retries,
                                                options->timeout_notice));
    RETURN_RC_IF_NEG(rc, configure_mqtt_session(options->cleansession));
    RETURN_RC_IF_NEG(rc, configure_mqtt_keepalive(options->keepalive));
    RETURN_RC_IF_NEG(rc, configure_mqtt_sslenable(options->sslenable));
    _ctx.options = options;
    return NSAPI_ERROR_OK;
}

nsapi_error_t BG96MQTTClient::configure_mqtt_version(int version)
{
    char cmd[80];
    sprintf(cmd, "AT+QMTCFG=\"version\",%d,%d",_ctx.mqtt_ctx_id,version);
    return _bg96->send_generic_cmd(cmd, BG96_AT_TIMEOUT);
}

nsapi_error_t BG96MQTTClient::configure_mqtt_pdpcid(int pdp_id)
{
    char cmd[80];
    sprintf(cmd, "AT+QMTCFG=\"pdpcid\",%d,%d",_ctx.mqtt_ctx_id, pdp_id);
    return _bg96->send_generic_cmd(cmd, BG96_AT_TIMEOUT);
}

nsapi_error_t BG96MQTTClient::configure_mqtt_will(  int will_fg,
                                                    int will_qos,
                                                    int will_retain,
                                                    const char* will_topic,
                                                    const char* will_msg)
{
    char cmd[256];
    sprintf(cmd, "AT+QMTCFG=\"will\",%d,%d,%d,%d,\"%s\",\"%s\"",_ctx.mqtt_ctx_id, will_fg,
                                                                          will_qos,
                                                                          will_retain,
                                                                          will_topic,
                                                                          will_msg);
    return _bg96->send_generic_cmd(cmd, BG96_AT_TIMEOUT);
}

nsapi_error_t BG96MQTTClient::configure_mqtt_timeout(int timeout, int retries,int timeout_notice)
{
    char cmd[80];
    sprintf(cmd, "AT+QMTCFG=\"timeout\",%d,%d,%d,%d",_ctx.mqtt_ctx_id, timeout, retries, timeout_notice);
    return _bg96->send_generic_cmd(cmd, BG96_AT_TIMEOUT);
}

nsapi_error_t BG96MQTTClient::configure_mqtt_session(int cleansession)
{
    char cmd[80];
    sprintf(cmd, "AT+QMTCFG=\"session\",%d,%d",_ctx.mqtt_ctx_id, cleansession);
    return _bg96->send_generic_cmd(cmd, BG96_AT_TIMEOUT);
}

nsapi_error_t BG96MQTTClient::configure_mqtt_keepalive(int keepalive)
{
    char cmd[80];
    sprintf(cmd, "AT+QMTCFG=\"keepalive\",%d,%d",_ctx.mqtt_ctx_id, keepalive);
    return _bg96->send_generic_cmd(cmd, BG96_AT_TIMEOUT);
}

nsapi_error_t BG96MQTTClient::configure_mqtt_sslenable(int sslenable)
{
    char cmd[80];
    sprintf(cmd, "AT+QMTCFG=\"ssl\",%d,%d,%d", _ctx.mqtt_ctx_id, sslenable, _ctx.ssl_ctx_id);
    return _bg96->send_generic_cmd(cmd, BG96_AT_TIMEOUT);
}

nsapi_error_t BG96MQTTClient::connect(MQTTConnect_Ctx* ctx)
{
    int rc=-1;
    ConnectResult result;
    rc = _bg96->mqtt_connect(_ctx.mqtt_ctx_id, ctx->client_id.payload,
                                               ctx->username.payload,
                                               ctx->password.payload,
                                               result);
    if (result.result == 0 && result.rc == 0)  {
        _running = true;
        return NSAPI_ERROR_OK;
    }
    debug("BG96MQTT: Connect return result: %d and error code: %d\r\n", result.result, result.rc);
    switch (result.rc) {
        case BG96_MQTT_CLIENT_CONNECT_ERROR_BAD_CREDENTIALS:
            rc = NSAPI_ERROR_AUTH_FAILURE;
            debug("BG96MQTTClient: Connect error. Bad credentials.\r\n");
            break;
        case BG96_MQTT_CLIENT_CONNECT_ERROR_IDENTIFIER_REJECTED:
            rc = NSAPI_ERROR_AUTH_FAILURE;
            debug("BG96MQTTClient: Connect error. Identifier rejected.\r\n");
            break;
        case BG96_MQTT_CLIENT_CONNECT_ERROR_SERVER_UNAVAILABLE:
            rc = NSAPI_ERROR_CONNECTION_TIMEOUT;
            debug("BG96MQTTClient: Connect error. Server not available.\r\n");
            break;
        case BG96_MQTT_CLIENT_CONNECT_ERROR_UNNACCEPTED_PROTOCOL:
            rc = NSAPI_ERROR_UNSUPPORTED;
            debug("BG96MQTTClient: Connect error. Protocol not accepted.\r\n");
            break;
        case BG96_MQTT_CLIENT_CONNECT_ERROR_AT_CMD_TIMEOUT:
            rc = NSAPI_ERROR_DEVICE_ERROR;
            debug("BG96MQTTClient: Connect error. AT command timed out.\r\n");
            break;
        default:
            debug("BG96MQTTClient: Connect error (%d)\r\n", result.rc);
            rc =-1 ;     
    }
    return rc;
}

nsapi_error_t BG96MQTTClient::disconnect()
{
   stopRunning();
   return _bg96->mqtt_disconnect(_ctx.mqtt_ctx_id);
}

nsapi_error_t BG96MQTTClient::subscribe(const char* topic, int qos, MQTTMessageHandler handler, void *param) {
    int rc=-1;
    char * atopic = (char *)malloc(strlen(topic)+1);
    memcpy(atopic, topic, strlen(topic)+1);
    MQTTSubscription* sub = (MQTTSubscription*) malloc(sizeof(MQTTSubscription));
    if (sub == NULL) return NSAPI_ERROR_NO_MEMORY;
    sub->handler = handler;
    sub->param = param;
    sub->msg_id = getNextMessageId();
    sub->qos = qos;
    sub->topic.payload = atopic;
    sub->topic.len = strlen(topic);
    if (findSubscriptionByTopic(atopic) == NULL) { //If not already subscribed, subscribe
        if (!append_subscription(sub)) return NSAPI_ERROR_DEVICE_ERROR;
        rc = _bg96->mqtt_subscribe(_ctx.mqtt_ctx_id, sub->topic.payload, sub->qos, sub->msg_id);
    } else { // already subscribed - no need to resubscribe
        rc = NSAPI_ERROR_OK;
    }
    return rc;
}

nsapi_error_t BG96MQTTClient::unsubscribe(const char* topic)
{
    int rc=-1;
    MQTTSubscription* sub = findSubscriptionByTopic(topic);
    if (sub == NULL) return BG96_MQTT_CLIENT_UNSUBSCRIBE_ERROR_TOPIC_NOT_FOUND;
    if (!remove_subscription(sub)) return NSAPI_ERROR_DEVICE_ERROR;
    rc = _bg96->mqtt_unsubscribe(_ctx.mqtt_ctx_id, sub->topic.payload, sub->msg_id);
    free((void *)sub->topic.payload);
    sub->topic.payload = NULL;
    free(sub); //we are done with the subscription release memory
    sub = NULL;
    return rc;
}

bool BG96MQTTClient::matchTopic(const char* topic1, const char* topic2)
{
    bool rc;
    char * t1 = (char*)malloc(strlen(topic1)+1);
    if (t1 == NULL) {
        debug("BG96MQTTClient: Cannot allocate memory for the topic matching function\r\n");
        return false;
    }
    strcpy(t1, topic1);
    char * t2 = (char*)malloc(strlen(topic2)+1);
    if (t1 == NULL) {
        debug("BG96MQTTClient: Cannot allocate memory for the topic matching function\r\n");
        return false;
    }
    strcpy(t2, topic2);    
    char *tt1 = t1 + strlen(topic1);
    while(*tt1 != '/') {
        *tt1 = '\0';
        tt1--;
    }
    char *tt2 = t2 + strlen(topic2);
    while(*tt2 != '/') {
        *tt2 = '\0';
        tt2--;
    }
    // at this stage t1 and t2 contains only the URI path
    if (strcmp(t1, t2)==0) {
        rc = true;
    } else {
        rc = false;
    }
    free(t1);
    free(t2);
    t1 = NULL;
    t2 = NULL;
    return rc;
}

MQTTSubscription* BG96MQTTClient::findSubscriptionByTopic(const char* topic)
{
    MQTTSubscription* iterator = _sublist;
    if (iterator != NULL) {
        if (matchTopic(iterator->topic.payload, topic)) return iterator;
        while(iterator->next != NULL) {
            if (matchTopic(iterator->topic.payload, topic)) break;
            iterator = (MQTTSubscription*)iterator->next;
        }
    }
    return iterator;
}

bool BG96MQTTClient::append_subscription(MQTTSubscription* sub)
{
    MQTTSubscription* iterator;
    if (sub == NULL) return false;
    if (_sublist == NULL) {
        _sublist = sub;
    } else {
        iterator = _sublist;
        while(iterator->next != NULL) iterator = (MQTTSubscription*) iterator->next;
        iterator->next = sub;
    }
    return true;
}

bool BG96MQTTClient::remove_subscription(MQTTSubscription* sub)
{
    MQTTSubscription* iterator;
    if (sub == NULL) return true;
    if (_sublist == NULL) return true;
    iterator = _sublist;
    if (iterator == sub) {
        if (iterator->next != NULL) {
            _sublist = (MQTTSubscription*) iterator->next; 
        } else {
            _sublist = NULL;
        }
    } else {
        while (iterator->next != sub) iterator = (MQTTSubscription*) iterator->next;
        if (sub->next != NULL) {
            iterator->next = sub->next;
            sub->next = NULL;
        } else {
            iterator->next = NULL; // sub was the last item in the list
        }
    }
    return true;
}

nsapi_error_t BG96MQTTClient::publish(MQTTMessage* message)
{
    if (message == NULL) return NSAPI_ERROR_DEVICE_ERROR;
    message->msg_id = getNextMessageId();
    return _bg96->mqtt_publish(_ctx.mqtt_ctx_id,
                                message->msg_id,
                                message->qos,
                                message->retain,
                                message->topic.payload,
                                message->msg.payload,
                                message->msg.len);
}

void * BG96MQTTClient::recv()
{
    return _bg96->mqtt_recv(_ctx.mqtt_ctx_id);
}

bool BG96MQTTClient::isRunning()
{
    bool ret;
    _mqtt_mutex.lock();
    ret = _running;
    _mqtt_mutex.unlock();
    return ret;
}

void BG96MQTTClient::stopRunning()
{
    _mqtt_mutex.lock();
    _running = false;
    _mqtt_mutex.unlock();
    if (_mqtt_thread != NULL) {
        _mqtt_thread->terminate();
        _mqtt_thread->join();
        delete _mqtt_thread;
        _mqtt_thread = NULL;
    }
}

static void mqtt_task(BG96MQTTClient* client)
{
    MQTTSubscription* subp = NULL;
    MQTTMessage* msg = NULL;
    if (client == NULL) {
        debug("BG96MQTTClient: Error null pointer in mqtt_task.\r\n");
    } else {
        while (client->isRunning()) {
            msg = (MQTTMessage *)client->recv();
            if (msg != NULL) {
                if (msg->msg.payload != NULL) {
                    debug("MQTT_TASK: received a message with content: %s.\r\n", msg->msg.payload);
                    if (msg->topic.payload != NULL) {
                        subp = client->findSubscriptionByTopic(msg->topic.payload);
                        if (subp != NULL && subp->handler != NULL) {
                            debug("MQTT_TASK: Found handler for the incoming message topic %s.\r\n", msg->topic.payload);
                            subp->handler(msg, subp->param); // handler is responsible for freeing memory hold by msg
                        } else {
                            debug("MQTT_TASK: Couldn't find handler for subscription topic %s.\r\n",msg->topic.payload);
                        }
                    } else {
                        debug("MQTT_TASK: Topic of received message is NULL.\r\n");
                    }
                }
                if (msg != NULL) msg = NULL;
            }
            wait(2);
        }
    } 
}

osStatus BG96MQTTClient::dowork()
{  
    _mqtt_mutex.lock();
    _running = true;
    _mqtt_mutex.unlock();
    _mqtt_thread = new Thread(callback(mqtt_task, this));
    if (_mqtt_thread != NULL) return osOK;
    return -1;
}


