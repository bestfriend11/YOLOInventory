#if !defined(AFX_DIALOG_PORTRAITS_H__74BA415D_8E70_4252_A924_2ECA72E5E3D2__INCLUDED_)
#define AFX_DIALOG_PORTRAITS_H__74BA415D_8E70_4252_A924_2ECA72E5E3D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Portraits.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Portraits dialog

typedef std::vector< gpstring > SkinVec;

class Dialog_Portraits : public CDialog, public Singleton <Dialog_Portraits>
{
// Construction
public:
	Dialog_Portraits(CWnd* pParent = NULL);   // standard constructor

	void Select( Goid object );

	void Draw();

	void AdjustRectToMouse( int x, int y );

	bool GetAllowDrag()				{ return m_bAllowDrag; }
	void SetAllowDrag( bool bSet )	{ m_bAllowDrag = bSet; }

// Dialog Data
	//{{AFX_DATA(Dialog_Portraits)
	enum { IDD = IDD_DIALOG_PORTRAITS };
	CEdit	m_iconName;
	CEdit	m_template;
	CEdit	m_scid;
	CButton	m_useSkins;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Portraits)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void CreatePortrait( gpstring sName );

	// Generated message map functions
	//{{AFX_MSG(Dialog_Portraits)
	afx_msg void OnButtonGeneratePortrait();
	afx_msg void OnButtonSetPortraitWindow();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	Goid m_object;
	RECT m_portraitRect;
	bool m_bAllowDrag;	
	unsigned int temp;
};


#define gDialogPortraits Dialog_Portraits::GetSingleton()

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_PORTRAITS_H__74BA415D_8E70_4252_A924_2ECA72E5E3D2__INCLUDED_)
