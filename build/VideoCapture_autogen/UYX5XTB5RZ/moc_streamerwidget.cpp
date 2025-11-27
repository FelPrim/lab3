/****************************************************************************
** Meta object code from reading C++ file 'streamerwidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/ui/streamerwidget.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'streamerwidget.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_StreamerWidget_t {
    uint offsetsAndSizes[46];
    char stringdata0[15];
    char stringdata1[14];
    char stringdata2[1];
    char stringdata3[9];
    char stringdata4[9];
    char stringdata5[22];
    char stringdata6[8];
    char stringdata7[19];
    char stringdata8[12];
    char stringdata9[7];
    char stringdata10[20];
    char stringdata11[12];
    char stringdata12[13];
    char stringdata13[8];
    char stringdata14[16];
    char stringdata15[6];
    char stringdata16[19];
    char stringdata17[8];
    char stringdata18[6];
    char stringdata19[15];
    char stringdata20[23];
    char stringdata21[22];
    char stringdata22[22];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_StreamerWidget_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_StreamerWidget_t qt_meta_stringdata_StreamerWidget = {
    {
        QT_MOC_LITERAL(0, 14),  // "StreamerWidget"
        QT_MOC_LITERAL(15, 13),  // "streamStopped"
        QT_MOC_LITERAL(29, 0),  // ""
        QT_MOC_LITERAL(30, 8),  // "uint32_t"
        QT_MOC_LITERAL(39, 8),  // "streamId"
        QT_MOC_LITERAL(48, 21),  // "streamingStateChanged"
        QT_MOC_LITERAL(70, 7),  // "enabled"
        QT_MOC_LITERAL(78, 18),  // "encodedPacketReady"
        QT_MOC_LITERAL(97, 11),  // "frameNumber"
        QT_MOC_LITERAL(109, 6),  // "packet"
        QT_MOC_LITERAL(116, 19),  // "disconnectRequested"
        QT_MOC_LITERAL(136, 11),  // "deviceIndex"
        QT_MOC_LITERAL(148, 12),  // "onVideoError"
        QT_MOC_LITERAL(161, 7),  // "message"
        QT_MOC_LITERAL(169, 15),  // "onRawFrameReady"
        QT_MOC_LITERAL(185, 5),  // "image"
        QT_MOC_LITERAL(191, 18),  // "onFrameForEncoding"
        QT_MOC_LITERAL(210, 7),  // "cv::Mat"
        QT_MOC_LITERAL(218, 5),  // "frame"
        QT_MOC_LITERAL(224, 14),  // "onFrameEncoded"
        QT_MOC_LITERAL(239, 22),  // "onStartStreamRequested"
        QT_MOC_LITERAL(262, 21),  // "onStopStreamRequested"
        QT_MOC_LITERAL(284, 21)   // "onDisconnectRequested"
    },
    "StreamerWidget",
    "streamStopped",
    "",
    "uint32_t",
    "streamId",
    "streamingStateChanged",
    "enabled",
    "encodedPacketReady",
    "frameNumber",
    "packet",
    "disconnectRequested",
    "deviceIndex",
    "onVideoError",
    "message",
    "onRawFrameReady",
    "image",
    "onFrameForEncoding",
    "cv::Mat",
    "frame",
    "onFrameEncoded",
    "onStartStreamRequested",
    "onStopStreamRequested",
    "onDisconnectRequested"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_StreamerWidget[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   80,    2, 0x06,    1 /* Public */,
       5,    2,   83,    2, 0x06,    3 /* Public */,
       7,    3,   88,    2, 0x06,    6 /* Public */,
      10,    1,   95,    2, 0x06,   10 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      12,    1,   98,    2, 0x0a,   12 /* Public */,
      14,    1,  101,    2, 0x0a,   14 /* Public */,
      16,    1,  104,    2, 0x0a,   16 /* Public */,
      19,    3,  107,    2, 0x0a,   18 /* Public */,
      20,    0,  114,    2, 0x08,   22 /* Private */,
      21,    0,  115,    2, 0x08,   23 /* Private */,
      22,    0,  116,    2, 0x08,   24 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3, QMetaType::Bool,    4,    6,
    QMetaType::Void, 0x80000000 | 3, QMetaType::Int, QMetaType::QByteArray,    4,    8,    9,
    QMetaType::Void, QMetaType::Int,   11,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,   13,
    QMetaType::Void, QMetaType::QImage,   15,
    QMetaType::Void, 0x80000000 | 17,   18,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::QByteArray,    4,    8,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject StreamerWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_StreamerWidget.offsetsAndSizes,
    qt_meta_data_StreamerWidget,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_StreamerWidget_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<StreamerWidget, std::true_type>,
        // method 'streamStopped'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        // method 'streamingStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'encodedPacketReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<uint32_t, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QByteArray &, std::false_type>,
        // method 'disconnectRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onVideoError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onRawFrameReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QImage &, std::false_type>,
        // method 'onFrameForEncoding'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const cv::Mat &, std::false_type>,
        // method 'onFrameEncoded'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QByteArray &, std::false_type>,
        // method 'onStartStreamRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onStopStreamRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDisconnectRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void StreamerWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<StreamerWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->streamStopped((*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 1: _t->streamingStateChanged((*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 2: _t->encodedPacketReady((*reinterpret_cast< std::add_pointer_t<uint32_t>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QByteArray>>(_a[3]))); break;
        case 3: _t->disconnectRequested((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->onVideoError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->onRawFrameReady((*reinterpret_cast< std::add_pointer_t<QImage>>(_a[1]))); break;
        case 6: _t->onFrameForEncoding((*reinterpret_cast< std::add_pointer_t<cv::Mat>>(_a[1]))); break;
        case 7: _t->onFrameEncoded((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QByteArray>>(_a[3]))); break;
        case 8: _t->onStartStreamRequested(); break;
        case 9: _t->onStopStreamRequested(); break;
        case 10: _t->onDisconnectRequested(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (StreamerWidget::*)(uint32_t );
            if (_t _q_method = &StreamerWidget::streamStopped; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (StreamerWidget::*)(uint32_t , bool );
            if (_t _q_method = &StreamerWidget::streamingStateChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (StreamerWidget::*)(uint32_t , int , const QByteArray & );
            if (_t _q_method = &StreamerWidget::encodedPacketReady; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (StreamerWidget::*)(int );
            if (_t _q_method = &StreamerWidget::disconnectRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
    }
}

const QMetaObject *StreamerWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *StreamerWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_StreamerWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int StreamerWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void StreamerWidget::streamStopped(uint32_t _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void StreamerWidget::streamingStateChanged(uint32_t _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void StreamerWidget::encodedPacketReady(uint32_t _t1, int _t2, const QByteArray & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void StreamerWidget::disconnectRequested(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
