#if !defined(AFX_DIALOG_TEXT_H__67C37FCF_5194_4FC2_9279_9CB019E3A1FB__INCLUDED_)
#define AFX_DIALOG_TEXT_H__67C37FCF_5194_4FC2_9279_9CB019E3A1FB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Text.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDialog_Text dialog

class CDialog_Text : public CDialog
{
// Construction
public:
	CDialog_Text(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDialog_Text)
	enum { IDD = IDD_DIALOG_TEXT };
	CComboBox	m_justify;
	CEdit	m_edit_text;
	CComboBox	m_combo_font;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialog_Text)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	DWORD m_dwColor;

	// Generated message map functions
	//{{AFX_MSG(CDialog_Text)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonColor();
	afx_msg void OnButtonTest();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_TEXT_H__67C37FCF_5194_4FC2_9279_9CB019E3A1FB__INCLUDED_)
