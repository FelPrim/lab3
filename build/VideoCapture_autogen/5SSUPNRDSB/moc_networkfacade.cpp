/****************************************************************************
** Meta object code from reading C++ file 'networkfacade.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/network/networkfacade.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'networkfacade.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_NetworkFacade_t {
    uint offsetsAndSizes[54];
    char stringdata0[14];
    char stringdata1[10];
    char stringdata2[1];
    char stringdata3[13];
    char stringdata4[14];
    char stringdata5[4];
    char stringdata6[20];
    char stringdata7[9];
    char stringdata8[3];
    char stringdata9[20];
    char stringdata10[19];
    char stringdata11[18];
    char stringdata12[16];
    char stringdata13[15];
    char stringdata14[9];
    char stringdata15[12];
    char stringdata16[10];
    char stringdata17[21];
    char stringdata18[8];
    char stringdata19[15];
    char stringdata20[18];
    char stringdata21[11];
    char stringdata22[22];
    char stringdata23[22];
    char stringdata24[21];
    char stringdata25[20];
    char stringdata26[18];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_NetworkFacade_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_NetworkFacade_t qt_meta_stringdata_NetworkFacade = {
    {
        QT_MOC_LITERAL(0, 13),  // "NetworkFacade"
        QT_MOC_LITERAL(14, 9),  // "connected"
        QT_MOC_LITERAL(24, 0),  // ""
        QT_MOC_LITERAL(25, 12),  // "disconnected"
        QT_MOC_LITERAL(38, 13),  // "errorOccurred"
        QT_MOC_LITERAL(52, 3),  // "err"
        QT_MOC_LITERAL(56, 19),  // "serverStreamCreated"
        QT_MOC_LITERAL(76, 8),  // "uint32_t"
        QT_MOC_LITERAL(85, 2),  // "id"
        QT_MOC_LITERAL(88, 19),  // "serverStreamDeleted"
        QT_MOC_LITERAL(108, 18),  // "serverStreamJoined"
        QT_MOC_LITERAL(127, 17),  // "serverStreamStart"
        QT_MOC_LITERAL(145, 15),  // "serverStreamEnd"
        QT_MOC_LITERAL(161, 14),  // "frameAssembled"
        QT_MOC_LITERAL(176, 8),  // "streamId"
        QT_MOC_LITERAL(185, 11),  // "frameNumber"
        QT_MOC_LITERAL(197, 9),  // "frameData"
        QT_MOC_LITERAL(207, 20),  // "networkErrorOccurred"
        QT_MOC_LITERAL(228, 7),  // "message"
        QT_MOC_LITERAL(236, 14),  // "onTcpConnected"
        QT_MOC_LITERAL(251, 17),  // "onTcpDisconnected"
        QT_MOC_LITERAL(269, 10),  // "onTcpError"
        QT_MOC_LITERAL(280, 21),  // "onServerStreamCreated"
        QT_MOC_LITERAL(302, 21),  // "onServerStreamDeleted"
        QT_MOC_LITERAL(324, 20),  // "onServerStreamJoined"
        QT_MOC_LITERAL(345, 19),  // "onServerStreamStart"
        QT_MOC_LITERAL(365, 17)   // "onServerStreamEnd"
    },
    "NetworkFacade",
    "connected",
    "",
    "disconnected",
    "errorOccurred",
    "err",
    "serverStreamCreated",
    "uint32_t",
    "id",
    "serverStreamDeleted",
    "serverStreamJoined",
    "serverStreamStart",
    "serverStreamEnd",
    "frameAssembled",
    "streamId",
    "frameNumber",
    "frameData",
    "networkErrorOccurred",
    "message",
    "onTcpConnected",
    "onTcpDisconnected",
    "onTcpError",
    "onServerStreamCreated",
    "onServerStreamDeleted",
    "onServerStreamJoined",
    "onServerStreamStart",
    "onServerStreamEnd"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_NetworkFacade[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      18,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      10,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  122,    2, 0x06,    1 /* Public */,
       3,    0,  123,    2, 0x06,    2 /* Public */,
       4,    1,  124,    2, 0x06,    3 /* Public */,
       6,    1,  127,    2, 0x06,    5 /* Public */,
       9,    1,  130,    2, 0x06,    7 /* Public */,
      10,    1,  133,    2, 0x06,    9 /* Public */,
      11,    1,  136,    2, 0x06,   11 /* Public */,
      12,    1,  139,    2, 0x06,   13 /* Public */,
      13,    3,  142,    2, 0x06,   15 /* Public */,
      17,    1,  149,    2, 0x06,   19 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      19,    0,  152,    2, 0x08,   21 /* Private */,
      20,    0,  153,    2, 0x08,   22 /* Private */,
      21,    1,  154,    2, 0x08,   23 /* Private */,
      22,    1,  157,    2, 0x08,   25 /* Private */,
      23,    1,  160,    2, 0x08,   27 /* Private */,
      24,    1,  163,    2, 0x08,   29 /* Private */,
      25,    1,  166,    2, 0x08,   31 /* Private */,
      26,    1,  169,    2, 0x08,   33 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::QByteArray,   14,   15,   16,
    QMetaType::Void, QMetaType::QString,   18,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, 0x80000000 | 7,    8,

       0        // eod
};

