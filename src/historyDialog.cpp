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
	
	for (HistoryEntry* entry : History::entries ()) {
		str text;
		QIcon entryIcon;
		
		switch (entry->type ()) {
		case HISTORY_Add:
			{
				AddHistory* subentry = static_cast<AddHistory*> (entry);
				ulong count = subentry->paObjs.size ();
				str verb = "Added";
				
				switch (subentry->eType) {
				case AddHistory::Paste:
					verb = "Pasted";
					entryIcon = getIcon ("paste");
					break;
				
				default:
					{
						// Determine a common type for these objects. If all objects are of the same
						// type, we display its addition icon. Otherwise, we draw a subfile addition
						// one as a default.
						LDObject::Type eCommonType = LDObject::Unidentified;
						for (LDObject* obj : subentry->paObjs) {
							if (eCommonType == LDObject::Unidentified or obj->getType() == eCommonType)
								eCommonType = obj->getType ();
							else {
								eCommonType = LDObject::Unidentified;
								break;
							}
						}
						
						// Set the icon based on the common type decided above.
						if (eCommonType == LDObject::Unidentified)
							entryIcon = getIcon ("add-subfile");
						else
							entryIcon = getIcon (fmt ("add-%s", g_saObjTypeIcons[eCommonType]));
					}
					break;
				}
				
				text.format ("%s %lu objects\n%s", verb.chars(), count,
					LDObject::objectListContents (subentry->paObjs).chars());
			}
			break;
		
		case HISTORY_QuadSplit:
			{
				QuadSplitHistory* subentry = static_cast<QuadSplitHistory*> (entry);
				ulong ulCount = subentry->paQuads.size ();
				text.format ("Split %lu quad%s to triangles", ulCount, PLURAL (ulCount));
				
				entryIcon = getIcon ("quad-split");
			}
			break;
		
		case HISTORY_Del:
			{
				DelHistory* subentry = static_cast<DelHistory*> (entry);
				ulong count = subentry->cache.size ();
				str verb = "Deleted";
				entryIcon = getIcon ("delete");
				
				switch (subentry->eType) {
				case DelHistory::Cut:
					entryIcon = getIcon ("cut");
					verb = "Cut";
					break;
				
				default:
					break;
				}
				
				text.format ("%s %lu objects:\n%s", verb.chars(), count,
					LDObject::objectListContents (subentry->cache).chars ());
			}
			break;
		
		case HISTORY_SetColor:
			{
				SetColorHistory* subentry = static_cast<SetColorHistory*> (entry);
				ulong count = subentry->ulaIndices.size ();
				text.format ("Set color of %lu objects to %d (%s)", count,
					subentry->dNewColor, getColor (subentry->dNewColor)->zName.chars());
				
				entryIcon = getIcon ("palette");
			}
			break;
		
		case HISTORY_ListMove:
			{
				ListMoveHistory* subentry = static_cast<ListMoveHistory*> (entry);
				ulong ulCount = subentry->ulaIndices.size ();
				
				text.format ("Moved %lu objects %s", ulCount,
					subentry->bUp ? "up" : "down");
				entryIcon = getIcon (subentry->bUp ? "arrow-up" : "arrow-down");
			}
			break;
		
		case HISTORY_Edit:
			{
				EditHistory* subentry = static_cast<EditHistory*> (entry);
				
				text.format ("Edited %u objects\n%s",
					subentry->paNewObjs.size(),
					LDObject::objectListContents (subentry->paOldObjs).chars ());
				entryIcon = getIcon ("set-contents");
			}
			break;
		
		case HISTORY_Inline:
			{
				InlineHistory* subentry = static_cast<InlineHistory*> (entry);
				
				text.format ("%s %lu subfiles:\n%lu resultants",
					(subentry->bDeep) ? "Deep-inlined" : "Inlined",
					(ulong) subentry->paRefs.size(),
					(ulong) subentry->ulaBitIndices.size());
				
				entryIcon = getIcon (subentry->bDeep ? "inline-deep" : "inline");
			}
			break;
		
		default:
			text = "???";
			break;
		}
		
		QListWidgetItem* item = new QListWidgetItem;
		item->setText (text);
		item->setIcon (entryIcon);
		historyList->addItem (item);
	}
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void HistoryDialog::slot_undo () {
	History::undo ();
	updateButtons ();
	updateSelection ();
}

// =============================================================================
void HistoryDialog::slot_redo () {
	History::redo ();
	updateButtons ();
	updateSelection ();
}

void HistoryDialog::updateSelection () {
	historyList->setCurrentItem (historyList->item (History::pos () + 1));
}

// =============================================================================
void HistoryDialog::slot_clear () {
	if (!confirm ("Are you sure you want to clear the edit history?\nThis cannot be undone."))
		return; // Canceled
	
	History::clear ();
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
	
	QListWidgetItem* qItem = historyList->selectedItems ()[0];
	
	// Find the index of the edit
	long lIdx;
	for (lIdx = 0; lIdx < historyList->count (); ++lIdx)
		if (historyList->item (lIdx) == qItem)
			break;
	
	// qHistoryList is 0-based, History is -1-based, thus decrement
	// the index now
	lIdx--;
	
	if (lIdx == History::pos ())
		return;
	
	// Seek to the selected edit by repeadetly undoing or redoing.
	while (History::pos () != lIdx) {
		if (History::pos () > lIdx)
			History::undo ();
		else
			History::redo ();
	}
	
	updateButtons ();
}