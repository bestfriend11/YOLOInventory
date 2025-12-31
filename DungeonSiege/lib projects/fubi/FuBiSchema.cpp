//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiSchema.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_FuBi.h"
#include "FuBiSchema.h"

#include "FuBi.h"
#include "FuBiPersistImpl.h"
#include "Fuel.h"
#include "ReportSys.h"

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// class ColumnSpec implementation

ColumnSpec :: ColumnSpec( const TypeSpec& type )
	: ParamSpec( type )
{
	gpassertm( type.m_Type != VAR_UNKNOWN, "Invalid type spec - VAR_UNKNOWN" );
}

void ColumnSpec :: SetName( const gpstring& name )
{
	gpassertf( IsValidSysId( name, NAMING_DATA_OR_CODE ), ( "Invalid column name '%s'", name.c_str() ));
	ParamSpec::SetName( name );
}

//////////////////////////////////////////////////////////////////////////////
// class HeaderSpec implementation

HeaderSpec :: HeaderSpec( int count, ... )
{
	m_Digest = 0;

	va_list args = va_args( count );
	for ( int i = 0 ; i < count ; ++i )
	{
		AddColumn( VAR_STRING, va_arg( args, const char* ) );
	}
}

void HeaderSpec :: SetName( const gpstring& name )
{
	//$$$ register/unregister with gFuBiSysExports
	//$$$ this is complicated by the copy ctor requirement...

	m_Name = name;
}

int HeaderSpec :: AddColumn( const TypeSpec& type, const gpstring& name  GPDEV_PARAM( const gpstring& docs ) )
{
	gpassert( !HasColumn( name ) );

	ColumnSpecColl::iterator i = m_ColumnSpecs.push_back( type );
	i->SetName( name );
	GPDEV_ONLY( i->m_Documentation = docs );

	return ( scast <int> ( m_ColumnSpecs.size() ) - 1 );
}

#if !GP_RETAIL

void HeaderSpec :: SetConstraint( int index, ConstraintSpec* spec )
{
	PrivateGetColumn( index ).SetConstraint( spec );
}

#endif // !GP_RETAIL

void HeaderSpec :: SetDefaultValue( int index, const gpstring& defValue, bool isCode )
{
	PrivateGetColumn( index ).SetDefaultValue( defValue, isCode );
}

void HeaderSpec :: SetNoDefaultValue( int index )
{
	PrivateGetColumn( index ).SetNoDefaultValue();
}

#if !GP_RETAIL

void HeaderSpec :: GenerateDocs( ReportSys::ContextRef context ) const
{
	ConstIter i = GetColumnBegin(), end = GetColumnEnd();
	for ( int index = 0 ; i != end ; ++i, ++index )
	{
		ReportSys::OutputF( context, "Column %2d: ", index );
		i->GenerateDocs( context );
        ReportSys::OutputEol( context );
	}
}

#endif // !GP_RETAIL

DWORD HeaderSpec :: GetDigest( void ) const
{
	if ( m_Digest == 0 )
	{
		// first put name in there
		m_Digest = GetCRC32( m_Digest, m_Name.c_str(), m_Name.length() + 1 );

		// now add columns to it
		for ( ConstIter i = GetColumnBegin(), end = GetColumnEnd() ; i != end ; ++i )
		{
			m_Digest += i->CalcDigest();
		}
	}
	return ( m_Digest );
}

bool HeaderSpec :: CanSkrit( void ) const
{
	ConstIter i, begin = GetColumnBegin(), end = GetColumnEnd();
	for ( i = begin ; i != end ; ++i )
	{
		if ( !i->m_Type.CanSkrit() )
		{
			return ( false );
		}
	}
	return ( true );
}

int HeaderSpec :: FindColumn( const char* name ) const
{
	gpassert( name != NULL );

	ConstIter i, begin = GetColumnBegin(), end = GetColumnEnd();
	for ( i = begin ; i != end ; ++i )
	{
		if ( i->GetName().same_no_case( name ) )
		{
			return ( i - begin );
		}
	}
	return ( INVALID_COLUMN_INDEX );
}

bool HeaderSpec :: operator == ( const HeaderSpec& other ) const
{
	if ( this == &other )
	{
		return ( true );
	}
	return ( GetDigest() == other.GetDigest() );
}

#if !GP_RETAIL

