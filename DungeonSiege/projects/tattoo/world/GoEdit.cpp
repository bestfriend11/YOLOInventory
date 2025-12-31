//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoEdit.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#if !GP_RETAIL

#include "FuBiSchemaImpl.h"
#include "FuBiPersistImpl.h"
#include "GoData.h"
#include "GoEdit.h"
#include "GoSupport.h"

DECLARE_DEV_GO_COMPONENT( GoEdit, "edit" );

//////////////////////////////////////////////////////////////////////////////
// class GoEdit implementation

GoEdit :: GoEdit( Go* parent )
	: Inherited( parent )
{
	m_Editing = false;
	m_bIsVisible = true;
}

GoEdit :: GoEdit( const GoEdit& source, Go* parent )
	: Inherited( source, parent )
{
	m_Editing = false;
	m_bIsVisible = true;
}

GoEdit :: ~GoEdit( void )
{
	// this space intentionally left blank...
}

static void CheckFirst( bool& firstAll, FuelHandle& fuel, bool& firstBlock, FuelHandle& compFuel, GoComponent* goc )
{
	// open new block if necessary
	if ( firstAll )
	{
		firstAll = false;
		Go* go = goc->GetGo();

		// build filename
		gpstring fileName( go->GetDataTemplate()->GetMetaGroupName() );
		fileName += ".gas";

		// create initial fuel block
		if ( fuel->IsDirectory() )
		{
			fuel = fuel->CreateChildBlock( ::ToString( go->GetScid() ), fileName );
		}
		else
		{
			fuel = fuel->CreateChildBlock( ::ToString( go->GetScid() ) );
		}

		fuel->SetType( go->GetTemplateName() );
	}

	// open new subblock if necessary
	if ( firstBlock )
	{
		firstBlock = false;
		compFuel = fuel->CreateChildBlock( goc->GetName() );
	}
}

void GoEdit :: SaveGo( FuelHandle& fuel )
{
	// xfer local into store
	BeginEdit();

	// set up for total go block creation
	bool firstAll = true;

	// for each component
	ComponentColl::const_iterator i, ibegin = m_ComponentColl.begin(), iend = m_ComponentColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		bool firstBlock = true;
		FuelHandle compFuel;
		const GoDataComponent* data = i->m_Component->GetData();

		// for each overridden field
		BitVector::const_iterator j, jbegin = i->m_OverrideColl.begin(), jend = i->m_OverrideColl.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			// overridden?
			if ( *j )
			{
				// preprocess block opening
				CheckFirst( firstAll, fuel, firstBlock, compFuel, i->m_Component );

				// get the data
				gpstring str;
				gpverify( i->m_TestRecord->Get( j - jbegin, str ) );

				// add the override - note that strings are quoted on request
				int index = j - jbegin;
				const FuBi::ColumnSpec& column = data->GetHeaderSpec()->GetColumn( index );
				FuBi::eXfer xfer = FuBi::XFER_NORMAL;
				if (   (column.m_Type == FuBi::VAR_STRING)
					&& (data->GetFlags( index ) & (  GoDataComponent::FLAG_LOCALIZE
												   | GoDataComponent::FLAG_QUOTED)) )
				{
					xfer = FuBi::XFER_QUOTED;
				}

				compFuel->Add( column.GetName(), str, FuBi::FuelWriter::GetFuelType( xfer, column.m_Type.m_Type ) );
			}
		}

		// now save out internals
		if ( i->m_Component->ShouldSaveInternals() )
		{
			CheckFirst( firstAll, fuel, firstBlock, compFuel, i->m_Component );
			i->m_Component->SaveInternals( compFuel );
		}
	}

	// must have saved something!
	gpassert( !firstAll );

	// all done
	CancelEdit();
}

void GoEdit :: BeginEdit( void )
{
	gpassert( GetGo()->AssertValid() );

	// no recursion
	gpassert( !m_Editing );
	m_Editing = true;

	// suck data out of go into local and backup stores
	ComponentColl::iterator i, begin = m_ComponentColl.begin(), end = m_ComponentColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		i->XferFromGo();
	}
}

void GoEdit :: CancelEdit( void )
{
	gpassert( GetGo()->AssertValid() );

	// must already be editing
	gpassert( m_Editing );
	m_Editing = false;

	// restore old overrides
	ComponentColl::iterator i, begin = m_ComponentColl.begin(), end = m_ComponentColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		i->CancelEdit();
	}
}

