VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form frmSpells 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Current Spell listing"
   ClientHeight    =   6945
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   14910
   Icon            =   "frmSpells.frx":0000
   LinkTopic       =   "Form1"
   LockControls    =   -1  'True
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   6945
   ScaleWidth      =   14910
   StartUpPosition =   1  'CenterOwner
   Begin VB.TextBox txtSampleTime 
      Height          =   285
      Left            =   7245
      TabIndex        =   19
      Text            =   "60.0"
      ToolTipText     =   "Enter sample time in seconds. Your entry should not be less than 30."
      Top             =   6240
      Width           =   720
   End
   Begin VB.CheckBox chkRepeat 
      Caption         =   "Repeat "
      Height          =   360
      Left            =   4950
      TabIndex        =   17
      ToolTipText     =   "Repeatadding monsters as they get killed"
      Top             =   6195
      Width           =   855
   End
   Begin VB.CheckBox chkOpenExcel 
      Caption         =   "Open Excel when done"
      Height          =   240
      Left            =   2820
      TabIndex        =   16
      Top             =   6255
      Value           =   1  'Checked
      Width           =   2000
   End
   Begin VB.ListBox lstHeroType 
      Height          =   255
      Left            =   945
      TabIndex        =   14
      Top             =   6255
      Width           =   1410
   End
   Begin VB.Frame Frame2 
      Caption         =   "Monsters"
      Height          =   765
      Left            =   105
      TabIndex        =   9
      Top             =   5220
      Width           =   7830
      Begin VB.ListBox lstOpponents 
         Height          =   255
         Left            =   1890
         Sorted          =   -1  'True
         TabIndex        =   12
         Top             =   255
         Width           =   3555
      End
      Begin VB.OptionButton chkMonsters 
         Caption         =   "Selected &Monster Only"
         Height          =   225
         Index           =   0
         Left            =   5535
         TabIndex        =   11
         ToolTipText     =   "All spells will be used against the current monster only."
         Top             =   165
         Width           =   1900
      End
      Begin VB.OptionButton chkMonsters 
         Caption         =   "&All Monsters"
         Height          =   225
         Index           =   1
         Left            =   5535
         TabIndex        =   10
         ToolTipText     =   "Be prepared to leave this test running overnight! All spells will be used against all monsters."
         Top             =   450
         Value           =   -1  'True
         Width           =   1200
      End
      Begin VB.Label lblOpponent 
         Caption         =   "&Opponent Under Test:"
         Height          =   225
         Left            =   135
         TabIndex        =   13
         Top             =   270
         Width           =   1605
      End
   End
   Begin VB.Frame Frame1 
      Caption         =   "Spells"
      Height          =   765
      Left            =   8040
      TabIndex        =   6
      Top             =   5220
      Width           =   2025
      Begin VB.OptionButton optSpell 
         Caption         =   "Selected &Spell Only"
         Height          =   225
         Index           =   0
         Left            =   150
         TabIndex        =   8
         Top             =   195
         Width           =   1690
      End
      Begin VB.OptionButton optSpell 
         Caption         =   "All S&pells"
         Height          =   195
         Index           =   1
         Left            =   150
         TabIndex        =   7
         Top             =   450
         Value           =   -1  'True
         Width           =   1000
      End
   End
   Begin VB.CommandButton cmdAbort 
      Caption         =   "A&bort"
      Enabled         =   0   'False
      Height          =   300
      Left            =   10170
      TabIndex        =   5
      Top             =   5490
      Width           =   1035
   End
   Begin VB.CommandButton cmdViewLog 
      Caption         =   "&View Log"
      Enabled         =   0   'False
      Height          =   300
      Left            =   11370
      TabIndex        =   4
      ToolTipText     =   "Click here to view the log file."
      Top             =   5490
      Width           =   1080
   End
   Begin MSComctlLib.StatusBar StatusBar1 
      Align           =   2  'Align Bottom
      Height          =   300
      Left            =   0
      TabIndex        =   3
      Top             =   6645
      Width           =   14910
      _ExtentX        =   26300
      _ExtentY        =   529
      Style           =   1
      _Version        =   393216
      BeginProperty Panels {8E3867A5-8586-11D1-B16A-00C0F0283628} 
         NumPanels       =   1
         BeginProperty Panel1 {8E3867AB-8586-11D1-B16A-00C0F0283628} 
         EndProperty
      EndProperty
   End
   Begin VB.CommandButton cmdExit 
      Cancel          =   -1  'True
      Caption         =   "C&ancel"
      Enabled         =   0   'False
      Height          =   300
      Index           =   1
      Left            =   13800
      TabIndex        =   2
      ToolTipText     =   "Exit this form without testing the spells."
      Top             =   5490
      Width           =   1005
   End
   Begin VB.CommandButton cmdExit 
      Caption         =   "&Run"
      Default         =   -1  'True
      Enabled         =   0   'False
      Height          =   300
      Index           =   0
      Left            =   12630
      TabIndex        =   1
      ToolTipText     =   "Run tests on all these spells, in the order shown..."
      Top             =   5490
      Width           =   1005
   End
   Begin MSComctlLib.ListView lstSpells 
      Height          =   5130
      Left            =   90
      TabIndex        =   0
      Top             =   30
      Width           =   14730
      _ExtentX        =   25982
      _ExtentY        =   9049
      View            =   3
      LabelEdit       =   1
      Sorted          =   -1  'True
      LabelWrap       =   -1  'True
      HideSelection   =   0   'False
      _Version        =   393217
      ForeColor       =   -2147483640
      BackColor       =   -2147483643
      BorderStyle     =   1
      Appearance      =   1
      NumItems        =   10
      BeginProperty ColumnHeader(1) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         Key             =   "a"
         Object.Tag             =   "a"
         Text            =   "template"
         Object.Width           =   3499
      EndProperty
      BeginProperty ColumnHeader(2) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         SubItemIndex    =   1
         Key             =   "b"
         Object.Tag             =   "b"
         Text            =   "specializes"
         Object.Width           =   3246
      EndProperty
      BeginProperty ColumnHeader(3) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         SubItemIndex    =   2
         Key             =   "c"
         Object.Tag             =   "c"
         Text            =   "damage_max"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(4) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         SubItemIndex    =   3
         Key             =   "d"
         Object.Tag             =   "d"
         Text            =   "damage_min"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(5) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         SubItemIndex    =   4
         Key             =   "e"
         Object.Tag             =   "e"
         Text            =   "is_one_shot"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(6) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         SubItemIndex    =   5
         Key             =   "f"
         Object.Tag             =   "f"
         Text            =   "mana_cost"
         Object.Width           =   1887
      EndProperty
      BeginProperty ColumnHeader(7) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         SubItemIndex    =   6
         Key             =   "g"
         Object.Tag             =   "g"
         Text            =   "required_level"
         Object.Width           =   2286
      EndProperty
      BeginProperty ColumnHeader(8) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         SubItemIndex    =   7
         Key             =   "h"
         Object.Tag             =   "h"
         Text            =   "screen_name"
         Object.Width           =   2533
      EndProperty
      BeginProperty ColumnHeader(9) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         SubItemIndex    =   8
         Key             =   "i"
         Object.Tag             =   "i"
         Text            =   "Class"
         Object.Width           =   1778
      EndProperty
      BeginProperty ColumnHeader(10) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         SubItemIndex    =   9
         Key             =   "j"
         Object.Tag             =   "j"
         Text            =   "Test Result"
         Object.Width           =   2286
      EndProperty
   End
   Begin VB.Label lblSampleTime 
      Caption         =   "Sample Time:"
      Height          =   225
      Left            =   6210
      TabIndex        =   18
      ToolTipText     =   "Enter Sample Time in seconds.  "
      Top             =   6270
      Width           =   975
   End
   Begin VB.Label lblParty 
      Caption         =   "PartyType:"
      Height          =   210
      Left            =   150
      TabIndex        =   15
      Top             =   6270
      Width           =   795
   End
