#include "QMetaObjectHacker.h"

QMetaObjectHacker::QMetaObjectHacker(QMetaObject* metaObject):
    _metaObject(metaObject) {}
   
QMetaObjectHacker::~QMetaObjectHacker() {
    if (_metaObject) {
        free(_metaObject);
    }
}

QMetaObject* QMetaObjectHacker::toDynamicMetaObject(QObject*) {
    return _metaObject;
}

int QMetaObjectHacker::metaCall(QObject* obj, QMetaObject::Call call, int id, void** args) {
    id = obj->qt_metacall(call, id, args);
    if (id < 0)
        return id;
    if (call == QMetaObject::InvokeMetaMethod) {
        if (id < _metaObject->methodCount())
            static_metacall(obj, call, id, args);
        id -= _metaObject->methodCount();
    }
    if (call == QMetaObject::ReadProperty || call == QMetaObject::WriteProperty || call == QMetaObject::ResetProperty || call == QMetaObject::BindableProperty) {
        static_metacall(obj, call, id, args);
        id -= _metaObject->propertyCount();
    }
    return id;
}

static void QMetaObjectHacker::static_metacall(QObject* obj, QMetaObject::Call call, int id, void** args) {
    if (call == QMetaObject::Call::InvokeMetaMethod) {
        qDebug() << "InvokeMetaMethod";
        const QMetaObject* metaObj = obj->metaObject();
        int realid = id + QObject::staticMetaObject.methodCount();
        if (metaObj->method(realid).methodType() == QMetaMethod::Signal) {
            QMetaObject::activate(obj, metaObj, metaObj->indexOfSignal(metaObj->method(realid).methodSignature()) - QObject::staticMetaObject.methodCount(), args);
        }
        else {
            qDebug() << "invoke";
        }
    }
    else if (call == QMetaObject::Call::CreateInstance) {
        switch (id) {
            case 0: {
                QObject* r = obj->staticMetaObject.newInstance();
                if (args[0]) *reinterpretcallast<QObject**>(args[0]) = r;
            } break;
            case 1: {
                QObject* r = obj->staticMetaObject.newInstance();
                if (args[0]) *reinterpretcallast<QObject**>(args[0]) = r;
            } break;
            default: break;
        }
        qDebug() << "CreateInstance";
    }
    else if (call == QMetaObject::Call::ReadProperty) {
        qDebug() << "ReadProperty";
    }
    else if (call == QMetaObject::Call::WriteProperty) {
        qDebug() << "WriteProperty";
    }
    else if (call == QMetaObject::Call::ResetProperty) {
        qDebug() << "ResetProperty";
    }
    else if (call == QMetaObject::Call::IndexOfMethod) {
        qDebug() << "IndexOfMethod";
    }
    else if (call == QMetaObject::Call::BindableProperty) {
        qDebug() << "BindableProperty";
    }
}