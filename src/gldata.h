#ifndef LDFORGE_GLDATA_H
#define LDFORGE_GLDATA_H

#include "types.h"
#include "gldraw.h"
#include <QMap>
#include <QRgb>

/* =============================================================================
 * -----------------------------------------------------------------------------
 * VertexCompiler
 *
 * This class manages vertex arrays for the GL renderer, compiling vertices into
 * VAO-readable triangles which can be requested with getMergedBuffer.
 *
 * There are 5 main array types:
 * - the normal polygon array, for triangles
 * - edge line array, for lines
 * - BFC array, this is the same as the normal polygon array except that the
 * -     polygons are listed twice, once normally and green and once reversed
 * -     and red, this allows BFC red/green view.
 * - Picking array, this is the samea s the normal polygon array except the
 * -     polygons are compiled with their index color, this way the picking
 * -     method is capable of determining which object was selected by pixel
 * -     color.
 * - Edge line picking array, the pick array version of the edge line array.
 *
 * There are also these same 5 arrays for every LDObject compiled. The main
 * arrays are generated on demand from the ones in the current file's
 * LDObjects and stored in cache for faster renmm dering.
 *
 * The nested Array class contains a vector-like buffer of the Vertex structs,
 * these structs are the VAOs that get passed to the renderer.
 */

class VertexCompiler
{
	public:
		enum ColorType
		{
			Normal,
			BFCFront,
			BFCBack,
			PickColor,
		};

		struct CompiledTriangle
		{
			::Vertex	verts[3];
			uint8		numVerts;	// 2 if a line
			uint32		rgb;		// Color of this poly normally
			uint32		pickrgb;	// Color of this poly while picking
			bool		isCondLine;	// Is this a conditional line?
			LDObject*	obj;		// Pointer to the object this poly represents
		};

		struct Vertex
		{
			float x, y, z;
			uint32 color;
			float pad[4];
		};

		using PolygonList = QList<CompiledTriangle>;
		using Array = QVector<VertexCompiler::Vertex>;

		VertexCompiler();
		~VertexCompiler();
		void setFile (LDDocument* file);
		void compileDocument();
		void forgetObject (LDObject* obj);
		void initObject (LDObject* obj);
		const Array* getMergedBuffer (GL::VAOType type);
		QColor getObjectColor (LDObject* obj, ColorType list) const;
		void needMerge();
		void stageForCompilation (LDObject* obj);

		static uint32 getColorRGB (const QColor& color);

	private:
		void compilePolygon (LDObject* drawobj, LDObject* trueobj, PolygonList& data);
		void compileObject (LDObject* obj);
		void compileSubObject (LDObject* obj, LDObject* topobj, PolygonList& data);
		Array* postprocess (const VertexCompiler::CompiledTriangle& i, GLRenderer::VAOType type);

		QMap<LDObject*, Array*>				m_objArrays;
		Array								m_mainArrays[GL::ENumArrays];
		LDDocument*							m_file;
		bool								m_changed[GL::ENumArrays];
		LDObjectList						m_staged; // Objects that need to be compiled
};

extern VertexCompiler g_vertexCompiler;

#endif // LDFORGE_GLDATA_H