End
Attribute VB_Name = "frmSpells"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

#Const SIMULATE = False   '   True    '
#Const bAppendToVar = True '  true if going to variable, false if to a file

Dim bInProcess As Boolean, sOpponent$
Dim sDirs$() ' array, not yet dimmed
Dim ndir%, sOldRpt$
Dim haveDir As Boolean, bAbort As Boolean, nLineCtr%, nPageCtr%, bRptAbort As Boolean
Dim cRpt As SpellRpt
Public bFPSmode As Boolean
Dim CurrentExcelFileName$, nSpellTimerIntervals%, nSpellTimerCount&

Private Function mylen%(ByVal nCurElt%)
    Select Case nCurElt  ' lookup how wide in chars each field should be:
        Case 2
            mylen = 17
        Case 7            ' spell screen name, only one currently showing in report
            mylen = 16
        Case 8
            mylen = 18
        Case 9
            mylen = 7
        Case Else
            mylen = 12
    End Select
End Function
Private Sub runTest()
    Dim i%, j%, k%, lix As ListItem, nOldTimerPD&
    Dim nPass%, nFail%
    Dim nFirst%, nLast%
    
    If optSpell(1).Value Then ' all spells are selected
        nFirst = 1
        nLast = lstSpells.ListItems.Count
    Else
        nFirst = 1
        Do While nFirst < lstSpells.ListItems.Count
            DoEvents
            If lstSpells.ListItems(nFirst).Selected Then
                Exit Do
            End If
            nFirst = nFirst + 1    '  should mean nothing selected
        Loop
        If Not lstSpells.ListItems(nFirst).Selected Then
            StatusBar1.SimpleText = "You have selected to run ""selected spell"" and no spell has been selected. Please select one now..."
            lstSpells.SetFocus
            Exit Sub
        Else
            nLast = nFirst
        End If
    End If
