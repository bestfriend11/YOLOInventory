#if !defined(AFX_PROPPAGE_PROPERTIES_H__30B031B7_5E6A_4F0B_A3CA_31E00CE6D66D__INCLUDED_)
#define AFX_PROPPAGE_PROPERTIES_H__30B031B7_5E6A_4F0B_A3CA_31E00CE6D66D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPage_Properties.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties dialog

class PropPage_Properties : public CPropertyPage
{
// Construction
public:
	PropPage_Properties(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(PropPage_Properties)
	enum { IDD = IDD_PROPPAGE_PROPERTIES };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PropPage_Properties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(PropPage_Properties)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGE_PROPERTIES_H__30B031B7_5E6A_4F0B_A3CA_31E00CE6D66D__INCLUDED_)
