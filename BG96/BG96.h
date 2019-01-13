/**
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: Apache-2.0
*/

/* 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
 
/**
*   @file   BG96.h
*   @brief  Implements NetworkInterface class for use with the Quectel BG96
*           data module running MBed OS v5.x
*
*   @author James Flynn
*
*   @date   1-April-2018
*/

#ifndef __BG96_H__
#define __BG96_H__

#include "mbed.h"
#include "GNSSLoc.h"

#ifndef DEFAULT_APN
#define DEFAULT_APN "m2m.tele2.com"
#endif

#ifndef DEFAULT_PDP
#define DEFAULT_PDP 1
#endif

#define none        "none"
#define usbnmea     "usbnmea"
#define uartnmea    "uartnmea"
 
// If target board does not support Arduino pins, define pins as Not Connected
#if defined(TARGET_FF_ARDUINO)
#if !defined(MBED_CONF_BG96_LIBRARY_BG96_TX)
#define MBED_CONF_BG96_LIBRARY_BG96_TX               D8
#endif
#if !defined(MBED_CONF_BG96_LIBRARY_BG96_RX)
#define MBED_CONF_BG96_LIBRARY_BG96_RX               D2
#endif
#if !defined(MBED_CONF_BG96_LIBRARY_BG96_RESET)
#define MBED_CONF_BG96_LIBRARY_BG96_RESET            D7
#endif
#if !defined(MBED_CONF_BG96_LIBRARY_BG96_WAKE)
#define MBED_CONF_BG96_LIBRARY_BG96_WAKE             D11
#endif
#if !defined(MBED_CONF_BG96_LIBRARY_BG96_PWRKEY)
#define MBED_CONF_BG96_LIBRARY_BG96_PWRKEY           D10
#endif
#else // !defined(TARGET_FF_ARDUINO)
#define MBED_CONF_BG96_LIBRARY_BG96_TX                NC
#define MBED_CONF_BG96_LIBRARY_BG96_RX                NC
#define MBED_CONF_BG96_LIBRARY_BG96_RESET             NC
#define MBED_CONF_BG96_LIBRARY_BG96_WAKE              NC
#define MBED_CONF_BG96_LIBRARY_BG96_PWRKEY            NC
#endif // !defined(TARGET_FF_ARDUINO)

#if !defined(MBED_CONF_BG96_LIBRARY_BG96_GNSS_OUTPORT)
#define MBED_CONF_BG96_LIBRARY_BG96_GNSS_OUTPORT                "usbnmea"
#endif 
#if !defined(MBED_CONF_BG96_LIBRARY_BG96_GNSS_NMEASRC)
#define MBED_CONF_BG96_LIBRARY_BG96_GNSS_NMEASRC                1
#endif 
#if !defined(MBED_CONF_BG96_LIBRARY_BG96_GNSS_GPSNMNEATYPE)
#define MBED_CONF_BG96_LIBRARY_BG96_GNSS_GPSNMNEATYPE           8
#endif 
#if !defined(MBED_CONF_BG96_LIBRARY_BG96_GNSS_GLONASSNMNEATYPE)
#define MBED_CONF_BG96_LIBRARY_BG96_GNSS_GLONASSNMNEATYPE       0
#endif 
#if !defined(MBED_CONF_BG96_LIBRARY_BG96_GNSS_GALILEONMNEATYPE)
#define MBED_CONF_BG96_LIBRARY_BG96_GNSS_GALILEONMNEATYPE       0
#endif
#if !defined(MBED_CONF_BG96_LIBRARY_BG96_GNSS_BEIDOUNMNEATYPE)
#define MBED_CONF_BG96_LIBRARY_BG96_GNSS_BEIDOUNMNEATYPE        0
#endif
#if !defined(MBED_CONF_BG96_LIBRARY_BG96_GNSS_GSVEXTNMEATYPE)
#define MBED_CONF_BG96_LIBRARY_BG96_GNSS_GSVEXTNMEATYPE         0
#endif
#if !defined(MBED_CONF_BG96_LIBRARY_BG96_GNSS_GNSSCONFIG)
#define MBED_CONF_BG96_LIBRARY_BG96_GNSS_GNSSCONFIG             1
#endif
#if !defined(MBED_CONF_BG96_LIBRARY_BG96_GNSS_AUTOGPS)
#define MBED_CONF_BG96_LIBRARY_BG96_GNSS_AUTOGPS                0
#endif

