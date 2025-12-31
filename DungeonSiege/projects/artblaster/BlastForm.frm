VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form BlastForm 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Art Blaster"
   ClientHeight    =   4416
   ClientLeft      =   36
   ClientTop       =   336
   ClientWidth     =   3276
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   4416
   ScaleWidth      =   3276
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton CopyToClip 
      Caption         =   "Clip"
      Height          =   372
      Left            =   2040
      TabIndex        =   29
      Top             =   720
      Width           =   492
   End
   Begin VB.CommandButton AdvanceToNextCell 
      Caption         =   "Next"
      Enabled         =   0   'False
      Height          =   372
      Left            =   2640
      TabIndex        =   28
      Top             =   720
      Width           =   492
   End
   Begin MSComctlLib.ProgressBar ProgressBar1 
      Height          =   252
      Left            =   120
      TabIndex        =   15
      Top             =   3000
      Width           =   3012
      _ExtentX        =   5313
      _ExtentY        =   445
      _Version        =   393216
      Appearance      =   1
   End
   Begin MSComctlLib.ProgressBar ProgressBar2 
      Height          =   252
      Left            =   120
      TabIndex        =   14
      Top             =   2640
      Width           =   3012
      _ExtentX        =   5313
      _ExtentY        =   445
      _Version        =   393216
      Appearance      =   1
   End
   Begin VB.CommandButton ExportEverythingTagged 
      Caption         =   "Export All"
      Height          =   372
      Left            =   1680
      TabIndex        =   13
      Top             =   2160
      Width           =   1452
   End
   Begin VB.CommandButton AbortBatchOperation 
      Caption         =   "Abort"
      Height          =   372
      Left            =   120
      TabIndex        =   12
      Top             =   2160
      Width           =   1452
   End
   Begin VB.CommandButton ExportSelected 
      Caption         =   "Export Selected"
      Height          =   372
      Left            =   1680
      TabIndex        =   11
      Top             =   1680
      Width           =   1452
   End
   Begin VB.CommandButton OpenSelected 
      Caption         =   "Open In Max"
      Height          =   372
      Left            =   120
      TabIndex        =   10
      Top             =   1680
      Width           =   1452
   End
   Begin VB.CommandButton ViewLocal 
      Caption         =   "Local"
      Height          =   372
      Left            =   1080
      TabIndex        =   7
      Top             =   720
      Width           =   852
   End
   Begin VB.CommandButton ViewShadow 
      Caption         =   "Shadow"
      Height          =   372
      Left            =   120
      TabIndex        =   6
      Top             =   720
      Width           =   852
   End
   Begin VB.CommandButton GetItemsWorkbook 
      Caption         =   "Item"
      Height          =   492
      Left            =   2640
      TabIndex        =   5
      Top             =   120
      Width           =   492
   End
   Begin VB.CommandButton Command5 
      Caption         =   "Command5"
      Height          =   612
      Left            =   3720
      TabIndex        =   4
      Top             =   240
      Width           =   732
   End
   Begin VB.CommandButton GetArmorWorkbook 
      Caption         =   "Armr"
      Height          =   492
      Left            =   2040
      TabIndex        =   3
      Top             =   120
      Width           =   492
   End
   Begin VB.CommandButton GetWeaponsWorkbook 
      Caption         =   "Wpn"
      Height          =   492
      Left            =   1380
      TabIndex        =   2
      Top             =   120
      Width           =   492
   End
   Begin VB.CommandButton GetAnimationWorkbook 
      Caption         =   "Anim"
      Height          =   492
      Left            =   756
      TabIndex        =   1
      Top             =   120
      Width           =   492
   End
   Begin VB.CommandButton GetCharacterWorkbook 
      Caption         =   "Char"
      Height          =   492
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   492
   End
   Begin VB.Label Label81 
      Height          =   252
      Left            =   1080
      TabIndex        =   31
      Top             =   4080
      Width           =   492
   End
   Begin VB.Label Label11 
      Caption         =   "Must Loop"
      Height          =   252
      Left            =   120
      TabIndex        =   30
      Top             =   4080
      Width           =   972
   End
   Begin VB.Label Label10 
      Alignment       =   1  'Right Justify
      ForeColor       =   &H000000FF&
      Height          =   252
      Left            =   2400
      TabIndex        =   27
      Top             =   3840
      Width           =   732
   End
   Begin VB.Label Label9 
      Alignment       =   1  'Right Justify
      ForeColor       =   &H00008000&
      Height          =   252
      Left            =   2400
      TabIndex        =   26
      Top             =   3600
      Width           =   732
   End
   Begin VB.Label Label13 
      Alignment       =   1  'Right Justify
      Height          =   252
      Left            =   2400
      TabIndex        =   25
      Top             =   3360
      Width           =   732
   End
   Begin VB.Label Label22 
      Caption         =   "Failed"
      Height          =   252
      Left            =   1680
      TabIndex        =   24
      Top             =   3840
      Width           =   732
   End
   Begin VB.Label Label21 
      Caption         =   "Success"
      Height          =   252
      Left            =   1680
      TabIndex        =   23
      Top             =   3600
      Width           =   732
   End
   Begin VB.Label Label20 
      Caption         =   "To Do"
      Height          =   252
      Left            =   1680
      TabIndex        =   22
      Top             =   3360
      Width           =   732
   End
   Begin VB.Label Label8 
      Height          =   252
      Left            =   1080
      TabIndex        =   21
      Top             =   3840
      Width           =   492
   End
   Begin VB.Label Label6 
      Height          =   252
      Left            =   1080
      TabIndex        =   20
      Top             =   3360
      Width           =   492
   End
   Begin VB.Label Label7 
      Height          =   252
      Left            =   1080
      TabIndex        =   19
      Top             =   3600
      Width           =   492
   End
   Begin VB.Label Label3 
      Caption         =   "Vertex Alpha"
      Height          =   252
      Left            =   120
      TabIndex        =   18
      Top             =   3840
      Width           =   972
   End
   Begin VB.Label Label2 
      Caption         =   "No Lighting"
      Height          =   252
      Left            =   120
      TabIndex        =   17
      Top             =   3600
      Width           =   972
   End
   Begin VB.Label Label1 
      Caption         =   "Alpha Cutoff"
      Height          =   252
      Left            =   120
      TabIndex        =   16
      Top             =   3360
      Width           =   972
   End
   Begin VB.Label StatusText2 
      Height          =   252
      Left            =   120
      TabIndex        =   9
      Top             =   1440
      Width           =   3012
   End
   Begin VB.Label StatusText1 
      Height          =   252
      Left            =   120
      TabIndex        =   8
      Top             =   1200
      Width           =   3012
   End
