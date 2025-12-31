Attribute VB_Name = "modSpells"
Option Explicit
Public sOldRpt$
Public Const sAstrskMsg$ = "***"
Public Const sAbortMsg$ = sAstrskMsg & " Aborted by User " & sAstrskMsg

Public Type spellAttrs
    template As String * 25
    specializes As String * 20
    damage_max  As Integer
    damage_min  As Integer
    is_one_shot As Boolean
    mana_cost  As Integer
    required_level As Integer
    screen_name As String * 25
    Class As String * 10
End Type
Public mySpells() As spellAttrs  ' track spells
Public nSpellCount%
Public ColumnHeaders(8) As String ' template,specializes to class : 0..8

Public sDirs()  As String        ' track gas files where monsters are defined
Public nDir&
Public haveDir As Boolean, bAbort  As Boolean, bPause As Boolean
Public bAllMonsters As Boolean, bAllSpells As Boolean, bInProcess As Boolean
Public bInAllSpells As Boolean

Dim nMaxErrors&, nMaxPass&
Dim tErrLookup()
Dim nOldStrt&, nOldLen&

' incorporate a "run from last monster, spell" function
Public sLastMonster$, sLastSpell$
Public sDetailRpt$, sSummaryRpt$
Public bSplashOn As Boolean
Public sCurBld As String, sCurDateTimeFile$

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
        If Not ((sCH = " ") Or (sCH = vbTab) Or (sCH = "_")) Then
            RmvSpace = RmvSpace & sCH
        End If
    Next i
End Function
Private Function acceptableMonster(ByVal sMonster$) As Boolean
    Dim sLeftSample$  ' bw 1/2/2002 opt out these : *_act, caged_phrak, skeleton_mercenary_archer_reveal and skeleton_mercenary_reveal
    
    sLeftSample = Left$(sMonster, 3)
    acceptableMonster = InStr(sMonster, "base") = 0 And InStr(sMonster, "BASE") = 0 And _
                        StrComp(sLeftSample, "1w_", vbTextCompare) <> 0 And _
                        StrComp(sLeftSample, "2w_", vbTextCompare) <> 0 And _
                        StrComp(sLeftSample, "3w_", vbTextCompare) <> 0

    If acceptableMonster Then
        sLeftSample = Left$(sMonster, 6)
        acceptableMonster = StrComp(sLeftSample, "dragon", vbTextCompare) <> 0 And _
                            StrComp(sLeftSample, "pupod_", vbTextCompare) <> 0 And _
                            StrComp(sLeftSample, "reaper", vbTextCompare) <> 0 And _
                            StrComp(sLeftSample, "caged_", vbTextCompare) <> 0
    End If
    If acceptableMonster Then
        sLeftSample = Right$(sMonster, 4)
        acceptableMonster = (StrComp(sLeftSample, "_act", vbTextCompare) <> 0) And (StrComp(sLeftSample, "body", vbTextCompare) <> 0)
    End If
    If acceptableMonster Then
        acceptableMonster = StrComp(sMonster, "skeleton_mercenary_archer_reveal", vbTextCompare) <> 0
    End If
    If acceptableMonster Then
        acceptableMonster = StrComp(sMonster, "skeleton_mercenary_reveal", vbTextCompare) <> 0
    End If
End Function
Public Sub process_gas(ByVal sFilePath)       '  BW 12/12/01  opt out unkillable dragon / pupod_0* / reaper
    Dim sCurFile$, sFullPath$, nFF%, sCurLine$
    Dim bHaveOppnt As Boolean, sCurOppt$, nTempDef&, nLen&
    
    sCurFile = Dir(sFilePath & "\*.gas", vbNormal Or vbHidden)
    Do While sCurFile > ""
        ' deal with this file
        sFullPath = sFilePath & "\" & sCurFile
'        StatusBar1.SimpleText = "Processing " & sFullPath & " ..."
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
'                            lstOpponents.AddItem sCurOppt  ' show no base class
                            frmMain.cmbMonsterList.AddItem sCurOppt
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
                        If haveFld(sCurLine, "doc", "dead") Then bHaveOppnt = False   ' opt dead ones out; otherwise, meaningless
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
                'lstOpponents.AddItem sCurOppt
                If acceptableMonster(sCurOppt) Then
                    frmMain.cmbMonsterList.AddItem sCurOppt
                End If
                sCurOppt = ""
            End If
            nFF = 0
        End If
        DoEvents
        sCurFile = Dir()
    Loop
