/****************************************************************************

    PROGRAM: LISPAPP.C

    PURPOSE: Demonstrates memory management with SmartHeap in the context of
        an application.  This application's functionality is identical to
        the CPP example, which is implemented in C++ rather than C.

    FUNCTIONS:

    WinMain() - registers with SmartHeap, establishes SmartHeap safety level
    and error handler, establishes Catch environment, processes message loop
    InitApplication() - initializes window data and registers window
    InitInstance() - saves instance handle and creates main window
    MyErrorHandler() - handles memory errors
    MainWndProc() - processes messages for menu commands
    UpdateListbox() - refreshes listbox with values from linked list
    GetValue() - prompts for a string value
    GetNumber() - prompts for a numeric value
    EnableMenuItems() - grays/enables menus items depending on state of list
    DisplayInt() - dynamically allocates a buffer for display of a message
        

    COMMENTS:
        Shows SmartHeap task registration, safety levels, error handling,
        and memory allocation.  Both variable-size and fixed-size memory
        pools are used.

        This application uses LISTLIB.DLL, the sources of which are in the
        LISTLIB example sub-directory.

        Note that the error handling implements full recovery from memory
        errors without any tests of the return values of allocation functions.
        Whether the errors occur in this module, or in the LISTLIB.DLL, the
        error handling is controlled completely by this application.

****************************************************************************/

/*** Include Files ***/
#include <windows.h>
#include <setjmp.h>
#include "smrtheap.h"
#include "..\listlib\listlib.h"
#include "listapp.h"

/*** Types ***/
#ifndef WINVER
// for backward-compatiblity with Windows 3.0 WINDOWS.H
typedef WORD WPARAM;
typedef LONG LPARAM;
typedef LONG (FAR PASCAL *WNDPROC)();
#endif

/* application's list link structure
   the link structure used by the LISTLIB DLL must have a far pointer as
   its first field, which is used by the list manager to link the list
   elements together; any other fields that are required may be present
   following the next field */
typedef struct StringLinkTag
{
    struct StringLinkTag FAR *next; /* FAR ptr to next link in list     */
    LPSTR lpStr;
} StringLink;
typedef StringLink FAR *LPSTRINGLINK;

/*** Constants ***/
#define INIT_LIST_SIZE 8            /* list links to initially allocate */

/*** Global Variables ***/
HANDLE hInst;                       /* current instance                 */
HWND hMainWnd;                      /* handle of main window            */
HMENU hMenu;                        /* handle of menu                   */
char szAppName[] = "List App";      /* application name                 */
MEM_POOL StringPool;                /* variable-size memory pool        */
jmp_buf CatchBuf;                   /* catch environment buffer         */
LPLIST lpList = NULL;               /* pointer to linked list structure */
LPSTR lpszValue;                    /* string allocated in GetValueProc */
MEM_ERROR_FN lpPrevErrorHandler;    /* return value of MemSetErrorHandler*/

/*** Function Prototypes ***/
BOOL InitApplication(HANDLE hInstance);
BOOL InitInstance(HANDLE hInstance, int nCmdShow);
long FAR PASCAL MainWndProc(HWND hWnd, unsigned message, WPARAM wParam,
    LPARAM lParam);
