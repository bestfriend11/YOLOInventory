Attribute VB_Name = "modParse"
Option Explicit
Public sCurToken$
Public sMyStr$
Public sIniFile$
Global sCurDir As String ' register where you invoke shell() from
Global sAllLogs As String
Global sPassDef$, sFailDef$, sInfoDef$, sStartDef$, sEndDef$  ' for highlighting the log file
Global bFirstRun As Boolean
'Build type paramaters





Public Declare Function GetPrivateProfileSection Lib "kernel32" Alias "GetPrivateProfileSectionA" (ByVal lpAppName As String, ByVal lpReturnedString As String, ByVal nSize As Long, ByVal lpFileName As String) As Long
Public Declare Function GetPrivateProfileString Lib "kernel32" Alias "GetPrivateProfileStringA" (ByVal lpApplicationName As String, ByVal lpKeyName As Any, ByVal lpDefault As String, ByVal lpReturnedString As String, ByVal nSize As Long, ByVal lpFileName As String) As Long
Public Declare Function WritePrivateProfileString Lib "kernel32" Alias "WritePrivateProfileStringA" (ByVal lpApplicationName As String, ByVal lpKeyName As Any, ByVal lpString As Any, ByVal lpFileName As String) As Long

Private Declare Function RegOpenKeyEx Lib "advapi32" Alias "RegOpenKeyExA" (ByVal hKey As Long, ByVal lpSubKey As String, ByVal ulOptions As Long, ByVal samDesired As Long, ByRef phkResult As Long) As Long
Private Declare Function RegQueryValueEx Lib "advapi32" Alias "RegQueryValueExA" (ByVal hKey As Long, ByVal lpValueName As String, ByVal lpReserved As Long, ByRef lpType As Long, ByVal lpData As String, ByRef lpcbData As Long) As Long
Private Declare Function RegCloseKey Lib "advapi32" (ByVal hKey As Long) As Long
Private Declare Function GetModuleFileName Lib "kernel32" Alias "GetModuleFileNameA" (ByVal hModule As Long, ByVal lpFileName As String, ByVal nSize As Long) As Long

Public Function BuildPContentQuery(PcontentIterations As Integer, sQuery As String) As String
    Dim PContentQuery As String
    PContentQuery = sQuery
    ReplaceStringWithString PContentQuery, "*", "Random"
    StripIllegalChars PContentQuery, "!@#$%^&*()_+{}[]|\:<>,.?/'"
    BuildPContentQuery = PContentQuery
    CurrentPcontent.PcontentIterations = PcontentIterations
    CurrentPcontent.PcontentCaseName = PContentQuery
    CurrentPcontent.PcontentExcelFileName = PContentQuery & ".csv"
End Function

Public Function CreateFPSFileName() As String
        Dim FirstSpace As Integer
        
        modDeclares.TimeDate = Now()
        FirstSpace = InStr(modDeclares.TimeDate, " ")
        modDeclares.TimeDate = Mid$(modDeclares.TimeDate, FirstSpace, Len(modDeclares.TimeDate))
        ReplaceStringWithString modDeclares.TimeDate, ":", "-"
        ReplaceStringWithString modDeclares.TimeDate, " ", ""
        CreateFPSFileName = "FPS-" & CurrentUserInfo.CurrentBuild & " - " & modDeclares.TimeDate & ".xls"

End Function
Public Function ReplaceStringWithString(ByRef sCheckString As String, ByVal sStringToChange As String, ByVal sReplaceMentString As String)
    ' This function takes a string and converts it to the string passed in
    Dim iCharPos As Integer
    Dim bAllDone As Boolean
    

    Do While bAllDone = False
    iCharPos = InStr(1, sCheckString, sStringToChange)
    If iCharPos > 1 Then
        If iCharPos < Len(sCheckString) Then
            sCheckString = Left$(sCheckString, iCharPos - 1) & sReplaceMentString & Mid$(sCheckString, iCharPos + Len(sStringToChange))
        ElseIf iCharPos = Len(sCheckString) Then
            sCheckString = Left$(sCheckString, iCharPos - 1) & sReplaceMentString
        End If
    ElseIf iCharPos = 1 Then
        sCheckString = sReplaceMentString & Mid$(sCheckString, iCharPos + Len(sStringToChange))
    Else
        Exit Do
    End If
