#if !defined(AFX_PROPPAGE_COLOR_H__5F75CA14_959A_419B_8208_1BEDBB284CF6__INCLUDED_)
#define AFX_PROPPAGE_COLOR_H__5F75CA14_959A_419B_8208_1BEDBB284CF6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPage_Color.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// PropPage_Color dialog

class PropPage_Color : public CPropertyPage
{
// Construction
public:
	PropPage_Color(CWnd* pParent = NULL);   // standard constructor

	HBRUSH m_hBrush;

// Dialog Data
	//{{AFX_DATA(PropPage_Color)
	enum { IDD = IDD_PROPPAGE_COLOR };
	CEdit	m_color_visual;
	float	m_color_blue;
	float	m_color_green;
	float	m_color_red;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PropPage_Color)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(PropPage_Color)
	afx_msg void OnButtonColorStandard();
	afx_msg void OnButtonColorAdvanced();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditColorRed();
	afx_msg void OnChangeEditColorGreen();
	afx_msg void OnChangeEditColorBlue();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGE_COLOR_H__5F75CA14_959A_419B_8208_1BEDBB284CF6__INCLUDED_)
