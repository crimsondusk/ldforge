#include "common.h"
#include "ldtypes.h"
#include "io.h"

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

str LDLine::getContents () {
	return str::mkfmt ("2 %d %f %f %f %f %f %f", dColor,
		vaCoords[0].x, vaCoords[0].y, vaCoords[0].z,
		vaCoords[1].x, vaCoords[1].y, vaCoords[1].z);
}

str LDTriangle::getContents () {
	return str::mkfmt ("3 %d %f %f %f %f %f %f %f %f %f", dColor,
		vaCoords[0].x, vaCoords[0].y, vaCoords[0].z,
		vaCoords[1].x, vaCoords[1].y, vaCoords[1].z,
		vaCoords[2].x, vaCoords[2].y, vaCoords[2].z);
}

str LDQuad::getContents () {
	// Oh, Jesus.
	return str::mkfmt ("4 %d %f %f %f %f %f %f %f %f %f %f %f %f", dColor,
		vaCoords[0].x, vaCoords[0].y, vaCoords[0].z,
		vaCoords[1].x, vaCoords[1].y, vaCoords[1].z,
		vaCoords[2].x, vaCoords[2].y, vaCoords[2].z,
		vaCoords[3].x, vaCoords[3].y, vaCoords[3].z);
}

str LDCondLine::getContents () {
	return str::mkfmt ("5 %d %f %f %f %f %f %f %f %f %f %f %f %f", dColor,
		vaCoords[0].x, vaCoords[0].y, vaCoords[0].z,
		vaCoords[1].x, vaCoords[1].y, vaCoords[1].z,
		vaControl[0].x, vaControl[0].y, vaControl[0].z,
		vaControl[1].x, vaControl[1].y, vaControl[1].z);
}

str LDGibberish::getContents () {
	return zContents;
}

str LDSubfile::getContents () {
	return str::mkfmt ("1 %d %f %f %f %f %f %f %f %f %f %f %f %f %s", dColor,
		vPosition.x, vPosition.y, vPosition.y,
		faMatrix[0], faMatrix[1], faMatrix[2],
		faMatrix[3], faMatrix[4], faMatrix[5],
		faMatrix[6], faMatrix[7], faMatrix[8],
		zFileName.chars());
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