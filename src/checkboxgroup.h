#include <QGroupBox>
#include <QCheckBox>
#include <QBoxLayout>
#include <map>

QBoxLayout::Direction makeDirection (Qt::Orientation orient, bool invert = false);

template<class T> class CheckBoxGroup : public QGroupBox {
public:
	CheckBoxGroup (const char* label, Qt::Orientation orient = Qt::Horizontal, QWidget* parent = null) : QGroupBox (parent) {
		m_layout = new QBoxLayout (makeDirection (orient));
		setTitle (label);
		setLayout (m_layout);
	}
	
	void addCheckBox (const char* label, T key, bool checked = false) {
		if (m_vals.find (key) != m_vals.end ())
			return;
		
		QCheckBox* box = new QCheckBox (label);
		box->setChecked (checked);
		
		m_vals[key] = box;
		m_layout->addWidget (box);
	}
	
	std::vector<T> checkedValues () const {
		std::vector<T> vals;
		
		for (const auto& kv : m_vals)
			if (kv.second->isChecked ())
				vals.push_back (kv.first);
		
		return vals;
	}
	
private:
	QBoxLayout* m_layout;
	std::map<T, QCheckBox*> m_vals;
};