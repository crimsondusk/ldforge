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
#include <qradiobutton.h>
#include <qcheckbox.h>
#include "gui.h"
#include "zz_addObjectDialog.h"
#include "file.h"
#include "colors.h"
#include "zz_colorSelectDialog.h"
#include "history.h"

#define APPLY_COORDS(OBJ, N) \
	for (short i = 0; i < N; ++i) \
		for (const Axis ax : g_Axes) \
			OBJ->vaCoords[i][ax] = dlg.dsb_coords[(i * 3) + ax]->value ();

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
AddObjectDialog::AddObjectDialog (const LDObjectType_e type, LDObject* obj, QWidget* parent) :
	QDialog (parent)
{
	short coordCount = 0;
	QPixmap icon = getIcon (fmt ("add-%s", g_saObjTypeIcons[type]));
	LDObject* defaults = LDObject::getDefault (type);
	
	lb_typeIcon = new QLabel;
	lb_typeIcon->setPixmap (icon);
	
	switch (type) {
	case OBJ_Comment:
		le_comment = new QLineEdit;
		if (obj)
			le_comment->setText (static_cast<LDComment*> (obj)->zText);
		break;
	
	case OBJ_Line:
		coordCount = 6;
		break;
	
	case OBJ_Triangle:
		coordCount = 9;
		break;
	
	case OBJ_Quad:
	case OBJ_CondLine:
		coordCount = 12;
		break;
	
	case OBJ_Vertex:
		coordCount = 3;
		break;
	
	case OBJ_BFC:
		rb_bfcType = new RadioBox ("Statement", {}, 0, Qt::Horizontal);
		
		for (int i = 0; i < LDBFC::NumStatements; ++i) {
			if (i % (LDBFC::NumStatements / 2) == 0)
				rb_bfcType->rowBreak ();
			
			rb_bfcType->addButton (new QRadioButton (LDBFC::saStatements[i]));
		}
		
		if (obj)
			rb_bfcType->setValue ((int) static_cast<LDBFC*> (obj)->eStatement);
		break;
	
	case OBJ_Subfile:
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
		le_subfileName = new QLineEdit ();
		
		if (obj) {
			LDSubfile* ref = static_cast<LDSubfile*> (obj);
			le_subfileName->setText (ref->zFileName);
		}
		break;
	
	case OBJ_Radial:
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
			
			rb_radType->setValue (rad->eRadialType);
			sb_radSegments->setValue (rad->dSegments);
			cb_radHiRes->setChecked ((rad->dDivisions == 48) ? Qt::Checked : Qt::Unchecked);
			sb_radRingNum->setValue (rad->dRingNum);
		}
		break;
	
	default:
		break;
	}
	
	// Show a color edit dialog for the types that actually use the color
	if (defaults->isColored ()) {
		if (obj != null)
			dColor = obj->dColor;
		else
			dColor = (type == OBJ_CondLine || type == OBJ_Line) ? edgecolor : maincolor;
		
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
	
	IMPLEMENT_DIALOG_BUTTONS
	
	QGridLayout* const layout = new QGridLayout;
	layout->addWidget (lb_typeIcon, 0, 0);
	
	switch (type) {
	case OBJ_Line:
	case OBJ_CondLine:
	case OBJ_Triangle:
	case OBJ_Quad:
		// Apply coordinates
		if (obj) {
			for (short i = 0; i < coordCount / 3; ++i)
			for (short j = 0; j < 3; ++j)
				dsb_coords[(i * 3) + j]->setValue (obj->vaCoords[i].coord (j));
		}
		break;
	
	case OBJ_Comment:
		layout->addWidget (le_comment, 0, 1);
		break;
	
	case OBJ_BFC:
		layout->addWidget (rb_bfcType, 0, 1);
		break;
	
	case OBJ_Radial:
		layout->addWidget (rb_radType, 1, 1, 3, 1);
		layout->addWidget (cb_radHiRes, 1, 2);
		layout->addWidget (lb_radSegments, 2, 2);
		layout->addWidget (sb_radSegments, 2, 3);
		layout->addWidget (lb_radRingNum, 3, 2);
		layout->addWidget (sb_radRingNum, 3, 3);
		
		if (obj)
			for (short i = 0; i < 3; ++i)
				dsb_coords[i]->setValue (static_cast<LDRadial*> (obj)->vPosition.coord (i));
		break;
	
	case OBJ_Subfile:
		layout->addWidget (tw_subfileList, 1, 1);
		layout->addWidget (le_subfileName, 2, 1);
		
		if (obj)
			for (short i = 0; i < 3; ++i)
				dsb_coords[i]->setValue (static_cast<LDSubfile*> (obj)->vPosition.coord (i));
		break;
	
	default:
		break;
	}
	
	if (defaults->isColored ())
		layout->addWidget (pb_color, 1, 0);
	
	if (coordCount > 0) {
		QGridLayout* const qCoordLayout = new QGridLayout;
		
		for (short i = 0; i < coordCount; ++i)
			qCoordLayout->addWidget (dsb_coords[i], (i / 3), (i % 3));
		
		layout->addLayout (qCoordLayout, 0, 1, (coordCount / 3), 3);
	}
	
	layout->addWidget (bbx_buttons, 5, 0, 1, 4);
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
		fmt ("background-color: %s", getColor (color)->zColorString.chars()).chars()
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
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
template<class T> T* initObj (LDObject*& obj) {
	if (obj == null)
		obj = new T;
	
	return static_cast<T*> (obj);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void AddObjectDialog::staticDialog (const LDObjectType_e type, LDObject* obj) {
	const bool newObject = (obj == null);
	AddObjectDialog dlg (type, obj);
	
	if (obj)
		assert (obj->getType () == type);
	
	if (dlg.exec () == false)
		return;
	
	LDObject* backup = null;
	if (!newObject)
		backup = obj->clone ();
	
	switch (type) {
	case OBJ_Comment:
		{
			LDComment* comm = initObj<LDComment> (obj);
			comm->zText = dlg.le_comment->text ();
		}
		break;
	
	case OBJ_Line:
		{
			LDLine* line = initObj<LDLine> (obj);
			line->dColor = dlg.dColor;
			APPLY_COORDS (line, 2)
		}
		break;
	
	case OBJ_Triangle:
		{
			LDTriangle* tri = initObj<LDTriangle> (obj);
			tri->dColor = dlg.dColor;
			APPLY_COORDS (tri, 3)
		}
		break;
	
	case OBJ_Quad:
		{
			LDQuad* quad = initObj<LDQuad> (obj);
			quad->dColor = dlg.dColor;
			APPLY_COORDS (quad, 4)
		}
		break;
	
	case OBJ_CondLine:
		{
			LDCondLine* line = initObj<LDCondLine> (obj);
			line->dColor = dlg.dColor;
			APPLY_COORDS (line, 4)
		}
		break;
	
	case OBJ_BFC:
		{
			LDBFC* bfc = initObj<LDBFC> (obj);
			bfc->eStatement = (LDBFC::Type) dlg.rb_bfcType->value ();
		}
		break;
	
	case OBJ_Vertex:
		{
			LDVertex* vert = initObj<LDVertex> (obj);
			vert->dColor = dlg.dColor;
			
			for (const Axis ax : g_Axes)
				vert->vPosition[ax] = dlg.dsb_coords[ax]->value ();
		}
		break;
	
	case OBJ_Radial:
		{
			LDRadial* pRad = initObj<LDRadial> (obj);
			pRad->dColor = dlg.dColor;
			
			for (const Axis ax : g_Axes)
				pRad->vPosition[ax] = dlg.dsb_coords[ax]->value ();
			
			pRad->dDivisions = (dlg.cb_radHiRes->checkState () != Qt::Checked) ? 16 : 48;
			pRad->dSegments = min<short> (dlg.sb_radSegments->value (), pRad->dDivisions);
			pRad->eRadialType = (LDRadial::Type) dlg.rb_radType->value ();
			pRad->dRingNum = dlg.sb_radRingNum->value ();
			pRad->mMatrix = g_identity;
		}
		break;
	
	case OBJ_Subfile:
		{
			str name = dlg.le_subfileName->text ();
			if (~name == 0)
				return; // no subfile filename
			
			LDSubfile* ref = initObj<LDSubfile> (obj);
			ref->dColor = dlg.dColor;
			
			for (const Axis ax : g_Axes)
				ref->vPosition[ax] = dlg.dsb_coords[ax]->value ();
			
			ref->zFileName = name;
			ref->mMatrix = g_identity;
			ref->pFile = loadSubfile (name);
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
	
	g_win->refresh ();
}