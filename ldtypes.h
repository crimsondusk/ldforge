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

#ifndef __LDTYPES_H__
#define __LDTYPES_H__

#include "common.h"
#include "types.h"

#define IMPLEMENT_LDTYPE(N) \
	LD##N (); \
	virtual ~LD##N (); \
	virtual LDObjectType_e getType () const { \
		return OBJ_##N; \
	} \
	virtual str getContents (); \
	virtual LD##N* clone () { \
		return new LD##N (*this); \
	} \
	virtual void move (vertex vVector);

class QTreeWidgetItem;
class LDSubfile;

// =============================================================================
// LDObjectType_e
// 
// Object type codes. Codes are sorted in order of significance.
// =============================================================================
enum LDObjectType_e {
	OBJ_Subfile,		// Object represents a sub-file reference
	OBJ_Quad,			// Object represents a quadrilateral
	OBJ_Triangle,		// Object represents a triangle
	OBJ_Line,			// Object represents a line
	OBJ_CondLine,		// Object represents a conditional line
	OBJ_Vertex,			// Object is a vertex, LDForge extension object
	OBJ_BFC,			// Object represents a BFC statement
	OBJ_Comment,		// Object represents a comment
	OBJ_Gibberish,		// Object is the result of failed parsing
	OBJ_Empty,			// Object represents an empty line
	OBJ_Unidentified,	// Object is an uninitialized (SHOULD NEVER HAPPEN)
	NUM_ObjectTypes		// Amount of object types
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
	virtual ~LDObject ();
	
	// Index (i.e. line number) of this object
	long getIndex (OpenFile* pFile);
	
	// Color used by this object. Comments, gibberish and empty entries
	// do not use this field.
	short dColor;
	
	// OpenGL list for this object
	uint uGLList, uGLPickList;
	
	// Object this object was referenced from, if any
	LDSubfile* parent;
	
	// Type enumerator of this object
	virtual LDObjectType_e getType () const {
		return OBJ_Unidentified;
	};
	
	// A string that represents this line
	virtual str getContents () {
		return "";
	}
	
	// Creates a new LDObject identical to this one and returns a pointer to it.
	virtual LDObject* clone () {
		return new LDObject (*this);
	}
	
	// Sets data common to all objects.
	void commonInit ();
	
	// Replace this LDObject with another LDObject. This method deletes the
	// object and any pointers to it become invalid.
    void replace (LDObject* replacement);
	
	// Swap this object with another.
	void swap (LDObject* other);
	
	// Moves this object using the given vertex as a movement vector
	virtual void move (vertex vVector);
	
	// What subfile in the current file ultimately references this?
	LDSubfile* topLevelParent ();
	
	static void moveObjects (std::vector<LDObject*> objs, const bool bUp);
	static str objectListContents (std::vector<LDObject*>& objs);
	
	QTreeWidgetItem* qObjListEntry;
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
	LDComment (str zText) : zText (zText) { commonInit (); }
	
	str zText; // The text of this comment
};

// =============================================================================
// LDBFC
// 
// Represents a 0 BFC statement in the LDraw code. eStatement contains the type
// of this statement.
// =============================================================================
class LDBFC : public LDComment {
public:
	IMPLEMENT_LDTYPE (BFC)
	LDBFC (const int dType) : dStatement (dType) { commonInit (); }
	
	// Statement strings
	static const char* saStatements[];
	
	static str statementString (short dValue);
	short dStatement;
};


// -----------------------------------------------------------------------------
// Enumerator for LDBFC's dStatement
enum LDBFCType_e {
	BFC_CertifyCCW,
	BFC_CCW,
	BFC_CertifyCW,
	BFC_CW,
	BFC_NoCertify,	// Winding becomes disabled (0 BFC NOCERTIFY)
	BFC_InvertNext,	// Winding is inverted for next object (0 BFC INVERTNEXT)
	NUM_BFCStatements
};

// =============================================================================
// LDSubfile
//
// Represents a single code-1 subfile reference.
// =============================================================================
class LDSubfile : public LDObject {
public:
	IMPLEMENT_LDTYPE (Subfile)
	
	vertex vPosition; // Position of the subpart
	matrix mMatrix; // Transformation matrix for the subpart
	str zFileName; // Filename of the subpart
	OpenFile* pFile; // Pointer to opened file for this subfile. null if unopened.
	
	// Gets the inlined contents of this subfile.
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
	IMPLEMENT_LDTYPE (Line)
	LDLine (vertex v1, vertex v2);
	
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
	
	vertex vaCoords[4]; // End points + control points of this line
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
	
	vertex vaCoords[4];
	
	// Split this quad into two triangles
	vector<LDTriangle*> splitToTriangles ();
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

// Icons for these types
extern const char* g_saObjTypeIcons[];

#endif