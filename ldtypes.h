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

#ifndef LDTYPES_H
#define LDTYPES_H

#include "common.h"
#include "types.h"

#define IMPLEMENT_LDTYPE(T, NUMVERTS) \
	LD##T () {} \
	virtual ~LD##T () {} \
	virtual LDObject::Type getType () const { \
		return LDObject::T; \
	} \
	virtual str getContents (); \
	virtual LD##T* clone () { \
		return new LD##T (*this); \
	} \
	virtual void move (vertex vVector); \
	virtual short vertices () const { return NUMVERTS; } \

#define LDOBJ_SETCOLORED(V) virtual bool isColored () const { return V; }
#define LDOBJ_COLORED LDOBJ_SETCOLORED (true)
#define LDOBJ_UNCOLORED LDOBJ_SETCOLORED (false)

#define LDOBJ_CUSTOM_SCHEMANTIC virtual bool isSchemantic () const
#define LDOBJ_SCHEMANTIC LDOBJ_CUSTOM_SCHEMANTIC { return true; }
#define LDOBJ_NON_SCHEMANTIC LDOBJ_CUSTOM_SCHEMANTIC { return false; }

class QListWidgetItem;
class LDSubfile;

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
	// Object type codes. Codes are sorted in order of significance.
	enum Type {
		Subfile,		// Object represents a sub-file reference
		Radial,			// Object represents a generic radial
		Quad,			// Object represents a quadrilateral
		Triangle,		// Object represents a triangle
		Line,			// Object represents a line
		CondLine,		// Object represents a conditional line
		Vertex,			// Object is a vertex, LDForge extension object
		BFC,			// Object represents a BFC statement
		Comment,		// Object represents a comment
		Gibberish,		// Object is the result of failed parsing
		Empty,			// Object represents an empty line
		Unidentified,	// Object is an uninitialized (SHOULD NEVER HAPPEN)
		NumTypes		// Amount of object types
	};

	LDObject ();
	virtual ~LDObject ();
	
	// Index (i.e. line number) of this object
	long getIndex (OpenFile* pFile);
	
	// Color used by this object. Comments, gibberish and empty entries
	// do not use this field.
	short dColor;
	
	// OpenGL list for this object
	uint uGLList, uGLPickList;
	
	// Vertices of this object
	vertex vaCoords[4];
	
	// Object this object was referenced from, if any
	LDObject* parent;
	
	// Type enumerator of this object
	virtual LDObject::Type getType () const {
		return LDObject::Unidentified;
	};
	
	// A string that represents this line
	virtual str getContents () {
		return "";
	}
	
	// Creates a new LDObject identical to this one and returns a pointer to it.
	virtual LDObject* clone () {
		return new LDObject (*this);
	}
	
	// Replace this LDObject with another LDObject. This method deletes the
	// object and any pointers to it become invalid.
    void replace (LDObject* replacement);
	
	// Swap this object with another.
	void swap (LDObject* other);
	
	// Moves this object using the given vertex as a movement vector
	virtual void move (vertex vVector);
	
	// What object in the current file ultimately references this?
	LDObject* topLevelParent ();
	
	// Number of vertices this object has
	virtual short vertices () const { return 0; }
	
	// Is this object colored?
	virtual bool isColored () const { return false; }
	
	// Does this object have meaning in the part model?
	virtual bool isSchemantic () const { return false; }
	
	// Returns a sample object by the given value
	static LDObject* getDefault (const LDObject::Type type);
	
	static void moveObjects (std::vector<LDObject*> objs, const bool bUp);
	static str objectListContents (const std::vector<LDObject*>& objs);
	
	// Object list entry for this object
	QListWidgetItem* qObjListEntry;
	
	uint32 groups () const { return m_groups; }
	void setGroups (const uint32 groups) { m_groups = groups; }
	
private:
	uint32 m_groups;
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
	IMPLEMENT_LDTYPE (Gibberish, 0)
	LDOBJ_UNCOLORED
	LDOBJ_SCHEMANTIC
	
	LDGibberish (str _zContent, str _zReason);
	
	// Content of this unknown line
	str zContents;
	
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
	IMPLEMENT_LDTYPE (Empty, 0)
	LDOBJ_UNCOLORED
	LDOBJ_NON_SCHEMANTIC
};

// =============================================================================
// LDComment
//
// Represents a code-0 comment in the LDraw code file. Member zText contains
// the text of the comment.
// =============================================================================
class LDComment : public LDObject {
public:
	IMPLEMENT_LDTYPE (Comment, 0)
	LDOBJ_UNCOLORED
	LDOBJ_NON_SCHEMANTIC
	
	LDComment (str zText) : text (zText) {}
	
	str text; // The text of this comment
};

