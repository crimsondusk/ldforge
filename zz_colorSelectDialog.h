/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 Santeri `arezey` Piippo
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

#include <qdialog.h>
#include <qdialogbuttonbox.h>
#include <qgraphicsscene.h>
#include <qlabel.h>

class ColorSelectDialog : public QDialog {
	Q_OBJECT
	
public:
	explicit ColorSelectDialog (short dDefault = -1, QWidget* parent = nullptr);
	static bool staticDialog (short& dValue, short dDefault = -1, QWidget* parent = nullptr);
	
	QGraphicsScene* qScene;
	QGraphicsView* qView;
	QLabel* qColorInfo;
	QDialogButtonBox* qButtons;
	short dSelColor;
	
private:
	void drawScene ();
	void drawColorInfo ();
	
private slots:
	void mousePressEvent (QMouseEvent* event);
};