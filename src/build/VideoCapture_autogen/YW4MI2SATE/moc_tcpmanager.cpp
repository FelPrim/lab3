/****************************************************************************
** Meta object code from reading C++ file 'tcpmanager.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../new/tcpmanager.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tcpmanager.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10TCPManagerE_t {};
} // unnamed namespace

template <> constexpr inline auto TCPManager::qt_create_metaobjectdata<qt_meta_tag_ZN10TCPManagerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "TCPManager",
        "connected",
        "",
        "disconnected",
        "errorOccurred",
        "error",
        "serverStreamCreated",
        "uint32_t",
        "streamId",
        "serverStreamDeleted",
        "serverStreamJoined",
        "serverStreamStart",
        "serverStreamEnd",
        "onConnected",
        "onDisconnected",
        "onReadyRead",
        "onErrorOccured",
        "QAbstractSocket::SocketError",
        "socketError"
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
        // Slot 'onConnected'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDisconnected'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onReadyRead'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onErrorOccured'
        QtMocHelpers::SlotData<void(QAbstractSocket::SocketError)>(16, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 17, 18 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<TCPManager, qt_meta_tag_ZN10TCPManagerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject TCPManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10TCPManagerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10TCPManagerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10TCPManagerE_t>.metaTypes,
    nullptr
} };

void TCPManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<TCPManager *>(_o);
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
        case 8: _t->onConnected(); break;
        case 9: _t->onDisconnected(); break;
        case 10: _t->onReadyRead(); break;
        case 11: _t->onErrorOccured((*reinterpret_cast<std::add_pointer_t<QAbstractSocket::SocketError>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 11:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAbstractSocket::SocketError >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (TCPManager::*)()>(_a, &TCPManager::connected, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (TCPManager::*)()>(_a, &TCPManager::disconnected, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (TCPManager::*)(const QString & )>(_a, &TCPManager::errorOccurred, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (TCPManager::*)(uint32_t )>(_a, &TCPManager::serverStreamCreated, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (TCPManager::*)(uint32_t )>(_a, &TCPManager::serverStreamDeleted, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (TCPManager::*)(uint32_t )>(_a, &TCPManager::serverStreamJoined, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (TCPManager::*)(uint32_t )>(_a, &TCPManager::serverStreamStart, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (TCPManager::*)(uint32_t )>(_a, &TCPManager::serverStreamEnd, 7))
            return;
    }
}

const QMetaObject *TCPManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TCPManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10TCPManagerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int TCPManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void TCPManager::connected()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void TCPManager::disconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void TCPManager::errorOccurred(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void TCPManager::serverStreamCreated(uint32_t _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void TCPManager::serverStreamDeleted(uint32_t _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void TCPManager::serverStreamJoined(uint32_t _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void TCPManager::serverStreamStart(uint32_t _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void TCPManager::serverStreamEnd(uint32_t _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1);
}
QT_WARNING_POP
