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
#include "radiobox.h"

#define APPLY_COORDS(OBJ, N) \
	for (short i = 0; i < N; ++i) \
		for (const Axis ax : g_Axes) \
			OBJ->coords[i][ax] = dlg.dsb_coords[(i * 3) + ax]->value ();

// =============================================================================
class SubfileListItem : public QTreeWidgetItem {
public:
	SubfileListItem (QTreeWidgetItem* parent, int subfileID) :
		QTreeWidgetItem (parent), subfileID (subfileID) {}
	SubfileListItem (QTreeWidget* parent, int subfileID) :
		QTreeWidgetItem (parent), subfileID (subfileID) {}
	
	int subfileID;
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
			rb_bfcType->addButton (new QRadioButton (LDBFC::statements[i]));
		
		if (obj)
			rb_bfcType->setValue ((int) static_cast<LDBFC*> (obj)->type);
		break;
	
	case LDObject::Subfile:
		coordCount = 3;
		
		enum {
			Parts,
			Subparts,
			Primitives,
			HiRes,
		};
		
		tw_subfileList = new QTreeWidget ();
		for (int i : vector<int> ({Parts, Subparts, Primitives, HiRes})) {
			SubfileListItem* parentItem = new SubfileListItem (tw_subfileList, -1);
			parentItem->setText (0, (i == Parts) ? "Parts" :
				(i == Subparts) ? "Subparts" :
				(i == Primitives) ? "Primitives" :
				"Hi-Res");
			
			ulong j = 0;
			for (partListEntry& part : g_PartList) {
				QList<QTreeWidgetItem*> subfileItems;
				
				str fileName = part.sName;
				const bool isSubpart = fileName.substr (0, 2) == "s\\";
				const bool isPrimitive = str (part.sTitle).substr (0, 9) == "Primitive";
				const bool isHiRes = fileName.substr (0, 3) == "48\\";
				
				if ((i == Subparts && isSubpart) ||
					(i == Primitives && isPrimitive) ||
					(i == HiRes && isHiRes) ||
					(i == Parts && !isSubpart && !isPrimitive && !isHiRes))
				{
					SubfileListItem* item = new SubfileListItem (parentItem, j);
					item->setText (0, fmt ("%s - %s", part.sName, part.sTitle));
					subfileItems.append (item);
				}
				
				j++;
			}
			
			tw_subfileList->addTopLevelItem (parentItem);
		}
		
		connect (tw_subfileList, SIGNAL (itemSelectionChanged ()), this, SLOT (slot_subfileTypeChanged ()));
		lb_subfileName = new QLabel ("File:");
		le_subfileName = new QLineEdit;
		le_subfileName->setFocus ();
		
		if (obj) {
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			le_subfileName->setText (ref->fileName);
		}
		break;
	
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
			
