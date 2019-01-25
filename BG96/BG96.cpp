/**
* copyright (c) 2018-2019, James Flynn
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
*/

/**
*  @file BG96.cpp
*  @brief Implements a standard NetworkInterface class for use with Quicktel BG96
*
*  @author James Flynn
* 
*  @date 19-Mar-2018
*  
*/

#include <ctype.h>

#include "mbed.h"
#include "mbed_debug.h"
#include "BG96.h"
#include "GNSSLoc.h"
#include "nsapi_types.h"
#include "BG96MQTTClient.h"

#ifndef DEFAULT_PDP
#define DEFAULT_PDP 1
#endif

//
// if DEBUG is enabled, these macros are used to dump data arrays
//
#if MBED_CONF_APP_BG96_DEBUG == true
#define TOSTR(x) #x
#define INTSTR(x) TOSTR(x)
#define DUMP_LOC (char*)(__FILE__ ":" INTSTR(__LINE__))

#define DUMP_ARRAY(x,s)	{                                  \
    int i, k;                                              \
    for (i=0; i<s; i+=16) {                                \
        printf("[%s]:0x%04X: ",DUMP_LOC,i);                \
        for (k=0; k<16; k++) {                             \
            if( (i+k)<s )                                  \
                printf("%02X ", x[i+k]);                   \
            else                                           \
                printf("   ");                             \
            }                                              \
        printf("    ");                                    \
        for (k=0; k<16; k++) {                             \
            if( (i+k)<s )                                  \
                printf("%c", isprint(x[i+k])? x[i+k]:'.'); \
            }                                              \
        printf("\n\r");                                    \
        }                                                  \
    }
#else
#define DUMP_ARRAY(x,s) /* not used */
#endif

/** ----------------------------------------------------------
* @brief  constructor
* @param  none
* @retval none
*/
BG96::BG96(bool debug) :  
    _contextID(DEFAULT_PDP), 
    _serial(MBED_CONF_BG96_LIBRARY_BG96_TX, MBED_CONF_BG96_LIBRARY_BG96_RX), 
    _parser(&_serial),
    _bg96_reset(MBED_CONF_BG96_LIBRARY_BG96_RESET),
    _vbat_3v8_en(MBED_CONF_BG96_LIBRARY_BG96_WAKE),
    _bg96_pwrkey(MBED_CONF_BG96_LIBRARY_BG96_PWRKEY)
{
    _serial.set_baud(115200);
    _parser.debug_on(debug);
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _parser.set_delimiter("\r\n");
    _gnss_loc = NULL;
}

BG96::~BG96(void)
{ 
    _bg96_pwrkey = 0;
    _vbat_3v8_en = 0;
    delete(_gnss_loc); 
}

/** ----------------------------------------------------------
* @brief  get BG96 SW version
* @param  none
* @retval string containing SW version
*/
const char* BG96::getRev(char* combined)
{
    bool        ok=false;
    char        buf1[20], buf2[20];

    _bg96_mutex.lock();
    ok = (tx2bg96((char*)"AT+CGMM") && _parser.recv("%s\n",buf1) && _parser.recv("OK") &&
          _parser.send("AT+CGMR") && _parser.recv("%s\n",buf2) && _parser.recv("OK")    );
    _bg96_mutex.unlock();

    if( ok ) 
        sprintf(combined,"%s Rev:%s",buf1,buf2);
    return ok? (const char*) combined : NULL;
}

/** ----------------------------------------------------------
* @brief  enable AT command tracing
* @param  integer, if msb is set, tracing enabled
* @retval none
*/
void BG96::doDebug(int f)
{
    _parser.debug_on(f&0x80);
}
    
/** ----------------------------------------------------------
* @brief  Tx a string to the BG96 and wait for an OK response
* @param  none
* @retval true if OK received, false otherwise
*/
bool BG96::tx2bg96(char* cmd) {
    bool ok=false;
    _bg96_mutex.lock();
    ok=_parser.send(cmd) && _parser.recv("OK");
    _bg96_mutex.unlock();
    return ok;
}

/** ----------------------------------------------------------
* @brief  set the contextID for the BG96. This context will
*         be used for all subsequent operations
* @param  int of desired context. if <1, return the current context
* @retval current context
*/
/*
* Context can be 1-16
*/
int BG96::setContext( int i )
{
    if( i >  16 )
        return -1;

    if( i < 1 )
        return _contextID;

    return _contextID = i;
}

/** ----------------------------------------------------------
* @brief  Configure the context for the BG96. This context will
*         be used for all subsequent operations
* @param  BG96_PDP_Ctx PDP Configuration
* @retval Context id if success | -1 if error
*/
/*
* Context can be 1-16
*/
int BG96::configure_pdp_context(BG96_PDP_Ctx * pdp_ctx)
{
    int rc=-1;
    if (pdp_ctx == NULL) return -1;
    setContext(pdp_ctx->pdp_id);
    _bg96_mutex.lock();
    if (_parser.send("AT+QICSGP=%d,1,\"%s\",\"\",\"\"", pdp_ctx->pdp_id,
                                            pdp_ctx->apn) && _parser.recv("OK")) rc = pdp_ctx->pdp_id; //pdp_ctx->username, pdp_ctx->password)
    _bg96_mutex.unlock();
    return rc;
}

