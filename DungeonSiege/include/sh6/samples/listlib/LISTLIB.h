/* LIST.H Generic Singly-Linked List Management Library.
 *
 */

#include "smrtheap.h"

/* Generic list link structure.
   Any C structure with a far pointer 'next' field
   in the first position may be used with this DLL. */
struct Link 
{
	struct Link FAR *next;
	char value[1];
};
typedef struct Link FAR *LPLINK;

typedef BOOL (*LPCMPFN)(LPLINK lpLink, LPVOID lpValue);
typedef void (*LPFREEFN)(LPLINK lpLink, BOOL bFreeAll);

struct List
{
	MEM_POOL linkPool;
	LPLINK lpHead;
	LPCMPFN lpCmpFn;
	LPFREEFN lpFreeFn;
};
typedef struct List FAR *LPLIST;

LPLIST CreateList(int nLinkSize, int nInitialLinkCount,
	LPCMPFN lpCmpFn, LPFREEFN lpFreeFn);
LPLINK InsertLink(LPLIST lpList, LPLINK lpPrev);
LPLINK AppendLink(LPLIST lpList);
LPLINK PushLink(LPLIST lpList);
BOOL DeleteLink(LPLIST lpList, LPLINK lpRemove);
BOOL DeleteValue(LPLIST lpList, LPVOID lpValue);
BOOL DeleteOccurrences(LPLIST lpList, LPVOID lpValue);
BOOL PopLink(LPLIST lpList);
LPLINK FindValue(LPLIST lpList, LPVOID lpValue);
LONG ValuePosition(LPLIST lpList, LPVOID lpValue);
LPLINK LastLink(LPLIST lpList);
LPLINK NthLink(LPLIST lpList, DWORD dwN);
DWORD ListLength(LPLIST lpList);
BOOL ReverseList(LPLIST lpList);
BOOL FreeList(LPLIST lpList);
BOOL CheckList(LPLIST lpList);