End
Attribute VB_Name = "BlastForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Dim WithEvents xl As Excel.Application
Attribute xl.VB_VarHelpID = -1
Dim MaxApp As Object

Private Const MaxSheetRows = 500

Private ExportColumn As String
Private MaxColumn As String
Private ASPColumn As String
Private AlphaCutoffColumn As Variant
Private VertexAlphaColumn As Variant
Private DisableLightColumn As Variant
Private AnimMustLoopColumn As Variant
Private DevNameColumn As Variant
Private ValidObjectColour As Long
Private MaxAvailable As Boolean
Private LocalAssets As String
Private ValidArtDoc As Boolean
Private SelectedCell As Excel.Range
Private AbortPressed As Boolean

Private SelectedDevName As String

Private ViewerHandles()

Private UseLocalFile As Boolean

Private hWndXL As Long

Private Const MAX_TEXTURES = 10
Dim TextureColumns(0 To MAX_TEXTURES - 1)

' These error codes should match the ones defined in BLASTTOOLS.MS
Enum BLAST_ERRORS
    BLAST_NO_ERROR = 0
    BLAST_ERROR_GENERIC = -1
    BLAST_ERROR_SMOOTHING = -2
    BLAST_ERROR_MISSING_FILE = -3
    BLAST_ERROR_BAD_MATERIAL = -4
    BLAST_ERROR_BAD_BONE = -5
    BLAST_ERROR_MISSING_MESH = -6
    BLAST_ERROR_READ_ONLY_FILE = -7
    BLAST_ERROR_MAXSCRIPT_CRASH = -8
End Enum

Enum BLAST_BOOKS
    BLAST_CHARACTERS
    BLAST_ANIMATIONS
    BLAST_WEAPONS
    BLAST_ARMOR
    BLAST_ITEMS
End Enum

Private BLAST_BOOKNAMES As Variant

Private Sub BuildBookNameArray()

     BLAST_BOOKNAMES = Array( _
        "character_master", _
        "animation_master", _
        "weapons_master", _
        "armor_export", _
        "items_master" _
    )

End Sub
    

Private Function FetchBlastError(errcode As Long) As String

    Select Case errcode
    Case BLAST_NO_ERROR
        FetchBlastError = "ok"
    Case BLAST_ERROR_GENERIC
        FetchBlastError = "failed"
    Case BLAST_ERROR_SMOOTHING
        FetchBlastError = "bad smth"
    Case BLAST_ERROR_MISSING_FILE
        FetchBlastError = "no file"
    Case BLAST_ERROR_BAD_MATERIAL
        FetchBlastError = "bad mats"
    Case BLAST_ERROR_BAD_BONE
        FetchBlastError = "bad bone"
    Case BLAST_ERROR_MISSING_MESH
        FetchBlastError = "no mesh"
    Case BLAST_ERROR_READ_ONLY_FILE
        FetchBlastError = "read/only"
    Case BLAST_ERROR_MAXSCRIPT_CRASH
        FetchBlastError = "maxscript"
    Case Else
        FetchBlastError = "error"
    End Select

    Exit Function

