#if !defined(AFX_DIALOG_MOOD_ADD_FOLDER_H__C9F733B2_95F5_4079_8B1E_2D79CE82F9A2__INCLUDED_)
#define AFX_DIALOG_MOOD_ADD_FOLDER_H__C9F733B2_95F5_4079_8B1E_2D79CE82F9A2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Mood_Add_Folder.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_Add_Folder dialog

class Dialog_Mood_Add_Folder : public CDialog
{
// Construction
public:
	Dialog_Mood_Add_Folder(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Mood_Add_Folder)
	enum { IDD = IDD_DIALOG_MOOD_ADD_FOLDER };
	CEdit	m_Name;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Mood_Add_Folder)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Mood_Add_Folder)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_MOOD_ADD_FOLDER_H__C9F733B2_95F5_4079_8B1E_2D79CE82F9A2__INCLUDED_)
