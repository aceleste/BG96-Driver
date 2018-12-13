#include "GNSSLoc.h"
#include "mbed_mktime.h"

COG::COG()
{}

COG::~COG()
{}

GNSSLoc::GNSSLoc()
{
}

GNSSLoc::~GNSSLoc()
{}


GNSSLoc::GNSSLoc(const GNSSLoc& loc)
{
    gnssutctime     = loc.gnssutctime;
    latitude        = loc.latitude;
    longitude       = loc.longitude;
    hdop            = loc.hdop;
    altitude        = loc.altitude;
    fix             = loc.fix;
    spkm            = loc.spkm;
    spkn            = loc.spkn;
    nsat            = loc.nsat;
    cog             = loc.cog;
}

GNSSLoc::GNSSLoc(const char* locationstring)
{
    int cogdeg, cogmin;
    int dummy;
    tm time;
    int shortyear;
    sscanf(locationstring, "%2d%2d%2d.%d,%f,%f,%f,%f,%d,%d.%d,%f,%f,%2d%2d%2d,%d",
        &(time.tm_hour), &(time.tm_min), &(time.tm_sec), &dummy, 
        &latitude, 
        &longitude, 
        &hdop, 
        &altitude, 
        &fix, 
        &cogdeg, &cogmin, 
        &spkm, 
        &spkn, 
        &(time.tm_mday), &(time.tm_mon), &shortyear, 
        &nsat);
    time.tm_mon = time.tm_mon - 1;
    time.tm_year = shortyear + 100;
    cog.setDegrees(cogdeg);
    cog.setMinutes(cogmin);
    _rtc_maketime(&time, &gnssutctime, RTC_4_YEAR_LEAP_YEAR_SUPPORT);
}