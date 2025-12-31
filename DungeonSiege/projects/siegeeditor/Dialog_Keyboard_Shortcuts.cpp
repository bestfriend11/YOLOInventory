// Dialog_Keyboard_Shortcuts.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Keyboard_Shortcuts.h"
#include "KeyMapper.h"
#include "stdhelp.h"
#include "keys.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Keyboard_Shortcuts dialog


Dialog_Keyboard_Shortcuts::Dialog_Keyboard_Shortcuts(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Keyboard_Shortcuts::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Keyboard_Shortcuts)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Keyboard_Shortcuts::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Keyboard_Shortcuts)
	DDX_Control(pDX, IDC_LIST_COMMANDS, m_list_commands);
	DDX_Control(pDX, IDC_COMBO_KEY, m_combo_key);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Keyboard_Shortcuts, CDialog)
	//{{AFX_MSG_MAP(Dialog_Keyboard_Shortcuts)
	ON_BN_CLICKED(IDC_ASSIGN, OnAssign)
	ON_LBN_SELCHANGE(IDC_LIST_COMMANDS, OnSelchangeListCommands)
	ON_BN_CLICKED(IDC_BUTTON_RESET, OnButtonReset)
	ON_NOTIFY(NM_CLICK, IDC_LIST_COMMANDS, OnClickListCommands)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, OnButtonClear)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

using namespace Keys;

struct StringToKeyEntry
{
	const char* m_Name;
	KEY         m_Key;
};

bool operator < ( const char* l, const StringToKeyEntry& r )
	{  return ( ::stricmp( l, r.m_Name ) < 0 );  }
bool operator < ( const StringToKeyEntry& l, const char* r )
	{  return ( ::stricmp( l.m_Name, r ) < 0 );  }
bool operator < ( const StringToKeyEntry& l, const StringToKeyEntry& r )
	{  return ( ::stricmp( l.m_Name, r.m_Name ) < 0 );  }

static StringToKeyEntry s_StringToKeyMap[] =
{
	{ "space",               KEY_SPACE               },
	{ "pageup",              KEY_PAGEUP              },
	{ "pagedown",            KEY_PAGEDOWN            },
	{ "end",                 KEY_END                 },
	{ "home",                KEY_HOME                },
	{ "left",                KEY_LEFT                },
	{ "up",                  KEY_UP                  },
	{ "right",               KEY_RIGHT               },
	{ "down",                KEY_DOWN                },
	{ "select",              KEY_SELECT              },
	{ "print",               KEY_PRINT               },
	{ "printscreen",         KEY_PRINTSCREEN         },
	{ "insert",              KEY_INSERT              },
	{ "delete",              KEY_DELETE              },
	{ "help",                KEY_HELP                },
	{ "0",                   KEY_0                   },
	{ "1",                   KEY_1                   },
	{ "2",                   KEY_2                   },
	{ "3",                   KEY_3                   },
	{ "4",                   KEY_4                   },
	{ "5",                   KEY_5                   },
	{ "6",                   KEY_6                   },
	{ "7",                   KEY_7                   },
	{ "8",                   KEY_8                   },
	{ "9",                   KEY_9                   },
	{ "a",                   KEY_A                   },
	{ "b",                   KEY_B                   },
	{ "c",                   KEY_C                   },
	{ "d",                   KEY_D                   },
	{ "e",                   KEY_E                   },
	{ "f",                   KEY_F                   },
	{ "g",                   KEY_G                   },
	{ "h",                   KEY_H                   },
	{ "i",                   KEY_I                   },
	{ "j",                   KEY_J                   },
	{ "k",                   KEY_K                   },
	{ "l",                   KEY_L                   },
	{ "m",                   KEY_M                   },
	{ "n",                   KEY_N                   },
	{ "o",                   KEY_O                   },
	{ "p",                   KEY_P                   },
	{ "q",                   KEY_Q                   },
	{ "r",                   KEY_R                   },
	{ "s",                   KEY_S                   },
	{ "t",                   KEY_T                   },
	{ "u",                   KEY_U                   },
	{ "v",                   KEY_V                   },
	{ "w",                   KEY_W                   },
	{ "x",                   KEY_X                   },
	{ "y",                   KEY_Y                   },
	{ "z",                   KEY_Z                   },
	{ "numpad0",             KEY_NUMPAD0             },
	{ "numpad1",             KEY_NUMPAD1             },
	{ "numpad2",             KEY_NUMPAD2             },
	{ "numpad3",             KEY_NUMPAD3             },
	{ "numpad4",             KEY_NUMPAD4             },
	{ "numpad5",             KEY_NUMPAD5             },
	{ "numpad6",             KEY_NUMPAD6             },
	{ "numpad7",             KEY_NUMPAD7             },
	{ "numpad8",             KEY_NUMPAD8             },
	{ "numpad9",             KEY_NUMPAD9             },
	{ "multiply",            KEY_MULTIPLY            },
	{ "add",                 KEY_ADD                 },
	{ "separator",           KEY_SEPARATOR           },
	{ "subtract",            KEY_SUBTRACT            },
	{ "decimal",             KEY_DECIMAL             },
	{ "divide",              KEY_DIVIDE              },
	{ "f1",                  KEY_F1                  },
	{ "f2",                  KEY_F2                  },
	{ "f3",                  KEY_F3                  },
	{ "f4",                  KEY_F4                  },
	{ "f5",                  KEY_F5                  },
	{ "f6",                  KEY_F6                  },
	{ "f7",                  KEY_F7                  },
	{ "f8",                  KEY_F8                  },
	{ "f9",                  KEY_F9                  },
	{ "f10",                 KEY_F10                 },
	{ "f11",                 KEY_F11                 },
	{ "f12",                 KEY_F12                 },
	{ "f13",                 KEY_F13                 },
	{ "f14",                 KEY_F14                 },
	{ "f15",                 KEY_F15                 },
	{ "f16",                 KEY_F16                 },
	{ "f17",                 KEY_F17                 },
	{ "f18",                 KEY_F18                 },
	{ "f19",                 KEY_F19                 },
	{ "f20",                 KEY_F20                 },
	{ "f21",                 KEY_F21                 },
	{ "f22",                 KEY_F22                 },
	{ "f23",                 KEY_F23                 },
	{ "f24",                 KEY_F24                 },
//	{ "numlock",             KEY_NUMLOCK             },
	{ "scrolllock",          KEY_SCROLLLOCK          },
};


