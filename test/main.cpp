#include <QtCore/QCoreApplication>
#include <QtCore/QObject>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaMethod>
#include <QtCore/QDebug>
#include <lua.hpp>
#include "QLuaEngine.h"

#include "test.h"

static QHash<QString, const QMetaObject*> g_dynamicMetaObjects;

void registerQtType(const char* typeName, const QMetaObject* mo) {
    g_dynamicMetaObjects.insert(typeName, mo);
}

static int lua_qt_object_newindex(lua_State* L);

static int lua_qml_register_type(lua_State* L) {
    const char* typeName = luaL_checkstring(L, 1);
    g_dynamicMetaObjects.insert(typeName, &QObject::staticMetaObject);
    return 0;
}

struct LuaQtArgStorage {
    int intValue;
    bool boolValue;
    double doubleValue;
    QString stringValue;
    QVariant variantValue;
};

static bool lua_to_method_arg(lua_State* L, int stackIndex, int typeId, LuaQtArgStorage& storage, QGenericArgument& argument) {
    switch (typeId) {
    case QMetaType::Int:
        storage.intValue = static_cast<int>(lua_tointeger(L, stackIndex));
        argument = QGenericArgument("int", &storage.intValue);
        return true;
    case QMetaType::Bool:
        storage.boolValue = lua_toboolean(L, stackIndex);
        argument = QGenericArgument("bool", &storage.boolValue);
        return true;
    case QMetaType::Double:
        storage.doubleValue = lua_tonumber(L, stackIndex);
        argument = QGenericArgument("double", &storage.doubleValue);
        return true;
    case QMetaType::QString:
        if (!lua_isstring(L, stackIndex)) return false;
        storage.stringValue = QString::fromUtf8(lua_tostring(L, stackIndex));
        argument = QGenericArgument("QString", &storage.stringValue);
        return true;
    case QMetaType::QVariant:
        storage.variantValue = LuaHelper::lua_to_qvariant(L, stackIndex);
        argument = QGenericArgument("QVariant", &storage.variantValue);
        return true;
    default:
        return false;
    }
}

static bool prepare_method_arguments(lua_State* L, int firstArgIndex, const QMetaMethod& method,
                                     QGenericArgument argArray[10], LuaQtArgStorage argStorage[10]) {
    int paramCount = method.parameterCount();
    if (paramCount > 10) return false;
    for (int i = 0; i < 10; ++i) {
        argArray[i] = QGenericArgument();
    }
    for (int i = 0; i < paramCount; ++i) {
        int typeId = method.parameterType(i);
        if (typeId == QMetaType::Void || typeId == QMetaType::UnknownType) return false;
        if (!lua_to_method_arg(L, firstArgIndex + i, typeId, argStorage[i], argArray[i])) {
            return false;
        }
    }
    return true;
}

static QVariant convert_return_value(int typeId, const LuaQtArgStorage& storage) {
    switch (typeId) {
    case QMetaType::Int:
        return QVariant(storage.intValue);
    case QMetaType::Bool:
        return QVariant(storage.boolValue);
    case QMetaType::Double:
        return QVariant(storage.doubleValue);
    case QMetaType::QString:
        return QVariant(storage.stringValue);
    case QMetaType::QVariant:
        return storage.variantValue;
    default:
        return QVariant();
    }
}

extern "C" int luaopen_qtbridge(lua_State* L) {
    LuaQObject::registerMetaTable(L);
    LuaQtMethod::registerMetaTable(L);
    LuaQMetaObject::registerMetaTable(L);

    lua_newtable(L);
    return 1;
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    luaL_requiref(L, "qtbridge", luaopen_qtbridge, 1);
    lua_setglobal(L, "qt");

    // Register MyDynamicObject
    LuaQMetaObject::push(L, &QObject::staticMetaObject);
    LuaQMetaObject::push(L, &MyDynamicObject::staticMetaObject);

    if (luaL_dofile(L, "test.lua") != LUA_OK) {
        qCritical() << "Lua error:" << lua_tostring(L, -1);
        lua_pop(L, 1);
    }

    lua_close(L);
    // qDeleteAll(g_dynamicMetaObjects);
    g_dynamicMetaObjects.clear();

    return 0;
}