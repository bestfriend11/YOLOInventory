#ifndef ENUMERATOR_H
#define ENUMERATOR_H

//******************************************
//
// Some useful tools to enumerate references
//
//******************************************

#include "MAX.H" 

//******************************************
// A call back to look for a specific class
// of object in the in the list of ReferenceMakers
// of an object. This routine will return only the
// first RefMaker that matches, and it will only look
// one level 'up' in the dependancy tree
class LookForObjectByClassCB : public DependentEnumProc {

public:

	Object* d_found;
	Class_ID d_searchClass;
	
	int proc(ReferenceMaker *rmaker) {

		Object* obj =  (Object *)rmaker;

		Class_ID id = obj->ClassID();
		
		if (id == d_searchClass) {
			d_found = (Object *)rmaker;
			return 1;
		}

		return 0;

	}
};

#endif