'    Debug.Print sCheckString
    Loop
    

End Function
Public Sub TrimExcessSpaces(ByRef sCheckString As String)  'This Function removes Excess spaces
                                 'And leaves just one space
    Dim bExtraSpacesCleared As Boolean
    Dim iCounter As Integer
    
    iCounter = 1
    Do While iCounter < Len(sCheckString)
        If Mid$(sCheckString, iCounter, 1) = " " Then
            If Mid$(sCheckString, iCounter + 1, 1) = " " Then
                sCheckString = Left$(sCheckString, iCounter - 1) & Mid$(sCheckString, iCounter + 1)
                iCounter = iCounter - 1
            End If
        End If

        iCounter = iCounter + 1
    Loop
End Sub

Public Sub StripIllegalChars(ByRef sCheckString As String, sCharsToStrip As String) '/ Strips illegal chars from a string and returns a stripped string
    Dim iCounterMain As Integer
    Dim iCounterCheckString As Integer
    Dim IllegalCharIP As Integer
    Dim sCurrentReservedChar As String
    Dim bIsFirst As Boolean
    Dim bIsLast As Boolean
    
    
    
    For iCounterMain = 1 To Len(sCharsToStrip)
        sCurrentReservedChar = Mid$(sCharsToStrip, iCounterMain, 1)
        Debug.Print "Looking for " & sCurrentReservedChar & "in " & sCheckString

        
        Do While InStr(1, sCheckString, sCurrentReservedChar) <> 0
            IllegalCharIP = InStr(1, sCheckString, sCurrentReservedChar)
            If IllegalCharIP <> 0 Then
                If IllegalCharIP = 1 Then
                    bIsFirst = True
                    bIsLast = False
                ElseIf IllegalCharIP = Len(sCheckString) Then
                    bIsFirst = False
                    bIsLast = True
                Else
                    bIsFirst = False
                    bIsLast = False
                End If
                If bIsFirst = False Then
                    If bIsLast = False Then
                        sCheckString = Left$(sCheckString, IllegalCharIP - 1) & Mid$(sCheckString, IllegalCharIP + 1)
                    ElseIf bIsLast = True Then
                        sCheckString = Left$(sCheckString, IllegalCharIP - 1)
                    End If
                ElseIf bIsFirst = True Then
                    sCheckString = Mid$(sCheckString, IllegalCharIP + 1)
                End If
            End If
        Loop
    Debug.Print sCheckString
    Next iCounterMain
    
End Sub

