/****************************************************************************
** Meta object code from reading C++ file 'conferencecontrolpanel.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/ui/conferencecontrolpanel.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'conferencecontrolpanel.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_ConferenceControlPanel_t {
    uint offsetsAndSizes[24];
    char stringdata0[23];
    char stringdata1[19];
    char stringdata2[1];
    char stringdata3[25];
    char stringdata4[21];
    char stringdata5[9];
    char stringdata6[9];
    char stringdata7[22];
    char stringdata8[19];
    char stringdata9[25];
    char stringdata10[21];
    char stringdata11[22];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_ConferenceControlPanel_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_ConferenceControlPanel_t qt_meta_stringdata_ConferenceControlPanel = {
    {
        QT_MOC_LITERAL(0, 22),  // "ConferenceControlPanel"
        QT_MOC_LITERAL(23, 18),  // "addDeviceRequested"
        QT_MOC_LITERAL(42, 0),  // ""
        QT_MOC_LITERAL(43, 24),  // "leaveConferenceRequested"
        QT_MOC_LITERAL(68, 20),  // "watchStreamRequested"
        QT_MOC_LITERAL(89, 8),  // "uint32_t"
        QT_MOC_LITERAL(98, 8),  // "streamId"
        QT_MOC_LITERAL(107, 21),  // "stopWatchingRequested"
        QT_MOC_LITERAL(129, 18),  // "onAddDeviceClicked"
        QT_MOC_LITERAL(148, 24),  // "onLeaveConferenceClicked"
        QT_MOC_LITERAL(173, 20),  // "onWatchStreamClicked"
        QT_MOC_LITERAL(194, 21)   // "onStopWatchingClicked"
    },
    "ConferenceControlPanel",
    "addDeviceRequested",
    "",
    "leaveConferenceRequested",
    "watchStreamRequested",
    "uint32_t",
    "streamId",
    "stopWatchingRequested",
    "onAddDeviceClicked",
    "onLeaveConferenceClicked",
    "onWatchStreamClicked",
    "onStopWatchingClicked"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_ConferenceControlPanel[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   62,    2, 0x06,    1 /* Public */,
       3,    0,   63,    2, 0x06,    2 /* Public */,
       4,    1,   64,    2, 0x06,    3 /* Public */,
       7,    1,   67,    2, 0x06,    5 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       8,    0,   70,    2, 0x08,    7 /* Private */,
       9,    0,   71,    2, 0x08,    8 /* Private */,
      10,    1,   72,    2, 0x08,    9 /* Private */,
      11,    1,   75,    2, 0x08,   11 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void, 0x80000000 | 5,    6,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void, 0x80000000 | 5,    6,

       0        // eod
};

Q_CONSTINIT const QMetaObject ConferenceControlPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ConferenceControlPanel.offsetsAndSizes,
    qt_meta_data_ConferenceControlPanel,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_ConferenceControlPanel_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ConferenceControlPanel, std::true_type>,
        // method 'addDeviceRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'leaveConferenceRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'watchStreamRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        // method 'stopWatchingRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        // method 'onAddDeviceClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onLeaveConferenceClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onWatchStreamClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        // method 'onStopWatchingClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>
    >,
    nullptr
} };

void ConferenceControlPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ConferenceControlPanel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->addDeviceRequested(); break;
        case 1: _t->leaveConferenceRequested(); break;
        case 2: _t->watchStreamRequested((*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 3: _t->stopWatchingRequested((*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 4: _t->onAddDeviceClicked(); break;
        case 5: _t->onLeaveConferenceClicked(); break;
        case 6: _t->onWatchStreamClicked((*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 7: _t->onStopWatchingClicked((*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ConferenceControlPanel::*)();
            if (_t _q_method = &ConferenceControlPanel::addDeviceRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ConferenceControlPanel::*)();
            if (_t _q_method = &ConferenceControlPanel::leaveConferenceRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ConferenceControlPanel::*)(uint32_t );
            if (_t _q_method = &ConferenceControlPanel::watchStreamRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ConferenceControlPanel::*)(uint32_t );
            if (_t _q_method = &ConferenceControlPanel::stopWatchingRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
    }
}

const QMetaObject *ConferenceControlPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ConferenceControlPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ConferenceControlPanel.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ConferenceControlPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void ConferenceControlPanel::addDeviceRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ConferenceControlPanel::leaveConferenceRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ConferenceControlPanel::watchStreamRequested(uint32_t _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ConferenceControlPanel::stopWatchingRequested(uint32_t _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
