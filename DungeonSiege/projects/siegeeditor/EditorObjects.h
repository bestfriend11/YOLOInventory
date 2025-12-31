//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorObjects.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __EDITOROBJECTS_H
#define __EDITOROBJECTS_H


// Include Files
#include "GoDefs.h"


// Structures
struct FIND_COMPONENT
{
	gpstring sFuelName;
	gpstring sScreenName;
	gpstring sDevName;
	gpstring sType;
	DWORD	 dwScid;
};


enum eSequenceType
{
	SEQUENCE_SELECT,
	SEQUENCE_LINK,
	SEQUENCE_MOVE,
	SEQUENCE_PATROL,
	SEQUENCE_STOP,
	SEQUENCE_FOLLOW,
	SEQUENCE_GUARD,
	SEQUENCE_ATTACK,
	SEQUENCE_GET,
	SEQUENCE_DROP,
	SEQUENCE_USE,
	SEQUENCE_EQUIP,
	SEQUENCE_GIVE,
};


struct ItemViewerFilter
{
	eEquipSlot	slot;
	bool		bSpell;
};


struct MacroComponent
{
	vector_3	pos;
	gpstring	sAddress;
	Scid		oldScid;
};

struct NextObject
{
	bool bRandPitch;
	bool bRandYaw;
	bool bRandRoll;
	bool bRandomScale;
	float minMultiplier;
	float maxMultiplier;		
};


class EditorObjects : public Singleton <EditorObjects>
{
public:

	EditorObjects();
	~EditorObjects() {};

	bool AddObject( gpstring sTemplate, SiegePos spos, Goid & newObject );
	Goid AddObject( Goid object, SiegePos spos );
	Goid AddObjectFromInstance( gpstring sFuelAddress, SiegePos spos );
	bool AddSelectedObject();

	void DeleteObject( Goid object );
	void DeleteSelectedObjects();
	void DeleteObject( Scid object );

	void SetSelectedTemplateName( gpstring sName ) { m_sSelectedTemplate = sName; }
	gpstring & GetSelectedTemplateName() { return m_sSelectedTemplate; }
	
	bool Find( FIND_COMPONENT & fc );
	void FindNext();
	void FindSelectAll();
	void ReplaceObject( gpstring sReplaceName );
	
	void Copy();
	void Cut();
	void Paste();

	void LinkSelectedObjectsToDest( Goid object );

	bool IsSequenceEditing() { return m_bSequenceEditing; }
	void SetSequenceEditing( bool bEditing ) { m_bSequenceEditing = bEditing; }

	eSequenceType	GetSequenceType()						{ return m_sequenceType; }
	void			SetSequenceType( eSequenceType type )	{ m_sequenceType = type; }

	void PlaceSequenceObject();
	void PlaceMeterPoint();

	void GrabSelectedScid();

	void SwitchObject( Goid object, gpstring sNewTemplate );

	void Draw();
	void DrawCommands();
	void DrawMeterPoints();

	void		SetHiddenComponents( StringVec hiddenComponents )	{ m_hiddenComponents = hiddenComponents; }
	StringVec & GetHiddenComponents()								{ return m_hiddenComponents; }
	bool		IsHiddenComponent( gpstring sComponent );
	void		AddHiddenComponent( gpstring sComponent );
	void		RemoveHiddenComponent( gpstring sComponent );

	void				SetItemViewerFilter( eEquipSlot slot, bool bSpell ) { m_ViewerFilter.slot = slot; m_ViewerFilter.bSpell = bSpell; }
	ItemViewerFilter	GetItemViewerFilter() { return m_ViewerFilter; }

	void		SetViewerSelectedTemplate( gpstring sSelected ) { m_ViewerSelected = sSelected; }
	gpstring	GetViewerSelectedTemplate()						{ return m_ViewerSelected;		}

	bool CreateMacroFromSelection( gpstring sMacro );
	bool CreateMacro( gpstring sMacro, std::vector< DWORD > objects );
	void DeleteMacro( gpstring sMacro );
	void PlaceMacroInSelectedNode( gpstring sMacro ); 
	void PlaceMacro( gpstring sMacro, SiegePos spos );
	void GetMacroNames( StringVec & macros );

	NextObject & GetNextObjectSettings() { return m_nextObject; }

private:	

	typedef std::map< Scid, Scid > OldScidToNewScidMap;

	gpstring				m_sSelectedTemplate;
	GoidColl				m_clipboard;	
	bool					m_bCutting;
	GoidColl				m_found_objects;
	GoidColl::iterator		m_found_iterator;
	bool					m_bSequenceEditing;
	eSequenceType			m_sequenceType;

	Goid					m_sequenceFirst;
	Goid					m_sequenceLast;

	StringVec				m_hiddenComponents;

	ItemViewerFilter		m_ViewerFilter;
	gpstring				m_ViewerSelected;

	NextObject				m_nextObject;
};


#define gEditorObjects EditorObjects::GetSingleton()


#endif