End Sub
Public Sub process_subfolder(ByVal sRootdir$)
    Dim sSubFolder$, sFullPath$
    
    sSubFolder = Dir(sRootdir & "\*.*", vbDirectory Or vbHidden)
    Do While sSubFolder > ""
        If Left$(sSubFolder, 1) <> "." Then
            ' deal with this subfolder
            sFullPath = sRootdir & "\" & sSubFolder
 '           StatusBar1.SimpleText = "Processing " & sFullPath & " ..."
            If ((GetAttr(sFullPath)) And vbDirectory) <> 0 Then
                ReDim Preserve sDirs(nDir + 1)
                sDirs(nDir) = sFullPath
                nDir = nDir + 1
                haveDir = True
            End If
        End If
        DoEvents
        sSubFolder = Dir()
    Loop
End Sub

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
'  BW  3/7/2002 add spell_damage_volume as valid spell  !!!
Public Function findApprovedSkrits(ByVal sDesigSkrit$) As Boolean
    findApprovedSkrits = (sDesigSkrit = "spell_area_effect") Or (sDesigSkrit = "spell_chain_attack") Or (sDesigSkrit = "spell_deathrain") Or (sDesigSkrit = "spell_default") Or _
                         (sDesigSkrit = "spell_fire") Or (sDesigSkrit = "spell_instant_hit") Or (sDesigSkrit = "spell_launch") Or (sDesigSkrit = "spell_lightning") Or _
                         (sDesigSkrit = "spell_turret") Or (sDesigSkrit = "spell_damage_volume")
End Function




Public Sub lookup_monsters_spells()
    Dim j%, k%, startx%, endx%
    Dim nFF% ' prepare to open spell specific gas file:
    Dim sDummy$, nSpellDef&, nStartSpellPos%
    Dim i%, sSearchDef$
    Dim bKeepThisLine As Boolean, bMonsterDerived As Boolean, bOKdTargetType As Boolean
    Dim sSpellSpec$, sSpellGas$, sCurItem$
    Dim lix As ListItem
    Dim chx As ColumnHeader
    
    nSpellCount = 0    ' start from square 1
    ' define spell attributes:  ' will be eventually used in form to display spell templates
    ColumnHeaders(0) = "Template"
    ColumnHeaders(1) = "specializes"
    ColumnHeaders(2) = "damage_max"
    ColumnHeaders(3) = "damage_min"
    ColumnHeaders(4) = "is_one_shot"
    ColumnHeaders(5) = "mana_cost"
    ColumnHeaders(6) = "required_level"
    ColumnHeaders(7) = "screen_name"
    ColumnHeaders(8) = "Class"
        
    frmMain.lvSpells.Left = frmMain.RichTextBox1.Left
    frmMain.lvSpells.Width = frmMain.RichTextBox1.Width
    frmMain.lvSpells.Top = frmMain.RichTextBox1.Top
    frmMain.lvSpells.Height = frmMain.RichTextBox1.Height
    modSpells.bInProcess = False
    ' populate the opponent list box
    ' bw 10/11/2001  general spell/monster init
    modParse.sCurDir = frmMain.txtExepath.Text
    modSpells.sOldRpt = modParse.sCurDir & "\SpellLog.txt"
    ' to be fleshed in later from appropriate gas files
    frmMain.cmbMonsterList.Clear
    nDir = 0
    modSpells.haveDir = False
    If CurrentUserInfo.CurrentBuild < "0.2.1.2302" Then
        modSpells.process_subfolder modParse.sCurDir & "\world\contentdb\templates\actors\evil"
    Else
        modSpells.process_subfolder modParse.sCurDir & "\world\contentdb\templates\regular\actors\evil"
    End If
    j = 0
    Do While modSpells.haveDir
        startx = j
        endx = nDir - 1
        j = nDir + 1
        modSpells.haveDir = False
        DoEvents
        For k = startx To endx
           modSpells.process_subfolder (sDirs(k))
        Next k
    Loop
'    MsgBox "Found " & CStr(nDir) & " subfolders." ' 0 to ndir - 1
    frmMain.cmbMonsterList.Clear
    For j = 0 To nDir - 1
        Call modSpells.process_gas(sDirs(j))
    Next j
    frmMain.cmbMonsterList.AddItem "[ All Monsters ]"
