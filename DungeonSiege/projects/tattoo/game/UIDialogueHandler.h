//////////////////////////////////////////////////////////////////////////////
//
// File     :  UIDialogueHandler.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __UIDIALOGUEHANDLER_H
#define __UIDIALOGUEHANDLER_H

class UIDialogueHandler  : public Singleton <UIDialogueHandler>
{
public:

	UIDialogueHandler();
	~UIDialogueHandler() {}

	void Deinit();

	// Dialogue
	void DialogueCallback( Goid talker, Goid member );
	void RequestResetDialogue();
	void ResetDialogue();
	void MuteDialogue();
	void UnMuteDialogue();
	void StopDialogue( bool bStopScroll = false );
	bool ExitDialogue( bool bStopTalking = true, bool bExplicitExit = false );
	void AdvanceDialogue();
	void NisUpdate( float deltaTime );
	void Update( float deltaTime );	
	void PlayStream( gpstring sSample );	

FEX	void RSSendReqTalkEnd( Goid member, Goid talker );

	Goid	GetTalkerGUID()					{ return m_talker; }
	Goid	GetMemberGUID()					{ return m_member; }
	void	SetTalkerGUID( Goid talker )	{ m_talker = talker; }
	void	SetMemberGUID( Goid member )	{ m_member = member; }

	void	SetTalkAttempt( bool bSet )		{ m_bTalkAttempt = bSet; }
	bool	GetTalkAttempt()				{ return m_bTalkAttempt; }

	void ShowHireStats();

	// Quests
	bool ToggleJournal();
	void ShowQuests( int selected );
	void JournalButton1();
	void JournalButton2();		

	void SelectQuest();

	void ShowDialogues( int questId );
	void ShowChapter( const gpwstring & sChapter );

	void StopQuestDialogue( bool bClearCallback = false );

	void ExitJournal();
	
	// Generic Button Handler
	void GenericButton1();
	void GenericButton2();
FEX	void RSSetButtonValue( const gpstring & sValue );

	void SetGenericButton1Value( gpstring sValue ) { m_sGenericButton1Value = sValue; }
	void SetGenericButton2Value( gpstring sValue ) { m_sGenericButton2Value = sValue; }

private:

	void	StopSampleCB( UINT32 param );

	void		SetDialogueText( gpwstring sText )	{ m_sDialogueText = sText; }
	gpwstring	GetDialogueText()					{ return m_sDialogueText; }

	void		SetDialogueSample( gpstring sSample )	{ m_sDialogueSample = sSample;	}
	gpstring	GetDialogueSample()						{ return m_sDialogueSample;		}

	void		SetScrollRate( float scrollRate )		{ m_scrollRate = scrollRate; }
	float		GetScrollRate()							{ return m_scrollRate; }

	Goid		m_talker;
	Goid		m_member;
	gpwstring	m_sDialogueText;
	gpstring	m_sDialogueSample;
	DWORD		m_currentSample;
	float		m_scrollRate;
	bool		m_bMiniMap;	
	float		m_elapsedTime;
	float		m_duration;
	bool		m_bUpdate;
	bool		m_bRestarted;
	bool		m_bNextUpdate;
	bool		m_bTalkAttempt;
	bool		m_bJournalSet;

	gpstring	m_sGenericButton1Value;
	gpstring	m_sGenericButton2Value;

	DWORD		m_speakerColor;
	DWORD		m_chapterColor;
	DWORD		m_activeQuestColor;
	DWORD		m_completedQuestColor;

	FUBI_SINGLETON_CLASS( UIDialogueHandler, "This is the UIDialogueHandler singleton" );
};


#define gUIDialogueHandler UIDialogueHandler::GetSingleton()


#endif