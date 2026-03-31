#pragma once

#include <QtCore/QObject>
#include <QtCore/QMetaObject>
#include <lua.hpp>

class QLuaItem: public QObject {
public:
    QLuaItem(QObject* parent = nullptr): QObject(parent) {}

    const QMetaObject* metaObject() const override {
        if (_metaObject) {
            return _metaObject;
        }
        return QObject::metaObject();
    }

    void setMetaObject(const QMetaObject* mo) {
        _metaObject = mo;
    }

private:
    const QMetaObject* _metaObject;
};