'    chkMonsters(1).Value = True ' done by line below...
'    chkMonsters_Click (1)
    bAbort = False
    
    modSpells.bInAllSpells = True
 '   On Error GoTo erh
            On Error GoTo 0   ' bring up vb dialog box
    nFF = FreeFile
    
'    "world\contentdb\templates\interactive\spl_spell.gas"
    If CurrentUserInfo.CurrentBuild < "0.2.1.2302" Then
        sSpellGas = modParse.sCurDir & "\world\contentdb\templates\interactive\spl_spell.gas"
    Else
        sSpellGas = modParse.sCurDir & "\world\contentdb\templates\regular\interactive\spl_spell.gas"
    End If
    If Dir(sSpellGas, vbNormal) > "" Then
        Open sSpellGas For Input As #nFF     ' Open file.
        
        Do While Not EOF(nFF)
            Line Input #nFF, sDummy
            If Len(sDummy) > 7 Then ' impose arbitrary min size of interest
                nSpellDef = InStr(sDummy, "t:template,n:spell")
                If nSpellDef > 0& Then
                     If nSpellCount > 0 Then  ' have a prior entry
                        bKeepThisLine = modSpells.findApprovedSkrits(sSpellSpec)
                        
                        ' the rules for removing are from Eric Tams 10/2/2001:
                        bKeepThisLine = bKeepThisLine And (CInt(mySpells(nSpellCount - 1).required_level) < 200)
                        If bMonsterDerived Or (Not bKeepThisLine) Or _
                                (Not bOKdTargetType) Then
 '                           MsgBox "Template " & CStr(nSpellCount - 1) & mySpells(nSpellCount - 1).template & " is now removed"
                            nSpellCount = nSpellCount - 1
                            If (nSpellCount > 0) Then
                                ReDim Preserve mySpells(nSpellCount - 1)   ' remove last entry  spellcount of n => 0 to n-1
                            Else
                                ReDim mySpells(0)    ' remove last entry
                            End If
                        End If
                     End If
                     nStartSpellPos = nSpellDef + 13&
                     sDummy = RTrim(Mid$(sDummy, nStartSpellPos, InStr(sDummy, "]") - nStartSpellPos))
                     ' a valid spell
'                     If InStr(sDummy, "]") = 0 Then Debug.Assert False
                     ReDim Preserve mySpells(nSpellCount)
                     mySpells(nSpellCount).template = sDummy
 '                    MsgBox "Template " & CStr(nSpellCount) & " is now " & mySpells(nSpellCount).template
 '                    Debug.Assert Trim(mySpells(nSpellCount).template) <> "spell_zap"
     '                Debug.Assert Trim(mySpells(nSpellCount).template) <> "spell_flash"
                     nSpellCount = nSpellCount + 1  ' prepare for next one
                     bKeepThisLine = False
                     bMonsterDerived = False
                     bOKdTargetType = False
                     sSpellSpec = ""
                Else
                    If (nSpellCount > 0) Then ' have entry
                         ' add subitems here:
                        For i = 1 To 8
                            sSearchDef = RTrim(ColumnHeaders(i)) + " "
                            nStartSpellPos = InStr(sDummy, sSearchDef)
                            If (nStartSpellPos > 0) Then
                                nSpellDef = nStartSpellPos + Len(sSearchDef)
                                sCurItem = Mid$(sDummy, nSpellDef + 2&, Len(sDummy) - nSpellDef - 2&)
                                nStartSpellPos = InStr(sCurItem, ";")  ' remove trailing semicolons
                                If nStartSpellPos > 1 Then
                                    sCurItem = Left$(sCurItem, nStartSpellPos - 1)
                                End If
                                Select Case i  ' remember that vb appends spaces to sCuritem in these assignments below...
                                    Case 1
                                        mySpells(nSpellCount - 1).specializes = sCurItem
                                    Case 2
                                        If IsNumeric(sCurItem) Then
                                            mySpells(nSpellCount - 1).damage_max = CInt(sCurItem)
                                        End If
                                    Case 3
                                        If IsNumeric(sCurItem) Then
                                            mySpells(nSpellCount - 1).damage_min = CInt(sCurItem)
                                        End If
                                    Case 4
                                        mySpells(nSpellCount - 1).is_one_shot = CBool(sCurItem)
                                    Case 5
                                        If IsNumeric(sCurItem) Then
                                            mySpells(nSpellCount - 1).mana_cost = CInt(sCurItem)
                                        End If
                                    Case 6
                                        If IsNumeric(sCurItem) Then
                                            mySpells(nSpellCount - 1).required_level = CInt(sCurItem)
                                        End If
                                    Case 7
                                        mySpells(nSpellCount - 1).screen_name = sCurItem
