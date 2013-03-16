#ifndef __POINTER_H__
#define __POINTER_H__

#include "ldtypes.h"
#include <vector>
using std::vector;

class objPointer;
extern vector<objPointer*> g_pObjectPointers;

#define POINTER_LDCAST(T) \
	operator T* () { \
		return static_cast<T*> (ptr); \
	}

class objPointer {
public:
	LDObject* ptr;
	
	objPointer ();
	objPointer (LDObject*);
	~objPointer ();
	
	void serialize ();
	void unSerialize ();
	static void replacePointers (LDObject* old, LDObject* repl);
	
	LDObject& operator* () {
		return *ptr;
	}
	
	LDObject* operator-> () {
		return ptr;
	}
	
	operator LDObject* () {
		return ptr;
	}
	
	POINTER_LDCAST (LDComment)
	POINTER_LDCAST (LDCondLine)
	POINTER_LDCAST (LDQuad)
	POINTER_LDCAST (LDTriangle)
	POINTER_LDCAST (LDSubfile)
	POINTER_LDCAST (LDGibberish)
	POINTER_LDCAST (LDVector)
	POINTER_LDCAST (LDVertex)
	POINTER_LDCAST (LDLine)
	
	LDObject* operator= (LDObject* repl) {
		ptr = repl;
		return ptr;
	}
};

#endif // __POINTER_H__
class LDLine;
