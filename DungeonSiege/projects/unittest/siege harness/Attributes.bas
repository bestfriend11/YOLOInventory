Attribute VB_Name = "Attributes"
Option Explicit


Global Const ConnectionString = "driver={SQL Server};SERVER=markgrimsqlsvr;database=unittestdb;dsn=''"



'Build type paramaters

Global Const DEBUG_EXE = "DungeonSiegeD.exe"
Global Const RELEASE_EXE = "DungeonSiegeR.exe"

Global sCommandLineString As String


'Case Action Vars

Enum CaseAction
    TC_EDIT_CASE = 1
    TC_NEW_CASE = 2
End Enum

Global eCaseAction As CaseAction


Enum PathType
    PT_SKRIT_PATH = 1
    PT_EXE_PATH = 2
    PT_DB_PATH = 3
End Enum



Global bIsNameOk As Boolean
Global bIsDBPathOk As Boolean
Global bIsSkritPathOk As Boolean
Global iCaseCount As Integer

'User Structure
Type UserInfo
    UserName As String
    Alias As String
    ExePath As String
    SkritPath As String
    IsDebug As Boolean
    MachineName As String
    OS As String
    LastRunCaseList As String
    MinimizeMain As Boolean
    MinimizeLog As Boolean
End Type

'Global userinfo struct for current user
Global CurrentUserInfo As UserInfo



'Case structure
Type Caseinfo
    TestArea As String      'This is the field in the db known as 'Area'
    CaseName As String      'This is the field 'Name' in the db
    Map As String
    Skrit As String
    Arguments As String
    Description As String
    Expected As String
    Teleport As String      'The book mark on the map where the character starts
    Author As String        'This will be the actual Author name of whomever authored the case
    Index As Integer        'This will actually be used to store the index of the case's node in the tree
    CaseNum As Integer      'This is the id that is stored in the db
End Type
'Array to hold the case structures
Public CaseArray() As Caseinfo



'Win32 declares
Public Const GW_HWNDNEXT = 2
Public Const READYSTATE_COMPLETE = 4
Public Const TEST_AREA_COUNT = 12
Public Declare Function GetParent Lib "user32" (ByVal hwnd As Long) As Long
Public Declare Function GetWindow Lib "user32" (ByVal hwnd As Long, _
  ByVal wCmd As Long) As Long
Public Declare Function FindWindow Lib "user32" Alias "FindWindowA" _
  (ByVal lpClassName As String, ByVal lpWindowName As String) As Long
Public Declare Function GetWindowText Lib "user32" Alias "GetWindowTextA" _
  (ByVal hwnd As Long, ByVal lpString As String, ByVal cch As Long) As Long
Public Declare Function GetWindowThreadProcessId Lib "user32" _
  (ByVal hwnd As Long, lpdwprocessid As Long) As Long
  
  
  'getuserstuff
Public Declare Function GetUserName Lib "advapi32.dll" Alias _
"GetUserNameA" (ByVal lpBuffer As String, ByRef nSize As Long) As Long

Public Declare Function GetComputerName Lib "Kernel32.dll" Alias _
"GetComputerNameA" (ByVal lpBuffer As String, ByRef nSize As Long) As Long





Public Function WriteIni(sSkritPath As String, sDBPath As String, sUserName As String)
    
    Dim fso As Object, fil As Object
    
    Set fso = CreateObject("Scripting.FileSystemObject")
    Set fil = fso.CreateTextFile("UnitTest.ini", True)
    
    With fil
        .WriteLine ("SkritPath=" & sSkritPath)
        .WriteLine ("DBPath=" & sDBPath)
        .WriteLine ("UserName=" & sUserName)
        .Close
    End With
    
    ' Get rid of File system objects
    Set fil = Nothing
    Set fso = Nothing
    
End Function


Function ProcIDFromWnd(ByVal hwnd As Long) As Long
   Dim idproc As Long
   
   ' Get PID for this HWnd
   GetWindowThreadProcessId hwnd, idproc
   
   ' Return PID
   ProcIDFromWnd = idproc
End Function
      
Function GetWinHandle(hInstance As Long) As Long
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


Public Sub CheckIni()

On Error GoTo GetIniSettings


    Dim fso As New FileSystemObject
    Dim fil1 As File
    Dim oTextStream As TextStream
    Dim sGetIniString As String
    
    
    

    Set fil1 = fso.GetFile("UnitTest.ini")
    ' Read the contents of the file.
    Set oTextStream = fil1.OpenAsTextStream(ForReading)
    Do While Not oTextStream.AtEndOfStream
        sGetIniString = oTextStream.ReadLine
        MsgBox sGetIniString
        ParseIni sGetIniString

    Loop
    
    
    oTextStream.Close

Exit Sub


    
    
GetIniSettings:
    MsgBox "There was no .ini file found.  Please set correct Paths.", vbOKOnly, ".ini Missing"
    'frmSettings.Show vbModal

End Sub

Public Function ParseIni(sIniString As String) As String
    Dim iLeftMarker As Integer
    Dim iRightMarker As Integer
    Dim sIniCheck
    
    iLeftMarker = InStr(sIniString, "=")
    ParseIni = Mid(sIniString, iLeftMarker + 1)

    iRightMarker = InStr(sIniString, "=")
    sIniCheck = Left(sIniString, iRightMarker - 1)
    
    Select Case sIniCheck:
        Case "DBPath"
            sDBPath = ParseIni
        Case "ExePath"
            sExePath = ParseIni
        Case "UserName"
            sUserName = ParseIni
    End Select
    
End Function



