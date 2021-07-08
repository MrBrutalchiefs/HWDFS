#include "SchemeFactory.h"

Scheme* SchemeFactory::createScheme(string scheme) {
    if(scheme == "FO") {
        return new Scheme();
    } else if(scheme == "FL") {
        return new FullLog();
    } else if(scheme == "PL") {
        return new ParityLog();
    } else if(scheme == "REPLICATION") {
        return new Replication();
    } else if(scheme == "REPLICATION_LOG") {
        return new ReplicationLog();
    } else if(scheme == "PARIX") {
        return new PARIX();
    } else if(scheme == "PARIX_LOG") {
        return new PARIXLog()
    } else if(scheme == "DDBHW") {
        return new DDBHW();
    } else if(scheme == "PWDR") {
        return new PWDR();
    }
}