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

#include "Main.h"
#include "GLRenderer.h"
#include "GLShared.h"
#include <QMap>

// =============================================================================
//
class GLCompiler
{
	PROPERTY (public,	LDDocumentPointer,	document,	setDocument,	STOCK_WRITE)

	public:
		struct ObjectVBOInfo
		{
			QVector<GLfloat> data[gNumVBOs];
		};

		GLCompiler();
		~GLCompiler();
		void				compileDocument();
		void				dropObject (LDObject* obj);
		void				initialize();
		QColor				getPolygonColor (LDPolygon& poly, LDObject* topobj) const;
		QColor				getIndexColor (int id) const;
		void				needMerge();
		void				prepareVBO (int vbonum);
		void				stageForCompilation (LDObject* obj);

		static uint32		getColorRGB (const QColor& color);

		static inline int	getVBONumber (EVBOSurface surface, EVBOComplement complement)
		{
			return (surface * vboNumComplements) + complement;
		}

		inline GLuint		getVBO (int vbonum) const
		{
			return mVBOs[vbonum];
		}

		inline int			getVBOCount (int vbonum) const
		{
			return mVBOData[vbonum].size() / 3;
		}

	private:
		void			compileStaged();
		void			compileObject (LDObject* obj);
		void			compileSubObject (LDObject* obj, LDObject* topobj, GLCompiler::ObjectVBOInfo* objinfo);
		void			writeColor (QVector<float>& array, const QColor& color);
		void			compilePolygon (LDPolygon& poly, LDObject* topobj, GLCompiler::ObjectVBOInfo* objinfo);

		QMap<LDObject*, ObjectVBOInfo>		mObjectInfo;
		QVector<GLfloat>					mVBOData[gNumVBOs];
		GLuint								mVBOs[gNumVBOs];
		bool								mChanged[gNumVBOs];
		LDObjectList						mStaged; // Objects that need to be compiled
};

#define checkGLError() { checkGLError_private (__FILE__, __LINE__); }
void checkGLError_private (const char* file, int line);

#endif // LDFORGE_GLCOMPILER_H
