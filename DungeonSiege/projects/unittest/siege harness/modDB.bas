Attribute VB_Name = "modDB"
Option Explicit


'Global Const ConnectionString = "driver={SQL Server};SERVER=markgrimsqlsvr;database=unittestdb;dsn=''"
Public ConnectionString As String
Public Const defSQLserver$ = "markgrimsqlsvr"

Sub dbOpenTable()

   ''Dim dbTests As Database
    Dim cnConnection As New ADODB.Connection
    Dim cmdCommand As New ADODB.Command
    Dim rstCases As New ADODB.Recordset
    Dim iCounter  As Integer
    
    iCounter = 0

    'Set dbtests = OpenDatabase("UnitTestdb.mdb")
    'Set rstCases = dbtests.OpenRecordset("Tests")
    
    With cnConnection
        .ConnectionString = ConnectionString
        .ConnectionTimeout = 100
        .Open
    End With
    Set cmdCommand.ActiveConnection = cnConnection
        cmdCommand.CommandText = "Select * from Tests"
        cmdCommand.CommandTimeout = 500
        'rstCases.CursorLocation = adUseClient
        With rstCases
        'Debug.Print "Table-type recordset: " & .Name
    rstCases.Open cmdCommand, , adOpenStatic, adLockBatchOptimistic

        Do While Not rstCases.EOF

            ''build the case array
            ReDim Preserve CaseArray(iCounter)
            
            CaseArray(iCounter).CaseName = !Name
            CaseArray(iCounter).TestArea = !Area
            CaseArray(iCounter).Author = !Author
            CaseArray(iCounter).Description = !Description
            CaseArray(iCounter).Expected = !Expected
            CaseArray(iCounter).Map = !Map
            CaseArray(iCounter).Skrit = !Skrit
            CaseArray(iCounter).Teleport = !Teleport
            If !Arguments > "" Then
                CaseArray(iCounter).Arguments = !Arguments
            End If
            
            iCounter = iCounter + 1
            
            .MoveNext
        
        Loop

        .Close
    End With

    cnConnection.Close

End Sub
Public Function bCheckCaseExistsInDb(sCaseName As String) As Boolean
    'Dim dbtests As Database    '///Oldschool

    Dim iCounter  As Integer
    Dim iAreaCounter As Integer
    Dim rstAreas As Recordset
    
    Dim cnConnection As New ADODB.Connection
    Dim cmdCommand As New ADODB.Command
    Dim rstCases As New ADODB.Recordset


    With cnConnection
        .ConnectionString = ConnectionString
        .ConnectionTimeout = 100
        .Open

    End With
    Set cmdCommand.ActiveConnection = cnConnection
    cmdCommand.CommandText = "tests"
    cmdCommand.CommandTimeout = 500
    rstCases.CursorLocation = adUseClient
    
    With rstCases

        .Open cmdCommand, , adOpenDynamic, adLockBatchOptimistic
        
        Do While Not .EOF
            If !Name = sCaseName Then
                bCheckCaseExistsInDb = True
                cnConnection.Close
                Exit Function
            End If
            
            .MoveNext
        
        Loop

        .Close
    End With

    cnConnection.Close
End Function
Public Sub ModifyExistingCase(CurrentCaseInfo As Caseinfo)

    Dim cnConnection As New ADODB.Connection
    Dim cmdCommand As New ADODB.Command
    Dim rstCases As New ADODB.Recordset

    With cnConnection
        .ConnectionString = ConnectionString
        .ConnectionTimeout = 100
        .Open

    End With
    
    Set cmdCommand.ActiveConnection = cnConnection
    cmdCommand.CommandText = "select * from tests where Name = '" & CurrentCaseInfo.CaseName & "'"
    cmdCommand.CommandTimeout = 500
    rstCases.CursorLocation = adUseClient
    
    With rstCases
    .Open cmdCommand, , adOpenDynamic, adLockBatchOptimistic

        !Description = CurrentCaseInfo.Description
        !Author = CurrentCaseInfo.Author
        !Expected = CurrentCaseInfo.Expected
        !Arguments = CurrentCaseInfo.Arguments
        !Map = CurrentCaseInfo.Map
        !Teleport = CurrentCaseInfo.Teleport
        !Skrit = CurrentCaseInfo.Skrit
        !Area = CurrentCaseInfo.TestArea
        
        .UpdateBatch

    
    .Close
    End With
    cnConnection.Close