/** ----------------------------------------------------------
* @brief  perform a HW reset of the BG96
* @param  none
* @retval none
*/
void BG96::reset(void)
{
    _bg96_reset = 0;
    _bg96_pwrkey = 0;
    _vbat_3v8_en = 0;
    wait_ms(300);

    _bg96_reset = 1;
    _vbat_3v8_en = 1;
    _bg96_pwrkey = 1;
    wait_ms(400);

    _bg96_reset = 0;
    wait_ms(10);
}

/** ----------------------------------------------------------
* @brief  wait for 'RDY' response from BG96
* @param  none
* @retval true if 'RDY' received, false otherwise
*/
bool BG96::BG96Ready(void)
{
    Timer t;
    int   done=false;
    
    _bg96_mutex.lock();
    reset();
    t.start();
    while( !done && t.read_ms() < BG96_WAIT4READY )
        done = _parser.recv("RDY");
    _bg96_mutex.unlock();
    return done;
}


/** ----------------------------------------------------------
* @brief  startup BG96 module
* @param  none
* @retval true if successful, false otherwise
*/
bool BG96::startup(void)
{
    int   done=false;
    
    if( !BG96Ready() )
        return false;
        
    _bg96_mutex.lock();
    _parser.set_timeout(BG96_1s_WAIT);
    if( tx2bg96((char*)"ATE0") ) {
        done = tx2bg96((char*)"AT+COPS?");
        done = tx2bg96("ATI"); // request product info
    }
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();
    if (done) { 
        done = configureGNSS();
    }
    return done;
 }


/** ----------------------------------------------------------
* @brief  connect to APN
* @param  apn string 
* @param  username (not used)
* @param  password (not used)
* @retval nsapi_error_t
*/
nsapi_error_t BG96::connect(const char *apn, const char *username, const char *password)
{
    char pdp_string[100];
    int i = 0;
	char* search_pt;
//	uint32_t time;
//	int pdp_retry = 0;
	
    _bg96_mutex.lock();
    memset(pdp_string, 0, sizeof(pdp_string));
	printf("Checking APN ...\r\n");
	_parser.send("AT+QICSGP=%d",_contextID);
	while(1){
			//read and store answer
		_parser.read(&pdp_string[i], 1);
		i++;
			//if OK rx, end string; if not, program stops
        search_pt = strstr(pdp_string, "OK\r\n");
		if (search_pt != 0)
		{
            printf("search_pt != 0.\r\n");
            return NSAPI_ERROR_DEVICE_ERROR;
            
		} else {
            printf("search_pt = 0 \r\n");
            break;
        }
		//TODO: add timeout if no aswer from module!!
    }
	printf("pdp_string: %s\r\n", pdp_string);
		//compare APN name, if match, no store is needed
	search_pt = strstr(pdp_string, apn);
	if (search_pt == 0)
	{
		//few delay to purge serial ...
		wait(1);
		printf("Storing APN %s ...\r\n", apn);
		//program APN and connection parameter only for PDP context 1, authentication NONE
		//TODO: add program for other context
		if (!(_parser.send("AT+QICSGP=%d,1,\"%s\",\"%s\",\"%s\",0", _contextID, &apn[0], &username[0], &password[0])
        && _parser.recv("OK"))) 
		{
			return NSAPI_ERROR_DEVICE_ERROR;
		}
	}
    wait(1);
	printf("End APN check\r\n\n");
    _bg96_mutex.unlock();

	//activate PDP context 1 ...	
    return connect(_contextID);
}

nsapi_error_t BG96::connect(int pdp_id)
{
    Timer timer_s;
    char cmd[100];
    printf("PDP activating ...\r\n");
    sprintf(cmd,"AT+QIACT=%d", _contextID);
    _bg96_mutex.lock();
    timer_s.reset();
    bool done=false;
    while( !done && timer_s.read_ms() < BG96_150s_TO ) 
        done = tx2bg96(cmd);
    if (done) printf("PDP started\r\n\n");
    //wait(5);
    //tx2bg96("AT+QNWINFO");
    _bg96_mutex.unlock();
    return done ? NSAPI_ERROR_OK:NSAPI_ERROR_DEVICE_ERROR;
}

/** ----------------------------------------------------------
* @brief  disconnect from an APN
* @param  none
* @retval true/false if disconnect was successful or not
*/
bool BG96::disconnect(void)
{
    char buff[15];
    _parser.set_timeout(BG96_60s_TO);
    sprintf(buff,"AT+QIDEACT=%d\r",_contextID);
    bool ok = tx2bg96(buff);
    _parser.set_timeout(BG96_AT_TIMEOUT);
    return ok;
}

