#ifndef EXTPROGS_H
#define EXTPROGS_H

#include <qobject.h>

class QProcess;
class ProcessWaiter : public QObject {
	Q_OBJECT
	
public:
	ProcessWaiter (QProcess* proc, bool& readyvar) : m_proc (proc), m_readyvar (readyvar) {
		m_readyvar = false;
	}
	
	int exitFlag () { return m_exitflag; }
	
public slots:
	void slot_procDone (int exitflag) {
		m_readyvar = true;
		m_exitflag = exitflag;
	}
	
private:
	QProcess* m_proc;
	bool& m_readyvar;
	int m_exitflag;
};

enum extprog {
	IseCalc,
	Intersector,
	Coverer,
	Ytruder,
	DATHeader
};

void runYtruder ();

#endif // EXTPROGS_H