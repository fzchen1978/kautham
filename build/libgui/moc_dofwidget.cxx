/****************************************************************************
** Meta object code from reading C++ file 'dofwidget.h'
**
** Created: Thu Apr 10 11:50:35 2014
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../libgui/dofwidget.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dofwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Kautham__DOFWidget[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      24,   20,   19,   19, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_Kautham__DOFWidget[] = {
    "Kautham::DOFWidget\0\0val\0sliderChanged(int)\0"
};

void Kautham::DOFWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        DOFWidget *_t = static_cast<DOFWidget *>(_o);
        switch (_id) {
        case 0: _t->sliderChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Kautham::DOFWidget::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Kautham::DOFWidget::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_Kautham__DOFWidget,
      qt_meta_data_Kautham__DOFWidget, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Kautham::DOFWidget::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Kautham::DOFWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Kautham::DOFWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Kautham__DOFWidget))
        return static_cast<void*>(const_cast< DOFWidget*>(this));
    return QWidget::qt_metacast(_clname);
}

int Kautham::DOFWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