End Function
Private Function FetchErrorCaption(errcode As Long) As String

    Select Case errcode
    Case BLAST_NO_ERROR
        FetchErrorCaption = ""
    Case BLAST_ERROR_GENERIC
        FetchErrorCaption = "Check MaxScript Listener"
    Case BLAST_ERROR_SMOOTHING
        FetchErrorCaption = "Bad Smoothing groups"
    Case BLAST_ERROR_MISSING_FILE
        FetchErrorCaption = "Could not open MAX file"
    Case BLAST_ERROR_BAD_MATERIAL
        FetchErrorCaption = "Check materials,check MaxScript Listener"
    Case BLAST_ERROR_BAD_BONE
        FetchErrorCaption = "Check BIP01_Pelvis parent"
    Case BLAST_ERROR_MISSING_MESH
        FetchErrorCaption = "Could not find mesh in file"
    Case Else
        FetchErrorCaption = "Unknown error code"
    End Select

    Exit Function
End Function
Private Sub SetupSelectedCell()
    
    Dim selname As Variant
    Dim ActiveRow As Long
    
    ActiveRow = ActiveCell.Row
   
    SelectedDevName = ""
    
    If Not IsError(ActiveSheet.Cells(ActiveRow, ASPColumn).Value) Then
        selname = ActiveSheet.Cells(ActiveRow, ASPColumn).Value
    Else
        selname = ""
    End If
    
    If InStr(UCase(selname), "M_") = 1 Or InStr(UCase(selname), "A_") = 1 Then
    
        StatusText1.Caption = "SELECTED:"
        StatusText2.Caption = selname
        
        If InStr(UCase(selname), "M_") = 1 And DevNameColumn <> "" Then
            SelectedDevName = ActiveSheet.Cells(ActiveRow, DevNameColumn).Value
        End If
        
        LocalAssets = Environ("TATTOO_ASSETS")    ' Get environment
        If Dir(LocalAssets + "\cdimage\art\NamingKey.NNK") <> "" Then
            ViewLocal.Enabled = True
        Else
            ViewLocal.Enabled = False
        End If
        
        ViewShadow.Enabled = True
        ExportSelected.Enabled = MaxAvailable
        ExportEverythingTagged.Enabled = MaxAvailable And Not ActiveWorkbook.ReadOnly
        OpenSelected.Enabled = MaxAvailable
        
        If (AlphaCutoffColumn <> "") Then
            Label6.Caption = UCase(ActiveSheet.Cells(ActiveRow, AlphaCutoffColumn).Value) = "X"
        Else
            Label6.Caption = False
        End If
        
        If (DisableLightColumn <> "") Then
            Label7.Caption = UCase(ActiveSheet.Cells(ActiveRow, DisableLightColumn).Value) = "X"
        Else
            Label7.Caption = False
        End If
        
        If (VertexAlphaColumn <> "") Then
            Label8.Caption = UCase(ActiveSheet.Cells(ActiveRow, VertexAlphaColumn).Value) = "X"
        Else
            Label8.Caption = False
        End If
        
        If (AnimMustLoopColumn <> "") Then
            Label81.Caption = UCase(ActiveSheet.Cells(ActiveRow, AnimMustLoopColumn).Value) = "X"
        Else
            Label81.Caption = False
        End If
                              
    Else
    
        StatusText1.Caption = ""
        StatusText2.Caption = ""
        Label6.Caption = ""
        Label7.Caption = ""
        Label8.Caption = ""
        Label9.Caption = ""
        Label10.Caption = ""
        Label13.Caption = ""
        ViewShadow.Enabled = False
        ExportSelected.Enabled = False
        ExportEverythingTagged.Enabled = False
        OpenSelected.Enabled = False
        
    End If
       
End Sub

Private Sub Cleanup()

    StatusText1.Caption = ""
    StatusText2.Caption = ""
    Label6.Caption = ""
    Label7.Caption = ""
    Label8.Caption = ""
    Label9.Caption = ""
    Label10.Caption = ""
    Label13.Caption = ""
    ProgressBar1.Value = ProgressBar1.Max
    ProgressBar2.Value = ProgressBar2.Max

    AbortPressed = False

End Sub

Function CheckFor3DStudio() As Boolean

    CheckFor3DStudio = ClassIDExists("MAX.Application.3")
  
End Function

