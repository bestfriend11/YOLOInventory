#if !defined(AFX_SECONDARYFRM_H__E98A1B27_3099_4550_80B3_27531F338161__INCLUDED_)
#define AFX_SECONDARYFRM_H__E98A1B27_3099_4550_80B3_27531F338161__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SecondaryFrm.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSecondaryFrm frame

class CSecondaryFrm : public CFrameWnd
{
	DECLARE_DYNCREATE(CSecondaryFrm)

public:

	virtual ~CSecondaryFrm();
	CSecondaryFrm();           // protected constructor used by dynamic creation

protected:

	CSplitterWnd	m_wndSplitterConsole;
	

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSecondaryFrm)
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
protected:
	

	// Generated message map functions
	//{{AFX_MSG(CSecondaryFrm)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SECONDARYFRM_H__E98A1B27_3099_4550_80B3_27531F338161__INCLUDED_)
