/****************************************************************************

    PROGRAM: LISPAPP.CPP

   PURPOSE: Demonstrates memory management with SmartHeap in the context of
        a C++ application.  This application's functionality is identical to
      the LISTAPP example, which is implemented in C rather than C++.

    FUNCTIONS:

    WinMain() - registers with SmartHeap, establishes SmartHeap safety level
    and error handler, processes message loop
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

      List, the C++ linked list class implementation, is in the file
      LIST.CPP.

      This application must be compiled in either the compact or large
      memory model, as must all C++ applications using the new and delete
      definitions in SMRTHEAP.HPP.

      Note that the error handling implements full recovery from memory
      errors without any tests of the return values of allocation functions.
      Whether the errors occur in this module, or in the LIST.CPP, the
      error handling is controlled completely by this application.

****************************************************************************/

/*** Include Files ***/
#include <windows.h>
#include <stdio.h>
#include "smrtheap.hpp"
#include "list.hpp"
#include "listapp.h"

/*** Types ***/
#ifndef WINVER
// for backward-compatiblity with Windows 3.0 WINDOWS.H
typedef WORD WPARAM;
typedef LONG LPARAM;
#endif

/*** Constants ***/
#define INIT_LIST_SIZE 8            /* list links to initially allocate */

/*** Global Variables ***/
HINSTANCE hInst;                    /* current instance                 */
HWND hMainWnd;                      /* handle of main window            */
HMENU hMenu;                        /* handle of menu                   */
char szAppName[] = "C++ List App";  /* application name                 */
MEM_POOL StringPool;                /* variable-size memory pool        */
List *lpList = NULL;                /* pointer to linked list structure */
char *Prompt;                       /* for passing dialog box text      */
char *dataBuffer;                   /* string allocated in GetValueProc */
   // note: the only reason dataBuffer is global is that there is no other
   // convenient mechanism for passing a 32-bit value from a dialog
   // procedure to routine that calls DialogBox.

/*** Function Prototypes ***/
BOOL InitApplication(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
long CALLBACK MainWndProc(HWND hWnd, unsigned message, WPARAM wParam,
   LPARAM lParam);
BOOL UpdateListbox(HWND, List *, int);
char *GetValue(HWND hWnd, char *prompt);
BOOL MEM_CALLBACK MyErrorHandler(MEM_ERROR_INFO far *errorInfo);
BOOL CALLBACK GetValueProc(HWND, unsigned, WORD, LONG);
int GetNumber(HWND hWnd, char *prompt);
BOOL CALLBACK GetNumberProc(HWND, unsigned, WORD, LONG);
BOOL CALLBACK AboutProc(HWND, unsigned, WORD, LONG);
void EnableMenuItems(List *);
void DisplayInt(char *, int);
// substitute for FARPROC, which can be mishandled in C++ apps (see MS Visual C++ KB
// ID: Q117428):
typedef int (CALLBACK *OURPROC)(struct HWND__ *,unsigned int,unsigned int,long);
/****************************************************************************

    FUNCTION: WinMain(HINSTANCE, HINSTANCE, LPSTR, int)

    PURPOSE: registers task with SmartHeap, calls initialization function,
        establishes error handler, and processes message loop

****************************************************************************/

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR, int nCmdShow)
{
   MSG msg;
   if (!hPrevInstance) /* Other instances of app running? */
        if (!InitApplication(hInstance)) /* Initialize shared things */
         return FALSE; /* Exits if unable to initialize */

    /* Perform initializations that apply to a specific instance */
    if (!InitInstance(hInstance, nCmdShow))
      return FALSE;

    /* Establish SmartHeap safety level for this task */
    dbgMemSetSafetyLevel(MEM_SAFETY_FULL);

    /* Create variable-size memory pool to contain string allocations. */
    if ((StringPool = MemPoolInit(FALSE)) == NULL)
        return FALSE;

    /* Establish SmartHeap error handler */
    MemSetErrorHandler(MyErrorHandler);

    /* Acquire and dispatch messages until a WM_QUIT message is received. */
    while (GetMessage(&msg, 0, 0, 0))
    {
       TranslateMessage(&msg);
       DispatchMessage(&msg);
    }

    /* detect any leakage generated while the program is running */
    dbgMemReportLeakage(NULL, 1, 1);
   
    return (int)msg.wParam;     /* Returns the value from PostQuitMessage */
}


