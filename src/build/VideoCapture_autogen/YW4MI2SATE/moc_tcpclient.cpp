/****************************************************************************
** Meta object code from reading C++ file 'tcpclient.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../new/tcpclient.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tcpclient.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN9TcpClientE_t {};
} // unnamed namespace

template <> constexpr inline auto TcpClient::qt_create_metaobjectdata<qt_meta_tag_ZN9TcpClientE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "TcpClient",
        "connected",
        "",
        "disconnected",
        "connectionError",
        "errorString",
        "streamCreated",
        "streamId",
        "streamDeleted",
        "streamJoined",
        "streamStart",
        "streamEnd",
        "protocolError",
        "error",
        "networkError",
        "onConnected",
        "onDisconnected",
        "onReadyRead",
        "onErrorOccurred",
        "QAbstractSocket::SocketError",
        "onStateChanged",
        "QAbstractSocket::SocketState",
        "state",
        "onReconnectTimer",
        "onKeepAliveTimer"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'connected'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'disconnected'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'connectionError'
        QtMocHelpers::SignalData<void(const QString &)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 5 },
        }}),
        // Signal 'streamCreated'
        QtMocHelpers::SignalData<void(quint32)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 7 },
        }}),
        // Signal 'streamDeleted'
        QtMocHelpers::SignalData<void(quint32)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 7 },
        }}),
        // Signal 'streamJoined'
        QtMocHelpers::SignalData<void(quint32)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 7 },
        }}),
        // Signal 'streamStart'
        QtMocHelpers::SignalData<void(quint32)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 7 },
        }}),
        // Signal 'streamEnd'
        QtMocHelpers::SignalData<void(quint32)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 7 },
        }}),
        // Signal 'protocolError'
        QtMocHelpers::SignalData<void(const QString &)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 13 },
        }}),
        // Signal 'networkError'
        QtMocHelpers::SignalData<void(const QString &)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 13 },
        }}),
        // Slot 'onConnected'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDisconnected'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onReadyRead'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onErrorOccurred'
        QtMocHelpers::SlotData<void(QAbstractSocket::SocketError)>(18, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 19, 13 },
        }}),
        // Slot 'onStateChanged'
        QtMocHelpers::SlotData<void(QAbstractSocket::SocketState)>(20, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 21, 22 },
        }}),
        // Slot 'onReconnectTimer'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onKeepAliveTimer'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<TcpClient, qt_meta_tag_ZN9TcpClientE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject TcpClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9TcpClientE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9TcpClientE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9TcpClientE_t>.metaTypes,
    nullptr
} };

void TcpClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<TcpClient *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->connected(); break;
        case 1: _t->disconnected(); break;
        case 2: _t->connectionError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->streamCreated((*reinterpret_cast<std::add_pointer_t<quint32>>(_a[1]))); break;
        case 4: _t->streamDeleted((*reinterpret_cast<std::add_pointer_t<quint32>>(_a[1]))); break;
        case 5: _t->streamJoined((*reinterpret_cast<std::add_pointer_t<quint32>>(_a[1]))); break;
        case 6: _t->streamStart((*reinterpret_cast<std::add_pointer_t<quint32>>(_a[1]))); break;
        case 7: _t->streamEnd((*reinterpret_cast<std::add_pointer_t<quint32>>(_a[1]))); break;
        case 8: _t->protocolError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->networkError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 10: _t->onConnected(); break;
        case 11: _t->onDisconnected(); break;
        case 12: _t->onReadyRead(); break;
        case 13: _t->onErrorOccurred((*reinterpret_cast<std::add_pointer_t<QAbstractSocket::SocketError>>(_a[1]))); break;
        case 14: _t->onStateChanged((*reinterpret_cast<std::add_pointer_t<QAbstractSocket::SocketState>>(_a[1]))); break;
        case 15: _t->onReconnectTimer(); break;
        case 16: _t->onKeepAliveTimer(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 13:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAbstractSocket::SocketError >(); break;
            }
            break;
        case 14:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAbstractSocket::SocketState >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (TcpClient::*)()>(_a, &TcpClient::connected, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (TcpClient::*)()>(_a, &TcpClient::disconnected, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (TcpClient::*)(const QString & )>(_a, &TcpClient::connectionError, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (TcpClient::*)(quint32 )>(_a, &TcpClient::streamCreated, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (TcpClient::*)(quint32 )>(_a, &TcpClient::streamDeleted, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (TcpClient::*)(quint32 )>(_a, &TcpClient::streamJoined, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (TcpClient::*)(quint32 )>(_a, &TcpClient::streamStart, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (TcpClient::*)(quint32 )>(_a, &TcpClient::streamEnd, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (TcpClient::*)(const QString & )>(_a, &TcpClient::protocolError, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (TcpClient::*)(const QString & )>(_a, &TcpClient::networkError, 9))
            return;
    }
}

const QMetaObject *TcpClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TcpClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9TcpClientE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int TcpClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    }
    return _id;
}

// SIGNAL 0
void TcpClient::connected()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void TcpClient::disconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void TcpClient::connectionError(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void TcpClient::streamCreated(quint32 _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void TcpClient::streamDeleted(quint32 _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void TcpClient::streamJoined(quint32 _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void TcpClient::streamStart(quint32 _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void TcpClient::streamEnd(quint32 _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}

// SIGNAL 8
void TcpClient::protocolError(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1);
}

// SIGNAL 9
void TcpClient::networkError(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1);
}
QT_WARNING_POP
