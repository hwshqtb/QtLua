#pragma once

static void qt_static_metacall(QObject* obj, QMetaObject::Call call, int id, void** args) {
    
}

class QMetaObjectHacker: public QDynamicMetaObjectData {
public:
    QMetaObjectHacker(QMetaObject* metaObj);

    ~QMetaObjectHacker();

    QMetaObject* toDynamicMetaObject(QObject*);

    int metaCall(QObject* obj, QMetaObject::Call call, int id, void** args);
    
    static void static_metacall(QObject* obj, QMetaObject::Call call, int id, void** args);

    QMetaObject* _metaObject = nullptr;
};