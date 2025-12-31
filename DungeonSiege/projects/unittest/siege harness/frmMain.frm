VERSION 5.00
Object = "{1B773E42-2509-11CF-942F-008029004347}#3.3#0"; "sysmon.ocx"
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Object = "{3B7C8863-D78F-101B-B9B5-04021C009402}#1.2#0"; "RICHTX32.OCX"
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "comdlg32.ocx"
Object = "{86CF1D34-0C5F-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCT2.OCX"
Begin VB.Form frmMain 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Siege Harness"
   ClientHeight    =   9810
   ClientLeft      =   150
   ClientTop       =   750
   ClientWidth     =   13410
   Icon            =   "frmMain.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   9810
   ScaleWidth      =   13410
   StartUpPosition =   3  'Windows Default
   Begin VB.Frame frameCaseMain 
      BorderStyle     =   0  'None
      Height          =   2535
      Index           =   3
      Left            =   120
      TabIndex        =   68
      Top             =   360
      Width           =   5625
      Begin VB.Frame frameOptions 
         Caption         =   "General PContent Analysis"
         Height          =   2295
         Left            =   0
         TabIndex        =   72
         Top             =   120
         Width           =   5535
         Begin VB.ComboBox cmbPcontentCritter 
            Enabled         =   0   'False
            Height          =   315
            Left            =   2160
            Style           =   2  'Dropdown List
            TabIndex        =   125
            Top             =   1320
            Width           =   3135
         End
         Begin VB.OptionButton OptUseMonsters 
            Caption         =   "Use Monsters of Type"
            Height          =   255
            Left            =   120
            TabIndex        =   124
            Top             =   1320
            Width           =   2055
         End
         Begin VB.OptionButton OptQuery 
            Caption         =   "Use Standard Query"
            Height          =   255
            Left            =   120
            TabIndex        =   123
            Top             =   840
            Value           =   -1  'True
            Width           =   1815
         End
         Begin VB.CheckBox chkExcel 
            Caption         =   "Open Spreadsheet upon completion"
            Height          =   495
            Left            =   120
            TabIndex        =   103
            Top             =   240
            Value           =   1  'Checked
            Width           =   2175
         End
         Begin VB.CommandButton Command1 
            Caption         =   "&Go"
            Height          =   375
            Left            =   4440
            TabIndex        =   93
            Top             =   1800
            Width           =   855
         End
         Begin VB.TextBox txtIterations 
            BeginProperty DataFormat 
               Type            =   1
               Format          =   "0"
               HaveTrueFalseNull=   0
               FirstDayOfWeek  =   0
               FirstWeekOfYear =   0
               LCID            =   1033
               SubFormatType   =   1
            EndProperty
            Height          =   375
            Left            =   3000
            TabIndex        =   91
            Text            =   "1"
            Top             =   1800
            Width           =   1095
         End
         Begin MSComCtl2.UpDown updnIterations 
            Height          =   375
            Left            =   4080
            TabIndex        =   92
            Top             =   1800
            Width           =   240
            _ExtentX        =   423
            _ExtentY        =   661
            _Version        =   393216
            Value           =   1
            AutoBuddy       =   -1  'True
            BuddyControl    =   "txtIterations"
            BuddyDispid     =   196657
            OrigLeft        =   2040
            OrigTop         =   1200
            OrigRight       =   2280
            OrigBottom      =   1575
            Max             =   10000
            Min             =   1
            SyncBuddy       =   -1  'True
            BuddyProperty   =   0
            Enabled         =   -1  'True
         End
         Begin VB.CheckBox chkLogs 
            Caption         =   "Do not save logs, I will save them from Excel.."
            Height          =   495
            Left            =   2400
            TabIndex        =   75
            Top             =   240
            Value           =   1  'Checked
            Width           =   2775
         End
         Begin VB.TextBox txtPcontentQuery 
            Height          =   315
            Left            =   2160
            TabIndex        =   73
            Top             =   840
            Width           =   3135
         End
         Begin VB.Label Label26 
            Caption         =   "Iterations"
            Height          =   375
            Left            =   2280
            TabIndex        =   90
            Top             =   1800
            Width           =   735
         End
      End
   End
   Begin VB.Frame frameCaseMain 
      BorderStyle     =   0  'None
      Height          =   6855
      Index           =   5
      Left            =   120
      TabIndex        =   70
      Top             =   360
      Width           =   6135
      Begin SystemMonitorCtl.SystemMonitor SystemMonitor1 
         Height          =   5895
         Left            =   120
         TabIndex        =   71
         Top             =   240
         Width           =   5775
         _Version        =   196611
         _ExtentX        =   10186
         _ExtentY        =   10398
         DisplayType     =   1
         ReportValueType =   0
         MaximumScale    =   100
         MinimumScale    =   0
         ShowLegend      =   -1  'True
         ShowToolbar     =   -1  'True
         ShowScaleLabels =   -1  'True
         ShowHorizontalGrid=   0   'False
         ShowVerticalGrid=   0   'False
         ShowValueBar    =   -1  'True
         ManualUpdate    =   0   'False
         Highlight       =   0   'False
         ReadOnly        =   0   'False
         MonitorDuplicateInstances=   -1  'True
         UpdateInterval  =   1
         BackColorCtl    =   -2147483633
         ForeColor       =   -1
         BackColor       =   -1
         GridColor       =   8421504
         TimeBarColor    =   255
         Appearance      =   -1
         BorderStyle     =   0
         GraphTitle      =   ""
         YAxisLabel      =   ""
         LogFileName     =   ""
         AmbientFont     =   -1  'True
         LegendColumnWidths=   "-11	-11.75	-14.25	-15.25	-12.75	-12.75	-16.25"
         LegendSortDirection=   56249512
         LegendSortColumn=   0
         CounterCount    =   0
         MaximumSamples  =   100
         SampleCount     =   0
      End
   End
   Begin VB.Frame frameCaseMain 
      BorderStyle     =   0  'None
      Height          =   3255
      Index           =   6
      Left            =   0
      TabIndex        =   107
      Top             =   360
      Width           =   6585
      Begin VB.CheckBox chkFPSExcel 
         Caption         =   "Open Excel after all samples done"
         Height          =   255
         Left            =   360
         TabIndex        =   118
         Top             =   2760
         Value           =   1  'Checked
         Width           =   2895
      End
      Begin VB.TextBox txtFPStime 
         Height          =   375
         Left            =   3480
         TabIndex        =   114
         Text            =   "60"
         Top             =   2760
         Width           =   495
      End
      Begin VB.CommandButton cmdGoFPS 
         Caption         =   "Go"
         Height          =   375
         Left            =   5640
         TabIndex        =   113
         Top             =   2760
         Width           =   855
      End
      Begin VB.Frame Frame3 
         Caption         =   "Unit Test Map"
         Height          =   2535
         Left            =   3240
         TabIndex        =   109
         Top             =   120
         Width           =   3255
      End
      Begin VB.Frame Frame2 
         Caption         =   "Kingdom of Ehb Test Regions"
         Height          =   2535
         Left            =   120
         TabIndex        =   108
         Top             =   120
         Width           =   3015
         Begin VB.CheckBox chkCustom 
            Caption         =   "Custom Teleport"
            Height          =   495
            Left            =   120
            TabIndex        =   117
            Top             =   1800
            Width           =   1455
         End
         Begin VB.TextBox txtCustom 
            Enabled         =   0   'False
            Height          =   375
            Left            =   1680
            TabIndex        =   116
            Top             =   1920
            Width           =   1215
         End
         Begin VB.CheckBox chkGom 
            Caption         =   "Gom's Lair Region 2 (gom2_b)"
            Height          =   375
            Left            =   120
            TabIndex        =   112
            Top             =   1080
            Width           =   2655
         End
         Begin VB.CheckBox chkIceCaves 
            Caption         =   "Ice Caves Region 5 (ic_r5)"
            Height          =   255
            Left            =   120
            TabIndex        =   111
            Top             =   720
            Width           =   2655
         End
         Begin VB.CheckBox chkGoblinInventor 
            Caption         =   "Goblin Inventor Region 4 (gi_r4)"
            Height          =   255
            Left            =   120
            TabIndex        =   110
            Top             =   360
            Width           =   2655
         End
      End
      Begin VB.Label Label21 
         Caption         =   "Sample time (seconds)"
         Height          =   255
         Left            =   3960
         TabIndex        =   115
         Top             =   2760
         Width           =   1695
      End
   End
   Begin VB.Frame frameCaseMain 
      BorderStyle     =   0  'None
      Height          =   7320
      Index           =   7
      Left            =   120
      TabIndex        =   0
      Top             =   360
      Width           =   8900
      Begin VB.CommandButton cmdWavRun 
         Caption         =   "&Run"
         Height          =   405
         Left            =   3930
         TabIndex        =   1
         ToolTipText     =   "examine WAV files for extraneous data."
         Top             =   6840
         Width           =   840
      End
      Begin VB.CommandButton cmdErrExport 
         Caption         =   "&Export"
         Default         =   -1  'True
         Enabled         =   0   'False
         Height          =   405
         Left            =   4850
         TabIndex        =   119
         ToolTipText     =   "export any errors encountered to a CSV file."
         Top             =   6840
         Width           =   840
      End
      Begin MSComctlLib.ListView ListViewWav 
         Height          =   6600
         Left            =   120
         TabIndex        =   105
         Top             =   135
         Width           =   8475
         _ExtentX        =   14949
         _ExtentY        =   11642
         View            =   3
         LabelEdit       =   1
         LabelWrap       =   -1  'True
         HideSelection   =   0   'False
         _Version        =   393217
         ForeColor       =   -2147483640
         BackColor       =   -2147483643
         BorderStyle     =   1
         Appearance      =   1
         NumItems        =   2
         BeginProperty ColumnHeader(1) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
            Key             =   "a"
            Object.Tag             =   "a"
            Text            =   "WAV"
            Object.Width           =   5256
         EndProperty
         BeginProperty ColumnHeader(2) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
            SubItemIndex    =   1
            Key             =   "b"
            Object.Tag             =   "b"
            Text            =   "Status"
            Object.Width           =   9189
         EndProperty
      End
   End
   Begin VB.Frame frameCaseMain 
      BorderStyle     =   0  'None
      Height          =   7935
      Index           =   2
      Left            =   120
      TabIndex        =   37
      Top             =   360
      Width           =   5535
      Begin VB.CommandButton cmdCancelSettings 
         Caption         =   "&Cancel"
         Enabled         =   0   'False
         Height          =   375
         Left            =   3360
         TabIndex        =   78
         Top             =   7200
         Width           =   855
      End
      Begin VB.Frame Frame1 
         Caption         =   "Siege Harness Info"
         Height          =   1935
         Left            =   0
         TabIndex        =   62
         Top             =   5160
         Width           =   5175
         Begin VB.Label lblinfo 
            Alignment       =   2  'Center
            Caption         =   "Siege Harness V 1.3"
            BeginProperty Font 
               Name            =   "MS Sans Serif"
               Size            =   9.75
               Charset         =   0
               Weight          =   700
               Underline       =   0   'False
               Italic          =   0   'False
               Strikethrough   =   0   'False
            EndProperty
            Height          =   255
            Left            =   960
            TabIndex        =   66
            Top             =   240
            Width           =   2775
         End
         Begin VB.Label Label14 
            Alignment       =   2  'Center
            Caption         =   "Bryce Jones, log parser by Brian Warris"
            Height          =   255
            Left            =   600
            TabIndex        =   65
            Top             =   600
            Width           =   3615
         End
         Begin VB.Label Label15 
            Alignment       =   2  'Center
            Caption         =   "email a-brycej@microsoft.com, bjones@gaspowered.com"
            Height          =   255
            Left            =   120
            TabIndex        =   64
            Top             =   840
            Width           =   4695
         End
         Begin VB.Label Label16 
            Alignment       =   2  'Center
            Caption         =   "©2001 Microsoft Corporation"
            Height          =   255
            Left            =   720
            TabIndex        =   63
            Top             =   1080
            Width           =   3375
         End
      End
      Begin VB.CommandButton cmdApply 
         Caption         =   "&Apply"
         Enabled         =   0   'False
         Height          =   370
         Left            =   4320
         TabIndex        =   58
         Top             =   7200
         Width           =   850
      End
      Begin VB.Frame frameUser 
         Caption         =   "User Settings"
         Height          =   1695
         Left            =   0
         TabIndex        =   51
         ToolTipText     =   " User settings: Do not modify unless they are incorrect"
         Top             =   0
         Width           =   5175
         Begin VB.Label lblCurrentBuild 
            Height          =   255
            Left            =   1440
            TabIndex        =   106
            Top             =   1320
            Width           =   3495
         End
         Begin VB.Label Label20 
            Caption         =   "Current DS build:"
            Height          =   255
            Left            =   120
            TabIndex        =   120
            Top             =   1320
            Width           =   1335
         End
         Begin VB.Label lblUsertxt 
            Caption         =   "User name:"
            Height          =   255
            Left            =   120
            TabIndex        =   57
            Top             =   240
            Width           =   855
         End
         Begin VB.Label lblMachinetxt 
            Caption         =   "Machine name:"
            Height          =   255
            Left            =   120
            TabIndex        =   56
            Top             =   480
            Width           =   1095
         End
         Begin VB.Label lblUserName 
            Height          =   255
            Left            =   1080
            TabIndex        =   55
            Top             =   240
            Width           =   2655
         End
         Begin VB.Label lblMachineName 
            Height          =   255
            Left            =   1320
            TabIndex        =   54
            Top             =   480
            Width           =   2775
         End
         Begin VB.Label lblPlat 
            Caption         =   "Platform:"
            Height          =   255
            Left            =   120
            TabIndex        =   53
            Top             =   720
            Width           =   735
         End
         Begin VB.Label lblPlatform 
            Height          =   495
            Left            =   840
            TabIndex        =   52
            Top             =   720
            Width           =   4005
         End
      End
      Begin VB.Frame frameGameSettings 
         Caption         =   "Dungeon Siege Settings"
         Height          =   1690
         Left            =   0
         TabIndex        =   41
         ToolTipText     =   "Set your Dungeon Siege settings here"
         Top             =   1800
         Width           =   5175
         Begin VB.OptionButton optRelease 
            Caption         =   "Release"
            Height          =   250
            Left            =   120
            TabIndex        =   48
            Top             =   240
            Width           =   970
         End
         Begin VB.OptionButton optDebug 
            Caption         =   "Debug"
            Height          =   250
            Left            =   1200
            TabIndex        =   47
            Top             =   240
            Value           =   -1  'True
            Width           =   850
         End
         Begin VB.TextBox txtExepath 
            Enabled         =   0   'False
            Height          =   370
            Left            =   1440
            TabIndex        =   46
            Top             =   720
            Width           =   2655
         End
         Begin VB.CommandButton cmdExeBrowse 
            Caption         =   "..."
            Height          =   370
            Left            =   4200
            TabIndex        =   45
            Top             =   720
            Width           =   850
         End
         Begin VB.CommandButton Command2 
            Caption         =   "Modify"
            Enabled         =   0   'False
            Height          =   370
            Left            =   4200
            TabIndex        =   44
            Top             =   1200
            Width           =   850
         End
         Begin VB.TextBox txtSkritPath 
            Enabled         =   0   'False
            Height          =   370
            Left            =   1440
            TabIndex        =   43
            Text            =   "auto\test"
            Top             =   1200
            Width           =   2655
         End
         Begin VB.CheckBox chkWIndowed 
            Caption         =   "Fullscreen"
            Height          =   250
            Left            =   2400
            TabIndex        =   42
            Top             =   240
            Width           =   1090
         End
         Begin VB.Label Label13 
            Caption         =   "Executable Path:"
            Height          =   250
            Left            =   120
            TabIndex        =   50
            Top             =   720
            Width           =   1330
         End
         Begin VB.Label Label12 
            Caption         =   "Skrit Path:"
            Height          =   250
            Left            =   120
            TabIndex        =   49
            Top             =   1200
            Width           =   1210
         End
      End
      Begin VB.Frame frameGeneral 
         Caption         =   "General Settings"
         Height          =   1450
         Left            =   0
         TabIndex        =   38
         Top             =   3600
         Width           =   5175
         Begin VB.CommandButton cmdSQLserver 
            Caption         =   "..."
            Height          =   370
            Left            =   4200
            TabIndex        =   77
            Top             =   600
            Width           =   855
         End
         Begin VB.TextBox txtSQLserver 
            Enabled         =   0   'False
            Height          =   370
            Left            =   1080
            TabIndex        =   76
            Top             =   600
            Width           =   3015
         End
         Begin VB.CheckBox chkHarnessMin 
            Caption         =   "Minimize Harness while suite is running"
            Height          =   250
            Left            =   120
            TabIndex        =   39
            Top             =   240
            Value           =   1  'Checked
            Width           =   3370
         End
         Begin VB.Label Label11 
            Caption         =   "SQL server:"
            Height          =   255
            Left            =   120
            TabIndex        =   40
            Top             =   600
            Width           =   975
         End
      End
   End
   Begin VB.Frame frameCaseMain 
      BorderStyle     =   0  'None
      Height          =   9375
      Index           =   4
      Left            =   120
      TabIndex        =   69
      Top             =   360
      Width           =   12898
      Begin MSComctlLib.ListView lvSpells 
         Height          =   1785
         Left            =   7305
         TabIndex        =   97
         ToolTipText     =   "Click to view the current parameters for the spells under test."
         Top             =   2640
         Visible         =   0   'False
         Width           =   2400
         _ExtentX        =   4233
         _ExtentY        =   3149
         View            =   3
         LabelEdit       =   1
         LabelWrap       =   -1  'True
         HideSelection   =   0   'False
         _Version        =   393217
         ForeColor       =   -2147483640
         BackColor       =   -2147483643
         BorderStyle     =   1
         Appearance      =   1
         NumItems        =   0
      End
      Begin VB.Frame frame 
         Caption         =   "Run"
         Height          =   1575
         Left            =   0
         TabIndex        =   82
         Top             =   120
         Width           =   7575
         Begin VB.CommandButton cmdExport 
            Caption         =   "&Export"
            Enabled         =   0   'False
            Height          =   375
            Left            =   6465
            TabIndex        =   104
            ToolTipText     =   "Export Spell Log Summary to CSV (comma separated variable) file ..."
            Top             =   630
            Width           =   975
         End
         Begin VB.CheckBox chkContinueSpellRun 
            Caption         =   "&Continue from last test Spell Run"
            Enabled         =   0   'False
            Height          =   225
            Left            =   120
            TabIndex        =   100
            ToolTipText     =   "Running all spells against all monsters takes ~ 32 hours - choose this option to continue from the last selected run."
            Top             =   840
            Width           =   2685
         End
         Begin VB.CommandButton cmdPause 
            Caption         =   "&Pause"
            Enabled         =   0   'False
            Height          =   375
            Left            =   3345
            TabIndex        =   99
            ToolTipText     =   "Pause the current  test."
            Top             =   1080
            Width           =   975
         End
         Begin VB.CommandButton cmdAbort 
            Caption         =   "&Abort"
            Enabled         =   0   'False
            Height          =   375
            Left            =   4390
            TabIndex        =   98
            ToolTipText     =   "View the current parameters for the spells under test."
            Top             =   1080
            Width           =   975
         End
         Begin VB.ComboBox cmbMonsterList 
            Height          =   315
            ItemData        =   "frmMain.frx":0BC2
            Left            =   3720
            List            =   "frmMain.frx":0BC4
            Sorted          =   -1  'True
            Style           =   2  'Dropdown List
            TabIndex        =   87
            Top             =   240
            Width           =   3735
         End
         Begin VB.ComboBox cmbSpellList 
            Height          =   315
            Left            =   120
            Sorted          =   -1  'True
            Style           =   2  'Dropdown List
            TabIndex        =   86
            Top             =   240
            Width           =   3135
         End
         Begin VB.CommandButton cmdViewSpells 
            Caption         =   "&View Spells"
            Enabled         =   0   'False
            Height          =   375
            Left            =   5435
            TabIndex        =   85
            ToolTipText     =   "View the current parameters for the spells under test."
            Top             =   1080
            Width           =   975
         End
         Begin VB.CommandButton cmdSpellsRun 
            Caption         =   "&Go!"
            Enabled         =   0   'False
            Height          =   375
            Left            =   6480
            TabIndex        =   84
            ToolTipText     =   "Run the spell vs monster test, as selected."
            Top             =   1080
            Width           =   975
         End
         Begin VB.CheckBox chkSpellLog 
            Caption         =   "Run as repro case (do not log to the DB)"
            Height          =   420
            Left            =   120
            TabIndex        =   83
            ToolTipText     =   "Disable the logging functionality; just run the test."
            Top             =   1095
            Value           =   1  'Checked
            Width           =   1815
         End
         Begin VB.Label txtSpellPos 
            Height          =   285
            Left            =   120
            TabIndex        =   95
            Top             =   615
            Width           =   3135
         End
         Begin VB.Label txtMonsterPos 
            Height          =   285
            Left            =   3720
            TabIndex        =   94
            Top             =   615
            Width           =   3720
         End
         Begin VB.Label Label24 
            Caption         =   "Vs."
            Height          =   255
            Left            =   3360
            TabIndex        =   88
            Top             =   240
            Width           =   255
         End
      End
      Begin VB.Frame frameExisiting 
         Caption         =   "View Existing"
         Height          =   1575
         Left            =   7800
         TabIndex        =   80
         Top             =   120
         Width           =   4935
         Begin VB.HScrollBar HScrollSpellTest 
            Height          =   345
            Left            =   776
            TabIndex        =   74
            Top             =   1095
            Width           =   2900
         End
         Begin VB.CommandButton cmdSpellRslt 
            Height          =   345
            Index           =   1
            Left            =   448
            Picture         =   "frmMain.frx":0BC6
            Style           =   1  'Graphical
            TabIndex        =   102
            ToolTipText     =   "See individual result of spell versus monster tests"
            Top             =   1095
            Width           =   285
         End
         Begin VB.CommandButton cmdSpellRslt 
            Height          =   345
            Index           =   0
            Left            =   120
            Picture         =   "frmMain.frx":0D10
            Style           =   1  'Graphical
            TabIndex        =   101
            ToolTipText     =   "See summary of spell versus monster tests."
            Top             =   1095
            Width           =   285
         End
         Begin MSComctlLib.ListView lstvBuildDTMachine 
            Height          =   795
            Left            =   90
            TabIndex        =   96
            ToolTipText     =   "Choose an already entered combination of build, date, time and computer under test."
            Top             =   240
            Width           =   4755
            _ExtentX        =   8387
            _ExtentY        =   1402
            View            =   3
            LabelEdit       =   1
            LabelWrap       =   -1  'True
            HideSelection   =   0   'False
            GridLines       =   -1  'True
            _Version        =   393217
            ForeColor       =   -2147483640
            BackColor       =   -2147483643
            BorderStyle     =   1
            Appearance      =   1
            NumItems        =   3
            BeginProperty ColumnHeader(1) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
               Key             =   "a"
               Object.Tag             =   "a"
               Text            =   "Build Number"
               Object.Width           =   1977
            EndProperty
            BeginProperty ColumnHeader(2) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
               SubItemIndex    =   1
               Key             =   "b"
               Object.Tag             =   "b"
               Text            =   "Date / Time"
               Object.Width           =   3404
            EndProperty
            BeginProperty ColumnHeader(3) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
               SubItemIndex    =   2
               Key             =   "c"
               Object.Tag             =   "c"
               Text            =   "Machine"
               Object.Width           =   2816
            EndProperty
         End
         Begin VB.CommandButton cmdRetrieve 
            Caption         =   "&Retrieve"
            Height          =   375
            Left            =   3720
            TabIndex        =   81
            ToolTipText     =   "Get the current list of tests ran, from the SQL Server table."
            Top             =   1080
            Width           =   975
         End
      End
      Begin RichTextLib.RichTextBox RichTextBox1 
         Height          =   7215
         Left            =   0
         TabIndex        =   79
         Top             =   2040
         Width           =   12735
         _ExtentX        =   22463
         _ExtentY        =   12726
         _Version        =   393217
         BackColor       =   12632256
         Enabled         =   -1  'True
         ReadOnly        =   -1  'True
         ScrollBars      =   3
         RightMargin     =   1.78184e5
         TextRTF         =   $"frmMain.frx":1152
         BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
            Name            =   "Courier New"
            Size            =   8.25
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
      End
      Begin VB.Label Label25 
         Caption         =   "Results"
         Height          =   255
         Left            =   0
         TabIndex        =   89
         Top             =   1800
         Width           =   1695
      End
   End
   Begin VB.Frame frameCaseMain 
      BorderStyle     =   0  'None
      Height          =   9375
      Index           =   1
      Left            =   120
      TabIndex        =   34
      Top             =   360
      Width           =   10815
      Begin VB.CommandButton cmdClearLstView 
         Caption         =   "&Clear"
         Height          =   375
         Left            =   2160
         TabIndex        =   67
         Top             =   8880
         Width           =   855
      End
      Begin VB.CommandButton cmdExit2 
         Caption         =   "E&xit"
         Height          =   375
         Left            =   9840
         TabIndex        =   61
         Top             =   8880
         Width           =   855
      End
      Begin RichTextLib.RichTextBox txtResult 
         Height          =   8295
         Left            =   3120
         TabIndex        =   35
         Top             =   480
         Width           =   7575
         _ExtentX        =   13361
         _ExtentY        =   14631
         _Version        =   393217
         BackColor       =   12632256
         Enabled         =   -1  'True
         ReadOnly        =   -1  'True
         ScrollBars      =   3
         DisableNoScroll =   -1  'True
         RightMargin     =   22000
         TextRTF         =   $"frmMain.frx":11D2
      End
      Begin MSComctlLib.ListView lstLogCase 
         Height          =   8295
         Left            =   240
         TabIndex        =   36
         Top             =   480
         Width           =   2880
         _ExtentX        =   5080
         _ExtentY        =   14631
         View            =   3
         LabelEdit       =   1
         LabelWrap       =   -1  'True
         HideSelection   =   0   'False
         _Version        =   393217
         Icons           =   "ImageList"
         SmallIcons      =   "ImageList"
         ForeColor       =   -2147483640
         BackColor       =   -2147483643
         BorderStyle     =   1
         Appearance      =   1
         NumItems        =   2
         BeginProperty ColumnHeader(1) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
            Text            =   "Case Name"
            Object.Width           =   4233
         EndProperty
         BeginProperty ColumnHeader(2) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
            SubItemIndex    =   1
            Text            =   "Time"
            Object.Width           =   4233
         EndProperty
      End
      Begin VB.Label Label18 
         Caption         =   "Results:"
         Height          =   255
         Left            =   7920
         TabIndex        =   60
         Top             =   240
         Width           =   1815
      End
      Begin VB.Label Label17 
         Caption         =   "Ran Cases Queue"
         Height          =   255
         Left            =   240
         TabIndex        =   59
         Top             =   240
         Width           =   2175
      End
   End
   Begin VB.Frame frameCaseMain 
      BorderStyle     =   0  'None
      Height          =   9375
      Index           =   0
      Left            =   120
      TabIndex        =   121
      Top             =   360
      Width           =   9975
      Begin VB.Timer Timer1 
         Enabled         =   0   'False
         Interval        =   1000
         Left            =   0
         Top             =   8880
      End
      Begin VB.Frame frameCaseinfo 
         Caption         =   "Case Information"
         Height          =   8985
         Left            =   4080
         TabIndex        =   10
         Top             =   120
         Width           =   5775
         Begin VB.TextBox txtMap 
            Enabled         =   0   'False
            Height          =   300
            Left            =   840
            TabIndex        =   22
            Top             =   960
            Width           =   4815
         End
         Begin VB.TextBox txtTeleport 
            Enabled         =   0   'False
            Height          =   300
            Left            =   840
            TabIndex        =   21
            Top             =   1320
            Width           =   4815
         End
         Begin VB.TextBox txtArgs 
            Enabled         =   0   'False
            Height          =   300
            Left            =   840
            TabIndex        =   20
            Top             =   1680
            Width           =   4815
         End
         Begin VB.TextBox txtSkrit 
            Enabled         =   0   'False
            Height          =   300
            Left            =   840
            TabIndex        =   19
            Top             =   2040
            Width           =   4335
         End
         Begin VB.CommandButton cmdEdit 
            Caption         =   "&Edit"
            Height          =   370
            Left            =   1920
            TabIndex        =   18
            Top             =   8520
            Width           =   850
         End
         Begin VB.CommandButton cmdSave 
            Caption         =   "S&ave"
            Enabled         =   0   'False
            Height          =   370
            Left            =   2880
            TabIndex        =   17
            Top             =   8520
            Width           =   850
         End
         Begin VB.CommandButton cmdNew 
            Caption         =   "&New"
            Height          =   370
            Left            =   3840
            TabIndex        =   16
            Top             =   8520
            Width           =   850
         End
         Begin VB.CommandButton cmdExit 
            Caption         =   "E&xit"
            Height          =   370
            Left            =   4800
            TabIndex        =   15
            Top             =   8520
            Width           =   850
         End
         Begin VB.CommandButton cmdBrowse 
            Caption         =   "..."
            Enabled         =   0   'False
            Height          =   300
            Left            =   5280
            TabIndex        =   14
            Top             =   2040
            Width           =   370
         End
         Begin VB.ComboBox cmbArea 
            Enabled         =   0   'False
            Height          =   315
            ItemData        =   "frmMain.frx":1273
            Left            =   840
            List            =   "frmMain.frx":1275
            TabIndex        =   13
            Text            =   "cmbArea"
            Top             =   2400
            Width           =   4815
         End
         Begin VB.TextBox txtCaseName 
            Enabled         =   0   'False
            Height          =   300
            Left            =   840
            TabIndex        =   12
            Top             =   240
            Width           =   4815
         End
         Begin VB.TextBox txtAuthor 
            Enabled         =   0   'False
            Height          =   300
            Left            =   840
            TabIndex        =   11
            Top             =   600
            Width           =   4815
         End
         Begin RichTextLib.RichTextBox rtfDescription 
            Height          =   2535
            Left            =   120
            TabIndex        =   23
            Top             =   3120
            Width           =   5535
            _ExtentX        =   9763
            _ExtentY        =   4471
            _Version        =   393217
            Enabled         =   0   'False
            ScrollBars      =   3
            TextRTF         =   $"frmMain.frx":1277
         End
         Begin RichTextLib.RichTextBox rtfExpected 
            Height          =   2535
            Left            =   120
            TabIndex        =   24
            Top             =   5880
            Width           =   5535
            _ExtentX        =   9763
            _ExtentY        =   4471
            _Version        =   393217
            Enabled         =   0   'False
            ScrollBars      =   3
            TextRTF         =   $"frmMain.frx":12F9
         End
         Begin VB.Label Label1 
            Caption         =   "Name"
            Height          =   255
            Left            =   120
            TabIndex        =   33
            Top             =   240
            Width           =   495
         End
         Begin VB.Label Label2 
            Caption         =   "Author"
            Height          =   255
            Left            =   120
            TabIndex        =   32
            Top             =   600
            Width           =   495
         End
         Begin VB.Label Label3 
            Caption         =   "Map"
            Height          =   255
            Left            =   120
            TabIndex        =   31
            Top             =   960
            Width           =   495
         End
         Begin VB.Label Label4 
            Caption         =   "Teleport"
            Height          =   255
            Left            =   120
            TabIndex        =   30
            Top             =   1320
            Width           =   615
         End
         Begin VB.Label Label5 
            Caption         =   "Args"
            Height          =   255
            Left            =   120
            TabIndex        =   29
            Top             =   1680
            Width           =   615
         End
         Begin VB.Label Label6 
            Caption         =   "Skrit"
            Height          =   255
            Left            =   120
            TabIndex        =   28
            Top             =   2040
            Width           =   615
         End
         Begin VB.Label Label7 
            Caption         =   "Expected"
            Height          =   255
            Left            =   120
            TabIndex        =   27
            Top             =   5640
            Width           =   855
         End
         Begin VB.Label Label8 
            Caption         =   "Description"
            Height          =   255
            Left            =   120
            TabIndex        =   26
            Top             =   2880
            Width           =   975
         End
         Begin VB.Label Label10 
            Caption         =   "Area"
            Height          =   255
            Left            =   120
            TabIndex        =   25
            Top             =   2400
            Width           =   615
         End
      End
      Begin VB.Frame frameCases 
         Caption         =   "Cases"
         Height          =   8985
         Left            =   120
         TabIndex        =   2
         Top             =   120
         Width           =   3975
         Begin VB.CommandButton cmdAdd 
            Caption         =   "A&dd"
            Height          =   370
            Left            =   120
            TabIndex        =   7
            Top             =   8520
            Width           =   850
         End
         Begin VB.CommandButton cmdRemove 
            Caption         =   "Remo&ve"
            Height          =   370
            Left            =   1080
            TabIndex        =   6
            Top             =   8520
            Width           =   850
         End
         Begin VB.CommandButton cmdClear 
            Caption         =   "&Clear"
            Height          =   370
            Left            =   2040
            TabIndex        =   5
            Top             =   8520
            Width           =   850
         End
         Begin VB.CommandButton cmdGo 
            Caption         =   "&Go!"
            Height          =   370
            Left            =   3000
            TabIndex        =   4
            Top             =   8520
            Width           =   850
         End
         Begin VB.ListBox lstCases 
            Height          =   4545
            Left            =   120
            TabIndex        =   3
            Top             =   3720
            Width           =   3735
         End
         Begin MSComctlLib.TreeView tvCases 
            Height          =   3015
            Left            =   120
            TabIndex        =   8
            Top             =   360
            Width           =   3735
            _ExtentX        =   6588
            _ExtentY        =   5318
            _Version        =   393217
            Indentation     =   353
            LabelEdit       =   1
            Style           =   7
            Appearance      =   1
            MouseIcon       =   "frmMain.frx":137B
         End
         Begin VB.Label Label9 
            Caption         =   "Test Case Queue"
            Height          =   255
            Left            =   360
            TabIndex        =   9
            Top             =   3480
            Width           =   1935
         End
      End
   End
   Begin MSComDlg.CommonDialog CommonDialog 
      Left            =   5160
      Top             =   7560
      _ExtentX        =   556
      _ExtentY        =   556
      _Version        =   393216
   End
   Begin MSComctlLib.TabStrip TabStrip1 
      Height          =   9975
      Left            =   0
      TabIndex        =   122
      Top             =   0
      Width           =   13455
      _ExtentX        =   23733
      _ExtentY        =   17595
      MultiRow        =   -1  'True
      HotTracking     =   -1  'True
      TabMinWidth     =   2
      _Version        =   393216
      BeginProperty Tabs {1EFB6598-857C-11D1-B16A-00C0F0283628} 
         NumTabs         =   8
         BeginProperty Tab1 {1EFB659A-857C-11D1-B16A-00C0F0283628} 
            Caption         =   "Cases"
            ImageVarType    =   2
         EndProperty
         BeginProperty Tab2 {1EFB659A-857C-11D1-B16A-00C0F0283628} 
            Caption         =   "Logs"
            ImageVarType    =   2
         EndProperty
         BeginProperty Tab3 {1EFB659A-857C-11D1-B16A-00C0F0283628} 
            Caption         =   "Settings"
            ImageVarType    =   2
         EndProperty
         BeginProperty Tab4 {1EFB659A-857C-11D1-B16A-00C0F0283628} 
            Caption         =   "Pcontent"
            ImageVarType    =   2
         EndProperty
         BeginProperty Tab5 {1EFB659A-857C-11D1-B16A-00C0F0283628} 
            Caption         =   "Spells"
            ImageVarType    =   2
         EndProperty
         BeginProperty Tab6 {1EFB659A-857C-11D1-B16A-00C0F0283628} 
            Caption         =   "Perf"
            ImageVarType    =   2
         EndProperty
         BeginProperty Tab7 {1EFB659A-857C-11D1-B16A-00C0F0283628} 
            Caption         =   "Framerate"
            ImageVarType    =   2
         EndProperty
         BeginProperty Tab8 {1EFB659A-857C-11D1-B16A-00C0F0283628} 
            Caption         =   "WAV"
            Key             =   "i"
            Object.Tag             =   "i"
            Object.ToolTipText     =   "Click here to run tests on WAV files."
            ImageVarType    =   2
         EndProperty
      EndProperty
   End
   Begin MSComctlLib.ImageList ImageList 
      Left            =   7320
      Top             =   7560
      _ExtentX        =   661
      _ExtentY        =   661
      BackColor       =   -2147483643
      ImageWidth      =   16
      ImageHeight     =   16
      MaskColor       =   12632256
      _Version        =   393216
      BeginProperty Images {2C247F25-8591-11D1-B16A-00C0F0283628} 
         NumListImages   =   6
         BeginProperty ListImage1 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":25FD
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":31CF
            Key             =   ""
         EndProperty
         BeginProperty ListImage3 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":3262
            Key             =   ""
         EndProperty
         BeginProperty ListImage4 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":35F5
            Key             =   ""
         EndProperty
         BeginProperty ListImage5 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":3A47
            Key             =   ""
         EndProperty
         BeginProperty ListImage6 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":3D99
            Key             =   ""
         EndProperty
      EndProperty
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub chkHarnessMin_Click()
    cmdApply.Enabled = True
    cmdCancelSettings.Enabled = True