'                                    Case 8  do nothing - detected by good / dark etc. below
                                End Select
                                If (i = 1) Then ' define the class as defined by the specialization : good -> nature; dark, monster -> combat
                                    If InStr(mySpells(nSpellCount - 1).specializes, "good") > 0 Then
                                        mySpells(nSpellCount - 1).Class = "Nature"
                                    Else
                                        mySpells(nSpellCount - 1).Class = "Combat"   '  dark
                                        If InStr(mySpells(nSpellCount - 1).specializes, "monster") > 0 Then
            Rem  You can't test some monster spells by making the player cast them. You might  <<<  email from eric tams
            Rem  want to just focus on things not specialized from base_spell_monster
                                            bMonsterDerived = True
                                        End If
                                    End If
                                Else
                                    If (i = 7) And (Len(sCurItem) > 2) Then ' take out the " delims
                                        mySpells(nSpellCount - 1).screen_name = Mid$(sCurItem, 2, Len(sCurItem) - 2)
                                    End If
                                End If
                                Exit For ' we have a match - no need to continue looking...
                            End If
                        Next i
                        nSpellDef = InStr(sDummy, "[spell")
                        If nSpellDef > 1& Then
                            sSpellSpec = Mid$(sDummy, nSpellDef + 1&, Len(sDummy) - 1& - nSpellDef)
                            nSpellDef = InStr(sSpellSpec, "]")
                            If (nSpellDef > 1&) Then
                                sSpellSpec = Left$(sSpellSpec, nSpellDef - 1&)
                            End If
                        End If
                        nSpellDef = InStr(sDummy, "target_type_flags")
                        If nSpellDef > 1& Then
                            bOKdTargetType = (InStr(sDummy, "tt_breakable") > 0) Or ((InStr(sDummy, "tt_conscious_enemy") > 0) And _
                                              (InStr(sDummy, "tt_unconscious_enemy") > 0))
                        End If ' bOKdTargetType
                    End If
                End If
            End If
        Loop
        Close nFF
    Else
        MsgBox sSpellGas & " was not found!"
    End If
    If nSpellCount > 0 Then  ' have a prior entry
        nSpellCount = nSpellCount - 1
        bKeepThisLine = modSpells.findApprovedSkrits(sSpellSpec)
        If bMonsterDerived Or Not bKeepThisLine Then
            nSpellCount = nSpellCount - 1
            ReDim Preserve mySpells(nSpellCount)  ' remove last entry
        End If
    End If
    With frmMain
        .cmbSpellList.Clear
        .lvSpells.ListItems.Clear ' problem with original way: array alphabetical, control sorte
    End With
    If (nSpellCount > 0) Then
        Call modSpells.MySort(mySpells, nSpellCount)
    End If
    frmMain.lvSpells.ColumnHeaders.Clear
    For i = 0 To 8
        Set chx = frmMain.lvSpells.ColumnHeaders.Add(, , ColumnHeaders(i))
        Select Case i
            Case 0
                chx.Width = 1883.68
            Case 1
                chx.Width = 1540.25
            Case 5
                chx.Width = 1069.79
            Case 6
                chx.Width = 1296#
            Case 7
                chx.Width = 1536.03
            Case 8
                chx.Width = 1018#
            Case Else
                chx.Width = 1400#
        End Select
        Set chx = Nothing
    Next i
    If (nSpellCount > 0) Then
        For i = 0 To nSpellCount '- 1
    '        Debug.Assert Trim(mySpells(i).screen_name) > ""
            If Asc(mySpells(i).screen_name) = 0 Then
                frmMain.cmbSpellList.AddItem Mid$(mySpells(i).template, 1 + InStr(mySpells(i).template, "_"))  ' drop spell_ prefix
                Debug.Print "added " & mySpells(i).template
            Else
                frmMain.cmbSpellList.AddItem mySpells(i).screen_name
                Debug.Print "added " & mySpells(i).screen_name
            End If
            
            Set lix = frmMain.lvSpells.ListItems.Add(, , mySpells(i).template)
            With lix
                .SubItems(1) = mySpells(i).specializes
                .SubItems(2) = CStr(mySpells(i).damage_max)
                .SubItems(3) = CStr(mySpells(i).damage_min)
                .SubItems(4) = CStr(mySpells(i).is_one_shot)
                .SubItems(5) = CStr(mySpells(i).mana_cost)
                .SubItems(6) = Format(mySpells(i).required_level, "##0")
                .SubItems(7) = CStr(mySpells(i).screen_name)
                .SubItems(8) = mySpells(i).Class
            End With
            Set lix = Nothing
        Next i
        frmMain.cmbSpellList.AddItem "[ All Spells ]"
        With frmMain
            .cmbMonsterList.ListIndex = .cmbMonsterList.ListCount - 1
            .cmbSpellList.ListIndex = .cmbSpellList.ListCount - 1
            .cmdViewSpells.Enabled = (nSpellCount > 0)
            .cmdSpellsRun.Enabled = (Dir(modParse.sCurDir & "\dungeonsiege" & IIf(.optRelease.Value, "R", "D") & ".exe", vbNormal) > "")
        End With
    Else
        frmMain.cmbSpellList.AddItem "[ All Spells ]"
    End If
 '   cmdExit(0).Enabled = True
 '   cmdExit(1).Enabled = True
    modSpells.bAllMonsters = True
    modSpells.bAllSpells = True
    modSpells.bInAllSpells = False
    Exit Sub