'        MsgBox "Will eventually run the tests..."
#If SIMULATE Then
        Randomize
#End If
        bInProcess = True
        With frmSpells
            .cmdExit(0).Enabled = False
            .cmdExit(1).Enabled = False
        End With
        lstSpells.ListItems(1).EnsureVisible  ' start from square 1
        
        
        nOldTimerPD = frmMain.Timer1.Interval
        'frmMain.Timer1.Interval = 40000   ' 40*2 = 80 secs
        
        If frmMain.tChosen = tdebug Then
            nSpellTimerCount = 1000& * (CLng(txtSampleTime.Text) + 200&) ' allow 200 second load + exit time for debug mode
        Else
            nSpellTimerCount = 1000& * (CLng(txtSampleTime.Text) + 120&) ' allow 120 second load + exit time for debug mode
        End If
        nSpellTimerIntervals = 0
        While nSpellTimerCount > 65535
            DoEvents
            nSpellTimerIntervals = nSpellTimerIntervals + 1
            nSpellTimerCount = nSpellTimerCount - 65535
        Wend
        frmMain.Timer1.Interval = nSpellTimerCount
        
        sOpponent = lstOpponents.List(lstOpponents.ListIndex)
        
        nPass = 0
        k = 0
        For i = nFirst To nLast       ' 9 To 11   '  lstSpells.ListItems.Count   ' 1  ..  2 '
            Call frmMain.InitLogCapture ' init the log capture
            If nLineCtr > 54 Then
                If (nPageCtr > 1) Then
                    ' insert formfeed - end previous page
                    cRpt.AddPiece Chr(12)
                End If
                cRpt.AddPiece "Result of spells test conducted on : Build " & frmMain.sVerID & "; Computer " & Environ$("computername") & "; " & CStr(Now()) & "   Page: " & FormatNumber(nPageCtr, 0) & vbCrLf & vbCrLf
                nPageCtr = nPageCtr + 1
                cRpt.AddPiece "Monster \/           | Spell >"
                For j = nFirst To nLast
                    cRpt.AddPiece padr(RmvSpace(lstSpells.ListItems(j).SubItems(7)), mylen(7)) ' list the spells under test
                Next j
                cRpt.AddPiece vbCrLf
                nLineCtr = 3
            End If
            If (i = nFirst) Then cRpt.AddPiece padr(sOpponent, 30)
            Set lix = lstSpells.ListItems(i)
            lix.SubItems(9) = ""
#If SIMULATE Then
            delay 0.5!  ' calls doEvents to handle pending windows events
            j = Int(2 * Rnd())  ' j now  in range [0..1]
#Else
            frmMain.nCounter = nSpellTimerIntervals  ' take nSpellTimerIntervals x timer1 intervals to evaluate pass / fail
            If Dir(frmMain.sCurDir & "\fps.log") > "" Then Kill frmMain.sCurDir & "\fps.log"
            j = ResultOfSpellTest(i)  ' run the next spell test
            If bFPSmode Then
                Do While Dir(frmMain.sCurDir & "\fps.log") = ""
                    DoEvents                                      ' wait until log is ready for processing
                Loop
                delay 0.03
                k = k + 1
                modExcel.ImportLog lstSpells.ListItems(i).Text, CurrentExcelFileName, k
            End If
#End If
            If (j = 0) Then
                lix.SubItems(9) = "Pass"
                nPass = nPass + 1
            Else
                nFail = nFail + 1
                lix.SubItems(9) = "Fail"
            End If

 '           sRptLine = sRptLine & vbTab & lix.SubItems(9)
            cRpt.AddPiece padr(lix.SubItems(9), mylen(7))

            lix.Selected = True
            If i > 22 Then
                lix.EnsureVisible
            End If
            Set lix = Nothing
            If bAbort Then
                If Not bRptAbort Then
                    cRpt.AddPiece vbCrLf & sAbortMsg & vbCrLf
                    bRptAbort = True
                End If
                Exit For
            End If
        Next i
        
        cRpt.AddPiece vbCrLf
        nLineCtr = nLineCtr + 1