Sub Open3DStudio()

    On Error Resume Next
    
    If Not ClassIDExists("MAX.Application.3") Then Return
    
    Set MaxApp = CreateObject("MAX.Application.3")
    If Err.Number <> 0 Then

        Err.Clear
        
        StatusText1.Caption = "WARNING:"
        StatusText2.Caption = "3DStudio Max not available"
        MaxAvailable = False
        
    Else
    
        'AppActivate "3D Studio MAX R3"
        
        StatusText1.Caption = "SUCCESS:"
        StatusText2.Caption = "3DStudio Max loaded!"
        MaxAvailable = True
        
    End If
    
    On Error GoTo 0
   
End Sub


Private Sub AdvanceToNextCell_Click()

    Dim NextName As Variant
    Dim NextRow As Long
    Dim FoundNext As Boolean
   
    NextRow = ActiveCell.Row
    Do
    
        NextRow = NextRow + 1
        
        If Not IsError(ActiveSheet.Cells(NextRow, ASPColumn).Value) Then
            NextName = ActiveSheet.Cells(NextRow, ASPColumn).Value
        Else
            NextName = ""
        End If
        
        FoundNext = ((InStr(UCase(NextName), "M_") = 1) Or (InStr(UCase(NextName), "A_") = 1))
        
    Loop While (NextRow < MaxSheetRows) And Not FoundNext
    
    If (FoundNext) Then
    
        ActiveSheet.Cells(NextRow, ASPColumn).Select
    
        If (ViewerHandles(UBound(ViewerHandles) - 1) <> 0) Then
            SendCloseMessage (ViewerHandles(UBound(ViewerHandles) - 1))
            ViewerHandles(UBound(ViewerHandles) - 1) = 0
            DoEvents
        End If
                     
        ViewObject

    End If
    
End Sub

Private Sub CopyToClip_Click()

    If SelectedDevName <> "" Then
    
        Clipboard.Clear
        
        If InStr(UCase(SelectedDevName), "M_C_FB_") = 1 Or _
           InStr(UCase(SelectedDevName), "M_C_FG_") = 1 Or _
           InStr(UCase(SelectedDevName), "M_C_DF_") = 1 Then
           
            Clipboard.SetText ("add p " & SelectedDevName)
            
        Else
        
            Clipboard.SetText ("add " & SelectedDevName)
        End If
        
    End If
    
End Sub

