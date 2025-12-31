// MainFrm.cpp : implementation of the CMainFrame class
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "MainFrm.h"
#include "LeftView.h"
#include "BottomView.h"
#include "SiegeEditorView.h"
#include "Preferences.h"
#include "CustomViewer.h"
#include "MenuCommands.h"
#include "EditorRegion.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#include "Splash.h"

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_ACTIVATEAPP()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_FILE_SAVE_ALL, MenuFunctions::SaveRegion)
	ON_COMMAND(ID_FILE_SAVEMAPAS, MenuFunctions::SaveMapAsDsmap)
	ON_COMMAND(ID_FILE_SAVEFOLDERAS, MenuFunctions::SaveFolderAsDsmod)
	ON_COMMAND(ID_FILE_EXTRACT_TANK, MenuFunctions::ConvertToFiles)
	ON_COMMAND(ID_FILE_OPEN, MenuFunctions::LoadRegion)
	ON_COMMAND(ID_FILE_NEW, MenuFunctions::NewRegion)
	ON_COMMAND(ID_FILE_NEWMAP, MenuFunctions::NewMap)
	ON_COMMAND(ID_FILE_CREATEBACKUPSONSAVE, MenuFunctions::CreateBackups)
	ON_COMMAND(ID_VIEW_PREFERENCES, MenuFunctions::Preferences)
	ON_COMMAND(ID_VIEW_OBJECTS, MenuFunctions::ShowObjects)
	ON_COMMAND(ID_VIEW_LIGHTS, MenuFunctions::ShowLights)
	ON_COMMAND(ID_VIEW_GIZMOS, MenuFunctions::ShowGizmos)	
	ON_COMMAND(ID_VIEW_TUNINGPOINTS, MenuFunctions::ShowTuningPoints)
	ON_COMMAND(ID_VIEW_KEYBOARDSHORTCUTS, MenuFunctions::KeyMapper)
	ON_COMMAND(ID_NODE_PAINTSELECTNODES,MenuFunctions::PaintSelectNodes)
	ON_COMMAND(ID_NODE_PROPERTIES,MenuFunctions::NodeProperties)
	ON_COMMAND(ID_FILE_STITCHBUILDER,MenuFunctions::StitchBuilder)
	ON_COMMAND(ID_VIEW_MESHPREVIEWER,MenuFunctions::MeshPreviewer)
	ON_COMMAND(ID_NODE_CURRENTNODESET,MenuFunctions::NodeSet)
	ON_COMMAND(ID_NODE_SETASTARGETNODE,MenuFunctions::SetAsTargetNode)
	ON_COMMAND(ID_NODE_SELECTALLNODES,MenuFunctions::SelectAllNodes)
	ON_COMMAND(ID_NODE_NODELIST,MenuFunctions::NodeList)
	ON_COMMAND(ID_NODE_MESHLIST,MenuFunctions::MeshList)
	ON_COMMAND(ID_NODE_AUTOCONNECTNODES,MenuFunctions::AutoConnectNodes)
	ON_COMMAND(ID_NODE_STITCHMANAGER,MenuFunctions::StitchManager)
	ON_COMMAND(ID_NODE_SELECTLASTADDED,MenuFunctions::SelectLastAdded)
	ON_COMMAND(ID_EDIT_DELETE,MenuFunctions::KeyDelete)
	ON_COMMAND(ID_OBJECT_AUTOSNAPTOGROUND,MenuFunctions::AutoSnapToGround)
	ON_COMMAND(ID_OBJECT_PLACEMENTMODE,MenuFunctions::PlacementMode)
	ON_COMMAND(ID_OBJECT_CONTENTSEARCH,MenuFunctions::ContentSearch)
	ON_COMMAND(ID_NODE_AUTOCONNECTSELECTEDNODES,MenuFunctions::AutoConnectSelectedNodes)
	ON_COMMAND(ID_KEY_FORWARD,MenuFunctions::KeyForward)
	ON_COMMAND(ID_KEY_BACKWARD,MenuFunctions::KeyBackward)
	ON_COMMAND(ID_OBJECT_CONTENTSEARCH,MenuFunctions::ContentSearch)
	ON_COMMAND(ID_ADDGOTOCUSTOMVIEW,MenuFunctions::AddGOToCustomView)
	ON_COMMAND(ID_EDIT_CUT,MenuFunctions::Cut)
	ON_COMMAND(ID_EDIT_COPY,MenuFunctions::Copy)
	ON_COMMAND(ID_EDIT_PASTE,MenuFunctions::Paste)
	ON_COMMAND(ID_EDIT_FIND,MenuFunctions::Find)
	ON_COMMAND(ID_LIGHTING_AMBIENCE,MenuFunctions::Ambience)
	ON_COMMAND(ID_LIGHTING_DRAWDIRECTIONALLIGHTS,MenuFunctions::DrawDirectionalLights)
	ON_COMMAND(ID_LIGHTING_EDITDIRECTIONALLIGHTS,MenuFunctions::EditDirectionalLights)
	ON_COMMAND(ID_LIGHTING_CALCULATEALLLIGHTS,MenuFunctions::CalculateAllLights)
	ON_COMMAND(ID_LIGHTING_UPDATEALLNODELIGHTING,MenuFunctions::UpdateAllNodeLighting)
	ON_COMMAND(ID_LIGHTING_UPDATESELECTEDNODESLIGHTING,MenuFunctions::UpdateSelectedNodeLighting)
	ON_COMMAND(ID_SETTINGS_MAP,MenuFunctions::MapSettings)	
	ON_COMMAND(ID_SETTINGS_REGION,MenuFunctions::RegionSettings)
	ON_COMMAND(ID_OBJECT_SPOTSNAPPER,MenuFunctions::SpotSnapper)
	ON_COMMAND(ID_OBJECT_VALIDATION,MenuFunctions::Validation)
	ON_COMMAND(ID_SETTINGS_BOOKMARKS,MenuFunctions::Bookmarks)
	ON_COMMAND(ID_OBJECT_PARTIES_PARTYEDITOR,MenuFunctions::PartyEditor)
	ON_COMMAND(ID_FILE_BATCHLNCGENERATION,MenuFunctions::BatchLNCGeneration)		
	ON_COMMAND(ID_VIEW_TOOLBARS_NODE,CMainFrame::ShowNodeBar)
	ON_COMMAND(ID_VIEW_TOOLBARS_OBJECTS,CMainFrame::ShowObjectBar)
	ON_COMMAND(ID_VIEW_TOOLBARS_LIGHTS,CMainFrame::ShowLightBar)
	ON_COMMAND(ID_VIEW_TOOLBARS_SEARCH,CMainFrame::ShowDlgBar)
	ON_COMMAND(ID_VIEW_TOOLBARS_VIEW,CMainFrame::ShowViewBar)
	ON_COMMAND(ID_VIEW_TOOLBARS_CAMERA,CMainFrame::ShowCameraBar)
	ON_COMMAND(ID_VIEW_TOOLBARS_PREFERENCES,CMainFrame::ShowPreferenceBar)	
	ON_COMMAND(ID_CAMERA_RESET,MenuFunctions::ResetCamera)
	ON_COMMAND(ID_CAMERA_SIDEVIEW,MenuFunctions::SideView)
	ON_COMMAND(ID_CAMERA_TOPVIEW,MenuFunctions::TopView)
	ON_COMMAND(ID_ATTACH_BUTTON,MenuFunctions::AttachNode)
	ON_COMMAND(ID_PANE_NEXT,MenuFunctions::SwitchView)	
	ON_COMMAND(ID_NODE_SHOWALLNODES,MenuFunctions::ShowAllNodes)	
	ON_COMMAND(ID_VIEW_MODE_OBJECTS,MenuFunctions::ModeObjects)
	ON_COMMAND(ID_VIEW_MODE_LIGHTS,MenuFunctions::ModeLights)
	ON_COMMAND(ID_VIEW_MODE_GIZMOS,MenuFunctions::ModeGizmos)	
	ON_COMMAND(ID_VIEW_MODE_NODE,MenuFunctions::ModeNodes)	
	ON_COMMAND(ID_VIEW_MODE_DECALS,MenuFunctions::ModeDecals)	
	ON_COMMAND(ID_VIEW_MODE_TUNINGPOINTS,MenuFunctions::ModeTuningPoints)
	ON_COMMAND(ID_VIEW_MODE_NODEPLACEMENT,MenuFunctions::ModeNodePlacement)
	ON_COMMAND(ID_VIEW_TOOLBARS_MODE,CMainFrame::ShowModeBar)
	ON_COMMAND(ID_OBJECT_SELECTIONMODE,MenuFunctions::MovementMode)
	ON_COMMAND(ID_EDIT_UNDO,MenuFunctions::Undo)
	ON_COMMAND(ID_LIGHTING_IGNOREVERTEXCOLOR,MenuFunctions::IgnoreVertexColor)
	ON_COMMAND(ID_SETTINGS_MAPHOTPOINTMANAGER,MenuFunctions::MapHotpoints)
	ON_COMMAND(ID_SETTINGS_EDITREGIONHOTPOINTS,MenuFunctions::EditRegionHotpoints)
	ON_COMMAND(ID_OBJECT_OBJECTLIST,MenuFunctions::ObjectList)
	ON_COMMAND(ID_OBJECT_HIDE,MenuFunctions::HideObject)
	ON_COMMAND(ID_OBJECT_SHOWALLOBJECTS,MenuFunctions::ShowAllObjects)
	ON_COMMAND(ID_VIEW_DEBUGHUDS,MenuFunctions::DebugHuds)
	ON_COMMAND(ID_DRAW_DEBUGHUDS,MenuFunctions::DrawDebugHuds)
	ON_COMMAND(ID_OBJECT_MODE_SELECTIONLOCKMODE,MenuFunctions::SelectionLock)
	ON_COMMAND(ID_OBJECT_MODE_PLACEMENTLOCKMODE,MenuFunctions::PlacementLock)
	ON_COMMAND(ID_NODE_HIDE,MenuFunctions::HideNodes)	
	ON_COMMAND(ID_OBJECT_PROPERTIES,MenuFunctions::ObjProperties)
	ON_COMMAND(ID_TRIGGER_PROPERTIES,MenuFunctions::TriggerProperties)
	ON_COMMAND(ID_OBJECT_EQUIPMENT,MenuFunctions::EquipmentProperties)
	ON_COMMAND(ID_OBJECT_INVENTORY,MenuFunctions::InventoryProperties)
	ON_COMMAND(ID_HITDETECTFLOORSONLY,MenuFunctions::HitDetectFloorsOnly)
	ON_COMMAND(ID_WIREFRAME,MenuFunctions::Wireframe)
	ON_COMMAND(ID_DOORDRAW,MenuFunctions::DoorDraw)
	ON_COMMAND(ID_DRAWSTITCHDOORS,MenuFunctions::DrawStitchDoors)
	ON_COMMAND(ID_FLOORDRAW,MenuFunctions::FloorDraw)
	ON_COMMAND(ID_HOTPOINT_PROPERTIES,MenuFunctions::HotpointProperties)
	ON_COMMAND(ID_STARTINGPOS_PROPERTIES,MenuFunctions::StartingPosProperties)
	ON_COMMAND(ID_STARTINGPOS_CAMERASET,MenuFunctions::SetCameraToCurrentPosition)
	ON_COMMAND(ID_OBJECT_SEQUENCEEDITOR,MenuFunctions::SequenceEditor)
	ON_COMMAND(ID_VIEW_DECALPREFERENCES,MenuFunctions::DecalPreferences)
	ON_COMMAND(ID_OBJECT_ROTATE_MODE,MenuFunctions::RotateMode)
	ON_COMMAND(ID_VIEW_CAMERACONTROLLER,MenuFunctions::CameraController)
	ON_COMMAND(ID_OBJECT_TUNINGPATHS,MenuFunctions::TuningPaths)
	ON_COMMAND(ID_SCRIPTING_SIEGEFXEDITOR,MenuFunctions::SiegeFxEditor)
	ON_COMMAND(ID_STARTINGPOS_PROPERTIES_INFO,MenuFunctions::StartingPositionInfo)
	ON_COMMAND(ID_VIEW_SELECTIONFILTERS,MenuFunctions::SelectionFilters)
	ON_COMMAND(ID_VIEW_MODE_LOGICALNODES,MenuFunctions::ModeLogicalNodes)
	ON_COMMAND(ID_NODE_LOGICALNODEPROPERTIES,MenuFunctions::LogicalNodeProperties)
	ON_COMMAND(ID_OBJECT_ROTATE_SELECTION,MenuFunctions::RotateSelection)
	ON_COMMAND(ID_OBJECT_PORTRAITS,MenuFunctions::GeneratePortraits)
	ON_COMMAND(ID_VIEW_DEBUGHUDS_ALL,MenuFunctions::DebugHudAllToggle)
	ON_COMMAND(ID_VIEW_DEBUGHUDS_ACTORS,MenuFunctions::DebugHudActorsToggle)
	ON_COMMAND(ID_VIEW_DEBUGHUDS_ITEMS,MenuFunctions::DebugHudItemsToggle)
	ON_COMMAND(ID_VIEW_DEBUGHUDS_COMMANDS,MenuFunctions::DebugHudCommandsToggle)
	ON_COMMAND(ID_VIEW_DEBUGHUDS_MISC,MenuFunctions::DebugHudMiscToggle)
	ON_COMMAND(ID_OBJECT_MOVEMENTLOCK,MenuFunctions::MovementLocking)	
	ON_COMMAND(ID_OBJECT_MOVEMENTLOCK_X,MenuFunctions::MovementLockX)	
	ON_COMMAND(ID_OBJECT_MOVEMENTLOCK_Y,MenuFunctions::MovementLockY)	
	ON_COMMAND(ID_OBJECT_MOVEMENTLOCK_Z,MenuFunctions::MovementLockZ)
	ON_COMMAND(ID_OBJECT_MOVEMENTLOCK_NONE,MenuFunctions::MovementLockNone)
	ON_COMMAND(ID_OBJECT_DRAG_COPY,MenuFunctions::ObjectDragCopy)
	ON_COMMAND(ID_OBJECT_HIDECOMPONENTS,MenuFunctions::HideComponents)
	ON_COMMAND(ID_FILE_SCIDREMAPPER,MenuFunctions::ScidRemapper)
	ON_COMMAND(ID_TUNING_BATCHPATHDATAGENERATION,MenuFunctions::BatchPathDataGeneration)
	ON_COMMAND(ID_NODE_RECALCULATEDECALS,MenuFunctions::RecalcDecals)
	ON_COMMAND(ID_MISC_SET_FOCUS_MAIN,MenuFunctions::SetFocusToMainWindow)
	ON_COMMAND(ID_OBJECT_GROUPING,MenuFunctions::ObjectGrouping)
