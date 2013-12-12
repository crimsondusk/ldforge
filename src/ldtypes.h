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

#ifndef LDFORGE_LDTYPES_H
#define LDFORGE_LDTYPES_H

#include "main.h"
#include "types.h"

#define LDOBJ(T) \
public: \
	virtual ~LD##T() {} \
	virtual LDObject::Type getType() const override { \
		return LDObject::T; \
	} \
	virtual str raw(); \
	virtual LD##T* clone() { \
		return new LD##T (*this); \
	} \
	virtual void move (vertex where); \
	virtual void invert();

#define LDOBJ_NAME(N)          virtual str getTypeName() const override { return #N; }
#define LDOBJ_VERTICES(V)      virtual int vertices() const override { return V; }
#define LDOBJ_SETCOLORED(V)    virtual bool isColored() const override { return V; }
#define LDOBJ_COLORED          LDOBJ_SETCOLORED (true)
#define LDOBJ_UNCOLORED        LDOBJ_SETCOLORED (false)

#define LDOBJ_CUSTOM_SCEMANTIC virtual bool isScemantic() const override
#define LDOBJ_SCEMANTIC        LDOBJ_CUSTOM_SCEMANTIC { return true; }
#define LDOBJ_NON_SCEMANTIC    LDOBJ_CUSTOM_SCEMANTIC { return false; }

#define LDOBJ_SETMATRIX(V)     virtual bool hasMatrix() const override { return V; }
#define LDOBJ_HAS_MATRIX       LDOBJ_SETMATRIX (true)
#define LDOBJ_NO_MATRIX        LDOBJ_SETMATRIX (false)

class QListWidgetItem;
class LDSubfile;
class LDDocument;
class LDSharedVertex;

// =============================================================================
// LDObject
//
// Base class object for all object types. Each LDObject represents a single line
// in the LDraw code file. The virtual method getType returns an enumerator
// which is a token of the object's type. The object can be casted into
// sub-classes based on this enumerator.
// =============================================================================
class LDObject
{	PROPERTY (public,		bool,			Hidden,		BOOL_OPS,	STOCK_WRITE)
	PROPERTY (public,		bool,			Selected,	BOOL_OPS,	STOCK_WRITE)
	PROPERTY (public,		LDObject*,	Parent,		NO_OPS,		STOCK_WRITE)
	PROPERTY (public,		LDDocument*,		File,			NO_OPS,		STOCK_WRITE)
	PROPERTY	(private,	int32,		ID,			NUM_OPS,		STOCK_WRITE)
	PROPERTY (public,		int,			Color,		NUM_OPS,		CUSTOM_WRITE)

	public:
		// Object type codes. Codes are sorted in order of significance.
		enum Type
		{	Subfile,        // Object represents a sub-file reference
			Quad,           // Object represents a quadrilateral
			Triangle,       // Object represents a triangle
			Line,           // Object represents a line
			CondLine,        // Object represents a conditional line
			Vertex,         // Object is a vertex, LDForge extension object
			BFC,            // Object represents a BFC statement
			Overlay,        // Object contains meta-info about an overlay image.
			Comment,        // Object represents a comment
			Error,          // Object is the result of failed parsing
			Empty,          // Object represents an empty line
			Unidentified,   // Object is an uninitialized (SHOULD NEVER HAPPEN)
			NumTypes        // Amount of object types
		};

		LDObject();
		virtual ~LDObject();

