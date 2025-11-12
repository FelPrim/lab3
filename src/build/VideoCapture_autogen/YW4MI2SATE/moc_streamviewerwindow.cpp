/****************************************************************************
** Meta object code from reading C++ file 'streamviewerwindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../new/streamviewerwindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'streamviewerwindow.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN18StreamViewerWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto StreamViewerWindow::qt_create_metaobjectdata<qt_meta_tag_ZN18StreamViewerWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "StreamViewerWindow",
        "streamJoined",
        "",
        "streamId",
        "displayId",
        "streamLeft",
        "setStreamId",
        "onJoinRequested",
        "onLeaveRequested",
        "onFrameReady",
        "QImage",
        "image",
        "onDecoderError",
        "message"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'streamJoined'
        QtMocHelpers::SignalData<void(int, const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::QString, 4 },
        }}),
        // Signal 'streamLeft'
        QtMocHelpers::SignalData<void(int)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Slot 'setStreamId'
        QtMocHelpers::SlotData<void(int, const QString &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::QString, 4 },
        }}),
        // Slot 'onJoinRequested'
        QtMocHelpers::SlotData<void(const QString &)>(7, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Slot 'onLeaveRequested'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onFrameReady'
        QtMocHelpers::SlotData<void(const QImage &, int)>(9, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 10, 11 }, { QMetaType::Int, 3 },
        }}),
        // Slot 'onDecoderError'
        QtMocHelpers::SlotData<void(const QString &)>(12, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 13 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<StreamViewerWindow, qt_meta_tag_ZN18StreamViewerWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject StreamViewerWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<StreamWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18StreamViewerWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18StreamViewerWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN18StreamViewerWindowE_t>.metaTypes,
    nullptr
} };

void StreamViewerWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<StreamViewerWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->streamJoined((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 1: _t->streamLeft((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->setStreamId((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 3: _t->onJoinRequested((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->onLeaveRequested(); break;
        case 5: _t->onFrameReady((*reinterpret_cast<std::add_pointer_t<QImage>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 6: _t->onDecoderError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (StreamViewerWindow::*)(int , const QString & )>(_a, &StreamViewerWindow::streamJoined, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (StreamViewerWindow::*)(int )>(_a, &StreamViewerWindow::streamLeft, 1))
            return;
    }
}

const QMetaObject *StreamViewerWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *StreamViewerWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18StreamViewerWindowE_t>.strings))
        return static_cast<void*>(this);
    return StreamWindow::qt_metacast(_clname);
}

int StreamViewerWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = StreamWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void StreamViewerWindow::streamJoined(int _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void StreamViewerWindow::streamLeft(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}
QT_WARNING_POP
