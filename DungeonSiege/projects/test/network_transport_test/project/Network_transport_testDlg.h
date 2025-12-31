// Network_transport_testDlg.h : header file
//

#if !defined(AFX_NETWORK_TRANSPORT_TESTDLG_H__E2C2CDC6_E062_11D2_B5CA_006008688288__INCLUDED_)
#define AFX_NETWORK_TRANSPORT_TESTDLG_H__E2C2CDC6_E062_11D2_B5CA_006008688288__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "NetPipe.h"


/////////////////////////////////////////////////////////////////////////////
// CNetwork_transport_testDlg dialog

class CNetwork_transport_testDlg : public CDialog
{
// Construction
public:
	CNetwork_transport_testDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CNetwork_transport_testDlg)
	enum { IDD = IDD_NETWORK_TRANSPORT_TEST_DIALOG };
	CEdit	m_NewConnectionName;
	CComboBox	m_ConnectionsCombo;
	CStatic	m_CurrentProvider;
	CStatic	m_SessionOpen;
	CComboBox	m_SessionsCombo;
	CComboBox	m_ProviderList;
	CEdit	m_NewSessionName;
	CButton	m_ListSessions;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetwork_transport_testDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CNetwork_transport_testDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnExit();
	afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEditchangeSessionsCombo();
	afx_msg void OnListSessions();
	afx_msg void OnChangeNewSessionName();
	afx_msg void OnListproviders();
	afx_msg void OnSelchangeProviderlist();
	afx_msg void OnResetnetworktransport();
	afx_msg void OnCreateconnection();
	afx_msg void OnChangeNewconnectionname();
	afx_msg void OnInitprovider();
	afx_msg void OnOpensession();
	afx_msg void OnSelchangeSessionscombo();
	afx_msg void OnCreateandopensession();
	afx_msg void OnListconnections();
	afx_msg void OnCloseconnection();
	afx_msg void OnEditchangeConnectionscombo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	NetPipe * mNetPipe;
};




//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NETWORK_TRANSPORT_TESTDLG_H__E2C2CDC6_E062_11D2_B5CA_006008688288__INCLUDED_)
