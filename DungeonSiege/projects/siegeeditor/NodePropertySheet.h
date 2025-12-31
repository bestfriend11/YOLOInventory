#if !defined(AFX_NODEPROPERTYSHEET_H__9019115C_FEC8_4FE3_8008_E59E6907D4B2__INCLUDED_)
#define AFX_NODEPROPERTYSHEET_H__9019115C_FEC8_4FE3_8008_E59E6907D4B2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NodePropertySheet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// NodePropertySheet

class NodePropertySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(NodePropertySheet)

// Construction
public:
	NodePropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	NodePropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(NodePropertySheet)
	public:
	virtual BOOL DestroyWindow();
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual BOOL OnInitDialog();
	protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~NodePropertySheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(NodePropertySheet)
		// NOTE - the ClassWizard will add and remove member functions here.		
	//}}AFX_MSG	
	DECLARE_MESSAGE_MAP()

private:
	
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NODEPROPERTYSHEET_H__9019115C_FEC8_4FE3_8008_E59E6907D4B2__INCLUDED_)
