Attribute VB_Name = "Module1"
Option Explicit

Private Const DRIVE_CDROM = 5
Private Const DRIVE_FIXED = 3
Private Const DRIVE_RAMDISK = 6
Private Const DRIVE_REMOTE = 4
Private Const DRIVE_REMOVABLE = 2
' Reg Key Security Options...
Private Const KEY_ALL_ACCESS = &H2003F
                                          
Private Const STANDARD_RIGHTS_REQUIRED = &HF0000
Private Const SYNCHRONIZE = &H100000
Private Const MUTANT_QUERY_STATE = &H1
Private Const MUTEX_ALL_ACCESS = STANDARD_RIGHTS_REQUIRED Or SYNCHRONIZE Or MUTANT_QUERY_STATE

' Reg Key ROOT Types...
Public Const HKEY_LOCAL_MACHINE& = &H80000002
Public Const HKEY_CLASSES_ROOT = &H80000000

Private Const ERROR_SUCCESS& = 0&
Private Const REG_SZ& = 1&                         ' Unicode nul terminated string
Private Const REG_DWORD& = 4&                      ' 32-bit number

Public Const CF_TEXT = 1

Public Const sAstrskMsg$ = "***"
Public Const sAbortMsg$ = sAstrskMsg & " Aborted by User " & sAstrskMsg

Public sIniFile$
Public sCurToken$
Public sMyStr$

Private Type SECURITY_ATTRIBUTES
        nLength As Long
        lpSecurityDescriptor As Long
        bInheritHandle As Long
End Type

Const WRITE_SKRIT = False
Public Declare Function WinExec Lib "kernel32" (ByVal lpCmdLine As String, ByVal nCmdShow As Long) As Long
Public Declare Function GetPrivateProfileSection Lib "kernel32" Alias "GetPrivateProfileSectionA" (ByVal lpAppName As String, ByVal lpReturnedString As String, ByVal nSize As Long, ByVal lpFileName As String) As Long
Public Declare Function GetPrivateProfileString Lib "kernel32" Alias "GetPrivateProfileStringA" (ByVal lpApplicationName As String, ByVal lpKeyName As Any, ByVal lpDefault As String, ByVal lpReturnedString As String, ByVal nSize As Long, ByVal lpFileName As String) As Long
Public Declare Function WritePrivateProfileString Lib "kernel32" Alias "WritePrivateProfileStringA" (ByVal lpApplicationName As String, ByVal lpKeyName As Any, ByVal lpString As Any, ByVal lpFileName As String) As Long
Public Declare Function GetDriveType Lib "kernel32" Alias "GetDriveTypeA" (ByVal nDrive As String) As Long
Public Declare Function CreateMutex Lib "kernel32" Alias "CreateMutexA" (lpMutexAttributes As SECURITY_ATTRIBUTES, ByVal bInitialOwner As Long, ByVal lpName As String) As Long
Public Declare Function ReleaseMutex Lib "kernel32" (ByVal hMutex As Long) As Long
Public Declare Function CloseHandle Lib "kernel32" (ByVal hObject As Long) As Long
Public Declare Function OpenMutex Lib "kernel32" Alias "OpenMutexA" (ByVal dwDesiredAccess As Long, ByVal bInheritHandle As Long, ByVal lpName As String) As Long

#Const STAND_ALONE = True ' set to false for incorporating into other apps
#Const READ_REG = False

Private Declare Function RegOpenKeyEx Lib "advapi32" Alias "RegOpenKeyExA" (ByVal hKey As Long, ByVal lpSubKey As String, ByVal ulOptions As Long, ByVal samDesired As Long, ByRef phkResult As Long) As Long
Private Declare Function RegQueryValueEx Lib "advapi32" Alias "RegQueryValueExA" (ByVal hKey As Long, ByVal lpValueName As String, ByVal lpReserved As Long, ByRef lpType As Long, ByVal lpData As String, ByRef lpcbData As Long) As Long
Private Declare Function RegCloseKey Lib "advapi32" (ByVal hKey As Long) As Long
Private Declare Function GetModuleFileName Lib "kernel32" Alias "GetModuleFileNameA" (ByVal hModule As Long, ByVal lpFileName As String, ByVal nSize As Long) As Long

