// LeftView.h : interface of the CLeftView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_LEFTVIEW_H__A8C8446E_14A4_4E47_8277_025818E19122__INCLUDED_)
#define AFX_LEFTVIEW_H__A8C8446E_14A4_4E47_8277_025818E19122__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// Child Window Defines
#define WORKSPACE_TAB	311
#define TREE_MAIN		312
#define TREE_CUSTOM		313
#define SOURCE_SPINNER	314
#define DEST_SPINNER	315
#define SOURCE_COMBO	316
#define DEST_COMBO		317
#define ATTACH_BUTTON	318


// Include Files
#include "gpcore.h"
#include <string>
#include <map>

// Structures
struct DIRECTORY_FOLDER 
{
	gpstring text;
	HTREEITEM	hItem;
};


class CSiegeEditorDoc;
class GoDataTemplate;


class CLeftView : public CView, public Singleton <CLeftView>
{
protected: // create from serialization only
	CLeftView();
	DECLARE_DYNCREATE(CLeftView)

private:

	CTabCtrl			m_wndTabs;
	CTreeCtrl			m_wndTree1;
	CTreeCtrl			m_wndTree2;
	CSpinButtonCtrl		m_wndSourceSpin;
	CSpinButtonCtrl		m_wndDestSpin;
	CComboBox			m_wndSourceCombo;
	CComboBox			m_wndDestCombo;
	CStatic				m_wndStaticSource;
	CStatic				m_wndStaticDest;
	CButton				m_wndAttachButton;
	CMenu				m_wndMainMenu;
	CMenu				m_wndCustomMenu;
	int					m_destNum;
	int					m_sourceNum;
	HTREEITEM			m_hHitItem;
	HTREEITEM			m_hDragItem;
	HTREEITEM			m_hTarget;
	bool				m_bItemDragging;
	CImageList *		m_pDragImageList;
	CToolTipCtrl		m_ToolTips;
	HTREEITEM			m_selectedCustomItem;

// Attributes
public:
	CSiegeEditorDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLeftView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL DestroyWindow();
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CLeftView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void InitializeMainTreeData();
	void InitializeCustomTreeData();
	CTreeCtrl & GetCustomView() { return m_wndTree2; }
	CTreeCtrl & GetMainView() { return m_wndTree1; }
	CTabCtrl & GetTabCtrl() { return m_wndTabs; }
	
	void SwitchContentDisplay();
	void SwitchContentDisplayChildren( CTreeCtrl & wndTree, HTREEITEM hItem );

	void PublishNodeList();
	void PublishObjectList();
	void PublishViewSearchObjectList( HTREEITEM hItem );
	void PublishNonParentObjectList();
	void PublishSourceDoorList();
	void PublishDestDoorList();
	void PublishDestDoorList( const gpstring & sSno );
	void SetDestDoor( int door );
	int	 GetNumSourceDoors()		{ return m_sourceNum;	};
	int	 GetNumDestDoors()			{ return m_destNum;		};
	void RequestAttach( char * szSource, char * szDest );
	void CreatePopupMenus();
	bool CanMoveFolderToFolder( HTREEITEM hSource, HTREEITEM & hDest );
	HTREEITEM GetSelectedCustomItem() { return m_selectedCustomItem; }
	bool RetrieveCustomFolder( HTREEITEM & hDest );

	void RotateSourceDoorLeft();
	void RotateSourceDoorRight();
	void RotateDestDoorUp();
	void RotateDestDoorDown();

	// Gets the active tree hwnd
	HWND GetTreeHWnd();
	void OnAttach();

	HTREEITEM CheckParentDirectory( gpstring item_name, HWND hWndTree, HTREEITEM hTop );

	void RecursiveInsertTemplate( GoDataTemplate * pTemplate );
	void RecursiveInsertTemplateToTree( GoDataTemplate * pTemplate, HTREEITEM hParent );

	HTREEITEM FindItemByName( gpstring sName );

	bool GetLoadedNodeMeshTree() { return m_bLoadedNodeMeshTree; }
	void SetLoadedNodeMeshTree( bool bSet ) { m_bLoadedNodeMeshTree = bSet; }

	bool GetLoadedNodeGlobalTree() { return m_bLoadedNodeGlobalTree; }
	void SetLoadedNodeGlobalTree( bool bSet ) { m_bLoadedNodeGlobalTree = bSet; }

	void InitializeNodeList( bool bForce = false );
	void PublishGlobalNodeList();
	void PublishMeshNodeList();

protected:

	std::map< gpstring, HTREEITEM, istring_less > m_loader_map;

	bool m_bLoadedNodeMeshTree;
	bool m_bLoadedNodeGlobalTree;
	HTREEITEM m_hTerrainNodes;

// Generated message map functions
protected:
	//{{AFX_MSG(CLeftView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnParentNotify( UINT message, LPARAM lParam );
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in LeftView.cpp
inline CSiegeEditorDoc* CLeftView::GetDocument()
   { return (CSiegeEditorDoc*)m_pDocument; }
#endif


bool IsSNOFile( char *szFile );

#define gLeftView CLeftView::GetSingleton()

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LEFTVIEW_H__A8C8446E_14A4_4E47_8277_025818E19122__INCLUDED_)
