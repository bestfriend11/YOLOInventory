VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "comdlg32.ocx"
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Object = "{3B7C8863-D78F-101B-B9B5-04021C009402}#1.2#0"; "RICHTX32.OCX"
Begin VB.Form frmMain 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Dungeon Siege test harness"
   ClientHeight    =   4920
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   14640
   Icon            =   "frmMain.frx":0000
   LinkTopic       =   "Form1"
   LockControls    =   -1  'True
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   4920
   ScaleWidth      =   14640
   StartUpPosition =   2  'CenterScreen
   Begin VB.CommandButton cmdConfig 
      Caption         =   "&Config"
      Height          =   420
      Left            =   13680
      TabIndex        =   7
      ToolTipText     =   "Configure launch parameters such as map name."
      Top             =   3960
      Width           =   900
   End
   Begin VB.Timer Timer1 
      Enabled         =   0   'False
      Interval        =   45535
      Left            =   2985
      Top             =   3945
   End
   Begin MSComctlLib.StatusBar StatusBar1 
      Align           =   2  'Align Bottom
      Height          =   420
      Left            =   0
      TabIndex        =   4
      Top             =   4500
      Width           =   14640
      _ExtentX        =   25823
      _ExtentY        =   741
      Style           =   1
      _Version        =   393216
      BeginProperty Panels {8E3867A5-8586-11D1-B16A-00C0F0283628} 
         NumPanels       =   1
         BeginProperty Panel1 {8E3867AB-8586-11D1-B16A-00C0F0283628} 
         EndProperty
      EndProperty
   End
   Begin VB.CommandButton cmdExit 
      Caption         =   "E&xit"
      Height          =   420
      Left            =   6945
      TabIndex        =   3
      ToolTipText     =   "Exit this application."
      Top             =   3960
      Width           =   735
   End
   Begin VB.Frame framResults 
      Caption         =   "Results"
      Height          =   3840
      Left            =   3945
      TabIndex        =   1
      Top             =   0
      Width           =   10650
      Begin RichTextLib.RichTextBox txtResult 
         Height          =   3525
         Left            =   75
         TabIndex        =   6
         Top             =   210
         Width           =   10485
         _ExtentX        =   18494
         _ExtentY        =   6218
         _Version        =   393217
         BackColor       =   12632256
         Enabled         =   -1  'True
         ScrollBars      =   3
         RightMargin     =   12000
         TextRTF         =   $"frmMain.frx":0BC2
      End
   End
   Begin VB.Frame framLaunch 
      Caption         =   "Launch"
      Height          =   3840
      Left            =   75
      TabIndex        =   0
      Top             =   0
      Width           =   3690
      Begin VB.CommandButton cmdLaunch 
         Caption         =   "&Launch"
         Enabled         =   0   'False
         Height          =   390
         Left            =   1425
         TabIndex        =   5
         ToolTipText     =   "Launch Dungeon Siege, running the chosen skrit file."
         Top             =   3270
         Width           =   825
      End
      Begin VB.ListBox List1 
         BackColor       =   &H00C0C0C0&
         Height          =   2790
         Left            =   195
         TabIndex        =   2
         ToolTipText     =   "Select your skrit file."
         Top             =   300
         Width           =   3300
      End
   End
   Begin MSComDlg.CommonDialog CommonDialog1 
      Left            =   6435
      Top             =   7140
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Public sLaunchDT$
Public sVerID$
Enum tExe
   tDebug
   tRelease
   tRetail
End Enum
Public tChosen As tExe
Public bDemo As Boolean, bRetrySkrit As Boolean, bWindowed As Boolean
Public sStartLoc$, sMap$, sCurDir$, bTestComplete As Boolean, nTestResult%
#Const bReportsStreamOK = False  '  True
#Const bAppendToVar = True '  true if going to variable, false if to a file
Const sAllSpells$ = "testALLmagic"
Const sAllWav$ = "testWAV"
Const sSpellFPS$ = "spellParty"

