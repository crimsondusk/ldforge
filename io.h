#ifndef __IO_H__
#define __IO_H__

#include "common.h"
#include "ldtypes.h"
#include "str.h"

class OpenFile {
public:
	str zFileName, zTitle;
	vector<objPointer> objects;
};

// PROTOTYPES
OpenFile* IO_FindLoadedFile (str name);
OpenFile* IO_OpenLDrawFile (str path);
LDObject* ParseLine (str zLine);

extern vector<OpenFile*> g_LoadedFiles;

#endif