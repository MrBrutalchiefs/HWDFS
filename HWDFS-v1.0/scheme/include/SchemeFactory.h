#pragma once

#include "Setting.h"
#include "Scheme.h"
#include "Replication.h"
#include "Replication_Log.h"
#include "FullLog.h"
#include "ParityLog.h"
#include "PARIX.h"
#include "PARIXLog.h"
#include "PWDR.h"
#include "DDBHW.h"

class SchemeFactory {
public:
    static Scheme* createScheme(string scheme);
};