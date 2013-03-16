#include "pointer.h"

vector<objPointer*> g_pObjectPointers;

void objPointer::replacePointers (LDObject* old, LDObject* repl) {
	for (ulong i = 0; i < g_pObjectPointers.size(); ++i) {
		objPointer* ptrptr = g_pObjectPointers[i];
		
		if ((*ptrptr).ptr == old)
			(*ptrptr).ptr = repl;
	}
}

objPointer::~objPointer () {
	
}

objPointer::objPointer () {
	ptr = nullptr;
}

objPointer::objPointer (LDObject* _ptr) {
	ptr = _ptr;
}

void objPointer::serialize () {
	g_pObjectPointers.push_back (this);
}

void objPointer::unSerialize () {
	for (ulong i = 0; i < g_pObjectPointers.size(); ++i)
		if (g_pObjectPointers[i] == this)
			g_pObjectPointers.erase (g_pObjectPointers.begin() + i);
}

void objPointer::deleteObj (LDObject* obj) {
	replacePointers (obj, nullptr);
	delete obj;
}