Attribute VB_Name = "modExcel"

'//Uncomment this block before dist and comment out the designtime instantiation
'//Global oExcel As Object
'//Private Sub CreateExcelObjectEx()
'//    oExcel = CreateObject("Excel.application.10")
'//End Sub

'global excel object allows you to pass the excel DOM around
Global oExcel As Excel.Application
Global oCurrentWorkbook As Workbook
Global oCurrentWorkSheet As Worksheet
Global oCurrentChart As Chart


'Sub simply instantiates excel if excel is installed
Public Sub CreateExcelObject()
    Set oExcel = New Excel.Application
End Sub

Public Sub CreateWorkbook(sWorkBookFileTitle As String)
    Set oCurrentWorkbook = oExcel.Workbooks.Add

End Sub
Public Sub OpenExisitingWorkBook(sWorkBookFileAndPath As String)
    Set oCurrentWorkbook = oExcel.Workbooks.Open(sWorkBookFileAndPath)
    oExcel.Visible = True
End Sub
Public Sub CreateWorkSheet(sWorksheetName As String)
    Set oCurrentWorkSheet = oCurrentWorkbook.Worksheets.Add
    oCurrentWorkSheet.Name = sWorksheetName
    oCurrentWorkSheet.Activate
    'oExcel.Visible = True
End Sub


Public Sub KillExcell()
 oCurrentWorkbook.Close False, , False
'oExcel.ActiveWindow.Close False
oExcel.Quit
Set oExcel = Nothing
End Sub

Public Sub ImportLog(sCurrentWorksheet As String, sCurrentXLSName As String, TestRunIndex As Integer)
    Dim fso As New FileSystemObject
    Dim Fpslog As File
    Dim tsTextStream As TextStream
    Dim sExcelLine As String
    Dim iindex As Integer
    Set oExcel = New Excel.Application
    Dim iCellCount As Integer
    
    If TestRunIndex = 1 Then
        Set oCurrentWorkbook = oExcel.Workbooks.Add
            oCurrentWorkbook.Activate
            oCurrentWorkbook.SaveAs sCurrentXLSName
    Else
        Set oCurrentWorkbook = oExcel.Workbooks.Open(sCurrentXLSName)
    End If
    
    Set oCurrentWorkSheet = oCurrentWorkbook.Worksheets.Add
    
    
    oCurrentWorkSheet.Name = sCurrentWorksheet
    
            

    delay 1
    iindex = 1

    oExcel.Worksheets(sCurrentWorksheet).Activate
    
    
    
    Set Fpslog = fso.GetFile(CurrentUserInfo.ExePath & "\fps.log")
          Set tsTextStream = Fpslog.OpenAsTextStream(ForReading)
    Do While Not tsTextStream.AtEndOfStream
        
        sExcelLine = tsTextStream.ReadLine
        oCurrentWorkSheet.Cells(iindex, 1).Value = sExcelLine
        Debug.Print sExcelLine
        iindex = iindex + 1
    
    Loop
    oCurrentWorkSheet.Range("A20", "A" & CStr(iindex - 1)).Select
    Set oCurrentChart = oExcel.Charts.Add
    With oCurrentChart
        .Activate
        .Name = sCurrentWorksheet & " Chart"

        .ChartType = xlArea
        'Range("A20", "A" & CStr(iindex + 20)).Select
        End With
    tsTextStream.Close
    Fpslog.Delete True
    Set fso = Nothing
 '   oExcel.Visible = True

    oCurrentWorkbook.Save
    oExcel.Quit
    Set oExcel = Nothing


    
End Sub


