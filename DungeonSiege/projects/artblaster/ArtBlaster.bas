Attribute VB_Name = "BlastModule"
Option Explicit

Public ArtDocDir As String
Public Const SmallDialogHeight = 1068   ' ViewShadow.Top
Public Const NoMaxDialogHeight = 2052   ' OpenSelected.Top
Public Const NoBlastDialogHeight = 2472 ' AbortBatchOperation.Top
Public Const FullDialogHeight = 4450
    

Sub Main()
    On Error GoTo 0
    
    'FindWindowByTitle ("ArtBlaster")

    Dim DOCSroot As String
    
    DOCSroot = Environ("GPG_DOCS")
    
    ArtDocDir = DOCSroot + "\art\"
    
    If Dir(ArtDocDir + "\*.xls") = "" Then
        ' ask them to find the docs first time they load
        MsgBox "Using the Art Docs on the SHADOW", , "ArtKeep"
        ArtDocDir = "\\packmule\vss_shadow\Docs\Art\"
    End If
        
    BlastForm.Height = SmallDialogHeight
    BlastForm.Show (vbModeless)
       
End Sub


