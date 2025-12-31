#if !defined(AFX_DIALOG_QUEST_CHAPTER_UPDATE_H__B72A35D5_6161_4652_9A3B_2C1D0DF68947__INCLUDED_)
#define AFX_DIALOG_QUEST_CHAPTER_UPDATE_H__B72A35D5_6161_4652_9A3B_2C1D0DF68947__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Quest_Chapter_Update.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Chapter_Update dialog

class Dialog_Quest_Chapter_Update : public CDialog
{
// Construction
public:
	Dialog_Quest_Chapter_Update(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Quest_Chapter_Update)
	enum { IDD = IDD_DIALOG_QUEST_CHAPTER_UPDATE };
	CEdit	m_Order;
	CEdit	m_Description;
	CComboBox	m_Samples;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Quest_Chapter_Update)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	gpstring m_sOriginalOrder;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Quest_Chapter_Update)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_QUEST_CHAPTER_UPDATE_H__B72A35D5_6161_4652_9A3B_2C1D0DF68947__INCLUDED_)
