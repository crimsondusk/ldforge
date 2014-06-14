/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
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

#include <QGridLayout>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QLabel>
#include <QListWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include "mainWindow.h"
#include "addObjectDialog.h"
#include "ldDocument.h"
#include "colors.h"
#include "colorSelector.h"
#include "editHistory.h"
#include "radioGroup.h"
#include "miscallenous.h"
#include "primitives.h"

// =============================================================================
//
AddObjectDialog::AddObjectDialog (const LDObjectType type, LDObjectPtr obj, QWidget* parent) :
	QDialog (parent)
{
	setlocale (LC_ALL, "C");

	int coordCount = 0;
	QString typeName = LDObject::typeName (type);

	switch (type)
	{
		case OBJ_Comment:
		{
			le_comment = new QLineEdit;

			if (obj)
				le_comment->setText (obj.staticCast<LDComment>()->text());

			le_comment->setMinimumWidth (384);
		} break;

		case OBJ_Line:
		{
			coordCount = 6;
		} break;

		case OBJ_Triangle:
		{
			coordCount = 9;
		} break;

		case OBJ_Quad:
		case OBJ_CondLine:
		{
			coordCount = 12;
		} break;

		case OBJ_Vertex:
		{
			coordCount = 3;
		} break;

		case OBJ_BFC:
		{
			rb_bfcType = new RadioGroup ("Statement", {}, 0, Qt::Vertical);

			for (int i = 0; i < LDBFC::NumStatements; ++i)
			{
				// Separate these in two columns
				if (i == LDBFC::NumStatements / 2)
					rb_bfcType->rowBreak();

				rb_bfcType->addButton (LDBFC::k_statementStrings[i]);
			}

			if (obj)
				rb_bfcType->setValue ((int) obj.staticCast<LDBFC>()->statement());
		} break;

		case OBJ_Subfile:
		{
			coordCount = 3;
			tw_subfileList = new QTreeWidget();
			tw_subfileList->setHeaderLabel (tr ("Primitives"));
			populatePrimitives (tw_subfileList, (obj != null ? obj.staticCast<LDSubfile>()->fileInfo()->name() : ""));

			connect (tw_subfileList, SIGNAL (itemSelectionChanged()), this, SLOT (slot_subfileTypeChanged()));
			lb_subfileName = new QLabel ("File:");
			le_subfileName = new QLineEdit;
			le_subfileName->setFocus();

			if (obj)
			{
				LDSubfilePtr ref = obj.staticCast<LDSubfile>();
				le_subfileName->setText (ref->fileInfo()->name());
			}
		} break;

		default:
		{
			critical (format ("Unhandled LDObject type %1 (%2) in AddObjectDialog", (int) type, typeName));
		} return;
	}

	QPixmap icon = getIcon (format ("add-%1", typeName));
	LDObjectPtr defaults = LDObject::getDefault (type);

	lb_typeIcon = new QLabel;
	lb_typeIcon->setPixmap (icon);

	// Show a color edit dialog for the types that actually use the color
	if (defaults->isColored())
	{
		if (obj != null)
			_color = obj->color();
		else
			_color = (type == OBJ_CondLine || type == OBJ_Line) ? edgecolor() : maincolor();

		pb_color = new QPushButton;
		setButtonBackground (pb_color, _color);
		connect (pb_color, SIGNAL (clicked()), this, SLOT (slot_colorButtonClicked()));
	}

	for (int i = 0; i < coordCount; ++i)
	{
		dsb_coords[i] = new QDoubleSpinBox;
		dsb_coords[i]->setDecimals (5);
		dsb_coords[i]->setMinimum (-10000.0);
		dsb_coords[i]->setMaximum (10000.0);
	}

	QGridLayout* const layout = new QGridLayout;
	layout->addWidget (lb_typeIcon, 0, 0);

	switch (type)
	{
		case OBJ_Line:
		case OBJ_CondLine:
		case OBJ_Triangle:
		case OBJ_Quad:
			// Apply coordinates
			if (obj != null)
			{
				for (int i = 0; i < coordCount / 3; ++i)
				{
					obj->vertex (i).apply ([&](Axis ax, double value)
					{
						dsb_coords[(i * 3) + ax]->setValue (value);
					});
				}
			}
			break;

		case OBJ_Comment:
			layout->addWidget (le_comment, 0, 1);
			break;

		case OBJ_BFC:
			layout->addWidget (rb_bfcType, 0, 1);
			break;

		case OBJ_Subfile:
			layout->addWidget (tw_subfileList, 1, 1, 1, 2);
			layout->addWidget (lb_subfileName, 2, 1);
			layout->addWidget (le_subfileName, 2, 2);
			break;

		default:
			break;
	}

	if (defaults->hasMatrix())
	{
		LDMatrixObjectPtr mo = obj.dynamicCast<LDMatrixObject>();
		QLabel* lb_matrix = new QLabel ("Matrix:");
		le_matrix = new QLineEdit;
		// le_matrix->setValidator (new QDoubleValidator);
		Matrix defaultMatrix = g_identity;

		if (mo != null)
		{
			mo->position().apply ([&](Axis ax, double value)
			{
				dsb_coords[ax]->setValue (value);
			});

			defaultMatrix = mo->transform();
		}

		le_matrix->setText (defaultMatrix.toString());
		layout->addWidget (lb_matrix, 4, 1);
		layout->addWidget (le_matrix, 4, 2, 1, 3);
	}

	if (defaults->isColored())
		layout->addWidget (pb_color, 1, 0);

	if (coordCount > 0)
	{
		QGridLayout* const qCoordLayout = new QGridLayout;

		for (int i = 0; i < coordCount; ++i)
			qCoordLayout->addWidget (dsb_coords[i], (i / 3), (i % 3));

		layout->addLayout (qCoordLayout, 0, 1, (coordCount / 3), 3);
	}

	QDialogButtonBox* bbx_buttons = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	QWidget::connect (bbx_buttons, SIGNAL (accepted()), this, SLOT (accept()));
	QWidget::connect (bbx_buttons, SIGNAL (rejected()), this, SLOT (reject()));
	layout->addWidget (bbx_buttons, 5, 0, 1, 4);
	setLayout (layout);
	setWindowTitle (format (tr ("Edit %1"), typeName));
	setWindowIcon (icon);
}