/** ----------------------------------------------------------
* @brief  perform DNS lookup of URL to determine IP address
* @param  string containing the URL 
* @retval string containing the IP results from the URL DNS
*/
bool BG96::resolveUrl(const char *name, char* ipstr)
{
    char buf2[50];
    bool ok=false;
    int  err=0, ipcount=0, dnsttl=0;

    memset(buf2,0,50);

    _bg96_mutex.lock();
    _parser.set_timeout(BG96_1s_WAIT);
    _parser.send("AT+QIDNSGIP=%d,\"%s\"",_contextID,name);
    ok = _parser.recv("OK");
    if (ok) { // timed out for ERROR, try OK
        _parser.set_timeout(BG96_60s_TO);
        if (_parser.recv("+QIURC: \"dnsgip\",%s\r\n", buf2)) {
            if ( sscanf(buf2, "%d", &err) == 1 && err > 0 ) {
                ok = false;
            } else if ( sscanf(buf2, "%d,%d,%d", &err, &ipcount, &dnsttl) == 3 && err==0 && ipcount > 0 ) {
                ok = _parser.recv("+QIURC: \"dnsgip\",\"%[^\"]\"",ipstr);       //use the first DNS value
                if (ok) {
                    _parser.set_timeout(BG96_1s_WAIT);
                    for(int i= 0; i<ipcount-1;i++) 
                        _parser.recv("+QIURC: \"dnsgip\",\"%[^\"]\"", buf2);   //and discrard the rest  if >1
                }
            }
        }
    } else {
        ok = false;
    }
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();  
    return ok;
}

/** ----------------------------------------------------------
* @brief  determine if BG96 is readable
* @param  none
* @retval true/false
*/
bool BG96::readable()
{
    return _serial.readable();
}

/** ----------------------------------------------------------
* @brief  determine if BG96 is writable
* @param  none
* @retval true/false
*/
bool BG96::writeable()
{
    return _serial.writable();
}

/** ----------------------------------------------------------
* @brief  obtain the current RSSI 
* @param  none
* @retval integet representing the RSSI, 1=poor,2=weak,3=mid-level,4=good,5=strong; 0=not available 
*/
int BG96::getRSSI(void)
{
    int   cs=0, er=0;
    bool  done=false;

    _bg96_mutex.lock();
    done = _parser.send("AT+CSQ") && _parser.recv("+CSQ: %d,%d",&cs,&er);
    _bg96_mutex.unlock();

    return done? cs:0;
}


/** ----------------------------------------------------------
* @brief  obtain the IP address socket is using
* @param  none
* @retval string containing IP or NULL on failure
*/
const char *BG96::getIPAddress(char *ipstr)
{
    int   dummy=0, cs=0, ct=0;
    bool  done=false;
    //char reply[80];
    _bg96_mutex.lock();
    _parser.set_timeout(BG96_60s_TO);
    //_parser.flush();
    done = _parser.send("AT+QIACT?") && _parser.recv("+QIACT:%d,%d,%d,\"%16[^\"]\"", &dummy, &cs, &ct, ipstr);
    //_parser.flush();
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();
    //printf("Received %s\r\n", reply);
    printf("ipstr: %s\r\n", ipstr);
    return done? ipstr:NULL;
}

/** ----------------------------------------------------------
* @brief  return the MAC
* @param  none
* @retval string containing the MAC or NULL on failure
*         MAC is created using the ICCID of the SIM
*/
const char *BG96::getMACAddress(char* sn)
{
 
    _bg96_mutex.lock();
    if( _parser.send("AT+QCCID") ) {
        _parser.recv("+QCCID: %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
            &sn[26], &sn[25], &sn[24],&sn[23],&sn[22],
            &sn[21], &sn[19], &sn[18],&sn[16],&sn[15],
            &sn[13], &sn[12], &sn[10],&sn[9], &sn[7],
            &sn[6],  &sn[4],  &sn[3], &sn[1], &sn[0]); 
        sn[2] = sn[5] = sn[8] = sn[11] = sn[14] = sn[17] = ':';
        sn[20] = 0x00; 
        }
    _bg96_mutex.unlock();

    return (const char*)sn;
}

/** ----------------------------------------------------------
* @brief  determine if BG96 is connected to an APN
* @param  none
* @retval true or false
*/
bool BG96::isConnected(void)
{
    char ipAddress[50];
    return resolveUrl("www.google.com", ipAddress);
}

/** ----------------------------------------------------------
* @brief  open a BG96 socket
* @param  type of socket to open ('u' or 't')
* @param  id of BG96 socket
* @param  address (IP)
* @param  port of the socket
* @retval true if successful, else false on failure
*/
bool BG96::open(const char type, int id, const char* addr, int port)
{
    char* stype = (char*)"TCP";
    char  cmd[20];
    int   err=1;
    bool  ok;
      
    if( type == 'u' ) 
      stype = (char*)"UDP";
      
    _bg96_mutex.lock();
    sprintf(cmd,"+QIOPEN: %d,%%d", id);
    _parser.set_timeout(BG96_150s_TO);
    ok=_parser.send("AT+QIOPEN=%d,%d,\"%s\",\"%s\",%d,0,0\r", _contextID, id, stype, addr, port)
        && _parser.recv(cmd, &err) 
        && err == 0;
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();
    if( ok )
        while( recv(id, cmd, sizeof(cmd)) )
            /* clear out any residual data in BG96 buffer */;

    return ok;
}

