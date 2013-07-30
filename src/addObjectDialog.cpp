/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri Piippo
 *  
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qgridlayout.h>
#include <qcheckbox.h>
#include <qdialogbuttonbox.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <qlistwidget.h>
#include <qtreewidget.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include "gui.h"
#include "addObjectDialog.h"
#include "file.h"
#include "colors.h"
#include "colorSelectDialog.h"
#include "history.h"
#include "widgets.h"
#include "misc.h"
#include "primitives.h"

class SubfileListItem : public QTreeWidgetItem {
	PROPERTY (Primitive*, primInfo, setPrimInfo)
	
public:
	SubfileListItem (QTreeWidgetItem* parent, Primitive* info) :
		QTreeWidgetItem (parent), m_primInfo (info) {}
	SubfileListItem (QTreeWidget* parent, Primitive* info) :
		QTreeWidgetItem (parent), m_primInfo (info) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
AddObjectDialog::AddObjectDialog (const LDObject::Type type, LDObject* obj, QWidget* parent) : QDialog (parent) {
	setlocale (LC_ALL, "C");
	
	short coordCount = 0;
	str typeName = LDObject::typeName( type );
	
	switch (type) {
	case LDObject::Comment:
		le_comment = new QLineEdit;
		if (obj)
			le_comment->setText (static_cast<LDCommentObject*> (obj)->text);
		
		le_comment->setMinimumWidth (384);
		break;
	
	case LDObject::Line:
		coordCount = 6;
		break;
	
	case LDObject::Triangle:
		coordCount = 9;
		break;
	
	case LDObject::Quad:
	case LDObject::CondLine:
		coordCount = 12;
		break;
	
	case LDObject::Vertex:
		coordCount = 3;
		break;
	
	case LDObject::BFC:
		rb_bfcType = new RadioBox ("Statement", {}, 0, Qt::Vertical);
		
		for (int i = 0; i < LDBFCObject::NumStatements; ++i)
			rb_bfcType->addButton (LDBFCObject::statements[i]);
		
		if (obj)
			rb_bfcType->setValue ((int) static_cast<LDBFCObject*> (obj)->type);
		break;
	
	case LDObject::Subfile:
		{
			coordCount = 3;
			
			// If the primitive lister is busy writing data, we have to wait
			// for that to happen first. This should be quite considerably rare.
			while (primitiveLoaderBusy())
				;
			
			tw_subfileList = new QTreeWidget ();
			tw_subfileList->setHeaderLabel (tr ("Primitives"));
			
			for (PrimitiveCategory& cat : g_PrimitiveCategories) {
				SubfileListItem* parentItem = new SubfileListItem (tw_subfileList, null);
				parentItem->setText (0, cat.name ());
				QList<QTreeWidgetItem*> subfileItems;
				
				for (Primitive& prim : cat.prims) {
					SubfileListItem* item = new SubfileListItem (parentItem, &prim);
					item->setText (0, fmt ("%1 - %2", prim.name, prim.title));
					subfileItems << item;
					
					// If this primitive is the one the current object points to,
					// select it by default
					if (obj && static_cast<LDSubfileObject*> (obj)->fileInfo ()->name () == prim.name)
						tw_subfileList->setCurrentItem (item);
				}
				
				tw_subfileList->addTopLevelItem (parentItem);
			}
			
			connect (tw_subfileList, SIGNAL (itemSelectionChanged ()), this, SLOT (slot_subfileTypeChanged ()));
			lb_subfileName = new QLabel ("File:");
			le_subfileName = new QLineEdit;
			le_subfileName->setFocus ();
			
			if (obj) {
				LDSubfileObject* ref = static_cast<LDSubfileObject*> (obj);
				le_subfileName->setText (ref->fileInfo ()->name ());
			}
			break;
		}
	
	default:
		critical (fmt ("Unhandled LDObject type %1 (%2) in AddObjectDialog", (int) type, typeName));
		return;
	}
	
	QPixmap icon = getIcon (fmt ("add-%1", typeName));
	LDObject* defaults = LDObject::getDefault (type);
	
	lb_typeIcon = new QLabel;
	lb_typeIcon->setPixmap (icon);
	
	// Show a color edit dialog for the types that actually use the color
	if (defaults->isColored ()) {
		if (obj != null)
			colnum = obj->color ();
		else
			colnum = (type == LDObject::CondLine || type == LDObject::Line) ? edgecolor : maincolor;
		
		pb_color = new QPushButton;
		setButtonBackground (pb_color, colnum);
		connect (pb_color, SIGNAL (clicked ()), this, SLOT (slot_colorButtonClicked ()));
	}
	
	for (short i = 0; i < coordCount; ++i) {
		dsb_coords[i] = new QDoubleSpinBox;
		dsb_coords[i]->setDecimals (5);
		dsb_coords[i]->setMinimum (-10000.0);
		dsb_coords[i]->setMaximum (10000.0);
	}
	
	QGridLayout* const layout = new QGridLayout;
	layout->addWidget (lb_typeIcon, 0, 0);
	
	switch (type) {
	case LDObject::Line:
	case LDObject::CondLine:
	case LDObject::Triangle:
	case LDObject::Quad:
		// Apply coordinates
		if (obj) {
			for (short i = 0; i < coordCount / 3; ++i)
			for (short j = 0; j < 3; ++j)
				dsb_coords[(i * 3) + j]->setValue (obj->getVertex (i).coord (j));
		}
		break;
	
	case LDObject::Comment:
		layout->addWidget (le_comment, 0, 1);
		break;
	
	case LDObject::BFC:
		layout->addWidget (rb_bfcType, 0, 1);
		break;
	
	case LDObject::Subfile:
		layout->addWidget (tw_subfileList, 1, 1, 1, 2);
		layout->addWidget (lb_subfileName, 2, 1);
		layout->addWidget (le_subfileName, 2, 2);
		break;
	
	default:
		break;
	}
	
	if (defaults->hasMatrix ()) {
		LDMatrixObject* mo = obj ? dynamic_cast<LDMatrixObject*> (obj) : null;
		
		QLabel* lb_matrix = new QLabel ("Matrix:");
		le_matrix = new QLineEdit;
		// le_matrix->setValidator (new QDoubleValidator);
		matrix defaultMatrix = g_identity;
		
		if (mo) {
			for (const Axis ax : g_Axes)
				dsb_coords[ax]->setValue (mo->position ()[ax]);
			
			defaultMatrix = mo->transform ();
		}
		
		le_matrix->setText (defaultMatrix.stringRep ());
		layout->addWidget (lb_matrix, 4, 1);
		layout->addWidget (le_matrix, 4, 2, 1, 3);
	}
	
	if (defaults->isColored ())
		layout->addWidget (pb_color, 1, 0);
	
	if (coordCount > 0) {
		QGridLayout* const qCoordLayout = new QGridLayout;
		
		for (short i = 0; i < coordCount; ++i)
			qCoordLayout->addWidget (dsb_coords[i], (i / 3), (i % 3));
		
		layout->addLayout (qCoordLayout, 0, 1, (coordCount / 3), 3);
	}
	
	QDialogButtonBox* bbx_buttons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	QWidget::connect (bbx_buttons, SIGNAL (accepted()), this, SLOT (accept()));
	QWidget::connect (bbx_buttons, SIGNAL (rejected()), this, SLOT (reject()));
	layout->addWidget (bbx_buttons, 5, 0, 1, 4);
	setLayout (layout);
	setWindowTitle (fmt (tr ("Edit %1"), typeName));
	
	setWindowIcon (icon);
	delete defaults;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void AddObjectDialog::setButtonBackground (QPushButton* button, short colnum) {
	LDColor* col = getColor ( colnum );
	
	button->setIcon (getIcon ("palette"));
	button->setAutoFillBackground (true);
	
	if( col )
		button->setStyleSheet (fmt ("background-color: %1", col->hexcode));
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
str AddObjectDialog::currentSubfileName () {
	SubfileListItem* item = static_cast<SubfileListItem*> (tw_subfileList->currentItem ());
	
	if (item->primInfo () == null)
		return ""; // selected a heading
	
	return item->primInfo ()->name;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void AddObjectDialog::slot_colorButtonClicked () {
	ColorSelector::getColor (colnum, colnum, this);
	setButtonBackground (pb_color, colnum);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void AddObjectDialog::slot_subfileTypeChanged () {
	str name = currentSubfileName ();
	
	if (name.length () > 0)
		le_subfileName->setText (name);
}

// =============================================================================
template<class T> T* initObj (LDObject*& obj) {
	if (obj == null)
		obj = new T;
	
	return static_cast<T*> (obj);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void AddObjectDialog::staticDialog (const LDObject::Type type, LDObject* obj) {
	setlocale (LC_ALL, "C");
	
	if (obj && obj->getType () == LDObject::Error)
		return;
	
	if (type == LDObject::Empty)
		return; // Nothing to edit with empties
	
	const bool newObject = (obj == null);
	AddObjectDialog dlg (type, obj);
	
	if (obj)
		assert (obj->getType () == type);
	
	if (dlg.exec () == false)
		return;
	
	matrix transform = g_identity;
	if( type == LDObject::Subfile ) {
		List<str> matrixstrvals = container_cast<QStringList, List<str>> (str (dlg.le_matrix->text ()).split (" "));
		
		if (matrixstrvals.size () == 9) {
			double matrixvals[9];
			int i = 0;
			
			for (str val : matrixstrvals)
				matrixvals[i++] = val.toFloat ();
			
			transform = matrix (matrixvals);
		}
	}
	
	switch (type) {
	case LDObject::Comment:
		{
			LDCommentObject* comm = initObj<LDCommentObject> (obj);
			comm->text = dlg.le_comment->text ();
		}
		break;
	
	case LDObject::Line:
	case LDObject::Triangle:
	case LDObject::Quad:
	case LDObject::CondLine:
		if (!obj)
			obj = LDObject::getDefault (type);
		
		for (short i = 0; i < obj->vertices (); ++i) {
			vertex v;
			for (const Axis ax : g_Axes)
				v[ax] = dlg.dsb_coords[(i * 3) + ax]->value ();
			
			obj->setVertex (i, v);
		}
		break;
	
	case LDObject::BFC:
		{
			LDBFCObject* bfc = initObj<LDBFCObject> (obj);
			bfc->type = (LDBFCObject::Type) dlg.rb_bfcType->value ();
		}
		break;
	
	case LDObject::Vertex:
		{
			LDVertexObject* vert = initObj<LDVertexObject> (obj);
			
			for (const Axis ax : g_Axes)
				vert->pos[ax] = dlg.dsb_coords[ax]->value ();
		}
		break;
	
	case LDObject::Subfile:
		{
			str name = dlg.le_subfileName->text ();
			if (name.length () == 0)
				return; // no subfile filename
			
			LDOpenFile* file = getFile (name);
			if (!file) {
				critical (fmt ("Couldn't open `%1': %2", name, strerror (errno)));
				return;
			}
			
			LDSubfileObject* ref = initObj<LDSubfileObject> (obj);
			
			for (const Axis ax : g_Axes)
				ref->setCoordinate (ax, dlg.dsb_coords[ax]->value ());
			
			ref->setTransform (transform);
			ref->setFileInfo (file);
		}
		break;
	
	default:
		break;
	}
	
	if (obj->isColored ())
		obj->setColor (dlg.colnum);
	
	if (newObject) {
		ulong idx = g_win->getInsertionPoint ();
		LDOpenFile::current()->insertObj (idx, obj);
	}
	
	g_win->fullRefresh ();
}