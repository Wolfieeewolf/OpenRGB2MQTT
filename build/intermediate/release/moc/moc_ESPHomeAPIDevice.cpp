/****************************************************************************
** Meta object code from reading C++ file 'ESPHomeAPIDevice.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/devices/esphome/ESPHomeAPIDevice.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ESPHomeAPIDevice.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ESPHomeAPIDevice_t {
    QByteArrayData data[10];
    char stringdata0[169];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ESPHomeAPIDevice_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ESPHomeAPIDevice_t qt_meta_stringdata_ESPHomeAPIDevice = {
    {
QT_MOC_LITERAL(0, 0, 16), // "ESPHomeAPIDevice"
QT_MOC_LITERAL(1, 17, 23), // "connectionStatusChanged"
QT_MOC_LITERAL(2, 41, 0), // ""
QT_MOC_LITERAL(3, 42, 9), // "connected"
QT_MOC_LITERAL(4, 52, 21), // "handleSocketConnected"
QT_MOC_LITERAL(5, 74, 24), // "handleSocketDisconnected"
QT_MOC_LITERAL(6, 99, 17), // "handleSocketError"
QT_MOC_LITERAL(7, 117, 28), // "QAbstractSocket::SocketError"
QT_MOC_LITERAL(8, 146, 5), // "error"
QT_MOC_LITERAL(9, 152, 16) // "handleSocketData"

    },
    "ESPHomeAPIDevice\0connectionStatusChanged\0"
    "\0connected\0handleSocketConnected\0"
    "handleSocketDisconnected\0handleSocketError\0"
    "QAbstractSocket::SocketError\0error\0"
    "handleSocketData"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ESPHomeAPIDevice[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   39,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    0,   42,    2, 0x08 /* Private */,
       5,    0,   43,    2, 0x08 /* Private */,
       6,    1,   44,    2, 0x08 /* Private */,
       9,    0,   47,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void,

       0        // eod
};

void ESPHomeAPIDevice::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ESPHomeAPIDevice *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->connectionStatusChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->handleSocketConnected(); break;
        case 2: _t->handleSocketDisconnected(); break;
        case 3: _t->handleSocketError((*reinterpret_cast< QAbstractSocket::SocketError(*)>(_a[1]))); break;
        case 4: _t->handleSocketData(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QAbstractSocket::SocketError >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ESPHomeAPIDevice::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ESPHomeAPIDevice::connectionStatusChanged)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ESPHomeAPIDevice::staticMetaObject = { {
    QMetaObject::SuperData::link<MQTTRGBDevice::staticMetaObject>(),
    qt_meta_stringdata_ESPHomeAPIDevice.data,
    qt_meta_data_ESPHomeAPIDevice,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ESPHomeAPIDevice::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ESPHomeAPIDevice::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ESPHomeAPIDevice.stringdata0))
        return static_cast<void*>(this);
    return MQTTRGBDevice::qt_metacast(_clname);
}

int ESPHomeAPIDevice::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = MQTTRGBDevice::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void ESPHomeAPIDevice::connectionStatusChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