Private Sub ExportEverythingTagged_Click()

    Dim wbkname As Variant
    Dim wrk As Worksheet
    Dim maxfile, aspfile, fname, mname, prevf, prevm As String
    Dim alphacut, dislight, vertalpha, animmustloop As Boolean
    Dim grandtotal, grandsuccess, grandfailed As Long
    Dim totaltodo, success_count, failed_count As Long
    Dim BlastRetCode As Long
    Dim expcell As Variant
    Dim myRange As Excel.Range
    
    Dim UseShadowFilesForExport As Boolean
    
    UseShadowFilesForExport = True
  
    Open3DStudio
    If Not MaxAvailable Then Exit Sub
    
    AbortBatchOperation.Enabled = True
    
    StatusText1.Caption = "Processing SHADOW data..."
    
    For Each wbkname In BLAST_BOOKNAMES
    
        ActivateExcelWorkBook (wbkname)
        
        If xl.ActiveWorkbook.ReadOnly = False Then
        
            BlastForm.Caption = "Blasting " & wbkname
                
            ProgressBar1.Value = 0
            
            ProgressBar2.Max = Worksheets.Count
            ProgressBar2.Value = 0
            DoEvents
            
            grandtotal = 0
            grandsuccess = 0
            grandfailed = 0
            
            For Each wrk In Worksheets
        
                If wrk.Name <> "Summary" And wrk.Name <> "Legend" Then
                                
                    wrk.Activate
                    
                    DoEvents
                    If (AbortPressed) Then
                    
                        Cleanup
                        AbortBatchOperation.Enabled = False
                        Exit Sub
                        
                    End If
                    
                    Set myRange = wrk.Range(ExportColumn + "1:" + ExportColumn + CStr(MaxSheetRows))
                    
                    For Each expcell In myRange
                    
                        If (AbortPressed) Then
                            Cleanup
                            Exit Sub
                        End If
                        
                        If (UCase(expcell.Value) = "X") Then
                            grandtotal = grandtotal + 1
                        End If
                        
                    Next
                    
                    DoEvents
                    
                End If
        
            Next wrk
            
            If grandtotal > 0 Then
                For Each wrk In Worksheets
                
                    If wrk.Name <> "Summary" And wrk.Name <> "Legend" Then
                    
                        ProgressBar2.Value = ProgressBar2.Value + 1
                        
                        DoEvents
                        
                        wrk.Activate
                        If (AbortPressed) Then
                        
                            Cleanup
                            AbortBatchOperation.Enabled = False
                            Exit Sub
                            
                        End If
                        
                        Set myRange = wrk.Range(ExportColumn + "1:" + ExportColumn + CStr(MaxSheetRows))
                       
                        success_count = 0
                        failed_count = 0
                        
                        totaltodo = 0
                        
                        For Each expcell In myRange
                        
                            With expcell
                                If UCase(.Value) = "X" Then
                                    totaltodo = totaltodo + 1
                                End If
                            End With
                            
                        Next
                        
                        If totaltodo = 0 Then
                        
                            ProgressBar1.Value = 0
                            
                        Else
                            ProgressBar1.Max = totaltodo + 1
                        
                            Label9.Caption = "0/" + CStr(grandsuccess)
                            Label10.Caption = "0/" + CStr(grandfailed)
                            Label13.Caption = CStr(totaltodo) + "/" + CStr(grandtotal)
                            
                            DoEvents
                           
                            prevf = ""
                            prevm = ""
                            
                            For Each expcell In myRange
                            
                                With expcell
                                
                                    'fname = and Range(expcell.Row, MAXColumn).Interior.PatternColor =
                                    If (UCase(.Value) = "X") Then
                                    
                                        If ProgressBar1.Value < ProgressBar1.Max Then
                                        ProgressBar1.Value = ProgressBar1.Value + 1
                                        End If
                                                
                                        fname = wrk.Cells(.Row, MaxColumn)
                                        mname = wrk.Cells(.Row, ASPColumn)
                                        
                                        If (fname = "" And mname = "") Then
                                        
                                            .Value = ""
                                            
                                        ElseIf fname = prevf And mname = prevm Then
                                        
                                            ' Don't write dups
                                            .Value = ""
                                            success_count = success_count + 1
                                            grandsuccess = grandsuccess + 1
                                            
                                        Else
                                        
                                           prevf = fname
                                           prevm = mname
                    
                                           If (AlphaCutoffColumn <> "") Then
                                               alphacut = UCase(wrk.Cells(.Row, AlphaCutoffColumn)) = "X"
                                           Else
                                               alphacut = False
                                           End If
                                           
                                           If (DisableLightColumn <> "") Then
                                               dislight = UCase(wrk.Cells(.Row, DisableLightColumn)) = "X"
                                           Else
                                               dislight = False
                                           End If
                                           
                                           If (VertexAlphaColumn <> "") Then
                                               vertalpha = UCase(wrk.Cells(.Row, VertexAlphaColumn)) = "X"
                                           Else
                                               vertalpha = False
                                           End If
                                           
                                           If (AnimMustLoopColumn <> "") Then
                                               animmustloop = UCase(wrk.Cells(.Row, AnimMustLoopColumn)) = "X"
                                           Else
                                               animmustloop = False
                                           End If
                                           
                                           StatusText2.Caption = mname
                                           Label6.Caption = alphacut
                                           Label7.Caption = dislight
                                           Label8.Caption = vertalpha
                                           
                                           DoEvents
                                       
                                           If (AbortPressed) Then
                                           
                                               Cleanup
                                               AbortBatchOperation.Enabled = False
                                               Exit Sub
                                               
                                           End If
                                       
                                           'BlastRetCode = MaxApp.BlastExportMesh(fname, mname, alphacut, dislight, vertalpha, animmustloop, False, UseShadowFilesForExport)
                                                                                      
                                            BlastRetCode = BLAST_ERROR_MAXSCRIPT_CRASH
                                            On Error Resume Next
                                            BlastRetCode = MaxApp.OpenMaxFileByName(fname, mname, UseShadowFilesForExport)
                                            Err.Clear
                                            On Error GoTo 0
                                            
                                            If BlastRetCode = BLAST_NO_ERROR Then
                                            
                                            
                                                BlastRetCode = BLAST_ERROR_MAXSCRIPT_CRASH
                                                On Error Resume Next
                                                BlastRetCode = MaxApp.BlastExportOpenMesh(alphacut, dislight, vertalpha, animmustloop, False)
                                                Err.Clear
                                                On Error GoTo 0
                                                
                                                If BlastRetCode = 0 Then
                                                     .Value = "ok"
                                                     success_count = success_count + 1
                                                     grandsuccess = grandsuccess + 1
                                                     Label9.Caption = CStr(success_count) + "/" + CStr(grandsuccess)
                                                 ElseIf BlastRetCode = BLAST_ERROR_READ_ONLY_FILE Then
                                                     ' Leave the 'X' in the value
                                                     failed_count = failed_count + 1
                                                     grandfailed = grandfailed + 1
                                                     Label10.Caption = CStr(failed_count) + "/" + CStr(grandfailed)
                                                 Else
                                                     .Value = FetchBlastError(BlastRetCode)
                                                     failed_count = failed_count + 1
                                                     grandfailed = grandfailed + 1
                                                     Label10.Caption = CStr(failed_count) + "/" + CStr(grandfailed)
                                                End If
                                                 
                                            Else
                                                .Value = FetchBlastError(BlastRetCode)
                                                failed_count = failed_count + 1
                                                grandfailed = grandfailed + 1
                                                Label10.Caption = CStr(failed_count) + "/" + CStr(grandfailed)
                                            End If
                                           
                                           
                                        End If
                                       
                                    End If
                                End With
                                
                            Next expcell
                            
                        End If
                            
                    End If
                    
                Next wrk
                
            End If
            
        End If ' readonly workbook
        
    Next wbkname
    
    BlastForm.Caption = "Art Blaster"
    StatusText1.Caption = "Done!"
    
    AbortBatchOperation.Enabled = False
    
    ProgressBar1.Value = ProgressBar1.Max
    ProgressBar2.Value = ProgressBar2.Max
    
    DoEvents
          
