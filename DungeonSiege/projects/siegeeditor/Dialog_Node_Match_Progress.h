#if !defined(AFX_DIALOG_NODE_MATCH_PROGRESS_H__41D9BEEE_41C7_4D5B_8195_2D8EBE84F01D__INCLUDED_)
#define AFX_DIALOG_NODE_MATCH_PROGRESS_H__41D9BEEE_41C7_4D5B_8195_2D8EBE84F01D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Node_Match_Progress.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Node_Match_Progress dialog

class Dialog_Node_Match_Progress : public CDialog, public Singleton <Dialog_Node_Match_Progress>
{
// Construction
public:
	Dialog_Node_Match_Progress(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Node_Match_Progress)
	enum { IDD = IDD_DIALOG_NODE_MATCH_PROGRESS };
	CStatic	m_Node;
	//}}AFX_DATA

	void SetCurrentNode( const gpstring & sNode );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Node_Match_Progress)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Node_Match_Progress)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#define gDialogNodeMatchProgress Dialog_Node_Match_Progress::GetSingleton()

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_NODE_MATCH_PROGRESS_H__41D9BEEE_41C7_4D5B_8195_2D8EBE84F01D__INCLUDED_)
