#if !defined(AFX_DIALOG_MOOD_MUSIC_H__15CFED52_A116_40A1_B590_4AF97DF3E2A4__INCLUDED_)
#define AFX_DIALOG_MOOD_MUSIC_H__15CFED52_A116_40A1_B590_4AF97DF3E2A4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Mood_Music.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_Music dialog

class Dialog_Mood_Music : public CDialog
{
// Construction
public:
	Dialog_Mood_Music(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Mood_Music)
	enum { IDD = IDD_DIALOG_MOOD_MUSIC };
	CEdit	m_StandardSound;
	CEdit	m_StandardRepeatDelay;
	CEdit	m_StandardIntroDelay;
	CEdit	m_BattleSound;
	CEdit	m_BattleRepeatDelay;
	CEdit	m_BattleIntroDelay;
	CEdit	m_AmbientSound;
	CEdit	m_AmbientRepeatDelay;
	CEdit	m_AmbientIntroDelay;
	CComboBox	m_RoomType;
	CButton	m_MusicEnable;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Mood_Music)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Mood_Music)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnButtonHelp();
	afx_msg void OnButtonAmbientFiles();
	afx_msg void OnButtonStandardSoundFiles();
	afx_msg void OnButtonBattleSoundFiles();
	afx_msg void OnCheckMusicEnable();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_MOOD_MUSIC_H__15CFED52_A116_40A1_B590_4AF97DF3E2A4__INCLUDED_)
