#include "common.h"
#include "ldtypes.h"
#include "io.h"
#include "misc.h"

const char* g_saObjTypeNames[] = {
	"unidentified",
	"unknown",
	"empty",
	"comment",
	"subfile",
	"line",
	"triangle",
	"quadrilateral",
	"condline",
	"vector",
	"vertex",
};

// =============================================================================
// LDObject constructors
LDObject::LDObject () {
	commonInit ();
}

void LDObject::commonInit () {
	qObjListEntry = nullptr;
}

LDGibberish::LDGibberish () {
	commonInit ();
}

LDGibberish::LDGibberish (str _zContent, str _zReason) {
	zContents = _zContent;
	zReason = _zReason;
	
	commonInit ();
}

LDEmpty::LDEmpty () {
	commonInit ();
}

LDComment::LDComment () {
	commonInit ();
}

LDSubfile::LDSubfile () {
	commonInit ();
}

LDLine::LDLine () {
	commonInit ();
}

LDTriangle::LDTriangle () {
	commonInit ();
}

LDQuad::LDQuad () {
	commonInit ();
}

LDCondLine::LDCondLine () {
	commonInit ();
}

LDVector::LDVector () {
	commonInit ();
}

LDVertex::LDVertex () {
	commonInit ();
}

ulong LDObject::getIndex () {
	if (!g_CurrentFile)
		return -1u;
	
	// TODO: shouldn't rely on g_CurrentFile
	for (ulong i = 0; i < g_CurrentFile->objects.size(); ++i) {
		if (g_CurrentFile->objects[i] == this)
			return i;
	}
	
	return -1u;
}

// =============================================================================
str LDComment::getContents () {
	return str::mkfmt ("0 %s", zText.chars ());
}

str LDSubfile::getContents () {
	str val = str::mkfmt ("1 %d %s ", dColor, vPosition.getStringRep (false).chars ());
	
	for (short i = 0; i < 9; ++i)
		val.appendformat ("%s ", ftoa (faMatrix[i]).chars());
	
	val += zFileName;
	return val;
}

str LDLine::getContents () {
	str val = str::mkfmt ("2 %d", dColor);
	
	for (ushort i = 0; i < 2; ++i)
		val.appendformat (" %s", vaCoords[i].getStringRep (false).chars ());
	
	return val;
}

str LDTriangle::getContents () {
	str val = str::mkfmt ("3 %d", dColor);
	
	for (ushort i = 0; i < 3; ++i)
		val.appendformat (" %s", vaCoords[i].getStringRep (false).chars ());
	
	return val;
}

str LDQuad::getContents () {
	str val = str::mkfmt ("4 %d", dColor);
	
	for (ushort i = 0; i < 4; ++i)
		val.appendformat (" %s", vaCoords[i].getStringRep (false).chars ());
	
	return val;
}

str LDCondLine::getContents () {
	str val = str::mkfmt ("5 %d", dColor);
	
	// Add the coordinates of end points
	for (ushort i = 0; i < 2; ++i)
		val.appendformat (" %s", vaCoords[i].getStringRep (false).chars ());
	
	// Add the control points
	for (ushort i = 0; i < 2; ++i)
		val.appendformat (" %s", vaControl[i].getStringRep (false).chars ());
	
	return val;
}

str LDGibberish::getContents () {
	return zContents;
}

str LDVector::getContents () {
	return str::mkfmt ("0 !LDFORGE VECTOR"); // TODO
}

str LDVertex::getContents () {
	return "!LDFORGE VERTEX"; // TODO
}

str LDEmpty::getContents () {
	return str ();
}