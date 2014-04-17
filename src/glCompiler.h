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

#ifndef LDFORGE_GLCOMPILER_H
#define LDFORGE_GLCOMPILER_H

#include "main.h"
#include "glRenderer.h"
#include "glShared.h"
#include <QMap>

// =============================================================================
//
class GLCompiler
{
public:
	struct ObjectVBOInfo
	{
		QVector<GLfloat>	data[g_numVBOs];
		bool				isChanged;
	};

	GLCompiler();
	~GLCompiler();
	void				compileDocument (LDDocument* doc);
	void				dropObject (LDObject* obj);
	void				initialize();
	QColor				polygonColor (LDPolygon& poly, LDObject* topobj) const;
	QColor				indexColorForID (int id) const;
	void				needMerge();
	void				prepareVBO (int vbonum);
	void				stageForCompilation (LDObject* obj);

	static uint32		colorToRGB (const QColor& color);

	static inline int	vboNumber (EVBOSurface surface, EVBOComplement complement)
	{
		return (surface * VBOCM_NumComplements) + complement;
	}

	inline GLuint		vbo (int vbonum) const
	{
		return m_vbo[vbonum];
	}

	inline int			vboSize (int vbonum) const
	{
		return m_vboSizes[vbonum];
	}

private:
	void			compileStaged();
	void			compileObject (LDObject* obj);
	void			compileSubObject (LDObject* obj, LDObject* topobj, GLCompiler::ObjectVBOInfo* objinfo);
	void			writeColor (QVector<float>& array, const QColor& color);
	void			compilePolygon (LDPolygon& poly, LDObject* topobj, GLCompiler::ObjectVBOInfo* objinfo);

	QMap<LDObject*, ObjectVBOInfo>		m_objectInfo;
	LDObjectList						m_staged; // Objects that need to be compiled
	GLuint								m_vbo[g_numVBOs];
	bool								m_vboChanged[g_numVBOs];
	int									m_vboSizes[g_numVBOs];
};

#define checkGLError() { checkGLError_private (__FILE__, __LINE__); }
void checkGLError_private (const char* file, int line);

#endif // LDFORGE_GLCOMPILER_H