// =============================================================================
// =============================================================================
void AddObjectDialog::setButtonBackground (QPushButton* button, LDColor color)
{
	button->setIcon (getIcon ("palette"));
	button->setAutoFillBackground (true);

	if (color != null)
		button->setStyleSheet (format ("background-color: %1", color->hexcode()));
}

// =============================================================================
// =============================================================================
QString AddObjectDialog::currentSubfileName()
{
	SubfileListItem* item = static_cast<SubfileListItem*> (tw_subfileList->currentItem());

	if (item->primitive() == null)
		return ""; // selected a heading

	return item->primitive()->name;
}

// =============================================================================
// =============================================================================
void AddObjectDialog::slot_colorButtonClicked()
{
	ColorSelector::selectColor (_color, _color, this);
	setButtonBackground (pb_color, _color);
}

// =============================================================================
// =============================================================================
void AddObjectDialog::slot_subfileTypeChanged()
{
	QString name = currentSubfileName();

	if (name.length() > 0)
		le_subfileName->setText (name);
}

// =============================================================================
// =============================================================================
template<typename T>
static QSharedPointer<T> initObj (LDObjectPtr& obj)
{
	if (obj == null)
		obj = spawn<T>();

	return obj.staticCast<T>();
}

// =============================================================================
// =============================================================================
void AddObjectDialog::staticDialog (const LDObjectType type, LDObjectPtr obj)
{
	setlocale (LC_ALL, "C");

	// FIXME: Redirect to Edit Raw
	if (obj && obj->type() == OBJ_Error)
		return;

	if (type == OBJ_Empty)
		return; // Nothing to edit with empties

	const bool newObject = (obj == null);
	Matrix transform = g_identity;
	AddObjectDialog dlg (type, obj);

	assert (obj == null || obj->type() == type);

	if (dlg.exec() == QDialog::Rejected)
		return;

	if (type == OBJ_Subfile)
	{
		QStringList matrixstrvals = dlg.le_matrix->text().split (" ", QString::SkipEmptyParts);

		if (matrixstrvals.size() == 9)
		{
			double matrixvals[9];
			int i = 0;

			for (QString val : matrixstrvals)
				matrixvals[i++] = val.toFloat();

			transform = Matrix (matrixvals);
		}
	}

	switch (type)
	{
		case OBJ_Comment:
		{
			LDCommentPtr comm = initObj<LDComment> (obj);
			comm->setText (dlg.le_comment->text());
		}
		break;

		case OBJ_Line:
		case OBJ_Triangle:
		case OBJ_Quad:
		case OBJ_CondLine:
		{
			if (not obj)
				obj = LDObject::getDefault (type);

			for (int i = 0; i < obj->numVertices(); ++i)
			{
				Vertex v;

				v.apply ([&](Axis ax, double& value)
				{
					value = dlg.dsb_coords[(i * 3) + ax]->value();
				});

				obj->setVertex (i, v);
			}
		} break;

		case OBJ_BFC:
		{
			LDBFCPtr bfc = initObj<LDBFC> (obj);
			bfc->setStatement ((LDBFC::Statement) dlg.rb_bfcType->value());
		} break;

		case OBJ_Vertex:
		{
			LDVertexPtr vert = initObj<LDVertex> (obj);
			vert->pos.apply ([&](Axis ax, double& value) { value = dlg.dsb_coords[ax]->value(); });
		}
		break;

		case OBJ_Subfile:
		{
			QString name = dlg.le_subfileName->text();

			if (name.length() == 0)
				return; // no subfile filename

			LDDocumentPtr file = getDocument (name);

			if (not file)
			{
				critical (format ("Couldn't open `%1': %2", name, strerror (errno)));
				return;
			}

			LDSubfilePtr ref = initObj<LDSubfile> (obj);
			assert (ref);

			for_axes (ax)
				ref->setCoordinate (ax, dlg.dsb_coords[ax]->value());

			ref->setTransform (transform);
			ref->setFileInfo (file);
		} break;

		default:
			break;
	}

	if (obj->isColored())
		obj->setColor (dlg._color);

	if (newObject)
	{
		int idx = g_win->getInsertionPoint();
		getCurrentDocument()->insertObj (idx, obj);
	}

	g_win->refresh();
}