// This map tracks string command names to their command values
struct CommandName
{
	const char* sName;
	UINT cmd;
};


bool operator < ( const CommandName& l, const gpstring& r )  {  return ( strcmp( l.sName, r.c_str()	) < 0 );  }
bool operator < ( const gpstring& l, const CommandName& r )  {  return ( strcmp( l.c_str(), r.sName	) < 0 );  }
bool operator < ( const CommandName& l, const CommandName& r )  {  return ( strcmp( l.sName, r.sName	) < 0 );  }

static CommandName stCommandMap[] = 
{
	{ "File - New Map",													ID_FILE_NEWMAP									},
	{ "Edit - Delete",													ID_EDIT_DELETE									},
	{ "View - Preferences",												ID_VIEW_PREFERENCES								},
	{ "View - Objects",													ID_VIEW_OBJECTS									},
	{ "View - Lights",													ID_VIEW_LIGHTS									},
	{ "View - Gizmos",													ID_VIEW_TRIGGERS								},	
	{ "View - Tuning Points",											ID_VIEW_TUNINGPOINTS							},	
	{ "Mode - Objects",													ID_VIEW_MODE_OBJECTS							},
	{ "Mode - Lights",													ID_VIEW_MODE_LIGHTS								},
	{ "Mode - Gizmos",													ID_VIEW_MODE_GIZMOS								},	
	{ "Mode - Nodes",													ID_VIEW_MODE_NODE								},
	{ "Mode - Decals",													ID_VIEW_MODE_DECALS								},
	{ "Mode - Logical Node",											ID_VIEW_MODE_LOGICALNODES						},
	{ "Mode - Tuning Points",											ID_VIEW_MODE_TUNINGPOINTS						},	
	{ "Mode - Node Placement",											ID_VIEW_MODE_NODEPLACEMENT						},
	{ "Keys - Left",													ID_KEY_LEFT										},
	{ "Keys - Up",														ID_KEY_UP										},
	{ "Keys - Right",													ID_KEY_RIGHT									},
	{ "Keys - Down",													ID_KEY_DOWN										},
	{ "Nodes - Mesh Previewer",											ID_VIEW_MESHPREVIEWER							},
	{ "Nodes - Properties",												ID_NODE_PROPERTIES								},
	{ "Nodes - Current Node Set",										ID_NODE_CURRENTNODESET							},
	{ "Nodes - Set as Target Node",										ID_NODE_SETASTARGETNODE							},
	{ "Nodes - Select All Nodes",										ID_NODE_SELECTALLNODES							},
	{ "Nodes - Node List",												ID_NODE_NODELIST								},
	{ "Nodes - Mesh List",												ID_NODE_MESHLIST								},
	{ "Nodes - Auto Node Connection",									ID_NODE_AUTOCONNECTNODES						},
	{ "Nodes - Paint Select Nodes",										ID_NODE_PAINTSELECTNODES						},
	{ "Nodes - Select Last Added",										ID_NODE_SELECTLASTADDED							},
	{ "Nodes - Auto Connect Selected Nodes",							ID_NODE_AUTOCONNECTSELECTEDNODES				},
	{ "Nodes - Attach Node",											ID_ATTACH_BUTTON								},
	{ "Nodes - Show All Nodes",											ID_NODE_SHOWALLNODES							},
	{ "Nodes - Hide Node(s)",											ID_NODE_HIDE									},
	{ "Nodes - Hit Detect Floor Only",									ID_HITDETECTFLOORSONLY							},
	{ "Nodes - Draw Doors",												ID_DOORDRAW										},
	{ "Nodes - Draw Stitch Doors",										ID_DRAWSTITCHDOORS								},
	{ "Nodes - Copy Guid to Clipboard",									ID_GRAB_GUID									},
	{ "Nodes - Logical Node Properties",								ID_NODE_LOGICALNODEPROPERTIES					},
	{ "Nodes - Floor Draw",												ID_FLOORDRAW									},	
	{ "Objects - Inventory",											ID_OBJECT_INVENTORY								},
	{ "Objects - Equipment",											ID_OBJECT_EQUIPMENT								},
	{ "Objects - Properties",											ID_OBJECT_PROPERTIES							},
	{ "Objects - Auto Snap Objects to Ground",							ID_OBJECT_AUTOSNAPTOGROUND						},	
	{ "Objects - Party Editor",											ID_OBJECT_PARTIES_PARTYEDITOR					},
	{ "Objects - Content Search",										ID_OBJECT_CONTENTSEARCH							},
	{ "Objects - Movement Mode",										ID_OBJECT_SELECTIONMODE							},
	{ "Objects - Placement Mode",										ID_OBJECT_PLACEMENTMODE							},	
	{ "Objects - Hide Selected Object(s)",								ID_OBJECT_HIDE									},
	{ "Objects - Show All Objects",										ID_OBJECT_SHOWALLOBJECTS						},
	{ "Objects - Movement Lock X",										ID_OBJECT_MOVEMENTLOCK_X						},
	{ "Objects - Movement Lock Y",										ID_OBJECT_MOVEMENTLOCK_Y						},
	{ "Objects - Movement Lock Z",										ID_OBJECT_MOVEMENTLOCK_Z						},
	{ "Objects - Movement Lock None",									ID_OBJECT_MOVEMENTLOCK_NONE						},
	{ "Objects - Drag Copy",											ID_OBJECT_DRAG_COPY								},
	{ "Objects - Copy Scid to Clipboard",								ID_GRAB_SCID									},
	{ "Objects - Rotate Selection",										ID_OBJECT_ROTATE_SELECTION						},
	{ "Lighting - Update All Directional Lights",						ID_LIGHTING_UPDATEALLNODELIGHTING				},
	{ "Lighting - Ambience",											ID_LIGHTING_AMBIENCE							},
	{ "Lighting - Edit Directional Lights",								ID_LIGHTING_EDITDIRECTIONALLIGHTS				},
	{ "Lighting - Draw Directional Lights",								ID_LIGHTING_DRAWDIRECTIONALLIGHTS				},
	{ "Lighting - Update Selected Node Lighting",						ID_LIGHTING_UPDATESELECTEDNODESLIGHTING			},	
	{ "Lighting - Ignore Vertex Color",									ID_LIGHTING_IGNOREVERTEXCOLOR					},
	{ "Lighting - Copy Light GUID",										ID_GRAB_LIGHT_GUID								},
	{ "Skrit - Editor",													ID_SCRIPTING_SKRITEDITOR						},
	{ "Skrit - Console",												ID_SCRIPTING_SKRITCONSOLE						},
	{ "Settings - Map",													ID_SETTINGS_MAP									},
	{ "Settings - Region",												ID_SETTINGS_REGION								},
	{ "Settings - Keyboard Shortcuts",									ID_VIEW_KEYBOARDSHORTCUTS						},
	{ "Settings - Bookmark Editor",										ID_SETTINGS_BOOKMARKS							},
	{ "Settings - Starting Positions",									ID_SETTINGS_STARTINGPOSITIONS					},
	{ "Settings - Toggle Debug HUDs",									ID_DRAW_DEBUGHUDS								},
	{ "Stitching - Manager",											ID_NODE_STITCHMANAGER							},
	{ "Stitching - Builder",											ID_FILE_STITCHBUILDER							},
	{ "File - Create GAS Backups on Save",								ID_FILE_CREATEBACKUPSONSAVE						},
	{ "File - Batch LNC Generation",									ID_FILE_BATCHLNCGENERATION						},
	{ "View - Switch to next Window Pane",								ID_PANE_NEXT									},	
	{ "View - Preference - Wireframe",									ID_WIREFRAME									},
	{ "View - Preference - View Camera Target",							ID_PREFERENCE_DRAW_CAMERA_TARGET				},
	{ "View - Preference - Restrict To Game Camera",					ID_PREFERENCE_RESTRICT_CAMERA					},
	{ "File - Save",													ID_FILE_SAVE_ALL								},
	{ "View - Debug HUD Dialog",										ID_VIEW_DEBUGHUDS								},
	{ "View - Toggle All Debug HUD",									ID_VIEW_DEBUGHUDS_ALL							},
	{ "View - Toggle Actor Debug HUD",									ID_VIEW_DEBUGHUDS_ACTORS						},
	{ "View - Toggle Items Debug HUD",									ID_VIEW_DEBUGHUDS_ITEMS							},
	{ "View - Toggle Commands Debug HUD",								ID_VIEW_DEBUGHUDS_COMMANDS						},
	{ "View - Toggle Misc Debug HUD",									ID_VIEW_DEBUGHUDS_MISC							},	
	{ "Tools - Auto-Stitch Tool",										ID_TOOLS_AUTOSTITCHTOOL							},
};