/****************************************************************************

    FUNCTION: InitApplication(HINSTANCE)

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

BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASSEX  wc;

    /* Fill in window class structure with parameters that describe the     */
    /* main window.                                                         */
	wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW; /* Class style(s).                  */
    wc.lpfnWndProc = MainWndProc; /* Function to retrieve messages  */
                                        /* for windows of this class.       */
    wc.cbClsExtra = 0;                  /* No per-class extra data.         */
    wc.cbWndExtra = 0;                  /* No per-window extra data.        */
    wc.hInstance = hInstance;           /* Application that owns the class. */
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LIST));
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); 
    wc.lpszMenuName =  "ListAppMenu";
    wc.lpszClassName = szAppName;
	wc.hIconSm = NULL;

   /* Register the window class and return success/failure code. */
    return RegisterClassEx(&wc);
}

/****************************************************************************

    FUNCTION:  InitInstance(HINSTANCE , int, HANDLE)

    PURPOSE:  Saves instance handle and creates main window

    COMMENTS:

       This function is called at initialization time for every instance of 
       this application.  This function performs initialization tasks that
       cannot be shared by multiple instances.  

       In this case, we save the instance handle in a static variable and 
       create and display the main program window.  
        
****************************************************************************/

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    /* Save the instance handle in static variable, which will be used in  */
    /* many subsequence calls from this application to Windows.            */

   hInst = hInstance;

    /* Create a main window for this application instance.  */

   hMainWnd = CreateWindow(
      szAppName,                      /* See RegisterClass() call.        */
"List Management Demonstration (C++)",/* Text for window title bar.       */
      WS_OVERLAPPEDWINDOW,            /* Window style.                    */
      CW_USEDEFAULT,                  /* Default horizontal position.     */
      CW_USEDEFAULT,                  /* Default vertical position.       */
      CW_USEDEFAULT,                  /* Default width.                   */
      CW_USEDEFAULT,                  /* Default height.                  */
      0,                              /* Overlapped windows have no parent*/
      0,                              /* Use the window class menu.       */
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

    FUNCTION:  MyErrorHandler(MEM_ERROR_INFO FAR *)

    PURPOSE:  SmartHeap Error-Handling callback routine

    COMMENTS:

        This function is called by SmartHeap when a memory error occurs.
        If the MEM_DEBUG flag is on, the error-handler calls the default
        SmartHeap error handler, so that during development error messages
        are displayed.

        Next, if the error is an out of memory condition, the handler prompts
        the user to close one or more windows to free up some memory.  If the
        user indicates "Retry", the handler returns TRUE to indicate to
        SmartHeap that the allocation should be attempted again.


****************************************************************************/

BOOL MEM_CALLBACK MyErrorHandler(MEM_ERROR_INFO FAR *errorInfo)
{
#ifdef MEM_DEBUG
    /* during development, display SmartHeap message box so we can see what
       memory errors are occurring */
    if (MemDefaultErrorHandler(errorInfo))
        return TRUE;
#endif

   /* if out of memory, attempt to retry */
    if (errorInfo->errorCode == MEM_OUT_OF_MEMORY)
        if (MessageBox(hMainWnd,
            "Out of Memory.  Please close one or more windows.",
            szAppName, MB_RETRYCANCEL)
                == IDRETRY)
            return TRUE;

    return 0;
}


/****************************************************************************

    FUNCTION: MainWndProc(HWND, UINT, WPARAM, LPARAM)

    PURPOSE:  Processes messages

    MESSAGES:

    WM_CREATE       - create listbox child window on client window
    WM_SIZE         - change size of listbox to match that of client window
    WM_COMMAND      - application menu commands (calls to list manager)
    WM_DESTROY      - destroy window
    WM_COMPACTING   - shrink memory pools to free up space in global heap

****************************************************************************/

LONG CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   static HWND hWndList;
   int nCurSel;
   char *val;

   switch (message)
   {
        case WM_CREATE:
        {
            RECT rect;
            GetClientRect(hWnd, &rect);

            hWndList = CreateWindow("listbox", NULL, 
                WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL, 0, 0,
                rect.right, rect.bottom, hWnd, (HMENU)IDC_LISTBOX, hInst,NULL);

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
            if (rLb.bottom - rLb.top != int(HIWORD(lParam)))
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
                   lpList = new List(INIT_LIST_SIZE);
                   break;

                case IDM_FREE:
                   /* Destroy the list */
                   delete lpList;
                   lpList = NULL;
                   break;

                case IDM_LENGTH:
                   /* Determine the length of the list */
                   DisplayInt("The list contains %d links.",
                      (int)lpList->Length());
                   break;

                case IDM_REVERSE:
                   /* Reverse all links in the list */
                   lpList->Reverse();
                   nCurSel = (int)(lpList->Length() - nCurSel - 1);
                   break;

                case IDM_CHECK:
                   /* Validate all list entries; check for circularities */
                   MessageBox(hMainWnd,
                        lpList->Check()
                            ? "List is OK."
                            : "Error(s) detected in list.",
                        szAppName, MB_OK);
                   break;

                case IDM_INSERT:
                  if ((val = GetValue(hWndList,
                        "&Enter value to insert into list:")) != NULL)
                  {
                     /* Insert a new link after the current selection */
                     Link *lpPrev = lpList->Nth(nCurSel++);
                     lpList->Insert(lpPrev, val);
                  }
                  break;

               case IDM_APPEND:
                  if ((val = GetValue(hWndList,
                    "&Enter value to append to list:")) != NULL)
                  {
                     /* Add a new link at the end of the list */
                     lpList->Append(val);
                     nCurSel = (int)lpList->Length() - 1;
                  }
                  break;

               case IDM_PUSH:
                  if ((val = GetValue(hWndList,
                        "&Enter value to push onto list:")) != NULL)
                  {
                     /* Add a new link at the beginning of the list */
                     lpList->Push(val);
                     nCurSel = 0;
                  }
                  break;

               case IDM_POP:
                  /* Delete the first link from the list */
                  lpList->Pop();
                  if (nCurSel != 0)
                     nCurSel--;
                  break;

               case IDM_DELETE:
               {
                  /* Delete the currently selected link from the list */
                  lpList->Delete(lpList->Nth(nCurSel));
                  break;
               }

               case IDM_NTH:
               {
                  /* Find the nth value of the list */
                  int nSel = GetNumber(hWndList,
               "&Enter the number of a list link\r(the first link is zero):");
                  if (nSel >= 0 && lpList->Nth(nSel))
                     nCurSel = nSel;
                  break;
               }

               case IDM_LAST:
                  /* Find the last value in the list */
                  if (lpList->Last())
                     nCurSel = (int)lpList->Length() - 1;
                  break;

               case IDM_FIND:
                  if ((val = GetValue(hWndList,
                        "&Enter value to find in list:")) != NULL)
                  {
                     /* Search for a value in the list */
                     if (lpList->Find(val))
                         nCurSel = (int)lpList->Position(val);
                  }
                  break;

               case IDM_POSITION:
                  if ((val = GetValue(hWndList,
                        "&Enter value to find position of:")) != NULL)
                  {
                     /* Find position of a value in the list */
                     int nSel = (int)lpList->Position(val);
                     if (nSel >= 0)
                     {
                         nCurSel = nSel;
                         DisplayInt("The value is at position %d.",
                             nCurSel);
                     }
                  }
                  break;

               case IDM_DELETE_VALUE:
                  if ((val = GetValue(hWndList,
                        "&Enter value to delete:")) != NULL)
                  {
                     /* Delete a value from the list */
                     lpList->DeleteValue(val);
                  }
                  break;

               case IDM_DELETE_OCCURRENCES:
                  if ((val = GetValue(hWndList,
                     "&Enter value to delete all occurrences of:")) !=NULL)
                  {
                     /* Delete all occurrences of a value from the list */
                     lpList->DeleteOccurrences(val);
                  }
                  break;

               case IDM_BAD_POOL:
                  /* Generate MEM_BAD_POOL error */
                  new ((MEM_POOL)(void near *)1) char;
                  break;

               case IDM_BAD_POINTER:
                  /* Generate MEM_BAD_POINTER error: */
                  delete (char *)(void near *)3;
                  break;

               case IDM_DOUBLE_FREE:
               {
                  /* Generate MEM_DOUBLE_FREE error: */
                  char *s = new (StringPool) char;
                  dbgMemPoolDeferFreeing(StringPool, TRUE);
                  delete s;
                  dbgMemPoolDeferFreeing(StringPool, FALSE);
                  delete s;
                  break;
               }

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
                  /* Lets Windows process it      */
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
                lpList->Shrink();
            break;

        case WM_DESTROY:
           delete lpList;
           lpList = NULL;
           delete dataBuffer;
           dataBuffer = NULL;
           PostQuitMessage(0);
           break;

        default:              /* Passes it on if unproccessed    */
           return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
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

BOOL UpdateListbox(HWND hWndList, List *, int nSel)
{
    Link *lpLink = lpList ? lpList->First() : NULL;

    SendMessage(hWndList, WM_SETREDRAW, 0, 0L);
    
    /* clear any items currently in listbox */
    SendMessage(hWndList, LB_RESETCONTENT, 0, 0L);

    /* add each item in the list to the listbox */
    while (lpLink)
    {
      if (SendMessage(hWndList, LB_ADDSTRING, 0, (DWORD)(lpLink->Value()))
                == LB_ERR)
            return FALSE;
        lpLink = lpLink->Next();
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

   Prompt = lpszPrompt;

   lpProc = MakeProcInstance((OURPROC)GetValueProc, hInst);
    BOOL bResult = DialogBox(hInst, "GetValue", hWnd, lpProc);
   FreeProcInstance(lpProc);

   return bResult ? dataBuffer : NULL;
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

BOOL CALLBACK GetValueProc(HWND hDlg, unsigned message, WORD wParam, LONG)
{
   BOOL bResult;

    switch (message)
    {
        case WM_INITDIALOG:        /* message: initialize dialog box */
         /* set the dialog's prompt */
            SetDlgItemText(hDlg, IDC_TEXT, Prompt);
            return TRUE;

        case WM_COMMAND:              /* message: received a command */
            if (wParam == IDOK)
            {
                // Allocate space for the new value from memory pool...
                int len = (int)SendDlgItemMessage(hDlg, IDC_EDIT,
                    WM_GETTEXTLENGTH, 0, 0L) + 1;
                if (dataBuffer)
                {
                    // if data buffer is too small for this value, expand it
                    if (MemSizePtr(dataBuffer) < (unsigned long)len)
                        dataBuffer = new (dataBuffer, 0) char[len];
                }
                else
                    dataBuffer = new (StringPool, MEM_ZEROINIT) char[len];

                // ... and copy the value from the edit control
                GetDlgItemText(hDlg, IDC_EDIT, dataBuffer, len);
                bResult = TRUE;
            }
            else if (wParam == IDCANCEL)
                bResult = FALSE;
            else
                break;

            EndDialog(hDlg, bResult); /* Exits the dialog box */
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

    Prompt = lpszPrompt;

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

BOOL CALLBACK GetNumberProc(HWND hDlg, unsigned message, WORD wParam, LONG)
{
   int nResult;

   switch (message)
   {
      case WM_INITDIALOG:        /* message: initialize dialog box */
         /* set the dialog's prompt */
         SetDlgItemText(hDlg, IDC_TEXT, Prompt);
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

void EnableMenuItems(List *)
{
   /* List Menu */
   EnableMenuItem(hMenu, IDM_CREATE, lpList ? MF_GRAYED : MF_ENABLED);
   EnableMenuItem(hMenu, IDM_FREE, lpList ? MF_ENABLED : MF_GRAYED);
   EnableMenuItem(hMenu, IDM_LENGTH, lpList ? MF_ENABLED : MF_GRAYED);
   EnableMenuItem(hMenu, IDM_REVERSE, lpList ? MF_ENABLED : MF_GRAYED);
   EnableMenuItem(hMenu, IDM_CHECK, lpList ? MF_ENABLED : MF_GRAYED);
   /* Link Menu */
   EnableMenuItem(hMenu, IDM_INSERT,
      lpList && lpList->First() ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_APPEND, lpList ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_PUSH, lpList ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_POP,
      lpList && lpList->First() ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DELETE,
      lpList && lpList->First() ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_NTH,
      lpList && lpList->First() ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_LAST,
      lpList && lpList->First() ? MF_ENABLED : MF_GRAYED);
    /* Value Menu */
    EnableMenuItem(hMenu, IDM_FIND,
      lpList && lpList->First() ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_POSITION,
      lpList && lpList->First() ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DELETE_VALUE,
      lpList && lpList->First() ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DELETE_OCCURRENCES,
      lpList && lpList->First() ? MF_ENABLED : MF_GRAYED);
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

BOOL CALLBACK AboutProc(HWND hDlg, unsigned message, WORD wParam, LONG)
{
   switch (message) 
   {
      case WM_INITDIALOG:        /* message: initialize dialog box */
         return (TRUE);

      case WM_COMMAND:              /* message: received a command */
         if (wParam == IDOK                /* "OK" box selected?      */
            || wParam == IDCANCEL)
         /* System menu close command? */
         {
            EndDialog(hDlg, TRUE);        /* Exits the dialog box        */
            return (TRUE);
         }
         break;
   }

   return FALSE;               /* Didn't process a message    */
}

/****************************************************************************

    FUNCTION: DisplayInt(LPSTR, int)

    PURPOSE:  Display a formatted message including one integer value

    COMMENTS:

    This function dynamically allocates a buffer of exactly the correct size
    rather than allocate a buffer of constant size on the stack, and risk
    scribbling on the stack if given a string too large.

****************************************************************************/

const MAX_INT_CHARS = 7;     /* sign, 5 digits, and NUL terminator */
void DisplayInt(LPSTR lpszFormat, int nVal)
{
    LPSTR lpBuf = new(StringPool) char[lstrlen(lpszFormat) + MAX_INT_CHARS];
    sprintf(lpBuf, lpszFormat, nVal);
    MessageBox(hMainWnd, lpBuf, szAppName, MB_OK);
    delete lpBuf;
}
