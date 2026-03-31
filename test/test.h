#pragma once

#include <QObject>
#include <lua.hpp>
#include <QtCore/QMetaMethod>

/**
 *  predefinitions for Lua bindings
 */

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

