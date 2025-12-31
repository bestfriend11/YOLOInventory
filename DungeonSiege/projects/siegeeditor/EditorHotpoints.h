//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorHotpoints.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once 
#ifndef __EDITORHOTPOINTS_H
#define __EDITORHOTPOINTS_H


// Include Files
#include "SiegeEditorTypes.h"
#include "vector_3.h"
#include "siege_pos.h"


// This class deals with anything having to do with hotpoints within the editor
class EditorHotpoints : public Singleton <EditorHotpoints>
{
public:

	EditorHotpoints() { m_bDraw = false; m_selectedID = -1; }
	~EditorHotpoints() {}

	void Save();
	void Load();

	void Draw();
	void Draw( gpstring sName );

	void		SetCurrentDirection( vector_3 vDir ) { m_vDirection = vDir; }
	vector_3	GetCurrentDirection()				 { return m_vDirection; }

	void		SetCanDraw( bool bDraw ) { m_bDraw = bDraw; }
	bool		GetCanDraw()			 { return m_bDraw; }

	void		SetSelectedID( int id ) { m_selectedID = id; }
	int			GetSelectedID()			{ return m_selectedID; }

	void		SetSelectedName( gpstring sName )	{ m_selectedName = sName; }
	gpstring	GetSelectedName()					{ return m_selectedName; }

	void		Delete( int id, bool bFull = false );	
	void		AddDirectionalHotpoint( vector_3 vDir, int id = -1 );

private:

	vector_3 m_vDirection;
	bool	 m_bDraw;
	int		 m_selectedID;
	gpstring m_selectedName;


};


#define gEditorHotpoints EditorHotpoints::GetSingleton()





#endif