/** ----------------------------------------------------------
* @brief  get last error code
* @param  none.
* @retval returns true/false if successful and updated error string
*/
bool BG96::getError(char *str)
{
    char lstr[4];
    int  err;
    memset(lstr,0x00,sizeof(lstr));
    _bg96_mutex.lock();
    bool done = (_parser.send("AT+QIGETERROR") 
              && _parser.recv("+QIGETERROR: %d,%[^\\r]",&err,lstr)
              && _parser.recv("OK") );
    _bg96_mutex.unlock();
    if( done )
        sprintf(str,"Error:%d",err);
    return done;
}


/** ----------------------------------------------------------
* @brief  close the BG96 socket
* @param  id of BG96 socket
* @retval true of close successful false on failure. <0 if error
*/
bool BG96::close(int id)
{
    bool  done=false;

    _bg96_mutex.lock();
    _parser.set_timeout(BG96_150s_TO);
    done = (_parser.send("AT+QICLOSE=%d,%d", id, BG96_CLOSE_TO) && _parser.recv("OK"));
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();
    return done;
}

/** ----------------------------------------------------------
* @brief  send data to the BG96
* @param  id of BG96 socket
* @param  pointer to the data to send
* @param  number of bytes to send
* @retval true if send successfull false otherwise
*/
bool BG96::send(int id, const void *data, uint32_t amount)
{
    bool done;
     
    _bg96_mutex.lock();
    _parser.set_timeout(BG96_TX_TIMEOUT);

    done = !_parser.send("AT+QISEND=%d,%ld", id, amount);
    if( !done && _parser.recv(">") )
        done = (_parser.write((char*)data, (int)amount) <= 0);

    if( !done )
        done = _parser.recv("SEND OK");
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();

    return done;
}

/** ----------------------------------------------------------
* @brief  check if RX data has arrived
* @param  id of BG96 socket
* @retval true/false
*/
bool BG96::chkRxAvail(int id)
{
    char  cmd[20];

    sprintf(cmd, "+QIURC: \"recv\",%d", id);
    _parser.set_timeout(1);
    int i = _parser.recv(cmd);
    _parser.set_timeout(BG96_AT_TIMEOUT);
    return i;
}

/** ----------------------------------------------------------
* @brief  check for the amount of data available to read
* @param  id of BG96 socket
* @retval number of bytes in RX buffer or 0
*/
int BG96::rxAvail(int id)
{
    int trl, hrl, url;

    _bg96_mutex.lock();
    bool done = ( _parser.send("AT+QIRD=%d,0",id) && _parser.recv("+QIRD:%d,%d,%d",&trl, &hrl, &url) ); 
    _bg96_mutex.unlock();
    if( done )
        return trl-hrl;
    return 0;
}


/** ----------------------------------------------------------
* @brief  receive data from BG96
* @param  id of BG96 socket
* @param  pointer to location to store returned data
* @param  count of the number of bytes to get
* @retval number of bytes returned or 0
*/
int32_t BG96::recv(int id, void *data, uint32_t cnt)
{
    int  rxCount, ret_cnt=0;

    _bg96_mutex.lock();
    chkRxAvail(id);

    if( _parser.send("AT+QIRD=%d,%d",id,(int)cnt) && _parser.recv("+QIRD:%d\r\n",&rxCount) ) {
        if( rxCount > 0 ) {
            _parser.getc(); //for some reason BG96 always outputs a 0x0A before the data
            _parser.read((char*)data, rxCount);

            if( !_parser.recv("OK") ) 
                rxCount = NSAPI_ERROR_DEVICE_ERROR;
            }
        ret_cnt = rxCount;
        }
    else
        ret_cnt = NSAPI_ERROR_DEVICE_ERROR;
    _bg96_mutex.unlock();
    return ret_cnt;
}