#if !defined(MBED_CONF_BG96_LIBRARY_BG96_GNSS_GNSSMODE)
#define MBED_CONF_BG96_LIBRARY_BG96_GNSS_GNSSMODE                1 //CURRENTLY ONLY ONE SUPPORTED BY BG96
#endif

#if !defined(MBED_CONF_BG96_LIBRARY_BG96_GNSS_FIXMAXTIME)
#define MBED_CONF_BG96_LIBRARY_BG96_GNSS_FIXMAXTIME              30
#endif

#if !defined(MBED_CONF_BG96_LIBRARY_BG96_GNSS_FIXMAXDIST)
#define MBED_CONF_BG96_LIBRARY_BG96_GNSS_FIXMAXDIST               50
#endif

#if !defined(MBED_CONF_BG96_LIBRARY_BG96_GNSS_FIXCOUNT)
#define MBED_CONF_BG96_LIBRARY_BG96_GNSS_FIXCOUNT                 10
#endif

#if !defined(MBED_CONF_BG96_LIBRARY_BG96_GNSS_FIXRATE)
#define MBED_CONF_BG96_LIBRARY_BG96_GNSS_FIXRATE                  10
#endif

typedef struct {
    int result;
    int rc;
} ConnectResult;

typedef struct {
    int pdp_id;
    char* apn;
    char* username;
    char* password;
} BG96_PDP_Ctx;

/** BG96Interface class.
    Interface to a BG96 module.
 */

class BG96
{
public:
    static const unsigned BG96_BUFF_SIZE = 1500;  
    
    BG96(bool debug=false);
    ~BG96();

    /**
    * Init the BG96
    *
    * @param mode mode in which to startup
    * @return true only if BG96 has started up correctly
    */
    bool startup(void);
 
    /**
    * Wait for 'RDY' signal or timeout waiting...
    *
    * @return none.
    */
    void waitBG96Ready(void);

    /**
    * Reset BG96
    *
    * @return true if BG96 resets successfully
    */
    void reset(void);
    
    /**
    * Connect BG96 to APN
    *
    * @param apn the name of the APN
    * @param username (not used)
    * @param password (not used)
    * @return nsapi_error_t
    */
    nsapi_error_t connect(const char *apn, const char *username, const char *password);
 
    /**
    * Connect BG96 to APN
    *
    * @param id of configured pdp context
    * @return nsapi_error_t
    */
    nsapi_error_t connect(int pdp_id);

    /**
    * Disconnect BG96 from AP
    *
    * @return true if BG96 is disconnected successfully
    */
    bool disconnect(void);
 
    /**
    * Get the RSSI of the BG96
    *
    * @retval integet representing the RSSI, 1=poor,2=weak,3=mid-level,4=good,5=strong; 
    *         0=not available 
    */
    int getRSSI(void);

    /**
    * Get the IP address of BG96
    *
    * @return null-teriminated IP address or null if no IP address is assigned
    */
    const char *getIPAddress(char*);
 
    /**
    * Get the MAC address of BG96
    *
    * @return null-terminated MAC address or null if no MAC address is assigned
    */
    const char *getMACAddress(char*);
 
    /**
    * Check if BG96 is conenected
    *
    * @return true only if the chip has an IP address
    */
    bool isConnected(void);
 
    /**
    * Open a socketed connection
    *
    * @param type the type of socket to open "u" (UDP) or "t" (TCP)
    * @param id for saving socket number to (returned by BG96)
    * @param port port to open connection with
    * @param addr the IP address of the destination
    * @return true only if socket opened successfully
    */
    bool open(const char type, int id, const char* addr, int port);
 