'        cRpt.AddPiece vbCrLf & "Passed: " & Format(nPass, "##0") & vbTab & "Failed: " & Format(nFail, "##0") & vbTab & "Total: " & Format(nPass + nFail, "###")
        frmMain.Timer1.Interval = nOldTimerPD
        With frmSpells
            .cmdExit(0).Enabled = True
            .cmdExit(1).Enabled = True
        End With
        bInProcess = False
End Sub

Private Sub chkMonsters_Click(Index As Integer)
    lstOpponents.Enabled = (Index = 0)  ' disable if "all monsters" option is selected...
'    chkMonsters(1 - Index).Value = 0  ' uncheck the other option
'   &Selected Monster Only
'   &All Monsters
End Sub
Private Sub cmdAbort_Click()
   cmdAbort.Enabled = False
   bAbort = True
   bRptAbort = False
End Sub

Private Sub cmdExit_Click(Index As Integer)
    Dim i%, vbr As VbMsgBoxResult, sOldText$
    
    If Index = 0 Then
        cmdAbort.Enabled = True
'        cmdViewLog.Enabled = False
        bAbort = False
        bRptAbort = False
        nLineCtr = 500 ' init to force a new heading
        nPageCtr = 1
        
        If Dir(sOldRpt, vbNormal) > "" Then
            vbr = MsgBox("Log file already exists; overwrite it?", vbYesNo)
            If vbr = vbYes Then
                If Dir(sOldRpt, vbNormal) > "" Then ' ensure it is still there...
                    Kill sOldRpt
                    StatusBar1.SimpleText = "Log file will be overwritten"
                End If
            End If
        End If
        Set cRpt = New SpellRpt
        If (chkMonsters(1).Value) Then
            sOldText = StatusBar1.SimpleText
            For i = 0 To lstOpponents.ListCount - 1
                StatusBar1.SimpleText = "On " & FormatNumber(i + 1, 0) & " of " & FormatNumber(lstOpponents.ListCount, 0) & " monsters..."
                lstOpponents.ListIndex = i
                lstOpponents.Refresh
                Call runTest
                If bAbort Then
                    If Not bRptAbort Then
                        cRpt.AddPiece vbCrLf & sAbortMsg & vbCrLf
                        bRptAbort = True
                    End If
                    Exit For
                End If
            Next i
            StatusBar1.SimpleText = sOldText
        Else
            CurrentExcelFileName = CreateFPSFileName
            Call runTest
            If bFPSmode Then
                If chkOpenExcel.Value Then
                    Dim yesno As VbMsgBoxResult
                        yesno = MsgBox("Tests are done.  Would you like to view the spreadsheet?", vbYesNo)
                    If yesno = vbYes Then
                        Set modExcel.oExcel = New Excel.Application
                        Set modExcel.oCurrentWorkbook = oExcel.Workbooks.Open(CurrentExcelFileName)
                        modExcel.oExcel.Visible = True
                    End If
                End If
            End If
        End If
        Set cRpt = Nothing
        cmdViewLog.Enabled = (Dir(sOldRpt) > "")
        cmdAbort.Enabled = False
        bAbort = False
    Else
        Unload Me
    End If
End Sub
Public Function CreateFPSFileName() As String
        Dim FirstSpace As Integer, TimeDate As String
        
        TimeDate = Now()
        
        FirstSpace = InStr(TimeDate, " ")
        TimeDate = Mid$(TimeDate, FirstSpace)
        ReplaceStringWithString TimeDate, ":", "-"
        ReplaceStringWithString TimeDate, " ", ""
        CreateFPSFileName = "FPS-" & frmMain.sVerID & " - " & TimeDate & ".xls"
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

