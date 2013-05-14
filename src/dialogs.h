#ifndef DIALOGS_H
#define DIALOGS_H

#include <QDialog>
#include "common.h"

class QDialogButtonBox;
class QDoubleSpinBox;
class QPushButton;
class QLineEdit;
class QSpinBox;
class RadioBox;
class CheckBoxGroup;
class QLabel;
class QAbstractButton;

class OverlayDialog : public QDialog {
	Q_OBJECT
	
public:
	explicit OverlayDialog (QWidget* parent = null, Qt::WindowFlags f = 0);
	
	str			fpath		() const;
	ushort		ofsx		() const;
	ushort		ofsy		() const;
	double		lwidth		() const;
	double		lheight		() const;
	int			 camera		() const;
	
private:
	RadioBox* rb_camera;
	QPushButton* btn_fpath;
	QLineEdit* le_fpath;
	QSpinBox* sb_ofsx, *sb_ofsy;
	QDoubleSpinBox* dsb_lwidth, *dsb_lheight;
	QDialogButtonBox* dbb_buttons;
	
private slots:
	void slot_fpath ();
	void slot_help ();
	void slot_dimensionsChanged ();
	void fillDefaults (int newcam);
};

class ReplaceCoordsDialog : public QDialog {
public:
	explicit ReplaceCoordsDialog (QWidget* parent = null, Qt::WindowFlags f = 0);
	
	std::vector< int > axes () const;
	double searchValue () const;
	double replacementValue () const;
	
private:
	CheckBoxGroup* cbg_axes;
	QLabel* lb_search, *lb_replacement;
	QDoubleSpinBox* dsb_search, *dsb_replacement;
};

// =============================================================================
// SetContentsDialog
//
// Performs the Set Contents dialog on the given LDObject. Object's contents
// are exposed to the user and is reinterpreted if the user accepts the new
// contents.
// =============================================================================
class SetContentsDialog : public QDialog {
public:
	explicit SetContentsDialog (QWidget* parent = null, Qt::WindowFlags f = 0);
	str text () const;
	void setObject (LDObject* obj);
	
private:
	QLabel* lb_contents, *lb_errorIcon, *lb_error;
	QLineEdit* le_contents;
};

class LDrawPathDialog : public QDialog {
	Q_OBJECT
	
public:
	explicit LDrawPathDialog (const bool validDefault, QWidget* parent = null, Qt::WindowFlags f = 0);
	str path () const;
	void setPath (str path);
	
private:
	Q_DISABLE_COPY (LDrawPathDialog)
	
	QLabel* lb_resolution;
	QLineEdit* le_path;
	QPushButton* btn_findPath, *btn_tryConfigure, *btn_cancel;
	QDialogButtonBox* dbb_buttons;
	const bool m_validDefault;
	
	QPushButton* okButton ();
	
private slots:
	void slot_findPath ();
	void slot_tryConfigure ();
	void slot_exit ();
};

class NewPartDialog : public QDialog {
public:
	enum { CCAL, NonCA, NoLicense };
	enum { CCW, CW, NoWinding };
	
	explicit NewPartDialog (QWidget* parent = null, Qt::WindowFlags f = 0);
	static void StaticDialog ();
	
	QLabel* lb_brickIcon, *lb_name, *lb_author, *lb_license, *lb_BFC;
	QLineEdit* le_name, *le_author;
	RadioBox* rb_license, *rb_BFC;
};

#endif // DIALOGS_H