End Sub
Public Sub WriteNewCase(CurrentCaseInfo As Caseinfo)
    Dim cnConnection As New ADODB.Connection
    Dim cmdCommand As New ADODB.Command
    Dim rstCases As New ADODB.Recordset


    With cnConnection
        .ConnectionString = ConnectionString
        .ConnectionTimeout = 100
        .Open

    End With
    Set cmdCommand.ActiveConnection = cnConnection
    cmdCommand.CommandText = "tests"
    cmdCommand.CommandTimeout = 500
    rstCases.CursorLocation = adUseClient
    
    With rstCases
    .Open cmdCommand, , adOpenDynamic, adLockBatchOptimistic
        .AddNew
        !Name = CurrentCaseInfo.CaseName
        !Description = CurrentCaseInfo.Description
        !Author = CurrentCaseInfo.Author
        !Expected = CurrentCaseInfo.Expected
        If CurrentCaseInfo.Arguments > "" Then
            !Arguments = CurrentCaseInfo.Arguments
        End If
        !Map = CurrentCaseInfo.Map
        !Teleport = CurrentCaseInfo.Teleport
        !Skrit = CurrentCaseInfo.Skrit
        !Area = CurrentCaseInfo.TestArea
        
        .UpdateBatch
        

    
    .Close
    End With
    cnConnection.Close

End Sub

Public Function CheckMachineExistsInDB(CurrentUserInfo As UserInfo, ByVal sUsername As String) As Boolean
    'Dim dbtests As Database    '///Oldschool

    Dim rstAreas As Recordset
    Dim cnConnection As New ADODB.Connection
    Dim cmdCommand As New ADODB.Command
    Dim rstCases As New ADODB.Recordset


    With cnConnection
        .ConnectionString = ConnectionString
        .ConnectionTimeout = 100
        .Open

    End With
    Set cmdCommand.ActiveConnection = cnConnection
    cmdCommand.CommandText = "Users"
    cmdCommand.CommandTimeout = 500
    rstCases.CursorLocation = adUseClient
    
    With rstCases

        .Open cmdCommand, , adOpenDynamic, adLockBatchOptimistic
        
        Do While Not .EOF
            If !MachineName = CurrentUserInfo.MachineName Then
                sUsername = !UserName
                CheckMachineExistsInDB = True
                rstCases.Close
                cnConnection.Close
                Exit Function
            End If
            
            .MoveNext
        
        Loop
        .Close
    End With
        
    cnConnection.Close


End Function
Public Sub AddCaseLogToDB(CurrentCaseLogInfo As LogInfo)
    Dim cnConnection As New ADODB.Connection
    Dim cmdCommand As New ADODB.Command
    Dim rstCases As New ADODB.Recordset


    With cnConnection
        .ConnectionString = ConnectionString
        .ConnectionTimeout = 100
        .Open

    End With
    Set cmdCommand.ActiveConnection = cnConnection
    cmdCommand.CommandText = "Logs"
    cmdCommand.CommandTimeout = 500
    rstCases.CursorLocation = adUseClient
    
    With rstCases
    .Open cmdCommand, , adOpenDynamic, adLockBatchOptimistic
        .AddNew
        !DateTimeStamp = CurrentCaseLogInfo.TimeDateStamp
        !MachineName = CurrentCaseLogInfo.MachineName
        !CaseName = CurrentCaseLogInfo.CaseName
        !caseLog = Left$(CurrentCaseLogInfo.LogData, 7900&)
        !Pass = CurrentCaseLogInfo.Pass
        !UserName = CurrentCaseLogInfo.UserName
        !IsDebug = CurrentCaseLogInfo.IsDebug
        !CurrentBuild = CurrentUserInfo.CurrentBuild
        .UpdateBatch
        

    
    .Close
    End With
    cnConnection.Close
End Sub
Public Function GetLogFromDb(sCaseName As String, sDateTimeStamp As String) As String
    Dim cnConnection As New ADODB.Connection
    Dim cmdCommand As New ADODB.Command
    Dim rstCases As New ADODB.Recordset


    With cnConnection
        .ConnectionString = ConnectionString
        .ConnectionTimeout = 100
        .Open

    End With
    Set cmdCommand.ActiveConnection = cnConnection
    cmdCommand.CommandText = "SELECT CaseLog FROM Logs WHERE DateTimeStamp = '" & sDateTimeStamp & "' AND CaseName = '" & sCaseName & "'"
    cmdCommand.CommandTimeout = 500
    
    With rstCases
        .Open cmdCommand, , adOpenDynamic, adLockBatchOptimistic
        GetLogFromDb = !caseLog
        .Close
   End With
   cnConnection.Close