    /**
    * Sends data to an open socket
    *
    * @param id of socket to send to
    * @param data to be sent
    * @param amount of data to be sent 
    * @return true only if data sent successfully
    */
    bool send(int id, const void *data, uint32_t amount);
 
    /**
    * Receives data from an open socket
    *
    * @param id to receive from
    * @param pointer to data for returned information
    * @param amount number of bytes to be received
    * @return the number of bytes received
    */
    int32_t recv(int, void *, uint32_t);

    
 
    /**
    * Closes a socket
    *
    * @param id id of socket to close, valid only 0-4
    * @return true only if socket is closed successfully
    */
    bool close(int id);
 
    /**
    * Checks if data is available
    */
    bool readable();
 
    /**
    * Checks if data can be written
    */
    bool writeable();
    
    /**
    * Resolves a URL name to IP address
    */
    bool resolveUrl(const char *name, char* str);
 
    /*
    * Obtain or set the current BG96 active context
    */
    int setContext( int i );
    /*
    * Configure PDP Context
    */
    int configure_pdp_context(BG96_PDP_Ctx * pdp_ctx);
    
    /*
    * enable/disable AT command tracing
    */
    void doDebug(int f);
    
    /** Return the BG96 revision info
     *
     *  @param          none.
     */
    const char* getRev(char*);

    /** Return the last error to occur
     *
     *  @param          char* [at least 40 long]
     */
    bool getError(char *);

    /** Return the amount a data available
     *
     *  @param          char* [at least 40 long]
     */
    int rxAvail(int);

    /** Return true/false if rx data is available 
     *
     *  @param          socket to check
     */
    bool        chkRxAvail(int id);

    /** Return true/false if configuration ok/failed 
     */
    bool        configureGNSS(void);    

     /** Start GNSS - Return true/false if start ok/failed 
     */
    bool        startGNSS(void);   

     /** Stop GNSS - Return true/false if start ok/failed 
     */
    bool        stopGNSS(void);  

    /** Return true/false if GNSS ON/OFF 
     */
    int         isGNSSOn(void);   

    bool        updateGNSSLoc(void);

    /** function to get the last GNSS location 
     */
    GNSSLoc*    getGNSSLoc();

    int         send_generic_cmd(const char* cmd, int timeout);

    int         send_file(const char* content, const char* filename, bool overrideok);

    int         configure_cacert_path(const char* path, int sslctx_id);

    int         configure_client_cert_path(const char* path, int sslctx_id);

    int         configure_privkey_path(const char* path, int sslctx_id);

    int         sslopen(const char* hostname, int port, int pdp_ctx, int client_id, int sslctx_id);

    int        sslsend(int client_id, const void *data, uint32_t amount);

    int        sslsend(int client_id, const void * data, uint32_t amount, int timeout);

    bool        sslChkRxAvail(int client_id);

    int32_t    sslrecv(int client_id, void *data, uint32_t cnt);

    bool       sslclose(int client_id);

    bool       ssl_client_status(int client_id);

    // int         sslrecv();

    // int         sslclose();

    int         mqtt_open(const char* hostname, int port);
    int         mqtt_close();
    int         mqtt_connect(int sslctx_id, const char* clientid, 
                                            const char* username, 
                                            const char* password,
                                            ConnectResult &result);
    int         mqtt_disconnect(int mqtt_id);

private:
    bool        tx2bg96(char* cmd);
    bool        BG96Ready(void);
    bool        hw_reset(void);

    int         _contextID;
    Mutex       _bg96_mutex;

    UARTSerial  _serial;
    ATCmdParser _parser;

    DigitalOut _bg96_reset;
    DigitalOut _vbat_3v8_en;
    DigitalOut _bg96_pwrkey;
    GNSSLoc *  _gnss_loc;
    
};
 
#endif  //__BG96_H__