// =============================================================================
// LDBFC
// 
// Represents a 0 BFC statement in the LDraw code. eStatement contains the type
// of this statement.
// =============================================================================
class LDBFC : public LDComment {
public:
	enum Type {
		CertifyCCW,
		CCW,
		CertifyCW,
		CW,
		NoCertify,	// Winding becomes disabled (0 BFC NOCERTIFY)
		InvertNext,	// Winding is inverted for next object (0 BFC INVERTNEXT)
		NumStatements
	};
	
	IMPLEMENT_LDTYPE (BFC, 0)
	LDOBJ_UNCOLORED
	LDOBJ_CUSTOM_SCHEMANTIC { return (type == InvertNext); }
	
	LDBFC (const LDBFC::Type eType) : type (eType) {}
	
	// Statement strings
	static const char* statements[];
	
	Type type;
};

// =============================================================================
// LDSubfile
//
// Represents a single code-1 subfile reference.
// =============================================================================
class LDSubfile : public LDObject {
public:
	IMPLEMENT_LDTYPE (Subfile, 0)
	LDOBJ_COLORED
	LDOBJ_SCHEMANTIC
	
	vertex vPosition; // Position of the subpart (FIXME: should get rid of this)
	matrix<3> mMatrix; // Transformation matrix for the subpart
	str zFileName; // Filename of the subpart
	OpenFile* pFile; // Pointer to opened file for this subfile. null if unopened.
	
	// Inlines this subfile. Note that return type is an array of heap-allocated
	// LDObject-clones, they must be deleted one way or another.
	std::vector<LDObject*> inlineContents (bool bDeepInline, bool bCache);
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
	IMPLEMENT_LDTYPE (Line, 2)
	LDOBJ_COLORED
	LDOBJ_SCHEMANTIC
	
	LDLine (vertex v1, vertex v2);
};

// =============================================================================
// LDCondLine
//
// Represents a single code-5 conditional line. The end-points v0 and v1 are
// inherited from LDLine, c0 and c1 are the control points of this line.
// =============================================================================
class LDCondLine : public LDLine {
public:
	IMPLEMENT_LDTYPE (CondLine, 4)
	LDOBJ_COLORED
	LDOBJ_SCHEMANTIC
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
	IMPLEMENT_LDTYPE (Triangle, 3)
	LDOBJ_COLORED
	LDOBJ_SCHEMANTIC
	
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
	IMPLEMENT_LDTYPE (Quad, 4)
	LDOBJ_COLORED
	LDOBJ_SCHEMANTIC
	
	// Split this quad into two triangles (note: heap-allocated)
	vector<LDTriangle*> splitToTriangles ();
};

// =============================================================================
// LDVertex
// 
// The vertex is an LDForce-specific extension which represents a single
// vertex which can be used as a parameter to tools or to store coordinates
// with. Vertices are a part authoring tool and they should not appear in
// finished parts.
// =============================================================================
class LDVertex : public LDObject {
public:
	IMPLEMENT_LDTYPE (Vertex, 0) // TODO: move vPosition to vaCoords[0]
	LDOBJ_COLORED
	LDOBJ_NON_SCHEMANTIC
	
	vertex vPosition;
};

// =============================================================================
// LDRadial
// 
// The generic radial primitive (radial for short) is another LDforge-specific
// extension which represents an arbitrary circular primitive. Radials can appear
// as circles, cylinders, rings, cones, discs and disc negatives; the point is to
// allow part authors to add radial primitives to parts without much hassle about
// non-existant primitive parts.
// =============================================================================
class LDRadial : public LDObject {
public:
	enum Type {
		Circle,
		Cylinder,
		Disc,
		DiscNeg,
		Ring,
		Cone,
		NumTypes
	};
	
	IMPLEMENT_LDTYPE (Radial, 0)
	LDOBJ_COLORED
	LDOBJ_SCHEMANTIC
	
	LDRadial::Type eRadialType;
	vertex vPosition;
	matrix<3> mMatrix;
	short dDivisions, dSegments, dRingNum;
	
	LDRadial (LDRadial::Type eRadialType, vertex vPosition, matrix<3> mMatrix,
		short dDivisions, short dSegments, short dRingNum) :
		eRadialType (eRadialType), vPosition (vPosition), mMatrix (mMatrix),
		dDivisions (dDivisions), dSegments (dSegments), dRingNum (dRingNum) {}
	
	// Returns a set of objects that provide the equivalent of this radial.
	// Note: objects are heap-allocated.
	std::vector<LDObject*> decompose (bool bTransform);
	
	// Compose a file name for this radial.
	str makeFileName ();
	
	char const* radialTypeName ();
	static char const* radialTypeName (const LDRadial::Type eType);
};

// =============================================================================
// Object type names. Pass the return value of getType as the index to get a
// string representation of the object's type.
extern const char* g_saObjTypeNames[];

// Icons for these types
extern const char* g_saObjTypeIcons[];

#endif // LDTYPES_H