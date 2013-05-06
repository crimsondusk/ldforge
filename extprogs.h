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