VERSION 5.00
Begin VB.Form frmSettings 
   BorderStyle     =   4  'Fixed ToolWindow
   Caption         =   "Preferences"
   ClientHeight    =   8100
   ClientLeft      =   -15
   ClientTop       =   -60
   ClientWidth     =   10365
   ControlBox      =   0   'False
   Icon            =   "frmSettings.frx":0000
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   8100
   ScaleWidth      =   10365
   ShowInTaskbar   =   0   'False
   StartUpPosition =   2  'CenterScreen
   Begin VB.Frame frameCaseMain 
      Caption         =   "Frame1"
      Height          =   7575
      Left            =   0
      TabIndex        =   0
      Top             =   0
      Width           =   5055
      Begin VB.Frame frameGeneral 
         Caption         =   "General Settings"
         Height          =   1450
         Left            =   0
         TabIndex        =   21
         Top             =   3600
         Width           =   4810
         Begin VB.CheckBox chkLogMin 
            Caption         =   "Minimize Log Window while suite is running"
            Height          =   250
            Left            =   120
            TabIndex        =   23
            Top             =   480
            Value           =   1  'Checked
            Width           =   4210
         End
         Begin VB.CheckBox chkHarnessMin 
            Caption         =   "Minimize Harness while suite is running"
            Height          =   250
            Left            =   120
            TabIndex        =   22
            Top             =   240
            Value           =   1  'Checked
            Width           =   3370
         End
         Begin VB.Label Label1 
            Caption         =   "SQL server:"
            Height          =   250
            Left            =   120
            TabIndex        =   24
            Top             =   720
            Width           =   970
         End
      End
      Begin VB.Frame frameGameSettings 
         Caption         =   "Dungeon Siege Settings"
         Height          =   1690
         Left            =   0
         TabIndex        =   11
         ToolTipText     =   "Set your Dungeon Siege settings here"
         Top             =   1800
         Width           =   4810
         Begin VB.CheckBox chkWIndowed 
            Caption         =   "Fullscreen"
            Height          =   250
            Left            =   3600
            TabIndex        =   18
            Top             =   240
            Width           =   1090
         End
         Begin VB.TextBox Text1 
            Enabled         =   0   'False
            Height          =   370
            Left            =   1440
            TabIndex        =   17
            Text            =   "auto\test"
            Top             =   1200
            Width           =   2290
         End
         Begin VB.CommandButton Command2 
            Caption         =   "Modify"
            Height          =   370
            Left            =   3840
            TabIndex        =   16
            Top             =   1200
            Width           =   850
         End
         Begin VB.CommandButton cmdBrowse 
            Caption         =   "..."
            Height          =   370
            Left            =   3840
            TabIndex        =   15
            Top             =   720
            Width           =   850
         End
         Begin VB.TextBox txtExepath 
            Height          =   370
            Left            =   1440
            TabIndex        =   14
            Text            =   "c:\"
            Top             =   720
            Width           =   2290
         End
         Begin VB.OptionButton Option2 
            Caption         =   "Debug"
            Height          =   250
            Left            =   1080
            TabIndex        =   13
            Top             =   240
            Width           =   850
         End
         Begin VB.OptionButton optRelease 
            Caption         =   "Release"
            Height          =   250
            Left            =   120
            TabIndex        =   12
            Top             =   240
            Value           =   -1  'True
            Width           =   970
         End
         Begin VB.Label Label4 
            Caption         =   "Skrit Path:"
            Height          =   250
            Left            =   120
            TabIndex        =   20
            Top             =   1200
            Width           =   1210
         End
         Begin VB.Label Label3 
            Caption         =   "Executable Path:"
            Height          =   250
            Left            =   120
            TabIndex        =   19
            Top             =   720
            Width           =   1330
         End
      End
      Begin VB.Frame frameUser 
         Caption         =   "User Settings"
         Height          =   1450
         Left            =   0
         TabIndex        =   4
         ToolTipText     =   " User settings: Do not modify unless they are incorrect"
         Top             =   0
         Width           =   4810
         Begin VB.Label lblPlatform 
            Height          =   610
            Left            =   840
            TabIndex        =   10
            Top             =   720
            Width           =   3730
         End
         Begin VB.Label lblPlat 
            Caption         =   "Platform:"
            Height          =   250
            Left            =   120
            TabIndex        =   9
            Top             =   720
            Width           =   4570
         End
         Begin VB.Label lblMachineName 
            Height          =   250
            Left            =   1320
            TabIndex        =   8
            Top             =   480
            Width           =   2410
         End
         Begin VB.Label lblUserName 
            Height          =   250
            Left            =   1080
            TabIndex        =   7
            Top             =   240
            Width           =   2650
         End
         Begin VB.Label lblMachinetxt 
            Caption         =   "Machine name:"
            Height          =   250
            Left            =   120
            TabIndex        =   6
            Top             =   480
            Width           =   1450
         End
         Begin VB.Label lblUsertxt 
            Caption         =   "User name:"
            Height          =   250
            Left            =   120
            TabIndex        =   5
            Top             =   240
            Width           =   1210
         End
      End
      Begin VB.CommandButton cmdApply 
         Caption         =   "&Apply"
         Height          =   370
         Left            =   3000
         TabIndex        =   3
         Top             =   5400
         Width           =   850
      End
      Begin VB.CommandButton cmdOk 
         Caption         =   "&Ok"
         Height          =   370
         Left            =   3960
         TabIndex        =   2
         Top             =   5400
         Width           =   850
      End
      Begin VB.CommandButton cmdCancel 
         Caption         =   "&Cancel"
         Height          =   370
         Left            =   2040
         TabIndex        =   1
         Top             =   5400
         Width           =   850
      End
   End