//	ON_COMMAND(ID_OBJECT_MACROS,MenuFunctions::ObjectMacros)
	ON_COMMAND(ID_OBJECT_NEXTOBJECT,MenuFunctions::NextObject)
	ON_COMMAND(ID_NODE_NODEFADEGROUPS,MenuFunctions::NodeFadeGroups)
	ON_COMMAND(ID_SETTINGS_STARTINGGROUPS,MenuFunctions::StartingGroups)
	ON_COMMAND(ID_DRAW_LOGICAL_NODES,MenuFunctions::DrawLogicalNodes)
	ON_COMMAND(ID_DRAW_DOORS,MenuFunctions::DrawDoors)
	ON_COMMAND(ID_HIDE_NODE_CAPS,MenuFunctions::NodeCaps)
	ON_COMMAND(ID_TOOLS_CONVERSATIONEDITOR,MenuFunctions::ConversationEditor)
	ON_COMMAND(ID_TOOLS_GLOBALCONVERSATIONMANAGER,MenuFunctions::GlobalConversationEditor)
	ON_COMMAND(ID_TOOLS_QUESTCHAPTEREDITOR,MenuFunctions::QuestEditor)
//	ON_COMMAND(ID_TOOLS_AUTOSTITCHTOOL,MenuFunctions::AutoStitchTool)
	ON_COMMAND(ID_TOOLS_NODEMATCHER,MenuFunctions::NodeMatches)
	ON_COMMAND(ID_PREFERENCE_DRAW_CAMERA_TARGET,MenuFunctions::DrawCameraTarget)
	ON_COMMAND(ID_PREFERENCE_RESTRICT_CAMERA,MenuFunctions::RestrictToGameCamera)
	ON_COMMAND(ID_TOOLS_MOODEDITOR,MenuFunctions::MoodEditor)
	ON_COMMAND(ID_VIEW_GRIDSETTINGS,MenuFunctions::GridSettings)
	ON_COMMAND(ID_TOOLS_METERPOINTEDITOR,MenuFunctions::MeterPointEditor)
	ON_COMMAND(ID_VIEW_VISUALNODEPLACEMENT,MenuFunctions::VisualNodePlacement)
	
	ON_UPDATE_COMMAND_UI_RANGE(AFX_ID_VIEW_MINIMUM, AFX_ID_VIEW_MAXIMUM, OnUpdateViewStyles)
	END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_pMenuCommands = NULL;
}

