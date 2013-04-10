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
#include "colors.h"
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
	
	qHistoryList->setAlternatingRowColors (true);
	
	qUndoButton->setIcon (getIcon ("undo"));
	qRedoButton->setIcon (getIcon ("redo"));
	
	connect (qUndoButton, SIGNAL (clicked ()), this, SLOT (slot_undo ()));
	connect (qRedoButton, SIGNAL (clicked ()), this, SLOT (slot_redo ()));
	connect (qClearButton, SIGNAL (clicked ()), this, SLOT (slot_clear ()));
	connect (qButtons, SIGNAL (rejected ()), this, SLOT (reject ()));
	connect (qHistoryList, SIGNAL (itemSelectionChanged ()), this, SLOT (slot_selChanged ()));
	
	QVBoxLayout* qButtonLayout = new QVBoxLayout;
	qButtonLayout->setDirection (QBoxLayout::TopToBottom);
	qButtonLayout->addWidget (qUndoButton);
	qButtonLayout->addWidget (qRedoButton);
	qButtonLayout->addWidget (qClearButton);
	qButtonLayout->addStretch ();
	
	QGridLayout* qLayout = new QGridLayout;
	qLayout->setColumnStretch (0, 1);
	qLayout->addWidget (qHistoryList, 0, 0, 2, 1);
	qLayout->addLayout (qButtonLayout, 0, 1);
	qLayout->addWidget (qButtons, 1, 1);
	
	setLayout (qLayout);
	setWindowIcon (getIcon ("history"));
	setWindowTitle (APPNAME_DISPLAY " - Edit history");
	
	populateList ();
	updateButtons ();
	updateSelection ();
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
void HistoryDialog::populateList () {
	qHistoryList->clear ();
	
	QListWidgetItem* qItem = new QListWidgetItem;
	qItem->setText ("[[ initial state ]]");
	qItem->setIcon (getIcon ("empty"));
	qHistoryList->addItem (qItem);
	
	for (HistoryEntry* entry : History::entries) {
		str zText;
		QIcon qEntryIcon;
		
		switch (entry->type ()) {
		case HISTORY_Add:
			{
				AddHistory* addentry = static_cast<AddHistory*> (entry);
				ulong ulCount = addentry->paObjs.size ();
				str zVerb = "Added";
				
				switch (addentry->eType) {
				case AddHistory::Paste:
					zVerb = "Pasted";
					qEntryIcon = getIcon ("paste");
					break;
				
				default:
					{
						// Determine a common type for these objects. If all objects are of the same
						// type, we display its addition icon. Otherwise, we draw a subfile addition
						// one as a default.
						LDObjectType_e eCommonType = OBJ_Unidentified;
						for (LDObject* obj : addentry->paObjs) {
							if (eCommonType == OBJ_Unidentified or obj->getType() == eCommonType)
								eCommonType = obj->getType ();
							else {
								eCommonType = OBJ_Unidentified;
								break;
							}
						}
						
						// Set the icon based on the common type decided above.
						if (eCommonType == OBJ_Unidentified)
							qEntryIcon = getIcon ("add-subfile");
						else
							qEntryIcon = getIcon (str::mkfmt ("add-%s", g_saObjTypeIcons[eCommonType]));
					}
					break;
				}
				
				zText.format ("%s %lu objects\n%s", zVerb.chars(), ulCount,
					LDObject::objectListContents (addentry->paObjs).chars());
			}
			break;
		
		case HISTORY_QuadSplit:
			{
				QuadSplitHistory* splitentry = static_cast<QuadSplitHistory*> (entry);
				ulong ulCount = splitentry->paQuads.size ();
				zText.format ("Split %lu quad%s to triangles", ulCount, PLURAL (ulCount));
				
				qEntryIcon = getIcon ("quad-split");
			}
			break;
		
		case HISTORY_Del:
			{
				DelHistory* delentry = static_cast<DelHistory*> (entry);
				ulong ulCount = delentry->cache.size ();
				str zVerb = "Deleted";
				qEntryIcon = getIcon ("delete");
				
				switch (delentry->eType) {
				case DelHistory::Cut:
					qEntryIcon = getIcon ("cut");
					zVerb = "Cut";
					break;
				
				default:
					break;
				}
				
				zText.format ("%s %lu objects:\n%s", zVerb.chars(), ulCount,
					LDObject::objectListContents (delentry->cache).chars ());
			}
			break;
		
		case HISTORY_SetColor:
			{
				SetColorHistory* colentry = static_cast<SetColorHistory*> (entry);
				ulong ulCount = colentry->ulaIndices.size ();
				zText.format ("Set color of %lu objects to %d (%s)", ulCount,
					colentry->dNewColor, getColor (colentry->dNewColor)->zName.chars());
				
				qEntryIcon = getIcon ("palette");
			}
			break;
		
		case HISTORY_ListMove:
			{
				ListMoveHistory* moveentry = static_cast<ListMoveHistory*> (entry);
				ulong ulCount = moveentry->ulaIndices.size ();
				
				zText.format ("Moved %lu objects %s", ulCount,
					moveentry->bUp ? "up" : "down");
				qEntryIcon = getIcon (moveentry->bUp ? "arrow-up" : "arrow-down");
			}
			break;
		
		case HISTORY_SetContents:
			{
				SetContentsHistory* setentry = static_cast<SetContentsHistory*> (entry);
				
				zText.format ("Set contents of %s\n%s (%s)",
					g_saObjTypeNames [setentry->oldObj->getType ()],
					setentry->newObj->getContents ().chars (),
					g_saObjTypeNames [setentry->newObj->getType ()]);
				qEntryIcon = getIcon ("set-contents");
			}
			break;
		
		case HISTORY_Inline:
			{
				InlineHistory* subentry = static_cast<InlineHistory*> (entry);
				
				zText.format ("%s %lu subfiles:\n%lu resultants",
					(subentry->bDeep) ? "Deep-inlined" : "Inlined",
					(ulong) subentry->paRefs.size(),
					(ulong) subentry->ulaBitIndices.size());
				
				qEntryIcon = getIcon (subentry->bDeep ? "inline-deep" : "inline");
			}
			break;
		
		default:
			zText = "???";
			break;
		}
		
		QListWidgetItem* qItem = new QListWidgetItem;
		qItem->setText (zText);
		qItem->setIcon (qEntryIcon);
		qHistoryList->addItem (qItem);
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
	qHistoryList->setCurrentItem (qHistoryList->item (History::pos () + 1));
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
	updateButtons ();
}

// =============================================================================
void HistoryDialog::updateButtons () {
	qUndoButton->setEnabled (ACTION_NAME (undo)->isEnabled ());
	qRedoButton->setEnabled (ACTION_NAME (redo)->isEnabled ());
}

// =============================================================================
void HistoryDialog::slot_selChanged () {
	if (qHistoryList->selectedItems ().size () != 1)
		return;
	
	QListWidgetItem* qItem = qHistoryList->selectedItems ()[0];
	
	// Find the index of the edit
	long idx = -1;
	QListWidgetItem* it;
	while ((it = qHistoryList->item (++idx)) != nullptr)
		if (it == qItem)
			break;
	
	idx--; // qHistoryList is 0-based, History is -1-based.
	
	if (idx == History::pos ())
		return;
	
	// Seek to the selected edit by repeadetly undoing or redoing.
	while (History::pos () != idx) {
		if (History::pos () > idx)
			History::undo ();
		else
			History::redo ();
	}
	
	updateButtons ();
}