#include <qdialog.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qdialogbuttonbox.h>
#include "ldtypes.h"

class Dialog_SetContents : public QDialog {
public:
	QLabel* qContentsLabel;
	QLineEdit* qContents;
	QDialogButtonBox* qOKCancel;
	
	LDObject* patient;
	
	Dialog_SetContents (LDObject* obj, QWidget* parent = nullptr);
	static void staticDialog (LDObject* obj, LDForgeWindow* parent);
	
private slots:
	void slot_handleButtons (QAbstractButton* qButton);
};