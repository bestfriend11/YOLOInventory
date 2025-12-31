/* LIST.CPP SmartHeap C++ Example: List Management Class.
 * 
 * Public Members:
 *  List() - creates a new list by initializing a fixed-size memory pool
 *  ~List() - deallocates all memory in use in a list
 *  Insert() - adds a new link to a list after a specified link
 *  Append() - adds a new link to the end of a list
 *  Push() - adds a new link to the beginning of a list
 *  Pop() - deletes a link from the beginning of a list
 *  Delete() - deletes a specified link from a list
 *  DeleteValue() - searches for and deletes a value from a list
 *  DeleteOccurrences() - deletes all occurrences of a value from a list
 *  Find() - searches for a value in a list, returning its link
 *  Position() - searches for a value in a list, returning its position
 *  First() - returns the first link of a list
 *  Last() - returns the last link of a list
 *  Nth() - returns the nth link of a list
 *  Length() - returns the number of links in a list
 *  Reverse() - reverses the links of a list
 *  Check() - validates each link of a list, and detects circularities   
 */

#include <windows.h>
#include <string.h>
#include "smrtheap.hpp"
#include "list.hpp"

/*** Link Member Functions ***/
// create a new list link: copy string value, and link into list
Link::Link(MEM_POOL dataPool, char const *val, Link *p, Link *n)
{
   value = new (dataPool) char[strlen(val)+1];
   strcpy(value, val);

   if ((prev = p) != NULL)
      prev->next = this;
   if ((next = n) != NULL)
      next->prev = this;
}

// destroy a link: deallocate value, and unlink from list
Link::~Link()
{
   delete value;

   if (prev)
      prev->next = next;
   if (next)
      next->prev = prev;
}

/*** List Member Functions ***/
// create a new list: allocate fixed-size memory pool for links,
// variable-size memory pool for strings, and clear head, tail and count
List::List(unsigned initialSize)
{
   linkPool = MemPoolInitFS(sizeof(Link), initialSize, FALSE);
   dataPool = MemPoolInit(FALSE);
   head = tail = NULL;
   count = 0;
}

// destroy a list: deallocate both the link and data pools
// no need to free individual links or values, as freeing the pool
// will return all memory to Windows
List::~List()
{
   MemPoolFree(linkPool);
   MemPoolFree(dataPool);
}

// insert a new link
Link *List::Insert(Link *prev, char *val)
{
   Link *next = prev ? prev->next : head;
   Link *l = new (linkPool) Link(dataPool, val, prev, next);

   if (!prev)
      head = l;
   if (!next)
      tail = l;
   count++;

   return l;
}

// remove a link; return FALSE if link NULL
BOOL List::Delete(Link *l)
{
   if (!l)
      return FALSE;

   if (head == l)
      head = l->next;
   if (tail == l)
      tail = l->prev;
   delete l;
   count--;

   return TRUE;
}

// find and delete all occurrences of a value from the list
BOOL List::DeleteOccurrences(char *val)
{
   BOOL result = FALSE;

   while (DeleteValue(val))
      result = TRUE;

   return result;
}

// find the ordinal position of a link with a given value
long List::Position(char *val) const
{
   long result;

   if (findValue(val, &result))
      return result;
   else
      return -1L;
}

// find the link at the given zero-based ordinal position
Link *List::Nth(long pos) const
{
   if (pos < 0 || pos > count)
      return NULL;

   // find closest known point of list to the desired index
   register long delta;
   register Link *l;

   if ((delta = (count - pos - 1)) < pos)
   {
      // pos is closer to tail
      l = tail;
      while (delta-- && l)
         l = l->prev;
   }
   else
   {
      // pos is closer to head
      delta = pos;
      l = head;
      while (delta-- && l)
         l = l->next;
   }

   return l;
}

// find the first link, and its zero-based ordinal position,
// with a given value
Link *List::findValue(char *val, long *pos) const
{
   register long i = 0;

   for (register Link *l = head;  l;  l = l->next, i++)
      if (strcmp(l->value, val) == 0)
         break;

   if (l && pos)
      *pos = i;

   return l;
}

// reverse the links of the list
BOOL List::Reverse()
{
   Link *link = head;

   // traverse list, reversing each link by swapping next and prev pointers
   while (link)
   {
      Link *next = link->next;
      link->next = link->prev;
      link->prev = next;
      link = next;
   }

   // swap head and tail pointers
   link = head;
   head = tail;
   tail = link;

   return TRUE;
}

// check all links in a list for validity, and detect circularities
BOOL List::Check() const
{
   Link *link = head;
   // use allocation count to detect circularities in the list
   long lMaxLinks = MemPoolCount(linkPool);

   // traverse list, returning immediately on error
   while (link)
      if (MemCheckPtr(linkPool, link) && lMaxLinks-- > 0)
         link = link->next;
      else
         return FALSE;

   return TRUE;
}

// returns excess memory in the list's data and link memory pools to Windows
DWORD List::Shrink()
{
   return MemPoolShrink(dataPool) + MemPoolShrink(linkPool);
}
