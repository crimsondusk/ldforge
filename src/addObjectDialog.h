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

#ifndef LDFORGE_ADDOBJECTDIALOG_H
#define LDFORGE_ADDOBJECTDIALOG_H

#include <QDialog>
#include "ldtypes.h"

class QTreeWidgetItem;
class QLineEdit;
class RadioGroup;
class QCheckBox;
class QSpinBox;
class QLabel;
class QTreeWidget;
class QDoubleSpinBox;

class AddObjectDialog : public QDialog
{	Q_OBJECT

	public:
		AddObjectDialog (const LDObject::Type type, LDObject* obj, QWidget* parent = null);
		static void staticDialog (const LDObject::Type type, LDObject* obj);

		QLabel* lb_typeIcon;

		// Comment line edit
		QLineEdit* le_comment;

		// Coordinate edits for.. anything with coordinates, really.
		QDoubleSpinBox* dsb_coords[12];

		// Color selection dialog button
		QPushButton* pb_color;

		// BFC-related widgets
		RadioGroup* rb_bfcType;

		// Subfile stuff
		QTreeWidget* tw_subfileList;
		QLineEdit* le_subfileName;
		QLabel* lb_subfileName;
		QLineEdit* le_matrix;

	private:
		void setButtonBackground (QPushButton* button, short color);
		str currentSubfileName();

		short colnum;

	private slots:
		void slot_colorButtonClicked();
		void slot_subfileTypeChanged();
};

#endif // ZZ_ADDOBJECTDIALOG_H
