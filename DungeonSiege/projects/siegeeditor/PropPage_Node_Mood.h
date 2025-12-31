#if !defined(AFX_PROPPAGE_NODE_MOOD_H__54D6389E_5BBB_4C28_8741_9C0A0431BD79__INCLUDED_)
#define AFX_PROPPAGE_NODE_MOOD_H__54D6389E_5BBB_4C28_8741_9C0A0431BD79__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPage_Node_Mood.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// PropPage_Node_Mood dialog

class PropPage_Node_Mood : public CPropertyPage
{
	DECLARE_DYNCREATE(PropPage_Node_Mood)

// Construction
public:
	PropPage_Node_Mood();
	~PropPage_Node_Mood();

	void SetNodeMood();

private:

	CComboBox * m_comboMood;
	CString		m_rMood;

// Dialog Data
	//{{AFX_DATA(PropPage_Node_Mood)
	enum { IDD = IDD_PROPPAGE_NODE_MOOD };
	CListBox	m_listMood;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(PropPage_Node_Mood)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(PropPage_Node_Mood)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonAddMood();
	afx_msg void OnButtonRemoveMood();
	afx_msg void OnButtonRemoveAllMoods();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGE_NODE_MOOD_H__54D6389E_5BBB_4C28_8741_9C0A0431BD79__INCLUDED_)