Q_CONSTINIT const QMetaObject NetworkFacade::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_NetworkFacade.offsetsAndSizes,
    qt_meta_data_NetworkFacade,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_NetworkFacade_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<NetworkFacade, std::true_type>,
        // method 'connected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'disconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'errorOccurred'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'serverStreamCreated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        // method 'serverStreamDeleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        // method 'serverStreamJoined'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        // method 'serverStreamStart'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        // method 'serverStreamEnd'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        // method 'frameAssembled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QByteArray &, std::false_type>,
        // method 'networkErrorOccurred'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onTcpConnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onTcpDisconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onTcpError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onServerStreamCreated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        // method 'onServerStreamDeleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        // method 'onServerStreamJoined'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        // method 'onServerStreamStart'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        // method 'onServerStreamEnd'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>
    >,
    nullptr
} };

void NetworkFacade::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<NetworkFacade *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->connected(); break;
        case 1: _t->disconnected(); break;
        case 2: _t->errorOccurred((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->serverStreamCreated((*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 4: _t->serverStreamDeleted((*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 5: _t->serverStreamJoined((*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 6: _t->serverStreamStart((*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 7: _t->serverStreamEnd((*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 8: _t->frameAssembled((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QByteArray>>(_a[3]))); break;
        case 9: _t->networkErrorOccurred((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 10: _t->onTcpConnected(); break;
        case 11: _t->onTcpDisconnected(); break;
        case 12: _t->onTcpError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 13: _t->onServerStreamCreated((*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 14: _t->onServerStreamDeleted((*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 15: _t->onServerStreamJoined((*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 16: _t->onServerStreamStart((*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 17: _t->onServerStreamEnd((*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (NetworkFacade::*)();
            if (_t _q_method = &NetworkFacade::connected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (NetworkFacade::*)();
            if (_t _q_method = &NetworkFacade::disconnected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (NetworkFacade::*)(const QString & );
            if (_t _q_method = &NetworkFacade::errorOccurred; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (NetworkFacade::*)(uint32_t );
            if (_t _q_method = &NetworkFacade::serverStreamCreated; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (NetworkFacade::*)(uint32_t );
            if (_t _q_method = &NetworkFacade::serverStreamDeleted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (NetworkFacade::*)(uint32_t );
            if (_t _q_method = &NetworkFacade::serverStreamJoined; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (NetworkFacade::*)(uint32_t );
            if (_t _q_method = &NetworkFacade::serverStreamStart; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (NetworkFacade::*)(uint32_t );
            if (_t _q_method = &NetworkFacade::serverStreamEnd; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (NetworkFacade::*)(int , int , const QByteArray & );
            if (_t _q_method = &NetworkFacade::frameAssembled; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (NetworkFacade::*)(const QString & );
            if (_t _q_method = &NetworkFacade::networkErrorOccurred; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
    }
}

const QMetaObject *NetworkFacade::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NetworkFacade::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_NetworkFacade.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int NetworkFacade::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 18)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 18;
    }
    return _id;
}

// SIGNAL 0
void NetworkFacade::connected()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void NetworkFacade::disconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void NetworkFacade::errorOccurred(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void NetworkFacade::serverStreamCreated(uint32_t _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void NetworkFacade::serverStreamDeleted(uint32_t _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void NetworkFacade::serverStreamJoined(uint32_t _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void NetworkFacade::serverStreamStart(uint32_t _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void NetworkFacade::serverStreamEnd(uint32_t _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void NetworkFacade::frameAssembled(int _t1, int _t2, const QByteArray & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void NetworkFacade::networkErrorOccurred(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
