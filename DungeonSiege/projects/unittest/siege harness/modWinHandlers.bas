Attribute VB_Name = "modWinHandlers"
   Option Explicit

'Win32 declares
Public Const GW_HWNDNEXT = 2
Public Const GWL_ID = (-12)
Public Const GW_CHILD = 5
Public Const VER_PLATFORM_WIN32s = 0
Public Const VER_PLATFORM_WIN32_WINDOWS = 1
Public Const VER_PLATFORM_WIN32_NT = 2
Public Const GP_CORE_ERROR = "GPCore Error *"
Public Const GP_CORE_ASSERT = "GPCore Assertion *"
Public Const GP_CORE_PERF = "GPCore Perf *"
Public Const GP_CORE_UNIMP$ = "GPCore"   '   was "GPCore Unimplemented Code! Failure"  - but did not achieve  closing of dialog
Public Const GP_FATAL$ = "Fatal Exception"

Public Type OSVERSIONINFO
        dwOSVersionInfoSize As Long
        dwMajorVersion As Long
        dwMinorVersion As Long
        dwBuildNumber As Long
        dwPlatformId As Long
        szCSDVersion As String * 128      '  Maintenance string for PSS usage
End Type

' get user stuff, win32 Declares
Public Declare Function GetUserName Lib "advapi32.dll" Alias _
"GetUserNameA" (ByVal lpBuffer As String, ByRef nSize As Long) As Long

Public Declare Function GetComputerName Lib "Kernel32.dll" Alias _
"GetComputerNameA" (ByVal lpBuffer As String, ByRef nSize As Long) As Long

Public Declare Function GetVersion Lib "kernel32" () As Long

Public Declare Function GetVersionEx Lib "kernel32" Alias _
"GetVersionExA" (ByRef lpVersionInformation As OSVERSIONINFO) As Long

'' Actual Windows api declarations for handling stuff

Public Declare Function SetFocusAPI Lib "user32" Alias "SetForegroundWindow" _
    (ByVal hwnd As Long) As Long

Public Declare Function GetWindow Lib "user32" (ByVal hwnd As Long, _
    ByVal wCmd As Long) As Long

Public Declare Function GetDesktopWindow Lib "user32" () As Long
   
Public Declare Function GetWindowLW Lib "user32" Alias "GetWindowLongA" _
    (ByVal hwnd As Long, ByVal nIndex As Long) As Long
    
Declare Function GetParent Lib "user32" (ByVal hwnd As Long) As Long

Declare Function GetClassName Lib "user32" Alias "GetClassNameA" _
    (ByVal hwnd As Long, ByVal lpClassName As String, _
     ByVal nMaxCount As Long) As Long

Declare Function GetWindowText Lib "user32" Alias "GetWindowTextA" _
    (ByVal hwnd As Long, ByVal lpString As String, ByVal cch As Long) _
     As Long

Public Declare Function FindWindow Lib "user32" Alias "FindWindowA" _
  (ByVal lpClassName As String, ByVal lpWindowName As String) As Long


Public Declare Function GetWindowThreadProcessId Lib "user32" _
  (ByVal hwnd As Long, lpdwprocessid As Long) As Long