		virtual LDObject* clone()
		{	return 0;   // Creates a new LDObject identical to this one.
		}
		long getIndex() const;                      // Index (i.e. line number) of this object
		virtual LDObject::Type getType() const;     // Type enumerator of this object
		const vertex& getVertex (int i) const;      // Get a vertex by index
		virtual str getTypeName() const;            // Type name of this object
		virtual bool hasMatrix() const;             // Does this object have a matrix and position? (see LDMatrixObject)
		virtual void invert();                      // Inverts this object (winding is reversed)
		virtual bool isColored() const;             // Is this object colored?
		virtual bool isScemantic() const;           // Does this object have meaning in the part model?
		virtual void move (vertex vect);            // Moves this object using the given vertex as a movement List
		LDObject* next() const;                     // Object after this in the current file
		LDObject* prev() const;                     // Object prior to this in the current file
		virtual str raw() {	return ""; }            // This object as LDraw code
		void replace (LDObject* other);             // Replace this LDObject with another LDObject. Object is deleted in the process.
		void select();
		void setVertex (int i, const vertex& vert); // Set a vertex to the given value
		void setVertexCoord (int i, Axis ax, double value); // Set a single coordinate of a vertex
		void swap (LDObject* other);                // Swap this object with another.
		LDObject* topLevelParent();                 // What object in the current file ultimately references this?
		void unselect();
		virtual int vertices() const;             // Number of vertices this object has

		static str typeName (LDObject::Type type); // Get type name by enumerator
		static LDObject* getDefault (const LDObject::Type type); // Returns a sample object by the given enumerator
		static void moveObjects (QList<LDObject*> objs, const bool up); // TODO: move this to LDDocument?
		static str describeObjects (const QList<LDObject*>& objs); // Get a description of a list of LDObjects
		static LDObject* fromID (int id);

		// TODO: make these private!
		// OpenGL list for this object
		uint glLists[4];

		// Object list entry for this object
		QListWidgetItem* qObjListEntry;

	protected:
		bool m_glinit;
		friend class GLRenderer;

	private:
		LDSharedVertex*	m_coords[4];
};

// =============================================================================
// LDSharedVertex
//
// For use as coordinates of LDObjects. Keeps count of references.
// -----------------------------------------------------------------------------
class LDSharedVertex
{	public:
		inline const vertex& data() const
		{	return m_data;
		}

		inline operator const vertex&() const
		{	return m_data;
		}

		void addRef (LDObject* a);
		void delRef (LDObject* a);

		static LDSharedVertex* getSharedVertex (const vertex& a);

	protected:
		LDSharedVertex (const vertex& a) : m_data (a) {}

	private:
		QList<LDObject*> m_refs;
		vertex m_data;
};

// =============================================================================
// LDMatrixObject
// =============================================================================
//
// Common code for objects with matrices. This class is multiple-derived in
// and thus not used directly other than as a common storage point for matrices
// and vertices.
//
// The link pointer is a pointer to this object's LDObject self - since this is
// multiple-derived in, static_cast or dynamic_cast won't budge here.
//
// In 0.1-alpha, there was a separate 'radial' type which had a position and
// matrix as well. Even though right now only LDSubfile uses this, I'm keeping
// this class distinct in case I get new extension ideas. :)
// =============================================================================
class LDMatrixObject
{	PROPERTY (public,	LDObject*,			LinkPointer,	NO_OPS,	STOCK_WRITE)
	PROPERTY (public,	matrix,				Transform,		NO_OPS,	CUSTOM_WRITE)

	public:
		LDMatrixObject() :
			m_Position (LDSharedVertex::getSharedVertex ({0, 0, 0})) {}

		LDMatrixObject (const matrix& transform, const vertex& pos) :
			m_Transform (transform),
			m_Position (LDSharedVertex::getSharedVertex (pos)) {}

		inline const vertex& getPosition() const
		{	return m_Position->data();
		}

		void setCoordinate (const Axis ax, double value)
		{	vertex v = getPosition();
			v[ax] = value;
			setPosition (v);
		}

		void setPosition (const vertex& a);

	private:
		LDSharedVertex*	m_Position;
};

// =============================================================================
// LDError
//
// Represents a line in the LDraw file that could not be properly parsed. It is
// represented by a (!) ERROR in the code view. It exists for the purpose of
// allowing garbage lines be debugged and corrected within LDForge. The member
// zContent contains the contents of the unparsable line.
// =============================================================================
class LDError : public LDObject
{	LDOBJ (Error)
	LDOBJ_NAME (error)
	LDOBJ_VERTICES (0)
	LDOBJ_UNCOLORED
	LDOBJ_SCEMANTIC
	LDOBJ_NO_MATRIX
	PROPERTY (public,	str, FileReferenced, STR_OPS,	STOCK_WRITE)