End Sub

Private Sub AbortBatchOperation_Click()

    AbortPressed = True
    
End Sub

Private Sub ExportSelected_Click()
        
    Dim maxfile, aspfile, fname, mname As String
    Dim alphacut, dislight, vertalpha, animmustloop As Boolean
    Dim BlastRetCode As Long
    
    SetupSelectedCell
    
    Open3DStudio
    
    If Not MaxAvailable Then Exit Sub
    
    maxfile = ActiveSheet.Cells(ActiveCell.Row, MaxColumn).Value
    
    If Len(maxfile) > 0 Then
        
        aspfile = ActiveSheet.Cells(ActiveCell.Row, ASPColumn).Value
    
        StatusText1.Caption = "Exporting..."
        StatusText2.Caption = maxfile
        Label9.Caption = ""
        Label10.Caption = ""
    
        fname = maxfile
        mname = aspfile
        
        If (AlphaCutoffColumn <> "") Then
            alphacut = UCase(ActiveSheet.Cells(ActiveCell.Row, AlphaCutoffColumn)) = "X"
        Else
            alphacut = False
        End If
        
        If (DisableLightColumn <> "") Then
            dislight = UCase(ActiveSheet.Cells(ActiveCell.Row, DisableLightColumn)) = "X"
        Else
            dislight = False
        End If
        
        If (VertexAlphaColumn <> "") Then
            vertalpha = UCase(ActiveSheet.Cells(ActiveCell.Row, VertexAlphaColumn)) = "X"
        Else
            vertalpha = False
        End If
        
        If (AnimMustLoopColumn <> "") Then
            animmustloop = UCase(ActiveSheet.Cells(ActiveCell.Row, AnimMustLoopColumn)) = "X"
        Else
            animmustloop = False
        End If
        
        StatusText2.Caption = mname
        Label6.Caption = alphacut
        Label7.Caption = dislight
        Label8.Caption = vertalpha

        DoEvents
              
        BlastRetCode = BLAST_ERROR_MAXSCRIPT_CRASH
        On Error Resume Next
        BlastRetCode = MaxApp.OpenMaxFileByName(fname, mname, False)
        Err.Clear
        On Error GoTo 0
        
        If BlastRetCode = BLAST_NO_ERROR Then
        
            BlastRetCode = BLAST_ERROR_MAXSCRIPT_CRASH
            On Error Resume Next
            BlastRetCode = MaxApp.BlastExportOpenMesh(alphacut, dislight, vertalpha, animmustloop, True)
            Err.Clear
            On Error GoTo 0
            
        End If
          
        If BlastRetCode = 0 Then
            StatusText1.Caption = "SUCCESS!"
            Label9.Caption = 1
        ElseIf BlastRetCode = -2 Then
            StatusText1.Caption = "WARNING:"
            StatusText2.Caption = FetchErrorCaption(BlastRetCode)
            Label9.Caption = 1
        Else
            StatusText1.Caption = "FAILURE:"
            StatusText2.Caption = FetchErrorCaption(BlastRetCode)
            Label10.Caption = 1
        End If
        
    End If
    
    Exit Sub
    
MaxFailure:
    Err.Clear
    StatusText1.Caption = "FAILURE:"
    StatusText2.Caption = "Lost contact with 3DS Max"

End Sub


Sub OpenExcelWorkBooks()
    
    Dim bookname As Variant
    
    Workbooks.Close
    
    For Each bookname In BLAST_BOOKNAMES
        Workbooks.Open (ArtDocDir + bookname + ".xls")
    Next bookname
    
    If (Workbooks.Count <> 5) Then
        MsgBox "Unable to open required Excel files"
        Unload Me
    End If
   