bool BG96::configureGNSS()
{
    // _bg96_mutex.lock();
    // bool done = ( _parser.send("AT+QGPSCFG=%s,%s","\"outport\"", "\"usbnmea\"") && _parser.recv("OK") );
    // if (done) {
    //     done = ( _parser.send("AT+QGPSCFG=%s,%d","\"nmeasrc\"", MBED_CONF_BG96_LIBRARY_BG96_GNSS_NMEASRC) && _parser.recv("OK") );
    // } else {
    //     return done;
    // }
    // if (done) {
    //     done = ( _parser.send("AT+QGPSCFG=%s,%d","\"gpsnmeatype\"", MBED_CONF_BG96_LIBRARY_BG96_GNSS_GPSNMNEATYPE) && _parser.recv("OK") );
    // } else {
    //     return done;
    // }
    // if (done) {
    //     done = ( _parser.send("AT+QGPSCFG=%s,%d","\"glonassnmeatype\"", MBED_CONF_BG96_LIBRARY_BG96_GNSS_GLONASSNMNEATYPE) && _parser.recv("OK") );
    // } else {
    //     return done;
    // }
    // if (done) {
    //     done = ( _parser.send("AT+QGPSCFG=%s,%d","\"galileonmeatype\"", MBED_CONF_BG96_LIBRARY_BG96_GNSS_GALILEONMNEATYPE) && _parser.recv("OK") );
    // } else {
    //     return done;
    // }
    // if (done) {
    //     done = ( _parser.send("AT+QGPSCFG=%s,%d","\"beidounmeatype\"", MBED_CONF_BG96_LIBRARY_BG96_GNSS_BEIDOUNMNEATYPE) && _parser.recv("OK") );
    // } else {
    //     return done;
    // }
    // if (done) {
    //     done = ( _parser.send("AT+QGPSCFG=%s,%d", "\"gsvextnmeatype\"", MBED_CONF_BG96_LIBRARY_BG96_GNSS_GSVEXTNMEATYPE) && _parser.recv("OK") );
    // } else {
    //     return done;
    // }
    // if (done) {
    //     done = ( _parser.send("AT+QGPSCFG=%s,%d", "\"gnssconfig\"", MBED_CONF_BG96_LIBRARY_BG96_GNSS_GNSSCONFIG) && _parser.recv("OK") );
    // } else {
    //     return done;
    // }
    // if (done) {
    //     done = ( _parser.send("AT+QGPSCFG=%s,%d", "\"autogps\"", MBED_CONF_BG96_LIBRARY_BG96_GNSS_AUTOGPS) && _parser.recv("OK") );
    // } else {
    //     return done;
    // }
    // _bg96_mutex.unlock();
    return true;
}

bool BG96::startGNSS(void)
{
    _bg96_mutex.lock();
    /*
    %d,%d,%d,%d", MBED_CONF_BG96_LIBRARY_BG96_GNSS_GNSSMODE, 
                                                        MBED_CONF_BG96_LIBRARY_BG96_GNSS_FIXMAXTIME,
                                                        MBED_CONF_BG96_LIBRARY_BG96_GNSS_FIXMAXDIST,
                                                        MBED_CONF_BG96_LIBRARY_BG96_GNSS_FIXCOUNT,
                                                        MBED_CONF_BG96_LIBRARY_BG96_GNSS_FIXRATE
    */
    bool done = ( _parser.send("AT+QGPS=1") && _parser.recv("OK") );
    _bg96_mutex.unlock();
    return done;
}

bool BG96::stopGNSS(void)
{
    _bg96_mutex.lock();
    bool done = ( _parser.send("AT+QGPSEND") && _parser.recv("OK") );
    _bg96_mutex.unlock();
    return done;   
}

int BG96::isGNSSOn(void)
{
    int state=0;
    bool done=false;
    _bg96_mutex.lock();
    _parser.set_timeout(BG96_1s_WAIT);
    done = (_parser.send("AT+QGPS?") && _parser.recv("+QGPS: %d", &state));
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();
    return done ? state : -1;
}

bool BG96::updateGNSSLoc(void)
{
    char locationstring[120];
//    char cmd[8];
    bool done;
    _bg96_mutex.lock();
    _parser.set_timeout(3000);  
    done = (_parser.send("AT+QGPSLOC=2") && _parser.recv("+QGPSLOC: %[^\r]\r\n", locationstring));  
    _parser.set_timeout(BG96_AT_TIMEOUT);
//    printf("[BG96Driver]: Received cmd -> %s\r\n", cmd);
    printf("[BG96Driver]: Received location string -> \r\n");
    printf("%s\r\n", &locationstring[0]);
//Test only
    // strcpy(locationstring, "200315.0,54.70573,-1.56611,1.3,163.0,2,0.00,0.0,0.0,131218,08");
    // strcpy(cmd, "QGPSLOC"); 
    // done = 1;   
//
    if (done) {
        GNSSLoc * loc = new GNSSLoc(locationstring);
        if (loc != NULL){ 
            delete(_gnss_loc); //free previous loc
            _gnss_loc = loc;
            done = true; 
        } else {
            printf("[BG96Driver]: Failed creating GNSSLoc instance.\r\n");
            done = false;
        }
    }
    _bg96_mutex.unlock();
    return done;
}

GNSSLoc * BG96::getGNSSLoc()
{
    _bg96_mutex.lock();
    GNSSLoc * result = new GNSSLoc(*_gnss_loc); 
    _bg96_mutex.unlock();
    return result;
}

int BG96::file_exists(const char* filename)
{
    int rc = 0;
    int done;
    int fsize;
    char cmd[128];
    char file[80];
    sprintf(cmd, "AT+QFLST=\"%s\"", filename);
    _bg96_mutex.lock();
    _parser.set_timeout(2000);
    done = _parser.send(cmd) && _parser.recv("+QFLST: \"%[^\"]\",%d", file, &fsize) && _parser.recv("OK") && strcmp(file, filename) == 0;
    if (done) rc = 1;
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();
    return rc;
}