End Sub

Private Sub cmbMonsterList_click()
    If cmbMonsterList.ListIndex = cmbMonsterList.ListCount - 1 Then
        txtMonsterPos.Caption = ""
        modSpells.bAllSpells = True
    Else
        txtMonsterPos.Caption = "Monster " & Format(cmbMonsterList.ListIndex + 1) & " of " & Format(cmbMonsterList.ListCount - 1)
        modSpells.bAllSpells = False
    End If
End Sub

Private Sub cmbSpellList_Click()
    If cmbSpellList.ListIndex = cmbSpellList.ListCount - 1 Then
        txtSpellPos.Caption = ""
        modSpells.bAllMonsters = True
    Else
        txtSpellPos.Caption = "Spell " & Format(cmbSpellList.ListIndex + 1) & " of " & Format(cmbSpellList.ListCount - 1)
        modSpells.bAllMonsters = False
    End If
End Sub

Private Sub cmdAbort_Click()
    modSpells.bAbort = True
    cmdAbort.Enabled = False
    cmdPause.Enabled = False
End Sub

Private Sub cmdApply_Click()
    With CurrentUserInfo
        .ExePath = txtExepath.Text
        modParse.sCurDir = txtExepath.Text
        .IsDebug = optDebug.Value
        .MachineName = lblMachineName.Caption
        .OS = lblPlatform.Caption
        .SkritPath = txtSkritPath
        .UserName = lblUserName.Caption
        .MinimizeMain = chkHarnessMin.Value

        .SQLserver = txtSQLserver.Text
        If .IsDebug = True Then
            .CurrentBuild = GetFileVersion(txtExepath.Text & "\" & DEBUG_EXE)
        Else
            .CurrentBuild = GetFileVersion(txtExepath.Text & "\" & RELEASE_EXE)
        End If
        Call modParse.WriteSQLSrvr      ' put in ini file
        modDB.ConnectionString = "driver={SQL Server};SERVER=" & CurrentUserInfo.SQLserver & ";database=unittestdb;dsn=''"
    End With
    If bFirstRun = True Then
        AddUserToDB CurrentUserInfo
    ElseIf bFirstRun = False Then
'        UpdateUserinDB CurrentUserInfo  NO GOOD !! ends up with multiple user entries !!
        DeleteUserInDB CurrentUserInfo
        AddUserToDB CurrentUserInfo
    End If
    lblCurrentBuild = CurrentUserInfo.CurrentBuild
    cmdApply.Enabled = False
    cmdCancelSettings.Enabled = False
End Sub
Private Sub UpdateUserUI(CurrentUserInfo As UserInfo)
    With CurrentUserInfo
        txtExepath.Text = .ExePath
        If .IsDebug = True Then
            optDebug.Value = True
            optRelease.Value = False
            .CurrentBuild = GetFileVersion(txtExepath.Text & "\" & DEBUG_EXE)
        Else
            optDebug.Value = False
            optRelease.Value = True
            .CurrentBuild = GetFileVersion(txtExepath.Text & "\" & RELEASE_EXE)
        End If
        
        lblMachineName.Caption = .MachineName
        lblPlatform.Caption = .OS
        lblCurrentBuild = .CurrentBuild
        If .SkritPath <> "" Then
            txtSkritPath = .SkritPath
        End If
        lblUserName.Caption = .UserName
        chkHarnessMin.Value = IIf(.MinimizeMain, 1, 0)
    End With
End Sub

Private Sub cmdCancelSettings_Click()
   txtSQLserver.Text = CurrentUserInfo.SQLserver
   txtExepath = CurrentUserInfo.ExePath
   cmdApply.Enabled = False
   cmdCancelSettings.Enabled = False
End Sub

Private Sub cmdClearLstView_Click()
    lstLogCase.ListItems.Clear
    txtResult.Text = ""
End Sub

Private Sub cmdExcelkill_Click()
KillExcell

End Sub



Private Sub cmdErrExport_Click()
    Dim lRtn&, i%
    Dim lix  As ListItem
    Dim sLine$
    
    cmdErrExport.Enabled = False
    cmdSpellRslt_Click (0)  ' ensure that this operates on summary
    On Error GoTo erh
    With CommonDialog
        .FileName = read_key("config", "CSVfileWAV", sIniFile)
        .InitDir = modParse.sCurDir
        If .FileName > "" Then
            lRtn = InStrRev(.FileName, "\")
            If lRtn > 0 Then
                .InitDir = Left$(.FileName, lRtn - 1&)
            End If
        End If
        .DialogTitle = "Enter a CSV filename for export to."
        .Filter = "CSV Files (*.CSV)|*.CSV"
        .Flags = cdlOFNCreatePrompt Or cdlOFNHideReadOnly Or cdlOFNExplorer
        .CancelError = True  ' False  // need to know when cancel button was pressed!
        .DefaultExt = "csv"
        .ShowOpen
        If (.FileName > "") Then  ' And (Dir(.FileName) > "") Then
            If (Dir(.FileName) > "") Then
                Kill .FileName  ' ensure that we start afresh on existing file
            End If
            With frmSplash
                .Show vbModeless
                .lblLoad.Caption = "Preparing CSV report"
                .lblLoad.Refresh
                .ProgressBar1.Max = modWAV.nErrors
                .ProgressBar1.Value = 0
                .ProgressBar1.Visible = True
            End With
    
            For i = 1 To ListViewWav.ListItems.Count
                DoEvents
                Set lix = ListViewWav.ListItems(i)
                If (lix.SubItems(1) <> "") Then ' for debugging state
                    If Left$(lix.SubItems(1), 4) <> "Pass" Then
                        ' we have an error to report
                        frmSplash.ProgressBar1.Value = frmSplash.ProgressBar1.Value + 1
                        sLine = lix.Text & "," & Chr$(34) & lix.SubItems(1) & Chr$(34)
                        Debug.Print sLine
                        Call store_append(.FileName, sLine)
                    End If
                End If
                Set lix = Nothing ' release memory
            Next i
            Unload frmSplash

            lRtn = WritePrivateProfileString("config", "CSVfileWAV", .FileName, sIniFile)
            Debug.Assert lRtn <> 0
        End If
    End With
erh:
    cmdErrExport.Enabled = True
    
    If Err.Number <> 0 Then
        If Err.Number <> cdlCancel Then
            MsgBox "cmdErrExport ERROR " & CStr(Err.Number) & " : " & Err.Description
        ' otherwise, error expected due to user clicking Cancel button
        End If
        Err.Clear
    End If
End Sub


Private Sub cmdExeBrowse_Click()
    Dim sExePath As String
    Dim sInitialExePath As String
        sInitialExePath = txtExepath.Text
    With CommonDialog
        .InitDir = sInitialExePath
        .DialogTitle = "Select either Release or Debug"
        If optRelease = True Then
            .Filter = "Dungeon Siege Release (DungeonSiegeR.exe)|DungeonSiegeR.exe"
        ElseIf optDebug = True Then
            .Filter = "Dungeon Siege Debug (DungeonSiegeD.exe)|DungeonSiegeD.exe"
        End If
        
        .Flags = cdlOFNFileMustExist Or cdlOFNHideReadOnly Or cdlOFNExplorer
        .ShowOpen
        
    End With
    If CommonDialog.FileName > "" Then
        sExePath = CommonDialog.FileName
        txtExepath.Text = Left$(sExePath, Len(sExePath) - 18)  ' safe as long as exe name remains DungeonSiege?.exe [17 chars, + 1 for \]

        CurrentUserInfo.ExePath = txtExepath.Text
        Call modParse.get_delims     ' new path - possible new delims...
        Call modSpells.lookup_monsters_spells
    End If
    
    If sInitialExePath <> txtExepath.Text Then
        cmdApply.Enabled = True
        cmdCancelSettings.Enabled = True
    End If
    
    
        'CurrentUserInfo.ExePath = CommonDialog.FileName
End Sub

Private Sub cmdExit2_Click()
Unload Me
End Sub

Private Sub cmdExport_Click()
    Dim lRtn&, lLineStart&, lLineEnd&, sLine$

    cmdExport.Enabled = False
    On Error GoTo erh
    With CommonDialog
        .FileName = read_key("config", "CSVfile", sIniFile)
        .InitDir = modParse.sCurDir
        If .FileName > "" Then
            lRtn = InStrRev(.FileName, "\")
            If lRtn > 0 Then
                .InitDir = Left$(.FileName, lRtn - 1&)
            End If
        End If
        .DialogTitle = "Enter a CSV filename for export to."
        .Filter = "CSV Files (*.CSV)|*.CSV"
        .Flags = cdlOFNCreatePrompt Or cdlOFNHideReadOnly Or cdlOFNExplorer
        .CancelError = True  ' False  // need to know when cancel button was pressed!
        .DefaultExt = "csv"
        .ShowOpen
        If (.FileName > "") Then  ' And (Dir(.FileName) > "") Then
            If (Dir(.FileName) > "") Then
                Kill .FileName  ' ensure that we start afresh on existing file
            End If
            cmdSpellRslt_Click (0)  ' force into summary mode
            modSpells.sSummaryRpt = RichTextBox1.Text
            With frmSplash
                .Show vbModeless
                .lblLoad.Caption = "Counting lines of report"
                .lblLoad.Refresh
                .ProgressBar1.Max = modSpells.countChars(modSpells.sSummaryRpt, vbCr) ' will
                .ProgressBar1.Value = 0
                .ProgressBar1.Visible = True
            End With
            lLineStart = 1&
            lLineEnd = InStr(modSpells.sSummaryRpt, vbCr)
            frmSplash.lblLoad.Caption = "Writing report to csv file"
            Do While (lLineEnd > 0) And (lLineEnd < Len(modSpells.sSummaryRpt))
                DoEvents
                frmSplash.ProgressBar1.Value = frmSplash.ProgressBar1.Value + 1
                sLine = Mid$(modSpells.sSummaryRpt, lLineStart, lLineEnd - lLineStart)   '  - 1&
                If Trim(sLine) > "" Then
                    modParse.ReplaceStringWithString sLine, "  ", " "
                    modParse.ReplaceStringWithString sLine, " ", ","    '  now should be comma delimited - output to csv
                    If (Left$(sLine, 8) = "Monster,") Then
                        sLine = "Monster \/           | Spell >" & Mid$(sLine, InStr(sLine, ">") + 1)
                    End If
                    Debug.Print sLine
                    Call store_append(.FileName, sLine)
                End If
                Do While Between(Asc(Mid$(modSpells.sSummaryRpt, lLineEnd, 1)), 10, 13)
                    lLineEnd = lLineEnd + 1&
                    If (lLineEnd >= Len(modSpells.sSummaryRpt)) Then Exit Do
                Loop
                lLineStart = lLineEnd
                lLineEnd = InStr(lLineStart, modSpells.sSummaryRpt, vbCr)
            Loop
            Unload frmSplash
            modSpells.sSummaryRpt = RichTextBox1.TextRTF

            lRtn = WritePrivateProfileString("config", "CSVfile", .FileName, sIniFile)
            Debug.Assert lRtn <> 0
        End If
    End With
erh:
    cmdExport.Enabled = True
    
    If Err.Number <> 0 Then
        If Err.Number <> cdlCancel Then
            MsgBox "cmdExport ERROR " & CStr(Err.Number) & " : " & Err.Description
        ' otherwise, error expected due to user clicking Cancel button
        End If
        Err.Clear
    End If
End Sub
Sub store_append(ByVal sResultsFile$, ByRef sContent$)
    Dim nFF%

    On Error GoTo erh
    nFF = FreeFile
    If nFF > 0 Then
        Open sResultsFile For Append As nFF
        Print #nFF, sContent
        Close #nFF
    End If
    Exit Sub
erh:
   MsgBox "store_append ERROR " & CStr(Err.Number) & " : " & Err.Description
   Err.Clear
End Sub

Private Sub cmdGoFPS_Click()
    Dim IsSomethingChecked As Boolean
    Dim Testindex As Integer
    
    
    Testindex = 0
    CurrentFPSInfo.SampleDuration = txtFPStime.Text
    CurrentFPSInfo.TestDone = False
    CurrentCaseType = FPS
    If chkCustom.Value = vbChecked And txtCustom.Text = "" Then
        MsgBox "Please enter a teleport location for custom sampling."
        Exit Sub
    End If
    
    CurrentFPSInfo.CurrentExcelFileName = CreateFPSFileName
    

    
            
    
    
    With CurrentFPSInfo
    

    .OpenExcel = CByte(chkFPSExcel.Value)
    .IsMapWorld = True
    If chkGoblinInventor.Value = vbChecked Then
        IsSomethingChecked = True
        .TeleportName = "gi_r4"
        .ExcelWorksheetName = "gi_r4"
        .IsMapWorld = True
        Testindex = Testindex + 1
        CurrentCaseType = FPS
        Call cmdGo_Click
        
        modExcel.ImportLog "gi_r4", .CurrentExcelFileName, Testindex
        
        'ReadFpsLog
    End If
    
    If chkIceCaves.Value = vbChecked Then
        IsSomethingChecked = True
        .TeleportName = "ac_r5"
        .ExcelWorksheetName = "ac_r5"
        .IsMapWorld = True
        Testindex = Testindex + 1
        CurrentCaseType = FPS
        Call cmdGo_Click
        
        modExcel.ImportLog "ic_r5", .CurrentExcelFileName, Testindex
    End If
    
    If chkGom.Value = vbChecked Then
        IsSomethingChecked = True
        .TeleportName = "gom2_b"
        .ExcelWorksheetName = "gom2_b"
        .IsMapWorld = True
        Testindex = Testindex + 1
        CurrentCaseType = FPS
        Call cmdGo_Click
        modExcel.ImportLog "Gom2_b", .CurrentExcelFileName, Testindex
    End If
    
    If chkCustom.Value = vbChecked Then
        IsSomethingChecked = True
        .TeleportName = txtCustom.Text
        .ExcelWorksheetName = txtCustom.Text
        .IsMapWorld = True
        Testindex = Testindex + 1
        Call cmdGo_Click
    End If
    
    If IsSomethingChecked = False Then
        MsgBox "Please select a region before clicking go."
    End If
    
    End With
        
    Dim yesno As Integer
        yesno = MsgBox("Tests are done.  Would yolu like to view the spreadsheet?", vbYesNo)
    If yesno = vbYes Then
        Set oExcel = New Excel.Application
        Set oCurrentWorkbook = oExcel.Workbooks.Open(CurrentFPSInfo.CurrentExcelFileName)
        oExcel.Visible = True
    End If
    
    
End Sub

Private Sub cmdPause_Click()
    ' provide a 'pause' feature for spells tests
    modSpells.bPause = Not modSpells.bPause ' simple toggle
    cmdPause.Enabled = False
    If modSpells.bPause Then
        cmdPause.Caption = "&Unpause"
    Else
        cmdPause.Caption = "&Pause"
    End If
End Sub

Private Sub cmdRetrieve_Click()
    cmdRetrieve.Enabled = False
    Call modSpells.setup_spells
    cmdRetrieve.Enabled = True
End Sub
Private Sub checkWinState()
    If Me.WindowState = vbMinimized Then
        Me.WindowState = vbNormal
    End If
End Sub

Private Sub cmdSpellRslt_Click(index As Integer)
   If index = 0 Then
       ' show summary
       HScrollSpellTest.Enabled = False
       RichTextBox1.TextRTF = modSpells.sSummaryRpt
       cmdExport.Enabled = Len(Trim(modSpells.sSummaryRpt)) > 0
   Else
       ' show detail view
       HScrollSpellTest.Enabled = True
       If Trim(modSpells.sDetailRpt) = "" Then
           Call frmMain.HScrollSpellTest_Change
       End If
       RichTextBox1.TextRTF = modSpells.sDetailRpt
       cmdExport.Enabled = False
   End If
End Sub

Private Sub cmdSpellsRun_Click()
 ' found that attempt to minimize this app led to av here - so now provide logic to avoid this
#Const SHOW_MSG = True
#Const RUN_IT = True
    Dim sRslt$, sCmd$   '
#If SHOW_MSG Then
    Dim sDesc$
#End If
#If RUN_IT Then
    Dim CurrentCaseInfo As Caseinfo
    Dim currentloginfo As LogInfo
    Dim sExePath$
#End If
    Dim CurrSpellCase As modDeclares.Spells
    Dim nFirstSpell%, nLastSpell%, nFirstMonster%, nLastMonster%, i%, j%
    Dim sBuildSpec$, bOldContinue As Boolean, nFirstSpellOld%
    
    modSpells.sCurBld = ""  ' init
    modSpells.sCurDateTimeFile = ""
    cmdAbort.Enabled = True
    cmdPause.Enabled = True
    cmdSpellsRun.Enabled = False
    cmbMonsterList.Enabled = False
    cmbSpellList.Enabled = False
    chkSpellLog.Enabled = False
    bOldContinue = chkContinueSpellRun.Enabled
    chkContinueSpellRun.Enabled = False
    bAbort = False ' set up to enable an abort
    bPause = False  '                    or pause
    cmdPause.Caption = "&Pause"
    sExePath = modParse.sCurDir & "\dungeonsiege" & IIf(optRelease.Value, "R", "D") & ".exe"
    If (Dir(sExePath, vbNormal) = "") Then
        MsgBox sExePath & " was not found. Aborting the test."
        Exit Sub
    End If
    sBuildSpec = GetFileVersion(sExePath)   ' fileversioninfo
    cmdViewSpells.Enabled = False
    cmdRetrieve.Enabled = False
    lstvBuildDTMachine.Enabled = False
    If cmbSpellList.ListIndex = cmbSpellList.ListCount - 1 Then
#If SHOW_MSG Then
        sDesc = "all spells"
#End If
        nFirstSpell = 0
        nLastSpell = cmbSpellList.ListCount - 2
    Else
#If SHOW_MSG Then
        sDesc = Trim(cmbSpellList.List(cmbSpellList.ListIndex))
#End If
        nFirstSpell = cmbSpellList.ListIndex
        nLastSpell = nFirstSpell
    End If
#If SHOW_MSG Then
    sDesc = sDesc & " against "
#End If
    If cmbMonsterList.ListIndex = cmbMonsterList.ListCount - 1 Then
#If SHOW_MSG Then
        sDesc = sDesc & "all monsters"
#End If
        nFirstMonster = 0
        nLastMonster = cmbMonsterList.ListCount - 2
    Else
#If SHOW_MSG Then
        sDesc = sDesc & Trim(cmbMonsterList.List(cmbMonsterList.ListIndex))
#End If
        nFirstMonster = cmbMonsterList.ListIndex
        nLastMonster = nFirstMonster
    End If
    If chkWIndowed.Value = 1 Then
#If SHOW_MSG Then
        Debug.Print "eventually run the chosen test(s) : " & sDesc & " full screen:"
#End If
    Else
#If SHOW_MSG Then
        Debug.Print "eventually run the chosen test(s) : " & sDesc & " windowed :"
#End If
    End If
    If frmMain.chkContinueSpellRun.Value = vbChecked Then ' correct starting sequence, if asked for
        nFirstMonster = GetMonsterIndex(sLastMonster)     ' advance to next monster, spell not yet tested
        nFirstSpellOld = nFirstSpell
        nFirstSpell = getSpellindex(sLastSpell)
        If nFirstSpell < cmbSpellList.ListCount - 2 Then
            nFirstSpell = nFirstSpell + 1
        Else
            nFirstSpell = nFirstSpellOld
            If nFirstMonster < cmbMonsterList.ListCount - 2 Then
                nFirstMonster = nFirstMonster + 1
            Else
                nFirstMonster = 0
            End If
        End If
        modDeclares.TimeDate = lstvBuildDTMachine.SelectedItem.SubItems(1)
    Else
        modDeclares.TimeDate = Now()
    End If
    For i = nFirstMonster To nLastMonster
        If (frmMain.chkContinueSpellRun.Value = vbChecked) And (i > nFirstMonster) And (nFirstSpell <> nFirstSpellOld) Then
            nFirstSpell = nFirstSpellOld
        End If
        For j = nFirstSpell To nLastSpell
            Call checkWinState
            txtMonsterPos.Caption = "Monster " & Format(i + 1) & " of " & Format(cmbMonsterList.ListCount - 1) & " : " & Trim(cmbMonsterList.List(i))
            txtSpellPos.Caption = "Spell " & Format(j + 1) & " of " & Format(cmbSpellList.ListCount - 1) & " : " & Trim(mySpells(j).screen_name)
            sCmd = sExePath & " skritbot=" & Trim(txtSkritPath.Text) & "\TestAllSpellOppt map=Halo_test_map  teleport=start "
            If chkWIndowed.Value = 0 Then
                sCmd = sCmd & " fullscreen=false "
            End If
            sCmd = sCmd & " Opponent=" & Trim(cmbMonsterList.List(i)) & "  Invincible=true  template=" & Trim(mySpells(j).template)
            sCmd = sCmd & " specializes=" & Trim(mySpells(j).specializes)
            sCmd = sCmd & " damage_max=" & CStr(mySpells(j).damage_max)
            sCmd = sCmd & " damage_min=" & CStr(mySpells(j).damage_min)
            sCmd = sCmd & " is_one_shot=" & CStr(mySpells(j).is_one_shot)
            sCmd = sCmd & " mana_cost=" & CStr(mySpells(j).mana_cost)
            sCmd = sCmd & " required_level=" & CStr(mySpells(j).required_level)
            sCmd = sCmd & " screen_name=" & Trim(mySpells(j).screen_name)
            sCmd = sCmd & " class=" & Trim(mySpells(j).Class)
            sCmd = sCmd & " demo=true"    '   comment out for verbose output: warnings
            Debug.Print sCmd
#If RUN_IT Then
            With CurrentCaseInfo
                .Author = "Brian Warris"
                .CaseName = "spell vs monster"
                .Description = "spell test"
                .Expected = "spell(s) to damage opponent"
                .Map = "Halo_test_map"
                .Teleport = "start"
                .TestArea = "spells"
                .Skrit = "TestAllSpellOppt"
                .SkritPath = txtSkritPath.Text
            End With
                
            With currentloginfo
                .CaseName = CurrentCaseInfo.CaseName
                .MachineName = CurrentUserInfo.MachineName
                .TimeDateStamp = modDeclares.TimeDate
                .UserName = CurrentUserInfo.UserName
                .IsDebug = optDebug.Value
            End With

            sRslt = bLaunchSpellTest(sCmd)
            Debug.Assert (sRslt = "")
            delay 0.2
            
            ReadTheLog currentloginfo
            With CurrSpellCase
                .PassFail = (currentloginfo.Pass)
                .BuildNumber = sBuildSpec
                .MachineName = CurrentUserInfo.MachineName
                .MonsterName = Trim(cmbMonsterList.List(i))
                .SpellName = Trim(cmbSpellList.List(j))
                .TimeDateStamp = modDeclares.TimeDate
                .LogData = currentloginfo.LogData
            End With
            If chkSpellLog.Value <> vbChecked Then
               SpellCaseLogToDB CurrSpellCase
            End If
            RichTextBox1.Text = currentloginfo.LogData
            Call modSpells.format_full_long(RichTextBox1)
            RichTextBox1.Refresh
            modSpells.sDetailRpt = RichTextBox1.TextRTF
            If modSpells.bAbort Then
                Exit For
            End If
            If modSpells.bPause Then
                Do While modSpells.bPause
                    delay 0.75!
                    DoEvents
                    cmdPause.Enabled = True ' enable the unpause
                Loop
            End If
#End If
        Next j
        If modSpells.bAbort Then
            Exit For
        End If
    Next i
    
    txtMonsterPos.Caption = ""
    txtSpellPos.Caption = ""
    cmdViewSpells.Enabled = True
    cmdRetrieve.Enabled = True
    cmdAbort.Enabled = False
    cmdPause.Enabled = False
    cmdSpellsRun.Enabled = True
    cmbMonsterList.Enabled = True
    cmbSpellList.Enabled = True
    chkSpellLog.Enabled = True
    chkContinueSpellRun.Enabled = bOldContinue
    lstvBuildDTMachine.Enabled = True
    If (chkSpellLog.Value <> vbChecked) Then
        Call cmdRetrieve_Click ' get results, if logging was invoked. Allows "continue from" option to have correct values
        HScrollSpellTest.Value = HScrollSpellTest.Max ' show last entry because that was the last displayed - no table read required
    End If
End Sub
Private Sub cmdSQLserver_Click()
txtSQLserver.Enabled = True
cmdApply.Enabled = True
cmdCancelSettings.Enabled = True

End Sub



Private Sub cmdViewSpells_Click()
    If cmdViewSpells.Caption = "&View Spells" Then
        cmdViewSpells.Caption = "&Hide Spells"
        cmdViewSpells.ToolTipText = "Hide the current parameters for the spells under test."
        cmdSpellsRun.Enabled = False
        lvSpells.Visible = True
    Else
        cmdViewSpells.Caption = "&View Spells"
        cmdViewSpells.ToolTipText = "View the current parameters for the spells under test."
        cmdSpellsRun.Enabled = True
        lvSpells.Visible = False
    End If
End Sub

Private Sub cmdWavRun_Click()
     Select Case cmdWavRun.Caption
         Case "&Run"
            cmdWavRun.Caption = "&Pause"
            Call modWAV.runWav
            If cmdWavRun.Caption = "Exit" Then
                cmdWavRun.Caption = "&Run"
            Else
                cmdErrExport.Enabled = (modWAV.nErrors > 0)    ' no WAV file test failed => button remains disabled : nothing to report
                If (Not cmdErrExport.Enabled) Then
                    MsgBox "The Export button remains disabled because there is nothing to report:" & vbCrLf & "all wav files checked out as OK"
                End If
            End If
         Case "&Pause"
            cmdWavRun.Caption = "&Unpause"
         Case "&Unpause"
            cmdWavRun.Caption = "&Pause"
     End Select
End Sub


Private Sub Command1_Click()
Command1.Enabled = False ' debounce the button
CurrentCaseType = pcontent
Call cmdGo_Click
Command1.Enabled = True ' debounce the button

End Sub






Private Sub Form_Load()

    Dim bSuccess As Boolean
    
    frmSplash.Show vbModeless
    
    'CheckIni
    'workaround for wonky rtf control behavior
    rtfDescription.Enabled = True
    rtfDescription.Enabled = False
    rtfExpected.Enabled = True
    rtfExpected.Enabled = False
    frameCaseMain(0).ZOrder (0)
'    Dim lstCaseItem As ListItem
    'Set lstCaseItem = lstLogCase.ListItems.Add(, , "No Cases run")
    frmMain.Width = frameCaseMain(TabStrip1.SelectedItem.index - 1).Width + 120
    frmMain.Height = frameCaseMain(TabStrip1.SelectedItem.index - 1).Height + 735
    CurrentCaseType = UnitTest
    'initialize UI and variable settings
    cmdNew.Enabled = False
    cmdEdit.Enabled = False
    'sBuildType = RELEASE_EXE
    modParse.sIniFile = App.Path & "\" & Trim(App.EXEName) & ".ini"
            ' read ini file
    If (Not modParse.ReadSQLSrvr()) Then ' choose a default entry: GPG standard
        CurrentUserInfo.SQLserver = modDB.defSQLserver      ' "markgrimsqlsvr"
    End If
    If modParse.SqlServerExists(CurrentUserInfo.SQLserver) Then ' two calls : other is on cmdApply_Click event
        txtSQLserver.Text = CurrentUserInfo.SQLserver
        Debug.Assert WriteSQLSrvr()
    Else
        If modParse.SqlServerExists(modDB.defSQLserver) Then
            CurrentUserInfo.SQLserver = modDB.defSQLserver
            txtSQLserver.Text = CurrentUserInfo.SQLserver ' revert to last known SQL server
        Else
            Call modParse.ExitNoSQLServer
        End If
    End If
    modDB.ConnectionString = "driver={SQL Server};SERVER=" & CurrentUserInfo.SQLserver & ";database=unittestdb;dsn=''"
    With CurrentUserInfo
        GetUserInfoFromSystem .OS, .UserName, .MachineName
        lblPlatform.Caption = .OS
        lblMachineName.Caption = .MachineName
        lblUserName.Caption = .UserName
        lblCurrentBuild = .CurrentBuild
    End With

    
    'Get the recordset
    dbOpenTable
    'build the tree control
    BuildTree
    bSuccess = GetUserInfoFromDb(CurrentUserInfo)
    If bSuccess = False Then
        MsgBox "This user has never logged in at this machine before, or you are currently running a different Operating system than indicated in the database.  Please update your user settings and click Apply."
        bFirstRun = True
        TabStrip1.Tabs(3).Selected = True
    Else
        bFirstRun = False
        Call modSpells.lookup_monsters_spells
    End If
        
    PopulatePcontentMonsters
        
    
    UpdateUserUI CurrentUserInfo
    Call modParse.get_delims ' USES  CurrentUserInfo.ExePath to locate ds exe
    Unload frmSplash
End Sub
Public Sub PopulatePcontentMonsters()
    Dim i As Integer
    For i = 1 To cmbMonsterList.ListCount - 1
        cmbPcontentCritter.AddItem cmbMonsterList.List(i)
    Next
    
    cmbPcontentCritter.ListIndex = i - 2
    
    
    
End Sub

Public Sub AddRunCaseToQueue()

End Sub
Private Sub cmdBrowse_Click()
    
    With CommonDialog
        .DialogTitle = "Select a skrit"
        .Filter = "Skrits (*.skrit)|*.skrit"
        .Flags = cdlOFNFileMustExist Or cdlOFNHideReadOnly Or cdlOFNExplorer
        .ShowOpen
    End With
        If CommonDialog.FileName > "" Then
            txtSkrit.Text = CommonDialog.FileName
        End If
End Sub

Private Sub cmdGo_Click()

    Dim iCount As Integer
    
    

    Dim sExePath As String

    cmdGo.Enabled = False ' debounce the button
    sExePath = CurrentUserInfo.ExePath
    If sExePath = "" Then
        MsgBox "Please enter the path to the desired Dungeon Siege Executable"
        TabStrip1.Tabs(3).Selected = True
        Call cmdExeBrowse_Click
        
        Exit Sub
    End If
    
    sCurDir = CurrentUserInfo.ExePath
    
    Select Case CurrentCaseType
        Case UnitTest
            RunUnitTests sExePath
        Case pcontent
            RunPContentQuery sExePath
        Case FPS
            RunFPSTests sExePath, CurrentFPSInfo.TeleportName
    End Select
    
    cmdGo.Enabled = True ' debounce the button
    frmMain.WindowState = vbNormal
End Sub

Private Function RunUnitTests(sExePath As String)
    Dim CurrentCaseInfo As Caseinfo
    Dim currentloginfo As LogInfo
    Dim sCommandstr As String
    


    If lstCases.ListCount = 0 Then
        MsgBox "Please add the desired test cases to the queue before launching."
        Exit Function
    End If
    
  '  If lstCases.Selected(0) = False Then
  '
  '      lstCases.Selected(0) = True
   ' End If
    lstCases.SetFocus
    lstCases.Selected(0) = True
    
If chkHarnessMin.Value = 1 Then
    frmMain.WindowState = vbMinimized
End If

    For iCaseCount = 0 To lstCases.ListCount - 1
        If iCaseCount > 0 Then
             lstCases.Selected(iCaseCount) = True
        End If
        

        CurrentCaseInfo = BuildCaseInfoFromUI
        modDeclares.TimeDate = Now()
        
        With currentloginfo
            .CaseName = CurrentCaseInfo.CaseName
            .MachineName = CurrentUserInfo.MachineName
            .TimeDateStamp = modDeclares.TimeDate
            .UserName = CurrentUserInfo.UserName
            .IsDebug = optDebug.Value
            
        End With
            
            TabStrip1.Tabs(2).Selected = True
            sCommandstr = BuildCommandLineString(CurrentCaseInfo, sExePath, CurrentUserInfo)
            
            bLaunchDungeonSiege sCommandstr, currentloginfo
            delay 0.2
            
            modParse.ReadTheLog currentloginfo
            AddCaseLogToDB currentloginfo
            UpdateRunTestList currentloginfo
            
    Next
End Function
Private Function RunPContentQuery(sExePath As String)
    Dim PcontentQueryString As String
    Dim currentloginfo As LogInfo
    Dim sCommandstr As String

    
If chkHarnessMin.Value = 1 Then
    frmMain.WindowState = vbMinimized
End If
        If OptUseMonsters.Value = True Then
            PcontentQueryString = BuildPContentQuery(txtIterations.Text, cmbPcontentCritter.List(cmbPcontentCritter.ListIndex))
            sCommandstr = BuildPcontentCommandLineString(cmbPcontentCritter.List(cmbPcontentCritter.ListIndex), sExePath)
        ElseIf OptQuery.Value = True Then
            PcontentQueryString = BuildPContentQuery(txtIterations.Text, txtPcontentQuery.Text)
            sCommandstr = BuildPcontentCommandLineString(txtPcontentQuery.Text, sExePath)
        End If
        
        modDeclares.TimeDate = Now()
        
        With currentloginfo
            .CaseName = CurrentPcontent.PcontentCaseName
            .MachineName = CurrentUserInfo.MachineName
            .TimeDateStamp = modDeclares.TimeDate
            .UserName = CurrentUserInfo.UserName
            .IsDebug = optDebug.Value
        End With
            
            TabStrip1.Tabs(2).Selected = True
            
            ChDir sExePath
            ChDrive Left$(sExePath, 1)
            
            bLaunchDungeonSiege sCommandstr, currentloginfo
            delay 0.2
            
            modParse.ReadTheLog currentloginfo
            AddCaseLogToDB currentloginfo
            UpdateRunTestList currentloginfo
            If chkExcel.Value = 1 Then
                CreateExcelObject
                OpenExisitingWorkBook sExePath & "\" & CurrentPcontent.PcontentExcelFileName
            End If
End Function
Private Function RunFPSTests(sExePath As String, CurrentTeleport As String)
    Dim PcontentQueryString As String
    Dim currentloginfo As LogInfo
    Dim sCommandstr As String
    Dim CurrentTimeDate As String
    Dim FirstSpace As Integer
    
If chkHarnessMin.Value = 1 Then
    frmMain.WindowState = vbMinimized
End If
        
        'currentfpsinfo.openexcel =
        
        CurrentTimeDate = Now()
        With currentloginfo
            .CaseName = CurrentFPSInfo.TeleportName
            .MachineName = CurrentUserInfo.MachineName
            .TimeDateStamp = CurrentTimeDate
            .UserName = CurrentUserInfo.UserName
            .IsDebug = optDebug.Value
        End With
            
        TabStrip1.Tabs(2).Selected = True
        
        sCommandstr = BuildFPSCommandLineString(sExePath)
        
        ChDir sExePath
        ChDrive Left$(sExePath, 1)
        
        bLaunchDungeonSiege sCommandstr, currentloginfo
        delay 0.2
        
        modParse.ReadTheLog currentloginfo
        AddCaseLogToDB currentloginfo
        UpdateRunTestList currentloginfo
        'If chkExcel.Value = 1 Then
        '    CreateExcelObject
        '    OpenExisitingWorkBook sExePath & "\" & CurrentPcontent.PcontentExcelFileName
        'End If

        
End Function

Private Sub UpdateRunTestList(currentloginfo As LogInfo)
    Dim MyListItem As ListItem
    Dim ImageIndex As Integer
    lstLogCase.SmallIcons = Me.ImageList
    lstLogCase.View = lvwReport
    If currentloginfo.Pass = True Then
        ImageIndex = 5
    Else
        ImageIndex = 6
    
    End If
    
    Set MyListItem = lstLogCase.ListItems.Add(, , currentloginfo.CaseName, , ImageIndex)

        With MyListItem
        
        .SubItems(1) = currentloginfo.TimeDateStamp
        .Selected = True
        lstLogCase_Click

        End With
    Set MyListItem = Nothing
End Sub
Private Function BuildCommandLineString(CurrentCaseInfo As Caseinfo, sExePath As String, CurrentUserInfo As UserInfo) As String
    Dim sCommandstr As String
    Dim sDSFlavor As String
    If CurrentUserInfo.IsDebug = True Then
        sDSFlavor = DEBUG_EXE
    Else: sDSFlavor = RELEASE_EXE
    End If
    If CurrentCaseInfo.Arguments > "" Then
    sCommandstr = sExePath & "\" & sDSFlavor & " noloadintro=true mpscids=true map=" & CurrentCaseInfo.Map & " teleport=" & CurrentCaseInfo.Teleport & " fullscreen=true " & " skritbot=" & CurrentCaseInfo.SkritPath & "\" & CurrentCaseInfo.Skrit & "?" & CurrentCaseInfo.Arguments
    Else
    
    sCommandstr = sExePath & "\" & sDSFlavor & " noloadintro=true mpscids=true skritbot=" & CurrentCaseInfo.SkritPath & "\" & CurrentCaseInfo.Skrit & " map=" & CurrentCaseInfo.Map & " teleport=" & CurrentCaseInfo.Teleport & " fullscreen=true"
    End If
    BuildCommandLineString = sCommandstr

End Function

Private Function BuildPcontentCommandLineString(PcontentQueryString As String, sExePath As String) As String
    Dim sCommandstr As String
    Dim sDSFlavor As String
    If CurrentUserInfo.IsDebug = True Then
        sDSFlavor = DEBUG_EXE
    Else: sDSFlavor = RELEASE_EXE
    End If
    sCommandstr = sExePath & "\" & sDSFlavor & " map=" & "halo_test_map" & " teleport=" & "start" & " fullscreen=" & IIf(chkWIndowed.Value, "true", "false") & " skritbot=" & "auto/test/" & PCONTENT_SKRIT & "?pcontentquerystring=" & PcontentQueryString & "&FileName=" & CurrentPcontent.PcontentExcelFileName & "&Iterations=" & CurrentPcontent.PcontentIterations & "&usecreature=" & IIf(OptUseMonsters.Value, "True", "False")
    
    

    BuildPcontentCommandLineString = sCommandstr

End Function
Private Function BuildFPSCommandLineString(sExePath As String) As String
    Dim sCommandstr As String
    Dim sDSFlavor As String
    Dim sMap As String
    If CurrentUserInfo.IsDebug = True Then
        sDSFlavor = DEBUG_EXE
    Else: sDSFlavor = RELEASE_EXE
    End If
    If CurrentFPSInfo.IsMapWorld = True Then
        sMap = "map_world"
    Else
        sMap = "Halo_test_map"
    End If
    sCommandstr = ""
    sCommandstr = sExePath & "\" & sDSFlavor & " map=" & sMap & " teleport=" & CurrentFPSInfo.TeleportName & " fullscreen=" & IIf(chkWIndowed.Value, "true", "false") & " mpscids=true fpslog=true skritbot=" & "auto/test/" & FPS_SKRIT & "?TestLength=" & CurrentFPSInfo.SampleDuration

    BuildFPSCommandLineString = sCommandstr

End Function

Private Sub cmdSave_Click()
    Dim InitialCaseName As String
    Dim MyCaseInfo As Caseinfo
    
    'validate that all the fields are filled in
    If CheckUIForNulls <> "NONULLS" Then
        MsgBox "Please enter a value for the following: " & CheckUIForNulls
        Exit Sub
    End If
    InitialCaseName = txtCaseName.Text
    MyCaseInfo = BuildCaseInfoFromUI
    
    If eCaseAction = TC_NEW_CASE Then
        If bCheckCaseExistsInDb(txtCaseName.Text) Then
            MsgBox "A test case with this name exists.  Please choose a new name or Cancel then select Edit."
            Exit Sub
        Else
            WriteNewCase MyCaseInfo
            
            cmdNew.Caption = "&New"
            cmdSave.Enabled = False
            
        End If

    ElseIf eCaseAction = TC_EDIT_CASE Then
        If bCheckCaseExistsInDb(txtCaseName.Text) Then

            ModifyExistingCase MyCaseInfo


        Else:
            WriteNewCase MyCaseInfo
            
        End If
    cmdEdit.Caption = "&Edit"
    cmdSave.Enabled = False
    End If
    
    UpdateTreeView
    tvCases.Enabled = True
    tvCases.SetFocus
    tvCases.Nodes(GetIndex(txtCaseName.Text)).Selected = True
    DisableCaseInfo
End Sub









Private Sub cmdClear_Click()
lstCases.Clear
End Sub

Private Sub cmdEdit_Click()
    If cmdEdit.Caption = "&Edit" Then
        cmdEdit.Caption = "&Cancel"
        tvCases.Enabled = False
        cmdSave.Enabled = True
        cmdBrowse.Enabled = True
        
        cmdNew.Enabled = False
        eCaseAction = TC_EDIT_CASE
        
        EnableCaseInfo
    
    
    ElseIf cmdEdit.Caption = "&Cancel" Then
        cmdEdit.Caption = "&Edit"
        cmdSave.Enabled = False
        cmdBrowse.Enabled = False
        tvCases.Enabled = True
        cmbArea.Enabled = False
        cmdNew.Enabled = True
        tvCases.SetFocus
        GetCaseInfo tvCases.SelectedItem.Text
        
        DisableCaseInfo
    End If
End Sub

Private Sub cmdExit_Click()
Unload Me


End Sub

Private Sub cmdNew_Click()
    If cmdNew.Caption = "&New" Then
        eCaseAction = TC_NEW_CASE
        cmdNew.Caption = "&Cancel"
        cmdSave.Enabled = True
        cmdBrowse.Enabled = True
        tvCases.Enabled = False
        cmdEdit.Enabled = False
        
        'sCaseAction = TC_NEW_CASE
        ClearCaseFileds
        txtAuthor.Text = CurrentUserInfo.UserName
        EnableCaseInfo

    
    ElseIf cmdNew.Caption = "&Cancel" Then
        cmdNew.Caption = "&New"
        tvCases.Enabled = True
        cmdSave.Enabled = False
        cmdBrowse.Enabled = False
        'sCaseAction = 0
        cmdEdit.Enabled = True
        
        tvCases.SetFocus
        GetCaseInfo tvCases.SelectedItem.Text
        DisableCaseInfo

        
        
    End If
    
End Sub
Private Sub DisableCaseInfo()
        txtArgs.Enabled = False
        txtAuthor.Enabled = False
        txtCaseName.Enabled = False
        txtMap.Enabled = False
        txtSkrit.Enabled = False
        txtTeleport.Enabled = False
        rtfDescription.Enabled = False
        rtfExpected.Enabled = False
        cmbArea.Enabled = False
End Sub
Private Sub EnableCaseInfo()
        txtArgs.Enabled = True
        txtAuthor.Enabled = True
        txtCaseName.Enabled = True
        txtMap.Enabled = True
        txtSkrit.Enabled = True
        txtTeleport.Enabled = True
        rtfDescription.Enabled = True
        rtfExpected.Enabled = True
        cmbArea.Enabled = True
End Sub
Private Sub cmdRemove_Click()
On Error Resume Next
Dim index As Integer
    index = lstCases.ListIndex
    If index > -1 Then
        lstCases.RemoveItem index
        If index > 0 Then
            lstCases.ListIndex = index - 1
        Else
            lstCases.ListIndex = 0
        End If
    Else
        lstCases.ListIndex = lstCases.ListCount - 1
    End If
End Sub


Private Sub BuildTree()
    Dim iNodeCounter As Integer
    Dim iNodeIndex As Integer
    Dim iCaseNodeIndex As Integer
    Dim oTreeNode As Node

    
    Set tvCases.ImageList = Me.ImageList
    Set oTreeNode = tvCases.Nodes.Add(, , , "Unit Tests", 1)
    cmbArea.Clear

    
    
    For iNodeCounter = 0 To UBound(CaseArray())

            If Not bExists(CaseArray(iNodeCounter).TestArea) Then
                AddToTree CaseArray(iNodeCounter).TestArea, True, 1
                 cmbArea.AddItem (CaseArray(iNodeCounter).TestArea)

            End If
            
            If Not bExists(CaseArray(iNodeCounter).CaseName) Then
                iNodeIndex = GetIndex(CaseArray(iNodeCounter).TestArea)
                AddToTree CaseArray(iNodeCounter).CaseName, False, iNodeIndex
                
                CaseArray(iNodeCounter).index = GetIndex(CaseArray(iNodeCounter).CaseName)
                
            End If
            

    Next iNodeCounter
    tvCases.Nodes.Item(1).Expanded = True
End Sub

Private Function bExists(sAreaName As String) As Boolean
    'Verify that a node in the tree does not exist.
    'If so, do not add twice.  All cases should have Unique names
    'In the db.  There will be several area entries,
    'but it is important that area names are only represented once
    Dim iCount As Integer
    Dim oTreeNode As Node
        
        For iCount = 1 To tvCases.Nodes.Count
        Set oTreeNode = tvCases.Nodes.Item(iCount)
        
            If oTreeNode.Text = sAreaName Then
                bExists = True
                Exit Function
            Else
                bExists = False
            End If
        
        Next iCount
    
End Function
Private Sub AddToTree(sNode As String, bIsArea As Boolean, iindex As Integer)
    'Adds both the nodes and images to the tree control
    Dim oTreeCaseNode As Node
    Dim oTreeAreaNode As Node
        
        'First, check to see if the node coming in represents a test area
        'If so, add a folder, and make it a child of the root node
        If bIsArea Then
            Set oTreeAreaNode = tvCases.Nodes.Add(iindex, tvwChild, , sNode, 2)
        
        'Otherwise, set it as a test case and give it the little paper
        'image
        ElseIf Not bIsArea Then
            Set oTreeAreaNode = tvCases.Nodes.Add(iindex, tvwChild, , sNode, 4)
        End If
    
End Sub

Private Function GetIndex(sText As String) As Integer
    'Finds the node index for any case in the tree
    Dim iCount As Integer
    Dim oTreeNode As Node
    
        For iCount = 1 To tvCases.Nodes.Count
            Set oTreeNode = tvCases.Nodes.Item(iCount)
            If oTreeNode.Text = sText Then
                GetIndex = oTreeNode.index
                Exit Function
            End If
        Next iCount
End Function



Private Sub Form_QueryUnload(Cancel As Integer, UnloadMode As Integer)
    Cancel = (cmdWavRun.Caption = "&Unpause")
    If Cancel Then cmdWavRun.Caption = "Exit"
End Sub

Public Sub HScrollSpellTest_Change()
  Dim sBuild$, sDateTime$, sMachine$, myConnStr$
  
  ' put data into RichTextBox1 as driven from listview lstvBuildDTMachine
  Dim cnConnection As New ADODB.Connection
  Dim cmdCommand As New ADODB.Command
  Dim rstCases As New ADODB.Recordset
  
    ' allow walking through results
  myConnStr = ConnectionString  '  SetDBasConnStr("SpellLogs")
  If (lstvBuildDTMachine.ListItems.Count > 0) Then
      sBuild = RTrim(lstvBuildDTMachine.SelectedItem.Text)
      sDateTime = lstvBuildDTMachine.SelectedItem.SubItems(1)
      sMachine = lstvBuildDTMachine.SelectedItem.SubItems(2)
  Else
      ' no data - out of here!
      GoTo erh
  End If
  On Error Resume Next
  With cnConnection
        .ConnectionString = myConnStr    ' was ConnectionString - need new db
        .ConnectionTimeout = 100
        .Open
  End With
  With cmdCommand
        Set .ActiveConnection = cnConnection
        .CommandText = "select spellLog from SpellResults where BuildNumber='" & sBuild & _
            "' and DateTimeStamp='" & sDateTime & "' and MachineName='" & sMachine & "' order by monsterName, SpellName"
            
        .CommandTimeout = 150
  End With
  With rstCases
        'Debug.Print "Table-type recordset: " & .Name
        
        .Open cmdCommand, , adOpenStatic, adLockBatchOptimistic
        If Err.Number <> 0 Then
            MsgBox "HScrollSpellTest_Change error " & CStr(Err.Number) & " : " & Err.Description
            Err.Clear
        Else
            .Move frmMain.HScrollSpellTest.Value, adBookmarkFirst
            RichTextBox1.Text = !spellLog
            Call modSpells.format_full_long(RichTextBox1)
            RichTextBox1.Refresh
            modSpells.sDetailRpt = RichTextBox1.TextRTF
        End If
  End With
  
  If Not rstCases Is Nothing Then
      If rstCases.State <> adStateClosed Then rstCases.Close
      Set rstCases = Nothing
  End If
  Set cmdCommand = Nothing
erh:
  If cnConnection.State <> adStateClosed Then cnConnection.Close
  Set cnConnection = Nothing ' release memory taken
End Sub

Private Sub lstCases_Click()
    'gets the caseinfo and populates the UI if a test in the list
    'is selected
    GetCaseInfo lstCases.Text
End Sub

Private Sub lstCases_KeyDown(KeyCode As Integer, Shift As Integer)
    
    'This ub basically deletes cases from the list view
    Shift = 0
    'Check for [delete] key_down
    If KeyCode = 46 Then
        Call cmdRemove_Click
    End If
End Sub

Private Sub lstLogCase_Click()
    Dim sCaseName As String
    Dim sDateTime As String
    Dim sLog As String
    On Error Resume Next
    
    sCaseName = lstLogCase.SelectedItem.Text
    sDateTime = lstLogCase.SelectedItem.SubItems(1)
    sLog = GetLogFromDb(sCaseName, sDateTime)
    txtResult.Text = sLog
    Call format_full_long(txtResult)
    Exit Sub
    

End Sub




Public Sub lstvBuildDTMachine_Click()
  Dim sBuild$, sDateTime$, sMachine$
  
  ' put data into RichTextBox1 as driven from listview lstvBuildDTMachine
  Dim cnConnection As New ADODB.Connection
  Dim cmdCommand As New ADODB.Command, cmdCommandSp As New ADODB.Command
  Dim rstCases As New ADODB.Recordset, rstSpells    As New ADODB.Recordset
  Dim myConnStr$, sPrvMonster$
  Dim nPage%, nLine%
  Dim nCurSelection%, i%, lRecCntr&
  
  lstvBuildDTMachine.Enabled = False
  nCurSelection = -1
  sPrvMonster = ""
  If lstvBuildDTMachine.ListItems.Count = 0 Then Exit Sub
  nCurSelection = lstvBuildDTMachine.SelectedItem.index
'  Debug.Assert (Between(nCurSelection, 0, lstvBuildDTMachine.ListItems.Count))
  If (nCurSelection < 0) Then
      GoTo erh
  End If
  If (StrComp(modSpells.sCurBld, lstvBuildDTMachine.ListItems(nCurSelection).Text) <> 0) Or _
            (StrComp(modSpells.sCurDateTimeFile, lstvBuildDTMachine.ListItems(nCurSelection).SubItems(1)) <> 0) Then
      myConnStr = ConnectionString       '  SetDBasConnStr("SpellLogs")
      sBuild = RTrim(lstvBuildDTMachine.ListItems(nCurSelection).Text)
      sDateTime = lstvBuildDTMachine.ListItems(nCurSelection).SubItems(1)
      sMachine = lstvBuildDTMachine.ListItems(nCurSelection).SubItems(2)
      sLastSpell = ""
      modSpells.sCurBld = sBuild
      modSpells.sCurDateTimeFile = sDateTime
      
      On Error Resume Next
      With cnConnection
            .ConnectionString = myConnStr    ' was ConnectionString - need new db
            .ConnectionTimeout = 100
            .Open
      End With
      With cmdCommand
            Set .ActiveConnection = cnConnection
            .CommandText = "select MonsterName, SpellName, PassFail from SpellResults where BuildNumber='" & sBuild & _
                "' and DateTimeStamp='" & sDateTime & "' and MachineName='" & sMachine & "' order by monsterName, SpellName"
                
            .CommandTimeout = 150
      End With
      With cmdCommandSp
            Set .ActiveConnection = cnConnection
            .CommandText = "select distinct SpellName from SpellResults where BuildNumber='" & sBuild & _
                "' and DateTimeStamp='" & sDateTime & "' and MachineName='" & sMachine & "'  order by SpellName"
                
            .CommandTimeout = 150
      End With
      With rstSpells
          .Open cmdCommandSp, , adOpenStatic, adLockBatchOptimistic
          If Err.Number <> 0 Then
                GoTo erh
          End If
      End With
      
      With rstCases
            'Debug.Print "Table-type recordset: " & .Name
            
            .Open cmdCommand, , adOpenStatic, adLockBatchOptimistic
            If Err.Number <> 0 Then
                MsgBox "frmMain.lstvBuildDTMachine_Click error " & CStr(Err.Number) & " : " & Err.Description
                GoTo erh
            Else
                nPage = 0
                nLine = 81
                sPrvMonster = ""
                modSpells.sSummaryRpt = ""
                cmdExport.Enabled = False
                lRecCntr = 0&
                If bSplashOn Then
                    With frmSplash.ProgressBar1
                        .Max = rstCases.RecordCount
                        .Value = 0&
                        .Refresh
                    End With
                    With frmSplash.lblLoad
                        .Caption = "Load Progress: last spell run summary"
                        .Refresh
                    End With
                End If
                Do While Not .EOF
                    If (sPrvMonster <> !MonsterName) Then
                        If (nLine > 65) Then ' start new heading
                            If (nPage > 0) Then modSpells.sSummaryRpt = modSpells.sSummaryRpt & vbCrLf
                            nPage = nPage + 1
                            modSpells.sSummaryRpt = modSpells.sSummaryRpt & "Result of spells test conducted on : Build " & sBuild & "; Computer " & _
                                                    sMachine & "; " & sDateTime & "   Page :" & Format(nPage, "###") & vbCrLf & vbCrLf
                            modSpells.sSummaryRpt = modSpells.sSummaryRpt & padr("Monster \/           | Spell >", 36)
                            rstSpells.MoveFirst
                            Do While Not rstSpells.EOF
                                modSpells.sSummaryRpt = modSpells.sSummaryRpt & padr(modSpells.RmvSpace(rstSpells("SpellName")), mylen(7))
                                rstSpells.MoveNext
                            Loop
                            nLine = 2
                        End If
                        ' start new line
                        modSpells.sSummaryRpt = modSpells.sSummaryRpt & vbCrLf & padr(Trim(!MonsterName), 36)
                        nLine = nLine + 1
                        sPrvMonster = RTrim(!MonsterName)
                    End If
                    modSpells.sSummaryRpt = modSpells.sSummaryRpt & padr(IIf(CBool(!PassFail), "Pass", "Fail"), mylen(7))
                    sLastSpell = RTrim(!SpellName)
                    lRecCntr = lRecCntr + 1&
                    If bSplashOn Then
                        With frmSplash
                            .ProgressBar1.Value = lRecCntr
                            If ((.ProgressBar1.Value Mod (.ProgressBar1.Max \ 5&)) = 0) Then .Refresh
                        End With
                    End If
                    
                    .MoveNext
                Loop
                modSpells.sSummaryRpt = modSpells.sSummaryRpt & vbCrLf
    '            .Close
            End If
      End With
      sLastMonster = sPrvMonster
      frmMain.chkContinueSpellRun.Enabled = (sLastMonster > "") And (sLastSpell > "") And (sBuild = GetFileVersion(modParse.sCurDir & "\dungeonsiege" & IIf(optRelease.Value, "R", "D") & ".exe"))
      If Not frmMain.chkContinueSpellRun.Enabled Then frmMain.chkContinueSpellRun.Value = vbUnchecked
      If Trim(modSpells.sSummaryRpt) > "" Then
          RichTextBox1.Text = modSpells.sSummaryRpt
          Call processStatus(RichTextBox1, "Fail", vbRed, 0)
          Call processStatus(RichTextBox1, "Pass", vbGreen, 1)
          modSpells.sSummaryRpt = RichTextBox1.TextRTF
          cmdExport.Enabled = True
      Else
          modSpells.sSummaryRpt = ""
          cmdExport.Enabled = False
      End If
  Else
      lRecCntr = HScrollSpellTest.Max + 1&
  End If
  On Error GoTo 0
erh:
  If lRecCntr > 0& Then
      HScrollSpellTest.Max = lRecCntr - 1&
      HScrollSpellTest.LargeChange = IIf(HScrollSpellTest.Max > 4&, HScrollSpellTest.Max / 5&, 1&)
  Else
      HScrollSpellTest.Max = 0&
      HScrollSpellTest.LargeChange = 1&
  End If
  HScrollSpellTest.Min = 0&
  HScrollSpellTest.SmallChange = 1&
  
  HScrollSpellTest.Value = 0&
  If Err.Number <> 0 Then
      MsgBox "frmMain.lstvBuildDTMachine_Click error " & CStr(Err.Number) & " : " & Err.Description
      Err.Clear
  End If
  If Not rstCases Is Nothing Then
      If rstCases.State <> adStateClosed Then rstCases.Close
      Set rstCases = Nothing
  End If
  If Not rstSpells Is Nothing Then
      If rstSpells.State <> adStateClosed Then rstSpells.Close
      Set rstSpells = Nothing
  End If
  Set cmdCommand = Nothing
  Set cmdCommandSp = Nothing
  If cnConnection.State <> adStateClosed Then cnConnection.Close
  Set cnConnection = Nothing ' release memory taken
  lstvBuildDTMachine.Enabled = True
End Sub

Private Sub optDebug_Click()
cmdApply.Enabled = True
cmdCancelSettings.Enabled = True
End Sub

Private Sub OptQuery_Click()
    txtPcontentQuery.Enabled = True
    cmbPcontentCritter.Enabled = False
End Sub

Private Sub optRelease_Click()
cmdApply.Enabled = True
cmdCancelSettings.Enabled = True
End Sub

Private Sub OptUseMonsters_Click()
    txtPcontentQuery.Enabled = False
    cmbPcontentCritter.Enabled = True
    
End Sub

Private Sub SystemMonitor1_OnDblClick(ByVal index As Long)
With SystemMonitor1
    .CollectSample
    .UpdateGraph
End With
End Sub

'Private Sub mnuDebug_Click()
    'switch from release to debug
 '   If mnuRelease.Checked = True Then
 '       mnuDebug.Checked = True
 '       mnuRelease.Checked = False
'    End If
'End Sub



'Private Sub mnuRelease_Click()
    'switch from debug to release
'    If mnuDebug.Checked = True Then
'        mnuRelease.Checked = True
'        mnuDebug.Checked = False
'    End If

'End Sub



Private Sub TabStrip1_Click()
'If cmdApply.Enabled = True Then
'    MsgBox "Please Press Apply to commit changes!"
'    Exit Sub
'End If
If frmMain.WindowState = vbNormal Then
    frameCaseMain(TabStrip1.SelectedItem.index - 1).ZOrder (0)
    frmMain.Width = frameCaseMain(TabStrip1.SelectedItem.index - 1).Width + 120
    frmMain.Height = frameCaseMain(TabStrip1.SelectedItem.index - 1).Height + 735
    CurrentCaseType = TabStrip1.SelectedItem.index
End If


End Sub

Private Sub Timer1_Timer()
Timer1.Enabled = False

End Sub

Private Sub tvCases_Click()

    'verifies that a case is indeed the selected node,
    'then returns the CaseInfo for said case
    If tvCases.SelectedItem.Children = 0 Then
        GetCaseInfo (tvCases.SelectedItem.Text)
        cmdNew.Enabled = True
        cmdEdit.Enabled = True
    Else
        cmdNew.Enabled = False
        cmdEdit.Enabled = False
        ClearCaseFileds
        'cmbArea.Text = ""
        Exit Sub
    End If
End Sub

Private Sub tvCases_Collapse(ByVal Node As MSComctlLib.Node)
    'Closes the folder graphic when an area node is colapsed
    If Node.Image = 3 Then
        Node.Image = 2
    End If

End Sub

Private Sub tvCases_DblClick()
    'Checks to make sure a testcase is the active selected
    'then adds it to the listbox control
    If tvCases.SelectedItem.Children = 0 Then
        lstCases.AddItem tvCases.SelectedItem
    Else
        Exit Sub
    End If
End Sub
Private Sub cmdAdd_Click()
    'Checks to make sure a testcase is the active selected
    'then adds it to the listbox control
    If tvCases.SelectedItem.Children = 0 Then
        lstCases.AddItem tvCases.SelectedItem
    Else
        Exit Sub
    End If
End Sub
Private Sub GetCaseInfo(sSelectedText As String)
    Dim iCount As Integer
    
    'Traverse the tree and find the case with matching case name
    For iCount = 0 To UBound(CaseArray())
        If sSelectedText = CaseArray(iCount).CaseName Then
            'Once found, populate the ui with the values found in the
            'corresponding Caseinfo struct
            With CaseArray(iCount)
                txtCaseName.Text = .CaseName
                txtAuthor.Text = .Author
                txtMap.Text = .Map
                txtTeleport.Text = .Teleport
                txtArgs.Text = .Arguments
                txtSkrit.Text = .Skrit
                rtfDescription.Text = .Description
                rtfExpected.Text = .Expected
                cmbArea.Text = .TestArea
            End With
        End If
    Next iCount
End Sub


Private Sub tvCases_Expand(ByVal Node As MSComctlLib.Node)
    'Change from closed folder to open folder
    If Node.Image = 2 Then
        Node.Image = 3
    End If
    
End Sub
Private Sub ClearCaseFileds()
        'simply clears all UI fields for new case, or when selecting
        'an area node
        txtArgs.Text = ""
        txtAuthor.Text = ""
        txtCaseName.Text = ""
        txtMap.Text = ""
        txtSkrit.Text = ""
        txtTeleport.Text = ""
        rtfDescription.Text = ""
        rtfExpected.Text = ""
End Sub




Private Sub UpdateTreeView()
    tvCases.Nodes.Clear
    dbOpenTable
    'build the tree control
    BuildTree

End Sub

Private Function bLaunchDungeonSiege(sCommandLineString As String, currentloginfo As LogInfo) As String
   Dim hInst As Long             ' Instance handle from Shell function.
   Dim hWndApp As Long           ' Window handle from GetWinHandle.
   Dim buffer As String          ' Holds caption of Window.
   Dim numChars As Integer       ' Count of bytes returned.
   Dim idproc As Long
   'build the command string
   
   ChDrive (Left$(sCommandLineString, 1))
   ChDir (modParse.sCurDir)
   ' Shell to an application
   modDeclares.tLaunchTD = Now()
   Debug.Print sCommandLineString
   hInst = Shell(sCommandLineString, vbNormalFocus)
   
   ' Begin search for handle
   hWndApp = GetWinHandle(hInst)
   
   If hWndApp <> 0 Then
      ' Init buffer
      buffer = Space$(128)
      
      ' Get caption of window
      numChars = GetWindowText(hWndApp, buffer, Len(buffer))
   

      'MsgBox "You shelled to the Application: " & Left$(buffer, numChars)
        'MsgBox "Hwnd = " & hWndApp
        idproc = ProcIDFromWnd(hWndApp)
  
        'MsgBox "Idproc " & idproc
  '  GetWindowThreadProcessId hWndApp, idproc
    SetFocusAPI (hWndApp)
        Do While idproc > 0
 '          Timer1.Enabled = True
 '          Timer1.Interval = 5000
           DoEvents
 '       ' Get PID for this HWnd
        'MsgBox "Still running"
          idproc = ProcIDFromWnd(hWndApp)
          FindAndKillPopUpWindows
          
        'SetFocusAPI (hWndApp)
          
          
        Loop
        'MsgBox idproc & " is the idproc, ds not running"
        Timer1.Enabled = False
        
   
   End If

End Function
Private Function CheckUIForNulls() As String
    CheckUIForNulls = ""
    If txtAuthor.Text = "" Then
        CheckUIForNulls = CheckUIForNulls & " Author,"

    End If
    
    If txtCaseName.Text = "" Then
        CheckUIForNulls = CheckUIForNulls & " Case Name,"
        
    End If
    
    If txtMap.Text = "" Then
        CheckUIForNulls = CheckUIForNulls & " Map,"
   
    End If
    
    If txtSkrit.Text = "" Then
        CheckUIForNulls = CheckUIForNulls & " Skrit,"

    End If
    
    If txtTeleport.Text = "" Then
        CheckUIForNulls = CheckUIForNulls & " Teleport,"
    End If
    
    If rtfDescription.Text = "" Then
        CheckUIForNulls = CheckUIForNulls & " Description,"
    End If
    
    If rtfExpected.Text = "" Then
        CheckUIForNulls = CheckUIForNulls & " Expected,"
    End If
    If cmbArea.Text = "" Then
        CheckUIForNulls = CheckUIForNulls & " Area."
        End If
        
    
    If CheckUIForNulls = "" Then
    CheckUIForNulls = "NONULLS"
    
    End If
End Function
Private Function BuildCaseInfoFromUI() As Caseinfo
    Dim TempCaseinfo As Caseinfo
    
    If CheckUIForNulls <> "NONULLS" Then
        MsgBox "Please fill in all case fields, (Arguments is optional)"
        Exit Function
    End If
    
    With TempCaseinfo
        .Author = txtAuthor.Text
        .CaseName = txtCaseName.Text
        .Description = rtfDescription.Text
        .Expected = rtfExpected.Text
        .Map = txtMap.Text
        .Teleport = txtTeleport.Text
        .TestArea = cmbArea.Text
        .Skrit = txtSkrit.Text
        If txtArgs.Text > "" Then
            .Arguments = txtArgs.Text
        End If
        .SkritPath = txtSkritPath.Text
        
        
    End With
    
    BuildCaseInfoFromUI = TempCaseinfo
    
    
End Function



Private Sub txtExepath_Change()
    If CurrentUserInfo.IsDebug = True Then
        lblCurrentBuild.Caption = GetFileVersion(txtExepath.Text & "\" & DEBUG_EXE)
    Else
           lblCurrentBuild.Caption = GetFileVersion(txtExepath.Text & "\" & RELEASE_EXE)
    End If
    CurrentUserInfo.CurrentBuild = lblCurrentBuild.Caption
End Sub

Private Sub txtSQLserver_LostFocus()
    txtSQLserver.Enabled = False
End Sub

Private Sub txtSQLserver_Validate(Cancel As Boolean)
    If (StrComp(CurrentUserInfo.SQLserver, txtSQLserver.Text, vbTextCompare) <> 0) Then
        If Not modParse.SqlServerExists(txtSQLserver.Text) Then  ' only proceeds if ok : form level validation
           If modParse.SqlServerExists(CurrentUserInfo.SQLserver) Then  ' reverse the user entry
                txtSQLserver.Text = CurrentUserInfo.SQLserver
           Else
                If modParse.SqlServerExists(modDB.defSQLserver) Then
                    txtSQLserver.Text = modDB.defSQLserver
                Else
                    Cancel = True
                    txtSQLserver.SetFocus
                End If
           End If
        End If
        txtSQLserver.Refresh
    End If
End Sub
