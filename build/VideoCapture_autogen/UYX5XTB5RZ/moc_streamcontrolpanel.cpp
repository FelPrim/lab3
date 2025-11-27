/****************************************************************************
** Meta object code from reading C++ file 'streamcontrolpanel.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/ui/streamcontrolpanel.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'streamcontrolpanel.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_StreamControlPanel_t {
    uint offsetsAndSizes[18];
    char stringdata0[19];
    char stringdata1[21];
    char stringdata2[1];
    char stringdata3[20];
    char stringdata4[21];
    char stringdata5[20];
    char stringdata6[19];
    char stringdata7[15];
    char stringdata8[20];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_StreamControlPanel_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_StreamControlPanel_t qt_meta_stringdata_StreamControlPanel = {
    {
        QT_MOC_LITERAL(0, 18),  // "StreamControlPanel"
        QT_MOC_LITERAL(19, 20),  // "startStreamRequested"
        QT_MOC_LITERAL(40, 0),  // ""
        QT_MOC_LITERAL(41, 19),  // "stopStreamRequested"
        QT_MOC_LITERAL(61, 20),  // "leaveStreamRequested"
        QT_MOC_LITERAL(82, 19),  // "disconnectRequested"
        QT_MOC_LITERAL(102, 18),  // "onStartStopClicked"
        QT_MOC_LITERAL(121, 14),  // "onLeaveClicked"
        QT_MOC_LITERAL(136, 19)   // "onDisconnectClicked"
    },
    "StreamControlPanel",
    "startStreamRequested",
    "",
    "stopStreamRequested",
    "leaveStreamRequested",
    "disconnectRequested",
    "onStartStopClicked",
    "onLeaveClicked",
    "onDisconnectClicked"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_StreamControlPanel[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   56,    2, 0x06,    1 /* Public */,
       3,    0,   57,    2, 0x06,    2 /* Public */,
       4,    0,   58,    2, 0x06,    3 /* Public */,
       5,    0,   59,    2, 0x06,    4 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       6,    0,   60,    2, 0x08,    5 /* Private */,
       7,    0,   61,    2, 0x08,    6 /* Private */,
       8,    0,   62,    2, 0x08,    7 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject StreamControlPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_StreamControlPanel.offsetsAndSizes,
    qt_meta_data_StreamControlPanel,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_StreamControlPanel_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<StreamControlPanel, std::true_type>,
        // method 'startStreamRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stopStreamRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'leaveStreamRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'disconnectRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onStartStopClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onLeaveClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDisconnectClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void StreamControlPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<StreamControlPanel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->startStreamRequested(); break;
        case 1: _t->stopStreamRequested(); break;
        case 2: _t->leaveStreamRequested(); break;
        case 3: _t->disconnectRequested(); break;
        case 4: _t->onStartStopClicked(); break;
        case 5: _t->onLeaveClicked(); break;
        case 6: _t->onDisconnectClicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (StreamControlPanel::*)();
            if (_t _q_method = &StreamControlPanel::startStreamRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (StreamControlPanel::*)();
            if (_t _q_method = &StreamControlPanel::stopStreamRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (StreamControlPanel::*)();
            if (_t _q_method = &StreamControlPanel::leaveStreamRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (StreamControlPanel::*)();
            if (_t _q_method = &StreamControlPanel::disconnectRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
    }
    (void)_a;
}

const QMetaObject *StreamControlPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *StreamControlPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_StreamControlPanel.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int StreamControlPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void StreamControlPanel::startStreamRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void StreamControlPanel::stopStreamRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void StreamControlPanel::leaveStreamRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void StreamControlPanel::disconnectRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