CMainFrame::~CMainFrame()
{
	delete m_pMenuCommands;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	CSplashWnd::ShowSplashScreen(this);

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.CreateEx(this) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	if (!m_wndDlgBar.Create(this, IDR_MAINFRAME, 
		CBRS_ALIGN_TOP, AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}

	if ( !m_wndNodeBar.CreateEx(this)  ||
		 !m_wndNodeBar.LoadToolBar(IDR_TOOLBAR_NODE))
	{
		TRACE0("Failed to create nodebar\n");
		return -1;		// fail to create
	}
	
	if ( !m_wndObjectBar.CreateEx(this)  ||
		 !m_wndObjectBar.LoadToolBar(IDR_TOOLBAR_OBJECT))
	{
		TRACE0("Failed to create objectbar\n");
		return -1;		// fail to create
	}

	if ( !m_wndLightBar.CreateEx(this)  ||
		 !m_wndLightBar.LoadToolBar(IDR_TOOLBAR_LIGHT))
	{
		TRACE0("Failed to create lightbar\n");
		return -1;		// fail to create
	}

	if ( !m_wndViewBar.CreateEx(this)  ||
		 !m_wndViewBar.LoadToolBar(IDR_TOOLBAR_VIEW))
	{
		TRACE0("Failed to create viewbar\n");
		return -1;		// fail to create
	}

	if ( !m_wndCameraBar.CreateEx(this) ||
		 !m_wndCameraBar.LoadToolBar(IDR_TOOLBAR_CAMERA))
	{
		TRACE0("Failed to create camera bar\n");
		return -1;		// fail to create
	}

	if ( !m_wndModeBar.CreateEx(this) ||
		 !m_wndModeBar.LoadToolBar(IDR_TOOLBAR_MODE))
	{
		TRACE0("Failed to create mode bar\n");
		return -1;		// fail to create
	}

	if ( !m_wndPreferenceBar.CreateEx(this) ||
		 !m_wndPreferenceBar.LoadToolBar(IDR_TOOLBAR_PREFERENCES) )
	{
		TRACE0("Failed to create preferences bar\n" );
		return -1;
	}

	if ( !m_wndReBar.Create(this) )		
	{
		TRACE0("Failed to create rebar\n");
		return -1;      // fail to create
	}	
	
	m_wndReBar.AddBar(&m_wndToolBar); 
	m_wndReBar.AddBar(&m_wndDlgBar);
	m_band_dialogbar = 1;

	m_wndReBar.AddBar(&m_wndNodeBar);
	m_band_nodebar = 2;
	
	m_wndReBar.AddBar(&m_wndObjectBar);
	m_band_objectbar = 3;

	m_wndReBar.AddBar(&m_wndLightBar);
	m_band_lightbar = 4;

	m_wndReBar.AddBar(&m_wndViewBar);
	m_band_viewbar = 5;

	m_wndReBar.AddBar(&m_wndCameraBar);
	m_band_camerabar = 6;

	m_wndReBar.AddBar(&m_wndModeBar);
	m_band_modebar = 7;

	m_wndReBar.AddBar(&m_wndPreferenceBar);
	m_band_preferencebar = 8;

	m_wndReBar.GetReBarCtrl().ShowBand( m_band_dialogbar, FALSE);
	m_wndReBar.GetReBarCtrl().ShowBand( m_band_nodebar, TRUE );
	m_wndReBar.GetReBarCtrl().ShowBand( m_band_lightbar, TRUE );
	m_wndReBar.GetReBarCtrl().ShowBand( m_band_viewbar, TRUE );
	m_wndReBar.GetReBarCtrl().ShowBand( m_band_objectbar, TRUE );
	m_wndReBar.GetReBarCtrl().ShowBand( m_band_camerabar, TRUE );
	m_wndReBar.GetReBarCtrl().ShowBand( m_band_modebar, TRUE );
	m_wndReBar.GetReBarCtrl().ShowBand( m_band_preferencebar, TRUE );

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	int status_parts[5];
	RECT rcClient;
	GetClientRect( &rcClient );
	status_parts[0] = rcClient.right-500;
	status_parts[1] = rcClient.right-325;
	status_parts[2] = rcClient.right-225;
	status_parts[3] = rcClient.right-125;
	status_parts[4] = rcClient.right;
	m_wndStatusBar.GetStatusBarCtrl().SetParts( 5, status_parts );

	m_wndStatusBar.GetStatusBarCtrl().SetText( "Ready", 0, SBT_NOBORDERS );

	// TODO: Remove this if you don't want tool tips
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);	
	m_wndNodeBar.SetBarStyle(m_wndNodeBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndObjectBar.SetBarStyle(m_wndObjectBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndLightBar.SetBarStyle(m_wndLightBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndViewBar.SetBarStyle(m_wndViewBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndCameraBar.SetBarStyle(m_wndCameraBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndModeBar.SetBarStyle(m_wndModeBar.GetBarStyle() | 
		CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndPreferenceBar.SetBarStyle(m_wndPreferenceBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);
	
	m_pMenuCommands = new MenuCommands();
	m_pMenuCommands->Init();

	gEditor.SetStatusBar( &m_wndStatusBar );

	// CG: The following line was added by the Splash Screen component.
	CSplashWnd::ShowSplashScreen(this);
	return 0;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/,
	CCreateContext* pContext)
{
	if (!m_wndSplitter.CreateStatic(this, 1, 2))
		return FALSE;

	if (!m_wndSplitter.CreateView( 0, 0, RUNTIME_CLASS(CLeftView), CSize(230, 100), pContext))
	{
		return FALSE;
	}

	if (!m_wndSplitterConsole.CreateStatic( &m_wndSplitter, 2, 1, WS_CHILD | WS_VISIBLE | WS_BORDER, m_wndSplitter.IdFromRowCol(0,1))) {
		return FALSE;
	}	
	
	if (!m_wndSplitterConsole.CreateView( 1, 0, RUNTIME_CLASS(BottomView), CSize(100, 100), pContext)) {
		return FALSE;
	}

	if (!m_wndSplitterConsole.CreateView( 0, 0, RUNTIME_CLASS(CSiegeEditorView), CSize(100, 500), pContext)) {
		return FALSE;
	}

	return TRUE;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
//	cs.style &= ~FWS_ADDTOTITLE;

	cs.dwExStyle |= WS_EX_CONTROLPARENT;
	
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

CSiegeEditorView* CMainFrame::GetRightPane()
{
	CWnd* pWnd = m_wndSplitter.GetPane(0, 1);
	CSiegeEditorView* pView = DYNAMIC_DOWNCAST(CSiegeEditorView, pWnd);
	return pView;
}

void CMainFrame::OnUpdateViewStyles(CCmdUI* pCmdUI)
{
	// TODO: customize or extend this code to handle choices on the
	// View menu.

	CSiegeEditorView* pView = GetRightPane(); 

	// if the right-hand pane hasn't been created or isn't a view,
	// disable commands in our range

	if (pView == NULL)
		pCmdUI->Enable(FALSE);
	else
	{
		DWORD dwStyle = pView->GetStyle() & LVS_TYPEMASK;

		// if the command is ID_VIEW_LINEUP, only enable command
		// when we're in LVS_ICON or LVS_SMALLICON mode

		if (pCmdUI->m_nID == ID_VIEW_LINEUP)
		{
			if (dwStyle == LVS_ICON || dwStyle == LVS_SMALLICON)
				pCmdUI->Enable();
			else
				pCmdUI->Enable(FALSE);
		}
		else
		{
			// otherwise, use dots to reflect the style of the view
			pCmdUI->Enable();
			BOOL bChecked = FALSE;

			switch (pCmdUI->m_nID)
			{
			case ID_VIEW_DETAILS:
				bChecked = (dwStyle == LVS_REPORT);
				break;

			case ID_VIEW_SMALLICON:
				bChecked = (dwStyle == LVS_SMALLICON);
				break;

			case ID_VIEW_LARGEICON:
				bChecked = (dwStyle == LVS_ICON);
				break;
	
			case ID_VIEW_LIST:
				bChecked = (dwStyle == LVS_LIST);
				break;

			default:
				bChecked = FALSE;
				break;
			}

			pCmdUI->SetRadio(bChecked ? 1 : 0);
		}
	}
}


void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);
	
	if ( m_wndStatusBar.GetSafeHwnd() ) 
	{
		int status_parts[5];
		RECT rcClient;
		GetClientRect( &rcClient );
		status_parts[0] = rcClient.right-500;
		status_parts[1] = rcClient.right-325;
		status_parts[2] = rcClient.right-225;
		status_parts[3] = rcClient.right-125;
		status_parts[4] = rcClient.right;
		m_wndStatusBar.GetStatusBarCtrl().SetParts( 5, status_parts );
	}	
}

void CMainFrame::OnDestroy() 
{
	CFrameWnd::OnDestroy();		
	if ( CustomViewer::DoesSingletonExist() && gCustomViewer.GetViewChanged() )
	{
		int result = MessageBoxEx( GetSafeHwnd(), "Your current custom view has changed.  Do you wish to save it?", "Custom View", MB_YESNO | MB_ICONQUESTION, LANG_ENGLISH );
		if ( result == IDYES )
		{
			gCustomViewer.SaveCustomView( gLeftView.GetCustomView() );
		}
	}

	if ( Preferences::DoesSingletonExist() )
	{
		gPreferences.Save();	
	}
}

void CMainFrame::ShowDlgBar()
{
	if ( !gPreferences.GetShowDlgBar() ) 
	{
		m_wndReBar.GetReBarCtrl().ShowBand( m_band_dialogbar, TRUE );
		gPreferences.SetShowDlgBar( true );
		GetMenu()->CheckMenuItem( ID_VIEW_TOOLBARS_SEARCH, MF_CHECKED );
	} 
	else 
	{
		m_wndReBar.GetReBarCtrl().ShowBand( m_band_dialogbar, FALSE );
		gPreferences.SetShowDlgBar( false );
		GetMenu()->CheckMenuItem( ID_VIEW_TOOLBARS_SEARCH, MF_UNCHECKED );
	}
}


void CMainFrame::ShowViewBar()
{
	if ( !gPreferences.GetShowViewBar() ) 
	{
		m_wndReBar.GetReBarCtrl().ShowBand( m_band_viewbar, TRUE );
		gPreferences.SetShowViewBar( true );
		GetMenu()->CheckMenuItem( ID_VIEW_TOOLBARS_VIEW, MF_CHECKED );
	} 
	else 
	{		
		m_wndReBar.GetReBarCtrl().ShowBand( m_band_viewbar, FALSE );
		gPreferences.SetShowViewBar( false );
		GetMenu()->CheckMenuItem( ID_VIEW_TOOLBARS_VIEW, MF_UNCHECKED );
	}
}


void CMainFrame::ShowModeBar()
{
	if ( !gPreferences.GetShowModeBar() ) 
	{
		m_wndReBar.GetReBarCtrl().ShowBand( m_band_modebar, TRUE );
		gPreferences.SetShowModeBar( true );
		GetMenu()->CheckMenuItem( ID_VIEW_TOOLBARS_MODE, MF_CHECKED );
	} 
	else 
	{		
		m_wndReBar.GetReBarCtrl().ShowBand( m_band_modebar, FALSE );
		gPreferences.SetShowModeBar( false );
		GetMenu()->CheckMenuItem( ID_VIEW_TOOLBARS_MODE, MF_UNCHECKED );
	}
}

void CMainFrame::ShowNodeBar()
{
	if ( !gPreferences.GetShowNodeBar() ) 
	{
		m_wndReBar.GetReBarCtrl().ShowBand( m_band_nodebar, TRUE );
		gPreferences.SetShowNodeBar( true );
		GetMenu()->CheckMenuItem( ID_VIEW_TOOLBARS_NODE, MF_CHECKED );
	} 
	else 
	{		
		m_wndReBar.GetReBarCtrl().ShowBand( m_band_nodebar, FALSE );
		gPreferences.SetShowNodeBar( false );
		GetMenu()->CheckMenuItem( ID_VIEW_TOOLBARS_NODE, MF_UNCHECKED );
	}
}


void CMainFrame::ShowObjectBar()
{
	if ( !gPreferences.GetShowObjectBar() ) 
	{
		m_wndReBar.GetReBarCtrl().ShowBand( m_band_objectbar, TRUE );
		gPreferences.SetShowObjectBar( true );
		GetMenu()->CheckMenuItem( ID_VIEW_TOOLBARS_OBJECTS, MF_CHECKED );
	} 
	else 
	{		
		m_wndReBar.GetReBarCtrl().ShowBand( m_band_objectbar, FALSE );
		gPreferences.SetShowObjectBar( false );
		GetMenu()->CheckMenuItem( ID_VIEW_TOOLBARS_OBJECTS, MF_UNCHECKED );
	}
}


void CMainFrame::ShowLightBar()
{
	if ( !gPreferences.GetShowLightBar() )
	{
		m_wndReBar.GetReBarCtrl().ShowBand( m_band_lightbar, TRUE );
		gPreferences.SetShowLightBar( true );
		GetMenu()->CheckMenuItem( ID_VIEW_TOOLBARS_LIGHTS, MF_CHECKED );
	} 
	else
	{
		m_wndReBar.GetReBarCtrl().ShowBand( m_band_lightbar, FALSE );
		gPreferences.SetShowLightBar( false );
		GetMenu()->CheckMenuItem( ID_VIEW_TOOLBARS_LIGHTS, MF_UNCHECKED );
	}
}


void CMainFrame::ShowCameraBar()
{
	if ( !gPreferences.GetShowCameraBar() ) 
	{
		m_wndReBar.GetReBarCtrl().ShowBand( m_band_camerabar, TRUE );
		gPreferences.SetShowCameraBar( true );
		GetMenu()->CheckMenuItem( ID_VIEW_TOOLBARS_CAMERA, MF_CHECKED );
	} 
	else 
	{
		m_wndReBar.GetReBarCtrl().ShowBand( m_band_camerabar, FALSE );
		gPreferences.SetShowCameraBar( false );
		GetMenu()->CheckMenuItem( ID_VIEW_TOOLBARS_CAMERA, MF_UNCHECKED );
	}
}


void CMainFrame::ShowPreferenceBar()
{
	if ( !gPreferences.GetShowPreferenceBar() ) 
	{
		m_wndReBar.GetReBarCtrl().ShowBand( m_band_preferencebar, TRUE );
		gPreferences.SetShowPreferenceBar( true );
		GetMenu()->CheckMenuItem( ID_VIEW_TOOLBARS_PREFERENCES, MF_CHECKED );
	} 
	else 
	{
		m_wndReBar.GetReBarCtrl().ShowBand( m_band_preferencebar, FALSE );
		gPreferences.SetShowPreferenceBar( false );
		GetMenu()->CheckMenuItem( ID_VIEW_TOOLBARS_PREFERENCES, MF_UNCHECKED );
	}
}


LRESULT CMainFrame::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	switch ( message ) {
	case WM_MENUSELECT:
		{
			// Since MFC wasn't calling my OnMenuSelect, I just used the window procedure to handle the message.  So there!
			CString sString;
			sString.LoadString( LOWORD(wParam) );
			m_wndStatusBar.GetStatusBarCtrl().SetText( sString.GetBuffer( sString.GetLength() ), 0, SBT_NOBORDERS );
		}
		break;
	}
	return CFrameWnd::WindowProc(message, wParam, lParam);
}

void CMainFrame::OnActivateApp(BOOL bActive, HTASK hTask) 
{
	CFrameWnd::OnActivateApp(bActive, hTask);
	
	if ( Singleton <SiegeEditorShell>::DoesSingletonExist() ) {
		gEditor.AppActivate( !!bActive );
	}
}

void CMainFrame::OnClose() 
{
	if ( EditorRegion::DoesSingletonExist() && gEditorRegion.GetRegionEnable() )
	{
		FuelHandle hRegion = gEditorRegion.GetRegionHandle();		
		if ( hRegion.IsValid() && gEditorRegion.GetRegionDirty() )
		{
			int result = MessageBoxEx( GetSafeHwnd(), "Do you wish to save the currently loaded region before exiting?", "Save Region", MB_YESNOCANCEL | MB_ICONQUESTION, LANG_ENGLISH );
			if ( result == IDYES )
			{
				MenuFunctions mf;
				mf.SaveRegion();
			}
			else if ( result == IDCANCEL )
			{
				return;
			}			
		}
	}
	
	CFrameWnd::OnClose();
}