End
Attribute VB_Name = "frmSettings"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False



Private mintCurFrame As Integer



Private Sub btnExit_Click()
    Unload frmMain
    Unload Me
End Sub


Private Sub cmdSave_Click()
    '/ make sure everything works
    If txtUserName = "" Then
        MsgBox "Please enter a valid user name"
    Else
        sUserName = txtUserName
    End If
    
    
    If bIsDBPathOk Then
        WriteIni sSkritPath, sDBPath, sUserName
        Unload Me
    Else
        MsgBox "Paths are invalid.  Please enter valid paths."
    End If
End Sub
Private Sub cmdDB_Click()
    sPathType = PathType.PT_DB_PATH
    With CommonDialog2
        .FileName = ""
        .DialogTitle = "Select either Release or Debug"
        .Filter = "UnitTestDB (UnitTestdb.mdb)|UnitTestdb.mdb"
        
        .Flags = cdlOFNFileMustExist Or cdlOFNHideReadOnly Or cdlOFNExplorer Or cdlOFNPathMustExist
        .ShowOpen
        
    End With
        If CommonDialog2.FileName > "" Then
            txtDBPath.Text = CommonDialog2.FileName
            bIsDBPathOk = True
        End If
End Sub

Private Sub cmdExe_Click()
    sPathType = PathType.PT_EXE_PATH
    With CommonDialog2
        .DialogTitle = "Select either Release or Debug"
        .Filter = "Dungeon Siege Release (DungeonSiegeR.exe)|DungeonSiegeR.exe|Dungeon Siege Debug (DungeonSiegeD.exe)|DungeonSiegeD.exe"
        .Flags = cdlOFNFileMustExist Or cdlOFNHideReadOnly Or cdlOFNExplorer
        .ShowOpen
        
    End With
        If CommonDialog2.FileName > "" Then
            txtExepath.Text = CommonDialog2.FileName
        End If
End Sub



Private Sub cmdCancel_Click()
Unload Me

End Sub


Private Sub Form_Load()
With CurrentUserInfo
    lblPlatform.Caption = .OS
    lblMachineName.Caption = .MachineName
    lblUserName.Caption = .UserName
End With
End Sub

