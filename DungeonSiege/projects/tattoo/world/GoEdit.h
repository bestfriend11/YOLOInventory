//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoEdit.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the edit component for Go's. Used by editing tools
//             (such as Siege Editor) only. Note that this component is not
//             available in retail builds.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GOEDIT_H
#define __GOEDIT_H
#if !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////

#include "Go.h"

//////////////////////////////////////////////////////////////////////////////
// class GoEdit declaration

class GoEdit : public GoComponent
{
public:
	SET_INHERITED( GoEdit, GoComponent );

// Setup.

	// ctor/dtor
	GoEdit( Go* parent );
	GoEdit( const GoEdit& source, Go* parent );
	virtual ~GoEdit( void );

	// persistence
	void SaveGo( FuelHandle& fuel );

// Modifications.

	// bracket editing with these
	void BeginEdit ( void );
	void CancelEdit( void );
	bool EndEdit   ( void );
	bool IsEditing ( void ) const		{  return ( m_Editing );  }

	// set/query which fields of which components are instance overrides
	void SetOverride( const char* component, const char* field, bool set = true );
	bool GetOverride( const char* component, const char* field );

	// component access
	int FindComponent( const char* component );
	FuBi::Record& GetRecord( int index );
	FuBi::Record* GetRecord( const char* component );

// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newParent );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );

	// event handlers
	virtual bool CommitCreation( void );
	virtual void CommitClone   ( const GoComponent* src );

// Interface

	bool IsVisible( void )			{ return m_bIsVisible; }
	void SetIsVisible( bool bSet )	{ m_bIsVisible = bSet; }

private:
	struct Component;

	Component* GetComponent( const char* name );

	struct Component
	{
		// "current" store is in the component's godc. "original" store is
		// here. during xfer into go, try the "test" store here, and if it
		// fails, fall back to "current". if success, copy "test" to "current".

		GoComponent*     m_Component;
		my FuBi::Store*  m_Test;
		my FuBi::Record* m_TestRecord;
		my FuBi::Store*  m_Original;
		BitVector        m_OverrideColl;
		BitVector        m_BackupOverrideColl;

		Component( void );
		Component( const Component& component );
	   ~Component( void );

		void Init( GoComponent* goc, const Component* other = NULL );

		void XferFromGo( void );
		bool XferToGo  ( bool useBackup = false );
		void CancelEdit( void );
		void EndEdit   ( void );

		Component& operator = ( const Component& component );
	};

	typedef std::vector <Component> ComponentColl;

	ComponentColl m_ComponentColl;
	bool m_Editing;
	bool m_bIsVisible;

	SET_NO_COPYING( GoEdit );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // !GP_RETAIL
#endif  // __GOEDIT_H

//////////////////////////////////////////////////////////////////////////////
