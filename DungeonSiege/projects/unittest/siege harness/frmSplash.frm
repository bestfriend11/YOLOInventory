VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form frmSplash 
   BackColor       =   &H00FFFFC0&
   BorderStyle     =   0  'None
   ClientHeight    =   1890
   ClientLeft      =   0
   ClientTop       =   0
   ClientWidth     =   4350
   Icon            =   "frmSplash.frx":0000
   LinkTopic       =   "Form1"
   ScaleHeight     =   1890
   ScaleWidth      =   4350
   ShowInTaskbar   =   0   'False
   Begin MSComctlLib.ProgressBar ProgressBar1 
      Height          =   315
      Left            =   150
      TabIndex        =   0
      Top             =   960
      Width           =   4095
      _ExtentX        =   7223
      _ExtentY        =   556
      _Version        =   393216
      Appearance      =   1
   End
   Begin VB.Label lblCopyRight 
      BackColor       =   &H00FFFFC0&
      BeginProperty Font 
         Name            =   "Palatino Linotype"
         Size            =   15.75
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      ForeColor       =   &H000040C0&
      Height          =   360
      Left            =   165
      TabIndex        =   3
      Top             =   1335
      Width           =   4095
   End
   Begin VB.Label lblTitle 
      BackColor       =   &H00FFFFC0&
      BeginProperty Font 
         Name            =   "Palatino Linotype"
         Size            =   18
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      ForeColor       =   &H000040C0&
      Height          =   510
      Left            =   165
      TabIndex        =   2
      Top             =   90
      Width           =   4095
   End
   Begin VB.Label lblLoad 
      BackColor       =   &H00FFFFC0&
      Caption         =   "Load Progress:"
      Height          =   240
      Left            =   165
      TabIndex        =   1
      Top             =   690
      Width           =   4065
   End
End
Attribute VB_Name = "frmSplash"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Form_Load()
   lblCopyRight.Caption = App.LegalCopyright
   lblTitle.Caption = App.ProductName
   modSpells.bSplashOn = True
   Left = (Screen.Width - Me.Width) / 4&
   Top = (Screen.Height - Me.Height) / 2&
End Sub

Private Sub Form_Unload(Cancel As Integer)
   modSpells.bSplashOn = False
End Sub
