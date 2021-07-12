#pragma once

#include "Setting.h"
#include "Scheme.h"
#include "Replication.h"
#include "Replication_Log.h"
#include "FullLog.h"
#include "ParityLog.h"
#include "DABRI.h"

class SchemeFactory {
public:
    static Scheme* createScheme(string scheme);
};