End Sub

Private Sub GetAnimationWorkbook_Click()

    ActivateExcelWorkBook (BLAST_BOOKNAMES(BLAST_ANIMATIONS))

End Sub

Private Sub GetArmorWorkbook_Click()

    ActivateExcelWorkBook (BLAST_BOOKNAMES(BLAST_ARMOR))
    
End Sub

Private Sub GetCharacterWorkbook_Click()

    ActivateExcelWorkBook (BLAST_BOOKNAMES(BLAST_CHARACTERS))
    
End Sub

Private Sub GetItemsWorkbook_Click()

    ActivateExcelWorkBook (BLAST_BOOKNAMES(BLAST_ITEMS))

End Sub

Private Sub GetWeaponsWorkbook_Click()

    ActivateExcelWorkBook (BLAST_BOOKNAMES(BLAST_WEAPONS))
    
    
End Sub

Sub ActivateExcelWorkBook(bookname As String)

    Dim wb As Workbook
        
    xl.Visible = True
    
    Set wb = xl.Workbooks(bookname + ".xls")
    
    xl.EnableEvents = False
    wb.Activate
    xl.EnableEvents = True
    
    SetupActiveWorkBook wb
    
End Sub

Sub SetupActiveWorkBook(wb As Excel.Workbook)

    Dim wrk As Worksheet
    Dim w As Long
    Dim i As Long
   
    For Each wrk In Worksheets
    
        If wrk.Name = "Legend" Then
                                                                                                                    
            ExportColumn = wrk.Range("A2").Value
            MaxColumn = wrk.Range("B2").Value
            ASPColumn = wrk.Range("C2").Value
            AlphaCutoffColumn = wrk.Range("D2").Value
            DisableLightColumn = wrk.Range("E2").Value
            VertexAlphaColumn = wrk.Range("F2").Value
            DevNameColumn = wrk.Range("G2").Value
            AnimMustLoopColumn = wrk.Range("L2").Value
                
            For i = 2 To (2 + MAX_TEXTURES - 1)
                TextureColumns(i - 2) = wrk.Range("H" & i).Value
            Next
            
            ValidArtDoc = ExportColumn <> "" And MaxColumn <> "" And ASPColumn <> ""
            
        End If
    Next
    
    If ValidArtDoc Then
    
        For w = 1 To wb.Windows.Count
            wb.Windows(w).WindowState = xlMaximized
            wb.Windows(w).Activate
        Next
    
        wb.ActiveSheet.Visible = True
            
        If (Not MaxAvailable) Then
            Me.Height = NoMaxDialogHeight
        Else
            If Not ActiveWorkbook.ReadOnly Then
                Me.Height = FullDialogHeight
            Else
                Me.Height = NoBlastDialogHeight
            End If
        End If
    
    End If
    
    SetupSelectedCell
    
    DoEvents
   
    'hwnd = WindowsAPI.FindWindowByTitle("Microsoft Excel - " & bookname)
    'MsgBox ("Setting focus to " & hwnd)
    'WindowsAPI.SetFocus (hwnd)
    
    
End Sub

Private Sub xl_SheetActivate(ByVal Sh As Object)

    SetupSelectedCell

End Sub

Private Sub xl_SheetSelectionChange(ByVal Sh As Object, ByVal Target As Excel.Range)

    SetupSelectedCell
    
End Sub

Private Sub xl_WorkbookActivate(ByVal wb As Excel.Workbook)

    SetupActiveWorkBook wb
  
End Sub

Private Sub xl_WorkbookBeforeClose(ByVal wb As Excel.Workbook, Cancel As Boolean)

    Unload BlastForm
    
End Sub

Private Sub OpenSelected_Click()

    Open3DStudio
    
    If Not MaxAvailable Then Exit Sub
    
    SetupSelectedCell
    
    Dim maxfile, aspname As String
    Dim retcode As Long

    maxfile = ActiveSheet.Cells(ActiveCell.Row, MaxColumn).Value
    aspname = ActiveSheet.Cells(ActiveCell.Row, ASPColumn).Value
    
    If Len(maxfile) > 0 Then
    
        StatusText1.Caption = "Opening..."
        StatusText2.Caption = maxfile
        
        retcode = MaxApp.OpenMaxFileByName(maxfile, aspname, False)
        
        If retcode = BLAST_NO_ERROR Then
            StatusText1.Caption = "SUCCESS!"
        ElseIf retcode = BLAST_ERROR_MISSING_FILE Then
            StatusText1.Caption = "FAILURE! File not found"
        ElseIf retcode = BLAST_ERROR_MISSING_MESH Then
            StatusText1.Caption = "WARNING: Mesh not found"
        Else
            StatusText1.Caption = "FAILURE: Check MaxScript Listener!"
        End If
        
    End If