int BG96::delete_file(const char* filename)
{
    char cmd[128];
    int done = false;
    sprintf(cmd, "AT+QFDEL=\"%s\"", filename);
    _bg96_mutex.lock();
    done = _parser.send(cmd) && _parser.recv("OK");
    _bg96_mutex.unlock();
    return done;
}

int BG96::send_file(const char* content, const char* filename, bool overrideok)
{
    //test if file exist
    //if file exist
    //   if overrideok is true
    //      delete existing file
    //      upload file
    //   else
    //      return 1 
    //   fi
    //else 
    //   upload file
    //fi
    bool done = false;
    bool upload = true;
    int good = 0;
    unsigned int upload_size=0;
    unsigned int checksum=0;
    size_t filesize = strlen(content)+1;
    char cmd[128];
    
    if (file_exists(filename)) { // File exists
        if (overrideok) {
            done = delete_file(filename);
            if (done) {
                upload = true;
            } else {
                upload = false;
                printf("BG96: Error trying to delete file %s\r\n", filename);
                return 0; //We should be overriding and we can't so we fail.
            }
        } else {
            upload = false;
            return 1; // we suppose the file that is here is ok
        }
    }
    if (upload) {
        sprintf(cmd, "AT+QFUPL=\"%s\",%d", filename, filesize);
        _bg96_mutex.lock();
        _parser.set_timeout(BG96_1s_WAIT);
        done = _parser.send(cmd) && _parser.recv("CONNECT");
        if (!done) {
            _bg96_mutex.unlock();
            return 0;
        } else { //We are now in transparent mode, send data to stream 
            uint16_t i; 
            for (i = 0 ; i < filesize; i++) {
                _parser.putc(*content++);
            }
        }
        _parser.set_timeout(BG96_1s_WAIT);
        done = _parser.recv("+QFUPL: %u, %X\r\n", &upload_size, &checksum);
        if (!done) {
            printf("BG96: Error uploading file %s\r\n", filename);
            good = 0;
        } else { // should check for upload_size == filesize and checksum ok.
            _parser.recv("OK");
            printf("BG96: Successfully uploaded file %s\r\n", filename);
            good = 1;
        }
    }
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();
    return good;
}

int BG96::configure_cacert_path(const char* path, int sslctx_id)
{
    char cmd[128];
    bool done=false;
    int good = -1;
    sprintf(cmd, "AT+QSSLCFG=\"cacert\",%d,\"%s\"",sslctx_id, path);
    _bg96_mutex.lock();
    _parser.set_timeout(3000);
    done = _parser.send(cmd) && _parser.recv("OK");
    if (done) {
        printf("BG96: Successfully configured CA certificate path\r\n");
        good = 1;
    } else {
        char errstring[20];
        if (getError(errstring)) sscanf(errstring,"Error:%d",&good);
        _parser.send("AT+QSSLCFG=?") && _parser.recv("OK");
    }
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();   
    return good;
}

int BG96::configure_client_cert_path(const char* path, int sslctx_id)
{
    char cmd[128];
    bool done=false;
    int good = 0;
    sprintf(cmd, "AT+QSSLCFG=\"clientcert\",%d,\"%s\"",sslctx_id, path);
    _bg96_mutex.lock();
    done = _parser.send(cmd) && _parser.recv("OK");
    if (done) {
        printf("BG96: Successfully configured client certificate path\r\n");
        good = 1;
    } else {
        good = 0;
    }
    _bg96_mutex.unlock();   
    return good;
}

int BG96::configure_privkey_path(const char* path, int sslctx_id)
{
    char cmd[128];
    bool done=false;
    int good = 0;
    sprintf(cmd, "AT+QSSLCFG=\"clientkey\",%d,\"%s\"",sslctx_id, path);
    _bg96_mutex.lock();
    done = _parser.send(cmd) && _parser.recv("OK");
    if (done) {
        printf("BG96: Successfully configured client key path\r\n");
        good = 1;
    } else {
        good = 0;
    }
    _bg96_mutex.unlock();   
    return good;
}

int BG96::sslopen(const char* hostname, int port, int pdp_ctx, int client_id, int sslctx_id)
{
    char cmd[128];
    bool done=false;
    int cid=0;
    int err=-1;

    if (client_id <0 || client_id > 11) {
        printf("BG96: Wrong Client ID\r\n");
        return 0;
    }

    if (pdp_ctx <1 || pdp_ctx > 11) {
        printf("BG96: Wrong PDP Context ID.\r\n");
        return 0;
    }

    sprintf(cmd, "AT+QSSLOPEN=%d,%d,%d,\"%s\",%d", pdp_ctx, client_id, sslctx_id, hostname, port);
    _bg96_mutex.lock();
    _parser.set_timeout(BG96_150s_TO);
    done =  _parser.send(cmd) && _parser.recv("OK");
    if (done) _parser.recv("+QSSLOPEN: %d,%d", &cid, &err);
    if (err != 0) done=false; 
    if (!done) {
        printf("BG96: Error opening TLS socket to host %s\r\n", hostname);
    }
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();
    return done;
}

