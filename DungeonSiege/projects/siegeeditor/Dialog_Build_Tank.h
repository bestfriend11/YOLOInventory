#if !defined(AFX_DIALOG_BUILD_TANK_H__BB551A75_1943_4B87_A02F_749260D7BCCB__INCLUDED_)
#define AFX_DIALOG_BUILD_TANK_H__BB551A75_1943_4B87_A02F_749260D7BCCB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Build_Tank.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Build_Tank dialog

class Dialog_Save_As_DSMod;
class Dialog_Extract_Tank;
class WorkerThread;

class Dialog_Build_Tank : public CDialog
{
// Construction
public:
	Dialog_Build_Tank(Dialog_Save_As_DSMod* pParent);   // standard constructor
	Dialog_Build_Tank(Dialog_Extract_Tank* pParent);   // standard constructor
	virtual ~Dialog_Build_Tank();

// Dialog Data
	//{{AFX_DATA(Dialog_Build_Tank)
	enum { IDD = IDD_DIALOG_BUILD_TANK_PROGRESS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Build_Tank)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	enum eState
	{
		STATE_RUNNING,
		STATE_CANCELLING,
		STATE_DONE,
	};

	void SetState( eState state );

	WorkerThread* m_WorkerThread;
	eState m_State;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Build_Tank)
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	afx_msg void OnDone();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_BUILD_TANK_H__BB551A75_1943_4B87_A02F_749260D7BCCB__INCLUDED_)
