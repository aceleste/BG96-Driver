#include "GNSSLoc.h"
#include "mbed_mktime.h"

GNSSLoc::GNSSLoc()
{
}

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
    cog.setDegrees(loc.cog.getDegrees);
    cog.setMinutes(loc.cog.getMinutes);
}

GNSSLoc::GNSSLoc(const char* locationstring)
{
    char datestr[6];
    int cogdeg, cogmin;
    int dummy;
    tm time;
    sscanf(locationstring, "%2d%2d%2d.%d,%.5f,%.5f,%.1f,%.1f,%d,%d.%d,%.1f,%.1f,%2d%2d%2d,%d",
        time.tm_hour, time.tm_min, time.tm_sec, dummy, latitude, longitude, hdop, altitude, fix, cogdeg, cogmin, spkm, spkn, datestr, nsat);
    cog.setDegrees(cogdeg);
    cog.setMinutes(cogmin);
    _rtc_mktime(&time, &gnssutctime, RTC_4_YEAR_LEAP_YEAR_SUPPORT);
}