bool BG96::ssl_client_status(int client_id)
{
    bool done=false;
    int id = -1;
    int remoteport, localport;
    int socket_state;
    int pdp_id, server_id, ssl_id;
    int access_mode;
    char dummy[10];
    char ATport[26];
    char ip[26];
    _bg96_mutex.lock();
    _parser.set_timeout(BG96_60s_TO);
    if(_parser.send("AT+QSSLSTATE=%d", client_id))
    _parser.recv("+QSSLSTATE:%d,\"%[^\"]\",\"%[^\"]\",%d,%d,%d,%d,%d,%d,\"%[^\"]\",%d",
                        &id,dummy,ip,&remoteport,&localport,&socket_state,
                        &pdp_id,&server_id,&access_mode,ATport,&ssl_id);
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();
    // printf("TLSState: \r\n");
    // printf("Client ID: %d\r\n", id);
    // printf("Server IP: %s\r\n", ip);
    // printf("Remote Port: %d\r\n", remoteport);
    // printf("Local Port: %d\r\n", localport);
    // printf("Socket State: %d\r\n", socket_state);
    // printf("Access_mode: %d\r\n", access_mode);
    // printf("AT Port: %s\r\n", ATport);
    if (done && id == client_id && socket_state == 2) return true;
    return false;

}

int BG96::sslsend(int client_id, const void * data, uint32_t amount)
{
    int size=-1;
     
    _bg96_mutex.lock();
    _parser.set_timeout(BG96_TX_TIMEOUT);

    if ( _parser.send("AT+QSSLSEND=%d,%ld", client_id, amount) && _parser.recv(">") )
        size = _parser.write((char*)data, (int)amount);
    _parser.recv("SEND OK");
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();

    return size;
}

int BG96::sslsend(int client_id, const void * data, uint32_t amount, int timeout)
{
    int size = -1;
     
    _bg96_mutex.lock();
    _parser.set_timeout(timeout);

    _parser.send("AT+QSSLSEND=%d,%ld", client_id, amount);
    if (_parser.recv(">")) {
        size = _parser.write((char*)data, (int)amount);
        _parser.recv("SEND OK");
    }
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();

    return size;    
}

/** ----------------------------------------------------------
* @brief  check if RX data has arrived on TLS socket
* @param  client_id of BG96 TLS socket
* @retval true/false
*/
bool BG96::sslChkRxAvail(int client_id)
{
    char  cmd[20];
    sprintf(cmd, "+QSSLURC: \"recv\",%d", client_id);
    _parser.set_timeout(1);
    bool i = _parser.recv(cmd);
    _parser.set_timeout(BG96_AT_TIMEOUT);
    return i;
}

/** ----------------------------------------------------------
* @brief  receive data from BG96
* @param  id of BG96 socket
* @param  pointer to location to store returned data
* @param  count of the number of bytes to get
* @retval number of bytes returned or 0
*/
int32_t BG96::sslrecv(int client_id, void *data, uint32_t cnt)
{
    int  rxCount, ret_cnt=0;

    _bg96_mutex.lock();
    sslChkRxAvail(client_id);
    _parser.set_timeout(BG96_RX_TIMEOUT);
    if (_parser.send("AT+QSSLRECV=%d,%d",client_id,(int)cnt) && _parser.recv("+QSSLRECV:%d\r\n",&rxCount)){
        if ( rxCount > 0 ) {
                //_parser.getc(); //for some reason BG96 always outputs a 0x0A before the data
            ret_cnt = _parser.read((char*)data, rxCount);

            if( !_parser.recv("OK") ) 
                rxCount = NSAPI_ERROR_DEVICE_ERROR;
        } else {
            ret_cnt = rxCount;
        }  
    }
    
    
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();
    return ret_cnt;
}

/** ----------------------------------------------------------
* @brief  close the BG96 TLS socket
* @param  client_id of BG96 socket
* @retval true of close successful false on failure. <0 if error
*/
bool BG96::sslclose(int client_id)
{
    bool  done=false;

    _bg96_mutex.lock();
    _parser.set_timeout(BG96_60s_TO); //10s network response time
    done = (_parser.send("AT+QSSLCLOSE=%d,%d", client_id, BG96_CLOSE_TO) && _parser.recv("OK"));
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();
    return done;
}

int BG96::mqtt_open(const char* hostname, int port)
{
    int id=-1;
    int rc=-1;
    char cmd[80];

    sprintf(cmd, "AT+QMTOPEN=%d,\"%s\",%d", 0, hostname, port);

    _bg96_mutex.lock();
    _parser.set_timeout(75000);
    if (_parser.send(cmd) && _parser.recv("OK")) {
        _parser.recv("+QMTOPEN: %d,%d\r\n", &id, &rc);
    } else {
        char errstring[15];
        _parser.set_timeout(BG96_AT_TIMEOUT);
        _bg96_mutex.unlock();   
        if(getError(errstring)) sscanf(errstring, "Error:%d", &rc);
        return rc;
    }
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();
    return rc;
}

