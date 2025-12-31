VERSION 5.00
Object = "{3B7C8863-D78F-101B-B9B5-04021C009402}#1.2#0"; "RICHTX32.OCX"
Begin VB.Form frmRptLog 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Result of testing all spells"
   ClientHeight    =   10560
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   16605
   FillColor       =   &H00FFFFFF&
   FillStyle       =   0  'Solid
   LinkTopic       =   "Form1"
   LockControls    =   -1  'True
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   10560
   ScaleWidth      =   16605
   StartUpPosition =   1  'CenterOwner
   Begin VB.CommandButton Command1 
      Caption         =   "E&xit"
      Height          =   450
      Left            =   15360
      TabIndex        =   4
      Top             =   9960
      Width           =   1140
   End
   Begin RichTextLib.RichTextBox RichTextBox1 
      Height          =   9825
      Left            =   1230
      TabIndex        =   3
      Top             =   0
      Width           =   15315
      _ExtentX        =   27014
      _ExtentY        =   17330
      _Version        =   393217
      BackColor       =   12632256
      ScrollBars      =   3
      RightMargin     =   2.00000e5
      TextRTF         =   $"frmRptLog.frx":0000
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "Courier New"
         Size            =   9
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
   End
   Begin VB.VScrollBar VScroll1 
      Height          =   1320
      Left            =   885
      TabIndex        =   2
      Top             =   135
      Width           =   240
   End
   Begin VB.Label lblTotal 
      Caption         =   "0"
      Height          =   195
      Left            =   45
      TabIndex        =   8
      Top             =   1140
      Width           =   795
   End
   Begin VB.Label lblOf 
      Caption         =   "of"
      Height          =   195
      Left            =   30
      TabIndex        =   7
      Top             =   847
      Width           =   210
   End
   Begin VB.Label lblDisplay 
      Caption         =   "0"
      Height          =   195
      Index           =   1
      Left            =   75
      TabIndex        =   6
      Top             =   2205
      Width           =   810
   End
   Begin VB.Label lblPasses 
      Caption         =   "Passes:"
      Height          =   225
      Left            =   90
      TabIndex        =   5
      Top             =   1935
      Width           =   600
   End
   Begin VB.Shape shpStatus 
      FillColor       =   &H000000FF&
      FillStyle       =   0  'Solid
      Height          =   405
      Left            =   360
      Shape           =   3  'Circle
      Top             =   1440
      Width           =   390
   End
   Begin VB.Label lblDisplay 
      Caption         =   "0"
      Height          =   195
      Index           =   0
      Left            =   75
      TabIndex        =   1
      Top             =   555
      Width           =   765
   End
   Begin VB.Label lblFailures 
      Caption         =   "&Failures:"
      Height          =   255
      Left            =   75
      TabIndex        =   0
      Top             =   135
      Width           =   570
   End
End
Attribute VB_Name = "frmRptLog"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Public sFile$, nMaxErrors&, nMaxPass&
Dim tErrLookup()
Dim nOldStrt&, nOldLen&

Private Sub Command1_Click()
   Unload Me
End Sub

Private Sub Form_Load()
    Dim nFF%, FileLength&, sDummy$
    
    On Error GoTo erh
    nFF = FreeFile
    Open sFile For Input As nFF
    FileLength = LOF(nFF)   ' Get length of file.
   
    nMaxPass = 0&
    nMaxErrors = 0&
    If (FileLength > 0&) Then
          Screen.MousePointer = vbHourglass
          RichTextBox1.Text = Input(FileLength, #nFF)
          Call processStatus(RichTextBox1, "Fail", vbRed, 0)
          Call processStatus(RichTextBox1, "Pass", vbGreen, 1)
          Call processStatus(RichTextBox1, "Total", vbMagenta, -1)
          Call processStatus(RichTextBox1, sAstrskMsg, vbCyan, -2)
          Select Case nMaxErrors
              Case 0
                   shpStatus.FillColor = vbGreen
              Case 1 To 9
                   shpStatus.FillColor = vbYellow
              Case Else
                   shpStatus.FillColor = vbRed
          End Select
          If nMaxErrors > 0 Then
              VScroll1.Max = nMaxErrors
              VScroll1.Min = 1&
          Else
              VScroll1.Max = 0
              VScroll1.Min = 0
          End If
          lblDisplay(0).Caption = Format(nMaxErrors, "###,##0")
          VScroll1.LargeChange = nMaxErrors / 5& + 1   ' Cross in 5 clicks.
          VScroll1.SmallChange = 1
          VScroll1.Value = VScroll1.Max
          lblDisplay(1).Caption = Format(nMaxPass, "###,##0")
          lblTotal.Caption = Format(VScroll1.Max, "###,##0")
          Screen.MousePointer = vbDefault
    End If
    Close #nFF
    nFF = -1
    Exit Sub
erh:
   If nFF > 0 Then
        Close #nFF  ' ensure closed
   End If
   MsgBox "Read log error " & CStr(Err.Number) & ": " & Err.Description
   Err.Clear
End Sub
Sub processStatus(ByRef cCtrl As RichTextBox, ByVal sSrch$, ByVal nCol&, nIndex%)
    Dim nStrt&, nSpc&, bCounted As Boolean
    
    On Error GoTo 0 ' stop on error
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
End Sub

Private Sub RichTextBox1_KeyPress(KeyAscii As Integer)
    KeyAscii = 0  ' swallow key strokes
End Sub

Private Sub VScroll1_Change()
    Dim nVal%
    
    nVal = VScroll1.Value   ' VScroll1.Max - VScroll1.Value + 1
    lblDisplay(0).Caption = Format(nVal, "###,##0")
    If ((nOldStrt > 0&) And (nOldLen > 0&)) Then
        RichTextBox1.SelColor = vbRed  ' unhighlight previous current error indicator
    End If
    If (nMaxErrors > 0&) Then   '  highlight new current error indicator
        RichTextBox1.SelStart = tErrLookup(VScroll1.Max - VScroll1.Value) - 1
        RichTextBox1.SelLength = 4
        RichTextBox1.SelColor = RGB(128, 0, 0)
        nOldStrt = RichTextBox1.SelStart  ' save current as previous
        nOldLen = RichTextBox1.SelLength

        RichTextBox1.Refresh
    End If
End Sub