bool GoEdit :: EndEdit( void )
{
	// must already be editing
	gpassert( m_Editing );

	bool success = true;

	// attempt to xfer from local to go
	ComponentColl::iterator i, begin = m_ComponentColl.begin(), end = m_ComponentColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( !i->XferToGo() )
		{
			success = false;
			break;
		}
	}

	if ( success )
	{
		// cool - copy test to current
		for ( i = begin ; i != end ; ++i )
		{
			i->EndEdit();
		}
	}
	else
	{
		// failed, xfer from backup instead
		for ( i = begin ; i != end ; ++i )
		{
			gpverify( i->XferToGo( true ) );
		}
	}

	// finish
	gpassert( GetGo()->AssertValid() );
	m_Editing = false;

	return ( success );
}

void GoEdit :: SetOverride( const char* component, const char* field, bool set )
{
	// look up vars
	Component* comp = GetComponent( component );
	GoDataComponent* data = comp->m_Component->GetOwnedData(); 
	const FuBi::StoreHeaderSpec* spec = data->GetStoreHeaderSpec();
	int index = spec->FindColumn( field );
	gpassert( (index >= 0) && (index < (int)comp->m_OverrideColl.size()) );

	// check for a change
	if ( comp->m_OverrideColl[ index ] != set )
	{
		// change it
		comp->m_OverrideColl[ index ] = set;

		// restore value if clearing override
		if ( !set )
		{
			// build a record for original data
			FuBi::StoreHeaderSpec::Record original( comp->m_Original, spec );

			// copy the field
			comp->m_TestRecord->CopyField( index, original );
			data->GetRecordForWrite()->CopyField( index, original );
		}
	}
}

bool GoEdit :: GetOverride( const char* component, const char* field )
{
	Component* comp = GetComponent( component );
	int index = comp->m_Component->GetData()->GetHeaderSpec()->FindColumn( field );
	gpassert( (index >= 0) && (index < (int)comp->m_OverrideColl.size()) );
	return ( comp->m_OverrideColl[ index ] );
}

int GoEdit :: FindComponent( const char* component )
{
	gpassert( component != NULL );
	int index = -1;

	ComponentColl::iterator i, begin = m_ComponentColl.begin(), end = m_ComponentColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( i->m_Component->GetName().same_no_case( component ) )
		{
			index = i - begin;
			break;
		}
	}

	return ( index );
}

FuBi::Record& GoEdit :: GetRecord( int index )
{
	gpassert( (index >= 0) && (index < (int)m_ComponentColl.size()) );
	return ( *m_ComponentColl[ index ].m_TestRecord );
}

FuBi::Record* GoEdit :: GetRecord( const char* component )
{
	int index = FindComponent( component );
	return ( (index != -1) ? &GetRecord( index ) : NULL );
}

GoComponent* GoEdit :: Clone( Go* newParent )
{
	return ( new GoEdit( *this, newParent ) );
}

bool GoEdit :: Xfer( FuBi::PersistContext& /*persist*/ )
{
	return ( true );
}

bool GoEdit :: CommitCreation( void )
{
	// only if we weren't cloning
	if ( m_ComponentColl.empty() )
	{
		// build stores - note that "this" should be first
		Go::ComponentColl::iterator i, begin = GetGo()->GetComponentBegin(), end = GetGo()->GetComponentEnd();
		gpassert( *begin == this );
		for ( i = begin, ++i ; i != end ; ++i )
		{
			m_ComponentColl.push_back( Component() );
			m_ComponentColl.back().Init( *i );
		}
	}

	// always succeeds
	return ( true );
}

void GoEdit :: CommitClone( const GoComponent* src )
{
	// get source edit
	GoEdit* srcEdit = src->GetGo()->GetEdit();

	// special: copy source stores and overrides but use my components
	Go::ComponentColl::iterator goci, begin = GetGo()->GetComponentBegin(), end = GetGo()->GetComponentEnd();
	gpassert( *begin == this );
	ComponentColl::const_iterator ci = srcEdit->m_ComponentColl.begin();
	for ( goci = begin, ++goci ; goci != end ; ++goci, ++ci )
	{
		m_ComponentColl.push_back( Component() );
		m_ComponentColl.back().Init( *goci, ci );
	}
}

