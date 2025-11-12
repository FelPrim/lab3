/****************************************************************************
** Meta object code from reading C++ file 'streampublisherwindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../new/streampublisherwindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'streampublisherwindow.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN21StreamPublisherWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto StreamPublisherWindow::qt_create_metaobjectdata<qt_meta_tag_ZN21StreamPublisherWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "StreamPublisherWindow",
        "streamStopped",
        "",
        "streamId",
        "setViewersStatus",
        "hasViewers",
        "setStreamId",
        "displayId",
        "onStopRequested",
        "onVideoError",
        "message"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'streamStopped'
        QtMocHelpers::SignalData<void(int)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 },
        }}),
        // Slot 'setViewersStatus'
        QtMocHelpers::SlotData<void(bool)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 5 },
        }}),
        // Slot 'setStreamId'
        QtMocHelpers::SlotData<void(int, const QString &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::QString, 7 },
        }}),
        // Slot 'onStopRequested'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onVideoError'
        QtMocHelpers::SlotData<void(const QString &)>(9, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 10 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<StreamPublisherWindow, qt_meta_tag_ZN21StreamPublisherWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject StreamPublisherWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<StreamWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN21StreamPublisherWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN21StreamPublisherWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN21StreamPublisherWindowE_t>.metaTypes,
    nullptr
} };

void StreamPublisherWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<StreamPublisherWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->streamStopped((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 1: _t->setViewersStatus((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 2: _t->setStreamId((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 3: _t->onStopRequested(); break;
        case 4: _t->onVideoError((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (StreamPublisherWindow::*)(int )>(_a, &StreamPublisherWindow::streamStopped, 0))
            return;
    }
}

const QMetaObject *StreamPublisherWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *StreamPublisherWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN21StreamPublisherWindowE_t>.strings))
        return static_cast<void*>(this);
    return StreamWindow::qt_metacast(_clname);
}

int StreamPublisherWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = StreamWindow::qt_metacall(_c, _id, _a);
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
void StreamPublisherWindow::streamStopped(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}
QT_WARNING_POP
