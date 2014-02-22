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

#include "InvokationDeferer.h"
#include "../Misc.h"

static InvokationDeferer* g_invokationDeferer = new InvokationDeferer();

// =============================================================================
//
InvokationDeferer::InvokationDeferer (QObject* parent) : QObject (parent)
{
	connect (this, SIGNAL (functionAdded()), this, SLOT (invokeFunctions()),
		Qt::QueuedConnection);
}

// =============================================================================
//
void InvokationDeferer::addFunctionCall (InvokationDeferer::FunctionType func)
{
	m_funcs << func;
	removeDuplicates (m_funcs);
	emit functionAdded();
}

// =============================================================================
//
void InvokationDeferer::invokeFunctions()
{
	for (FunctionType func : m_funcs)
		(*func)();

	m_funcs.clear();
}

// =============================================================================
//
void invokeLater (InvokationDeferer::FunctionType func)
{
	g_invokationDeferer->addFunctionCall (func);
}