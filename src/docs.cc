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

#include <QDialog>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QBoxLayout>
#include "main.h"
#include "types.h"

// =============================================================================
// -----------------------------------------------------------------------------
class DocumentViewer : public QDialog
{	public:
		explicit DocumentViewer (QWidget* parent = null, Qt::WindowFlags f = 0) : QDialog (parent, f)
		{	te_text = new QTextEdit (this);
			te_text->setMinimumSize (QSize (400, 300));
			te_text->setReadOnly (true);

			QDialogButtonBox* bbx_buttons = new QDialogButtonBox (QDialogButtonBox::Close);
			QVBoxLayout* layout = new QVBoxLayout (this);
			layout->addWidget (te_text);
			layout->addWidget (bbx_buttons);

			connect (bbx_buttons, SIGNAL (rejected()), this, SLOT (reject()));
		}

		void setText (const char* text)
		{	te_text->setText (text);
		}

	private:
		QTextEdit* te_text;
};

const char* g_docs_overlays =
	"<h1>Overlay images</h1><br />"
	"<p>" APPNAME " supports drawing transparent images over the part model. This "
	"can be used to have, for instance, a photo of the part overlaid on top of the "
	"model and use it for drawing curves somewhat accurately.</p>"
	"<p>For this purpose, a specific photo has to be taken of the part; it should "
	"represent the part as true as possible to the actual camera used for editing. "
	"The image should be taken from straight above the part, at as an orthogonal "
	"angle as possible. It is recommended to take a lot of pictures this way and "
	"select the best candidate.</p>"
	"<p>The image should then be cropped with the knowledge of the image's LDU "
	"dimensions in mind. The offset should then be identified in the image in pixels.</p>"
	"<p>Finally, use the \"Set Overlay Image\" dialog and fill in the details. The "
	"overlay image should then be ready for use.";

// =============================================================================
// -----------------------------------------------------------------------------
void showDocumentation (const char* text)
{	DocumentViewer dlg;
	dlg.setText (text);
	dlg.exec();
}
