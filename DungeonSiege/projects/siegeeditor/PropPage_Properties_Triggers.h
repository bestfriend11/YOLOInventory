#if !defined(AFX_PROPPAGE_PROPERTIES_TRIGGERS_H__758C35F2_0454_4195_8A28_5C746DDF043A__INCLUDED_)
#define AFX_PROPPAGE_PROPERTIES_TRIGGERS_H__758C35F2_0454_4195_8A28_5C746DDF043A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPage_Properties_Triggers.h : header file
//

#include "ResizablePage.h"
#include "PropertyGrid.h"
#include "GridCtrl\\BtnDataBase.h"
#include "GoDefs.h"

/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Triggers dialog

class PropPage_Properties_Triggers : public CResizablePage
{
	DECLARE_DYNCREATE(PropPage_Properties_Triggers)

// Construction
public:
	PropPage_Properties_Triggers();
	~PropPage_Properties_Triggers();

// Dialog Data
	//{{AFX_DATA(PropPage_Properties_Triggers)
	enum { IDD = IDD_PROPPAGE_PROPERTIES_TRIGGERS };
	CButton	m_selfdestruct;
	CButton	m_singleplayer;
	CButton	m_multiplayer;
	CButton	m_active;
	CEdit	m_occupants;
	CEdit	m_delay;
	CEdit	m_resetdelay;
	CButton	m_flipflop;
	CButton	m_oneshot;
	CTreeCtrl	m_triggers;
	CStatic	m_replace_conditions;
	CStatic	m_replace_actions;
	//}}AFX_DATA
	
	void Reset();
	void Apply();
	void Cancel();

	bool CanApply() { return false; }
	
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(PropPage_Properties_Triggers)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:

	bool			m_bApply;
	CPropertyGrid	m_grid_conditions;	
	CPropertyGrid	m_grid_actions;	
	CBtnDataBase	m_BtnDataBaseAction;
	CBtnDataBase	m_BtnDataBaseCondition;

	void SetupTriggerTree();
	bool RetrieveSelectedData( Goid & object, int & index );
	void InitializeTriggerGrids();
	void BeginTriggerEdit();
	void EndTriggerEdit();

	std::map< WORD, WORD > m_conditionIdMap;

	// Generated message map functions
	//{{AFX_MSG(PropPage_Properties_Triggers)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangedTreeTriggers(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeEditResetDelay();
	afx_msg void OnButtonAddCondition();
	afx_msg void OnButtonRemoveCondition();
	afx_msg void OnButtonAddAction();
	afx_msg void OnButtonRemoveActions();
	afx_msg void OnButtonNewTrigger();
	afx_msg void OnButtonRemoveTrigger();
	afx_msg void OnCheckOneShot();
	afx_msg void OnCheckFlipflop();
	afx_msg void OnChangeEditDelay();
	afx_msg void OnChangeEditOccupants();
	afx_msg void OnCheckActive();
	afx_msg void OnCheckSingleplayer();
	afx_msg void OnCheckMultiplayer();
	afx_msg void OnCheckCanSelfDestruct();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGE_PROPERTIES_TRIGGERS_H__758C35F2_0454_4195_8A28_5C746DDF043A__INCLUDED_)
