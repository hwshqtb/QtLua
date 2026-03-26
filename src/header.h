#pragma once

#include <QObject>
#include <lua.hpp>
#include <QtCore/QMetaMethod>

/**
 *  predefinitions for Lua bindings
 */
struct LuaQObject;
struct LuaQMetaObject;
struct LuaFunction;

static QVariant lua_to_qvariant(lua_State* L, int idx);

static void lua_push_qvariant(lua_State* L, const QVariant& value);

class MyDynamicObject: public QObject {
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)

public:
    Q_INVOKABLE MyDynamicObject(QObject* parent = nullptr):
        QObject(parent),
        m_value(0) {}

    int value() const {
        return m_value;
    }

    void setValue(int v) {
        if (m_value != v) {
            m_value = v;
            emit valueChanged(static_cast<double>(v));
        }
    }

    Q_INVOKABLE void doSomething() {
        qDebug() << "doSomething called";
    }

signals:
    void valueChanged(double value);

private:
    int m_value;
};

enum struct OwnerShip {
    CppOwnership,
    LuaOwnership
};

struct LuaQObject {
    QObject* object;
    OwnerShip ownership;
};

class QLuaItem: public QObject {
public:
    QLuaItem(QObject* parent = nullptr):
        QObject(parent) {}

    const QMetaObject* metaObject() const override {
        return mo ? mo : QObject::metaObject();
    }

    void setMetaObject(const QMetaObject* m) {
        mo = m;
    }

private:
    const QMetaObject* mo;
};

struct LuaQMetaObject {
    QMetaObject* mo;
    OwnerShip ownership;
};

struct LuaFunction {
    lua_State* L;
    int ref;

    QVariantList call(const QVariantList& args, int nresults = 0) const {
        if (ref == LUA_NOREF) {
            return {};
        }
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        for (const QVariant& arg : args) {
            lua_push_qvariant(L, arg);
        }
        if (lua_pcall(L, args.size(), nresults, 0) != LUA_OK) {
            const char* errorMsg = lua_tostring(L, -1);
            qWarning() << "Error calling Lua function:" << errorMsg;
            lua_pop(L, 1);
            return {};
        }
        QVariantList results;
        for (int i = 0; i < nresults; ++i) {
            results.append(lua_to_qvariant(L, -nresults + i));
        }
        lua_pop(L, nresults);
        return results;
    }
};

Q_DECLARE_METATYPE(LuaFunction)

class LuaSlotProxy: public QObject {
public:
    LuaSlotProxy(LuaFunction callback, QMetaMethod m, QObject* parent = nullptr):
        QObject(parent),
        _callback(callback),
        _m(m) {
        lua_rawgeti(_callback.L, LUA_REGISTRYINDEX, _callback.ref);
        _callback.ref = luaL_ref(_callback.L, LUA_REGISTRYINDEX);
    }

    ~LuaSlotProxy() {
        if (_callback.ref != LUA_NOREF) {
            luaL_unref(_callback.L, LUA_REGISTRYINDEX, _callback.ref);
        }
    }

    int qt_metacall(QMetaObject::Call call, int methodId, void** a) override {
        methodId = QObject::qt_metacall(call, methodId, a);
        if (methodId < 0)
            return methodId;

        if (call == QMetaObject::InvokeMetaMethod) {
            if (methodId == 0) {
                QVariantList args;
                for (int i = 0; i < _m.parameterCount(); ++i) {
                    int typeId = _m.parameterType(i);
                    args.append(QVariant::fromMetaType(QMetaType(typeId), a[i + 1]));
                }
                _callback.call(args);
            }
            --methodId;
        }
        return methodId;
    }

private:
    LuaFunction _callback;
    QMetaMethod _m;
};

// class Test: public QObject {
//     Q_OBJECT
//     Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

// public:
//     Test(QObject* parent = nullptr):
//         QObject(parent) {}

//     QString name() const {
//         return _name;
//     }

//     Q_INVOKABLE void setName(const QString& name) {
//         _name = name;
//     }

// public slots:
//     void setTestName(const QString& name) {
//         setName(name);
//     }

// signals:
//     void nameChanged(const QString& name);

// private:
//     QString _name;
// };

class QLuaValue {
public:
    QLuaValue(lua_State* L, int ref = LUA_NOREF):
        _L(L),
        _ref(ref) {}

    ~QLuaValue() {
        if (_ref != LUA_NOREF) {
            luaL_unref(_L, LUA_REGISTRYINDEX, _ref);
        }
    }