Const sHeadDelim$ = "-==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==-"
Public nCounter%
#If bAppendToVar Then
Private sAllLogs$  '  generic, warning, error and fatal logs get appended to this string variable
#Else
Private sResultsFile$
#End If
Public bInAllSpells As Boolean, bInAllWav As Boolean
Private sPassDef$, sFailDef$, sInfoDef$, sStartDef$, sEndDef$  ' for highlighting the log file


' History :
' 10/10/2001   bw  add a config root path option : sCurDir
' 10/11/2001   bw  impose a limit on HOW MUCH error-*.log is read in, as repeated assertions => 101,292 lines !!!

Public Sub InitLogCapture()
    sAllLogs = ""
'    txtResult.Text = ""  ' blank out the display
End Sub
Private Sub cmdConfig_Click()
   frmConfig.Show vbModal
End Sub

Private Sub cmdExit_Click()
   Unload Me
End Sub

Private Function getLatestDS$()
   Dim sPath$  '  , fLatest!  ' , fCurrent!
   Dim n1us%, n2us%  ' position of underscores
'   fLatest = 0#
   sPath = sCurDir ' & "\DungeonSiege" & IIf(tChosen = tDebug, "D", IIf(tChosen = tRelease, "R", "")) & ".exe"
   
   getLatestDS = ""
'   sPath = Dir("H:\PreCompiled\??.????", vbDirectory)
 '  sPath = getpath(sCurDir)
   n1us = InStrRev(sPath, "_")
   n2us = InStrRev(sPath, "_", n1us - 1)
   If ((n1us > n2us) And (n2us > 0)) Then
        getLatestDS = Mid$(sPath, n2us + 1, n1us - (n2us + 1))
   End If
'   Do While sPath > ""
'       DoEvents
'       If IsNumeric(sPath) Then
'            fCurrent = CSng(sPath)
'            If fCurrent > fLatest Then
'                 fLatest = fCurrent
'                 getLatestDS = sPath
'            End If
'       End If
'       sPath = Dir()
'   Loop
   Debug.Print "found " & getLatestDS & " to be the most recent version of Dungeon Siege installed."
End Function
Private Sub cmdLaunch_Click()
    Dim dRsp#
    Dim sFullPath$, sMySkrit$, sType$
    Dim i%
    
    cmdLaunch.Enabled = False
    cmdExit.Enabled = False
    
    On Error GoTo erh
    sMySkrit = List1.List(List1.ListIndex)
    
    If (StrComp(sMySkrit, sAllSpells, vbTextCompare) = 0) Then
        TstAllSpells
        Exit Sub
    End If
    ' testWAV
    If (StrComp(sMySkrit, sAllWav, vbTextCompare) = 0) Then
        TstWav
        Exit Sub
    End If
    If (StrComp(sMySkrit, sSpellFPS, vbTextCompare) = 0) Then
        TstSpellParty
        Exit Sub
    End If
    
    Select Case tChosen
        Case tDebug
            sType = "D"
        Case tRelease
            sType = "R"
        Case Else ' retail
            sType = ""
    End Select
    bTestComplete = False
    sFullPath = sCurDir & "\DungeonSiege" & sType & ".exe" '
    
    If Dir(sFullPath) = "" Then
        StatusBar1.SimpleText = "DungeonSiege" & sType & ".exe was not found at " & sCurDir '   at H:\PreCompiled\" & sVerID & "  !"
        cmdLaunch.Enabled = True
        cmdExit.Enabled = True
        Exit Sub
    Else                                                                 ' skrit_retry=true
        sFullPath = sFullPath & "  skritbot=auto\test\" & sMySkrit & "   map=" & sMap & "   teleport=" & sStartLoc
        If bDemo Then
           sFullPath = sFullPath & " demo=true "
        End If
        If bRetrySkrit Then
           sFullPath = sFullPath & " skrit_retry=true "
        End If
        If bWindowed Then
            sFullPath = sFullPath & " fullscreen=false"
        End If
          '
        Debug.Print "Cmd = " & sFullPath
    End If
    sLaunchDT = Now()
    
