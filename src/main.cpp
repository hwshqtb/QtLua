#include <QtCore/QCoreApplication>
#include <QtCore/QObject>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaMethod>
#include <QtCore/QDebug>
#include <lua.hpp>
#include "header.h"

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

static LuaQObject* check_qobject(lua_State* L, int idx) {
    void* ud = luaL_checkudata(L, idx, "QObject");
    if (!ud) luaL_error(L, "expected QObject");
    return static_cast<LuaQObject*>(ud);
}

static const char* check_qmethod(lua_State* L, int idx) {
    void* ud = luaL_checkudata(L, idx, "QtMethod");
    if (!ud) luaL_error(L, "expected QtMethod");
    return static_cast<const char*>(ud);
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
        storage.variantValue = lua_to_qvariant(L, stackIndex);
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

static int lua_qml_create(lua_State* L) {
    const char* typeName = luaL_checkstring(L, 1);
    auto mo = g_dynamicMetaObjects.value(typeName, nullptr);
    if (!mo) {
        return luaL_error(L, "type '%s' is not registered", typeName);
    }
    
    QObject* obj = mo->newInstance();
    LuaQObject* ud = static_cast<LuaQObject*>(lua_newuserdata(L, sizeof(LuaQObject)));
    ud->object = obj;
    ud->ownership = OwnerShip::LuaOwnership;
    luaL_getmetatable(L, "QObject");
    lua_setmetatable(L, -2);

    // 为 QObject userdata 创建一个 uservalue 表，用于存放 Lua 侧动态字段（如信号回调）
    lua_newtable(L);
    lua_setuservalue(L, -2);

    return 1;
}

static int lua_qobject_connect(lua_State* L) {
    LuaQObject* obj = check_qobject(L, 1);
    const char* signalSig;
    if (lua_isuserdata(L, 2)) {
        signalSig = check_qmethod(L, 2);
    } else {
        signalSig = luaL_checkstring(L, 2);
    }
    if (lua_isfunction(L, 3)) {
        lua_pushvalue(L, 3);
        int fnRef = luaL_ref(L, LUA_REGISTRYINDEX);

        int signalIndex = obj->object->metaObject()->indexOfSignal(signalSig);
        if (signalIndex < 0) {
            // fallback: match by name for overloaded signal variants
            int mcount = obj->object->metaObject()->methodCount();
            for (int i = 0; i < mcount; ++i) {
                QMetaMethod m = obj->object->metaObject()->method(i);
                if (m.methodType() == QMetaMethod::Signal && QString(m.name()) == QString(signalSig)) {
                    signalIndex = i;
                    break;
                }
            }
        }
        if (signalIndex < 0) {
            return luaL_error(L, "signal '%s' not found", signalSig);
        }
        QMetaMethod signalMethod = obj->object->metaObject()->method(signalIndex);

        LuaSlotProxy* proxy = new LuaSlotProxy(LuaFunction{L, fnRef}, signalMethod, obj->object);

        int slotIndex = QObject::staticMetaObject.methodCount();
        if (!QMetaObject::connect(obj->object, signalIndex, proxy, slotIndex)) {
            luaL_error(L, "failed to connect signal");
            return 0;
        }

        lua_pushboolean(L, true);
    }
    else if (lua_isuserdata(L, 3)) {

    }
    return 1;
}

static int lua_qobject_gc(lua_State* L) {
    LuaQObject* obj = check_qobject(L, 1);
    if (obj) {
        if (obj->ownership == OwnerShip::LuaOwnership) {
            obj->object->deleteLater();
        }
    }
    return 0;
}

static int lua_qobject_tostring(lua_State* L) {
    LuaQObject* obj = check_qobject(L, 1);
    lua_pushfstring(L, "<QObject: %p>", obj->object);
    return 1;
}

static int lua_qt_object_index(lua_State* L) {
    LuaQObject* obj = check_qobject(L, 1);
    const char* key = luaL_checkstring(L, 2);

    // 从 uservalue 中查找（Lua 侧动态字段）
    lua_getuservalue(L, 1);
    if (lua_istable(L, -1)) {
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);
        if (!lua_isnil(L, -1)) {
            return 1; // 返回 uservalue 中的值
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    // Check if it's a special method "connect"
    if (strcmp(key, "connect") == 0) {
        lua_pushcfunction(L, lua_qobject_connect);
        return 1;
    }
    // Check if it's a property
    if (obj->object->metaObject()->indexOfProperty(key) >= 0) {
        QVariant value = obj->object->property(key);
        lua_push_qvariant(L, value);
        return 1;
    }
    // Check if it's a method
    int methodCount = obj->object->metaObject()->methodCount();
    for (int i = 0; i < methodCount; ++i) {
        QMetaMethod method = obj->object->metaObject()->method(i);
        if (QString(method.name()) == key) {
            const QByteArray name = method.name();
            size_t len = name.size() + 1;
            char* ud = static_cast<char*>(lua_newuserdata(L, len));
            memcpy(ud, name.constData(), len);
            luaL_getmetatable(L, "QtMethod");
            lua_setmetatable(L, -2);
            return 1;
        }
    }
    lua_pushnil(L);
    return 1;
}

static int lua_method_caller(lua_State* L) {
    // L[1] = methodName, L[2] = obj, L[3+] = args
    LuaQObject* obj = check_qobject(L, 2);
    const char* methodName = check_qmethod(L, 1);

    int argCount = lua_gettop(L) - 2;
    const QMetaObject* mo = obj->object->metaObject();
    QMetaMethod matchedMethod;
    QGenericArgument argArray[10];
    LuaQtArgStorage argStorage[10];
    bool found = false;

    for (int i = 0; i < mo->methodCount(); ++i) {
        QMetaMethod method = mo->method(i);
        if (QString(method.name()) != QString(methodName)) continue;
        if (method.parameterCount() != argCount) continue;

        QGenericArgument testArgs[10];
        LuaQtArgStorage testStorage[10];
        if (!prepare_method_arguments(L, 3, method, testArgs, testStorage)) {
            continue;
        }

        matchedMethod = method;
        for (int j = 0; j < 10; ++j) {
            argArray[j] = testArgs[j];
            argStorage[j] = testStorage[j];
        }
        found = true;
        break;
    }

    if (!found) {
        return luaL_error(L, "method '%s' with %d args not found", methodName, argCount);
    }

    bool ok = false;
    QVariant result;

    int retType = matchedMethod.returnType();
    LuaQtArgStorage returnStorage;
    QGenericReturnArgument returnArg;

    if (retType != QMetaType::Void) {
        switch (retType) {
        case QMetaType::Int:
            returnStorage.intValue = 0;
            returnArg = QGenericReturnArgument("int", &returnStorage.intValue);
            break;
        case QMetaType::Bool:
            returnStorage.boolValue = false;
            returnArg = QGenericReturnArgument("bool", &returnStorage.boolValue);
            break;
        case QMetaType::Double:
            returnStorage.doubleValue = 0.0;
            returnArg = QGenericReturnArgument("double", &returnStorage.doubleValue);
            break;
        case QMetaType::QString:
            returnStorage.stringValue.clear();
            returnArg = QGenericReturnArgument("QString", &returnStorage.stringValue);
            break;
        case QMetaType::QVariant:
            returnStorage.variantValue.clear();
            returnArg = QGenericReturnArgument("QVariant", &returnStorage.variantValue);
            break;
        default:
            return luaL_error(L, "unsupported return type for method '%s'", methodName);
        }

        ok = matchedMethod.invoke(obj->object, Qt::AutoConnection,
                                  returnArg,
                                  argArray[0], argArray[1], argArray[2], argArray[3], argArray[4],
                                  argArray[5], argArray[6], argArray[7], argArray[8], argArray[9]);
    } else {
        ok = matchedMethod.invoke(obj->object, Qt::AutoConnection,
                                  argArray[0], argArray[1], argArray[2], argArray[3], argArray[4],
                                  argArray[5], argArray[6], argArray[7], argArray[8], argArray[9]);
    }

    if (!ok) {
        return luaL_error(L, "failed to invoke method '%s'", methodName);
    }

    if (retType != QMetaType::Void) {
        result = convert_return_value(retType, returnStorage);
        if (result.isValid()) {
            lua_push_qvariant(L, result);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

static int lua_qmetaobject_new(lua_State* L) {
    const char* typeName = lua_tostring(L, lua_upvalueindex(1));
    lua_pushcfunction(L, lua_qml_create);
    lua_pushstring(L, typeName);
    lua_call(L, 1, 1);
    return 1;
}

void registerQtTypeInLua(lua_State* L, const QMetaObject* mo, const char* typeName = nullptr) {
    if (!typeName) {
        typeName = mo->className();
    }
    // Register in C++
    registerQtType(typeName, mo);

    // Set up in Lua
    lua_getglobal(L, "qt");
    lua_pushstring(L, typeName);
    lua_newtable(L);
    lua_pushstring(L, "new");
    lua_pushstring(L, typeName);
    lua_pushcclosure(L, lua_qmetaobject_new, 1);
    lua_settable(L, -3);
    lua_settable(L, -3);
    lua_pop(L, 1);
}

static int lua_qt_object_newindex(lua_State* L) {
    LuaQObject* obj = check_qobject(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (obj->object->metaObject()->indexOfProperty(key) >= 0) {
        QVariant value = lua_to_qvariant(L, 3);
        obj->object->setProperty(key, value);
        return 0;
    }

    // 非属性：存放到 userdata 的 uservalue（Lua 侧扩展字段）
    lua_getuservalue(L, 1);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
    }
    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    lua_rawset(L, -3);
    lua_setuservalue(L, 1);

    return 0;
}

static void registerQObjectType(lua_State* L) {
    luaL_newmetatable(L, "QObject");

    lua_pushcfunction(L, lua_qobject_gc);
    lua_setfield(L, -2, "__gc");

    lua_pushcfunction(L, lua_qobject_tostring);
    lua_setfield(L, -2, "__tostring");

    lua_pushcfunction(L, lua_qt_object_newindex);
    lua_setfield(L, -2, "__newindex");

    lua_pushcfunction(L, lua_qt_object_index);
    lua_setfield(L, -2, "__index");

    lua_pop(L, 1);
}

static void registerQtMethodType(lua_State* L) {
    luaL_newmetatable(L, "QtMethod");

    lua_pushcfunction(L, lua_method_caller);
    lua_setfield(L, -2, "__call");

    lua_pushcfunction(L, lua_qobject_tostring);
    lua_setfield(L, -2, "__tostring");

    lua_pop(L, 1);
}

static void lua_qmetaobject_call(lua_State* L) {

}

static void lua_qmetaobject_index(lua_State* L) {
}

static void lua_qmetaobject_newindex(lua_State* L) {
}

static void lua_qmetaobject_gc(lua_State* L) {
}

static void registerQMetaObject(lua_State* L) {
    luaL_newmetatable(L, "QMetaObject");
    
    lua_pushcfunction(L, lua_qmetaobject_call);
    lua_setfield(L, -2, "__call");
    
    lua_pushcfunction(L, lua_qmetaobject_new);
    lua_setfield(L, -2, "new");
    
    lua_pop(1);
}

extern "C" int luaopen_qtbridge(lua_State* L) {
    registerQObjectType(L);
    registerQtMethodType(L);

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
    registerQtTypeInLua(L, &MyDynamicObject::staticMetaObject);

    const char* script = R"(
local obj = qt.MyDynamicObject:new()
obj.value = 123
print('obj value', obj.value)

function obj.onValueChanged(newValue)
    print('Signal received: value changed to', newValue)
end
obj:connect(obj.valueChanged, obj.onValueChanged)
print("1234")
obj:valueChanged(456.0)
obj:doSomething()
print('Signal test completed')
print('Lua Qt bridge test completed successfully')
)";

    if (luaL_dostring(L, script) != LUA_OK) {
        qCritical() << "Lua error:" << lua_tostring(L, -1);
        lua_pop(L, 1);
    }

    lua_close(L);
    // qDeleteAll(g_dynamicMetaObjects);
    g_dynamicMetaObjects.clear();

    return 0;
}