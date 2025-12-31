#if !defined(INDEXLISTS_H)
/* ========================================================================
   Header File: facelists.h
   Declares: face index list 
   ======================================================================== */

#define INDEXLISTS_H

#include <list>

typedef std::list<unsigned short> IndexList;
typedef IndexList::iterator IndexListIter;
typedef IndexList::const_iterator IndexListConstIter;

#endif
