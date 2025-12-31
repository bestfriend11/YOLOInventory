Attribute VB_Name = "WindowsAPI"
Option Explicit
Private Const NOERROR = 0&

Private Const SWP_NOMOVE = 2&
Private Const SWP_NOSIZE = 1&
Private Const FLAGS = SWP_NOMOVE Or SWP_NOSIZE
Private Const HWND_TOP = -1&
Private Const HWND_TOPMOST = -1&
Private Const HWND_NOTOPMOST = -2&

Private Const WM_DESTROY = &H2&
Private Const WM_CLOSE = &H10&
Private Const WM_QUIT = &H12&
Private Const INFINITE = &HFFFFFFFF
Private Const NORMAL_PRIORITY_CLASS = &H20&

Private Const GW_HWNDFIRST = 0&
Private Const GW_HWNDLAST = 1&
Private Const GW_HWNDNEXT = 2&
Private Const GW_HWNDPREV = 3&
Private Const GW_OWNER = 4&
Private Const GW_CHILD = 5&

Private WinText As String
Private WinHandle As Long
Private InstanceHandle As Long

Private Type STARTUPINFO
   cb As Long
   lpReserved As String
   lpDesktop As String
   lpTitle As String
   dwX As Long
   dwY As Long
   dwXSize As Long
   dwYSize As Long
   dwXCountChars As Long
   dwYCountChars As Long
   dwFillAttribute As Long
   dwFlags As Long
   wShowWindow As Integer
   cbReserved2 As Integer
   lpReserved2 As Long
   hStdInput As Long
   hStdOutput As Long
   hStdError As Long
End Type

Private Type PROCESS_INFORMATION
   hProcess As Long
   hThread As Long
   dwProcessID As Long
   dwThreadID As Long
End Type

Private Type GUID
    Data1 As Long
    Data2 As Integer
    Data3 As Integer
    Data4(7) As Byte
End Type

Private Declare Function SetWindowPos Lib "user32" _
      (ByVal hwnd As Long, _
      ByVal hWndInsertAfter As Long, _
      ByVal x As Long, _
      ByVal y As Long, _
      ByVal cx As Long, _
      ByVal cy As Long, _
      ByVal wFlags As Long) As Long
      
Private Declare Function GetWindowText Lib "user32" Alias "GetWindowTextA" _
    (ByVal hwnd As Long, ByVal lpString As String, _
    ByVal cch As Long) As Long
         
Public Declare Function SetFocus Lib "user32" _
    (ByVal hwnd As Long) As Long
Public Declare Function GetFocus Lib "user32" _
    () As Long

Private Declare Function GetDesktopWindow Lib "user32" _
    () As Long
    
Private Declare Function GetParent Lib "user32" _
    (ByVal hwnd As Long) As Long
    
Private Declare Function GetWindow Lib "user32" _
    (ByVal hwnd As Long, uCmd As Long) As Long
    
Private Declare Function IsWindow Lib "user32" _
    (ByVal hwnd As Long) As Long

Private Declare Function FindWindow Lib "user32" Alias "FindWindowA" _
    (ByVal lpszClass As String, _
    ByVal lpszWindow As String) As Long
      
Private Declare Function FindWindowEx Lib "user32" Alias "FindWindowExA" _
    (ByVal hWndParent As Long, _
    ByVal hwndChildAfter As Long, _
    ByVal lpszClass As String, _
    ByVal lpszWindow As String) As Long
        
Private Declare Function EnumChildWindows Lib "user32" _
    (ByVal hWndParent As Long, _
    ByVal lpEnumFunc As Long, _
    ByVal lparam As Long) As Long
   
Private Declare Function GetWindowThreadProcessId Lib "user32" _
    (ByVal hwnd As Long, ByVal lpdwprocessid As Long) As Long
  
Private Declare Function PostMessage Lib "user32" Alias "PostMessageA" _
    (ByVal hwnd As Long, _
    ByVal msg As Long, _
    ByVal wparam As Long, _
    ByVal lparam As Long) As Long
  
Private Declare Function WaitForSingleObject Lib "kernel32" _
   (ByVal hHandle As Long, _
   ByVal dwMilliseconds As Long) As Long


