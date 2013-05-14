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
template<class T> class CheckBoxGroup;
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
	
	vector<Axis> axes () const;
	double searchValue () const;
	double replacementValue () const;
	
private:
	CheckBoxGroup<Axis>* cbg_axes;
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

#endif // DIALOGS_H