BOOL MEM_CALLBACK MyErrorHandler(MEM_ERROR_INFO FAR *lpErrorInfo);
BOOL StringLinkCmp(LPSTRINGLINK, LPSTR);
void StringFree(LPSTRINGLINK, BOOL);
BOOL UpdateListbox(HWND, LPLIST, int);
LPSTR GetValue(HWND hWnd, LPSTR lpszPrompt);
BOOL CALLBACK GetValueProc(HWND, unsigned, WORD, LONG);
int GetNumber(HWND hWnd, LPSTR lpszPrompt);
BOOL CALLBACK GetNumberProc(HWND, unsigned, WORD, LONG);
BOOL CALLBACK AboutProc(HWND, unsigned, WORD, LONG);
void EnableMenuItems(LPLIST);
void DisplayInt(LPSTR, int);
typedef int (CALLBACK *OURPROC)(struct HWND__ *,unsigned int,unsigned int,long);
/****************************************************************************

    FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

    PURPOSE: registers task with SmartHeap, calls initialization function,
        establishes error handler, and processes message loop

****************************************************************************/

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
    int nCmdShow)
{
   MSG msg;

   (void)lpCmdLine;

   if (!hPrevInstance) /* Other instances of app running? */
        if (!InitApplication(hInstance)) /* Initialize shared things */
            return FALSE; /* Exits if unable to initialize */

   /* Perform initializations that apply to a specific instance */
   if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

   /* Establish SmartHeap safety level for this task */
   dbgMemSetSafetyLevel(MEM_SAFETY_FULL);

   /* Create variable-size memory pool to contain string allocations. */
   if ((StringPool = MemPoolInit(MEM_POOL_DEFAULT)) == NULL)
   {
       return FALSE;
   }

    /* Establish SmartHeap error handler */
    lpPrevErrorHandler = MemSetErrorHandler(MyErrorHandler);
    /* set up catch buffer; the Throw from MyErrorHandler
       will result in a non-zero return from Catch below */
    for (;;)
    {
        if (setjmp(CatchBuf) == 0)
            break;
        MessageBox(hMainWnd, "Operation aborted due to memory error.",
            szAppName, MB_OK);
    }

    /* detect any garbage generated while the program is running */
	 dbgMemReportLeakage(NULL, 1, 1);

    /* Acquire and dispatch messages until a WM_QUIT message is received. */
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }


    return msg.wParam;     /* Returns the value from PostQuitMessage */
}


/****************************************************************************

    FUNCTION: InitApplication(HANDLE)

    PURPOSE: Initializes window data and registers window class

    COMMENTS:

        This function is called at initialization time only if no other 
        instances of the application are running.  This function performs 
        initialization tasks that can be done once for any number of running 
        instances.  

        In this case, we initialize a window class by filling out a data
        structure of type WNDCLASS and calling the Windows RegisterClass() 
        function. Since all instances of this application use the same window 
        class, we only need to do this when the first instance is initialized.

****************************************************************************/

BOOL InitApplication(HANDLE hInstance)
{
    WNDCLASS  wc;

    /* Fill in window class structure with parameters that describe the     */
    /* main window.                                                         */

    wc.style = CS_HREDRAW | CS_VREDRAW; /* Class style(s).                  */
    wc.lpfnWndProc = MainWndProc;       /* Function to retrieve messages for*/
                                        /* windows of this class.           */
    wc.cbClsExtra = 0;                  /* No per-class extra data.         */
    wc.cbWndExtra = 0;                  /* No per-window extra data.        */
    wc.hInstance = hInstance;           /* Application that owns the class. */
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LIST));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH); 
    wc.lpszMenuName =  "ListAppMenu";
    wc.lpszClassName = szAppName;

    /* Register the window class and return success/failure code. */
    return RegisterClass(&wc);
}

/****************************************************************************

    FUNCTION:  InitInstance(HANDLE, int, HANDLE)

    PURPOSE:  Saves instance handle and creates main window

    COMMENTS:

        This function is called at initialization time for every instance of 
        this application.  This function performs initialization tasks that
        cannot be shared by multiple instances.  

        In this case, we save the instance handle in a static variable and 
        create and display the main program window.  
        
****************************************************************************/

BOOL InitInstance(HANDLE hInstance, int nCmdShow)
{
    /* Save the instance handle in static variable, which will be used in  */
    /* many subsequence calls from this application to Windows.            */

    hInst = hInstance;

    /* Create a main window for this application instance.  */

    hMainWnd = CreateWindow(
        szAppName,                      /* See RegisterClass() call.        */
        "List Management Demonstration",/* Text for window title bar.       */
        WS_OVERLAPPEDWINDOW,            /* Window style.                    */
        CW_USEDEFAULT,                  /* Default horizontal position.     */
        CW_USEDEFAULT,                  /* Default vertical position.       */
        CW_USEDEFAULT,                  /* Default width.                   */
        CW_USEDEFAULT,                  /* Default height.                  */
        NULL,                           /* Overlapped windows have no parent*/
        NULL,                           /* Use the window class menu.       */
        hInstance,                      /* This instance owns this window.  */
        NULL                            /* Pointer not needed.              */
    );

    /* If window could not be created, return "failure" */
    if (!hMainWnd)
        return (FALSE);

    hMenu = GetMenu(hMainWnd);
	 EnableMenuItems(NULL);

    /* Make the window visible; update its client area; and return "success"*/
    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);     /* Sends WM_PAINT message                 */

    return TRUE;                /* Returns the value from PostQuitMessage */
}


