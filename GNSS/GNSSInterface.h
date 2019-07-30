#ifndef __GNSS_INTERFACE_H__ 
#define __GNSS_INTERFACE_H__
#include "GNSSLoc.h"

class GNSSInterface
{
public:
	virtual ~GNSSInterface() {}

    virtual bool getGNSSLocation(GNSSLoc& loc)=0;

    virtual bool initializeGNSS(void)=0;

};

#endif //__GNSS_INTERFACE_H__
