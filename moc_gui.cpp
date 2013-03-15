/****************************************************************************
** Meta object code from reading C++ file 'gui.h'
**
** Created: Fri Mar 15 19:46:05 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "gui.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'gui.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_LDForgeWindow[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      18,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      15,   14,   14,   14, 0x08,
      26,   14,   14,   14, 0x08,
      38,   14,   14,   14, 0x08,
      50,   14,   14,   14, 0x08,
      64,   14,   14,   14, 0x08,
      76,   14,   14,   14, 0x08,
      94,   14,   14,   14, 0x08,
     109,   14,   14,   14, 0x08,
     128,   14,   14,   14, 0x08,
     143,   14,   14,   14, 0x08,
     162,   14,   14,   14, 0x08,
     180,   14,   14,   14, 0x08,
     197,   14,   14,   14, 0x08,
     214,   14,   14,   14, 0x08,
     225,   14,   14,   14, 0x08,
     237,   14,   14,   14, 0x08,
     250,   14,   14,   14, 0x08,
     263,   14,   14,   14, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_LDForgeWindow[] = {
    "LDForgeWindow\0\0slot_new()\0slot_open()\0"
    "slot_save()\0slot_saveAs()\0slot_exit()\0"
    "slot_newSubfile()\0slot_newLine()\0"
    "slot_newTriangle()\0slot_newQuad()\0"
    "slot_newCondLine()\0slot_newComment()\0"
    "slot_newVector()\0slot_newVertex()\0"
    "slot_cut()\0slot_copy()\0slot_paste()\0"
    "slot_about()\0slot_aboutQt()\0"
};

void LDForgeWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        LDForgeWindow *_t = static_cast<LDForgeWindow *>(_o);
        switch (_id) {
        case 0: _t->slot_new(); break;
        case 1: _t->slot_open(); break;
        case 2: _t->slot_save(); break;
        case 3: _t->slot_saveAs(); break;
        case 4: _t->slot_exit(); break;
        case 5: _t->slot_newSubfile(); break;
        case 6: _t->slot_newLine(); break;
        case 7: _t->slot_newTriangle(); break;
        case 8: _t->slot_newQuad(); break;
        case 9: _t->slot_newCondLine(); break;
        case 10: _t->slot_newComment(); break;
        case 11: _t->slot_newVector(); break;
        case 12: _t->slot_newVertex(); break;
        case 13: _t->slot_cut(); break;
        case 14: _t->slot_copy(); break;
        case 15: _t->slot_paste(); break;
        case 16: _t->slot_about(); break;
        case 17: _t->slot_aboutQt(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData LDForgeWindow::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject LDForgeWindow::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_LDForgeWindow,
      qt_meta_data_LDForgeWindow, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &LDForgeWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *LDForgeWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *LDForgeWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_LDForgeWindow))
        return static_cast<void*>(const_cast< LDForgeWindow*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int LDForgeWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