			rb_radType->addButton (new QRadioButton (LDRadial::radialTypeName ((LDRadial::Type) i)));
		}
		
		connect (rb_radType, SIGNAL (sig_buttonPressed (int)), this, SLOT (slot_radialTypeChanged (int)));
		
		cb_radHiRes = new QCheckBox ("Hi-Res");
		
		sb_radSegments = new QSpinBox;
		sb_radSegments->setMinimum (1);
		
		sb_radRingNum = new QSpinBox;
		sb_radRingNum->setEnabled (false);
		
		if (obj) {
			LDRadial* rad = static_cast<LDRadial*> (obj);
			
			rb_radType->setValue (rad->radType);
			sb_radSegments->setValue (rad->segs);
			cb_radHiRes->setChecked ((rad->divs == 48) ? Qt::Checked : Qt::Unchecked);
			sb_radRingNum->setValue (rad->ringNum);
		}
		break;
	
	default:
		assert (false);
		return;
	}
	
	QPixmap icon = getIcon (fmt ("add-%s", g_saObjTypeIcons[type]));
	LDObject* defaults = LDObject::getDefault (type);
	
	lb_typeIcon = new QLabel;
	lb_typeIcon->setPixmap (icon);
	
	// Show a color edit dialog for the types that actually use the color
	if (defaults->isColored ()) {
		if (obj != null)
			dColor = obj->color;
		else
			dColor = (type == LDObject::CondLine || type == LDObject::Line) ? edgecolor : maincolor;
		
		pb_color = new QPushButton;
		setButtonBackground (pb_color, dColor);
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
				dsb_coords[(i * 3) + j]->setValue (obj->coords[i].coord (j));
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
		
		if (obj)
			for (short i = 0; i < 3; ++i)
				dsb_coords[i]->setValue (static_cast<LDRadial*> (obj)->pos.coord (i));
		break;
	
	case LDObject::Subfile:
		layout->addWidget (tw_subfileList, 1, 1, 1, 2);
		layout->addWidget (lb_subfileName, 2, 1);
		layout->addWidget (le_subfileName, 2, 2);
		
		if (obj)
			for (short i = 0; i < 3; ++i)
				dsb_coords[i]->setValue (static_cast<LDSubfile*> (obj)->pos.coord (i));
		break;
	
	default:
		break;
	}
	
	if (type == LDObject::Subfile || type == LDObject::Radial) {
		QLabel* lb_matrix = new QLabel ("Matrix:");
		le_matrix = new QLineEdit;
		// le_matrix->setValidator (new QDoubleValidator);
		matrix defval = g_identity;
		
		if (obj) {
			if (obj->getType () == LDObject::Subfile)
				defval = static_cast<LDSubfile*> (obj)->transform;
			else
				defval = static_cast<LDRadial*> (obj)->transform;
		}
		
		le_matrix->setText (defval.stringRep ());
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
	setWindowTitle (fmt (APPNAME ": New %s",
		g_saObjTypeNames[type]).chars());
	
	setWindowIcon (icon);
	delete defaults;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void AddObjectDialog::setButtonBackground (QPushButton* button, short color) {
	button->setIcon (getIcon ("palette"));
	button->setAutoFillBackground (true);
	button->setStyleSheet (
		fmt ("background-color: %s", getColor (color)->hexcode.chars()).chars()
	);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
char* AddObjectDialog::currentSubfileName() {
	SubfileListItem* item = static_cast<SubfileListItem*> (tw_subfileList->currentItem ());
	
	if (item->subfileID == -1)
		return null; // selected a heading
	
	return g_PartList[item->subfileID].sName;
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void AddObjectDialog::slot_colorButtonClicked () {
	ColorSelectDialog::staticDialog (dColor, dColor, this);
	setButtonBackground (pb_color, dColor);
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
	char* name = currentSubfileName ();
	
	if (name)
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
	
	LDObject* backup = null;
	if (!newObject)
		backup = obj->clone ();
	
	matrix transform = g_identity;
	if (type == LDObject::Subfile || type == LDObject::Radial) {
		vector<str> matrixstrvals = str (dlg.le_matrix->text ()).split (" ");
		
		if (matrixstrvals.size () == 9) {
			double matrixvals[9];
			int i = 0;
			
			for (str val : matrixstrvals)
				matrixvals[i++] = atof (val);
			
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
		{
			LDLine* line = initObj<LDLine> (obj);
			line->color = dlg.dColor;
			APPLY_COORDS (line, 2)
		}
		break;
	
	case LDObject::Triangle:
		{
			LDTriangle* tri = initObj<LDTriangle> (obj);
			tri->color = dlg.dColor;
			APPLY_COORDS (tri, 3)
		}
		break;
	
	case LDObject::Quad:
		{
			LDQuad* quad = initObj<LDQuad> (obj);
			quad->color = dlg.dColor;
			APPLY_COORDS (quad, 4)
		}
		break;
	
	case LDObject::CondLine:
		{
			LDCondLine* line = initObj<LDCondLine> (obj);
			line->color = dlg.dColor;
			APPLY_COORDS (line, 4)
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
			vert->color = dlg.dColor;
			
			for (const Axis ax : g_Axes)
				vert->pos[ax] = dlg.dsb_coords[ax]->value ();
		}
		break;
	
	case LDObject::Radial:
		{
			LDRadial* pRad = initObj<LDRadial> (obj);
			pRad->color = dlg.dColor;
			
			for (const Axis ax : g_Axes)
				pRad->pos[ax] = dlg.dsb_coords[ax]->value ();
			
			pRad->divs = (dlg.cb_radHiRes->checkState () != Qt::Checked) ? 16 : 48;
			pRad->segs = min<short> (dlg.sb_radSegments->value (), pRad->divs);
			pRad->radType = (LDRadial::Type) dlg.rb_radType->value ();
			pRad->ringNum = dlg.sb_radRingNum->value ();
			pRad->transform = transform;
		}
		break;
	
	case LDObject::Subfile:
		{
			str name = dlg.le_subfileName->text ();
			if (~name == 0)
				return; // no subfile filename
			
			OpenFile* file = loadSubfile (name);
			if (!file)
				return;
			
			LDSubfile* ref = initObj<LDSubfile> (obj);
			ref->color = dlg.dColor;
			
			for (const Axis ax : g_Axes)
				ref->pos[ax] = dlg.dsb_coords[ax]->value ();
			
			ref->fileName = name;
			ref->transform = transform;
			ref->fileInfo = file;
		}
		break;
	
	default:
		break;
	}
	
	if (newObject) {
		ulong idx = g_win->getInsertionPoint ();
		g_curfile->insertObj (idx, obj);
		History::addEntry (new AddHistory ({(ulong) idx}, {obj->clone ()}));
	} else {
		History::addEntry (new EditHistory ({(ulong) obj->getIndex (g_curfile)}, {backup}, {obj->clone ()}));
	}
	
	g_win->fullRefresh ();
}