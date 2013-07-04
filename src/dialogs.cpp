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

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QBoxLayout>
#include <QGridLayout>
#include <qprogressbar.h>
#include <QCheckBox>

#include "dialogs.h"
#include "widgets.h"
#include "gui.h"
#include "gldraw.h"
#include "docs.h"
#include "file.h"
#include "dialogs.h"
#include "ui_overlay.h"

extern_cfg (str, io_ldpath);

// =============================================================================
OverlayDialog::OverlayDialog( QWidget* parent, Qt::WindowFlags f ) : QDialog( parent, f )
{
	ui = new Ui_OverlayUI;
	ui->setupUi( this );
	
	m_cameraArgs = {
		{ ui->top,    GL::Top },
		{ ui->bottom, GL::Bottom },
		{ ui->front,  GL::Front },
		{ ui->back,   GL::Back },
		{ ui->left,   GL::Left },
		{ ui->right,  GL::Right }
	};
	
	GL::Camera cam = g_win->R()->camera();
	
	if( cam == GL::Free )
		cam = GL::Top;
	
	connect( ui->width, SIGNAL( valueChanged( double )), this, SLOT( slot_dimensionsChanged() ));
	connect( ui->height, SIGNAL( valueChanged( double )), this, SLOT( slot_dimensionsChanged() ));
	connect( ui->buttonBox, SIGNAL( helpRequested() ), this, SLOT( slot_help() ));
	connect( ui->fileSearchButton, SIGNAL( clicked( bool )), this, SLOT( slot_fpath() ));
	
	slot_dimensionsChanged();
	fillDefaults( cam );
}

OverlayDialog::~OverlayDialog()
{
	delete ui;
}

void OverlayDialog::fillDefaults( int newcam )
{
	overlayMeta& info = g_win->R()->getOverlay( newcam );
	
	radioDefault<int>( newcam, m_cameraArgs );
	
	if( info.img != null )
	{
		ui->filename->setText( info.fname );
		ui->originX->setValue( info.ox );
		ui->originY->setValue( info.oy );
		ui->width->setValue( info.lw );
		ui->height->setValue( info.lh );
	}
	else
	{
		ui->filename->setText( "" );
		ui->originX->setValue( 0 );
		ui->originY->setValue( 0 );
		ui->width->setValue( 0.0f );
		ui->height->setValue( 0.0f );
	}
}

str OverlayDialog::fpath() const
{
	return ui->filename->text();
}

ushort OverlayDialog::ofsx() const
{
	return ui->originX->value();
}

ushort OverlayDialog::ofsy() const
{
	return ui->originY->value();
}

double OverlayDialog::lwidth() const
{
	return ui->width->value();
}

double OverlayDialog::lheight() const
{
	return ui->height->value();
}

int OverlayDialog::camera() const
{
	return radioSwitch<int>( GL::Top, m_cameraArgs );
}

void OverlayDialog::slot_fpath()
{
	ui->filename->setText( QFileDialog::getOpenFileName( null, "Overlay image" ));
}

void OverlayDialog::slot_help()
{
	showDocumentation( g_docs_overlays );
}

void OverlayDialog::slot_dimensionsChanged()
{
	bool enable = ( ui->width->value() != 0 ) || (  ui->height->value() != 0 );
	ui->buttonBox->button( QDialogButtonBox::Ok )->setEnabled( enable );
}

