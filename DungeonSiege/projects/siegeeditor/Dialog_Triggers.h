#if !defined(AFX_DIALOG_TRIGGERS_H__B9634DE3_1A3E_4780_BA8C_DB783C0E26B9__INCLUDED_)
#define AFX_DIALOG_TRIGGERS_H__B9634DE3_1A3E_4780_BA8C_DB783C0E26B9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Triggers.h : header file
//

#include "ResizableDialog.h"
#include "PropertyGrid.h"
#include "GridCtrl\\BtnDataBase.h"
#include "GoDefs.h"


/////////////////////////////////////////////////////////////////////////////
// Dialog_Triggers dialog

class Dialog_Triggers : public CResizableDialog
{
// Construction
public:
	Dialog_Triggers(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Triggers)
	enum { IDD = IDD_DIALOG_TRIGGERS };
	CTreeCtrl	m_triggers;
	CStatic	m_replace_actions;
	CStatic	m_replace_conditions;
	CEdit	m_resetdelay;
	CButton	m_oneshot;
	CButton	m_flipflop;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Triggers)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:

	CPropertyGrid	m_grid_conditions;	
	CPropertyGrid	m_grid_actions;	
	CBtnDataBase	m_BtnDataBaseAction;
	CBtnDataBase	m_BtnDataBaseCondition;

	void SetupTriggerTree();
	bool RetrieveSelectedData( Goid & object, int & index );
	void InitializeTriggerGrids();

	// Generated message map functions
	//{{AFX_MSG(Dialog_Triggers)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnButtonAddCondition();
	afx_msg void OnButtonRemoveCondition();
	afx_msg void OnButtonAddAction();
	afx_msg void OnButtonRemoveActions();
	afx_msg void OnButtonNewTrigger();
	afx_msg void OnButtonRemoveTrigger();
	afx_msg void OnSelchangedTreeTriggers(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCheckOneShot();
	afx_msg void OnCheckFlipflop();
	afx_msg void OnChangeEditResetDelay();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_TRIGGERS_H__B9634DE3_1A3E_4780_BA8C_DB783C0E26B9__INCLUDED_)
