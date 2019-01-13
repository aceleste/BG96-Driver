#ifndef __BG96_MQTT_CLIENT_H__
#define __BG96_MQTT_CLIENT_H__
#include "BG96.h"
#include "BG96TLSSocket.h"

#define BG96_MQTT_CLIENT_ERROR_FAILED_NETWORK_OPEN  -1
#define BG96_MQTT_CLIENT_ERROR_SUCCESSFUL           0
#define BG96_MQTT_CLIENT_ERROR_WRONG_PARAMETER      1
#define BG96_MQTT_CLIENT_ERROR_IDENTIFIER_OCCUPIED  2
#define BG96_MQTT_CLIENT_ERROR_PDP_ACTIVATE_FAIL    3
#define BG96_MQTT_CLIENT_ERROR_PARSE_DOMAIN_FAIL    4
#define BG96_MQTT_CLIENT_ERROR_NETWORK_DISCONNECT   5
#define BG96_MQTT_CLIENT_ERROR_CLOSE_NETWORK_FAIL   -1

#define BG96_MQTT_CLIENT_CONNECT_ERROR_ACCEPTED             0
#define BG96_MQTT_CLIENT_CONNECT_ERROR_UNNACCEPTED_PROTOCOL 1
#define BG96_MQTT_CLIENT_CONNECT_ERROR_IDENTIFIER_REJECTED  2
#define BG96_MQTT_CLIENT_CONNECT_ERROR_SERVER_UNAVAILABLE   3
#define BG96_MQTT_CLIENT_CONNECT_ERROR_BAD_CREDENTIALS      4
#define BG96_MQTT_CLIENT_CONNECT_ERROR_NOT_AUTHORIZED       5

#define BG96_MQTT_CLIENT_SUBSCRIBE_SUCCESSFUL               0
#define BG96_MQTT_CLIENT_SUBSCRIBE_PACKET_RETRANSMIT        1
#define BG96_MQTT_CLIENT_SUBSCRIBE_PACKET_SEND_FAIL         2

typedef struct {
    char* payload;
    u_int32_t len;
} MQTTString;

typedef struct {
    MQTTString client_id;
    MQTTString username;
    MQTTString password;
} MQTTConnect_Ctx;

typedef struct
{
	MQTTString hostname;
    int port;
    MQTTString ca_cert;
    MQTTString client_cert;
    MQTTString client_key; 
} MQTTNetwork_Ctx;

typedef struct {
    int pdp_ctx_id;
    int ssl_ctx_id;
    int mqtt_ctx_id;    
    MQTTClientOptions* options;
} MQTTClient_Ctx;

#define BG96MQTTClientOptions_Initializer {4,120,5,0,3,0,0,0,{NULL,0},{NULL,0},1,1}

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
    nsapi_error_t       configure_mqtt_timeout(int timeout);
    nsapi_error_t       configure_mqtt_session(int cleansession);
    nsapi_error_t       configure_mqtt_keepalive(int keepalive);
    nsapi_error_t       configure_mqtt_sslenable(int sslenable);

    void                get_mqtt_cfg(MQTTClientOptions &options);

// MQTT Connection to Server/Broker
    MQTTConnect_Ctx*    get_default_connect();           //memset connect struct here
    nsapi_error_t       connect(MQTTConnect_Ctx* data);
    MQTTCONNECTSTATE    get_connect_state();
    nsapi_error_t       disconnect();                    //free connect struct here



    nsapi_error_t       subscribe(const char* topic);
    nsapi_error_t       unsuscribe(const char* topic);
    nsapi_error_t       publish(MQTTMessage* message);
    void                dowork();
protected:
    MQTTSubscription*   get_default_subscription(MQTTMessageHandler handler);
    bool                append_subscription(MQTTSubscription* sublist, MQTTSubscription* newsub);
private:
    void            mqtt_task();

    Thread          _mqtt_thread;
    BG96*           _bg96;
    BG96TLSSocket   _tls;
    MQTTClient_Ctx  _ctx;
    NSAPI
};

#endif //__BG96_MQTT_CLIENT_H__