	public:
		LDError();
		LDError (str contents, str reason) : contents (contents), reason (reason) {}

		// Content of this unknown line
		str contents;

		// Why is this gibberish?
		str reason;
};

// =============================================================================
// LDEmpty
//
// Represents an empty line in the LDraw code file.
// =============================================================================
class LDEmpty : public LDObject
{	LDOBJ (Empty)
	LDOBJ_VERTICES (0)
	LDOBJ_UNCOLORED
	LDOBJ_NON_SCEMANTIC
	LDOBJ_NO_MATRIX
};

// =============================================================================
// LDComment
//
// Represents a code-0 comment in the LDraw code file. Member text contains
// the text of the comment.
// =============================================================================
class LDComment : public LDObject
{	LDOBJ (Comment)
	LDOBJ_NAME (comment)
	LDOBJ_VERTICES (0)
	LDOBJ_UNCOLORED
	LDOBJ_NON_SCEMANTIC
	LDOBJ_NO_MATRIX

	public:
		LDComment() {}
		LDComment (str text) : text (text) {}

		str text; // The text of this comment
};

// =============================================================================
// LDBFC
//
// Represents a 0 BFC statement in the LDraw code. eStatement contains the type
// of this statement.
// =============================================================================
class LDBFC : public LDObject
{	public:
		enum Type
		{	CertifyCCW,
			CCW,
			CertifyCW,
			CW,
			NoCertify,
			InvertNext,
			Clip,
			ClipCCW,
			ClipCW,
			NoClip,
			NumStatements
		};

		LDOBJ (BFC)
		LDOBJ_NAME (bfc)
		LDOBJ_VERTICES (0)
		LDOBJ_UNCOLORED
		LDOBJ_CUSTOM_SCEMANTIC { return (type == InvertNext); }
		LDOBJ_NO_MATRIX

	public:
		LDBFC() {}
		LDBFC (const LDBFC::Type type) : type (type) {}

		// Statement strings
		static const char* statements[];

		Type type;
};

// =============================================================================
// LDSubfile
//
// Represents a single code-1 subfile reference.
// =============================================================================
class LDSubfile : public LDObject, public LDMatrixObject
{	LDOBJ (Subfile)
	LDOBJ_NAME (subfile)
	LDOBJ_VERTICES (0)
	LDOBJ_COLORED
	LDOBJ_SCEMANTIC
	LDOBJ_HAS_MATRIX
	PROPERTY (public,	LDDocument*, FileInfo, NO_OPS,	STOCK_WRITE)

	public:
		enum InlineFlag
		{	DeepInline     = (1 << 0),
			CacheInline    = (1 << 1),
			RendererInline = (1 << 2),

			DeepCacheInline = DeepInline | CacheInline,
		};

		Q_DECLARE_FLAGS (InlineFlags, InlineFlag)

		LDSubfile()
		{	setLinkPointer (this);
		}

		// Inlines this subfile. Note that return type is an array of heap-allocated
		// LDObject-clones, they must be deleted one way or another.
		QList<LDObject*> inlineContents (InlineFlags flags);
};

Q_DECLARE_OPERATORS_FOR_FLAGS (LDSubfile::InlineFlags)

// =============================================================================
// LDLine
//
// Represents a single code-2 line in the LDraw code file. v0 and v1 are the end
// points of the line. The line is colored with dColor unless uncolored mode is
// set.
// =============================================================================
class LDLine : public LDObject
{	LDOBJ (Line)
	LDOBJ_NAME (line)
	LDOBJ_VERTICES (2)
	LDOBJ_COLORED
	LDOBJ_SCEMANTIC
	LDOBJ_NO_MATRIX

	public:
		LDLine() {}
		LDLine (vertex v1, vertex v2);
};

