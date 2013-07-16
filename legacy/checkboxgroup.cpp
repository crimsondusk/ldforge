// =============================================================================
// CheckBoxGroup
// =============================================================================
class CheckBoxGroup : public QGroupBox {
	Q_OBJECT
	
public:
	CheckBoxGroup (const char* label, Qt::Orientation orient = Qt::Horizontal, QWidget* parent = null);
	
	void			addCheckBox		(const char* label, int key, bool checked = false);
	vector<int>	checkedValues		() const;
	QCheckBox*		getCheckBox		(int key);
	bool			buttonChecked		(int key);
	
signals:
	void selectionChanged	();
	
private:
	QBoxLayout* m_layout;
	std::map<int, QCheckBox*> m_vals;
	
private slots:
	void buttonChanged		();
};

CheckBoxGroup::CheckBoxGroup (const char* label, Qt::Orientation orient, QWidget* parent) : QGroupBox (parent) {
	m_layout = new QBoxLayout (makeDirection (orient));
	setTitle (label);
	setLayout (m_layout);
}

void CheckBoxGroup::addCheckBox (const char* label, int key, bool checked) {
	if (m_vals.find (key) != m_vals.end ())
		return;
	
	QCheckBox* box = new QCheckBox (label);
	box->setChecked (checked);
	
	m_vals[key] = box;
	m_layout->addWidget (box);
	
	connect (box, SIGNAL (stateChanged (int)), this, SLOT (buttonChanged ()));
}

vector<int> CheckBoxGroup::checkedValues () const {
	vector<int> vals;
	
	for (const auto& kv : m_vals)
		if (kv.second->isChecked ())
			vals << kv.first;
	
	return vals;
}

QCheckBox* CheckBoxGroup::getCheckBox (int key) {
	return m_vals[key];
}

void CheckBoxGroup::buttonChanged () {
	emit selectionChanged ();
}

bool CheckBoxGroup::buttonChecked (int key) {
	return m_vals[key]->isChecked ();
}

CheckBoxGroup* makeAxesBox() {
	CheckBoxGroup* cbg_axes = new CheckBoxGroup ("Axes", Qt::Horizontal);
	cbg_axes->addCheckBox ("X", X);
	cbg_axes->addCheckBox ("Y", Y);
	cbg_axes->addCheckBox ("Z", Z);
	return cbg_axes;
}