Private Sub process_gas(ByVal sFilePath)
    Dim sCurFile$, sFullPath$, nFF%, sCurLine$
    Dim bHaveOppnt As Boolean, sCurOppt$, nTempDef&, nLen&
    
    sCurFile = Dir(sFilePath & "\*.gas", vbNormal Or vbHidden)
    Do While sCurFile > ""
        ' deal with this file
        sFullPath = sFilePath & "\" & sCurFile
        StatusBar1.SimpleText = "Processing " & sFullPath & " ..."
        If ((GetAttr(sFullPath)) And vbDirectory) = 0 Then ' must be file
            nFF = FreeFile
            Open sFullPath For Input As #nFF   ' Open file.
            sCurOppt = ""
            bHaveOppnt = True
            Do While Not EOF(nFF)
                Line Input #nFF, sCurLine
                nTempDef = InStr(sCurLine, "[t:template,n:")
                If nTempDef > 0 Then
                    If (sCurOppt > "") And bHaveOppnt Then
                        If acceptableMonster(sCurOppt) Then
                            lstOpponents.AddItem sCurOppt  ' show no base class
                        End If
                        sCurOppt = ""
                    Else
                        bHaveOppnt = True
                    End If
                    nTempDef = nTempDef + 14
                    nLen = InStrRev(sCurLine, "]") - nTempDef
                    If nLen > 0 Then
                        sCurOppt = Mid$(sCurLine, nTempDef, nLen)
                    End If
                Else
                    If InStr(sCurLine, "Base template") > 0 Then
                        bHaveOppnt = False  ' opt current base class out: want descendents only
                    Else
                        If haveFld(sCurLine, "category_name", "good") Then bHaveOppnt = False   ' opt current this one out: want evil only, otherwise no battle ensues
                        If haveFld(sCurLine, "material", "wood") Then bHaveOppnt = False        ' reject stuff that is worn by opponents
                        If haveFld(sCurLine, "is_invincible", "true") Then bHaveOppnt = False   ' reject stuff that is invincible
                        If InStr(sCurLine, "alignment") > 0 Then
                            If Not haveFld(sCurLine, "alignment", "evil") Then bHaveOppnt = False   ' alignment evil only acceptable monster bw 10/17/01
                        End If
                    End If
                End If
            Loop
            Close nFF
            If (sCurOppt > "") And bHaveOppnt Then  ' process last opponent found in file
                If acceptableMonster(sCurOppt) Then
                    lstOpponents.AddItem sCurOppt
                    sCurOppt = ""
                End If
            End If
            nFF = 0
        End If
        DoEvents
        sCurFile = Dir()
    Loop
End Sub
Private Function acceptableMonster(ByVal sMonster$) As Boolean
    Dim sLeftSample$
    
    sLeftSample = Left$(sMonster, 3)
    acceptableMonster = InStr(sMonster, "base") = 0 And InStr(sMonster, "BASE") = 0 And _
                        StrComp(sLeftSample, "1w_", vbTextCompare) <> 0 And _
                        StrComp(sLeftSample, "2w_", vbTextCompare) <> 0 And _
                        StrComp(sLeftSample, "3w_", vbTextCompare) <> 0

    If acceptableMonster Then
        sLeftSample = Left$(sMonster, 6)
        acceptableMonster = StrComp(sLeftSample, "dragon", vbTextCompare) <> 0 And _
                            StrComp(sLeftSample, "pupod_", vbTextCompare) <> 0 And _
                            StrComp(sLeftSample, "reaper", vbTextCompare) <> 0
    End If
End Function

Private Sub process_subfolder(ByVal sRootdir$)
    Dim sSubFolder$, sFullPath$
    
    sSubFolder = Dir(sRootdir & "\*.*", vbDirectory Or vbHidden)
    Do While sSubFolder > ""
        If Left$(sSubFolder, 1) <> "." Then
            ' deal with this subfolder
            sFullPath = sRootdir & "\" & sSubFolder
            StatusBar1.SimpleText = "Processing " & sFullPath & " ..."
            If ((GetAttr(sFullPath)) And vbDirectory) <> 0 Then
                ReDim Preserve sDirs(ndir + 1)
                sDirs(ndir) = sFullPath
                ndir = ndir + 1
                haveDir = True
            End If
        End If
        DoEvents
        sSubFolder = Dir()
    Loop
End Sub

Private Sub cmdViewLog_Click()
#Const bUseMyForm = True
#If Not bUseMyForm Then
    Dim dRtVal#
#End If

    If Dir(sOldRpt) > "" Then
#If bUseMyForm Then
        With frmRptLog
            .sFile = sOldRpt
            .Show vbModal
        End With
#Else
        dRtVal = Shell("notepad " & sOldRpt, vbNormalFocus)
        Debug.Assert dRtVal <> 0#
#End If
    Else
        StatusBar1.SimpleText = "Log file for this run was not found!"
    End If
End Sub

Private Sub Form_Load()
    Dim j%, k%, startx%, endx%
    Dim nFF% ' prepare to open spell specific gas file:
    Dim sDummy$, nSpellDef%, nStartSpellPos%
    Dim lix As ListItem
    Dim i%, sSearchDef$
    Dim bKeepThisLine As Boolean, bMonsterDerived As Boolean, bOKdTargetType As Boolean
    Dim sSpellSpec$
        
    bInProcess = False
    
    sOldRpt = frmMain.sCurDir & "\SpellLog.txt"
    cmdViewLog.Enabled = (Dir(sOldRpt) > "")
    ' populate the opponent list box
    ' to be fleshed in later from appropriate gas files
    lstOpponents.Clear
    ndir = 0
    haveDir = False
    If (Dir(frmMain.sCurDir & "\world\contentdb\templates\regular", vbDirectory) > "") Then
        process_subfolder frmMain.sCurDir & "\world\contentdb\templates\regular\actors\evil"
    Else
        process_subfolder frmMain.sCurDir & "\world\contentdb\templates\actors\evil"
    End If
    j = 0
    Do While haveDir
        startx = j
        endx = ndir - 1
        j = ndir + 1
        haveDir = False
        DoEvents
        For k = startx To endx
           process_subfolder (sDirs(k))
        Next k
    Loop
    StatusBar1.SimpleText = "Found " & CStr(ndir) & " subfolders." ' 0 to ndir - 1
    For j = 0 To ndir - 1
        Call process_gas(sDirs(j))
    Next j
    If (lstOpponents.ListIndex = -1) And (lstOpponents.ListCount > 0) Then
        lstOpponents.ListIndex = 0  ' default to first item in list...
        StatusBar1.SimpleText = "List of opponents contains " & FormatNumber(lstOpponents.ListCount, 0) & " monsters"
    End If
