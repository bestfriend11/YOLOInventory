/* LISTLIB.C Generic Singly-Linked List Management Library.
 * 
 * Public Entry-Points:
 *  CreateList() - creates a new list by initializing a fixed-size memory pool
 *  InsertLink() - adds a new link to a list after a specified link
 *  AppendLink() - adds a new link to the end of a list
 *  PushLink() - adds a new link to the beginning of a list
 *  PopLink() - deletes a link from the beginning of a list
 *  DeleteLink() - deletes a specified link from a list
 *  DeleteValue() - searches for and deletes a value from a list
 *  DeleteOccurrences() - deletes all occurrences of a value from a list
 *  FindValue() - searches for a value in a list, returning its link
 *  ValuePosition() - searches for a value in a list, returning its position
 *  NthLink() - returns the nth link of a list
 *  LastLink() - returns the last link of a list
 *  ListLength() - returns the number of links in a list
 *  ReverseList() - reverses the links of a list
 *  FreeList() - deallocates all memory in use in a list
 *  CheckList() - validates each link of a list, and detects circularities  
 *
 * Comments:
 *  This DLL can either be used as a template for your own linked list
 *  implementation, or you can use this linked list manager unmodified
 *  in your application.  To use the DLL as is:
 *   1) Define your own Link structure whose first field is a far pointer
 *      reserved for the list manager (this is used internally as the 'next'
 *      field to link list elements together).
 *   2) Define a comparison function with prototype CmpFn(LPLINK, LPVOID),
 *      where the first argument is a pointer to your Link structure, and
 *      the second argument is a pointer to the type of value you will store
 *      in the list.
 *   3) Pass the size of your structure defined in 1) as the first argument,
 *      and the function defined in 2) as the second argument, to CreateList.
 *
 * See Also:
 *  The LISTAPP example, which is an application that uses this DLL.
 */

/*** Include Files ***/
#include "windows.h"
#include "smrtheap.h"
#include "listlib.h"

/*** Global Variables ***/
MEM_POOL ListPool = NULL;

/*** Forward References ***/
static LPLINK NEAR PASCAL findValueInternal(LPLIST lpList, LPVOID lpValue,
   LPLINK FAR *lplpPrev, LONG FAR *lpPos);
static BOOL NEAR PASCAL deleteLinkInternal(LPLIST lpList, LPLINK lpLink,
   LPLINK lpPrev);


/*** Function Definitions ***/

/* Initialize a fixed-size memory pool to store list links. */
LPLIST CreateList(int nLinkSize, int nInitialLinkCount,
   LPCMPFN lpCmpFn, LPFREEFN lpFreeFn)
{
   LPLIST lpList;

   if (!ListPool)
      /* initialize fixed-size memory pool to store list headers;
        * allocate as shareable so that one pool can be used for the headers
       * of all lists, regardless of which tasks allocates them */
      if ((ListPool = MemPoolInitFS(sizeof(struct List), 1, MEM_POOL_DEFAULT))
          == NULL)
         return NULL;

   if ((lpList = (LPLIST)MemAllocFS(ListPool)) == NULL)
      return NULL;

   if ((lpList->linkPool = 
      MemPoolInitFS((unsigned short)nLinkSize, nInitialLinkCount, 0)) == NULL)
   {
      MemFreeFS(lpList);
      return NULL;
   }

   lpList->lpCmpFn = lpCmpFn;
   lpList->lpFreeFn = lpFreeFn;
   lpList->lpHead = NULL;
   return lpList;
}

/* add a new link to a list after a specified link */
LPLINK InsertLink(LPLIST lpList, LPLINK lpPrev)
{
   /* allocate a new Link */
   LPLINK lpNew = (LPLINK)MemAllocFS(lpList->linkPool);

   /* return if allocation failed */
   if (lpNew == NULL)
      return NULL;

   /* initialize the new link */
   if (lpPrev)
   {
      lpNew->next = lpPrev->next;
      lpPrev->next = lpNew;
   }
   else
   {
      lpNew->next = lpList->lpHead;
      lpList->lpHead = lpNew;
   }

   return lpNew;
}

/* add a new link to the end of a list */
LPLINK AppendLink(LPLIST lpList)
{
   return InsertLink(lpList, LastLink(lpList));
}

/* add a new link to the beginning of a list */
LPLINK PushLink(LPLIST lpList)
{
   return InsertLink(lpList, NULL);
}

/* delete an element from a linked list */
BOOL DeleteLink(LPLIST lpList, LPLINK lpRemove)
{
   LPLINK lpPrev = NULL;
   LPLINK lpLink = lpList->lpHead;

   /* find previous link */
   while (lpLink && lpLink != lpRemove)
   {
      lpPrev = lpLink;
      lpLink = lpLink->next;
   }

   return deleteLinkInternal(lpList, lpLink, lpPrev);
}

/* find and delete a value from a linked list */
BOOL DeleteValue(LPLIST lpList, LPVOID lpValue)
{
   LPLINK lpPrev;
   LPLINK lpLink = findValueInternal(lpList, lpValue, &lpPrev, NULL);

   return deleteLinkInternal(lpList, lpLink, lpPrev);
}