/////////////////////////////////////////////////////////////////////////////
// Dialog_Keyboard_Shortcuts message handlers

void Dialog_Keyboard_Shortcuts::OnOK() 
{
	gKeyMapper.SaveKeyMappings();	
	CDialog::OnOK();
}

void Dialog_Keyboard_Shortcuts::OnAssign() 
{
	ACCEL accel;

	int list_index = m_list_commands.GetSelectionMark();
	if ( list_index == LB_ERR ) 
	{
		return;
	}

	CString rListText;
	rListText = m_list_commands.GetItemText( list_index, 0 );
	
	// make sure the list is sorted
	static bool sSorted = false;
	if ( !sSorted )
	{
		std::sort( stCommandMap, ARRAY_END( stCommandMap ) );
		sSorted = true;
	}
	
	const CommandName* found = stdx::binary_search(
			stCommandMap,
			ARRAY_END( stCommandMap ),
			rListText.GetBuffer( rListText.GetLength() ) );
	if ( found != ARRAY_END( stCommandMap ) )
	{
		accel.cmd = found->cmd;
		
		if ( ((CButton *)(GetDlgItem( IDC_RADIO_ALT )))->GetCheck() ) {
			accel.fVirt = FALT;
		}
		else if ( ((CButton *)(GetDlgItem( IDC_RADIO_CONTROL )))->GetCheck() ) {
			accel.fVirt = FCONTROL;
		}
		else if ( ((CButton *)(GetDlgItem( IDC_RADIO_SHIFT )))->GetCheck() ) {			
			accel.fVirt = FSHIFT;
		}
		else {
			accel.fVirt = 0;
		}

		int sel = m_combo_key.GetCurSel();
		if ( sel == CB_ERR )
		{
			return;
		}

		CString rEditText;
		m_combo_key.GetLBText( sel, rEditText );		
		
		if ( rEditText == "" ) 
		{
			gKeyMapper.RemoveKeyAssignment( accel.cmd );
		}
		else {
			//char cKey = rEditText.GetAt( 0 );
			Keys::KEY key;
			
			gpstring sKey = "key_";
			sKey += rEditText.GetBuffer( 0 );
			Keys::FromString( sKey.c_str(), key );
			accel.key = (int)key;
			accel.fVirt |= FVIRTKEY;
			
			gKeyMapper.InsertKeyAssignment( accel );
		}
	}

	Refresh();

	return;	
}


