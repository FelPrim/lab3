/****************************************************************************
** Meta object code from reading C++ file 'streammanager.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../new/streammanager.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'streammanager.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN13StreamManagerE_t {};
} // unnamed namespace

template <> constexpr inline auto StreamManager::qt_create_metaobjectdata<qt_meta_tag_ZN13StreamManagerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "StreamManager",
        "streamWindowCreated",
        "",
        "StreamWindow*",
        "window",
        "streamWindowClosed",
        "streamId",
        "connectionStatusChanged",
        "connected",
        "errorOccurred",
        "message",
        "onStreamStopped",
        "onStreamLeft",
        "onWindowClosed",
        "onServerStreamCreated",
        "uint32_t",
        "onServerStreamDeleted",
        "onServerStreamJoined",
        "onServerStreamStart",
        "onServerStreamEnd"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'streamWindowCreated'
        QtMocHelpers::SignalData<void(StreamWindow *)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'streamWindowClosed'
        QtMocHelpers::SignalData<void(int)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 6 },
        }}),
        // Signal 'connectionStatusChanged'
        QtMocHelpers::SignalData<void(bool)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 8 },
        }}),
        // Signal 'errorOccurred'
        QtMocHelpers::SignalData<void(const QString &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 10 },
        }}),
        // Slot 'onStreamStopped'
        QtMocHelpers::SlotData<void(int)>(11, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 6 },
        }}),
        // Slot 'onStreamLeft'
        QtMocHelpers::SlotData<void(int)>(12, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 6 },
        }}),
        // Slot 'onWindowClosed'
        QtMocHelpers::SlotData<void(int)>(13, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 6 },
        }}),
        // Slot 'onServerStreamCreated'
        QtMocHelpers::SlotData<void(uint32_t)>(14, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 15, 6 },
        }}),
        // Slot 'onServerStreamDeleted'
        QtMocHelpers::SlotData<void(uint32_t)>(16, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 15, 6 },
        }}),
        // Slot 'onServerStreamJoined'
        QtMocHelpers::SlotData<void(uint32_t)>(17, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 15, 6 },
        }}),
        // Slot 'onServerStreamStart'
        QtMocHelpers::SlotData<void(uint32_t)>(18, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 15, 6 },
        }}),
        // Slot 'onServerStreamEnd'
        QtMocHelpers::SlotData<void(uint32_t)>(19, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 15, 6 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<StreamManager, qt_meta_tag_ZN13StreamManagerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject StreamManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13StreamManagerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13StreamManagerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN13StreamManagerE_t>.metaTypes,
    nullptr
} };

void StreamManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<StreamManager *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->streamWindowCreated((*reinterpret_cast<std::add_pointer_t<StreamWindow*>>(_a[1]))); break;
        case 1: _t->streamWindowClosed((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->connectionStatusChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 3: _t->errorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->onStreamStopped((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->onStreamLeft((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->onWindowClosed((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->onServerStreamCreated((*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 8: _t->onServerStreamDeleted((*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 9: _t->onServerStreamJoined((*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 10: _t->onServerStreamStart((*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 11: _t->onServerStreamEnd((*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< StreamWindow* >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (StreamManager::*)(StreamWindow * )>(_a, &StreamManager::streamWindowCreated, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (StreamManager::*)(int )>(_a, &StreamManager::streamWindowClosed, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (StreamManager::*)(bool )>(_a, &StreamManager::connectionStatusChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (StreamManager::*)(const QString & )>(_a, &StreamManager::errorOccurred, 3))
            return;
    }
}

const QMetaObject *StreamManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *StreamManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13StreamManagerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int StreamManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void StreamManager::streamWindowCreated(StreamWindow * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void StreamManager::streamWindowClosed(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void StreamManager::connectionStatusChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void StreamManager::errorOccurred(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}
QT_WARNING_POP