Public Function GetKeyValue(KeyRoot As Long, KeyName As String, SubKeyRef As String, ByRef KeyVal As String) As Boolean
        Dim i As Long                                           ' Loop Counter
        Dim rc As Long                                          ' Return Code
        Dim hKey As Long                                        ' Handle To An Open Registry Key
        Dim hDepth As Long                                      '
        Dim KeyValType As Long                                  ' Data Type Of A Registry Key
        Dim tmpVal As String                                    ' Tempory Storage For A Registry Key Value
        Dim KeyValSize As Long                                  ' Size Of Registry Key Variable
        '------------------------------------------------------------
        ' Open RegKey Under KeyRoot {HKEY_LOCAL_MACHINE...}
        '------------------------------------------------------------
        rc = RegOpenKeyEx(KeyRoot, KeyName, 0, KEY_ALL_ACCESS, hKey) ' Open Registry Key
        
        If (rc <> ERROR_SUCCESS) Then GoTo GetKeyError          ' Handle Error...
        
        tmpVal = String$(1024, 0)                             ' Allocate Variable Space
        KeyValSize = 1024                                       ' Mark Variable Size

        '------------------------------------------------------------
        ' Retrieve Registry Key Value...
        '------------------------------------------------------------
        rc = RegQueryValueEx(hKey, SubKeyRef, 0, KeyValType, tmpVal, KeyValSize)    ' Get/Create Key Value
                                                

        If (rc <> ERROR_SUCCESS) Then GoTo GetKeyError          ' Handle Errors

        If (Asc(Mid(tmpVal, KeyValSize, 1)) = 0) Then           ' Win95 Adds Null Terminated String...
                tmpVal = Left(tmpVal, KeyValSize - 1)               ' Null Found, Extract From String
        Else                                                    ' WinNT Does NOT Null Terminate String...
                tmpVal = Left(tmpVal, KeyValSize)                   ' Null Not Found, Extract String Only
        End If
        '------------------------------------------------------------
        ' Determine Key Value Type For Conversion...
        '------------------------------------------------------------
        Select Case KeyValType                                  ' Search Data Types...
            Case REG_DWORD                                          ' Double Word Registry Key Data Type
                For i = Len(tmpVal) To 1 Step -1                    ' Convert Each Bit
                        KeyVal = KeyVal + Hex(Asc(Mid(tmpVal, i, 1)))   ' Build Value Char. By Char.
                Next i
                KeyVal = Format$("&h" + KeyVal)                     ' Convert Double Word To String
            Case Else
'            Case REG_SZ                                             ' String Registry Key Data Type
                KeyVal = tmpVal                                     ' Copy String Value
        End Select
        

        GetKeyValue = True                                      ' Return Success
        rc = RegCloseKey(hKey)                                  ' Close Registry Key
        Exit Function                                           ' Exit
        
GetKeyError:    ' Cleanup After An Error Has Occured...
        KeyVal = ""                                             ' Set Return Val To Empty String
        GetKeyValue = False                                     ' Return Failure
        rc = RegCloseKey(hKey)                                  ' Close Registry Key
End Function

#If STAND_ALONE Then

Public Function ComNullClear(Ptr As String) As String
Dim Pt As Integer

    'NULLŒŸõ
    Pt = InStr(Ptr, Chr$(0))
    
    If Pt > 0 Then 'NULL”­Œ©
        'NULLƒNƒŠƒA
        ComNullClear = Mid$(Ptr, 1, Pt - 1)
    Else
        ComNullClear = Ptr
    End If
End Function

Public Function Between(a%, b%, c%)
   Between = (a >= b) And (a <= c)
End Function

#End If
Public Function myCInt(ByVal sX$) As Integer
    If (sX > "") And (IsNumeric(sX)) Then
        myCInt = CInt(sX)
    Else
        myCInt = 0
    End If
End Function

Public Sub delay(ByVal nS!)
    Dim nX!
    
    nX = Timer() + nS ' time to wait
    Do While Timer < nX
        DoEvents
    Loop
End Sub
Public Function read_key(ByVal sSection$, ByVal sKey$, ByVal sIniF$) As String
    Dim lRV&, sBS$, nBS&
    
    sBS = String(128, Chr(0))
    nBS = 127
    
    read_key = "" ' default value
    lRV = GetPrivateProfileString(sSection, sKey, "", sBS, nBS, sIniF)
    Debug.Assert lRV >= 0
    If lRV > 0 Then ' have actual value
        '  test ini file for number of projects
    '    nBS = InStr(sBS, Chr(0)) - 1
        nBS = CInt(lRV)
        Debug.Assert nBS > 0
        If nBS > 0 Then
            read_key = Left(sBS, nBS)
        End If
    End If