void HeaderSpec :: Dump( bool sort, ReportSys::ContextRef ctx ) const
{
	// setup output
	ReportSys::TextTableSink sink;
	ReportSys::LocalContext loc( &sink, false );
	loc.SetSchema( new ReportSys::Schema( 4, "Name", "Type", "Default", "Documentation" ), true );

	// dump elements
	ColumnSpecColl::const_iterator i,
								   begin = m_ColumnSpecs.begin(),
								   end   = m_ColumnSpecs.end  ();
	for ( i = begin ; i != end ; ++i )
	{
		// get the type
		const ColumnSpec& col = *i;

		// out the name
		ReportSys::AutoReport autoReport( loc );
		loc.OutputField( col.GetName() );

		// out the type
		loc.OutputField( gFuBiSysExports.MakeFullTypeName( col.m_Type ) );

		// out the default
		if ( col.m_Extra != NULL )
		{
			loc.OutputField( col.m_Extra->m_DefaultValue );
		}
		else
		{
			loc.OutputField( "<None>" );
		}

		// docs
		loc.OutputField( col.m_Documentation );
	}

	// out header
	ctx->OutputF( "Table '%s':\n\n", m_Name.c_str() );
	if ( (m_Docs != NULL) && (*m_Docs != '\0') )
	{
		ReportSys::AutoIndent autoIndent( ctx );
		ctx->OutputF( "%s\n\n", m_Docs );
	}

	// out table
	if ( !m_ColumnSpecs.empty() )
	{
		if ( sort )
		{
			sink.Sort();
		}
		sink.OutputReport( ctx );
	}
	else
	{
		ctx->OutputF( "<No fields!>\n\n" );
	}
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class Record implementation

Record :: Record( const Record& other )
{
	m_Spec = other.m_Spec;
	m_XferMode = other.m_XferMode;
}

FUBI_DECLARE_CAST_TRAITS( eXMode, DWORD );

bool Record :: XferFlags( PersistContext& persist )
{
	persist.Xfer( "m_XferMode",      m_XferMode      );
	persist.Xfer( "m_QuotedStrings", m_QuotedStrings );
	return ( true );
}

bool Record :: GetAsString( int index, gpstring& out, eXfer xfer ) const
{
	return ( ::ToString( GetHeaderSpec()->GetColumn( index ).m_Type, out,
						 OnGetFieldPtr( index ), xfer, m_XferMode ) );
}

bool Record :: SetAsString( int index, const char* str )
{
	const TypeSpec& type = GetHeaderSpec()->GetColumn( index ).m_Type;
	void* field = OnGetFieldPtr( index );

	if ( type == VAR_STRING )
	{
		HeaderSpec::StringCollCb cb = GetHeaderSpec()->GetStringCollectorCb();
		if ( cb )
		{
			*rcast <gpstring*> ( field ) = cb( str );
		}
		else
		{
			// $ this is an optimization - bypass fromstring()
			*rcast <gpstring*> ( field ) = str;
		}
		return ( true );
	}
	else
	{
		return ( ::FromString( type, str, field, m_XferMode ) );
	}
}

bool Record :: SetAsString( int index, const gpstring& str )
{
	const TypeSpec& type = GetHeaderSpec()->GetColumn( index ).m_Type;
	void* field = OnGetFieldPtr( index );

	if ( type == VAR_STRING )
	{
		HeaderSpec::StringCollCb cb = GetHeaderSpec()->GetStringCollectorCb();
		if ( cb )
		{
			*rcast <gpstring*> ( field ) = cb( str );
		}
		else
		{
			// $ this is an optimization - bypass fromstring()
			*rcast <gpstring*> ( field ) = str;
		}
		return ( true );
	}
	else
	{
		return ( ::FromString( type, str, field, m_XferMode ) );
	}
}

bool Record :: GetAsString( const char* fieldName, gpstring& out, eXfer xfer ) const
{
	int index = FindColumn( fieldName );
	if ( index != -1 )
	{
		return ( GetAsString( index, out, xfer ) );
	}
	else
	{
#		if !GP_RETAIL
		if ( ShouldReportNameErrors( m_XferMode ) )
		{
			gpreportf( gXferErrors, ( "GetAsString() error: no field name '%s'\n", fieldName ));
		}
#		endif // !GP_RETAIL
		return ( false );
	}
}

bool Record :: SetAsString( const char* fieldName, const char* str )
{
	int index = FindColumn( fieldName );
	if ( index != -1 )
	{
		return ( SetAsString( index, str ) );
	}
	else
	{
#		if !GP_RETAIL
		if ( ShouldReportNameErrors( m_XferMode ) )
		{
			gpreportf( gXferErrors, ( "SetAsString() error: no field name '%s'\n", fieldName ));
		}
#		endif // !GP_RETAIL
		return ( false );
	}
}

bool Record :: SetAsString( const char* fieldName, const gpstring& str )
{
	int index = FindColumn( fieldName );
	if ( index != -1 )
	{
		return ( SetAsString( index, str ) );
	}
	else
	{
#		if !GP_RETAIL
		if ( ShouldReportNameErrors( m_XferMode ) )
		{
			gpreportf( gXferErrors, ( "SetAsString() error: no field name '%s'\n", fieldName ));
		}
#		endif // !GP_RETAIL
		return ( false );
	}
}

bool Record :: GetField( int index, eVarType type, void* obj ) const
{
	const TypeSpec& fieldType = GetHeaderSpec()->GetColumn( index ).m_Type;
	gpassert( fieldType.IsPassByValue() || fieldType.IsPointerClass());

	if ( fieldType.m_Type == type )
	{
		return ( gFuBiSysExports.CopyVar( type, obj, OnGetFieldPtr( index ), m_XferMode ) );
	}
	else
	{
		// have to do a conversion $$$ for now do this through a string (yuck)
		gpstring temp;
		if ( GetAsString( index, temp ) )
		{
			return ( ::FromString( type, temp, obj, m_XferMode ) );
		}
		else
		{
			return ( false );
		}
	}
}

bool Record :: GetField( const char* fieldName, eVarType type, void* obj ) const
{
	int index = FindColumn( fieldName );
	if ( index != -1 )
	{
		return ( GetField( index, type, obj ) );
	}
	else
	{
#		if !GP_RETAIL
		if ( ShouldReportNameErrors( m_XferMode ) )
		{
			gpreportf( gXferErrors, ( "GetField() error: no field name '%s'\n", fieldName ));
		}
#		endif // !GP_RETAIL
		return ( false );
	}
}

bool Record :: SetField( int index, eVarType type, const void* obj )
{
	const TypeSpec& fieldType = GetHeaderSpec()->GetColumn( index ).m_Type;
	gpassert( fieldType.IsPassByValue() || fieldType.IsPointerClass());

	if ( fieldType == type )
	{
		void* field = OnGetFieldPtr( index );

		HeaderSpec::StringCollCb cb = GetHeaderSpec()->GetStringCollectorCb();
		if ( cb && (type == VAR_STRING) )
		{
			*rcast <gpstring*> ( field ) = cb( *rcast <const gpstring*> ( obj ) );
			return ( true );
		}
		else
		{
			return ( gFuBiSysExports.CopyVar( type, field, obj, m_XferMode ) );
		}
	}
	else
	{
		// have to do a conversion $$$ for now do this through a string (yuck)
		gpstring temp;
		if ( ::ToString( type, temp, obj, XFER_NORMAL, m_XferMode ) )
		{
			return ( SetAsString( index, temp ) );
		}
		else
		{
			return ( false );
		}
	}
}

bool Record :: SetField( const char* fieldName, eVarType type, const void* obj )
{
	int index = FindColumn( fieldName );
	if ( index != -1 )
	{
		return ( SetField( index, type, obj ) );
	}
	else
	{
#		if !GP_RETAIL
		if ( ShouldReportNameErrors( m_XferMode ) )
		{
			gpreportf( gXferErrors, ( "SetField() error: no field name '%s'\n", fieldName ));
		}
#		endif // !GP_RETAIL
		return ( false );
	}
}

bool Record :: Set( const char* fieldName, const char* obj )
{
	int index = FindColumn( fieldName );
	if ( index != -1 )
	{
		return ( Set( index, obj ) );
	}
	else
	{
#		if !GP_RETAIL
		if ( ShouldReportNameErrors( m_XferMode ) )
		{
			gpreportf( gXferErrors, ( "Set() error: no field name '%s'\n", fieldName ));
		}
#		endif // !GP_RETAIL
		return ( false );
	}
}

void Record :: CopyField( int index, const Record& other )
{
	gpassert( *GetHeaderSpec() == *other.GetHeaderSpec() );
	gpverify( gFuBiSysExports.CopyVar(
			GetHeaderSpec()->GetColumn( index ).m_Type,
			OnGetFieldPtr( index ),
			other.OnGetFieldPtr( index ),
			m_XferMode ) );
}

int Record :: CompareField( int index, const Record& other ) const
{
	gpassert( *GetHeaderSpec() == *other.GetHeaderSpec() );
	return( gFuBiSysExports.CompareVar(
			GetHeaderSpec()->GetColumn( index ).m_Type,
			OnGetFieldPtr( index ),
			other.OnGetFieldPtr( index ) ) );
}

bool Record :: LoadFromFuel( FastFuelHandle fuel )
{
	// must be valid fuel block
	gpassert( fuel );

	// build xfer objects
	FastFuelReader reader( fuel );
	PersistContext persist( &reader );

	// xfer in params
	gpstring str;
	bool success = true;
	HeaderSpec::ConstIter i, begin = GetHeaderSpec()->GetColumnBegin(), end = GetHeaderSpec()->GetColumnEnd();
	for ( i = begin ; i != end ; ++i )
	{
		if ( fuel.Get( i->GetName(), str ) )
		{
			if ( !SetAsString( i - begin, str ) )
			{
				success = false;
			}
		}
	}

	// returns false if any of the parameters failed to convert properly
	return ( success );
}

bool Record :: LoadFromString( const char* params )
{
	bool success = true;

	// get each key/value pair
	stringtool::Extractor extractor( "&", params );
	const char* kb = NULL, * ke = NULL;
	const char* vb = NULL, * ve = NULL;
	char* key = NULL, * val = NULL;
	while ( extractor.GetNextString( kb, ke ) )
	{
		if ( kb != ke )
		{
			// find divider and value
			const char* eq = ::strnchr( kb, '=', ke - kb );
			if ( eq != NULL )
			{
				vb = eq + 1;
				ve = ke;
				ke = eq;

				stringtool::RemoveBorderingWhiteSpace( vb, ve );
				stringtool::RemoveBorderingQuotes( vb, ve );
			}

			// kill w/s
			stringtool::RemoveBorderingWhiteSpace( kb, ke );

			// alloc local key
			int keyLen = ke - kb;
			key = (char*)::_alloca( keyLen + 1 );
			::memcpy( key, kb, keyLen );
			key[ keyLen ] = '\0';

			// alloc local value
			int valLen = ve - vb;
			val = (char*)::_alloca( valLen + 1 );
			::memcpy( val, vb, valLen );
			val[ valLen ] = '\0';

			// set it
			int col = FindColumn( key );
			if ( col != INVALID_COLUMN_INDEX )
			{
				if ( !SetAsString( col, val ) )
				{
					success = false;
				}
			}
			else
			{
				gpwarningf(( "Record '%s' error - cannot set property '%s' to '%s' because it does not exist!\n",
							 GetHeaderSpec()->GetName().c_str(), key, val ));
				success = false;
			}
		}
	}

	return ( success );
}

#if !GP_RETAIL

void Record :: ReportAsTable( ReportSys::ContextRef ctx, eXfer xfer ) const
{
	// setup output
	ReportSys::TextTableSink sink;
	ReportSys::LocalContext loc( &sink, false );
	loc.SetSchema( new ReportSys::Schema( 4, "Name", "Type", "Data", "Documentation" ), true );

	// dump elements
	HeaderSpec::ConstIter i,
						  begin = GetHeaderSpec()->GetColumnBegin(),
						  end   = GetHeaderSpec()->GetColumnEnd  ();
	int index = 0;
	for ( i = begin ; i != end ; ++i, ++index )
	{
		// get the type
		const ColumnSpec& col = *i;

		// out the name
		ReportSys::AutoReport autoReport( loc );
		loc.OutputField( col.GetName() );

		// out the type
		loc.OutputField( gFuBiSysExports.MakeFullTypeName( col.m_Type ) );

		// print it
		gpstring value;
		if ( GetAsString( index, value, xfer ) )
		{
			loc.OutputField( value );
		}
		else
		{
			loc.OutputField( "<not available>" );
		}

		// docs
		loc.OutputField( col.m_Documentation );
	}

	// out table
	sink.OutputReport( ctx );
}

void Record :: ReportAsRow( ReportSys::ContextRef ctx, eXfer xfer ) const
{
	// dump elements
	HeaderSpec::ConstIter i, begin = GetHeaderSpec()->GetColumnBegin(), end = GetHeaderSpec()->GetColumnEnd();
	int index = 0;
	for ( i = begin ; i != end ; ++i, ++index )
	{
		// get the type and the param itself
		const ColumnSpec& col = *i;

		// only supporting by-value for now
		gpassert( col.m_Type.IsPassByValue() );

		// get value
		gpstring value;
		if ( !GetAsString( index, value, xfer ) )
		{
			value = "<not available>";
		}

		// report
		ctx->OutputF( "%s %s = %s; ", gFuBiSysExports.MakeFullTypeName( col.m_Type ).c_str(),
					  col.GetName().c_str(), value.c_str() );
	}
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// struct ClassHeaderSpec implementation

int ClassHeaderSpec :: AddColumn( const TypeSpec& type, const char* name, int offset  GPDEV_PARAM( const char* docs ) )
{
	m_Offsets.push_back( offset );
    return ( Inherited::AddColumn( type, name  GPDEV_PARAM( docs ? docs : gpstring::EMPTY ) ) );
}

#if !GP_RETAIL

void ClassHeaderSpec :: GenerateDocs( ReportSys::ContextRef context )
{
	HeaderSpec::ConstIter i = GetColumnBegin(), end = GetColumnEnd();
    OffsetColl::const_iterator j = m_Offsets.begin();

	for ( int index = 0 ; i != end ; ++i, ++j, ++index )
	{
		ReportSys::OutputF( context, "Column %2d: ", index );
		i->GenerateDocs( context );
        ReportSys::OutputF( context, ", offset = %d\n", *j );
	}
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