    bool isNil() const {
        if (_ref == LUA_NOREF) return true;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_isnil(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

    bool isBoolean() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_isboolean(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

    bool toBoolean() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_toboolean(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

    bool isNumber() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_isnumber(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

    double toNumber() const {
        if (_ref == LUA_NOREF) return 0.0;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        double result = lua_tonumber(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

    bool isString() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_isstring(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

    QString toString() const {
        if (_ref == LUA_NOREF) return QString();
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        QString result = QString::fromUtf8(lua_tostring(_L, -1));
        lua_pop(_L, 1);
        return result;
    }

    bool isUserData() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_isuserdata(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

    bool isQObject() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = luaL_checkudata(_L, -1, "QObject") != nullptr;
        lua_pop(_L, 1); // pop the original value
        return result;
    }

    QObject* toQObject() const {
        if (!isQObject()) return nullptr;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        LuaQObject* ud = static_cast<LuaQObject*>(luaL_checkudata(_L, -1, "QObject"));
        QObject* obj = ud->object;
        lua_pop(_L, 1);
        return obj;
    }

    bool isQMetaObject() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = luaL_checkudata(_L, -1, "QMetaObject") != nullptr;
        lua_pop(_L, 1); // pop the original value
        return result;
    }

    const QMetaObject* toQMetaObject() const {
        if (!isQMetaObject()) return nullptr;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        LuaQMetaObject* ud = static_cast<LuaQMetaObject*>(luaL_checkudata(_L, -1, "QMetaObject"));
        const QMetaObject* mo = ud->mo;
        lua_pop(_L, 1);
        return mo;
    }

    bool isFunction() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_isfunction(_L, -1);
        lua_pop(_L, 1);
        return result;
    }
    LuaFunction toFunction() const {
        if (!isFunction()) return LuaFunction{ _L, LUA_NOREF };
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        int funcRef = luaL_ref(_L, LUA_REGISTRYINDEX);
        return LuaFunction{ _L, funcRef };
    }

    bool isThread() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_isthread(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

    bool isTable() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_istable(_L, -1);
        lua_pop(_L, 1);
        return result;
    }
    void setTableField(const char* key, const QLuaValue& value) const {
        if (!isTable()) return;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        value.pushToStack();
        lua_setfield(_L, -2, key);
        lua_pop(_L, 1); // pop the original table
    }
    void deleteTableField(const char* key) const {
        if (!isTable()) return;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        lua_pushnil(_L);
        lua_setfield(_L, -2, key);
        lua_pop(_L, 1); // pop the original table
    }

    void pushToStack() const {
        if (_ref == LUA_NOREF) {
            lua_pushnil(_L);
        } else {
            lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        }
    }

    bool isLightUserData() const {
        if (_ref == LUA_NOREF) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _ref);
        bool result = lua_islightuserdata(_L, -1);
        lua_pop(_L, 1);
        return result;
    }

private:
    lua_State* _L;
    int _ref;
};

static QVariant lua_to_qvariant(lua_State* L, int idx) {
    if (lua_isnumber(L, idx)) {
        return QVariant((double)lua_tonumber(L, idx));
    }
    if (lua_isinteger(L, idx)) {
        return QVariant((int)lua_tointeger(L, idx));
    }
    if (lua_isstring(L, idx)) {
        return QVariant(QString::fromUtf8(lua_tostring(L, idx)));
    }
    if (lua_isboolean(L, idx)) {
        return QVariant((bool)lua_toboolean(L, idx));
    }
    if (lua_isfunction(L, idx)) {
        lua_pushvalue(L, idx);
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);
        return QVariant::fromValue(LuaFunction{ L, ref });
    }
    return QVariant();
}

static void lua_push_qvariant(lua_State* L, const QVariant& value) {
    if (value.typeName() == QMetaType::typeName(QMetaType::Int)) {
        lua_pushinteger(L, value.toInt());
    }
    else if (value.typeName() == QMetaType::typeName(QMetaType::Double)) {
        lua_pushnumber(L, value.toDouble());
    }
    else if (value.typeName() == QMetaType::typeName(QMetaType::QString)) {
        lua_pushstring(L, value.toString().toUtf8().constData());
    }
    else if (value.typeName() == QMetaType::typeName(QMetaType::Bool)) {
        lua_pushboolean(L, value.toBool());
    }
    else if (value.typeName() == "LuaFunction") {
        LuaFunction func = value.value<LuaFunction>();
        lua_rawgeti(L, LUA_REGISTRYINDEX, func.ref);
    }
    else {
        lua_pushnil(L);
    }
}