erh:
   MsgBox "TstAllSpells Error " & CStr(Err.Number) & ": " & Err.Description  '  StatusBar1.SimpleText =
   modSpells.bInAllSpells = False
   Err.Clear
End Sub

Function SetDBasConnStr(ByVal sDBname$)  ' from ConnectionString
    Dim nEQ%, nSemiCol%, nDB%
    
    SetDBasConnStr = ConnectionString
    nDB = InStrRev(ConnectionString, "database")
    If nDB > 0 Then
        nEQ = InStr(nDB + 8, ConnectionString, "=")
        If (nEQ > nDB) Then
            nSemiCol = InStr(nEQ + 1, ConnectionString, ";")
            If (nSemiCol > nEQ) Then
                SetDBasConnStr = Left$(ConnectionString, nEQ) & sDBname & Mid$(ConnectionString, nSemiCol)
            End If
        End If
    End If
End Function

Public Sub lookup_buildDTmac()
  '  select From table
  Dim cnConnection As New ADODB.Connection
  Dim cmdCommand As New ADODB.Command
  Dim rstCases As New ADODB.Recordset
  Dim lix As ListItem, myConnStr$
  Dim nMostRecent%  ' get most recent build id NUMERICALLY
  
  myConnStr = ConnectionString  '  SetDBasConnStr("SpellLogs")
  
  With cnConnection
        .ConnectionString = myConnStr    ' was ConnectionString - need new db
        .ConnectionTimeout = 100
        .Open
  End With
  Set cmdCommand.ActiveConnection = cnConnection
  cmdCommand.CommandText = "select distinct  BuildNumber, DateTimeStamp, MachineName from SpellResults"
  cmdCommand.CommandTimeout = 150
  
  With rstCases
        'Debug.Print "Table-type recordset: " & .Name
        On Error Resume Next
        .Open cmdCommand, , adOpenStatic, adLockBatchOptimistic
        If Err.Number <> 0 Then
            MsgBox "modspells.lookup_buildDTmac error " & CStr(Err.Number) & " : " & Err.Description
            Err.Clear
        Else
            frmMain.lstvBuildDTMachine.ListItems.Clear
            If bSplashOn Then
                With frmSplash.ProgressBar1
                    .Max = rstCases.RecordCount
                    .Value = 0&
                    .Refresh
                End With
                frmSplash.lblLoad.Caption = "Load Progress: list all spell runs"
            End If
            Do While Not .EOF
                DoEvents
                If bSplashOn Then
                    With frmSplash
                        .ProgressBar1.Value = frmSplash.ProgressBar1.Value + 1&
                        If ((.ProgressBar1.Value Mod (.ProgressBar1.Max \ 5&)) = 0) Then .Refresh
                    End With
                End If

                Set lix = frmMain.lstvBuildDTMachine.ListItems.Add(, , Trim(rstCases("BuildNumber")))
                If Not lix Is Nothing Then
                    With lix
                        .SubItems(1) = Trim(rstCases("DateTimeStamp"))
                        .SubItems(2) = Trim(rstCases("MachineName"))
                        .SubItems(3) = Trim(rstCases("Pass"))
                    End With
                    Set lix = Nothing
                Else
                    Debug.Assert False ' flag this for debugging
                End If
                .MoveNext
            Loop
    
            .Close
        End If
  End With
  On Error GoTo 0
  Set rstCases = Nothing
  Set cmdCommand = Nothing
  cnConnection.Close
  Set cnConnection = Nothing ' release memory taken
  If (frmMain.lstvBuildDTMachine.ListItems.Count > 0) Then
     nMostRecent = GetLatestBuild()
     If Between(nMostRecent, 1, frmMain.lstvBuildDTMachine.ListItems.Count) Then
        frmMain.lstvBuildDTMachine.ListItems(nMostRecent).Selected = True  ' select last entry
        frmMain.lstvBuildDTMachine_Click
     End If
  End If
