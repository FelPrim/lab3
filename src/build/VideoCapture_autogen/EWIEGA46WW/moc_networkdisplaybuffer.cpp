/****************************************************************************
** Meta object code from reading C++ file 'networkdisplaybuffer.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../networkdisplaybuffer.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'networkdisplaybuffer.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN20NetworkDisplayBufferE_t {};
} // unnamed namespace

template <> constexpr inline auto NetworkDisplayBuffer::qt_create_metaobjectdata<qt_meta_tag_ZN20NetworkDisplayBufferE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "NetworkDisplayBuffer",
        "frameReady",
        "",
        "QImage",
        "image",
        "streamId",
        "errorOccurred",
        "message",
        "addFrame",
        "frameNumber",
        "frameData",
        "onFrameDecoded",
        "processNextFrameImmediately"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'frameReady'
        QtMocHelpers::SignalData<void(const QImage &, int)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { QMetaType::Int, 5 },
        }}),
        // Signal 'errorOccurred'
        QtMocHelpers::SignalData<void(const QString &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 7 },
        }}),
        // Slot 'addFrame'
        QtMocHelpers::SlotData<void(int, const QByteArray &)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 9 }, { QMetaType::QByteArray, 10 },
        }}),
        // Slot 'onFrameDecoded'
        QtMocHelpers::SlotData<void(const QImage &, int)>(11, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { QMetaType::Int, 9 },
        }}),
        // Slot 'processNextFrameImmediately'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<NetworkDisplayBuffer, qt_meta_tag_ZN20NetworkDisplayBufferE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject NetworkDisplayBuffer::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20NetworkDisplayBufferE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20NetworkDisplayBufferE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN20NetworkDisplayBufferE_t>.metaTypes,
    nullptr
} };

void NetworkDisplayBuffer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<NetworkDisplayBuffer *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->frameReady((*reinterpret_cast<std::add_pointer_t<QImage>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 1: _t->errorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->addFrame((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QByteArray>>(_a[2]))); break;
        case 3: _t->onFrameDecoded((*reinterpret_cast<std::add_pointer_t<QImage>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 4: _t->processNextFrameImmediately(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (NetworkDisplayBuffer::*)(const QImage & , int )>(_a, &NetworkDisplayBuffer::frameReady, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkDisplayBuffer::*)(const QString & )>(_a, &NetworkDisplayBuffer::errorOccurred, 1))
            return;
    }
}

const QMetaObject *NetworkDisplayBuffer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NetworkDisplayBuffer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20NetworkDisplayBufferE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int NetworkDisplayBuffer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void NetworkDisplayBuffer::frameReady(const QImage & _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void NetworkDisplayBuffer::errorOccurred(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}
QT_WARNING_POP