'    chkMonsters(1).Value = True ' done by line below...
    
    bAbort = False
    bRptAbort = False
    
    frmMain.bInAllSpells = True
    On Error GoTo erh
'        On Error GoTo 0   ' bring up vb dialog box
    nFF = FreeFile
    
'    "world\contentdb\templates\interactive\spl_spell.gas"
    
    If (Dir(frmMain.sCurDir & "\world\contentdb\templates\regular", vbDirectory) > "") Then
        Open frmMain.sCurDir & "\world\contentdb\templates\regular\interactive\spl_spell.gas" For Input As #nFF   ' Open file.
    Else
        Open frmMain.sCurDir & "\world\contentdb\templates\interactive\spl_spell.gas" For Input As #nFF   ' Open file.
    End If
    Set lix = Nothing
    Do While Not EOF(nFF)
        Line Input #nFF, sDummy
        If Len(sDummy) > 7 Then ' impose arbitrary min size of interest
            nSpellDef = InStr(sDummy, "t:template,n:spell")
            If nSpellDef > 0 Then
                If lstSpells.ListItems.Count > 0 Then ' have a prior entry
                    bKeepThisLine = findApprovedSkrits(sSpellSpec)
                    
                    ' the rules for removing are from Eric Tams 10/2/2001:
                    If (IsNumeric(lix.SubItems(6))) Then
                        bKeepThisLine = bKeepThisLine And (CInt(lix.SubItems(6)) < 200)
                    End If
                    If bMonsterDerived Or (Not bKeepThisLine) Or _
                            (Not bOKdTargetType) Then
                        lstSpells.ListItems.Remove lix.Index
                    Else
                        If Trim(lix.SubItems(6)) = "" Then
                            lix.SubItems(6) = "0"  ' have a default required level of 0
                        End If
                    End If
                    Set lix = Nothing
                 End If
                 nStartSpellPos = nSpellDef + 13
                 Set lix = lstSpells.ListItems.Add(, , Mid$(sDummy, nStartSpellPos, InStr(sDummy, "]") - nStartSpellPos))
                 bKeepThisLine = False
                 bMonsterDerived = False
                 bOKdTargetType = False
                 sSpellSpec = ""
            Else
                If Not lix Is Nothing Then
                     ' add subitems here:
                    For i = 1 To 7
                        sSearchDef = RTrim(lstSpells.ColumnHeaders(i + 1).Text) + " "
                        nStartSpellPos = InStr(sDummy, sSearchDef)
                        If (nStartSpellPos > 0) Then
                            nSpellDef = nStartSpellPos + Len(sSearchDef)
                            lix.SubItems(i) = Mid$(sDummy, nSpellDef + 2, Len(sDummy) - nSpellDef - 2)
                            nStartSpellPos = InStr(lix.SubItems(i), ";")  ' remove trailing semicolons
                            If nStartSpellPos > 1 Then
                                lix.SubItems(i) = Left$(lix.SubItems(i), nStartSpellPos - 1)
                            End If
                            If (i = 1) Then ' define the class as defined by the specialization : good -> nature; dark, monster -> combat
                                If InStr(lix.SubItems(i), "good") > 0 Then
                                    lix.SubItems(8) = "Nature"
                                Else
                                    lix.SubItems(8) = "Combat"   '  dark
                                    If InStr(lix.SubItems(i), "monster") > 0 Then
        Rem  You can't test some monster spells by making the player cast them. You might  <<<  email from eric tams
        Rem  want to just focus on things not specialized from base_spell_monster
                                        bMonsterDerived = True
                                    End If
                                End If
                            Else
                                If (i = 7) And (Len(lix.SubItems(7)) > 2) Then ' take out the " delims
         '                           Debug.Assert InStr(lix.SubItems(7), "Dancing") = 0
                                    lix.SubItems(7) = Mid$(lix.SubItems(7), 2, Len(lix.SubItems(7)) - 2)
                                End If
                            End If
                            nStartSpellPos = InStr(lix.SubItems(i), ";")
                            If nStartSpellPos > 1 Then
                                lix.SubItems(i) = Left$(lix.SubItems(i), nStartSpellPos - 1)
                            End If
                            Exit For ' have a match - no need to stay in for loop
                        End If
                    Next i
                    nSpellDef = InStr(sDummy, "[spell")
                    If nSpellDef > 1 Then
                        sSpellSpec = Mid$(sDummy, nSpellDef + 1, Len(sDummy) - 1 - nSpellDef)
                        nSpellDef = InStr(sSpellSpec, "]")
                        If (nSpellDef > 1&) Then
                            sSpellSpec = Left$(sSpellSpec, nSpellDef - 1&)
                        End If
                    End If ' bOKdTargetType
                    nSpellDef = InStr(sDummy, "target_type_flags")
                    If nSpellDef > 1 Then
                        bOKdTargetType = (InStr(sDummy, "tt_breakable") > 0) Or ((InStr(sDummy, "tt_conscious_enemy") > 0) And _
                                          (InStr(sDummy, "tt_unconscious_enemy") > 0))
                    End If ' bOKdTargetType
                End If
            End If
        End If
    Loop
    Close nFF
    If bFPSmode Then
        optSpell(0).Value = True  ' select current spell
        chkMonsters(0).Value = True
        chkMonsters(0).Enabled = False
        chkMonsters(1).Enabled = False
