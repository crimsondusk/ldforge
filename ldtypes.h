#ifndef __LDTYPES_H__
#define __LDTYPES_H__

#include "common.h"
#include "str.h"

#define IMPLEMENT_LDTYPE(N) \
	LD##N (); \
	virtual LDObjectType_e getType () const { \
		return OBJ_##N; \
	}

// =============================================================================
// LDObjectType_e
// 
// Object type codes
// =============================================================================
enum LDObjectType_e {
	OBJ_Unidentified,	// Object is uninitialized (LDObject)
	OBJ_Gibberish,		// Object is unknown (LDUnknown; bad code number)
	OBJ_Empty,			// Object represents an empty line (LDEmpty)
	OBJ_Comment,		// Object represents a comment (LDComment, code: 0)
	OBJ_Subfile,		// Object represents a sub-file reference (LDSubfile, code: 1)
	OBJ_Line,			// Object represents a line (LDLine, code: 2)
	OBJ_Triangle,		// Object represents a triangle (LDTriangle, code: 3)
	OBJ_Quad,			// Object represents a quadrilateral (LDQuad, code: 4)
	OBJ_CondLine,		// Object represents a conditional line (LDCondLine, code: 5)
	OBJ_Vector,			// Object is a vector, LDForge extension object (LDVector)
	OBJ_Vertex			// Object is a vertex, LDForge extension object (LDVertex)
};

// =============================================================================
// LDObject
// 
// Base class object for all LD* types. Each LDObject represents a single line
// in the LDraw code file. The virtual method getType returns an enumerator
// which is a token of the object's type. The object can be casted into
// sub-classes based on this enumerator.
// =============================================================================
class LDObject {
public:
	LDObject ();
	
	virtual LDObjectType_e getType () const {
		return OBJ_Unidentified;
	};
};

// =============================================================================
// LDGibberish
//
// Represents a line in the LDraw file that could not be properly parsed. It is
// represented by a (!) ERROR in the code view. It exists for the purpose of
// allowing garbage lines be debugged and corrected within LDForge. The member
// zContent contains the contents of the unparsable line.
// =============================================================================
class LDGibberish : public LDObject {
public:
	IMPLEMENT_LDTYPE (Gibberish)
	
	LDGibberish (str _zContent, str _zReason);
	
	// Content of this unknown line
	str zContent;
	
	// Why is this gibberish?
	str zReason;
};

// =============================================================================
// LDEmptyLine 
// 
// Represents an empty line in the LDraw code file.
// =============================================================================
class LDEmpty : public LDObject {
public:
	IMPLEMENT_LDTYPE (Empty)
};

// =============================================================================
// LDComment
//
// Represents a code-0 comment in the LDraw code file. Member zText contains
// the text of the comment.
// =============================================================================
class LDComment : public LDObject {
public:
	IMPLEMENT_LDTYPE (Comment)
	
	str zText; // The text of this comment
};

// =============================================================================
// LDSubfile
//
// Represents a single code-1 subfile reference.
// =============================================================================
class LDSubfile : public LDObject {
public:
	IMPLEMENT_LDTYPE (Subfile)
	
	short dColor; // Color used by the reference
	vertex vPosition; // Position of the subpart
	double faMatrix[9]; // Transformation matrix for the subpart
	str zFileName; // Filename of the subpart
};

// =============================================================================
// LDLine
//
// Represents a single code-2 line in the LDraw code file. v0 and v1 are the end
// points of the line. The line is colored with dColor unless uncolored mode is
// set.
// =============================================================================
class LDLine : public LDObject {
public:
	IMPLEMENT_LDTYPE (Line)
	
	short dColor; // Color of this line
	vertex vaCoords[2]; // End points of this line
};

// =============================================================================
// LDCondLine
//
// Represents a single code-5 conditional line. The end-points v0 and v1 are
// inherited from LDLine, c0 and c1 are the control points of this line.
// =============================================================================
class LDCondLine : public LDLine {
public:
	IMPLEMENT_LDTYPE (CondLine)
	
	vertex vaControl[2]; // Control points
};

// =============================================================================
// LDTriangle
//
// Represents a single code-3 triangle in the LDraw code file. Vertices v0, v1
// and v2 contain the end-points of this triangle. dColor is the color the
// triangle is colored with.
// =============================================================================
class LDTriangle : public LDObject {
public:
	IMPLEMENT_LDTYPE (Triangle)
	
	short dColor;
	vertex vaCoords[3];
	
	LDTriangle (vertex _v0, vertex _v1, vertex _v2) {
		vaCoords[0] = _v0;
		vaCoords[1] = _v1;
		vaCoords[2] = _v2;
	}
};

// =============================================================================
// LDQuad
//
// Represents a single code-4 quadrilateral. v0, v1, v2 and v3 are the end points
// of the quad, dColor is the color used for the quad.
// =============================================================================
class LDQuad : public LDObject {
public:
	IMPLEMENT_LDTYPE (Quad)
	
	short dColor;
	vertex vaCoords[4];
};

// =============================================================================
// LDVector
// 
// The vector is a LDForge-specific extension. It is essentially a line with an
// alternative definition. Instead of being defined with two vertices, the vector
// is defined with a single vertex, a length and a bearing. This makes it useful
// for dealing with lines positioned on arbitrary angles without fear of precision
// loss of vertex coordinates. A vector can be transformed into a line and vice
// versa. Vectors are not a part of official LDraw standard and should be
// converted to lines for finished parts.
// 
// TODO: should a conditional vector be considered?
// =============================================================================
class LDVector : public LDObject {
public:
	IMPLEMENT_LDTYPE (Vector)
	
	vertex vPos;
	bearing gAngle3D;
	ulong ulLength;
};

// =============================================================================
// LDVertex
// 
// The vertex is another LDForce-specific extension. It represents a single
// vertex which can be used as a parameter to tools or to store coordinates
// with. Vertices are a part authoring tool and they should not appear in
// finished parts.
// =============================================================================
class LDVertex : public LDObject {
public:
	IMPLEMENT_LDTYPE (Vertex)
	
	vertex vPosition;
};

// =============================================================================
// Object type names. Pass the return value of getType as the index to get a
// string representation of the object's type.
extern const char* g_saObjTypeNames[];

#endif