/****************************************************************************

    FUNCTION:  MyErrorHandler(MEM_ERROR_INFO)

    PURPOSE:  SmartHeap Error-Handling callback routine

    COMMENTS:

        This function is called by SmartHeap when a memory error occurs.
        The error-handler first checks to see if the error occurred in a
        memory pool belonging to this application (as opposed to a library,
        for example).  If the error is someone else's, the handler chains to
        the previous error handler.

        If the MEM_DEBUG flag is on, the error-handler calls the default
        SmartHeap error handler, so that during development error messages
		  are displayed.

        Next, if the error is an out of memory condition, the handler prompts
        the user to close one or more windows to free up some memory.  If the
        user indicates "Retry", the handler returns TRUE to indicate to
        SmartHeap that the allocation should be attempted again.

        Finally, if none of the above conditions resolved the error, the
        handler throws back to top level (to the Catch statement in WinMain)
        to prevent the function in which the error occurred from returning.
        The application will continue normally; the erroneous operation that
        was attempted will have been canceled.

****************************************************************************/

BOOL MEM_CALLBACK MyErrorHandler(MEM_ERROR_INFO FAR *lpErrorInfo)
{
    /* chain if this is not our error */
    if (lpErrorInfo->pool != StringPool
			 && (!lpList || lpErrorInfo->pool != lpList->linkPool)
          && lpPrevErrorHandler)
        return (*lpPrevErrorHandler)(lpErrorInfo);

#ifdef MEM_DEBUG
    /* during development, display SmartHeap message box so we can see what
       memory errors are occurring */
    if (MemDefaultErrorHandler(lpErrorInfo))
        return TRUE;
#endif

    /* if out of memory, attempt to retry */
    if (lpErrorInfo->errorCode == MEM_OUT_OF_MEMORY)
        if (MessageBox(hMainWnd,
            "Out of Memory.  Please close one or more windows.",
            szAppName, MB_RETRYCANCEL)
                == IDRETRY)
            return TRUE;

	 /* communicate to SmartHeap that error handler won't return */
	 MemErrorUnwind();
	 
    /* throw back to top-level (cancel) */
    longjmp(CatchBuf, 1);

    /* note: no value returned here, as the above Throw non-local exits */
}


/****************************************************************************

    FUNCTION: MainWndProc(HWND, unsigned, WPARAM, LPARAM)

    PURPOSE:  Processes messages

    MESSAGES:

    WM_CREATE       - create listbox child window on client window
    WM_SIZE         - change size of listbox to match that of client window
    WM_COMMAND      - application menu commands (calls to list manager)
    WM_DESTROY      - destroy window
    WM_COMPACTING   - shrink memory pools to free up space in global heap

****************************************************************************/