'        optSpell(0).Enabled = False
'        optSpell(1).Enabled = False
        Me.Caption = "FPS test with 8 in party, casting the chosen spell"
        Me.Height = 7320
        lstHeroType.Clear
        lstHeroType.AddItem "FarmBoy"
        lstHeroType.AddItem "FarmGirl"
        lstHeroType.ListIndex = 1
    Else
        chkMonsters(0).Enabled = True
        chkMonsters(1).Enabled = True
        optSpell(0).Enabled = True
        optSpell(1).Enabled = True
        optSpell(1).Value = True
        chkMonsters_Click (1)
        Me.Caption = "Current Spell listing"
        Me.Height = 6810
        lblParty.Visible = False
        lstHeroType.Visible = False
        chkOpenExcel.Visible = False
        chkRepeat.Visible = False
        lblSampleTime.Visible = False
        txtSampleTime.Visible = False
    End If
    cmdExit(0).Enabled = True
    cmdExit(1).Enabled = True
    Exit Sub
erh:
   StatusBar1.SimpleText = "Form load Error " & CStr(Err.Number) & ": " & Err.Description
   Err.Clear
End Sub

Private Sub Form_QueryUnload(Cancel As Integer, UnloadMode As Integer)
    If (bInProcess) Then
        Cancel = True
    End If
End Sub

Private Sub Form_Unload(Cancel As Integer)
    With frmMain
        .cmdLaunch.Enabled = True
        .cmdExit.Enabled = True
        .bInAllSpells = False
    End With
End Sub

#If Not SIMULATE Then

Private Function ResultOfSpellTest(ByVal nLineNum%) As Integer
    Dim dRsp#
    Dim sFullPath$, sMySkrit$, sType$
    Dim i%
    
    ResultOfSpellTest = 1 ' 0 => success, 1 => failure
    
    On Error GoTo erh
    If bFPSmode Then
        sMySkrit = "spellParty"         ' frmMain.sSpellFPS
    Else
        sMySkrit = "TestAllSpellOppt"   ' pass opponent on cmd line: opponent=
    End If
    
    Select Case frmMain.tChosen
        Case tdebug
            sType = "D"
        Case tRelease
            sType = "R"
        Case Else ' retail
            sType = ""
    End Select
    
    sFullPath = frmMain.sCurDir & "\DungeonSiege" & sType & ".exe" '
    
    If Dir(sFullPath) = "" Then
        StatusBar1.SimpleText = "DungeonSiege" & sType & ".exe was not found at " & frmMain.sCurDir  '  H:\PreCompiled\" & frmMain.sVerID & "  !"
        Exit Function
    Else                                                                 ' skrit_retry=true
        sFullPath = sFullPath & "  skritbot=auto\test\" & sMySkrit & "   map=Halo_test_map  teleport=start "
        If frmMain.bDemo Then
           sFullPath = sFullPath & " demo=true "
        End If
        If frmMain.bRetrySkrit Then
           sFullPath = sFullPath & " skrit_retry=true "
        End If
        If frmMain.bWindowed Then
            sFullPath = sFullPath & " fullscreen=false"
        End If
          '
        For i = 1 To 9
            If i = 1 Then
                If bFPSmode Then
                    sFullPath = sFullPath & " spell=" & lstSpells.ListItems(nLineNum).Text
                Else
                    sFullPath = sFullPath & " " & lstSpells.ColumnHeaders(i).Text & "=" & lstSpells.ListItems(nLineNum).Text
                End If
            Else
                sFullPath = sFullPath & " " & lstSpells.ColumnHeaders(i).Text & "=" & lstSpells.ListItems(nLineNum).SubItems(i - 1)
            End If
        Next i
        sFullPath = sFullPath & " Opponent=" & sOpponent
        sFullPath = sFullPath & " Invincible=true" '  an afterthought: to give actor maximum chance to kill opponent
        If bFPSmode Then
            sFullPath = sFullPath & "  fpslog=true  clones=" & lstHeroType.List(lstHeroType.ListIndex) & " repeat=" & IIf(chkRepeat.Value, "true", "false")
            If Len(Trim(txtSampleTime.Text)) > 0 Then
                sFullPath = sFullPath & " MAXTime=" & txtSampleTime.Text
            End If
        End If
        Debug.Print "Cmd = " & sFullPath
    End If
    frmMain.sLaunchDT = Now()
    frmMain.bTestComplete = False
    