// =================================================================================================
LDrawPathDialog::LDrawPathDialog (const bool validDefault, QWidget* parent, Qt::WindowFlags f)
	: QDialog (parent, f), m_validDefault (validDefault)
{
	QLabel* lb_description = null;
	lb_resolution = new QLabel ("---");
	
	if (validDefault == false)
		lb_description = new QLabel ("Please input your LDraw directory");
	
	QLabel* lb_path = new QLabel ("LDraw path:");
	le_path = new QLineEdit;
	btn_findPath = new QPushButton;
	btn_findPath->setIcon (getIcon ("folder"));
	
	btn_cancel = new QPushButton;
	
	if (validDefault == false) {
		btn_cancel->setText ("Exit");
		btn_cancel->setIcon (getIcon ("exit"));
	} else {
		btn_cancel->setText ("Cancel");
		btn_cancel->setIcon (getIcon ("cancel"));
	}
	
	dbb_buttons = new QDialogButtonBox (QDialogButtonBox::Ok);
	dbb_buttons->addButton (btn_cancel, QDialogButtonBox::RejectRole);
	okButton ()->setEnabled (false);
	
	QHBoxLayout* inputLayout = new QHBoxLayout;
	inputLayout->addWidget (lb_path);
	inputLayout->addWidget (le_path);
	inputLayout->addWidget (btn_findPath);
	
	QVBoxLayout* mainLayout = new QVBoxLayout;
	
	if (validDefault == false)
		mainLayout->addWidget (lb_description);
	
	mainLayout->addLayout (inputLayout);
	mainLayout->addWidget (lb_resolution);
	mainLayout->addWidget (dbb_buttons);
	setLayout (mainLayout);
	
	connect (le_path, SIGNAL (textEdited (QString)), this, SLOT (slot_tryConfigure ()));
	connect (btn_findPath, SIGNAL (clicked ()), this, SLOT (slot_findPath ()));
	connect (dbb_buttons, SIGNAL (accepted ()), this, SLOT (accept ()));
	connect (dbb_buttons, SIGNAL (rejected ()), this, (validDefault) ? SLOT (reject ()) : SLOT (slot_exit ()));
	
	setPath (io_ldpath);
	if (validDefault)
		slot_tryConfigure ();
}

QPushButton* LDrawPathDialog::okButton () {
	return dbb_buttons->button (QDialogButtonBox::Ok);
}

void LDrawPathDialog::setPath (str path) {
	le_path->setText (path);
}

str LDrawPathDialog::filename () const {
	return le_path->text ();
}

void LDrawPathDialog::slot_findPath () {
	str newpath = QFileDialog::getExistingDirectory (this, "Find LDraw Path");
	
	if (newpath.length () > 0 && newpath != filename ()) {
		setPath (newpath);
		slot_tryConfigure ();
	}
}

void LDrawPathDialog::slot_exit () {
	exit (1);
}

void LDrawPathDialog::slot_tryConfigure () {
	if (LDPaths::tryConfigure (filename ()) == false) {
		lb_resolution->setText (fmt ("<span style=\"color:red; font-weight: bold;\">%1</span>",
			LDPaths::getError()));
		okButton ()->setEnabled (false);
		return;
	}
	
	lb_resolution->setText ("<span style=\"color: #7A0; font-weight: bold;\">OK!</span>");
	okButton ()->setEnabled (true);
}

// =============================================================================
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// =============================================================================
OpenProgressDialog::OpenProgressDialog (QWidget* parent, Qt::WindowFlags f) : QDialog (parent, f) {
	progressBar = new QProgressBar;
	progressText = new QLabel( "Parsing..." );
	setNumLines (0);
	m_progress = 0;
	
	QDialogButtonBox* dbb_buttons = new QDialogButtonBox;
	dbb_buttons->addButton (QDialogButtonBox::Cancel);
	connect (dbb_buttons, SIGNAL (rejected ()), this, SLOT (reject ()));
	
	QVBoxLayout* layout = new QVBoxLayout (this);
	layout->addWidget (progressText);
	layout->addWidget (progressBar);
	layout->addWidget (dbb_buttons);
}

READ_ACCESSOR (ulong, OpenProgressDialog::numLines) {
	return m_numLines;
}

SET_ACCESSOR (ulong, OpenProgressDialog::setNumLines) {
	m_numLines = val;
	progressBar->setRange (0, numLines ());
	updateValues ();
}

void OpenProgressDialog::updateValues () {
	progressText->setText( fmt( "Parsing... %1 / %2", progress(), numLines() ));
	progressBar->setValue( progress() );
}

void OpenProgressDialog::updateProgress (int progress) {
	m_progress = progress;
	updateValues ();
}