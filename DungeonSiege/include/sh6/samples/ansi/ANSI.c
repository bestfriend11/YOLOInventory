/* ansi.c Sample application using ANSI C API.
 * 
 * Purpose:
 *  To demonstrate use of the ANSI C functions (malloc, calloc, realloc, free)
 *  Several other SmartHeap functions are also demonstrated.
 *
 * Functions:
 *  DisplayMessage() - uses malloc to allocate buffer for sprintf
 *  SearchPool() - searches a memory pool for a character string
 *  AddEntry() - uses calloc to init and realloc to dynamically expand a table
 *
 * Comments:
 *  Note STDLIB.H is included rather than shmalloc.h for ANSI declarations.
 *  The latter is needed only for memory models other than large.
 *  WinMain below does not contain a message loop -- this application is
 *  intended only to show how use the ANSI memory API in the large memory
 *  model.  No windows are created, and no messages are processed.
 */

/*** Include Files ***/
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "smrtheap.h" /* Needed only for non-ANSI function declarations */

/*** Global Variables ***/
char **lpStringTable = NULL;
WORD wStringTableSize = 0;
char szAppName[] = "ANSI C Large Model";

/*** Constants ***/
#define INIT_TABLE_SIZE 4
#define MAX_INT_CHARS 10

/*** Forward References ***/
BOOL DisplayMessage(char *msg, int num);
BOOL AddEntry(char *lpNewEntry);
char *SearchPool(MEM_POOL pool, char *lpStr);

/* Example WinMain, showing task registration, and calling the other
   example functions in this application.  Note that no message loop
   is present -- all I/O is performed with simple message boxes. */
void main()
{
    /* register with SmartHeap */ 
    MemRegisterTask();

    /* DisplayMessage will dynamically allocate a buffer of just the
       right size for sprintf. */
    DisplayMessage("Buffer allocated with malloc", 1);

    /* Allocate several strings in default memory pool... */
    strcpy((char *)malloc(20), "umbrella");
    strcpy((char *)malloc(10), "foobar");  
    strcpy((char *)malloc(5), "tree");     

	 /* ... then use MemPoolWalk (in SearchPool) to search for the strings */
    if (!SearchPool(MemDefaultPool, "foo"))
        DisplayMessage("String \"foo\" not found in default memory pool", 2);
    if (SearchPool(MemDefaultPool, "foobar"))
        DisplayMessage("String \"foobar\" found in default memory pool", 3);

    /* Add several strings to a string table: AddEntry uses realloc to
       dynamically expand the string table as the number of entries added
       exceed its current size. */
    AddEntry("foo");
    AddEntry("bar");
    AddEntry("fairdinkum");
    AddEntry("true blue");
    AddEntry("flower");
    if (wStringTableSize > INIT_TABLE_SIZE)
        DisplayMessage("String table was expanded dynamically", 4);

    /* unregister with SmartHeap */ 
    MemUnregisterTask();
}

/* add an entry to a string table assumes large or compact model */
BOOL AddEntry(char *lpNewEntry)
{
    WORD i;

    if (!lpStringTable)
    {
        /* initialize table */
        if ((lpStringTable = (char **)calloc(sizeof(char *), 4)) == NULL)
            /* allocation failed */
            return FALSE;
        else
            wStringTableSize = INIT_TABLE_SIZE;
    }

    /* find an empty entry in table */
    for (;;)
    {
        /* search for an emtpy entry */
        for (i = 0; i < wStringTableSize; i++)
            if (lpStringTable[i] == NULL)
                break;

        if (i < wStringTableSize)
            break;
        else
            /* all entries in use: expand table */
        {
            char **lpNewStrTable = (char **)realloc(lpStringTable,
                sizeof(char *) * wStringTableSize * 2);
            if (lpNewStrTable)
            {
                lpStringTable = lpNewStrTable;
                memset(lpStringTable+wStringTableSize, 0,
                    sizeof(char *) * wStringTableSize);
                wStringTableSize *= 2;
            }
            else
                return FALSE;
        }
    }

    /* add the new entry to string table */
    lpStringTable[i] = lpNewEntry;

    return TRUE;
}

/* Search all blocks in a memory pool for
   the specified character string. Assumes
   large or compact model run-time library. */
char *SearchPool(MEM_POOL pool, char *lpStr)
{
    MEM_POOL_ENTRY poolEntry;
    unsigned len = strlen(lpStr) + 1;

    poolEntry.entry = NULL;
    while (MemPoolWalk(pool, &poolEntry) == MEM_POOL_OK)
    {
        if (poolEntry.isInUse
            && len <= poolEntry.size
            && memcmp(poolEntry.entry, lpStr, len) == 0)

        return (char *)poolEntry.entry;
    }

    return NULL;
}

/* dynamically allocates a buffer for sprintf, to avoid any possibility of
   "scribbling on the stack" if buffer were allocated in an automatic array */
BOOL DisplayMessage(char *msg, int num)
{
    /* allocate buffer in which to format message */
    char *buf = (char *)malloc(strlen(msg) + MAX_INT_CHARS + 13);

    /* return if allocation failed */
    if (buf == NULL)
        return FALSE;

    /* format and display the message */
    sprintf(buf, "Message #%d: %s.", num, msg);
    MessageBox(GetFocus(), buf, szAppName, MB_OK);

    /* free the buffer */
    free(buf);

    return TRUE;
}
