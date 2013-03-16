#include "pointer.h"

vector<objPointer*> g_pObjectPointers;

// #define POINTER_DEBUG

#ifdef POINTER_DEBUG
#include <stdio.h>
#define DEBUGMESSAGE printf
#else
#define DEBUGMESSAGE
#endif

void objPointer::replacePointers (LDObject* old, LDObject* repl) {
	DEBUGMESSAGE ("replacing %p with %p\n", old, repl);
	DEBUGMESSAGE ("%u objects in pointers\n", g_pObjectPointers.size());
	
	for (ulong i = 0; i < g_pObjectPointers.size(); ++i) {
		objPointer* ptrptr = g_pObjectPointers[i];
		DEBUGMESSAGE ("%lu. (%p <-> %p)\n", i, (*ptrptr).ptr, old);
		if ((*ptrptr).ptr == old) {
			DEBUGMESSAGE ("\treplacing... %p -> %p\n", (*ptrptr).ptr, repl);
			(*ptrptr).ptr = repl;
			
			DEBUGMESSAGE ("%lu. (%p <-> %p)\n", i, (*ptrptr).ptr, old);
		}
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
	DEBUGMESSAGE ("Adding %p to pointers (ptr = %p)... ", this, ptr);
	g_pObjectPointers.push_back (this);
	DEBUGMESSAGE ("%u objects in pointers\n", g_pObjectPointers.size());
}

void objPointer::unSerialize () {
	DEBUGMESSAGE ("Removing %p from pointers... ", this);
	for (ulong i = 0; i < g_pObjectPointers.size(); ++i)
		if (g_pObjectPointers[i] == this)
			g_pObjectPointers.erase (g_pObjectPointers.begin() + i);
	DEBUGMESSAGE ("%u objects in pointers\n", g_pObjectPointers.size());
}