long FAR PASCAL MainWndProc(HWND hWnd, unsigned message, WPARAM wParam,
    LPARAM lParam)
{
    static HWND hWndList;
    static OURPROC lpCmpProc, lpFreeProc;
    int nCurSel;
    LPSTR val;

    switch (message)
    {
        case WM_CREATE:
        {
            RECT rect;
            GetClientRect(hWnd, &rect);

            hWndList = CreateWindow("listbox", NULL, 
                WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL, 0, 0,
                rect.right, rect.bottom, hWnd, (HMENU)IDC_LISTBOX, hInst,NULL);

            lpCmpProc = MakeProcInstance((OURPROC)StringLinkCmp, hInst);
            lpFreeProc = MakeProcInstance((OURPROC)StringFree, hInst);

            break;
        }

        case WM_SIZE:
        {
            RECT rMain, rLb;

            if (wParam != SIZEFULLSCREEN && wParam != SIZENORMAL)
                break;

            /* Adjust size of listbox to match client area of main window */
            MoveWindow(hWndList, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);

            /* Make sure main window is aligned with listbox (which will be
               the nearest multiple of the height of a text line). */
            GetWindowRect(hWndList, &rLb);
            if (rLb.bottom - rLb.top != (int)HIWORD(lParam))
            {
                GetWindowRect(hWnd, &rMain);
                MoveWindow(hWnd, rMain.left, rMain.top,
                    rLb.right + GetSystemMetrics(SM_CXFRAME) - rMain.left,
                    rLb.bottom + GetSystemMetrics(SM_CYFRAME) - rMain.top,
                    TRUE);
            }
            break;
        }

        case WM_SETFOCUS:
            SetFocus(hWndList);
            break;

        case WM_COMMAND:       /* message: command from application menu */

            if (wParam != IDC_LISTBOX)
                nCurSel = (int)SendMessage(hWndList, LB_GETCURSEL, 0, 0L);

            switch (wParam)
            {
                case IDM_CREATE:
                    /* Create a new list */
                    lpList = CreateList(sizeof(StringLink), INIT_LIST_SIZE,
                        (LPCMPFN)lpCmpProc, (LPFREEFN)lpFreeProc);
                    break;

                case IDM_FREE:
                    /* Destroy the list */
                    FreeList(lpList);
                    lpList = NULL;
                    break;

                case IDM_LENGTH:
                    /* Determine the length of the list */
                    DisplayInt("The list contains %d links.",
                        (int)ListLength(lpList));
                    break;

                case IDM_REVERSE:
                    /* Reverse all links in the list */
                    ReverseList(lpList);
                    nCurSel = (int)(ListLength(lpList) - nCurSel - 1);
                    break;

                case IDM_CHECK:
                    /* Validate all list entries; check for circularities */
                    MessageBox(hMainWnd,
                        CheckList(lpList)
                            ? "List is OK."
                            : "Error(s) detected in list.",
                        szAppName, MB_OK);
                    break;

                case IDM_INSERT:
                    if ((val = GetValue(hWndList,
                        "&Enter value to insert into list:")) != NULL)
                    {
                        /* Insert a new link after the current selection */
                        LPLINK lpPrev = NthLink(lpList, nCurSel++);
                        LPSTRINGLINK lpLink =
                            (LPSTRINGLINK)InsertLink(lpList, lpPrev);
                        lpLink->lpStr = val;
                    }
                    break;

                case IDM_APPEND:
                    if ((val = GetValue(hWndList,
                        "&Enter value to append to list:")) != NULL)
                    {
                        /* Add a new link at the end of the list */
                        LPSTRINGLINK lpLink =(LPSTRINGLINK)AppendLink(lpList);
                        lpLink->lpStr = val;
                        nCurSel = (int)ListLength(lpList) - 1;
                    }
                    break;

                case IDM_PUSH:
                    if ((val = GetValue(hWndList,
                        "&Enter value to push onto list:")) != NULL)
                    {
                        /* Add a new link at the beginning of the list */
                        LPSTRINGLINK lpLink = (LPSTRINGLINK)PushLink(lpList);
                        lpLink->lpStr = val;
                        nCurSel = 0;
                    }
                    break;

                case IDM_POP:
                    /* Delete the first link from the list */
                    PopLink(lpList);
                    if (nCurSel != 0)
                        nCurSel--;
                    break;

                case IDM_DELETE:
                {
                    /* Delete the currently selected link from the list */
                    LPLINK lpLink = NthLink(lpList, nCurSel);
                    DeleteLink(lpList, lpLink);
                    break;
                }

                case IDM_NTH:
                {
                    /* Find the nth value of the list */
                    int nSel = GetNumber(hWndList,
            "&Enter the number of a list link\r(the first link is zero):");
                    if (nSel >= 0 && NthLink(lpList, nSel))
                        nCurSel = nSel;
                    break;
                }

                case IDM_LAST:
                    /* Find the last value in the list */
                    if (LastLink(lpList))
                        nCurSel = (int)ListLength(lpList) - 1;
                    break;

                case IDM_FIND:
                    if ((val = GetValue(hWndList,
                        "&Enter value to find in list:")) != NULL)
                    {
                        /* Search for a value in the list */
                        if (FindValue(lpList, val))
                            nCurSel = (int)ValuePosition(lpList, val);
                        MemFreePtr(val);
                    }
                    break;

                case IDM_POSITION:
                    if ((val = GetValue(hWndList,
                        "&Enter value to find position of:")) != NULL)
                    {
                        /* Find position of a value in the list */
                        int nSel = (int)ValuePosition(lpList, val);
                        if (nSel >= 0)
                        {
                            nCurSel = nSel;
                            DisplayInt("The value is at position %d.",
                                nCurSel);
                        }
                        MemFreePtr(val);
                    }
                    break;

                case IDM_DELETE_VALUE:
                    if ((val = GetValue(hWndList,
                        "&Enter value to delete:")) != NULL)
                    {
                        /* Delete a value from the list */
                        DeleteValue(lpList, val);
                        MemFreePtr(val);
                    }
                    break;

                case IDM_DELETE_OCCURRENCES:
                    if ((val = GetValue(hWndList,
                        "&Enter value to delete all occurrences of:")) !=NULL)
                    {
                        /* Delete all occurrences of a value from the list */
                        DeleteOccurrences(lpList, val);
                        MemFreePtr(val);
                    }
                    break;

                case IDM_BAD_POOL:
                    /* Generate MEM_BAD_POOL error: 
                       this will throw to top level */
                    MemAllocFS((MEM_POOL)0);
                    break;

                case IDM_BAD_POINTER:
                    /* Generate MEM_BAD_POINTER error:
                       this will throw to top level */
                    MemFreePtr((LPVOID)0);
                    break;

                case IDM_DOUBLE_FREE:
                {
                    /* Generate MEM_DOUBLE_FREE error:
                       this will throw to top level */
                    LPVOID lpVoid = MemAllocPtr(StringPool, 1, 0);
						  dbgMemPoolDeferFreeing(StringPool, TRUE);
                    MemFreePtr(lpVoid);
						  dbgMemPoolDeferFreeing(StringPool, FALSE);
                    MemFreePtr(lpVoid);
                    break;
                }

                case IDM_ALLOC_ZERO:
                    /* Generate MEM_ALLOC_ZERO error:
                       this will throw to top level */
                    MemAllocPtr(StringPool, 0, 0);
                    break;

                case IDM_ABOUT:
                {
                    OURPROC lpProcAbout; /* pointer to the "About" function */

                    lpProcAbout = MakeProcInstance((OURPROC)AboutProc, hInst);

                    DialogBox(hInst, "AboutBox", hWnd, lpProcAbout);

                    FreeProcInstance(lpProcAbout);
                    break;
                }

                case IDM_EXIT:
                    DestroyWindow(hWnd);
                    return 0;
                
                default:
                    /* Lets Windows process it       */
                    return (DefWindowProc(hWnd, message, wParam, lParam));
            }
            if (wParam != IDC_LISTBOX)
            {
                UpdateListbox(hWndList, lpList, nCurSel);
                EnableMenuItems(lpList);
            }
            break;

        case WM_COMPACTING:
            /* release as much memory as possible if Windows is spending
               a lot of time compacting the global heap */
            MemPoolShrink(StringPool);
            if (lpList)
                MemPoolShrink(lpList->linkPool);
            break;

        case WM_DESTROY:
            if (lpList)
            {
                FreeList(lpList);
                lpList = NULL;
            }
            FreeProcInstance(lpCmpProc);
            FreeProcInstance(lpFreeProc);
            PostQuitMessage(0);
            break;

        default:              /* Passes it on if unproccessed    */
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

/****************************************************************************

    FUNCTION: StringLinkCmp(LPSTRINGLINK, LPVOID)

    PURPOSE:  String Link Comparison call-back function for list searching.

    COMMENTS:
        This function is called by the ListLib DLL when searching for list
        links given a value.  FindValue, ValuePosition, DeleteValue, and
        DeleteOccurrences implicitly call this comparison function.

****************************************************************************/

BOOL StringLinkCmp(LPSTRINGLINK lpLink, LPSTR lpszValue)
{
    return lstrcmp(lpLink->lpStr, lpszValue) == 0;
}

/****************************************************************************

    FUNCTION: StringFree(LPSTRINGLINK)

    PURPOSE:  String freeing call-back function.

    COMMENTS:
        This function is called by the ListLib DLL when freeing a link.
        Since the list manager doesn't know what kind of value is stored
        in the list, or where is was allocated from, it has to call back
        to the owner to deallocate.

****************************************************************************/

void StringFree(LPSTRINGLINK lpLink, BOOL bFreeAll)
{
    if (bFreeAll)
    {
        while (lpLink)
        {
            MemFreePtr(lpLink->lpStr);
            lpLink = lpLink->next;
        }
    }
    else
        MemFreePtr(lpLink->lpStr);
}

/****************************************************************************

    FUNCTION: UpdateListbox(HWND, LPLIST, int)

    PURPOSE:  Refreshes the listbox control with the current values in a 
        linked list.

    COMMENTS:
        This function is called from the MainWndProc whenever the linked list
        is modified.  It keeps the user interface in synch with the data
        structure.  It would be more efficient to change the listbox
        incrementally as the list is changed, rather than refreshing the
        entire listbox after each change to the list.  However, in the
        interest of keeping this example simple, all updates to the listbox
        are localized to this function.

****************************************************************************/

BOOL UpdateListbox(HWND hWndList, LPLIST lpList, int nSel)
{
    LPSTRINGLINK lpLink = lpList ? (LPSTRINGLINK)(lpList->lpHead) : NULL;

    SendMessage(hWndList, WM_SETREDRAW, 0, 0L);
    
    /* clear any items currently in listbox */
    SendMessage(hWndList, LB_RESETCONTENT, 0, 0L);

    /* add each item in the list to the listbox */
    while (lpLink)
    {
        if (SendMessage(hWndList, LB_ADDSTRING, 0, (DWORD)(lpLink->lpStr))
                == LB_ERR)
            return FALSE;
        lpLink = lpLink->next;
    }

    SendMessage(hWndList, WM_SETREDRAW, 1, 0L);
    if (SendMessage(hWndList, LB_SETCURSEL, nSel, 0L) == LB_ERR)
    {
        int nCount = (int)SendMessage(hWndList, LB_GETCOUNT, 0, 0L);
        if (nCount > 0)
            SendMessage(hWndList, LB_SETCURSEL, 
                nSel > nCount-1 ? nCount-1 : 0, 0L);
    }

    return TRUE;
}


/****************************************************************************

    FUNCTION: GetValue(HWND, LPSTR)

    PURPOSE:  Prompts with a dialog box for a string value.

****************************************************************************/

LPSTR GetValue(HWND hWnd, LPSTR lpszPrompt)
{
    OURPROC lpProc;

    lpszValue = lpszPrompt;

    lpProc = MakeProcInstance((OURPROC)GetValueProc, hInst);
    DialogBox(hInst, "GetValue", hWnd, lpProc);
    FreeProcInstance(lpProc);

    return lpszValue;
}

/****************************************************************************

    FUNCTION: GetValueProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "GetValue" dialog box

    MESSAGES:

    WM_INITDIALOG - initialize dialog box
    WM_COMMAND    - Input received

    COMMENTS:

    Initialization sets the dialog's caption and prompt.

****************************************************************************/

BOOL CALLBACK GetValueProc(HWND hDlg, unsigned message,
    WORD wParam, LONG lParam)
{
    (void)lParam;

    switch (message)
    {
        case WM_INITDIALOG:        /* message: initialize dialog box */
            /* set the dialog's prompt */
            SetDlgItemText(hDlg, IDC_TEXT, lpszValue);
            return TRUE;

        case WM_COMMAND:              /* message: received a command */
            if (wParam == IDOK)
            {
                /* Allocate space for the new value from memory pool... */
                int len = (int)SendDlgItemMessage(hDlg, IDC_EDIT,
                    WM_GETTEXTLENGTH, 0, 0L);
                if ((lpszValue = (LPSTR)MemAllocPtr(StringPool, len+1, 0))
                        != NULL)
                    /* ... and copy the value from the edit control */
                    GetDlgItemText(hDlg, IDC_EDIT, lpszValue, len+1);
            }
            else if (wParam == IDCANCEL)
                lpszValue = NULL;
            else
                break;

            EndDialog(hDlg, lpszValue != NULL); /* Exits the dialog box */
            return TRUE;
    }

    return FALSE;                 /* Didn't process a message    */
}

/****************************************************************************

    FUNCTION: GetNumber(HWND, LPSTR)

    PURPOSE:  Prompts with a dialog box for an integer value.

****************************************************************************/

int GetNumber(HWND hWnd, LPSTR lpszPrompt)
{
    OURPROC lpProc;
    int nResult;

    lpszValue = lpszPrompt;

    lpProc = MakeProcInstance((OURPROC)GetNumberProc, hInst);
    nResult = DialogBox(hInst, "GetValue", hWnd, lpProc);
    FreeProcInstance(lpProc);

    return nResult;
}

/****************************************************************************

    FUNCTION: GetNumberProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "GetNumber" dialog box

    MESSAGES:

    WM_INITDIALOG - initialize dialog box
    WM_COMMAND    - Input received

    COMMENTS:

    Initialization sets the dialog's caption and prompt.

****************************************************************************/

BOOL CALLBACK GetNumberProc(HWND hDlg, unsigned message,
    WORD wParam, LONG lParam)
{
    int nResult;

    (void)lParam;

    switch (message)
    {
        case WM_INITDIALOG:        /* message: initialize dialog box */
            /* set the dialog's prompt */
            SetDlgItemText(hDlg, IDC_TEXT, lpszValue);
            return TRUE;

        case WM_COMMAND:              /* message: received a command */
            if (wParam == IDOK)
            {
                BOOL bTranslated;

                nResult = GetDlgItemInt(hDlg, IDC_EDIT, &bTranslated, FALSE);
                if (!bTranslated)
                {
                    MessageBox(hDlg,
                        "Invalid input, please enter a positive number.",
                        szAppName, MB_OK);
                    return TRUE;
                }
            }
            else if (wParam == IDCANCEL)
                nResult = -1;
            else
                break;

            EndDialog(hDlg, nResult);   /* Exits the dialog box */
            return TRUE;
    }

    return FALSE;                 /* Didn't process a message    */
}

/****************************************************************************

    FUNCTION: EnableMenuItems(LPLIST)

    PURPOSE:  Grays/enables menu items depending on the state of linked list

****************************************************************************/

void EnableMenuItems(LPLIST lpList)
{
    /* List Menu */
    EnableMenuItem(hMenu, IDM_CREATE, lpList ? MF_GRAYED : MF_ENABLED);
    EnableMenuItem(hMenu, IDM_FREE, lpList ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_LENGTH, lpList ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_REVERSE, lpList ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_CHECK, lpList ? MF_ENABLED : MF_GRAYED);
    /* Link Menu */
    EnableMenuItem(hMenu, IDM_INSERT,
        lpList && lpList->lpHead ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_APPEND, lpList ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_PUSH, lpList ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_POP,
        lpList && lpList->lpHead ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DELETE,
        lpList && lpList->lpHead ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_NTH,
        lpList && lpList->lpHead ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_LAST,
        lpList && lpList->lpHead ? MF_ENABLED : MF_GRAYED);
    /* Value Menu */
    EnableMenuItem(hMenu, IDM_FIND,
        lpList && lpList->lpHead ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_POSITION,
        lpList && lpList->lpHead ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DELETE_VALUE,
        lpList && lpList->lpHead ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DELETE_OCCURRENCES,
        lpList && lpList->lpHead ? MF_ENABLED : MF_GRAYED);
    /* Error Menu */
#ifdef MEM_DEBUG   
    EnableMenuItem(hMenu, IDM_BAD_POOL, MF_ENABLED);
    EnableMenuItem(hMenu, IDM_BAD_POINTER, MF_ENABLED);
    EnableMenuItem(hMenu, IDM_DOUBLE_FREE, MF_ENABLED);
#endif
}

/****************************************************************************

    FUNCTION: AboutProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "About" dialog box

    MESSAGES:

    WM_INITDIALOG - initialize dialog box
    WM_COMMAND    - Input received

    COMMENTS:

    No initialization is needed for this particular dialog box, but TRUE
    must be returned to Windows.

    Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

BOOL CALLBACK AboutProc(HWND hDlg, unsigned message,
    WORD wParam, LONG lParam)
{
    (void)lParam;

    switch (message) {
    case WM_INITDIALOG:        /* message: initialize dialog box */
        return (TRUE);

    case WM_COMMAND:              /* message: received a command */
        if (wParam == IDOK                /* "OK" box selected?      */
                || wParam == IDCANCEL) {      /* System menu close command? */
        EndDialog(hDlg, TRUE);        /* Exits the dialog box        */
        return (TRUE);
        }
        break;
    }
    return (FALSE);               /* Didn't process a message    */
}

/****************************************************************************

    FUNCTION: DisplayInt(LPSTR, int)

    PURPOSE:  Display a formatted message including one integer value

    COMMENTS:

    This function dynamically allocates a buffer of exactly the correct size
    rather than allocate a buffer of constant size on the stack, and risk
    scribbling on the stack if given a string too large.
    Note that the result of MemAllocPtr need not be tested: the SmartHeap
    error-handler defined by MyErrorHandler above will throw to top level
    on error.

****************************************************************************/

#define MAX_INT_CHARS 7     /* sign, 5 digits, and NUL terminator */
void DisplayInt(LPSTR lpszFormat, int nVal)
{
    LPSTR lpBuf =
        MemAllocPtr(StringPool, lstrlen(lpszFormat) + MAX_INT_CHARS, 0);
    wsprintf(lpBuf, lpszFormat, nVal);
    MessageBox(hMainWnd, lpBuf, szAppName, MB_OK);
    MemFreePtr(lpBuf);
}