End Function
Public Sub AddUserToDB(CurrentUserInfo As UserInfo)
    Dim cnConnection As New ADODB.Connection
    Dim cmdCommand As New ADODB.Command
    Dim rstCases As New ADODB.Recordset
    Dim MachineExists As Boolean


    With cnConnection
        .ConnectionString = ConnectionString
        .ConnectionTimeout = 100
        .Open

    End With
    Set cmdCommand.ActiveConnection = cnConnection
        cmdCommand.CommandText = "Users"
        cmdCommand.CommandTimeout = 500
        rstCases.CursorLocation = adUseClient
'    MachineExists = CheckMachineExistsInDB(CurrentUserInfo)
'    If MachineExists Then
'        cmdCommand.CommandText = "Select * FROM Users WHERE MachineName = '" & CurrentUserInfo.MachineName & "'"
'    Else


 '   End If
    
    With rstCases
        .Open cmdCommand, , adOpenDynamic, adLockBatchOptimistic
            .AddNew
            !ExePath = CurrentUserInfo.ExePath
            !IsDebug = CurrentUserInfo.IsDebug
            !MachineName = CurrentUserInfo.MachineName
            !Platform = CurrentUserInfo.OS
            !SkritPath = CurrentUserInfo.SkritPath
            !UserName = CurrentUserInfo.UserName
            !MinimizeMain = CurrentUserInfo.MinimizeMain
            !CurrentBuild = CurrentUserInfo.CurrentBuild
        
        .UpdateBatch
        
        .Close
    End With
    Set rstCases = Nothing
    cnConnection.Close
    Set cnConnection = Nothing
End Sub
Public Sub DeleteUserInDB(CurrentUserInfo As UserInfo)
    Dim cnConnection As New ADODB.Connection
    Dim cmdCommand As New ADODB.Command
    Dim rstCases As New ADODB.Recordset
    Dim MachineExists As Boolean

    On Error GoTo erh
    With cnConnection
        .ConnectionString = ConnectionString
        .ConnectionTimeout = 100
        .Open

    End With
    Set cmdCommand.ActiveConnection = cnConnection
        cmdCommand.CommandText = "Users"
        cmdCommand.CommandTimeout = 500
        rstCases.CursorLocation = adUseClient
        cmdCommand.CommandText = "delete FROM Users WHERE MachineName = '" & CurrentUserInfo.MachineName & "' AND UserName = '" & CurrentUserInfo.UserName & "'"

    
    rstCases.Open cmdCommand, , adOpenDynamic, adLockBatchOptimistic
         
    Set rstCases = Nothing
    cnConnection.Close
    Set cnConnection = Nothing
erh:
   If (Err.Number <> 0) Then
        MsgBox "DeleteUserInDB Error " & CStr(Err.Number) & " : " & Err.Description
'   StatusBar1.SimpleText = sFile & " READ Error " & CStr(Err.Number) & ": " & Err.Description
        Err.Clear
   End If
End Sub
Public Sub UpdateUserinDB(CurrentUserInfo As UserInfo)
    Dim cnConnection As New ADODB.Connection
    Dim cmdCommand As New ADODB.Command
    Dim rstCases As New ADODB.Recordset
    Dim MachineExists As Boolean


    With cnConnection
        .ConnectionString = ConnectionString
        .ConnectionTimeout = 100
        .Open

    End With
    Set cmdCommand.ActiveConnection = cnConnection
        cmdCommand.CommandText = "Users"
        cmdCommand.CommandTimeout = 500
        rstCases.CursorLocation = adUseClient
        cmdCommand.CommandText = "Select * FROM Users WHERE MachineName = '" & CurrentUserInfo.MachineName & "' AND UserName = '" & CurrentUserInfo.UserName & "'"

    
    With rstCases
        .Open cmdCommand, , adOpenDynamic, adLockBatchOptimistic
            !ExePath = CurrentUserInfo.ExePath
            !IsDebug = CurrentUserInfo.IsDebug
            !MachineName = CurrentUserInfo.MachineName
            !Platform = CurrentUserInfo.OS
            !SkritPath = CurrentUserInfo.SkritPath
            !UserName = CurrentUserInfo.UserName
            !MinimizeMain = CurrentUserInfo.MinimizeMain
            !CurrentBuild = CurrentUserInfo.CurrentBuild
        
        .UpdateBatch
        

    
        .Close
    End With
    Set rstCases = Nothing
    cnConnection.Close
    Set cnConnection = Nothing
