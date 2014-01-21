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

#ifndef LDFORGE_COLORSELECTOR_H
#define LDFORGE_COLORSELECTOR_H

#include <QDialog>
#include "Main.h"

class LDColor;
class Ui_ColorSelUI;
class QGraphicsScene;

class ColorSelector : public QDialog
{
	Q_OBJECT
	PROPERTY (private,	LDColor*,	Selection,	NO_OPS,	STOCK_WRITE)

	public:
		explicit ColorSelector (int defval = -1, QWidget* parent = null);
		virtual ~ColorSelector();
		static bool selectColor (int& val, int defval = -1, QWidget* parent = null);

	protected:
		void mousePressEvent (QMouseEvent* event);
		void resizeEvent (QResizeEvent* ev);

	private:
		Ui_ColorSelUI* ui;
		QGraphicsScene* m_scene;
		bool m_firstResize;

		int numRows() const;
		int viewportWidth() const;
		void drawScene();
		void drawColorInfo();
};

#endif // LDFORGE_COLORSELECTOR_H