void Dialog_Keyboard_Shortcuts::OnSelchangeListCommands() 
{	
}

BOOL Dialog_Keyboard_Shortcuts::OnInitDialog() 
{
	CDialog::OnInitDialog();	

	int size = 92;
	for ( int j = 0; j != size; ++j )
	{
		gpstring sTest = s_StringToKeyMap[j].m_Name;
		m_combo_key.AddString( s_StringToKeyMap[j].m_Name );
	}

	m_list_commands.InsertColumn( 0, "Command", LVCFMT_LEFT, 220 );
	m_list_commands.InsertColumn( 1, "Key", LVCFMT_LEFT, 100 );
		
	for ( int i = 0; i != (sizeof( stCommandMap )/sizeof( CommandName )); ++i ) 
	{
		int item = m_list_commands.InsertItem( 0, stCommandMap[i].sName, 0 );
		m_list_commands.SetItemData( item, stCommandMap[i].cmd );
	}

	Refresh();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void Dialog_Keyboard_Shortcuts::Refresh()
{	
	for ( int i = 0; i != (sizeof( stCommandMap )/sizeof( CommandName )); ++i ) 
	{
		ACCEL accel = gKeyMapper.GetKeyAssignment( stCommandMap[i].cmd );

		LVFINDINFO info;
		info.flags	= LVFI_STRING;
		info.psz	= stCommandMap[i].sName;
		int item = m_list_commands.FindItem( &info );
				
		gpstring sKey = Keys::ToString( (KEY)accel.key );
		sKey = sKey.substr( 4, sKey.size() );	
		if (( accel.fVirt == FALT ) || ( accel.fVirt == (FALT | FVIRTKEY) )) 
		{
			sKey.append( " - Alt" );			
		}
		else if (( accel.fVirt == FCONTROL ) || ( accel.fVirt == (FCONTROL | FVIRTKEY) )) {
			sKey.append( " - Control" );			
		}
		else if (( accel.fVirt == FSHIFT ) || ( accel.fVirt == (FSHIFT | FVIRTKEY) )) {
			sKey.append( " - Shift" );			
		}
		
		m_list_commands.SetItemText( item, 1, sKey.c_str() );
	}
}


gpstring GetCommandName( int cmd )
{
	for ( int i = 0; i != (sizeof( stCommandMap )/sizeof( CommandName )); ++i ) 
	{
		if ( stCommandMap[i].cmd == cmd )
		{
			return stCommandMap[i].sName;
		}
	}

	return "";
}

void Dialog_Keyboard_Shortcuts::OnButtonReset() 
{
	int result = MessageBoxEx( gEditor.GetHWnd(), "You are about to delete all current mappings, continue?", "Key Mapping", MB_YESNO | MB_ICONQUESTION, LANG_ENGLISH );
	if ( result == IDYES )
	{
		gKeyMapper.ResetMappings();
		Refresh();
	}	
}

void Dialog_Keyboard_Shortcuts::OnClickListCommands(NMHDR* pNMHDR, LRESULT* pResult) 
{
	m_combo_key.SetCurSel( -1 );
	((CButton *)(GetDlgItem( IDC_RADIO_NONE )))->SetCheck( 1 );
	((CButton *)(GetDlgItem( IDC_RADIO_ALT )))->SetCheck( 0 );			
	((CButton *)(GetDlgItem( IDC_RADIO_CONTROL )))->SetCheck( 0 );
	((CButton *)(GetDlgItem( IDC_RADIO_SHIFT )))->SetCheck( 0 );

	int list_index = m_list_commands.GetSelectionMark();
	if ( list_index == LB_ERR ) 
	{
		return;
	}

	CString rListText;
	rListText = m_list_commands.GetItemText( list_index, 0 );
	
	static bool sSorted = false;
	if ( !sSorted )
	{
		std::sort( stCommandMap, ARRAY_END( stCommandMap ) );
		sSorted = true;
	}
	
	const CommandName* found = stdx::binary_search(
			stCommandMap,
			ARRAY_END( stCommandMap ),
			rListText.GetBuffer( rListText.GetLength() ) );
	if ( found != ARRAY_END( stCommandMap ) )
	{
		ACCEL accel = gKeyMapper.GetKeyAssignment( found->cmd );
		if ( accel.cmd == 0 && accel.fVirt == 0 && accel.key == 0 ) {
			return;
		}

		
		gpstring sKey = Keys::ToString( (KEY)accel.key );
		sKey = sKey.substr( 4, sKey.size() );
		int found = m_combo_key.FindStringExact( 0, sKey.c_str() );
		if ( found != CB_ERR )
		{
			m_combo_key.SetCurSel( found );
		}
		if (( accel.fVirt == FALT ) || ( accel.fVirt == (FALT | FVIRTKEY) )) {
			((CButton *)(GetDlgItem( IDC_RADIO_ALT )))->SetCheck( 1 );
			((CButton *)(GetDlgItem( IDC_RADIO_NONE )))->SetCheck( 0 );
			((CButton *)(GetDlgItem( IDC_RADIO_CONTROL )))->SetCheck( 0 );
			((CButton *)(GetDlgItem( IDC_RADIO_SHIFT )))->SetCheck( 0 );
		}
		else if (( accel.fVirt == FCONTROL ) || ( accel.fVirt == (FCONTROL | FVIRTKEY) )) {
			((CButton *)(GetDlgItem( IDC_RADIO_CONTROL )))->SetCheck( 1 );
			((CButton *)(GetDlgItem( IDC_RADIO_NONE )))->SetCheck( 0 );
			((CButton *)(GetDlgItem( IDC_RADIO_ALT )))->SetCheck( 0 );						
			((CButton *)(GetDlgItem( IDC_RADIO_SHIFT )))->SetCheck( 0 );
		}
		else if (( accel.fVirt == FSHIFT ) || ( accel.fVirt == (FSHIFT | FVIRTKEY) )) {
			((CButton *)(GetDlgItem( IDC_RADIO_SHIFT )))->SetCheck( 1 );
			((CButton *)(GetDlgItem( IDC_RADIO_NONE )))->SetCheck( 0 );
			((CButton *)(GetDlgItem( IDC_RADIO_ALT )))->SetCheck( 0 );			
			((CButton *)(GetDlgItem( IDC_RADIO_CONTROL )))->SetCheck( 0 );			
		}
		else {
			((CButton *)(GetDlgItem( IDC_RADIO_NONE )))->SetCheck( 1 );
			((CButton *)(GetDlgItem( IDC_RADIO_ALT )))->SetCheck( 0 );			
			((CButton *)(GetDlgItem( IDC_RADIO_CONTROL )))->SetCheck( 0 );
			((CButton *)(GetDlgItem( IDC_RADIO_SHIFT )))->SetCheck( 0 );
		}
	}	
	
	*pResult = 0;
}

void Dialog_Keyboard_Shortcuts::OnButtonClear() 
{
	int list_index = m_list_commands.GetSelectionMark();
	if ( list_index == LB_ERR ) 
	{
		return;
	}

	CString rListText;
	rListText = m_list_commands.GetItemText( list_index, 0 );

	// make sure the list is sorted
	static bool sSorted = false;
	if ( !sSorted )
	{
		std::sort( stCommandMap, ARRAY_END( stCommandMap ) );
		sSorted = true;
	}
	
	const CommandName* found = stdx::binary_search(
			stCommandMap,
			ARRAY_END( stCommandMap ),
			rListText.GetBuffer( rListText.GetLength() ) );
	
	gKeyMapper.RemoveKeyAssignment( found->cmd );		
	Refresh();
}

void Dialog_Keyboard_Shortcuts::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