#If bAppendToVar Then
    sFullPath = sFullPath & vbCr
#Else
    sResultsFile = Environ$("computername") & MyTimeStamp(frmMain.sLaunchDT) & ".log"
    sFullPath = sFullPath & "  FileName=" & sResultsFile & vbCr
'    sResultsFile = Environ$("computername") & ".log"   '  & MyTimeStamp(sLaunchDT)  && this is the old way - writing to skrit file
'    Call WriteSkrit(sCurDir & "\auto\test\logfile.skrit", _
'                    sResultsFile)                                                   && no longer necessary with build 9.2402
    sResultsFile = sCurDir & "\" & sResultsFile
    For i = 0 To 2
        frmMain.txtWarnings(i).Text = ""
    Next i
#End If
    ChDrive Left$(frmMain.sCurDir, 1)    ' "h"
    ChDir frmMain.sCurDir
'    Debug.Print "running " & sFullPath
    
    dRsp = Shell(sFullPath, vbNormalFocus)
    Debug.Print "value of dRsp is " & Format(dRsp, "#,###,###.0##")
    Debug.Assert dRsp <> 0#
    If dRsp <> 0# Then
        frmMain.StatusBar1.SimpleText = "Launched DungeonSiege" & sType & " with " & lstSpells.ListItems(nLineNum).Text & " against " & sOpponent & " OK ..."
        frmMain.Timer1.Enabled = True
        Do While Not frmMain.bTestComplete
            ' wait for frmmain.timer1 to fire ...
            DoEvents
        Loop                 '  \/  location of pass indicator in log file: > 0 => success
        ResultOfSpellTest = IIf(frmMain.nTestResult = 0, 0, 1)  ' 0 => success, 1 => failure
        StatusBar1.SimpleText = "Test " & lstSpells.ListItems(nLineNum).Text & " vs " & sOpponent & " complete."
    Else
        frmMain.StatusBar1.SimpleText = "DungeonSiege" & sType & " with " & lstSpells.ListItems(nLineNum).Text & " against " & sOpponent & " did NOT launch!"
    End If
    Exit Function
erh:
   frmMain.StatusBar1.SimpleText = "ResultOfSpellTest() Error " & CStr(Err.Number) & ": " & Err.Description
   Err.Clear
End Function
#End If

Function haveFld(ByVal sCand$, ByVal sProperty$, ByVal sValue$) As Boolean
    Dim nGood1%, nGood2%
    
    haveFld = False
    nGood1 = InStr(sCand, sProperty)
    If nGood1 > 0 Then
        nGood2 = InStr(sCand, sValue)
        If (nGood2 > nGood1) Then
            haveFld = True
        End If
    End If
End Function

Private Function findApprovedSkrits(ByVal sDesigSkrit$) As Boolean
    findApprovedSkrits = (sDesigSkrit = "spell_area_effect") Or (sDesigSkrit = "spell_chain_attack") Or (sDesigSkrit = "spell_deathrain") Or (sDesigSkrit = "spell_default") Or _
                         (sDesigSkrit = "spell_fire") Or (sDesigSkrit = "spell_instant_hit") Or (sDesigSkrit = "spell_launch") Or (sDesigSkrit = "spell_lightning") Or _
                         (sDesigSkrit = "spell_turret")
End Function
Private Sub txtSampleTime_KeyPress(KeyAscii As Integer)
    If Not Between(KeyAscii, Asc("0"), Asc("9")) Then
        KeyAscii = 0
    End If
End Sub
