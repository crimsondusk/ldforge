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

#ifndef LDRAWPATHDIALOG_H
#define LDRAWPATHDIALOG_H

#include <qdialog.h>
#include "common.h"

class QLabel;
class QLineEdit;
class QDialogButtonBox;

class LDrawPathDialog : public QDialog {
	Q_OBJECT
	
public:
	explicit LDrawPathDialog (const bool validDefault, QWidget* parent = null, Qt::WindowFlags f = 0);
	str path () const;
	void setPath (str path);
	void (*callback ()) () const { return m_callback; }
	void setCallback (void (*callback) ()) { m_callback = callback; }
	
private:
	Q_DISABLE_COPY (LDrawPathDialog)
	
	QLabel* lb_resolution;
	QLineEdit* le_path;
	QPushButton* btn_findPath, *btn_tryConfigure, *btn_cancel;
	QDialogButtonBox* dbb_buttons;
	void (*m_callback) ();
	const bool m_validDefault;
	
	QPushButton* okButton ();
	
private slots:
	void slot_findPath ();
	void slot_tryConfigure ();
	void slot_exit ();
};

#endif // LDRAWPATHDIALOG_H