End Sub
Public Sub SpellCaseLogToDB(CurrentSpells As Spells)
    Dim cnConnection As New ADODB.Connection
    Dim cmdCommand As New ADODB.Command
    Dim rstSpellCases As New ADODB.Recordset


    With cnConnection
        .ConnectionString = ConnectionString
        .ConnectionTimeout = 100
        .Open

    End With
    Set cmdCommand.ActiveConnection = cnConnection
    cmdCommand.CommandText = "SpellResults"
    rstSpellCases.CursorLocation = adUseClient
    cmdCommand.CommandTimeout = 500
    
    With rstSpellCases
        .Open cmdCommand, , adOpenDynamic, adLockBatchOptimistic
        .AddNew
        !DateTimeStamp = CurrentSpells.TimeDateStamp
        !MachineName = CurrentSpells.MachineName
        !MonsterName = CurrentSpells.MonsterName
        !SpellName = CurrentSpells.SpellName
        !PassFail = CurrentSpells.PassFail
        !BuildNumber = CurrentSpells.BuildNumber
        !spellLog = Left$(CurrentSpells.LogData, 7900&)
        .UpdateBatch
        .Close
    End With
    cnConnection.Close
    Set rstSpellCases = Nothing  '  release memory
    Set cnConnection = Nothing
    Set cmdCommand = Nothing
End Sub




Public Function GetUserInfoFromDb(CurrentUserInfo As UserInfo) As Boolean
    Dim cnConnection As New ADODB.Connection
    Dim cmdCommand As New ADODB.Command
    Dim rstCases As New ADODB.Recordset
    Dim MachineExists As Boolean
    Dim sUsername As String


    With cnConnection
        .ConnectionString = ConnectionString
        .ConnectionTimeout = 100
        .Open

    End With
    Set cmdCommand.ActiveConnection = cnConnection
    
    MachineExists = CheckMachineExistsInDB(CurrentUserInfo, sUsername)
    
    On Error GoTo erh
    If MachineExists Then
        cmdCommand.CommandText = "Select * FROM Users WHERE MachineName ='" & CurrentUserInfo.MachineName & "'  and username='" & CurrentUserInfo.UserName & "'"
        cmdCommand.CommandTimeout = 120
        cmdCommand.CommandType = adCmdText
        With rstCases
            .Open cmdCommand, , adOpenDynamic, adLockBatchOptimistic
            If (Err.Number = 0) Then
                If (!UserName <> RTrim(CurrentUserInfo.UserName)) Or ((.State And adStateOpen) = 0) Then
                    
                    GetUserInfoFromDb = False
                    GoTo cleanup
                    
                End If
            
                CurrentUserInfo.ExePath = !ExePath
                CurrentUserInfo.IsDebug = !IsDebug
                CurrentUserInfo.MachineName = !MachineName
                CurrentUserInfo.OS = !Platform
                CurrentUserInfo.SkritPath = !SkritPath
                CurrentUserInfo.UserName = !UserName
                CurrentUserInfo.MinimizeMain = !MinimizeMain
                CurrentUserInfo.CurrentBuild = !CurrentBuild
                If Len(Trim(CurrentUserInfo.CurrentBuild)) = 0 Then
                    If CurrentUserInfo.IsDebug Then
                        CurrentUserInfo.CurrentBuild = GetFileVersion(CurrentUserInfo.ExePath & "\" & DEBUG_EXE)
                    Else
                        CurrentUserInfo.CurrentBuild = GetFileVersion(CurrentUserInfo.ExePath & "\" & RELEASE_EXE)
                    End If
                End If
                .UpdateBatch
                GetUserInfoFromDb = True
                frmMain.txtExepath.Text = CurrentUserInfo.ExePath
            Else
                Err.Clear
                GetUserInfoFromDb = False
            End If
            .Close
        End With
        cnConnection.Close
    Else
        GetUserInfoFromDb = False
    End If
    GoTo cleanup
erh:
    If Err.Number <> 0 Then Err.Clear
    GetUserInfoFromDb = False
cleanup:
    If Not rstCases Is Nothing Then
        If ((rstCases.State And adStateOpen) <> 0) Then
            rstCases.Close
        End If
        Set rstCases = Nothing
    End If
    If Not cnConnection Is Nothing Then
        If ((cnConnection.State And adStateOpen) <> 0) Then
            cnConnection.Close
        End If
        Set cnConnection = Nothing
    End If
    
    Set cmdCommand = Nothing
End Function

