Attribute VB_Name = "modWAV"
Option Explicit
Private sResult$
Public nErrors%

#Const SIM_TEST = False

Private Function myExtraneous$(ByVal n&)
   Dim nEOL&
   
   If (n = 0&) Then
       myExtraneous = "Pass"
   Else
        nErrors = nErrors + 1
        nEOL = InStr(n, sResult, Chr(13))
        If nEOL > 1& Then
            myExtraneous = Mid$(sResult, n, nEOL - n)
        Else
            myExtraneous = "Fail"
        End If
    End If
End Function

Public Sub runWav()
   Dim sCurFile$, nTotal&, i&   '   , j&    'used to locate call_spider wav
   Dim lix  As ListItem
   
   Dim nResult%
   
   frmMain.ListViewWav.ListItems.Clear
   nErrors = 0
   nTotal = 0&
   modDeclares.TimeDate = Now()
   sCurFile = Dir(modParse.sCurDir & "\sound\Effects\*.wav", vbNormal)
   Do While sCurFile > ""
       'call
 '      Debug.Print " on  " & sCurFile
       nTotal = nTotal + 1&
       Set lix = frmMain.ListViewWav.ListItems.Add(, , Left$(sCurFile, InStrRev(sCurFile, ".") - 1))
'       If StrComp(Trim(lix.Text), "s_e_call_spider") = 0 Then j = nTotal
       Set lix = Nothing
       sCurFile = Dir
   Loop
   
   With frmSplash
       .ProgressBar1.Max = nTotal
       .lblTitle.Caption = "making a list of WAV files"
       .Show vbModeless
       .Refresh
   End With
   
   i = 1&
'   i = j
   Do While i <= frmMain.ListViewWav.ListItems.Count
       Set lix = frmMain.ListViewWav.ListItems.Item(i)
#If SIM_TEST Then
       nResult = Rnd()
       lix.SubItems(1) = IIf(nResult < 1, "Pass", myExtraneous(nResult))
#Else
'       shell to DS
       nResult = ResultOfWavTest(lix.Text)
       Debug.Print " on  " & lix.Text
       lix.SubItems(1) = myExtraneous(nResult)
#End If
       
       Set lix = Nothing
       With frmSplash
            .lblLoad.Caption = "Load Progress: " & Format(i) & " of " & Format(nTotal) & " wav files scanned"
            .ProgressBar1.Value = i
            .Refresh
       End With
       i = i + 1&
       Do While frmMain.cmdWavRun.Caption <> "&Pause"
           DoEvents
           If frmMain.cmdWavRun.Caption = "Exit" Then Exit Do
       Loop
       If frmMain.cmdWavRun.Caption = "Exit" Then Exit Do
   Loop
   Unload frmSplash
End Sub
#If Not SIM_TEST Then

Private Function ResultOfWavTest(ByVal sWav$) As Integer
    Dim sRslt$
    Dim sFullPath$, sMySkrit$, sType$
    Dim i%
    Dim CurrentCaseInfo As Caseinfo
    Dim currentloginfo As LogInfo
    
    ResultOfWavTest = 1 ' 0 => success, 1 => failure
    On Error GoTo erh
    sMySkrit$ = "testWAV"   ' pass wav on cmd line: wav=
    
    sType = "D"
    
    sFullPath = modParse.sCurDir & "\DungeonSiege" & sType & ".exe" '
    
    If Dir(sFullPath) = "" Then
        MsgBox "DungeonSiege" & sType & ".exe was not found at " & modParse.sCurDir  '  H:\PreCompiled\" & frmMain.sVerID & "  !"
        Exit Function
    Else                                                                 ' skrit_retry=true
        sFullPath = sFullPath & "  skritbot=auto\test\" & sMySkrit & "   map=Halo_test_map  teleport=start MAXTime=4.5 "
        If frmMain.chkWIndowed.Value = 0 Then
            sFullPath = sFullPath & " fullscreen=false"
        End If
          '
        sFullPath = sFullPath & " wav=#" & sWav
        
        Debug.Print "Cmd = " & sFullPath
    End If
    
    sFullPath = sFullPath & vbCr
        With CurrentCaseInfo
            .Author = "Brian Warris"
            .CaseName = "testWav"
            .Description = "wav test"
            .Expected = "no extraneous data"
            .Map = "Halo_test_map"
            .Teleport = "start"
            .TestArea = "SOUND"
            .Skrit = sMySkrit
            .SkritPath = frmMain.txtSkritPath.Text
        End With
            
        With currentloginfo
            .CaseName = CurrentCaseInfo.CaseName
            .MachineName = CurrentUserInfo.MachineName
            .TimeDateStamp = modDeclares.TimeDate
            .UserName = CurrentUserInfo.UserName
            .IsDebug = True  ' optDebug.Value
        End With

        sRslt = bLaunchSpellTest(sFullPath)
        Debug.Assert (sRslt = "")
        delay 0.2
        
        modParse.ReadTheLog currentloginfo
        ResultOfWavTest = InStrRev(currentloginfo.LogData, "extraneous")   ' 0 => success, otherwise => failure
        sResult = currentloginfo.LogData
    Exit Function
erh:
'   frmMain.StatusBar1.SimpleText = "ResultOfWavTest() Error " & CStr(Err.Number) & ": " & Err.Description
   MsgBox "ResultOfWavTest() Error " & CStr(Err.Number) & ": " & Err.Description
   Err.Clear
End Function


#End If