#If bAppendToVar Then
    sFullPath = sFullPath & vbCr
#Else
    sResultsFile = Environ$("computername") & MyTimeStamp(sLaunchDT) & ".log"
    sFullPath = sFullPath & "  FileName=" & sResultsFile & vbCr
'    sResultsFile = Environ$("computername") & ".log"   '  & MyTimeStamp(sLaunchDT)  && this is the old way - writing to skrit file
'    Call WriteSkrit(sCurDir & "\auto\test\logfile.skrit", _
'                    sResultsFile)                                                   && no longer necessary with build 9.2402
    sResultsFile = sCurDir & "\" & sResultsFile
    For i = 0 To 2
        txtWarnings(i).Text = ""
    Next i
#End If
    ChDrive Left$(sCurDir, 1)   ' "h"
    ChDir sCurDir
    Debug.Print "running " & sFullPath
    dRsp = Shell(sFullPath, vbNormalFocus)
    Debug.Print "value of dRsp is " & Format(dRsp, "#,###,###.0##")
    Debug.Assert dRsp <> 0#
    If dRsp <> 0# Then
        StatusBar1.SimpleText = "Launched DungeonSiege" & sType & " with " & sMySkrit$ & " OK ..."
        Call InitLogCapture
        nCounter = 0
        Timer1.Enabled = True
    Else
        StatusBar1.SimpleText = "DungeonSiege" & sType & " with " & sMySkrit$ & " did NOT launch!"
        cmdLaunch.Enabled = True
        cmdExit.Enabled = True
    End If
    Exit Sub
erh:
   StatusBar1.SimpleText = "Launch Error " & CStr(Err.Number) & ": " & Err.Description
   Err.Clear
End Sub
Private Function GetExeType(ByVal sPath$) As tExe
    Dim nPd%, sChar$
    
    nPd = InStrRev(sPath, ".")
    If nPd > 0 Then
        sChar = UCase(Mid$(sPath, nPd - 1, 1))  ' get char just before the period
    Else
        sChar = "R"  ' default to release
    End If
    Select Case sChar
        Case "D"   ' DungeonSiegeD.exe
            GetExeType = tDebug
        Case "R"  ' DungeonSiegeR.exe
            GetExeType = tRelease
        Case Else  ' DungeonSiege.exe => "E"
            GetExeType = tRetail
    End Select
