/****************************************************************************
** Meta object code from reading C++ file 'videodisplay.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../videodisplay.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'videodisplay.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN12VideoDisplayE_t {};
} // unnamed namespace

template <> constexpr inline auto VideoDisplay::qt_create_metaobjectdata<qt_meta_tag_ZN12VideoDisplayE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "VideoDisplay",
        "displayFrame",
        "",
        "QImage",
        "frame",
        "displayFrameFromNetwork"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'displayFrame'
        QtMocHelpers::SlotData<void(const QImage &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Slot 'displayFrameFromNetwork'
        QtMocHelpers::SlotData<void(const QImage &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<VideoDisplay, qt_meta_tag_ZN12VideoDisplayE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject VideoDisplay::staticMetaObject = { {
    QMetaObject::SuperData::link<QLabel::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12VideoDisplayE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12VideoDisplayE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN12VideoDisplayE_t>.metaTypes,
    nullptr
} };

void VideoDisplay::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<VideoDisplay *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->displayFrame((*reinterpret_cast<std::add_pointer_t<QImage>>(_a[1]))); break;
        case 1: _t->displayFrameFromNetwork((*reinterpret_cast<std::add_pointer_t<QImage>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *VideoDisplay::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *VideoDisplay::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12VideoDisplayE_t>.strings))
        return static_cast<void*>(this);
    return QLabel::qt_metacast(_clname);
}

int VideoDisplay::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QLabel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 2;
    }
    return _id;
}
QT_WARNING_POP
