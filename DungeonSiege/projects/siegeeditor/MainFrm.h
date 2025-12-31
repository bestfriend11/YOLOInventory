// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__DEF0C9D7_D6BF_4FF6_BF7E_312DF2C7A1E8__INCLUDED_)
#define AFX_MAINFRM_H__DEF0C9D7_D6BF_4FF6_BF7E_312DF2C7A1E8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "gpcore.h"


// Defines
#define STATUS_CAMERA		0
#define STATUS_DISTANCE		1	
#define STATUS_NODE			2
#define STATUS_SNOS			3


class CSiegeEditorView;
class MenuCommands;


class CMainFrame : public CFrameWnd, public Singleton <CMainFrame>
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
protected:
	CSplitterWnd	m_wndSplitter;
	CSplitterWnd	m_wndSplitterConsole;
	
public:

// Operations
public:

	CSplitterWnd &	GetSplitterWnd() { return m_wndSplitter; }
	CSplitterWnd &	GetConsoleSplitterWnd() { return m_wndSplitterConsole; }
	void ShowDlgBar();
	void ShowViewBar();
	void ShowNodeBar();
	void ShowObjectBar();
	void ShowLightBar();
	void ShowCameraBar();
	void ShowModeBar();
	void ShowPreferenceBar();
	
	CWnd * GetCWnd() { return (CWnd *)this; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
	CSiegeEditorView* GetRightPane();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	CStatusBar	& GetStatusBar()	{ return m_wndStatusBar; }
	CDialogBar	& GetDlgBar()		{ return m_wndDlgBar; }
	CToolBar	& GetNodeBar()		{ return m_wndNodeBar; }
	CToolBar	& GetObjectBar()	{ return m_wndObjectBar; }
	CToolBar	& GetLightBar()		{ return m_wndLightBar; }
	CToolBar	& GetViewBar()		{ return m_wndViewBar; }
	CToolBar	& GetModeBar()		{ return m_wndModeBar; }
	CToolBar	& GetPreferenceBar(){ return m_wndPreferenceBar; }

	MenuCommands *m_pMenuCommands;

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;
	CReBar      m_wndReBar;
	CDialogBar  m_wndDlgBar;
	CToolBar	m_wndNodeBar;
	CToolBar	m_wndObjectBar;
	CToolBar	m_wndLightBar;
	CToolBar	m_wndViewBar;	
	CToolBar	m_wndCameraBar;
	CToolBar	m_wndModeBar;
	CToolBar	m_wndPreferenceBar;
	int			m_band_dialogbar;
	int			m_band_viewbar;
	int			m_band_nodebar;
	int			m_band_objectbar;
	int			m_band_lightbar;
	int			m_band_camerabar;
	int			m_band_modebar;
	int			m_band_preferencebar;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnActivateApp(BOOL bActive, HTASK hTask);
	afx_msg void OnClose();
	//}}AFX_MSG
	afx_msg void OnUpdateViewStyles(CCmdUI* pCmdUI);	

	DECLARE_MESSAGE_MAP()
};


#define gMainFrm CMainFrame::GetSingleton()


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__DEF0C9D7_D6BF_4FF6_BF7E_312DF2C7A1E8__INCLUDED_)
