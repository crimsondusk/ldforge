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

#include <qboxlayout.h>
#include <qmessagebox.h>
#include "historyDialog.h"
#include "history.h"
#include "colors.h"
#include "file.h"

EXTERN_ACTION (undo);
EXTERN_ACTION (redo);

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
HistoryDialog::HistoryDialog (QWidget* parent, Qt::WindowFlags f) : QDialog (parent, f) {
	historyList = new QListWidget;
	undoButton = new QPushButton ("Undo");
	redoButton = new QPushButton ("Redo");
	clearButton = new QPushButton ("Clear");
	buttons = new QDialogButtonBox (QDialogButtonBox::Close);
	
	historyList->setAlternatingRowColors (true);
	
	undoButton->setIcon (getIcon ("undo"));
	redoButton->setIcon (getIcon ("redo"));
	
	connect (undoButton, SIGNAL (clicked ()), this, SLOT (slot_undo ()));
	connect (redoButton, SIGNAL (clicked ()), this, SLOT (slot_redo ()));
	connect (clearButton, SIGNAL (clicked ()), this, SLOT (slot_clear ()));
	connect (buttons, SIGNAL (rejected ()), this, SLOT (reject ()));
	connect (historyList, SIGNAL (itemSelectionChanged ()), this, SLOT (slot_selChanged ()));
	
	QVBoxLayout* qButtonLayout = new QVBoxLayout;
	qButtonLayout->setDirection (QBoxLayout::TopToBottom);
	qButtonLayout->addWidget (undoButton);
	qButtonLayout->addWidget (redoButton);
	qButtonLayout->addWidget (clearButton);
	qButtonLayout->addStretch ();
	
	QGridLayout* qLayout = new QGridLayout;
	qLayout->setColumnStretch (0, 1);
	qLayout->addWidget (historyList, 0, 0, 2, 1);
	qLayout->addLayout (qButtonLayout, 0, 1);
	qLayout->addWidget (buttons, 1, 1);
	
	setLayout (qLayout);
	setWindowIcon (getIcon ("history"));
	setWindowTitle ("Edit History");
	
	populateList ();
	updateButtons ();
	updateSelection ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void HistoryDialog::populateList () {
	historyList->clear ();
	
	QListWidgetItem* qItem = new QListWidgetItem;
	qItem->setText ("[[ initial state ]]");
	qItem->setIcon (getIcon ("empty"));
	historyList->addItem (qItem);
	
#if 0
	for (AbstractHistoryEntry* entry : g_curfile->history ().entries ()) {
		str text;
		QIcon entryIcon;
		
		switch (entry->type ()) {
		default:
			text = "???";
			break;
		}
		
		QListWidgetItem* item = new QListWidgetItem;
		item->setText (text);
		item->setIcon (entryIcon);
		historyList->addItem (item);
	}
#endif
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void HistoryDialog::slot_undo () {
	g_curfile->undo ();
	updateButtons ();
	updateSelection ();
}

// =============================================================================
void HistoryDialog::slot_redo () {
	g_curfile->redo ();
	updateButtons ();
	updateSelection ();
}

void HistoryDialog::updateSelection () {
	historyList->setCurrentItem (historyList->item (g_curfile->history ().pos () + 1));
}

// =============================================================================
void HistoryDialog::slot_clear () {
	if (!confirm ("Are you sure you want to clear the edit history?\nThis cannot be undone."))
		return; // Canceled
	
	g_curfile->clearHistory ();
	populateList ();
	updateButtons ();
}

// =============================================================================
void HistoryDialog::updateButtons () {
	undoButton->setEnabled (ACTION (undo)->isEnabled ());
	redoButton->setEnabled (ACTION (redo)->isEnabled ());
}

// =============================================================================
void HistoryDialog::slot_selChanged () {
	if (historyList->selectedItems ().size () != 1)
		return;
	
	QListWidgetItem* item = historyList->selectedItems ()[0];
	
	// Find the index of the edit
	long index;
	for (index = 0; index < historyList->count (); ++index)
		if (historyList->item (index) == item)
			break;
	
	// qHistoryList is 0-based, History is -1-based, thus decrement
	// the index now
	index--;
	
	// Seek to the selected edit by repeadetly undoing or redoing.
	while (g_curfile->history ().pos () != index) {
		if (g_curfile->history ().pos () > index)
			g_curfile->undo ();
		else
			g_curfile->redo ();
	}
	
	updateButtons ();
}