#ifndef __BG96_MQTT_CLIENT_H__
#define __BG96_MQTT_CLIENT_H__
#include "BG96.h"
#include "BG96TLSSocket.h"

#define BG96_MQTT_NETWORK_ERROR_FAILED_NETWORK_OPEN         -1
#define BG96_MQTT_NETWORK_ERROR_SUCCESSFUL                  0
#define BG96_MQTT_NETWORK_ERROR_WRONG_PARAMETER             1
#define BG96_MQTT_NETWORK_ERROR_MQTT_OCCUPIED               2
#define BG96_MQTT_NETWORK_ERROR_PDP_ACTIVATION_ERROR        3
#define BG96_MQTT_NETWORK_ERROR_FAIL_DOMAIN_NAME_PARSING    4
#define BG96_MQTT_NETWORK_ERROR_NETWORK_DISCONNECTED        5
#define BG96_MQTT_NETWORK_ERROR_CLOSE_NETWORK_FAIL          -1

#define BG96_MQTT_CLIENT_CONNECT_ERROR_ACCEPTED             0
#define BG96_MQTT_CLIENT_CONNECT_ERROR_UNNACCEPTED_PROTOCOL 1
#define BG96_MQTT_CLIENT_CONNECT_ERROR_IDENTIFIER_REJECTED  2
#define BG96_MQTT_CLIENT_CONNECT_ERROR_SERVER_UNAVAILABLE   3
#define BG96_MQTT_CLIENT_CONNECT_ERROR_BAD_CREDENTIALS      4
#define BG96_MQTT_CLIENT_CONNECT_ERROR_NOT_AUTHORIZED       5
#define BG96_MQTT_CLIENT_CONNECT_ERROR_AT_CMD_TIMEOUT       6

#define BG96_MQTT_CLIENT_SUBSCRIBE_SUCCESSFUL               0
#define BG96_MQTT_CLIENT_SUBSCRIBE_PACKET_RETRANSMIT        1
#define BG96_MQTT_CLIENT_SUBSCRIBE_PACKET_SEND_FAIL         2

typedef struct {
    char* payload;
    u_int32_t len;
} MQTTString;

typedef struct {
    const char* payload;
    u_int32_t len;
} MQTTConstString;

typedef struct {
    MQTTConstString client_id;
    MQTTString username;
    MQTTConstString password;
} MQTTConnect_Ctx;

typedef struct
{
	MQTTConstString hostname;
    int port;
    MQTTConstString ca_cert;
    MQTTConstString client_cert;
    MQTTConstString client_key;
} MQTTNetwork_Ctx;

typedef struct {
    int version; // 3 -> 3.1; 4 -> 3.1.1
    int keepalive;
    int timeout;
    int timeout_notice;
    int retries;
    int will_fg;
    int will_qos;
    int will_retain;
    MQTTString will_topic;
    MQTTString will_msg;
    int cleansession;
    int sslenable;
} MQTTClientOptions;

typedef struct {
    int pdp_ctx_id;
    int ssl_ctx_id;
    int mqtt_ctx_id;
    MQTTClientOptions* options;
} MQTTClient_Ctx;

#define BG96MQTTClientOptions_Initializer {4,120,5,0,3,0,0,0,{NULL,0},{NULL,0},1,1}



typedef struct {
    MQTTString  topic;
    MQTTString  msg;
} MQTTMessage;

typedef void(*MQTTMessageHandler)(void* ctx, MQTTMessage* payload) ;

typedef struct {
    int  msg_id;
    MQTTString topic;
    int  qos;
    MQTTMessageHandler handler;
    void* next;
} MQTTSubscription;

class BG96MQTTClient
{
public:
    BG96MQTTClient(BG96* bg96, BG96TLSSocket* tls);
    ~BG96MQTTClient();

    /** Open a network socket
     *
     *  @param          char* [at least 40 long]
     */
    nsapi_error_t       open(MQTTNetwork_Ctx* network_ctx);

    nsapi_error_t       close();

    nsapi_error_t       configure_pdp_context(BG96_PDP_Ctx* pdp_ctx);
    nsapi_error_t       configure_mqtt(MQTTClientOptions* options);
    nsapi_error_t       configure_mqtt_version(int version);
    nsapi_error_t       configure_mqtt_pdpcid(int version);
    nsapi_error_t       configure_mqtt_will(int will_fg,
                                            int will_qos,
                                            int will_retain,
                                            const char* will_topic,
                                            const char* will_msg);
    nsapi_error_t       configure_mqtt_timeout(int timeout, int retries, int timeout_notice);
    nsapi_error_t       configure_mqtt_session(int cleansession);
    nsapi_error_t       configure_mqtt_keepalive(int keepalive);
    nsapi_error_t       configure_mqtt_sslenable(int sslenable);

//    void                get_mqtt_cfg(MQTTClientOptions &options);

// MQTT Connection to Server/Broker
//    MQTTConnect_Ctx*    get_default_connect();           //memset connect struct here
    nsapi_error_t       connect(MQTTConnect_Ctx* data);
//    MQTTCONNECTSTATE    get_connect_state();
    nsapi_error_t       disconnect();                    //free connect struct here



    nsapi_error_t       subscribe(const char* topic) {return -1;};
    nsapi_error_t       unsuscribe(const char* topic) {return -1;};
    nsapi_error_t       publish(MQTTMessage* message) {return -1;};
    void                dowork() {};
protected:
    MQTTSubscription*   get_default_subscription(MQTTMessageHandler handler) {return NULL;};
    bool                append_subscription(MQTTSubscription* sublist, MQTTSubscription* newsub) {return false;};
private:
    void                mqtt_task() {};

//    Thread          _mqtt_thread;
    BG96*               _bg96;
    BG96TLSSocket*      _tls;
    MQTTClient_Ctx      _ctx;
    MQTTSubscription*   _sublist;
};

#endif //__BG96_MQTT_CLIENT_H__
