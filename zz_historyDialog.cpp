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

#include "zz_historyDialog.h"
#include "history.h"
#include <qboxlayout.h>
#include <qmessagebox.h>

EXTERN_ACTION (undo);
EXTERN_ACTION (redo);

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
HistoryDialog::HistoryDialog (QWidget* parent, Qt::WindowFlags f) : QDialog (parent, f) {
	qHistoryList = new QListWidget;
	qUndoButton = new QPushButton ("Undo");
	qRedoButton = new QPushButton ("Redo");
	qClearButton = new QPushButton ("Clear");
	qButtons = new QDialogButtonBox (QDialogButtonBox::Close);
	
	qUndoButton->setIcon (getIcon ("undo"));
	qRedoButton->setIcon (getIcon ("redo"));
	
	connect (qUndoButton, SIGNAL (clicked ()), this, SLOT (slot_undo ()));
	connect (qRedoButton, SIGNAL (clicked ()), this, SLOT (slot_redo ()));
	connect (qClearButton, SIGNAL (clicked ()), this, SLOT (slot_clear ()));
	connect (qButtons, SIGNAL (rejected ()), this, SLOT (reject ()));
	
	QVBoxLayout* qButtonLayout = new QVBoxLayout;
	qButtonLayout->setDirection (QBoxLayout::TopToBottom);
	qButtonLayout->addWidget (qUndoButton);
	qButtonLayout->addWidget (qRedoButton);
	qButtonLayout->addWidget (qClearButton);
	qButtonLayout->addStretch ();
	
	QGridLayout* qLayout = new QGridLayout;
	qLayout->addWidget (qHistoryList, 0, 0, 2, 1);
	qLayout->addLayout (qButtonLayout, 0, 1);
	qLayout->addWidget (qButtons, 1, 1);
	
	setLayout (qLayout);
	setWindowIcon (getIcon ("history"));
	setWindowTitle (APPNAME_DISPLAY " - Edit history");
	
	populateList ();
	updateButtons ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void HistoryDialog::populateList () {
	qHistoryList->clear ();
	
	for (HistoryEntry* entry : History::entries) {
		str zText;
		
		switch (entry->type ()) {
		case HISTORY_Addition:
			{
				AdditionHistory* addentry = static_cast<AdditionHistory*> (entry);
				zText.format ("Added %s", LDObject::objectListContents (addentry->paObjs).chars());
			}
			break;
		
		case HISTORY_QuadSplit:
			{
				QuadSplitHistory* splitentry = static_cast<QuadSplitHistory*> (entry);
				ulong ulCount = splitentry->paQuads.size ();
				zText.format ("Split %lu quad%s to triangles", ulCount, PLURAL (ulCount));
				break;
			}
		
		default:
			zText = "???";
			break;
		}
		
		QListWidgetItem* qItem = new QListWidgetItem;
		qItem->setText (zText);
		qHistoryList->addItem (qItem);
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void HistoryDialog::slot_undo () {
	History::undo ();
	updateButtons ();
}

// =============================================================================
void HistoryDialog::slot_redo () {
	History::redo ();
	updateButtons ();
}

// =============================================================================
void HistoryDialog::slot_clear () {
	if (QMessageBox::question (this, "Confirm", "Are you sure you want to "
		"clear the edit history?\nThis cannot be undone.",
		(QMessageBox::Yes | QMessageBox::No), QMessageBox::No) != QMessageBox::Yes)
	{
		// Canceled
		return;
	}
	
	History::clear ();
	populateList ();
}

// =============================================================================
void HistoryDialog::updateButtons () {
	qUndoButton->setEnabled (ACTION_NAME (undo)->isEnabled ());
	qRedoButton->setEnabled (ACTION_NAME (redo)->isEnabled ());
}