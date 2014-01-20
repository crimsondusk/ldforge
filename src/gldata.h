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
 * LDObjects and stored in cache for faster rendering.
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
			QRgb		rgb;		// Color of this poly normally
			QRgb		pickrgb;	// Color of this poly while picking
			bool		isCondLine;	// Is this a conditional line?
			LDObject*	obj;		// Pointer to the object this poly represents
		};

		struct Vertex
		{
			float x, y, z;
			uint32 color;
			float pad[4];
		};

		class Array
		{
			public:
				typedef int32 Size;

				Array();
				Array (const Array& other) = delete;
				~Array();

				void clear();
				void merge (Array* other);
				void resizeToFit (Size newSize);
				const Size& allocatedSize() const;
				Size writtenSize() const;
				const Vertex* data() const;
				void write (const VertexCompiler::Vertex& f);
				Array& operator= (const Array& other) = delete;

			private:
				Vertex* m_data;
				Vertex* m_ptr;
				Size m_size;
		};

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

		static uint32 getColorRGB (QColor& color);

	private:
		void compilePolygon (LDObject* drawobj, LDObject* trueobj, QList<CompiledTriangle>& data);
		void compileObject (LDObject* obj);
		void compileSubObject (LDObject* obj, LDObject* topobj, QList<CompiledTriangle>& data);
		Array* postprocess (const CompiledTriangle& i, GL::VAOType type);

		QMap<LDObject*, Array*>	m_objArrays;
		QMap<LDDocument*, Array*>	m_fileCache;
		Array					m_mainArrays[GL::NumArrays];
		LDDocument*					m_file;
		bool					m_changed[GL::NumArrays];
		LDObjectList			m_staged;
};

extern VertexCompiler g_vertexCompiler;

#endif // LDFORGE_GLDATA_H
