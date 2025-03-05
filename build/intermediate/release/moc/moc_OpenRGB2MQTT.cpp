/****************************************************************************
** Meta object code from reading C++ file 'OpenRGB2MQTT.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/plugin/OpenRGB2MQTT.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qplugin.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'OpenRGB2MQTT.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_OpenRGB2MQTT_t {
    QByteArrayData data[17];
    char stringdata0[342];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_OpenRGB2MQTT_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_OpenRGB2MQTT_t qt_meta_stringdata_OpenRGB2MQTT = {
    {
QT_MOC_LITERAL(0, 0, 12), // "OpenRGB2MQTT"
QT_MOC_LITERAL(1, 13, 22), // "onConnectButtonClicked"
QT_MOC_LITERAL(2, 36, 0), // ""
QT_MOC_LITERAL(3, 37, 20), // "onAutoConnectChanged"
QT_MOC_LITERAL(4, 58, 5), // "state"
QT_MOC_LITERAL(5, 64, 19), // "onDDPEnabledChanged"
QT_MOC_LITERAL(6, 84, 26), // "onDDPDiscoverButtonClicked"
QT_MOC_LITERAL(7, 111, 27), // "onDDPAddDeviceButtonClicked"
QT_MOC_LITERAL(8, 139, 30), // "onDDPRemoveDeviceButtonClicked"
QT_MOC_LITERAL(9, 170, 27), // "onDDPDeviceSelectionChanged"
QT_MOC_LITERAL(10, 198, 22), // "onDDPDiscoveryFinished"
QT_MOC_LITERAL(11, 221, 5), // "count"
QT_MOC_LITERAL(12, 227, 23), // "onESPHomeEnabledChanged"
QT_MOC_LITERAL(13, 251, 22), // "onZigbeeRefreshClicked"
QT_MOC_LITERAL(14, 274, 27), // "onDeviceActionButtonClicked"
QT_MOC_LITERAL(15, 302, 17), // "refreshDeviceList"
QT_MOC_LITERAL(16, 320, 21) // "updateAllDevicesTable"

    },
    "OpenRGB2MQTT\0onConnectButtonClicked\0"
    "\0onAutoConnectChanged\0state\0"
    "onDDPEnabledChanged\0onDDPDiscoverButtonClicked\0"
    "onDDPAddDeviceButtonClicked\0"
    "onDDPRemoveDeviceButtonClicked\0"
    "onDDPDeviceSelectionChanged\0"
    "onDDPDiscoveryFinished\0count\0"
    "onESPHomeEnabledChanged\0onZigbeeRefreshClicked\0"
    "onDeviceActionButtonClicked\0"
    "refreshDeviceList\0updateAllDevicesTable"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_OpenRGB2MQTT[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   79,    2, 0x08 /* Private */,
       3,    1,   80,    2, 0x08 /* Private */,
       5,    1,   83,    2, 0x08 /* Private */,
       6,    0,   86,    2, 0x08 /* Private */,
       7,    0,   87,    2, 0x08 /* Private */,
       8,    0,   88,    2, 0x08 /* Private */,
       9,    0,   89,    2, 0x08 /* Private */,
      10,    1,   90,    2, 0x08 /* Private */,
      12,    1,   93,    2, 0x08 /* Private */,
      13,    0,   96,    2, 0x08 /* Private */,
      14,    0,   97,    2, 0x08 /* Private */,
      15,    0,   98,    2, 0x08 /* Private */,
      16,    0,   99,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   11,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void OpenRGB2MQTT::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<OpenRGB2MQTT *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->onConnectButtonClicked(); break;
        case 1: _t->onAutoConnectChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->onDDPEnabledChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->onDDPDiscoverButtonClicked(); break;
        case 4: _t->onDDPAddDeviceButtonClicked(); break;
        case 5: _t->onDDPRemoveDeviceButtonClicked(); break;
        case 6: _t->onDDPDeviceSelectionChanged(); break;
        case 7: _t->onDDPDiscoveryFinished((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 8: _t->onESPHomeEnabledChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 9: _t->onZigbeeRefreshClicked(); break;
        case 10: _t->onDeviceActionButtonClicked(); break;
        case 11: _t->refreshDeviceList(); break;
        case 12: _t->updateAllDevicesTable(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject OpenRGB2MQTT::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_OpenRGB2MQTT.data,
    qt_meta_data_OpenRGB2MQTT,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *OpenRGB2MQTT::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *OpenRGB2MQTT::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_OpenRGB2MQTT.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "OpenRGBPluginInterface"))
        return static_cast< OpenRGBPluginInterface*>(this);
    if (!strcmp(_clname, "com.OpenRGBPluginInterface"))
        return static_cast< OpenRGBPluginInterface*>(this);
    return QObject::qt_metacast(_clname);
}

int OpenRGB2MQTT::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 13;
    }
    return _id;
}

QT_PLUGIN_METADATA_SECTION
static constexpr unsigned char qt_pluginMetaData[] = {
    'Q', 'T', 'M', 'E', 'T', 'A', 'D', 'A', 'T', 'A', ' ', '!',
    // metadata version, Qt version, architectural requirements
    0, QT_VERSION_MAJOR, QT_VERSION_MINOR, qPluginArchRequirements(),
    0xbf, 
    // "IID"
    0x02,  0x78,  0x1a,  'c',  'o',  'm',  '.',  'O', 
    'p',  'e',  'n',  'R',  'G',  'B',  'P',  'l', 
    'u',  'g',  'i',  'n',  'I',  'n',  't',  'e', 
    'r',  'f',  'a',  'c',  'e', 
    // "className"
    0x03,  0x6c,  'O',  'p',  'e',  'n',  'R',  'G', 
    'B',  '2',  'M',  'Q',  'T',  'T', 
    0xff, 
};
QT_MOC_EXPORT_PLUGIN(OpenRGB2MQTT, OpenRGB2MQTT)

QT_WARNING_POP
QT_END_MOC_NAMESPACE
