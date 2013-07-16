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

#ifndef ZZ_ABOUTDIALOG_H
#define ZZ_ABOUTDIALOG_H

#include <QDialog>
#include "common.h"
#include "types.h"

class QPushButton;

class AboutDialog : public QDialog
{
	Q_OBJECT
	
public:
	AboutDialog( QWidget* parent = null, Qt::WindowFlags f = 0 );
	
private slots:
	void slot_mail();
};

#endif // ZZ_ABOUTDIALOG_H