int BG96::mqtt_close()
{
    int id = -1;
    int rc=-1;

    _bg96_mutex.lock();
    if(_parser.send("AT+QMTCLOSE=0") && _parser.recv("OK"))
    {
        _parser.recv("+QMTCLOSE: %d,%d\r\n", &id, &rc);
    } else {
        rc = NSAPI_ERROR_TIMEOUT;
    }
    _bg96_mutex.unlock();
    return rc;
}

int BG96::send_generic_cmd(const char* cmd, int timeout)
{
    int rc=-NSAPI_ERROR_DEVICE_ERROR;
    if (cmd == NULL) return NSAPI_ERROR_PARAMETER;
    _bg96_mutex.lock();
    _parser.set_timeout(timeout);
    rc = _parser.send(cmd) && _parser.recv("OK");
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();
    if (rc < 0) {
        char errstring[15];
        if (getError(errstring)) sscanf(errstring, "Error:%d", &rc);
    }
    return rc;
}

int BG96::mqtt_connect(int sslctx_id, const char* clientid,
                                      const char* username,
                                      const char* password,
                                      ConnectResult &result)
{
    char cmd[256];

    int id=-1;
    int rc=-1;

    sprintf(cmd, "AT+QMTCONN=%d,\"%s\",\"%s\",\"%s\"", sslctx_id, clientid, username, password);
    _bg96_mutex.lock();
    _parser.set_timeout(10000);
    rc = _parser.send(cmd) && _parser.recv("OK");
    if (!rc) {
        char errstring[15];
        _parser.set_timeout(BG96_AT_TIMEOUT);
        _bg96_mutex.unlock();
        if (getError(errstring)) sscanf(errstring, "Error:%d", &result.rc);
        result.result = -1;
        return rc;
    } else {
        _parser.recv("+QMTCONN:%d,%d,%d", &id, &result.result, &result.rc);
    }
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();
    return rc;    
}

int BG96::mqtt_disconnect(int mqtt_id)
{
    int rc=-1;
    _bg96_mutex.lock();
    if (_parser.send("AT+QMTDISC=%d", mqtt_id) && _parser.recv("OK")) rc=NSAPI_ERROR_OK;
    _bg96_mutex.unlock();
    return rc;
}

 int BG96::mqtt_subscribe(int mqtt_id, const char* topic, int qos, int msg_id)
 {
    int rc=-1;
    _bg96_mutex.lock();
    _parser.set_timeout(15000);
    if (_parser.send("AT+QMTSUB=%d,%d,\"%s\",%d", mqtt_id, msg_id, topic, qos) && _parser.recv("OK")) rc = NSAPI_ERROR_OK;
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();
    return rc;
 }

 int BG96::mqtt_unsubscribe(int mqtt_id, const char* topic, int msg_id)
 {
    int rc=-1;
    _bg96_mutex.lock();
    _parser.set_timeout(15000);
    if (_parser.send("AT+QMTUNS=%d,%d,\"%s\"", mqtt_id, msg_id, topic) && _parser.recv("OK")) rc = NSAPI_ERROR_OK;
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();
    return rc;  
 }

int BG96::mqtt_publish(int mqtt_id, int msg_id, int qos, int retain, const char* topic, const void* data, int amount)
{
    int rc=-1;
    int id, mid, res;
    bool done;
     
    _bg96_mutex.lock();
    _parser.set_timeout(BG96_TX_TIMEOUT);

    done = _parser.send("AT+QMTPUB=%d,%d,%d,%d,\"%s\"", mqtt_id, msg_id, qos, retain, topic); 
    if( done && _parser.recv(">") ) {
        done = (_parser.write((char*)data, (int)amount) <= 0 && _parser.recv("OK"));
    } else { 
        rc=-1;
        _parser.set_timeout(BG96_AT_TIMEOUT);
        _bg96_mutex.unlock();
        return rc;
    }
    if ( done && _parser.recv("+QMTPUB: %d,%d,%d", &id, &mid, &res) && res == 0 ) rc = 1;
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();
    return rc;
}

void* BG96::mqtt_checkAvail(int mqtt_id)
{
    MQTTMessage msg;
    int id, msg_id;
    char payload[256];
    char topic[80];
    _parser.set_timeout(1);
    int i = _parser.recv("+QMTRECV: %d,%d,\"[^\"]\",[^[\r\n]]", &id, &msg_id, topic, payload);
    if (i && id == mqtt_id) {
        msg.topic.len = strlen(topic);
        msg.topic.payload = topic;
        msg.msg.len = strlen(payload);
        msg.msg.payload = payload;
        _parser.set_timeout(BG96_AT_TIMEOUT);
        return (void *)&msg;
    } else {
        _parser.set_timeout(BG96_AT_TIMEOUT);
        return NULL;
    }
}

void* BG96::mqtt_recv(int mqtt_id)
{
    MQTTMessage* msg = (MQTTMessage *)mqtt_checkAvail(mqtt_id);
    return (void *)&msg;
}