/****************************************************************************
** Meta object code from reading C++ file 'networkfacade.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../new/networkfacade.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'networkfacade.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN13NetworkFacadeE_t {};
} // unnamed namespace

template <> constexpr inline auto NetworkFacade::qt_create_metaobjectdata<qt_meta_tag_ZN13NetworkFacadeE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
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
        "sendUdpDatagram",
        "data",
        "QHostAddress",
        "host",
        "port",
        "onTcpConnected",
        "onTcpDisconnected",
        "onTcpError",
        "onServerStreamCreated",
        "onServerStreamDeleted",
        "onServerStreamJoined",
        "onServerStreamStart",
        "onServerStreamEnd"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'connected'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'disconnected'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'errorOccurred'
        QtMocHelpers::SignalData<void(const QString &)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 5 },
        }}),
        // Signal 'serverStreamCreated'
        QtMocHelpers::SignalData<void(uint32_t)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 8 },
        }}),
        // Signal 'serverStreamDeleted'
        QtMocHelpers::SignalData<void(uint32_t)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 8 },
        }}),
        // Signal 'serverStreamJoined'
        QtMocHelpers::SignalData<void(uint32_t)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 8 },
        }}),
        // Signal 'serverStreamStart'
        QtMocHelpers::SignalData<void(uint32_t)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 8 },
        }}),
        // Signal 'serverStreamEnd'
        QtMocHelpers::SignalData<void(uint32_t)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 8 },
        }}),
        // Signal 'sendUdpDatagram'
        QtMocHelpers::SignalData<void(const QByteArray &, const QHostAddress &, quint16)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QByteArray, 14 }, { 0x80000000 | 15, 16 }, { QMetaType::UShort, 17 },
        }}),
        // Slot 'onTcpConnected'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onTcpDisconnected'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onTcpError'
        QtMocHelpers::SlotData<void(const QString &)>(20, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 5 },
        }}),
        // Slot 'onServerStreamCreated'
        QtMocHelpers::SlotData<void(uint32_t)>(21, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 7, 8 },
        }}),
        // Slot 'onServerStreamDeleted'
        QtMocHelpers::SlotData<void(uint32_t)>(22, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 7, 8 },
        }}),
        // Slot 'onServerStreamJoined'
        QtMocHelpers::SlotData<void(uint32_t)>(23, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 7, 8 },
        }}),
        // Slot 'onServerStreamStart'
        QtMocHelpers::SlotData<void(uint32_t)>(24, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 7, 8 },
        }}),
        // Slot 'onServerStreamEnd'
        QtMocHelpers::SlotData<void(uint32_t)>(25, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 7, 8 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<NetworkFacade, qt_meta_tag_ZN13NetworkFacadeE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject NetworkFacade::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13NetworkFacadeE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13NetworkFacadeE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN13NetworkFacadeE_t>.metaTypes,
    nullptr
} };

void NetworkFacade::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<NetworkFacade *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->connected(); break;
        case 1: _t->disconnected(); break;
        case 2: _t->errorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->serverStreamCreated((*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 4: _t->serverStreamDeleted((*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 5: _t->serverStreamJoined((*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 6: _t->serverStreamStart((*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 7: _t->serverStreamEnd((*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 8: _t->sendUdpDatagram((*reinterpret_cast<std::add_pointer_t<QByteArray>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QHostAddress>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<quint16>>(_a[3]))); break;
        case 9: _t->onTcpConnected(); break;
        case 10: _t->onTcpDisconnected(); break;
        case 11: _t->onTcpError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 12: _t->onServerStreamCreated((*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 13: _t->onServerStreamDeleted((*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 14: _t->onServerStreamJoined((*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 15: _t->onServerStreamStart((*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 16: _t->onServerStreamEnd((*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (NetworkFacade::*)()>(_a, &NetworkFacade::connected, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkFacade::*)()>(_a, &NetworkFacade::disconnected, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkFacade::*)(const QString & )>(_a, &NetworkFacade::errorOccurred, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkFacade::*)(uint32_t )>(_a, &NetworkFacade::serverStreamCreated, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkFacade::*)(uint32_t )>(_a, &NetworkFacade::serverStreamDeleted, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkFacade::*)(uint32_t )>(_a, &NetworkFacade::serverStreamJoined, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkFacade::*)(uint32_t )>(_a, &NetworkFacade::serverStreamStart, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkFacade::*)(uint32_t )>(_a, &NetworkFacade::serverStreamEnd, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkFacade::*)(const QByteArray & , const QHostAddress & , quint16 )>(_a, &NetworkFacade::sendUdpDatagram, 8))
            return;
    }
}

const QMetaObject *NetworkFacade::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NetworkFacade::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13NetworkFacadeE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int NetworkFacade::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 17)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 17;
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
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void NetworkFacade::serverStreamCreated(uint32_t _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void NetworkFacade::serverStreamDeleted(uint32_t _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void NetworkFacade::serverStreamJoined(uint32_t _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void NetworkFacade::serverStreamStart(uint32_t _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void NetworkFacade::serverStreamEnd(uint32_t _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}

// SIGNAL 8
void NetworkFacade::sendUdpDatagram(const QByteArray & _t1, const QHostAddress & _t2, quint16 _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1, _t2, _t3);
}
QT_WARNING_POP
