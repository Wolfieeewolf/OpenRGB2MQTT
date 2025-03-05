/****************************************************************************
** Meta object code from reading C++ file 'ESPHomeAPIManager.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/devices/esphome/ESPHomeAPIManager.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ESPHomeAPIManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ESPHomeAPIManager_t {
    QByteArrayData data[10];
    char stringdata0[140];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ESPHomeAPIManager_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ESPHomeAPIManager_t qt_meta_stringdata_ESPHomeAPIManager = {
    {
QT_MOC_LITERAL(0, 0, 17), // "ESPHomeAPIManager"
QT_MOC_LITERAL(1, 18, 17), // "deviceListChanged"
QT_MOC_LITERAL(2, 36, 0), // ""
QT_MOC_LITERAL(3, 37, 23), // "handleServiceDiscovered"
QT_MOC_LITERAL(4, 61, 9), // "QHostInfo"
QT_MOC_LITERAL(5, 71, 4), // "info"
QT_MOC_LITERAL(6, 76, 17), // "handleLookupError"
QT_MOC_LITERAL(7, 94, 17), // "QDnsLookup::Error"
QT_MOC_LITERAL(8, 112, 5), // "error"
QT_MOC_LITERAL(9, 118, 21) // "handleDnsRecordLookup"

    },
    "ESPHomeAPIManager\0deviceListChanged\0"
    "\0handleServiceDiscovered\0QHostInfo\0"
    "info\0handleLookupError\0QDnsLookup::Error\0"
    "error\0handleDnsRecordLookup"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ESPHomeAPIManager[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   34,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       3,    1,   35,    2, 0x08 /* Private */,
       6,    1,   38,    2, 0x08 /* Private */,
       9,    0,   41,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 4,    5,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void,

       0        // eod
};

void ESPHomeAPIManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ESPHomeAPIManager *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->deviceListChanged(); break;
        case 1: _t->handleServiceDiscovered((*reinterpret_cast< const QHostInfo(*)>(_a[1]))); break;
        case 2: _t->handleLookupError((*reinterpret_cast< QDnsLookup::Error(*)>(_a[1]))); break;
        case 3: _t->handleDnsRecordLookup(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QHostInfo >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ESPHomeAPIManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ESPHomeAPIManager::deviceListChanged)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ESPHomeAPIManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ESPHomeAPIManager.data,
    qt_meta_data_ESPHomeAPIManager,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ESPHomeAPIManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ESPHomeAPIManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ESPHomeAPIManager.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ESPHomeAPIManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void ESPHomeAPIManager::deviceListChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