Function FindWindowLike(hWndArray() As Long, ByVal hWndStart As Long, WindowText As String, Classname As String, ID) As Long

    Dim hwnd As Long
    Dim r As Long
       
    ' Hold the level of recursion:
    Static level As Long
       
    ' Hold the number of matching windows:
    Static iFound As Long
            
      
       
    Dim sWindowText As String
    Dim sClassname As String
    Dim sID
    
    
    ' Initialize if necessary:
    If level = 0 Then
         iFound = 0
        ReDim hWndArray(0 To 0)
        
        If hWndStart = 0 Then hWndStart = GetDesktopWindow()
    End If
    ' Increase recursion counter:
    
    level = level + 1
    ' Get first child window:
    
    hwnd = GetWindow(hWndStart, GW_CHILD)
    Do Until hwnd = 0
        DoEvents ' Not necessary
        
        ' Search children by recursion:
        r = FindWindowLike(hWndArray(), hwnd, WindowText, Classname, ID)
        
        ' Get the window text and class name:
        sWindowText = Space(255)
        r = GetWindowText(hwnd, sWindowText, 255)
        sWindowText = Left(sWindowText, r)
        sClassname = Space(255)
        r = GetClassName(hwnd, sClassname, 255)
        sClassname = Left(sClassname, r)
            
        ' If window is a child get the ID:
        If GetParent(hwnd) <> 0 Then
            r = GetWindowLW(hwnd, GWL_ID)
            sID = CLng("&H" & Hex(r))
        Else
            sID = Null
        End If
        
        ' Check that window matches the search parameters:
        If modParse.MyLike(sWindowText, WindowText) And modParse.MyLike(sClassname, Classname) Then
            If IsNull(ID) Then
            ' If find a match, increment counter and add handle to array:
                iFound = iFound + 1
                ReDim Preserve hWndArray(0 To iFound)
                hWndArray(iFound) = hwnd
            ElseIf Not IsNull(sID) Then
                If CLng(sID) = CLng(ID) Then
                    ' If find a match increment counter and add handle to array:
                    iFound = iFound + 1
                    ReDim Preserve hWndArray(0 To iFound)
                    hWndArray(iFound) = hwnd
                End If
            End If
        
            Debug.Print "Window Found: "
            Debug.Print "  Window Text  : " & sWindowText
            Debug.Print "  Window Class : " & sClassname
            Debug.Print "  Window Handle: " & CStr(hwnd)
        End If
        ' Get next child window:
        hwnd = GetWindow(hwnd, GW_HWNDNEXT)
   Loop
   ' Decrement recursion counter:
   level = level - 1
   ' Return the number of windows found:
   FindWindowLike = iFound
End Function
Public Function ProcIDFromWnd(ByVal hwnd As Long) As Long
   Dim idproc As Long
   
   ' Get PID for this HWnd
   GetWindowThreadProcessId hwnd, idproc
   
   ' Return PID
   ProcIDFromWnd = idproc
End Function
      
Public Function GetWinHandle(hInstance As Long) As Long
   Dim tempHwnd As Long
   
   ' Grab the first window handle that Windows finds:
   tempHwnd = FindWindow(vbNullString, vbNullString)
   
   ' Loop until you find a match or there are no more window handles:
   Do Until tempHwnd = 0
      ' Check if no parent for this window
      If GetParent(tempHwnd) = 0 Then
         ' Check for PID match
         If hInstance = ProcIDFromWnd(tempHwnd) Then
            ' Return found handle
            GetWinHandle = tempHwnd
            ' Exit search loop
            Exit Do
         End If
      End If
   
      ' Get the next window handle
      tempHwnd = GetWindow(tempHwnd, GW_HWNDNEXT)
   Loop
