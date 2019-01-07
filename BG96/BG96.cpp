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

#define BG96_1s_WAIT            1000   //will wait for 1 second for startup
#define BG96_60s_TO             60000  //wait 60 seconds
#define BG96_150s_TO            150000 //wait 150s (2.5 mins)
#define BG96_TX_TIMEOUT         2000   //time before a TX timeout occurs
#define BG96_RX_TIMEOUT         1000   //time before a TX timeout occurs
#define BG96_WAIT4READY         15000  //wait 15 seconds for 'RDY' after reset
#define BG96_AT_TIMEOUT         1000   //standard AT command timeout
#define BG96_WRK_CONTEXT        1      //we will only use context 1 in driver
#define BG96_CLOSE_TO           1      //wait x seconds for a socket close
#define BG96_MISC_TIMEOUT       1000

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
    _contextID(1), 
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
    if( tx2bg96((char*)"ATE0") )
        done = tx2bg96((char*)"AT+COPS?");
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
    Timer timer_s;
//	uint32_t time;
//	int pdp_retry = 0;
	
    _bg96_mutex.lock();
    memset(pdp_string, 0, sizeof(pdp_string));
	printf("Checking APN ...\r\n");
	_parser.send("AT+QICSGP=1");
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
		if (!(_parser.send("AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",0", &apn[0], &username[0], &password[0])
        && _parser.recv("OK"))) 
		{
			return NSAPI_ERROR_DEVICE_ERROR;
		}
	}
    wait(1);
	printf("End APN check\r\n\n");
		
	//activate PDP context 1 ...
    char cmd[100];
    printf("PDP activating ...\r\n");
    sprintf(cmd,"AT+QIACT=%d", 1);
    timer_s.reset();
    bool done=false;
    while( !done && timer_s.read_ms() < BG96_150s_TO ) 
        done = tx2bg96(cmd);
    if (done) printf("PDP started\r\n\n");
    _bg96_mutex.unlock();
	// int a = 1;
	// while(a==1)
	// {
	// 	_parser.send("AT+QIACT=1");
	// 	timer_s.reset();
	// 	timer_s.start();
	// 	while (1)
	// 	{
	// 		if (_parser.recv("OK")){
	// 			a=0;
	// 			break;
	// 		}
						
	// 		time = timer_s.read_ms();
	// 		uint32_t end_time = (BG96_MISC_TIMEOUT*(5+(pdp_retry*3)));
	// 		if (time > end_time) 
	// 		{
	// 			pdp_retry++;
	// 			if (pdp_retry > 3)
	// 			{
	// 				return NSAPI_ERROR_DEVICE_ERROR;
	// 			}
	// 			break;
	// 		}
	// 	}		
	// }
	
    return done ? NSAPI_ERROR_OK:NSAPI_ERROR_DEVICE_ERROR;

    // char cmd[100],_apn[50];
    // bool done = false;
    // Timer t;
    // int   cntx;
    
    // _bg96_mutex.lock();
    // t.start();
    // do {
    //     _parser.send("AT+QICSGP=%d",_contextID);
    //     done = _parser.recv("+QICSGP: %d,\"%50[^\"]\"",&cntx, _apn);
    //     printf("[BG96]: APN to find is %s.\r\n", apn);
    //     printf("[BG96]: Found %s APN in PDP context.\r\n", _apn);
    //     wait_ms(2);
    //     }
    // while( !done && t.read_ms() < BG96_60s_TO );

    // if( !done ) {
    //     _bg96_mutex.unlock();
    //     return NSAPI_ERROR_DEVICE_ERROR;
    //     }

    // _parser.flush();    
    // if( strcmp(_apn,apn) == 0 ) {
    //     printf("[BG96]: APN of PDP context is correct.\r\n");
    //     sprintf(cmd,"AT+QICSGP=%d,1,\"%s\",\"%s\",\"%s\",0", _contextID, &apn[0], &username[0], &password[0]);
    //     if( !tx2bg96(cmd) )  {
    //         _bg96_mutex.unlock();
    //         return NSAPI_ERROR_DEVICE_ERROR;
    //         }
    //     }

    // sprintf(cmd,"AT+QIACT=%d", _contextID);
    // t.reset();
    // done=false;
    // while( !done && t.read_ms() < BG96_150s_TO ) 
    //     done = tx2bg96(cmd);

    // _bg96_mutex.unlock();
    
    // return done? NSAPI_ERROR_OK : NSAPI_ERROR_DEVICE_ERROR;
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
    bool ok;
    int  err, ipcount, dnsttl;
    
    _bg96_mutex.lock();
    _parser.flush();
    _parser.set_timeout(BG96_60s_TO);
    ok =  (    _parser.send("AT+QIDNSGIP=%d,\"%s\"",_contextID,name)
            && _parser.recv("OK") 
            && _parser.recv("+QIURC: \"dnsgip\",%d,%d,%d",&err, &ipcount, &dnsttl) 
            && err==0 
            && ipcount > 0
          );

    if( ok ) {
        ok = _parser.recv("+QIURC: \"dnsgip\",\"%[^\"]\"",ipstr);       //use the first DNS value
        for( int i=0; i<ipcount-1; i++ )
            _parser.recv("+QIURC: \"dnsgip\",\"%[^\"]\"", buf2);   //and discrard the rest  if >1
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
    int   cs, er;
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
    int   cs, ct;
    bool  done=false;

    _bg96_mutex.lock();
    _parser.set_timeout(BG96_150s_TO);
    done = _parser.send("AT+QIACT?") && _parser.recv("+QIACT: 1, %d,%d,\"%16[^\"]\"",&cs,&ct,ipstr) && _parser.recv("OK");
    _parser.set_timeout(BG96_AT_TIMEOUT);
    _bg96_mutex.unlock();
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
    char ip[25];
    return getIPAddress(ip) != NULL;
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
    int state;
    bool done;
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
    done = (_parser.send("AT+QGPSLOC=2") && _parser.recv("+QGPSLOC: %s\r\n", locationstring));  
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
    bool upload = false;
    int good = 0;
    unsigned int upload_size;
    unsigned int checksum;
    size_t filesize = strlen(content) + 1;
    char cmd[128];
    sprintf(cmd, "AT+QFLST=\"%s\"", filename);
    _bg96_mutex.lock();
    _parser.set_timeout(BG96_1s_WAIT);
    done = _parser.send(cmd) && _parser.recv("OK");
    if (done) { // File exists
        if (overrideok) {
            sprintf(cmd, "AT+QFDEL=\"%s\"", filename);
            done = _parser.send(cmd) && _parser.recv("OK");
            if (done) {
                upload = true;
            } else {
                printf("BG96: Error trying to delete file %s\r\n", filename);
                 _bg96_mutex.unlock();
                return 0;
            }
        } else {
            _bg96_mutex.unlock();
            return 1; // we suppose the file that is here is ok
        }
    }
    _bg96_mutex.unlock();
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
    return 0;
}

