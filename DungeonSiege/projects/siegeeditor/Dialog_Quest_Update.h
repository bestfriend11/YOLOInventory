#if !defined(AFX_DIALOG_QUEST_UPDATE_H__36B35BE6_2E28_4065_A37D_6A50BA7C0415__INCLUDED_)
#define AFX_DIALOG_QUEST_UPDATE_H__36B35BE6_2E28_4065_A37D_6A50BA7C0415__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Quest_Update.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Update dialog

class Dialog_Quest_Update : public CDialog
{
// Construction
public:
	Dialog_Quest_Update(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Quest_Update)
	enum { IDD = IDD_DIALOG_QUEST_UPDATE };
	CEdit	m_Speaker;
	CEdit	m_Order;
	CEdit	m_Descriptions;
	CComboBox	m_Conversations;
	CButton	m_RequiredUpdate;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Quest_Update)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	gpstring m_sOriginalOrder;
	StringVec m_ConversationAddresses;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Quest_Update)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_QUEST_UPDATE_H__36B35BE6_2E28_4065_A37D_6A50BA7C0415__INCLUDED_)
