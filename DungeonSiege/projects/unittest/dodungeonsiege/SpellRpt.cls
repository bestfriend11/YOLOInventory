VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "SpellRpt"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit

' SpellRpt  - terse Report on outcome of spells tested:
' spec:  what build was tested; computer name; date & time
'
Private sFileName$, nFF%, nCurLine%

Private Sub init()
    sFileName = frmMain.sCurDir & "\SpellLog.txt"
    nFF = FreeFile
    On Error GoTo erh
    Open sFileName For Append As nFF
    
    Exit Sub
erh:
   MsgBox "SpellRpt.init() " & CStr(Err.Number) & ": " & Err.Description
   Err.Clear
   nFF = -1
End Sub

Private Sub deInit()
    If nFF >= 0 Then
        Close #nFF
        nFF = -1
    End If
End Sub

Public Sub AddPiece(ByVal sPiece$)
    init
    If nFF >= 0 Then
        Print #nFF, sPiece;
        ' above syntax will not end line with cr, lf
        deInit
    End If
End Sub