GoEdit::Component* GoEdit :: GetComponent( const char* name )
{
	Component* component = NULL;

	ComponentColl::iterator i, begin = m_ComponentColl.begin(), end = m_ComponentColl.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( i->m_Component->GetName().same_no_case( name ) )
		{
			component = i;
		}
	}

	gpassert( component != NULL );
	return ( component );
}

//////////////////////////////////////////////////////////////////////////////
// class GoEdit::Component implementation

GoEdit::Component :: Component( void )
{
	m_Component  = NULL;
	m_Test       = NULL;
	m_TestRecord = NULL;
	m_Original   = NULL;
}

GoEdit::Component :: Component( const Component& component )
	: m_OverrideColl( component.m_OverrideColl )
{
	m_Component  = NULL;
	m_Test       = NULL;
	m_TestRecord = NULL;
	m_Original   = NULL;

	if ( component.m_Component != NULL )
	{
		Init( component.m_Component, &component );
	}
}

GoEdit::Component :: ~Component( void )
{
	delete ( m_Test       );
	delete ( m_TestRecord );
	delete ( m_Original   );
}

void GoEdit::Component :: Init( GoComponent* goc, const Component* other )
{
	// only init once
	gpassert( m_Component == NULL );
	gpassert( goc != NULL );

	// setup
	m_Component = goc;
	m_Component->ForceOwned();
	const GoDataComponent* godc = goc->GetData();

	// optionally copy from other goedit component
	if ( other != NULL )
	{
		gpassert( other->m_Test != NULL );
		m_Test = new FuBi::Store( *other->m_Test );
		m_Original = new FuBi::Store( *other->m_Original );
		m_OverrideColl = other->m_OverrideColl;
	}
	else
	{
		// test is based on current state of go
		m_Test = new FuBi::Store( *godc->GetStore() );

		// original comes from the *template*
		const GoDataComponent* ogodc = goc->GetGo()->GetDataTemplate()->FindComponentByName( goc->GetName() );
		gpassert( ogodc != NULL );
		m_Original = new FuBi::Store( *ogodc->GetStore() );

		// setup overrides
		const BitVector* overrideColl = goc->GetOverrideColl();
		if ( (overrideColl != NULL) && !overrideColl->empty() )
		{
			m_OverrideColl = *overrideColl;
		}
		else
		{
			m_OverrideColl.resize( godc->GetHeaderSpec()->GetColumnCount(), false );
		}
	}

	// finish up
	m_TestRecord = new FuBi::StoreHeaderSpec::Record( m_Test, godc->GetStoreHeaderSpec() );
}

void GoEdit::Component :: XferFromGo( void )
{
	gpassert( m_Component != NULL );

	// xfer from go into test store
	FuBi::RecordWriter writer( *m_TestRecord );
	FuBi::PersistContext persist( &writer, false );
	gpverify( m_Component->Xfer( persist ) );

	// now compare this with original storage, update overrides accordingly
	FuBi::StoreHeaderSpec::Record original( m_Original, m_Component->GetData()->GetStoreHeaderSpec() );
	for ( int i = 0, count = original.GetHeaderSpec()->GetColumnCount() ; i != count ; ++i )
	{
		if ( original.CompareField( i, *m_TestRecord ) != 0 )
		{
			m_OverrideColl[ i ] = true;
		}
	}

	// backup overrides
	m_BackupOverrideColl = m_OverrideColl;
}

bool GoEdit::Component :: XferToGo( bool useBackup )
{
	gpassert( m_Component != NULL );

	const FuBi::Record* record = useBackup ? m_Component->GetData()->GetRecord() : m_TestRecord;
	FuBi::RecordReader reader( *record );
	FuBi::PersistContext persist( &reader, false );
	return ( m_Component->Xfer( persist ) );
}

void GoEdit::Component :: CancelEdit( void )
{
	m_OverrideColl = m_BackupOverrideColl;
}

void GoEdit::Component :: EndEdit( void )
{
	*m_Component->GetOwnedData()->GetStoreForWrite() = *m_Test;
}

GoEdit::Component& GoEdit::Component :: operator = ( const Component& component )
{
	m_Component = NULL;
	Delete( m_Test       );
	Delete( m_TestRecord );
	Delete( m_Original   );
	m_OverrideColl.clear();

	Init( component.m_Component, &component );

	return ( *this );
}

//////////////////////////////////////////////////////////////////////////////

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
