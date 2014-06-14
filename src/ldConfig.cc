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

#include <QFile>
#include "ldDocument.h"
#include "ldConfig.h"
#include "mainWindow.h"
#include "miscallenous.h"
#include "colors.h"

// =============================================================================
//
// Helper function for parseLDConfig
//
static bool parseLDConfigTag (LDConfigParser& pars, char const* tag, QString& val)
{
	int pos;

	// Try find the token and get its position
	if (not pars.findToken (pos, tag, 1))
		return false;

	// Get the token after it and store it into val
	return pars.getToken (val, pos + 1);
}

// =============================================================================
//
void LDConfigParser::parseLDConfig()
{
	QFile* fp = openLDrawFile ("LDConfig.ldr", false);

	if (fp == null)
	{
		critical (QObject::tr ("Unable to open LDConfig.ldr for parsing."));
		return;
	}

	// Read in the lines
	while (not fp->atEnd())
	{
		QString line = QString::fromUtf8 (fp->readLine());

		if (line.isEmpty() || line[0] != '0')
			continue; // empty or illogical

		line.remove ('\r');
		line.remove ('\n');

		// Parse the line
		LDConfigParser pars (line, ' ');

		int code = 0, alpha = 255;
		QString name, facename, edgename, valuestr;

		// Check 0 !COLOUR, parse the name
		if (not pars.tokenCompare (0, "0") ||
			not pars.tokenCompare (1, "!COLOUR") ||
			not pars.getToken (name, 2))
		{
			continue;
		}

		// Replace underscores in the name with spaces for readability
		name.replace ("_", " ");

		// Get the CODE tag
		if (not parseLDConfigTag (pars, "CODE", valuestr))
			continue;

		// Ensure that the code is within [0 - 511]
		bool ok;
		code = valuestr.toShort (&ok);

		if (not ok || code < 0 || code >= 512)
			continue;

		// VALUE and EDGE tags
		if (not parseLDConfigTag (pars, "VALUE", facename) || not parseLDConfigTag (pars, "EDGE", edgename))
			continue;

		// Ensure that our colors are correct
		QColor faceColor (facename),
			edgeColor (edgename);

		if (not faceColor.isValid() || not edgeColor.isValid())
			continue;

		// Parse alpha if given.
		if (parseLDConfigTag (pars, "ALPHA", valuestr))
			alpha = clamp (valuestr.toInt(), 0, 255);

		LDColor col (new LDColorData);
		col->_name = name;
		col->_faceColor = faceColor;
		col->_edgeColor = edgeColor;
		col->_hexcode = facename;
		col->_faceColor.setAlpha (alpha);
		col->_index = code;
		LDColor::addLDConfigColor (code, col);
	}

	fp->close();
	fp->deleteLater();
}

// =============================================================================
//
LDConfigParser::LDConfigParser (QString inText, char sep)
{
	m_tokens = inText.split (sep, QString::SkipEmptyParts);
	m_pos = -1;
}

// =============================================================================
//
bool LDConfigParser::isAtBeginning()
{
	return m_pos == -1;
}

// =============================================================================
//
bool LDConfigParser::isAtEnd()
{
	return m_pos == m_tokens.size() - 1;
}

// =============================================================================
//
bool LDConfigParser::getToken (QString& val, const int pos)
{
	if (pos >= m_tokens.size())
		return false;

	val = m_tokens[pos];
	return true;
}

// =============================================================================
//
bool LDConfigParser::getNextToken (QString& val)
{
	return getToken (val, ++m_pos);
}

// =============================================================================
//
bool LDConfigParser::peekNextToken (QString& val)
{
	return getToken (val, m_pos + 1);
}

// =============================================================================
//
bool LDConfigParser::findToken (int& result, char const* needle, int args)
{
	for (int i = 0; i < (m_tokens.size() - args); ++i)
	{
		if (m_tokens[i] == needle)
		{
			result = i;
			return true;
		}
	}

	return false;
}

// =============================================================================
//
void LDConfigParser::rewind()
{
	m_pos = -1;
}

// =============================================================================
//
void LDConfigParser::seek (int amount, bool rel)
{
	m_pos = (rel ? m_pos : 0) + amount;
}

// =============================================================================
//
int LDConfigParser::getSize()
{
	return m_tokens.size();
}

// =============================================================================
//
bool LDConfigParser::tokenCompare (int inPos, const char* sOther)
{
	QString tok;

	if (not getToken (tok, inPos))
		return false;

	return (tok == sOther);
}