End Sub

Private Sub Form_Activate()

    On Error Resume Next
    
    ' Flaky VB crap!
    BuildBookNameArray
    
    SetTopMostWindow Me.hwnd, True
    
    StatusText1.Caption = "Loading Excel..."
    DoEvents
    
    Set xl = CreateObject("Excel.Application")
    xl.Visible = True
    
    OpenExcelWorkBooks
    ActivateExcelWorkBook (BLAST_BOOKNAMES(BLAST_CHARACTERS))
    
    StatusText1.Caption = "Loading 3DStudio Max..."
    DoEvents
    
    MaxAvailable = CheckFor3DStudio
    If (MaxAvailable) Then
        StatusText1.Caption = "Can Export"
        StatusText2.Caption = "3DStudio Max is available"
    Else
        StatusText1.Caption = "Cannot Export"
        StatusText2.Caption = "3DStudio Max is not available"
    End If

    ReDim ViewerHandles(1)
    
End Sub


Private Sub Form_Terminate()

    Dim h As Long
    
    Cleanup
    
    For h = 0 To UBound(ViewerHandles) - 1
        If ViewerHandles(h) <> 0 Then
            SendCloseMessage (ViewerHandles(h))
        End If
    Next
    
    ' Bring Excel to the front so we see its 'save changes' dialog
    ' ok = SetFocus(hwndXL)
    
    xl.Quit
    
    Set xl = Nothing
    Set MaxApp = Nothing
    
End Sub

Private Sub ViewObject()

    Dim aspfile, viewcmd As String
    Dim pos As Long
    Dim i As Integer
    Dim textureval As String

    Dim hinstViewer As Long
    
    SetupSelectedCell
 
    With ActiveSheet
    
        aspfile = .Cells(ActiveCell.Row, ASPColumn).Value
        
        If Len(aspfile) > 0 Then
        
            StatusText1.Caption = "Preview..."
            StatusText2.Caption = aspfile
            
            pos = InStr(aspfile, ".")
            
            If pos > 0 Then
                aspfile = Left(aspfile, pos - 1)
            End If
    
            viewcmd = aspfile
            
            If UseLocalFile Then
                viewcmd = viewcmd + " defaultdir=" + LocalAssets + "\cdimage"
            End If
            
            For i = 0 To MAX_TEXTURES - 1
                If TextureColumns(i) <> "" Then
                
                    textureval = .Cells(ActiveCell.Row, TextureColumns(i)).Value
                    
                    pos = InStr(textureval, ".")
       
                    If pos > 0 Then
                        textureval = Left(textureval, pos - 1)
                    End If

                    If (textureval <> "") Then
                        viewcmd = viewcmd & " TEXTURE" & CStr(i) & "=" _
                                 & Chr(34) & textureval & Chr(34)
                    End If
                End If
            Next
            
            viewcmd = "\\packmule\vss_shadow\TATTOO_ASSETS\Tools\ANIMVIEWER.EXE QUICKVIEW=" + viewcmd
'            viewcmd = "E:\GPG_HOME\GPG\Projects\AnimViewer\Debug\ANIMVIEWER.EXE QUICKVIEW=" & viewcmd
    
            hinstViewer = Shell(viewcmd, 1)
            
            If hinstViewer <> 0 Then
                ' Kill off the current Viewer
                ' hwndViewer = FindWindowByInstanceHandle(hinstViewer)
                ' hwndViewer = GetWinHandle(hinstViewer)
                 ViewerHandles(UBound(ViewerHandles) - 1) = FindWindowByTitle((aspfile))
                 
                ' Add the current viewer to the list of apps we want to shut down
                StatusText1.Caption = "If not visible, check name on spreadsheet"
            Else
                ' Need to be able to wait for a return?
                StatusText1.Caption = "Was not able to view object"
                ViewerHandles(UBound(ViewerHandles) - 1) = 0
            End If
      
        End If
        
    End With

End Sub

Private Sub ViewLocal_Click()

    UseLocalFile = True
    
    ' Add the current viewer to the list of apps we want to shut down
    ReDim Preserve ViewerHandles(UBound(ViewerHandles) + 1)
    
    AdvanceToNextCell.Enabled = True
    
    ViewObject

End Sub
Private Sub ViewShadow_Click()

    UseLocalFile = False
    
    ReDim Preserve ViewerHandles(UBound(ViewerHandles) + 1)
    
    AdvanceToNextCell.Enabled = True
    
    ViewObject
    
End Sub