''''Brians Log Code
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
Public Sub read_the_file(ByVal sFile$)
   Dim FileLength&, ff%, tC&, sX$, sDummy$, sFileIn$, sSessionTime$
   Dim bIsGeneric As Boolean, bReadOk As Boolean, bIsCrash As Boolean
   Dim nCurPos&, nEOL&, sLine$, sCH$, sExplanation$

   bIsGeneric = (StrComp(sFile, "GENERIC", vbTextCompare) = 0)
   
   If (StrComp(sFile, "crash", vbTextCompare) = 0) Then
      sFileIn = modParse.sCurDir & "\dungeonsiege" & IIf(frmMain.optRelease.Value, "R", "D") & ".crash"
 '     Debug.Assert False
      bIsCrash = True
   Else
      sFileIn = sCurDir & "\" & sFile & "-" & Trim(Environ$("computername")) & ".log"
      bIsCrash = False
   End If
   sExplanation = ""
   If Dir(sFileIn) > "" Then
      On Error GoTo erh
      ff = FreeFile
      Debug.Print "trying to read " & sFileIn
      Open sFileIn For Input As #ff   ' Open file.
      FileLength = LOF(ff)   ' Get length of file.
   
'      StatusBar1.SimpleText =  "file opened ok: len " & Format$(FileLength, "###,###,###,### bytes")
      'Debug.Assert FileLength > 0&
      bReadOk = False
      If (FileLength > 0&) Then
          tC = 0&
          Screen.MousePointer = vbHourglass
          sDummy$ = Input(FileLength, #ff)
          
          ' process the stuff read
          If bIsCrash Then
              tC = InStrRev(sDummy$, "Session")
          Else
              tC = InStrRev(sDummy$, "App")
          End If
          If (tC > 0&) Then
              tC = InStrRev(sDummy$, "-==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==-", tC - 1&)
              sDummy = Mid$(sDummy, tC)
              sSessionTime = SessionTime(sDummy)
              If (sSessionTime = "") Then GoTo erh
 '             If (InStr(sSessionTime, " 12:") > 0) And Right$(sSessionTime, 2) = "AM" Then  ' see bug 6784  |  now closed
 '                   sSessionTime = Left$(sSessionTime, Len(sSessionTime) - 2) & "PM"
 '             End If
              Debug.Print "Comparing launch time " & modDeclares.tLaunchTD & " with session time " & sSessionTime & " from log"
              If Not (IsDate(tLaunchTD) And IsDate(sSessionTime)) Then GoTo erh
              If CDate(tLaunchTD) <= CDate(sSessionTime) Then
                  If bIsGeneric Then
                  ' ensure that debugging lines inserted by devs do NOT appear:
'                       sAllLogs = sAllLogs & sDummy  ' have stuff
                      nCurPos = 1&
                      Do While nCurPos > 0&
                           DoEvents
                           nEOL = InStr(nCurPos + 1&, sDummy, vbCr)
                           If (nEOL = 0&) Then
                               nEOL = Len(sDummy)
                           End If
                           If nEOL > nCurPos Then
                                sLine = Mid$(sDummy, nCurPos, nEOL - nCurPos)
                           Else
                                Exit Do
                           End If
                           sCH = Left$(sLine, 1&)
                           If ((sCH = "<") Or (sCH = "-") Or (sCH = "+")) Then
                                If (((InStr(sLine, "time") = 0) Or (InStr(sLine, "angle") = 0)) And _
                                       (InStr(sLine, "curr_ratio") = 0) And (InStr(sLine, "deviation") = 0)) And _
                                     ((Left$(sLine, 3) = "-==") Or (InStr(sLine, "[") = 0)) Then   ' opt out dev comments
                                    sAllLogs = sAllLogs & sLine & vbCrLf ' have stuff
                                    bReadOk = True
                                End If
                           End If
                           nCurPos = nEOL + 1&
                           sCH = Mid$(sDummy, nCurPos, 1&)  ' avoid reading empty lines
                           Do While ((sCH = Chr(13)) Or (sCH = Chr(10)))
                                DoEvents
                                nCurPos = nCurPos + 1&
                                If (nCurPos >= Len(sDummy)) Then
                                     Exit Do
                                Else
                                     sCH = Mid$(sDummy, nCurPos, 1&)  ' avoid reading empty lines
                                End If
                           Loop
    
                           If (nCurPos >= Len(sDummy)) Then
                                Exit Do
                           End If
                      Loop
                  Else
' 10/11/2001   bw  impose a limit on HOW MUCH error-*.log is read in, as repeated assertions => 101,292 lines !!!
                      If Len(sDummy) > 3000 Then
                           sAllLogs = sAllLogs & Left$(sDummy, 3000) & vbCrLf & "*** Truncated as this file was excessively long ***" & vbCrLf  ' have stuff
                      Else
                           sAllLogs = sAllLogs & sDummy  ' have a little stuff
                      End If
                      bReadOk = True
                  End If
              Else
                  If bIsGeneric Then
                     If bInAllSpells Then
                         sExplanation = "The skrit produced no " & sFile & " output." & vbCrLf   '  "A SKRIT error prevented any output." & vbCrLf
                     End If
                  End If
              End If
          End If
          Screen.MousePointer = vbDefault
      End If
      Close #ff   ' Close file.
      ff = -1
   End If
   
   If Not bReadOk Then
      If bIsGeneric Then
         If bInAllSpells Then
            sAllLogs = sAllLogs & sExplanation
         End If
      Else
         If bIsCrash Then
            sAllLogs = sAllLogs & "No " & sFile & " was detected." & vbCrLf
         Else
            sAllLogs = sAllLogs & "No " & sFile & " were generated." & vbCrLf
        End If
      End If
   End If
   
   Exit Sub
erh:
   If ff >= 0 Then
      Close #ff   ' Close file.
   End If
   Screen.MousePointer = vbDefault
   If (Err.Number <> 0) Then
        sAllLogs = sAllLogs & " READ Error " & CStr(Err.Number) & ": " & Err.Description & vbCrLf
'   StatusBar1.SimpleText = sFile & " READ Error " & CStr(Err.Number) & ": " & Err.Description
        Err.Clear
   End If
End Sub

Private Function SessionTime$(ByVal sSample$)
    Dim nStart%, nEnd%
    
    nStart = InStr(sSample, "Session") ' + 15
    If nStart > 0 Then
        nStart = nStart + 15
        nEnd = InStr(nStart, sSample, vbCr)
        Debug.Assert nEnd > nStart + 1
        SessionTime = Mid$(sSample, nStart, nEnd - nStart)
    Else
        SessionTime = ""
    End If
End Function
Public Sub delay(ByVal nS!)  ' change BW  1/21/02  make safe for times approaching midnight
    Dim nX!
    
    nX = Timer() + nS ' time to wait  |  note that fuction timer() Returns a Single representing the number of seconds elapsed since midnight.
    If (nX > (86400 - 60)) Then
        nX = nS  ' wait only up to ns seconds after midnight
    End If
    Do While Timer < nX
        DoEvents
    Loop
End Sub
Public Sub get_delims() ' get the output delimiters as defined in skrit
    Dim nFF%, nI%, sKritDefs$, sLine$
    Dim sStDelim$, sEndDelim$, sPass$, sFail$, sInfo$, sStart$, sEnd$, sName$ ' vars used to decode the skrit def file
    Dim bGotEm As Boolean
    
    bGotEm = False
    modSpells.sCurBld = ""  ' init
    modSpells.sCurDateTimeFile = ""
    sCurDir = CurrentUserInfo.ExePath
    If sCurDir > "" Then
        On Error GoTo erh
        nFF = FreeFile
        sKritDefs = sCurDir & "\auto\test\logout.skrit"
        If (Dir(sKritDefs) > "") Then  ' file exists
            Open sKritDefs For Input Access Read As nFF
            Do While Not EOF(nFF)
                DoEvents
                Line Input #nFF, sLine
                nI = InStr(sLine, "property string")
                If nI > 0 Then  ' have another def to process
                    bGotEm = True
                    sName = strTok(LTrim(Mid$(sLine, nI + 16)), " ")
                    Select Case (sName)
                        Case "PASS_ID$"
                            sPass = btwnQuotes(sLine)
                        Case "FAIL_ID$"
                            sFail = btwnQuotes(sLine)
                        Case "INFO_ID$"
                            sInfo = btwnQuotes(sLine)
                        Case "START_ID$"
                            sStart = btwnQuotes(sLine)
                        Case "END_ID$"
                            sEnd = btwnQuotes(sLine)
                        Case "DELIMS_ID$"
                            sStDelim = btwnQuotes(sLine)
                        Case "DELIME_ID$"
                            sEndDelim = btwnQuotes(sLine)
                        Case Else
                            Debug.Assert False ' there are lines in the skrit file unknown to this vb app
                    End Select
                    If (sPass > "") And (sFail > "") And (sInfo > "") And (sStart > "") And (sEnd > "") And (sStDelim > "") And (sEndDelim > "") Then
                        Exit Do
                    End If
                End If
            Loop
            Close #nFF
            ' define sPassDef, sFailDef, sInfoDef, sStartDef, sEndDef  :
            Debug.Assert (sPass > "") And (sFail > "") And (sInfo > "") And (sStart > "") And (sEnd > "") And (sStDelim > "") And (sEndDelim > "")
            sPassDef = sStDelim & sPass & sEndDelim
            sFailDef = sStDelim & sFail & sEndDelim
            sInfoDef = sStDelim & sInfo & sEndDelim
            sStartDef = sStDelim & sStart & sEndDelim
            sEndDef = sStDelim & sEnd & sEndDelim
        End If
    End If
    If Not bGotEm Then
        'frmMain.StatusBar1.SimpleText = "Required SKRIT def file " & sKritDefs & " was not found. Using default values."
        sPassDef = "<<PASS>>"
        sFailDef = "<<FAIL>>"
        sInfoDef = "<<INFO>>"
        sStartDef = "<<START>>"
        sEndDef = "<<END>>"
    End If
    If (frmMain.lstvBuildDTMachine.ListItems.Count > 0) Then
        frmMain.chkContinueSpellRun.Enabled = (RTrim(frmMain.lstvBuildDTMachine.SelectedItem.Text) = modSpells.GetFileVersion(modParse.sCurDir & "\dungeonsiege" & IIf(frmMain.optRelease.Value, "R", "D") & ".exe"))
        If Not frmMain.chkContinueSpellRun.Enabled Then frmMain.chkContinueSpellRun.Value = vbUnchecked
    End If
    Exit Sub
erh:
   'frmMain.StatusBar1.SimpleText = "get_delims " & CStr(Err.Number) & ": " & Err.Description
   Err.Clear
End Sub

Public Sub FormatControl(ByRef cCtrl As RichTextBox, ByVal sTarget$, nColor&, Optional nI&)  ' provide colour highlighting
    Dim nStart&, nLen&
   
    nStart = InStrRev(cCtrl.Text, sTarget)
    If (nI = 0&) Then
        nLen = Len(sTarget)
    Else
        nLen = nI
    End If
    Do While nStart > 0
        cCtrl.SelStart = nStart - 1&
        cCtrl.SelLength = nLen
        cCtrl.SelColor = nColor  '
        cCtrl.SelBold = True
        nStart = InStrRev(cCtrl.Text, sTarget, nStart - 1&)
    Loop
End Sub
Public Sub FormatHeading(ByRef cCtrl As RichTextBox)  ' provide colour highlighting for heading(s)
    Dim nStart&, nLen&, nLastLine&
    
    nStart = InStrRev(cCtrl.Text, sHeadDelim) ' this marks last line of heading
    nLastLine = nStart + Len(sHeadDelim) + 1&
    nStart = InStrRev(cCtrl.Text, sHeadDelim, nStart - 1&) ' this marks first line of heading
    
    Do While nStart > 0&
        nLen = nLastLine - nStart
        cCtrl.SelStart = nStart - 1&
        cCtrl.SelLength = nLen
        cCtrl.SelColor = RGB(55, 155, 55)   '
        cCtrl.SelBold = True
        
        If nStart > 1& Then
            ' get next heading
            nStart = InStrRev(cCtrl.Text, sHeadDelim, nStart - 1&)  ' this marks last line of heading
            If (nStart <= 1&) Then
                Exit Do
            End If
            nLastLine = nStart + Len(sHeadDelim) + 1&
            nStart = InStrRev(cCtrl.Text, sHeadDelim, nStart - 1&) ' this marks first line of heading
        Else
            Exit Do
        End If
    Loop
End Sub

Public Function MyLike(ByVal s1$, ByVal s2$) As Boolean
    Dim nComp&
    
    nComp = Len(s2)
    If Len(Trim$(s1)) = 0 Then
        MyLike = False
    Else
        If s2 = "*" Then
            MyLike = True
        Else
            MyLike = StrComp(Left$(Trim$(s1), nComp), s2, vbTextCompare) = 0
        End If
    End If
End Function
Public Sub ReadTheLog(ByRef currentloginfo As LogInfo)
    Dim PassFail As Boolean
    
    sAllLogs = ""
    Call modParse.read_the_file("generic")        ' process intended output, warnings and errors
    Call modParse.read_the_file("warnings")
    Call modParse.read_the_file("errors")
    Call modParse.read_the_file("fatals")
    Call modParse.read_the_file("crash")  ' added bw 10/17/2001
    currentloginfo.LogData = sAllLogs
    PassFail = IIf(InStr(currentloginfo.LogData, sPassDef), 1, 0)
    currentloginfo.Pass = PassFail
End Sub



Public Function WriteSQLSrvr() As Boolean
    Dim lRtn&
    
    lRtn = WritePrivateProfileString("config", "SQLSrvr", CurrentUserInfo.SQLserver, sIniFile)
    Debug.Assert lRtn <> 0
    WriteSQLSrvr = (lRtn <> 0)
End Function

Public Function ReadSQLSrvr() As Boolean
    Dim sSQLServr$
    
    sSQLServr = read_key("config", "SQLSrvr", sIniFile)
    If (sSQLServr > "") Then
        ' only overwrite current value if we have a meaningful entry
        CurrentUserInfo.SQLserver = sSQLServr
        ReadSQLSrvr = True
    Else
        ReadSQLSrvr = False
    End If
End Function
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

Public Function sqlSrvrOK(ByVal sSqlName$, ByRef sTitle$) As Boolean
  '  select From table
  Dim cnConnection As New ADODB.Connection, myConnStr$, bNoGood As Boolean
  
  myConnStr = "driver={SQL Server};SERVER=" & sSqlName & ";database=unittestdb;dsn=''"  '  SetDBasConnStr("SpellLogs")
  On Error Resume Next  ' GoTo 0
  bNoGood = False
  With cnConnection
        .ConnectionString = myConnStr    ' was ConnectionString - need new db
        .ConnectionTimeout = 100&
        .Open
        If (Err.Number > 0) Then
            bNoGood = True
            sTitle = Err.Description
            Err.Clear
        Else
            If cnConnection.State <> adStateOpen Then
                bNoGood = True
                sTitle = "Cannot open " & sSqlName ' CurrentUserInfo.SQLserver
            Else
                .Close
            End If
        End If
  End With
  Set cnConnection = Nothing
  sqlSrvrOK = Not bNoGood
End Function
Public Function SqlServerExists(ByRef sSqlServer$) As Boolean
  '  select From table
  Dim sTitle$, bStatus As Boolean, fFrm As Form
  
  bStatus = sqlSrvrOK(sSqlServer, sTitle)
  
  Do While Not (bStatus)
 '     MsgBox "SQL Server " & sSqlServer & " was not found."

      sSqlServer = InputBox("Please enter a SQL Server name", sTitle, sSqlServer)
      If (sSqlServer = "") Then ' canceled
          SqlServerExists = False
          Exit Do
      End If
      bStatus = sqlSrvrOK(sSqlServer, sTitle)
      If bStatus Then frmMain.txtSQLserver.Text = sSqlServer
  Loop
  SqlServerExists = bStatus
End Function

Public Sub ExitNoSQLServer()
    Dim i%
    
    MsgBox "Exiting as no valid SQL Server name is available."
    For i = 0 To Forms.Count - 1
       Unload Forms(i)
    Next i
    End ' out of here
End Sub
