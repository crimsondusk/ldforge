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

#endif // DIALOGS_H