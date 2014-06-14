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
#include <QColor>
#include "main.h"

class LDColor;

class LDColorData
{
protected:
	QString _name;
	QString _hexcode;
	QColor _faceColor;
	QColor _edgeColor;
	qint32 _index;
	friend class LDConfigParser;
	friend class LDColor;
	friend void initColors();

public:
	LDColorData(){}

	readAccess (edgeColor)
	readAccess (faceColor)
	readAccess (hexcode)
	readAccess (index)
	QString		indexString() const;
	bool		isDirect() const;
	readAccess (name)
};


class LDColor : public QSharedPointer<LDColorData>
{
public:
	using Super = QSharedPointer<LDColorData>;
	using Self = LDColor;

	LDColor() : Super() {}
	LDColor (LDColorData* data) : Super (data) {}
	LDColor (Super const& other) : Super (other) {}
	LDColor (QWeakPointer<LDColorData> const& other) : Super (other) {}

	template <typename Deleter>
	LDColor (LDColorData* data, Deleter dlt) : Super (data, dlt) {}

	inline bool			operator== (Self const& other);
	inline bool			operator== (decltype(nullptr)) { return data() == nullptr; }
	inline bool			operator!= (decltype(nullptr)) { return data() != nullptr; }
	inline LDColorData*	operator->() const { return data(); }

	static void			addLDConfigColor (qint32 index, LDColor color);
	static LDColor		fromIndex (qint32 index);
};

void initColors();
int luma (const QColor& col);
int numLDConfigColors();

// Main and edge colors
LDColor maincolor();
LDColor edgecolor();
static constexpr int mainColorIndex = 16;
static constexpr int edgeColorIndex = 24;

bool LDColor::operator== (LDColor const& other)
{
	if ((data() == nullptr) ^ (other == nullptr))
		return false;

	if (data() != nullptr)
		return data()->index() == other->index();

	// both are null
	return true;
}