End Sub
Private Function GetLatestBuild%()
    Dim m&, n&, i%, sExePath$, sBuildSpec$
   
    GetLatestBuild = 0
    sExePath = modParse.sCurDir & "\dungeonsiege" & IIf(frmMain.optRelease.Value, "R", "D") & ".exe"
    sBuildSpec = GetFileVersion(sExePath)
    ' 1) try to find a match with exe selected in setup tab:
    For i = 1 To frmMain.lstvBuildDTMachine.ListItems.Count
        Debug.Print frmMain.lstvBuildDTMachine.ListItems(i).Text  'the build
        If (StrComp(frmMain.lstvBuildDTMachine.ListItems(i).Text, sBuildSpec) = 0) Then
            If IsDate(modDeclares.TimeDate) Then
                If (CDate(frmMain.lstvBuildDTMachine.ListItems(i).SubItems(1)) = CDate(modDeclares.TimeDate)) Then
                    GetLatestBuild = i
                    Exit For
                End If
            Else
                GetLatestBuild = i
                Exit For
            End If
        End If
    Next i
    
    ' 2) if no match found in the above step, just get the latest build available
    If (GetLatestBuild = 0) Then
         n = 0&
         GetLatestBuild = 0
         For i = 1 To frmMain.lstvBuildDTMachine.ListItems.Count
             Debug.Print frmMain.lstvBuildDTMachine.ListItems(i).Text  'the build
             m = effectiveNum(Trim(frmMain.lstvBuildDTMachine.ListItems(i).Text))
             If (m > n) Then
                 n = m  ' now the most recent
                 If IsDate(modDeclares.TimeDate) Then
                     If (CDate(frmMain.lstvBuildDTMachine.ListItems(i).SubItems(1)) = CDate(modDeclares.TimeDate)) Then
                         GetLatestBuild = i
                     End If
                 Else
                     GetLatestBuild = i
                 End If
             End If
         Next i
     End If
End Function
Private Function effectiveNum&(ByVal sVerNum$)
   Dim sPiece$, i%
   
   ' passed : form "0.1.10.802"  0.yr.mo.day
   effectiveNum = 0&
   i = 0
   sPiece = strTok(sVerNum, ".")
   Do While sPiece > ""
      If IsNumeric(sPiece) Then
         i = i + 1
         If i = 4 Then
             effectiveNum = 10000& * effectiveNum + CLng(sPiece) ' shift over 4 decimal places, then add: last set
         Else
             effectiveNum = 100& * effectiveNum + CLng(sPiece)   ' shift over 2 decimal places, then add
         End If
      End If
      sPiece = strTok(sVerNum, "")
   Loop
