#if !defined(AFX_DIALOG_KEYBOARD_SHORTCUTS_H__1378D398_1FCE_4A84_9E68_9CD10C32C5FC__INCLUDED_)
#define AFX_DIALOG_KEYBOARD_SHORTCUTS_H__1378D398_1FCE_4A84_9E68_9CD10C32C5FC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Keyboard_Shortcuts.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Keyboard_Shortcuts dialog

class Dialog_Keyboard_Shortcuts : public CDialog
{
// Construction
public:
	Dialog_Keyboard_Shortcuts(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Keyboard_Shortcuts)
	enum { IDD = IDD_DIALOG_KEYBOARD_SETTINGS };
	CListCtrl	m_list_commands;
	CComboBox	m_combo_key;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Keyboard_Shortcuts)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void Refresh();

	// Generated message map functions
	//{{AFX_MSG(Dialog_Keyboard_Shortcuts)
	virtual void OnOK();
	afx_msg void OnAssign();
	afx_msg void OnSelchangeListCommands();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonReset();
	afx_msg void OnClickListCommands(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonClear();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

gpstring GetCommandName( int cmd );

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_KEYBOARD_SHORTCUTS_H__1378D398_1FCE_4A84_9E68_9CD10C32C5FC__INCLUDED_)