Private Declare Function CreateProcessA Lib "kernel32" Alias "CreateProcess" _
    (ByVal lpApplicationName As Long, _
    ByVal lpCommandLine As String, _
    ByVal lpProcessAttributes As Long, _
    ByVal lpThreadAttributes As Long, _
    ByVal bInheritHandles As Long, _
    ByVal dwCreationFlags As Long, _
    ByVal lpEnvironment As Long, _
    ByVal lpCurrentDirectory As Long, _
    lpStartupInfo As STARTUPINFO, _
    lpProcessInformation As _
    PROCESS_INFORMATION) As Long

Private Declare Function CloseHandle Lib "kernel32" _
   (ByVal hObject As Long) As Long

Private Declare Function GetExitCodeProcess Lib "kernel32" _
   (ByVal hProcess As Long, lpExitCode As Long) As Long

Private Declare Function GetLastError Lib "kernel32" _
    () As Long
      
Private Declare Function CLSIDFromString Lib "ole32.dll" _
    (ByVal lpszProgID As Long, pCLSID As GUID) As Long

    
  
      
Private Function ProcIDFromWnd(ByVal hwnd As Long) As Long
   Dim idProc As Long
   
   ' Get PID for this HWnd
   GetWindowThreadProcessId hwnd, idProc
   
   ' Return PID
   ProcIDFromWnd = idProc
   
End Function
            
      

Public Function SetTopMostWindow(hwnd As Long, Topmost As Boolean) As Long

   If Topmost = True Then 'Make the window topmost
      SetTopMostWindow = SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, FLAGS)
   Else
      SetTopMostWindow = SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, FLAGS)
      SetTopMostWindow = False
   End If
End Function


Function MatchTitleEnumProc(ByVal lhWnd As Long, ByVal lparam As Long) As Long

    Dim RetVal As Long
    Dim WinTitleBuf As String * 255

    RetVal = GetWindowText(lhWnd, WinTitleBuf, 255)
    
    If InStr(UCase(WinTitleBuf), UCase(WinText)) <> 0 Then
        WinHandle = lhWnd
        MatchTitleEnumProc = False
    Else
        MatchTitleEnumProc = True
    End If
    
End Function

Function FindWindowByTitle(title As String)

    Dim hwnd, ret, lparam As Long
    
    hwnd = GetDesktopWindow()
    
    WinText = title
    
    WinHandle = 0
    
    ret = EnumChildWindows(hwnd, AddressOf MatchTitleEnumProc, lparam)
    
    FindWindowByTitle = WinHandle
    
End Function


Function MatchHInstanceEnumProc(ByVal lhWnd As Long, ByVal lparam As Long) As Long

    Dim RetVal As Long
    Dim WinTitleBuf As String * 255

    RetVal = ProcIDFromWnd(lhWnd)
    
    If RetVal = InstanceHandle Then
        WinHandle = lhWnd
        MatchHInstanceEnumProc = False
    Else
        MatchHInstanceEnumProc = True
    End If
    
End Function
Public Function FindWindowByInstanceHandle(hInstance As Long) As Long
      
    Dim hwnd, ret, lparam As Long
    Dim WinTitleBuf As String * 255
   
    If hInstance = 0 Then
        FindWindowByInstanceHandle = 0
        Exit Function
    End If
    
    hwnd = GetDesktopWindow()
    
    InstanceHandle = hInstance
    
    WinHandle = 0
    
    ret = EnumChildWindows(hwnd, AddressOf MatchHInstanceEnumProc, lparam)
    
    FindWindowByInstanceHandle = WinHandle
    
    'ret = GetWindowText(WinHandle, WinTitleBuf, 255)
    'DoEvents
    
End Function


Function GetWinHandle(hInstance As Long) As Long
   Dim tempHwnd As Long
   
    If hInstance = 0 Then
        GetWinHandle = 0
        Exit Function
    End If
    
   ' Grab the desktop
   tempHwnd = GetDesktopWindow()
   
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

Public Sub SendCloseMessage(hwnd As Long)

    Dim ret As Long
    If IsWindow(hwnd) Then
        ret = PostMessage(hwnd, WM_CLOSE, 0&, 0&)
        If ret = 0 Then
            ret = GetLastError()
        End If
    End If
    
    
End Sub

Public Function ClassIDExists(clsstr As String) As Boolean

    Dim udtCLSID As GUID
    Dim lngRet As Long
    
    lngRet = CLSIDFromString(StrPtr(clsstr), udtCLSID)
      
    ClassIDExists = lngRet = NOERROR
    
End Function

