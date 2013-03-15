#include "common.h"
#include "ldtypes.h"

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