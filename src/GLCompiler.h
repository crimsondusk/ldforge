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
// GLCompiler
//
// This class manages vertex arrays for the GL renderer, compiling vertices into
// VAO-readable triangles which can be requested with getMergedBuffer.
//
// There are 6 main array types:
// - the normal polygon array, for triangles
// - edge line array, for lines
// - conditional line array, for conditional lines. Kept separate so that they
//       can be drawn as dashed liness
// - BFC array, this is the same as the normal polygon array except that the
//       polygons are listed twice, once normally and green and once reversed
//       and red, this allows BFC red/green view.
// - Picking array, this is the same as the normal polygon array except the
//       polygons are compiled with their index color, this way the picking
//       method is capable of determining which object was selected by pixel
//       color.
// - Edge line picking array, the pick array version of the edge line array.
//       Conditional lines are grouped with normal edgelines here.
//
// There are also these same 5 arrays for every LDObject compiled. The main
// arrays are generated on demand from the ones in the current file's
// LDObjects and stored in cache for faster renmm dering.
//
// The nested Array class contains a vector-like buffer of the Vertex structs,
// these structs are the VAOs that get passed to the renderer.
//
class GLCompiler
{
	public:
		enum E_ColorType
		{
			E_NormalColor,
			E_BFCFrontColor,
			E_BFCBackColor,
			E_PickColor,
		};

		struct CompiledTriangle
		{
			Vertex		verts[3];
			uint8		numVerts;	// 2 if a line
			uint32		rgb;		// Color of this poly normally
			uint32		pickrgb;	// Color of this poly while picking
			bool		isCondLine;	// Is this a conditional line?
			LDObject*	obj;		// Pointer to the object this poly represents
		};

		struct VAO
		{
			float x, y, z;
			uint32 color;
			float pad[4];
		};

		using PolygonList = QList<CompiledTriangle>;
		using VertexArray = QVector<VAO>;

		GLCompiler();
		~GLCompiler();
		void setFile (LDDocument* file);
		void compileDocument();
		void forgetObject (LDObject* obj);
		void initObject (LDObject* obj);
		const VertexArray* getMergedBuffer (E_VertexArrayType type);
		QColor getObjectColor (LDObject* obj, E_ColorType colortype) const;
		void needMerge();
		void stageForCompilation (LDObject* obj);

		static uint32 getColorRGB (const QColor& color);

	private:
		void compilePolygon (LDObject* drawobj, LDObject* trueobj, PolygonList& data);
		void compileObject (LDObject* obj);
		void compileSubObject (LDObject* obj, LDObject* topobj, PolygonList& data);
		VertexArray* postprocess (const CompiledTriangle& poly, E_VertexArrayType type);

		QMap<LDObject*, VertexArray*>				m_objArrays;
		VertexArray								m_mainArrays[E_NumVertexArrays];
		LDDocument*							m_file;
		bool								m_changed[E_NumVertexArrays];
		LDObjectList						m_staged; // Objects that need to be compiled
};

extern GLCompiler g_vertexCompiler;

#endif // LDFORGE_GLCOMPILER_H
