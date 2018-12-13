#ifndef __GNSS_INTERFACE_H__ 
#define __GNSS_INTERFACE_H__
#include "GNSSLoc.h"

class GNSSInterface
{
public:
    virtual ~GNSSInterface() {};

    virtual GNSSLoc *getGNSSLocation(); 

};

#endif //__GNSS_INTERFACE_H__