End Function
Public Sub GetUserInfoFromSystem(ByVal OS As String, ByVal UserName As String, ByVal MachineName As String)

    Dim Version As String
    Dim lResultUser As Long
    Dim lResultMachine As Long
    Dim sMachineName As String * 40
    Dim lLengthUserName As Long
    Dim lLengthMachineName As Long
    Dim sUsername As String * 40
    Dim lResult As Long
    Dim TempVersionInfo As OSVERSIONINFO
    Dim sTempPlatformString As String
    
    

    

    

    
    'Get the current version info
    sTempPlatformString = ""
    TempVersionInfo.dwOSVersionInfoSize = Len(TempVersionInfo)
    lResult = 0
    lResult = GetVersionEx(TempVersionInfo)
    'Ok, I did this the insanely difficult way, but it works, so I'm leaving it.
    With TempVersionInfo
        Select Case .dwMajorVersion
            Case 3
                sTempPlatformString = "Windows NT 3.51"
            Case 4
                Select Case .dwMinorVersion
                    Case 0
                        Select Case .dwPlatformId
                            Case VER_PLATFORM_WIN32_NT
                                sTempPlatformString = "Windows NT 4.0"
                            Case VER_PLATFORM_WIN32_WINDOWS
                                sTempPlatformString = "Windows 95"
                        End Select
                    Case 10
                        sTempPlatformString = "Windows 98"
                    Case 90
                        sTempPlatformString = "Windows Me"
                        
                End Select
            Case 5
                Select Case .dwMinorVersion
                    Case 0
                        sTempPlatformString = "Windows 2000"
                    Case 1
                        sTempPlatformString = "Windows XP"
                End Select
            End Select
            
            sTempPlatformString = sTempPlatformString & ", Version " & .dwMajorVersion & "." & .dwMinorVersion & ", Build " & .dwBuildNumber & ", " & .szCSDVersion
            
        End With
    With CurrentUserInfo:
        
        
        'Get the User's name from the system and trim trailing spaces
        lLengthUserName = 40
        sUsername = Space$(lLengthUserName)
        lResult = GetUserName(sUsername, lLengthUserName)
        .UserName = Left$(sUsername, lLengthUserName - 1)
        
        
        
        'Get the Machine's name from the system and trim trailing spaces
        lLengthMachineName = 40
        sMachineName = Space$(lLengthMachineName)
        lResult = GetComputerName(sMachineName, lLengthMachineName)

        .MachineName = Left$(sMachineName, lLengthMachineName)
        .OS = sTempPlatformString
    End With
End Sub

Public Function FindAndKillPopUpWindows() As Boolean
    Dim hwndDesktop As Long
    Dim hwndChild As Long
    Dim hResult As Long
    Dim hWnds() As Long
    Dim sMsgBoxTitle$
    Dim sWindowtitle As String
    
    
    hwndDesktop = GetDesktopWindow()
      hResult = FindWindowLike(hWnds(), 0, GP_CORE_ERROR, "*", Null)
      If hResult = 1 Then

        
            Debug.Print "Found "; Hex(hWnds(1))
            SetFocusAPI hWnds(1)
            SendKeys Chr$(71), True
            
            If (Not CurrentUserInfo.IsDebug) Then
                SendKeys " ", True
            End If
      End If
        
        hResult = FindWindowLike(hWnds(), 0, GP_CORE_ASSERT, "*", Null)
        If hResult = 1 Then

        
            Debug.Print "Found "; Hex(hWnds(1))
            SetFocusAPI hWnds(1)
   '         SendKeys "%g", True
            SendKeys Chr$(71), True
            If (Not CurrentUserInfo.IsDebug) Then SendKeys " ", True
      End If
      
      hResult = FindWindowLike(hWnds(), 0, GP_CORE_PERF, "*", Null)
       If hResult = 1 Then

        
            Debug.Print "Found "; Hex(hWnds(1))
            SetFocusAPI hWnds(1)
            SendKeys Chr$(71), True
            If (Not CurrentUserInfo.IsDebug) Then SendKeys " ", True    '
      End If

      hResult = FindWindowLike(hWnds(), 0, GP_CORE_UNIMP, "*", Null)
       If hResult = 1 Then

        
            Debug.Print "Found "; Hex(hWnds(1))
            SetFocusAPI hWnds(1)
            SendKeys Chr$(71), True  ' 71 = 0x47 = Alt G
            If (Not CurrentUserInfo.IsDebug) Then SendKeys " ", True
      End If

    hResult = FindWindowLike(hWnds(), 0, GP_FATAL, "*", Null)
       If hResult = 1 Then

        
            Debug.Print "Found "; Hex(hWnds(1))
            SetFocusAPI hWnds(1)
            SendKeys Chr$(9), True  ' tab to nice abort key
            SendKeys Chr$(9), True
            SendKeys Chr$(13), True  ' close dialog
            If (Not CurrentUserInfo.IsDebug) Then SendKeys " ", True    '
      End If
    FindAndKillPopUpWindows = False
        
End Function
