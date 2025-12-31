VERSION 5.00
Begin VB.Form FrmPath 
   Caption         =   "Form1"
   ClientHeight    =   2025
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   2595
   LinkTopic       =   "Form1"
   ScaleHeight     =   2025
   ScaleWidth      =   2595
   StartUpPosition =   1  'CenterOwner
   Begin VB.CommandButton btnCancel 
      Caption         =   "&Cancel"
      Height          =   370
      Left            =   1920
      TabIndex        =   3
      Top             =   960
      Width           =   610
   End
   Begin VB.CommandButton cmdSet 
      Caption         =   "&Set"
      Height          =   370
      Left            =   1920
      TabIndex        =   2
      Top             =   1440
      Width           =   610
   End
   Begin VB.DirListBox Dir1 
      Height          =   1380
      Left            =   120
      TabIndex        =   1
      Top             =   360
      Width           =   1690
   End
   Begin VB.DriveListBox Drive1 
      Height          =   280
      Left            =   120
      TabIndex        =   0
      Top             =   0
      Width           =   1690
   End
End
Attribute VB_Name = "FrmPath"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub btnCancel_Click()
Unload Me

End Sub

Private Sub cmdSet_Click()

Dim sCheckDir As String
Dim sCurrentPath As String
Dim bPathIsGood As Boolean


On Error GoTo ErrHandler


sCurrentPath = Dir1
If Not bIsSkritPathOk Or Not bIsDBPathOk Then


    Select Case FrmPath.Caption
        
        Case "Skrit Path":
            sCheckDir = Dir(Dir1 & "\*.skrit")
            
            If sCheckDir = "" Then
                MsgBox "No Skrits found, please select a valid path."
                bIsSkritPathOk = False
            
            ElseIf sCheckDir > "" Then
                sSkritPath = Dir1
                bIsSkritPathOk = True
               ' frmSettings.txtSkritPath.Text = sSkritPath
                Unload Me
                
            End If
        
        Case "Exe Path":
           sCheckDir = Dir(Dir1 & "\DungeonSiege?.exe")
            
            If sCheckDir = "" Then
                MsgBox "Dungeon Siege executables not found.  Please enter the correct path."
                bIsDBPathOk = False
            
            ElseIf sCheckDir > "" Then
                sDBPath = Dir1
                bIsDBPathOk = True
                frmSettings.txtExepath.Text = sDBPath
                Unload Me
                
            End If
    End Select

End If


Exit Sub

ErrHandler:
    MsgBox "Directory does not exist, or Dirve not ready, Please enter a valid drive.", vbCritical
    

End Sub



Private Sub Drive1_Change()

On Error Resume Next
Dir1.Path = Drive1.Drive
End Sub

Private Sub Form_Load()

FrmPath.Caption = sPathType

On Error GoTo ErrHandler

Select Case FrmPath.Caption
    
    Case "Skrit Path":
       ' Drive1.Drive = Left$(frmSettings.txtSkritPath.Text, 4)
        'Dir1.Path = frmSettings.txtSkritPath.Text
    
    Case "Exe Path":
        Drive1.Drive = Left$(frmSettings.txtExepath.Text, 4)
        Dir1.Path = frmSettings.txtExepath.Text
        
End Select
Exit Sub

ErrHandler:
    MsgBox "Invalid drive or path specified", vbCritical
    


End Sub
