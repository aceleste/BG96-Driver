/**
 * @file GNSSLoc.cpp
 * @author Alain CELESTE (alain.celeste@polaris-innovation.com)
 * @brief 
 * @version 0.1
 * @date 2019-08-14
 * 
 * @copyright Copyright (c) 2019 Polaris Innovation
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