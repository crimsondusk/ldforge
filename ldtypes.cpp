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
	
}

LDGibberish::LDGibberish () {
	
}

LDGibberish::LDGibberish (str _zContent, str _zReason) {
	zContent = _zContent;
	zReason = _zReason;
}

LDEmpty::LDEmpty () {
	
}

LDComment::LDComment () {
	
}

LDSubfile::LDSubfile () {
	
}

LDLine::LDLine () {
	
}

LDTriangle::LDTriangle () {
	
}

LDQuad::LDQuad () {
	
}

LDCondLine::LDCondLine () {
	
}

LDVector::LDVector () {
	
}

LDVertex::LDVertex () {
	
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