End Function

Public Function MyTimeValue%(ByVal sTime$)
    Dim nColon%
    
    nColon = InStr(sTime, ":")
    MyTimeValue = 0
    
    If nColon > 1 Then
        MyTimeValue = (60 * myCInt(Left$(sTime, nColon - 1))) + myCInt(Mid$(sTime, nColon + 1))
    Else
        MyTimeValue = MyTimeValue + myCInt(sTime)
    End If
End Function

Function makeDouble(ByVal sTime$) As Double
    Dim nColon%
    
    If IsNumeric(Left$(sTime, 1)) Then  '  (Len(sTime) > 0) And
        nColon = InStr(sTime, ":")
        If nColon > 0 Then
            makeDouble = CDbl(Left(sTime, nColon - 1)) + (CDbl(Mid$(sTime, nColon + 1)) / 60#)
        Else
            makeDouble = CDbl(sTime)
        End If
    Else
        makeDouble = 0#
    End If
End Function

Function padl(ByVal sStr$, ByVal nChars%) As String
    Dim nSpc%
    
    nSpc = nChars - Len(sStr)
    If nSpc > 0 Then
        padl = Space$(2 * nSpc) & sStr ' assumes MS San Serif, NOT courier new
    Else
        padl = sStr
    End If
End Function
#If WRITE_SKRIT Then
Public Sub WriteSkrit(ByVal sFilePath$, ByVal sFileRef$)
    Dim nFF%
    
    On Error GoTo erh
    nFF = FreeFile
    Open sFilePath For Output As nFF
    Print #nFF, "Property string sFileName$ = """ & sFileRef & """;  // modified " & CStr(Now())
    Close #nFF
    Exit Sub
erh:
   frmMain.StatusBar1.SimpleText = "WriteSkrit " & CStr(Err.Number) & ": " & Err.Description
   Err.Clear
End Sub
#End If
Public Function MyTimeStamp$(sCurTime$)
    MyTimeStamp = Format(CDate(sCurTime), "YYYYMMDDhhmmss")
End Function
Function strTok(ByVal sCurLine$, ByVal sToken$)
    Dim nLTkn%, i%, nCurPos%
    
    If (sToken > "") Then
        sCurToken = sToken
        sMyStr = sCurLine
    End If
    nLTkn = Len(sCurToken)
    nCurPos = InStr(sMyStr, sCurToken)
    If nCurPos > 0 Then
        strTok = Left$(sMyStr, nCurPos - 1)
        strTok = RmvdLdSpc(strTok)
        sMyStr = Mid$(sMyStr, nCurPos + 1)
    Else
        strTok = sMyStr
        sMyStr = ""
    End If
End Function

Public Function RmvdLdSpc$(ByVal sStr$)
    Dim cChar$
    
    cChar = Left$(sStr, 1)
    RmvdLdSpc = sStr
    Do While ((cChar = vbTab) Or (cChar = " ")) And (RmvdLdSpc > "")
        RmvdLdSpc = Mid$(RmvdLdSpc, 2)
        cChar = Left$(RmvdLdSpc, 1)
    Loop
End Function
Public Function btwnQuotes$(ByVal sSample$)
    Dim nFrst%, nScnd%
    
    nFrst = InStr(sSample, Chr(34))
    nScnd = InStr(nFrst + 1, sSample, Chr(34))
    If nScnd > nFrst Then
        btwnQuotes = Mid$(sSample, nFrst + 1, nScnd - nFrst - 1)
    Else
        btwnQuotes = ""
        Debug.Assert False ' must have a meaningful entry!
    End If
End Function

Public Function padr$(ByVal sStr$, ByVal nLen%)
    Dim nCurlen%
    
    sStr = RTrim$(sStr)
    nCurlen = Len(sStr)
    If (nCurlen < nLen) Then
        padr = sStr & Space$(nLen - nCurlen)
    Else
        padr = sStr
    End If
End Function
Public Function RmvSpace$(ByVal sX$)
    Dim i%, sCH$
    
    RmvSpace = ""
    For i = 1 To Len(sX)
        sCH = Mid$(sX, i, 1)
        If Not ((sCH = " ") Or (sCH = vbTab)) Then
            RmvSpace = RmvSpace & sCH
        End If
    Next i
End Function