End Function
Public Function bLaunchSpellTest(sCommandLineString As String) As String
   Dim hInst As Long             ' Instance handle from Shell function.
   Dim hWndApp As Long           ' Window handle from GetWinHandle.
   Dim buffer As String          ' Holds caption of Window.
   Dim numChars As Integer       ' Count of bytes returned.
   Dim idproc As Long
   'build the command string
   
   ' Shell to an application
   ChDrive (Left$(sCommandLineString, 1))
   ChDir (modParse.sCurDir)
   modDeclares.tLaunchTD = Now()
   Debug.Print sCommandLineString
   hInst = Shell(sCommandLineString, vbNormalFocus)
   
   ' Begin search for handle
   hWndApp = GetWinHandle(hInst)
   
   If hWndApp <> 0 Then
        ' Init buffer
        buffer = Space$(128)
        
        ' Get caption of window
        numChars = GetWindowText(hWndApp, buffer, Len(buffer))
        'MsgBox "Hwnd = " & hWndApp
        idproc = ProcIDFromWnd(hWndApp)
        
        'MsgBox "Idproc " & idproc
        '  GetWindowThreadProcessId hWndApp, idproc
        SetFocusAPI (hWndApp)
        Do While idproc > 0
            DoEvents
            '       ' Get PID for this HWnd
            'MsgBox "Still running"
            idproc = ProcIDFromWnd(hWndApp)
            modWinHandlers.FindAndKillPopUpWindows
            
            'SetFocusAPI (hWndApp)
        Loop
        'MsgBox idproc & " is the idproc, ds not running"
   End If
End Function

Function GetFileVersion(ByVal sPathSpec$) As String
   Dim fso As Object, temp As String
   
   Set fso = CreateObject("Scripting.FileSystemObject")
   temp = fso.GetFileVersion(sPathSpec)
   If Len(temp) Then
      GetFileVersion = temp
   Else
      GetFileVersion = "No version information available."
   End If
End Function

Public Function mylen%(ByVal nCurElt%)
    Select Case nCurElt  ' lookup how wide in chars each field should be:
        Case 2
            mylen = 17
        Case 7            ' spell screen name, only one currently showing in report
            mylen = 18    ' chngd from 16 to 18 for dragon_breath_short
        Case 8
            mylen = 18
        Case 9
            mylen = 7
        Case Else
            mylen = 12
    End Select
End Function
Public Function Between(a%, b%, c%)
   Between = (a >= b) And (a <= c)
End Function
Public Sub format_full_long(ByRef cCtrl As Control)
   Dim nStart%, nLen%, sCand$
   
   ' set default text color:
   cCtrl.SelStart = 0
   cCtrl.SelLength = Len(cCtrl.Text)
   cCtrl.SelColor = vbBlack
   
   Call FormatControl(cCtrl, sPassDef, vbGreen) '  green
   Call FormatControl(cCtrl, sFailDef, vbRed) '  red
   Call FormatControl(cCtrl, sInfoDef, RGB(55, 55, 225))
   Call FormatControl(cCtrl, sStartDef, vbYellow) ' , vbMagenta)
   Call FormatControl(cCtrl, sEndDef, vbYellow)   ' , vbMagenta)
   Call FormatControl(cCtrl, "+00:", RGB(55, 55, 125), 13)  ' highlight world time to blue
   Call FormatHeading(cCtrl)
End Sub
Sub processStatus(ByRef cCtrl As RichTextBox, ByVal sSrch$, ByVal nCol&, nIndex%)
    Dim nStrt&, nSpc&, bCounted As Boolean
    
    On Error GoTo 0 ' stop on error
    If bSplashOn Then
        With frmSplash
            .lblLoad.Caption = "formatting " & sSrch & " on spell summary"
            .ProgressBar1.Visible = False
        End With
    End If
    nStrt = InStrRev(cCtrl.Text, sSrch)
    Do While nStrt > 0
        DoEvents
        
        If (nIndex = -2) Then  ' *** flag
            nStrt = InStrRev(cCtrl.Text, sSrch, nStrt - 1&)
            nSpc = InStr(nStrt + 2&, cCtrl.Text, sSrch) + 3&  ' *** : now at end of 2nd ***
            cCtrl.SelStart = nStrt - 1&
            bCounted = False
        Else
            cCtrl.SelStart = nStrt - 1&
            If (Mid$(cCtrl.Text, nStrt + Len(sSrch), 1) = " ") Then
                nSpc = cCtrl.SelStart + Len(sSrch) + 1&  ' exact match
                bCounted = True
            Else
                nSpc = InStr(cCtrl.SelStart, cCtrl.Text, " ")
                bCounted = False
            End If
        End If
        If nSpc > nStrt Then
            cCtrl.SelLength = nSpc - (cCtrl.SelStart + 1)
        Else
            cCtrl.SelLength = Len(sSrch)
        End If
        cCtrl.SelColor = nCol  '
        cCtrl.SelBold = True
        Select Case (nIndex)
            Case 0
                ' fail
                ReDim Preserve tErrLookup(nMaxErrors)
                tErrLookup(nMaxErrors) = nStrt
                If bCounted Then
                    nMaxErrors = nMaxErrors + 1&
                End If
            Case 1
                ' pass
                If bCounted Then
                    nMaxPass = nMaxPass + 1&
                End If