End Function
Private Function getpath$(ByVal sPath$)
    Dim nBS%
    
    nBS = InStrRev(sPath, "\")
    If nBS > 0 Then
        getpath = Left$(sPath, nBS - 1)
    Else
        getpath = sPath
    End If
End Function
Private Sub Form_Load()
   Dim bNeedRoot As Boolean, sTitle$  ' sSubFolder$,
   
   List1.AddItem "KillBarrel"
   List1.AddItem "KillBarrelRange"
   List1.AddItem "testCMagic"
   List1.AddItem "testNMagic"
   List1.AddItem "KillBarrel_7"
   List1.AddItem "KillBarrelRange_7"
   List1.AddItem "testCMagic_7"
   List1.AddItem "testNMagic_7"
   List1.AddItem sAllSpells
   List1.AddItem sAllWav
   List1.AddItem sSpellFPS

   List1.ListIndex = 0
   cmdLaunch.Enabled = (List1.ListCount > 0)
   
   sIniFile = App.Path & "\do_ds.ini"
   If Dir(sIniFile) > "" Then
       
       tChosen = ToInt(read_key("config", "exe_type", sIniFile))
       bDemo = ToBool(read_key("config", "demo", sIniFile), True)
       bRetrySkrit = ToBool(read_key("config", "retry", sIniFile), True) ' As Boolean
       sStartLoc = CStr(read_key("config", "start_loc", sIniFile))
       sMap = CStr(read_key("config", "map", sIniFile))
       bWindowed = ToBool(read_key("config", "windowed", sIniFile), True)
       sCurDir = CStr(read_key("config", "root_path", sIniFile))
   Else
       tChosen = tRelease
       bDemo = True
       bRetrySkrit = True
       sStartLoc = "start"
       sMap = "KillBarrel"
       bWindowed = True
       sCurDir = ""
   End If
   
   If (sCurDir = "") Then
        bNeedRoot = True
        sTitle = "A root directory has not been set"
   Else
        If (Dir(sCurDir & "\Dun*.exe", vbNormal) = "") Then
            bNeedRoot = True
            sTitle = "Dungeon Siege has not been found at " & sCurDir
        Else
            bNeedRoot = False
        End If
   End If
   Do While bNeedRoot
        If bNeedRoot Then
            With CommonDialog1
                 ' get a root directory for Dungeon Siege!
                 .CancelError = False  ' throw no error
                 .DialogTitle = sTitle
                 .Flags = cdlOFNExplorer Or cdlOFNFileMustExist Or cdlOFNPathMustExist Or cdlOFNLongNames
                 .DefaultExt = "exe"
                 .Filter = "Exe (*.exe)|Dun*.exe"
                 .ShowOpen
                 If (.FileName > "") Then
                     sCurDir = getpath(.FileName)
                     tChosen = GetExeType(.FileName)
                     bNeedRoot = False
                 Else
                     sTitle = "A root directory is required"
                 End If
            End With
        End If
   Loop
   sVerID = getLatestDS()
'   sSubFolder = Dir("H:\PreCompiled\" & sVerID & "\" & "CDimage_BITS_01.*", vbDirectory)
'   sCurDir = "H:\PreCompiled\" & sVerID & "\" & sSubFolder
   Call get_delims
   bInAllSpells = False
   bInAllWav = False
End Sub
Private Function ToInt%(sSample$)
   If sSample = "" Then
       ToInt = 0
   Else
       ToInt = CStr(sSample)
   End If
End Function
Private Function ToBool(sSample$, ByVal bDefault As Boolean) As Boolean
   If sSample = "" Then
       ToBool = bDefault
   Else
       ToBool = CBool(sSample)
   End If
End Function
Private Function SessionTime$(ByVal sSample$)
    Dim nStart%, nEnd%
    
    nStart = InStr(sSample, "Session") + 15
    nEnd = InStr(nStart, sSample, vbCr)
    Debug.Assert nEnd > nStart + 1
    SessionTime = Mid$(sSample, nStart, nEnd - nStart)
End Function


#If bAppendToVar Then
Private Sub read_the_file(ByVal sFile$)
   Dim FileLength&, ff%, tC&, sX$, sDummy$, sFileIn$, sSessionTime$, nCurPos&, nEOL&, sLine$, sCH$
   Dim bIsGeneric As Boolean, bReadOk As Boolean

   bIsGeneric = (StrComp(sFile, "GENERIC", vbTextCompare) = 0)
   
#If bReportsStreamOK Then
   If (StrComp(sFile, "generic", vbTextCompare) = 0) Then
      sFileIn = sResultsFile   ' used to be: "H:\PreCompiled\" & sVerID & "\CDimage_BITS_01.0" & sVerID & "_DEV\" & sFile & "-" & Trim(Environ$("computername")) & ".log"
  Else
#End If
      sFileIn = sCurDir & "\" & sFile & "-" & Trim(Environ$("computername")) & ".log"
#If bReportsStreamOK Then
    End If
#End If
   If Dir(sFileIn) > "" Then
      On Error GoTo erh
      ff = FreeFile
      Debug.Print "trying to read " & sFileIn
      Open sFileIn For Input As #ff   ' Open file.
      FileLength = LOF(ff)   ' Get length of file.
   
'      StatusBar1.SimpleText =  "file opened ok: len " & Format$(FileLength, "###,###,###,### bytes")
'      Debug.Assert FileLength > 0&
      bReadOk = False
      If (FileLength > 0&) Then
          tC = 0&
          Screen.MousePointer = vbHourglass
          sDummy$ = Input(FileLength, #ff)
          
          ' process the stuff read
          tC = InStrRev(sDummy$, "App")
          If (tC > 0&) Then
              tC = InStrRev(sDummy$, "-==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==-", tC - 1&)
              sDummy = Mid$(sDummy, tC)
              sSessionTime = SessionTime(sDummy)
              If (InStr(sSessionTime, " 12:") > 0) And Right$(sSessionTime, 2) = "AM" Then  ' see bug 6784
                    sSessionTime = Left$(sSessionTime, Len(sSessionTime) - 2) & "PM"
              End If
              Debug.Print "Comparing launch time " & sLaunchDT & " with session time " & sSessionTime & " from log"
              If CDate(sLaunchDT) <= CDate(sSessionTime) Then
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
                                sAllLogs = sAllLogs & sLine & vbCrLf ' have stuff
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
                  End If
                  bReadOk = True
              Else
                  If Not bIsGeneric Then
                     sAllLogs = sAllLogs & "No " & sFile & " were generated." & vbCrLf
                  End If
              End If
          Else
              sAllLogs = sAllLogs & sDummy   ' have stuff
              bReadOk = True
          End If
          If bIsGeneric Then
              If bReadOk Then
                  StatusBar1.SimpleText = "Results are ready and shown above."
              Else
                  StatusBar1.SimpleText = "There was a SKRIT error preventing successful automation."
              End If
          End If
          Screen.MousePointer = vbDefault
      Else
          If bIsGeneric Then
              StatusBar1.SimpleText = "file " & sFileIn & " is zero bytes! There is nothing to read."
          Else
              sAllLogs = sAllLogs & "No " & sFile & " were generated." & vbCrLf
          End If
      End If
      Close #ff   ' Close file.
   Else
       If bIsGeneric Then
           StatusBar1.SimpleText = "file " & sFileIn & " was not found."
       Else
           sAllLogs = sAllLogs & "No " & sFile & " were generated." & vbCrLf
       End If
   End If
   
   Exit Sub
erh:
   Screen.MousePointer = vbDefault
   StatusBar1.SimpleText = sFile & " READ Error " & CStr(Err.Number) & ": " & Err.Description
   Err.Clear
End Sub
#Else
Private Sub read_the_file(ByVal sFile$, ByRef cCtrl As RichTextBox)
   Dim FileLength&, ff%, tC&, sX$, sDummy$, sFileIn$, sSessionTime$
   Dim bIsGeneric As Boolean, bReadOk As Boolean

   bIsGeneric = (StrComp(sFile, "GENERIC", vbTextCompare) = 0)
   
#If bReportsStreamOK Then
   If (StrComp(sFile, "generic", vbTextCompare) = 0) Then
      sFileIn = sResultsFile   ' used to be: "H:\PreCompiled\" & sVerID & "\CDimage_BITS_01.0" & sVerID & "_DEV\" & sFile & "-" & Trim(Environ$("computername")) & ".log"
  Else
#End If
      sFileIn = sCurDir & "\" & sFile & "-" & Trim(Environ$("computername")) & ".log"
#If bReportsStreamOK Then
    End If
#End If
   If Dir(sFileIn) > "" Then
      On Error GoTo erh
      ff = FreeFile
      Debug.Print "trying to read " & sFileIn
      Open sFileIn For Input As #ff   ' Open file.
      FileLength = LOF(ff)   ' Get length of file.
   
'      StatusBar1.SimpleText =  "file opened ok: len " & Format$(FileLength, "###,###,###,### bytes")
      Debug.Assert FileLength > 0&
      bReadOk = False
      If (FileLength > 0&) Then
          cCtrl.Text = ""
          tC = 0&
          Screen.MousePointer = vbHourglass
          cCtrl.Visible = False
          sDummy$ = Input(FileLength, #ff)
          
          ' process the stuff read
          tC = InStrRev(sDummy$, "App")
          If (tC > 0&) Then
              tC = InStrRev(sDummy$, "-==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==-", tC - 1&)
              sDummy = Mid$(sDummy, tC)
              sSessionTime = SessionTime(sDummy)
              If (InStr(sSessionTime, " 12:") > 0) And Right$(sSessionTime, 2) = "AM" Then  ' see bug 6784
                    sSessionTime = Left$(sSessionTime, Len(sSessionTime) - 2) & "PM"
              End If
              Debug.Print "Comparing launch time " & sLaunchDT & " with session time " & sSessionTime & " from log"
              If CDate(sLaunchDT) <= CDate(sSessionTime) Then
                  cCtrl.Text = sDummy  ' have stuff
                  bReadOk = True
              Else
                  If Not bIsGeneric Then
                     cCtrl.Text = "No " & sFile & " were generated."
                  End If
              End If
          Else
              cCtrl.Text = sDummy  ' have stuff
              bReadOk = True
          End If
          If bIsGeneric Then
              If bReadOk Then
                  StatusBar1.SimpleText = "Results are ready and shown above."
              Else
                  StatusBar1.SimpleText = "There was a SKRIT error preventing successful automation."
              End If
          End If
          cCtrl.Visible = True
          Screen.MousePointer = vbDefault
      Else
          If bIsGeneric Then
              StatusBar1.SimpleText = "file " & sFileIn & " is zero bytes! There is nothing to read."
          Else
              cCtrl.Text = "No " & sFile & " were generated."
          End If
      End If
      Close #ff   ' Close file.
   Else
       If bIsGeneric Then
           StatusBar1.SimpleText = "file " & sFileIn & " was not found."
       Else
           cCtrl.Text = "No " & sFile & " were generated."
       End If
   End If
   cCtrl.SelStart = Len(cCtrl.Text)  ' auto scroll  to end
   
   Exit Sub
erh:
   Screen.MousePointer = vbDefault
   StatusBar1.SimpleText = sFile & " READ Error " & CStr(Err.Number) & ": " & Err.Description
   Err.Clear
End Sub

#End If
Private Sub Form_QueryUnload(Cancel As Integer, UnloadMode As Integer)
   If bInAllSpells Then
'       frmSpells.SetFocus
 '      Beep
 '      Cancel = True
        Unload frmSpells
   End If
   If bInAllWav Then
        Unload frmWav
    End If
End Sub

Private Sub Form_Unload(Cancel As Integer)
   Call saveEntries
End Sub

Private Sub Timer1_Timer()
    Dim nPs&, nHdg&
    
    If (nCounter = 0) Then
        Timer1.Enabled = False
    #If bAppendToVar Then
        
        Call read_the_file("generic")        ' process intended output, warnings and errors
        nPs = InStrRev(sAllLogs, sPassDef)
        nHdg = InStrRev(sAllLogs, sHeadDelim)
        nTestResult = IIf(nPs > nHdg, 0, 1)  ' > 0 only on success
        Call read_the_file("warnings")
        Call read_the_file("errors")
        Call read_the_file("fatals")
        txtResult.Text = sAllLogs
        txtResult.SelStart = Len(txtResult.Text)  ' auto scroll  to end
        bTestComplete = True
    #Else
        Call read_the_file("generic", txtResult)        ' process intended output, warnings and errors
        Call read_the_file("warnings", txtWarnings(0))
        Call read_the_file("errors", txtWarnings(1))
        Call read_the_file("fatals", txtWarnings(2))
        Call store_saved
    #End If
        If Not bInAllSpells Then  ' restore button functionality for main form
            cmdLaunch.Enabled = True
            cmdExit.Enabled = True
        End If
    Else
        nCounter = nCounter - 1 ' prepare for another pass
    End If
End Sub

Private Sub txtResult_Change()
   Dim nStart%, nLen%, sCand$
   
   ' set default text color:
   txtResult.SelStart = 0
   txtResult.SelLength = Len(txtResult.Text)
   txtResult.SelColor = vbBlack
   
   Call FormatControl(txtResult, sPassDef, vbGreen) '  green
   Call FormatControl(txtResult, sFailDef, vbRed) '  red
   Call FormatControl(txtResult, sInfoDef, RGB(55, 55, 225))
   Call FormatControl(txtResult, sStartDef, vbYellow) ' , vbMagenta)
   Call FormatControl(txtResult, sEndDef, vbYellow)   ' , vbMagenta)
   Call FormatControl(txtResult, "+00:", RGB(55, 55, 125), 13)  ' highlight world time to blue
   Call FormatHeading(txtResult)
End Sub

Private Sub txtResult_KeyPress(KeyAscii As Integer)
   KeyAscii = 0
End Sub

Private Sub saveEntries()
    Dim lRtn&
    
    lRtn = WritePrivateProfileString("config", "exe_type", CStr(CInt(tChosen)), sIniFile)
    Debug.Assert lRtn <> 0
    lRtn = WritePrivateProfileString("config", "demo", CStr(CInt(bDemo)), sIniFile)
    Debug.Assert lRtn <> 0
    lRtn = WritePrivateProfileString("config", "retry", CStr(CInt(bRetrySkrit)), sIniFile)
    Debug.Assert lRtn <> 0
    lRtn = WritePrivateProfileString("config", "start_loc", CStr(sStartLoc), sIniFile)
    Debug.Assert lRtn <> 0
    lRtn = WritePrivateProfileString("config", "map", sMap, sIniFile)
    Debug.Assert lRtn <> 0
    lRtn = WritePrivateProfileString("config", "windowed", CStr(CInt(bWindowed)), sIniFile)
    Debug.Assert lRtn <> 0
    lRtn = WritePrivateProfileString("config", "root_path", CStr(sCurDir), sIniFile)
    Debug.Assert lRtn <> 0
End Sub
#If Not bAppendToVar Then
Private Sub txtWarnings_KeyPress(Index As Integer, KeyAscii As Integer)
   KeyAscii = 0  '  swallow keystrokes
End Sub

Sub store_saved()
    Dim nFF%, nI%
    
    On Error GoTo erh
    nFF = FreeFile
    Open sResultsFile For Output Access Write As nFF
    Print #nFF, txtResult.Text
    For nI = 0 To 2
        Print #nFF, txtWarnings(nI).Text
    Next nI
    Close #nFF
    Exit Sub
erh:
   frmMain.StatusBar1.SimpleText = "store_saved " & CStr(Err.Number) & ": " & Err.Description
   Err.Clear
End Sub
#End If
Private Sub get_delims() ' get the output delimiters as defined in skrit
    Dim nFF%, nI%, sKritDefs$, sLine$
    Dim sStDelim$, sEndDelim$, sPass$, sFail$, sInfo$, sStart$, sEnd$, sName$ ' vars used to decode the skrit def file
    
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
    Else
        frmMain.StatusBar1.SimpleText = "Required SKRIT def file " & sKritDefs & " was not found. Using default values."
        sPassDef = "<<PASS>>"
        sFailDef = "<<FAIL>>"
        sInfoDef = "<<INFO>>"
        sStartDef = "<<START>>"
        sEndDef = "<<END>>"
    End If
    Exit Sub
erh:
   frmMain.StatusBar1.SimpleText = "get_delims " & CStr(Err.Number) & ": " & Err.Description
   Err.Clear
End Sub

Sub FormatControl(ByRef cCtrl As RichTextBox, ByVal sTarget$, nColor&, Optional nI&)  ' provide colour highlighting
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
Sub FormatHeading(ByRef cCtrl As RichTextBox)  ' provide colour highlighting for heading(s)
    Dim nStart&, nLen&, nLastLine&
    
'    sHeadDelim = "-==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==-"
   
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
Sub TstSpellParty()
    frmSpells.bFPSmode = True
    frmSpells.Show vbModal
End Sub
Sub TstAllSpells()
    frmSpells.bFPSmode = False
    frmSpells.Show vbModal
End Sub

Sub TstWav()
    frmWav.Show vbModal
End Sub
