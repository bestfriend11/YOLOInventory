VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form frmConfig 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "SKRIT configuration"
   ClientHeight    =   3285
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   6360
   Icon            =   "frmConfig.frx":0000
   LinkTopic       =   "Form1"
   LockControls    =   -1  'True
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   3285
   ScaleWidth      =   6360
   StartUpPosition =   1  'CenterOwner
   Begin MSComctlLib.StatusBar StatusBar1 
      Align           =   2  'Align Bottom
      Height          =   300
      Left            =   0
      TabIndex        =   16
      Top             =   2985
      Width           =   6360
      _ExtentX        =   11218
      _ExtentY        =   529
      Style           =   1
      _Version        =   393216
      BeginProperty Panels {8E3867A5-8586-11D1-B16A-00C0F0283628} 
         NumPanels       =   1
         BeginProperty Panel1 {8E3867AB-8586-11D1-B16A-00C0F0283628} 
         EndProperty
      EndProperty
   End
   Begin VB.Frame frmRootPath 
      Caption         =   "Dungeon Siege Root &Path"
      Height          =   1095
      Left            =   1260
      TabIndex        =   13
      Top             =   1320
      Width           =   5055
      Begin VB.DirListBox Dir1 
         Height          =   765
         Left            =   1980
         TabIndex        =   15
         Top             =   225
         Width           =   3000
      End
      Begin VB.DriveListBox Drive1 
         Height          =   315
         Left            =   105
         TabIndex        =   14
         Top             =   225
         Width           =   1800
      End
   End
   Begin VB.CheckBox chkWindowed 
      Caption         =   "&Windowed"
      Height          =   360
      Left            =   2850
      TabIndex        =   12
      ToolTipText     =   "Check for Windowed output, uncheck for screen output."
      Top             =   75
      Width           =   1080
   End
   Begin VB.TextBox txtMap 
      Height          =   330
      Left            =   2490
      TabIndex        =   11
      Top             =   960
      Width           =   3825
   End
   Begin VB.TextBox txtStartLoc 
      Height          =   330
      Left            =   2490
      TabIndex        =   10
      Top             =   525
      Width           =   3825
   End
   Begin VB.CheckBox chkRetrySkrit 
      Caption         =   "Retry &Skrit"
      Height          =   360
      Left            =   4192
      TabIndex        =   7
      ToolTipText     =   "Check here to retry compilation after editing skrit file, in event of skrit error."
      Top             =   75
      Width           =   1080
   End
   Begin VB.CheckBox chkDemo 
      Caption         =   "De&mo"
      Height          =   360
      Left            =   5535
      TabIndex        =   6
      ToolTipText     =   "Produces no assertions, warnings, errors or fatal log files."
      Top             =   75
      Width           =   750
   End
   Begin VB.Frame Frame1 
      Caption         =   "Type of exe"
      Height          =   1200
      Left            =   60
      TabIndex        =   2
      Top             =   45
      Width           =   1110
      Begin VB.OptionButton Option1 
         Caption         =   "Retai&l"
         Enabled         =   0   'False
         Height          =   240
         Index           =   2
         Left            =   150
         TabIndex        =   5
         ToolTipText     =   "Currently disabled because the console object is not instantiated in the retail version. "
         Top             =   900
         Width           =   740
      End
      Begin VB.OptionButton Option1 
         Caption         =   "&Release"
         Height          =   240
         Index           =   1
         Left            =   150
         TabIndex        =   4
         ToolTipText     =   "Test the release version."
         Top             =   585
         Width           =   880
      End
      Begin VB.OptionButton Option1 
         Caption         =   "&Debug"
         Height          =   240
         Index           =   0
         Left            =   150
         TabIndex        =   3
         ToolTipText     =   "Test the debug version."
         Top             =   270
         Width           =   800
      End
   End
   Begin VB.CommandButton cmdExit 
      Caption         =   "C&ancel"
      Height          =   315
      Index           =   1
      Left            =   5325
      TabIndex        =   1
      Top             =   2535
      Width           =   990
   End
   Begin VB.CommandButton cmdExit 
      Caption         =   "&Ok"
      Height          =   315
      Index           =   0
      Left            =   4140
      TabIndex        =   0
      Top             =   2535
      Width           =   990
   End
   Begin VB.Label lblMap 
      Caption         =   "Map &Name:"
      Height          =   225
      Left            =   1560
      TabIndex        =   9
      Top             =   1013
      Width           =   900
   End
   Begin VB.Label lblStart 
      Caption         =   "Start &Location:"
      Height          =   270
      Left            =   1335
      TabIndex        =   8
      Top             =   555
      Width           =   1125
   End
End
Attribute VB_Name = "frmConfig"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub cmdExit_Click(Index As Integer)
   Dim i%
   
   If Index = 0 Then
        ' save settings
        If Not all_ok() Then
            Exit Sub
        Else
            For i = 0 To Option1.Count - 1
               If Option1(i).Value Then
                  frmMain.tChosen = i
                  Exit For
               End If
            Next i
            frmMain.bRetrySkrit = (chkRetrySkrit.Value = 1)
            frmMain.bDemo = (chkDemo.Value = 1)
            frmMain.sStartLoc = txtStartLoc.Text
            frmMain.sMap = txtMap.Text
            frmMain.bWindowed = (chkWindowed.Value = 1)
            frmMain.sCurDir = Dir1.Path
        End If
   End If
   Unload Me
End Sub

Private Function Dir1_hasNo_exe()
    Dim sExePath$
    
    sExePath = Dir1.Path & "\dun*.exe"
    If (Dir(sExePath, vbNormal) = "") Then ' no ds exe file!
        Dir1_hasNo_exe = True
        StatusBar1.SimpleText = "Your chosen directory must have a Dungeon Siege executable."
    Else
        Dir1_hasNo_exe = False
        StatusBar1.SimpleText = ""
    End If
End Function
Private Function Dir1_hasNo_map()
    Dim sMapPath$
    
    sMapPath = Dir1.Path & "\world\maps\" & txtMap.Text
    If (Dir(sMapPath, vbDirectory) = "") Then ' no ds exe file!
        Dir1_hasNo_map = True
        StatusBar1.SimpleText = "Your chosen map name must exist in subfolder world\maps"
    Else
        Dir1_hasNo_map = False
        StatusBar1.SimpleText = ""
    End If
End Function

Private Sub Form_Load()
   Option1(frmMain.tChosen).Value = True
   chkRetrySkrit.Value = IIf(frmMain.bRetrySkrit, 1, 0)
   chkDemo.Value = IIf(frmMain.bDemo, 1, 0)
   txtStartLoc.Text = frmMain.sStartLoc
   txtMap.Text = frmMain.sMap
   chkWindowed.Value = IIf(frmMain.bWindowed, 1, 0)
   Dir1.Path = frmMain.sCurDir
End Sub

Private Sub Form_QueryUnload(Cancel As Integer, UnloadMode As Integer)
    Cancel = Not all_ok()
End Sub

Private Function all_ok() As Boolean
    all_ok = True
    If Dir1_hasNo_exe() Then
        Beep
        Dir1.SetFocus
        all_ok = False
    Else
        ' check txtMap.text next
        If Dir1_hasNo_map() Then
            Beep
            txtMap.SetFocus
            all_ok = False
        End If
    End If
End Function
