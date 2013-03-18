#include "gui.h"
#include <QDialog>
#include <qlabel.h>
#include <qlineedit.h>
#include <qdialogbuttonbox.h>
#include <qpushbutton.h>

class ConfigDialog : public QDialog {
	Q_OBJECT
	
public:
	QLabel* qLDrawPathLabel;
	QLineEdit* qLDrawPath;
	QPushButton* qLDrawPathFindButton;
	
	QDialogButtonBox* qButtons;
	
	ConfigDialog (ForgeWindow* parent);
	static void staticDialog (ForgeWindow* window);
	
private slots:
	void slot_findLDrawPath ();
};