// =============================================================================
// LDCondLine
//
// Represents a single code-5 conditional line. The end-points v0 and v1 are
// inherited from LDLine, c0 and c1 are the control points of this line.
// =============================================================================
class LDCondLine : public LDLine
{	LDOBJ (CondLine)
	LDOBJ_NAME (condline)
	LDOBJ_VERTICES (4)
	LDOBJ_COLORED
	LDOBJ_SCEMANTIC
	LDOBJ_NO_MATRIX

	public:
		LDCondLine() {}
		LDLine* demote();
};

// =============================================================================
// LDTriangle
//
// Represents a single code-3 triangle in the LDraw code file. Vertices v0, v1
// and v2 contain the end-points of this triangle. dColor is the color the
// triangle is colored with.
// =============================================================================
class LDTriangle : public LDObject
{	LDOBJ (Triangle)
	LDOBJ_NAME (triangle)
	LDOBJ_VERTICES (3)
	LDOBJ_COLORED
	LDOBJ_SCEMANTIC
	LDOBJ_NO_MATRIX

	public:
		LDTriangle() {}
		LDTriangle (vertex v0, vertex v1, vertex v2)
		{	setVertex (0, v0);
			setVertex (1, v1);
			setVertex (2, v2);
		}
};

// =============================================================================
// LDQuad
//
// Represents a single code-4 quadrilateral. v0, v1, v2 and v3 are the end points
// of the quad, dColor is the color used for the quad.
// =============================================================================
class LDQuad : public LDObject
{	LDOBJ (Quad)
	LDOBJ_NAME (quad)
	LDOBJ_VERTICES (4)
	LDOBJ_COLORED
	LDOBJ_SCEMANTIC
	LDOBJ_NO_MATRIX

	public:
		LDQuad() {}
		LDQuad (const vertex& v0, const vertex& v1, const vertex& v2, const vertex& v3);

		// Split this quad into two triangles (note: heap-allocated)
		QList<LDTriangle*> splitToTriangles();
};

// =============================================================================
// LDVertex
//
// The vertex is an LDForce-specific extension which represents a single
// vertex which can be used as a parameter to tools or to store coordinates
// with. Vertices are a part authoring tool and they should not appear in
// finished parts.
// =============================================================================
class LDVertex : public LDObject
{	LDOBJ (Vertex)
	LDOBJ_NAME (vertex)
	LDOBJ_VERTICES (0) // TODO: move pos to vaCoords[0]
	LDOBJ_COLORED
	LDOBJ_NON_SCEMANTIC
	LDOBJ_NO_MATRIX

	public:
		LDVertex() {}

		vertex pos;
};

// =============================================================================
// LDOverlay
//
// Overlay image meta, stored in the header of parts so as to preserve overlay
// information.
// =============================================================================
class LDOverlay : public LDObject
{	LDOBJ (Overlay)
	LDOBJ_NAME (overlay)
	LDOBJ_VERTICES (0)
	LDOBJ_UNCOLORED
	LDOBJ_NON_SCEMANTIC
	LDOBJ_NO_MATRIX
	PROPERTY (public,	int, Camera,	NUM_OPS,	STOCK_WRITE)
	PROPERTY (public,	int, X,			NUM_OPS,	STOCK_WRITE)
	PROPERTY (public,	int, Y,			NUM_OPS,	STOCK_WRITE)
	PROPERTY (public,	int, Width,		NUM_OPS,	STOCK_WRITE)
	PROPERTY (public,	int, Height,	NUM_OPS,	STOCK_WRITE)
	PROPERTY (public,	str, FileName,	STR_OPS,	STOCK_WRITE)
};

// Other common LDraw stuff
static const str CALicense = "!LICENSE Redistributable under CCAL version 2.0 : see CAreadme.txt",
				 NonCALicense = "!LICENSE Not redistributable : see NonCAreadme.txt";
static const int lores = 16;
static const int hires = 48;

str getLicenseText (int id);

#endif // LDFORGE_LDTYPES_H
