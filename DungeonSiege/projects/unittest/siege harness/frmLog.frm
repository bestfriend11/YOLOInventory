VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Object = "{3B7C8863-D78F-101B-B9B5-04021C009402}#1.2#0"; "RICHTX32.OCX"
Begin VB.Form frmLog 
   Caption         =   "Form1"
   ClientHeight    =   6560
   ClientLeft      =   40
   ClientTop       =   330
   ClientWidth     =   9830
   LinkTopic       =   "Form1"
   ScaleHeight     =   6560
   ScaleWidth      =   9830
   StartUpPosition =   3  'Windows Default
   Begin MSComctlLib.ListView lstLogCase 
      Height          =   6130
      Left            =   120
      TabIndex        =   1
      Top             =   0
      Width           =   5050
      _ExtentX        =   8908
      _ExtentY        =   10813
      View            =   3
      LabelWrap       =   -1  'True
      HideSelection   =   -1  'True
      _Version        =   393217
      ForeColor       =   -2147483640
      BackColor       =   -2147483643
      BorderStyle     =   1
      Appearance      =   1
      NumItems        =   3
      BeginProperty ColumnHeader(1) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         Text            =   "Case"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(2) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         SubItemIndex    =   1
         Text            =   "Action"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(3) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         SubItemIndex    =   2
         Text            =   "Time"
         Object.Width           =   2540
      EndProperty
   End
   Begin RichTextLib.RichTextBox RichTextBox1 
      Height          =   6130
      Left            =   5160
      TabIndex        =   0
      Top             =   0
      Width           =   4450
      _ExtentX        =   7849
      _ExtentY        =   10813
      _Version        =   393217
      Enabled         =   -1  'True
      TextRTF         =   $"frmLog.frx":0000
   End
End
Attribute VB_Name = "frmLog"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
 

Private Sub Form_Load()
Dim lstCaseItem As ListItem
Set lstCaseItem = lstLogCase.ListItems.Add(, , "No Cases run")
End Sub

Private Sub lstCases_BeforeLabelEdit(Cancel As Integer)
End Sub
