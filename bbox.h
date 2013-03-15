#ifndef __BBOX_H__
#define __BBOX_H__

#include "common.h"

class bbox {
public:
	vertex v0;
	vertex v1;
	
	bbox ();
	void calculate ();
	
private:
	void checkVertex (vertex v);
};

#endif