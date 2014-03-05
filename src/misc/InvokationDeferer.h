/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
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

#pragma once
#include <QObject>

class InvokationDeferer : public QObject
{
	Q_OBJECT

	public:
		using FunctionType = void(*)();

		explicit InvokationDeferer (QObject* parent = 0);
		void addFunctionCall (FunctionType func);

	signals:
		void functionAdded();

	private:
		QList<FunctionType>	m_funcs;

	private slots:
		void invokeFunctions();
};

void invokeLater (InvokationDeferer::FunctionType func);