/* find and delete all occurrences of a value from a linked list */
BOOL DeleteOccurrences(LPLIST lpList, LPVOID lpValue)
{
   LPLINK lpPrev;
   LPLINK lpLink;
   BOOL bFound = FALSE;

   while ((lpLink = findValueInternal(lpList, lpValue, &lpPrev, NULL))
         != NULL)
      bFound = deleteLinkInternal(lpList, lpLink, lpPrev);

   return bFound;
}

BOOL PopLink(LPLIST lpList)
{
   return deleteLinkInternal(lpList, lpList->lpHead, NULL);
}

/* delete a link from a linked list */
BOOL NEAR PASCAL deleteLinkInternal(LPLIST lpList,LPLINK lpLink,LPLINK lpPrev)
{
   if (lpLink)
   {
      /* unlink lpLink */
      if (lpPrev)
         lpPrev->next = lpLink->next;
      else
         lpList->lpHead = lpLink->next;

      /* call back to free the value */
      (*(lpList->lpFreeFn))(lpLink, FALSE);

      /* and free the deleted link */
      MemFreeFS(lpLink);
   }

   return lpLink != NULL;
}

/* find a value in a linked list */
LPLINK FindValue(LPLIST lpList, LPVOID lpValue)
{
   return findValueInternal(lpList, lpValue, NULL, NULL);
}

/* find a value in a linked list */
LONG ValuePosition(LPLIST lpList, LPVOID lpValue)
{
   LONG lPos;
   LPLINK lpLink = findValueInternal(lpList, lpValue, NULL, &lPos);

   if (lpLink)
      return lPos;
   else
      return -1L;
}

/* find a value in a linked list, also returning the link
   previous to that which contains the found value */
LPLINK NEAR PASCAL findValueInternal(LPLIST lpList, LPVOID lpValue,
   LPLINK FAR *lplpPrev, LONG FAR *lpPos)
{
   LPLINK lpPrev = NULL;
   LPLINK lpLink = lpList->lpHead;
   DWORD dwPos = 0;

   if (lpList->lpCmpFn == NULL)
      return NULL;

   /* search list for specified value */
   while (lpLink && !(*(lpList->lpCmpFn))(lpLink, lpValue))
   {
      lpPrev = lpLink;
      lpLink = lpLink->next;
      dwPos++;
   }

   if (lpLink)
   {
      if (lplpPrev)
         /* store previous value */
         *lplpPrev = lpPrev;

      if (lpPos)
         *lpPos = dwPos;
   }

   return lpLink;
}

/* return the last link of a list */
LPLINK LastLink(LPLIST lpList)
{
   LPLINK lpTail = lpList->lpHead;

   /* find the end of the list */
   while (lpTail && lpTail->next)
      lpTail = lpTail->next;

   return lpTail;
}

/* return the nth link of a list, where the head of the list is "zeroth" */
LPLINK NthLink(LPLIST lpList, DWORD dwN)
{
   LPLINK lpLink = lpList->lpHead;

   /* find the end of the list */
   while (lpLink && dwN--)
      lpLink = lpLink->next;

   return lpLink;
}

/* return the number of links in a list */
DWORD ListLength(LPLIST lpList)
{
   /* use the allocation count from the memory pool rather than compute */
   return MemPoolCount(lpList->linkPool);
}

/* reverse the links of a list */
BOOL ReverseList(LPLIST lpList)
{
   LPLINK lpLink = lpList->lpHead;
   LPLINK lpPrev = NULL;

   /* traverse list, reversing each link */
   while (lpLink)
   {
      LPLINK lpNext = lpLink->next;
      lpLink->next = lpPrev;
      lpPrev = lpLink;
      lpLink = lpNext;
   }

   /* assign the list head to (what was) the last link */
   lpList->lpHead = lpPrev;

   return TRUE;
}

/* free an entire list */ 
BOOL FreeList(LPLIST lpList)
{
   if (lpList)
   {
      /* call back to free the list values */
      (*(lpList->lpFreeFn))(lpList->lpHead, TRUE);

      /* Freeing the pool returns all memory in the list to Windows
         in one call: much faster and less error prone than freeing
         each link individually. */
      MemPoolFree(lpList->linkPool);

      /* free the list header */
      MemFreeFS(lpList);

      /* if this is was the only list in use, free the list pool */
      if (MemPoolCount(ListPool) == 0)
      {
         MemPoolFree(ListPool);
         ListPool = NULL;
      }

      return TRUE;
   }

   return FALSE;
}

/* check all links in a list for validity, and detect circularities */
BOOL CheckList(LPLIST lpList)
{
   LPLINK lpLink = lpList->lpHead;
   /* use allocation count to detect circularities in the list */
   DWORD dwMaxLinks = MemPoolCount(lpList->linkPool);

   /* traverse list, returning immediately on error */
   while (lpLink)
      if (MemCheckPtr(lpList->linkPool, lpLink) && dwMaxLinks--)
         lpLink = lpLink->next;
      else
         return FALSE;

   return TRUE;
}

/* LibMain - Initialize library */
BOOL FAR PASCAL LibMain(HANDLE hInstance, WORD wDataSegment,
   WORD wHeapSize, LPSTR CommandLine)
{
   (void)wDataSegment; (void)hInstance;
   (void)wHeapSize; (void)CommandLine;

   return TRUE;
}

/* WEP - Windows Exit Procedure */
int FAR PASCAL WEP(BOOL SystemExit)
{
   (void)SystemExit;
   return 1;
}