'            Case Else  ' the totaled area
        End Select
        nStrt = InStrRev(cCtrl.Text, sSrch, nStrt - 1&)
    Loop
    If bSplashOn Then
        With frmSplash
            .lblLoad.Caption = "All done!"
            .Refresh
        End With
    End If
End Sub

Public Function GetMonsterIndex%(ByVal sMonsterName$)
    Dim i%
    
    GetMonsterIndex = 0  '      If (i > (frmMain.cmbMonsterList.ListCount - 2)) Then GetMonsterIndex = 0 ' default to first entry when not found

    For i = 0 To frmMain.cmbMonsterList.ListCount - 2
        If (StrComp(frmMain.cmbMonsterList.List(i), sMonsterName) = 0) Then
            GetMonsterIndex = i
            Exit For
        End If
    Next i
End Function

Public Function getSpellindex%(ByVal sSpellName$)
    Dim i%
    
    getSpellindex = 0  '      If (i > (frmMain.cmbSpellList.ListCount - 2)) Then getSpellindex = 0 ' default to first entry when not found

    For i = 0 To frmMain.cmbSpellList.ListCount - 2
        If (StrComp(RTrim(frmMain.cmbSpellList.List(i)), sSpellName) = 0) Then
            getSpellindex = i
            Exit For
        End If
    Next i
End Function
Public Sub setup_spells()
    frmSplash.Show vbModeless
    ' bw 10/11/2001  lookup monsters  &  spells, etc
    Call modSpells.lookup_monsters_spells
    Call modSpells.lookup_buildDTmac
    Unload frmSplash
End Sub
Private Sub moveSpell(ByRef xDest As spellAttrs, ByRef xSrc As spellAttrs)
    xDest.Class = xSrc.Class
    xDest.damage_max = xSrc.damage_max
    xDest.damage_min = xSrc.damage_min
    xDest.is_one_shot = xSrc.is_one_shot
    xDest.mana_cost = xSrc.mana_cost
    xDest.required_level = xSrc.required_level
    xDest.screen_name = xSrc.screen_name
    xDest.specializes = xSrc.specializes
    xDest.template = xSrc.template
End Sub
Public Sub MySort(ByRef pArray() As spellAttrs, ByVal nArrCount%)
    Dim i%, j%, sTemp As spellAttrs
     ' simple bubble sort alpha
     
    If bSplashOn Then
        With frmSplash.lblLoad
            .Caption = "Sorting spells alphabetically"
            .Refresh
        End With
        With frmSplash.ProgressBar1
            .Value = 0
            .Max = nArrCount - 1
        End With
    End If
    For i = 0 To nArrCount - 1
        If Asc(pArray(i).screen_name) = 0 Then
           pArray(i).screen_name = modSpells.RmvSpace(Mid$(pArray(i).template, InStr(pArray(i).template, "_") + 1))
        End If
    Next i
    For i = 0 To nArrCount - 1
        For j = i To nArrCount - 1
             If (StrComp(pArray(i).screen_name, pArray(j).screen_name, vbTextCompare) > 0) Then
                  Call moveSpell(sTemp, pArray(i))  ' make ith elt. come first alphabetically before j  for all j
                  Call moveSpell(pArray(i), pArray(j))
                  Call moveSpell(pArray(j), sTemp)
             End If
        Next j
        If bSplashOn Then frmSplash.ProgressBar1.Value = i
    Next i
End Sub
Public Function countChars(ByRef sTextStr$, ByVal sChars$) As Long
    Dim lLoc&
    
    countChars = 0&
    lLoc = InStr(sTextStr, sChars)
    Do While lLoc > 0&
        countChars = countChars + 1&
        DoEvents
        lLoc = InStr(lLoc + 1&, sTextStr, sChars)
    Loop
End Function
