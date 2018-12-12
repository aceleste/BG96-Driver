#ifndef __GNSSLOC_H__
#define __GNSSLOC_H__

#include "mbed.h"
class COG
{
public:
    COG();
    ~COG();
    void setDegrees(int degrees) { degrees = degrees; };
    void setMinutes(int minutes) { minutes = minutes; };
    int getDegrees() { return degrees; };
    int getMinutes() { return minutes; };

private:
    int degrees;
    int minutes;
};

class GNSSLoc
{
public:
    GNSSLoc(void);
    GNSSLoc(const GNSSLoc& loc);
    GNSSLoc(char* locstring);    
    ~GNSSLoc();
    GNSSLoc& operator=(const GNSSLoc& rhs) {};
    time_t getGNSSTime() {return gnssutctime;};
    float getGNSSLatitude() {return latitude; };
    float getGNSSLongitude() {return longitude; };
    float getGNSSHdop() { return hdop; };
    float getGNSSAltitude() { return altitude; };
    int getGNSSFix() { return fix ; };
    COG* getGNSSCog() { return &cog; };
    float getGNSSSPKM() { return spkm; };
    float getGNSSSPKN() { return spkn; };

private:
    time_t gnssutctime;
    float latitude;
    float longitude;
    float hdop;
    float altitude;
    int fix;
    COG cog;
    float spkm;
    float spkn;
    int nsat;
};

#endif //__GNSSLOC_H__