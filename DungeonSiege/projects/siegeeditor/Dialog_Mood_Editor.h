#if !defined(AFX_DIALOG_MOOD_EDITOR_H__E768B7E2_9EDB_4A22_B9A7_EFE4463E4B2D__INCLUDED_)
#define AFX_DIALOG_MOOD_EDITOR_H__E768B7E2_9EDB_4A22_B9A7_EFE4463E4B2D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Mood_Editor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_Editor dialog

class Dialog_Mood_Editor : public CDialog, public Singleton <Dialog_Mood_Editor>
{
// Construction
public:
	Dialog_Mood_Editor(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Mood_Editor)
	enum { IDD = IDD_DIALOG_MOOD_EDITOR };
	CTreeCtrl	m_Tanked;
	CTreeCtrl	m_Moods;
	//}}AFX_DATA

	FuelHandle & GetSelectedMood();

	void SetNewMoodName( gpstring sName )	{ m_sNewMood = sName; }
	void SetNewFolderName( gpstring sName ) { m_sNewFolder = sName; }

	void SetSelectedFolder( gpstring sFolder )	{ m_sSelectedFolder = sFolder;	}
	const gpstring & GetSelectedFolder()		{ return m_sSelectedFolder;		}

	void Refresh();

	bool DoesMoodExist( const gpstring & sMood );

	void		SetFogColor( COLORREF color )	{ m_FogColor = color;	}
	COLORREF	GetFogColor()					{ return m_FogColor;	}

	void SaveMoods();
	void ShutdownDbs();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Mood_Editor)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	gpstring m_sNewMood;
	gpstring m_sNewFolder;
	gpstring m_sSelectedFolder;	
	COLORREF m_FogColor;
	FuelHandle m_SelectedMood;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Mood_Editor)
	afx_msg void OnButtonHelp();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonRemove();
	afx_msg void OnButtonEditProperties();
	afx_msg void OnButtonEditWeather();
	afx_msg void OnButtonEditFog();
	afx_msg void OnButtonEditMusic();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonTest();
	afx_msg void OnButtonAddFolder();
	afx_msg void OnButtonStop();
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	virtual void OnOK();
	afx_msg void OnButtonTestAmbient();
	afx_msg void OnButtonTestBattle();
	afx_msg void OnButtonTestStandard();
	afx_msg void OnClickTreeMoods(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClickTreeMoodsTanked(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#define gDialogMoodEditor Singleton <Dialog_Mood_Editor>::GetSingleton()

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_MOOD_EDITOR_H__E768B7E2_9EDB_4A22_B9A7_EFE4463E4B2D__INCLUDED_)
