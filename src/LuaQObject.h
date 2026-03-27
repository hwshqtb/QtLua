#pragma once

#include <QtCore/QObject>
#include "LuaHelper.h"

struct LuaQObject {
    QObject* object;
    QMetaObject* mo;
    Ownership ownership;
};
