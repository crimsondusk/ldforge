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

class SubfileListItem : public QTreeWidgetItem {
	PROPERTY (PrimitiveInfo*, primInfo, setPrimInfo)
	
public:
	SubfileListItem (QTreeWidgetItem* parent, PrimitiveInfo* info) :
		QTreeWidgetItem (parent), m_primInfo (info) {}
	SubfileListItem (QTreeWidget* parent, PrimitiveInfo* info) :
		QTreeWidgetItem (parent), m_primInfo (info) {}
};

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
AddObjectDialog::AddObjectDialog (const LDObject::Type type, LDObject* obj, QWidget* parent) :
	QDialog (parent)
{
	setlocale (LC_ALL, "C");
	
	short coordCount = 0;
	
	switch (type) {
	case LDObject::Comment:
		le_comment = new QLineEdit;
		if (obj)
			le_comment->setText (static_cast<LDComment*> (obj)->text);
		
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
		
		for (int i = 0; i < LDBFC::NumStatements; ++i)
			rb_bfcType->addButton (LDBFC::statements[i]);
		
		if (obj)
			rb_bfcType->setValue ((int) static_cast<LDBFC*> (obj)->type);
		break;
	
	case LDObject::Subfile:
		{
			coordCount = 3;
			
			tw_subfileList = new QTreeWidget ();
			SubfileListItem* parentItem = new SubfileListItem (tw_subfileList, null);
			parentItem->setText (0, "Primitives");
			QList<QTreeWidgetItem*> subfileItems;
			
			for (PrimitiveInfo& info : g_Primitives) {
				SubfileListItem* item = new SubfileListItem (parentItem, &info);
				item->setText (0, fmt ("%1 - %2", info.name, info.title));
				subfileItems << item;
			}
			
			tw_subfileList->addTopLevelItem (parentItem);
			connect (tw_subfileList, SIGNAL (itemSelectionChanged ()), this, SLOT (slot_subfileTypeChanged ()));
			lb_subfileName = new QLabel ("File:");
			le_subfileName = new QLineEdit;
			le_subfileName->setFocus ();
			
			if (obj) {
				LDSubfile* ref = static_cast<LDSubfile*> (obj);
				le_subfileName->setText (ref->fileInfo ()->name ());
			}
			break;
		}
	
	case LDObject::Radial:
		coordCount = 3;
		
		lb_radType = new QLabel ("Type:");
		lb_radResolution = new QLabel ("Resolution:");
		lb_radSegments = new QLabel ("Segments:");
		lb_radRingNum = new QLabel ("Ring number:");
		
		rb_radType = new RadioBox ("Type", {}, 0, Qt::Vertical);
		
		for (int i = 0; i < LDRadial::NumTypes; ++i) {
			if (i % (LDRadial::NumTypes / 2) == 0)
				rb_radType->rowBreak ();
			
			rb_radType->addButton (LDRadial::radialTypeName ((LDRadial::Type) i));
		}
		
		connect (rb_radType, SIGNAL (buttonPressed (int)), this, SLOT (slot_radialTypeChanged (int)));
		
		cb_radHiRes = new QCheckBox ("Hi-Res");
		
		sb_radSegments = new QSpinBox;
		sb_radSegments->setMinimum (1);
		
		sb_radRingNum = new QSpinBox;
		sb_radRingNum->setEnabled (false);
		
		if (obj) {
			LDRadial* rad = static_cast<LDRadial*> (obj);
			
			rb_radType->setValue (rad->type ());
			sb_radSegments->setValue (rad->segments ());
			cb_radHiRes->setChecked ((rad->divisions () == hires) ? Qt::Checked : Qt::Unchecked);
			sb_radRingNum->setValue (rad->number ());
		}
		break;
	
	default:
		critical (fmt ("Unhandled LDObject type %1 (%2) in AddObjectDialog", (int) type, g_saObjTypeNames[type]));
		return;
	}
	
	QPixmap icon = getIcon (fmt ("add-%1", g_saObjTypeIcons[type]));
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
	
	case LDObject::Radial:
		layout->addWidget (rb_radType, 1, 1, 3, 2);
		layout->addWidget (cb_radHiRes, 1, 3);
		layout->addWidget (lb_radSegments, 2, 3);
		layout->addWidget (sb_radSegments, 2, 4);
		layout->addWidget (lb_radRingNum, 3, 3);
		layout->addWidget (sb_radRingNum, 3, 4);
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
	
	layout->addWidget (makeButtonBox (*this), 5, 0, 1, 4);
	setLayout (layout);
	setWindowTitle (fmt (APPNAME ": New %1",
		g_saObjTypeNames[type]));
	
	setWindowIcon (icon);
	delete defaults;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void AddObjectDialog::setButtonBackground (QPushButton* button, short color) {
	button->setIcon (getIcon ("palette"));
	button->setAutoFillBackground (true);
	button->setStyleSheet (fmt ("background-color: %1", getColor (color)->hexcode));
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
	ColorSelectDialog::staticDialog (colnum, colnum, this);
	setButtonBackground (pb_color, colnum);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void AddObjectDialog::slot_radialTypeChanged (int dType) {
	LDRadial::Type eType = (LDRadial::Type) dType;
	sb_radRingNum->setEnabled (eType == LDRadial::Ring || eType == LDRadial::Cone);
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
	
	if (obj && obj->getType () == LDObject::Gibberish)
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
	if (type == LDObject::Subfile || type == LDObject::Radial) {
		vector<str> matrixstrvals = container_cast<QStringList, vector<str>> (str (dlg.le_matrix->text ()).split (" "));
		
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
			LDComment* comm = initObj<LDComment> (obj);
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
			LDBFC* bfc = initObj<LDBFC> (obj);
			bfc->type = (LDBFC::Type) dlg.rb_bfcType->value ();
		}
		break;
	
	case LDObject::Vertex:
		{
			LDVertex* vert = initObj<LDVertex> (obj);
			
			for (const Axis ax : g_Axes)
				vert->pos[ax] = dlg.dsb_coords[ax]->value ();
		}
		break;
	
	case LDObject::Radial:
		{
			LDRadial* rad = initObj<LDRadial> (obj);
			
			for (const Axis ax : g_Axes)
				rad->setCoordinate (ax, dlg.dsb_coords[ax]->value ());
			
			rad->setDivisions (dlg.cb_radHiRes->isChecked () ? hires : lores);
			rad->setSegments (min<short> (dlg.sb_radSegments->value (), rad->divisions ()));
			rad->setType ((LDRadial::Type) dlg.rb_radType->value ());
			rad->setNumber (dlg.sb_radRingNum->value ());
			rad->setTransform (transform);
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
			
			LDSubfile* ref = initObj<LDSubfile> (obj);
			
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
		g_curfile->insertObj (idx, obj);
	}
	
	g_win->fullRefresh ();
}