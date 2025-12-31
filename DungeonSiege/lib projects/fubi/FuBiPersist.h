//////////////////////////////////////////////////////////////////////////////
//
// File     :  FuBiPersist.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains persistence (state-saving) classes.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __FUBIPERSIST_H
#define __FUBIPERSIST_H

//////////////////////////////////////////////////////////////////////////////

#include "FuBiDefs.h"

#include <stack>

namespace FuBi  {  // begin of namespace FuBi

//////////////////////////////////////////////////////////////////////////////
// interface declarations

struct PersistWriter
{
	virtual ~PersistWriter( void )  {  }
	virtual bool AsText( void ) = 0;
	virtual bool EnterBlock( const char* name ) = 0;
	virtual bool LeaveBlock( void ) = 0;
	virtual bool WriteString( eXfer xfer, const char* key, const gpstring& value, eVarType type ) = 0;
	virtual bool WriteString( eXfer xfer, const char* key, const gpwstring& value ) = 0;
	virtual bool WriteBinary( const char* key, const_mem_ptr ptr ) = 0;
};

struct PersistReader
{
	virtual ~PersistReader( void )  {  }
	virtual bool AsText( void ) = 0;
	virtual bool EnterBlock( const char* name ) = 0;
	virtual bool LeaveBlock( void ) = 0;
	virtual bool ReadString( eXfer xfer, const char* key, gpstring& value ) = 0;
	virtual bool ReadString( eXfer xfer, const char* key, gpwstring& value ) = 0;
	virtual bool ReadBinary( const char* key, mem_ptr ptr ) = 0;
};

//////////////////////////////////////////////////////////////////////////////
// class PersistContext declaration

class PersistContext
{
public:
	SET_NO_INHERITED( PersistContext );

	enum eDir
	{
		DIR_SAVE,
		DIR_RESTORE,
	};

	enum eFormat
	{
		FORMAT_TEXT,
		FORMAT_BINARY,
	};

	static const char* VALUE_STR;			// "_value"

// Setup.

	// ctor/dtor
	PersistContext( PersistWriter* writer, bool isFullXfer = true, eXMode xmode = XMODE_DEFAULT, gpstring* reportOutput = NULL );
	PersistContext( PersistReader* reader, bool isFullXfer = true, eXMode xmode = XMODE_DEFAULT, gpstring* reportOutput = NULL );
	virtual ~PersistContext( void );

	// mode query
	bool IsSaving       ( void ) const		{  return ( m_Writer != NULL );  }
	bool IsRestoring    ( void ) const		{  return ( m_Reader != NULL );  }
	bool IsFullXfer     ( void ) const		{  return ( m_IsFullXfer );  }
	bool IsInBadBlock   ( void ) const		{  return ( !m_BadNest.empty() );  }
	bool IsInFatalError ( void ) const		{  return ( m_IsFatalError || m_IsWatsonError );  }
	bool IsInWatsonError( void ) const		{  return ( m_IsWatsonError );  }
	bool AsText         ( void ) const		{  return (  m_IsText );  }
	bool AsBinary       ( void ) const		{  return ( !m_IsText );  }

	// options
	void               SetXferMode( eXMode xmode )				{  m_XferMode = xmode;  }
	eXMode             GetXferMode( void ) const				{  return ( m_XferMode );  }
	void               SetVersion ( const gpversion& version )	{  m_Version = version;  }
	const gpversion&   GetVersion ( void ) const				{  return ( m_Version );  }

	// errors
	void IncSeriousErrorCount( void )		{  ++m_SeriousErrorCount;  }
	int  GetSeriousErrorCount( void ) const {  return ( m_SeriousErrorCount );  }
	bool HasSeriousErrors    ( void ) const {  return ( GetSeriousErrorCount() > 0 );  }

// Control.

	// blocks
	bool EnterBlock( const char* name );
	bool LeaveBlock( void );

	// collections
	bool EnterColl( const char* name );
	bool LeaveColl( void );
	bool AdvanceCollIter( void );

	// errors
	void SetFatalError( void )				{  m_IsFatalError = true;  }
	void SetWatsonError( void )				{  SetFatalError();  m_IsWatsonError = true;  }

	// nonfatal error reporting
	ReportSys::ContextRef GetReportContext   ( void ) const		{  return ( m_ReportContext );  }
	const gpstring&       GetReportOutput    ( void ) const		{  return ( (m_ReportOutput != NULL) ? *m_ReportOutput : gpstring::EMPTY );  }
	void                  ReportPersistErrors( void );

// Basic persistence methods.

	// basic methods
	bool XferString( eXfer xfer, const char* key, gpstring& value, eVarType type )
		{  return ( IsSaving() ? m_Writer->WriteString( xfer, key, value, type ) : (!IsInBadBlock() && m_Reader->ReadString( xfer, key, value )) );  }
	bool XferString( eXfer xfer, const char* key, gpwstring& value )
		{  return ( IsSaving() ? m_Writer->WriteString( xfer, key, value ) : (!IsInBadBlock() && m_Reader->ReadString( xfer, key, value )) );  }
	bool XferString( const char* key, gpstring& value, eVarType type )
		{  return ( XferString( XFER_NORMAL, key, value, type ) );  }
	bool XferString( const char* key, gpwstring& value )
		{  return ( XferString( XFER_NORMAL, key, value ) );  }
	bool XferBinary( const char* key, mem_ptr ptr )
		{  return ( IsSaving() ? m_Writer->WriteBinary( key, ptr ) : (!IsInBadBlock() && m_Reader->ReadBinary( key, ptr )) );  }

	// transfer as text
	template <typename T, typename U>
	bool XferAsText( eXfer xfer, const char* key, T& obj, const U& defValue, eVarType type )
	{
		gpstring temp;

		if ( IsSaving() )
		{
			::ToString( temp, obj, xfer );
		}

		bool success = XferString( xfer, key, temp, type );

		if ( IsRestoring() )
		{
			if ( success )
			{
				success = ::FromString( temp, obj );
				if ( !success )
				{
#					if !GP_RETAIL
					ReportConversionError( temp, key );
#					endif // !GP_RETAIL
				}
			}

			if ( !success )
			{
				obj = defValue;
			}
		}

		return ( success );
	}

	template <typename T, typename U>
	bool XferAsText( const char* key, T& obj, const U& defValue )
		{  return ( XferAsText( XFER_NORMAL, key, obj, defValue ) );  }

	// transfer as binary
	template <typename T, typename U>
	bool XferAsBinary( const char* key, T& obj, const U& defValue )
	{
		bool found = true;
		if ( !XferBinary( key, make_mem_ptr( obj ) ) )
		{
			found = false;
			obj = defValue;
		}
		return ( found );
	}

	// $$$ XferRequired() that gives error/fatal if read fails (can't use default value)

	// $$$ xferinstance - check POD flag, verify version, enter/exit block... ** version at header level rather than per-class??

	// error reporting ( so it's not in the template)
#	if !GP_RETAIL
	void ReportConversionError( const gpstring& str, const char* key );
#	endif // !GP_RETAIL

// General persistence methods.

	// these will cover 95% of our single-xfer cases

	template <typename T, typename U>
	bool Xfer( eXfer xfer, const char* key, T& obj, const U& defValue )
		{  return ( Traits <T>::Xfer( *this, xfer, key, obj, defValue ) );  }

	template <typename T, typename U>
	bool Xfer( const char* key, T& obj, const U& defValue )
		{  return ( Xfer( XFER_NORMAL, key, obj, defValue ) );  }

	template <typename T>
	bool Xfer( eXfer xfer, const char* key, T& obj )
		{  return ( Traits <T>::XferDef( *this, xfer, key, obj ) );  }

	template <typename T>
	bool Xfer( const char* key, T& obj )
		{  return ( Xfer( XFER_NORMAL, key, obj ) );  }

	template <typename T, typename U>
	bool XferHex( const char* key, T& obj, const U& defValue )
		{  return ( Xfer( XFER_HEX, key, obj, defValue ) );  }

	template <typename T>
	bool XferHex( const char* key, T& obj )
		{  return ( Xfer( XFER_HEX, key, obj ) );  }

	template <typename T, typename U>
	bool XferFourCc( const char* key, T& obj, const U& defValue )
		{  return ( Xfer( XFER_FOURCC, key, obj, defValue ) );  }

	template <typename T>
	bool XferFourCc( const char* key, T& obj )
		{  return ( Xfer( XFER_FOURCC, key, obj ) );  }

	template <typename T, typename U>
	bool XferQuoted( const char* key, T& obj, const U& defValue )
		{  return ( Xfer( XFER_QUOTED, key, obj, defValue ) );  }

	template <typename T>
	bool XferQuoted( const char* key, T& obj )
		{  return ( Xfer( XFER_QUOTED, key, obj ) );  }

	template <typename T, typename ENUM>
	bool XferOption( const char* name, T& obj, ENUM option, bool* defValue = NULL )
	{
		bool rc, b = obj.TestOptions( option );
		if ( defValue != NULL )
		{
			rc = Xfer( name, b, *defValue );		// use specified default
		}
		else
		{
			rc = Xfer( name, b );					// default comes from traits or whatever
		}
		obj.SetOptions( option, b );
		return ( rc );
	}

	template <typename T, typename ENUM>
	bool XferOption( const char* name, T& obj, ENUM option, bool defValue )
	{
		return ( XferOption( name, obj, option, &defValue ) );
	}

	// specializations

	template <>
	bool Xfer <gpwstring> ( eXfer xfer, const char* key, gpwstring& value )
		{  return ( XferString( xfer, key, value ) );  }

	template <>
	bool Xfer <gpwstring> ( const char* key, gpwstring& value )
		{  return ( Xfer( XFER_NORMAL, key, value ) );  }

	template <>
	bool Xfer <gpwstring> ( eXfer xfer, const char* key, gpwstring& value, const wchar_t* defValue )
	{
		bool rc = Xfer( xfer, key, value );
		if ( !rc && IsRestoring() )
		{
			value = defValue;
		}
		return ( rc );
	}

	template <>
	bool Xfer <gpwstring> ( const char* key, gpwstring& value, const wchar_t* defValue )
		{  return ( Xfer( XFER_NORMAL, key, value, defValue ) );  }

	template <>
	bool Xfer <Record> ( eXfer xfer, const char* key, Record& record )
		{  return ( XferRecord( xfer, key, record, NULL ) );  }

	template <>
	bool Xfer <Record> ( const char* key, Record& record )
		{  return ( Xfer( XFER_NORMAL, key, record ) );  }

	template <>
	bool Xfer <Record> ( eXfer xfer, const char* key, Record& record, const Record& defValue )
		{  return ( XferRecord( xfer, key, record, &defValue ) );  }

	template <>
	bool Xfer <Record> ( const char* key, Record& record, const Record& defValue )
		{  return ( Xfer( XFER_NORMAL, key, record, defValue ) );  }


// Collection persistence methods.

	// this will work on lists, deques, vectors...
	struct XferCollValueHelper
	{
		template <typename COLL, typename ITER>
		bool Save( PersistContext& persist, eXfer xfer, COLL&, ITER i )
		{
			return ( persist.Xfer( xfer, VALUE_STR, *i ) );
		}

		template <typename COLL>
		bool Restore( PersistContext& persist, COLL& coll )
		{
			COLL::value_type value;
			if ( persist.Xfer( VALUE_STR, value ) )
			{
				coll.push_back( value );
				return ( true );
			}
			else
			{
				return ( false );
			}
		}
	};

	// this will work on UnsafeBucketVectors
	struct XferUnsafeBucketVectorValueHelper
	{
		template <typename COLL, typename ITER>
		bool Save( PersistContext& persist, eXfer xfer, COLL&, ITER i )
		{
			UINT index = i.GetIndex();
			return (   persist.Xfer( xfer, "_index", index)
					&& persist.Xfer( xfer, VALUE_STR, *i ) );
		}

		template <typename COLL>
		bool Restore( PersistContext& persist, COLL& coll )
		{
			UINT index;
			COLL::value_type value;
			if (   persist.Xfer( "_index", index )
				&& persist.Xfer( VALUE_STR, value ) )
			{
				coll.AllocSpecific( value, index );
				return ( true );
			}
			else
			{
				return ( false );
			}
		}
	};

	// this will work on sets
	struct XferSetValueHelper
	{
		template <typename COLL, typename ITER>
		bool Save( PersistContext& persist, eXfer xfer, COLL&, ITER i )
		{
			return ( persist.Xfer( xfer, VALUE_STR, *i ) );
		}

		template <typename COLL>
		bool Restore( PersistContext& persist, COLL& coll )
		{
			COLL::value_type value;
			if ( persist.Xfer( VALUE_STR, value ) )
			{
				coll.insert( value );
				return ( true );
			}
			else
			{
				return ( false );
			}
		}
	};

	// this will work on unique maps
	struct XferMapValueHelper
	{
		template <typename COLL, typename ITER>
		bool Save( PersistContext& persist, eXfer xfer, COLL&, ITER i )
		{
			return (   persist.Xfer( xfer, "_key", ccast <COLL::key_type&> ( i->first ) )
					&& persist.Xfer( xfer, VALUE_STR, i->second ) );
		}

		template <typename COLL>
		bool Restore( PersistContext& persist, COLL& coll )
		{
			COLL::value_type value;
			if (   persist.Xfer( "_key", ccast <COLL::key_type&> ( value.first ) )
				&& persist.Xfer( VALUE_STR, value.second ) )
			{
				coll.insert( value );
				return ( true );
			}
			else
			{
				return ( false );
			}
		}
	};

	template <typename COLL, typename HELPER>
	bool XferColl( eXfer xfer, const char* key, COLL& coll, HELPER helper, bool assertNotEmpty = true )
	{
		if( assertNotEmpty )
		{
			gpassert( IsSaving() || coll.empty() );
		}

		if ( IsSaving() && coll.empty() )
		{
			return ( true );
		}

		// attempt to enter the collection
		bool success = true;
		if ( EnterColl( key ) )
		{
			// xfer it
			if ( IsSaving() )
			{
				COLL::iterator i, begin = coll.begin(), end = coll.end();
				for ( i = begin ; i != end ; ++i )
				{
					if ( !AdvanceCollIter() || !helper.Save( *this, xfer, coll, i ) )
					{
						success = false;
					}
				}
			}
			else
			{
				while ( AdvanceCollIter() )
				{
					if ( !helper.Restore( *this, coll ) )
					{
						success = false;
					}
				}
			}
		}

		// clean up
		LeaveColl();

		// done
		return ( success );
	}

	template <typename COLL, typename HELPER>
	bool XferColl( const char* key, COLL& coll, HELPER helper, bool assertNotEmpty )
		{  return ( XferColl( XFER_NORMAL, key, coll, helper, assertNotEmpty ) );  }

	template <typename COLL, typename VAL_HELPER, typename IF_HELPER>
	bool XferIfColl( eXfer xfer, const char* key, COLL& coll, VAL_HELPER valHelper, IF_HELPER ifHelper )
	{
		bool success = true;
		if ( IsSaving() )
		{
			if ( coll.empty() )
			{
				return ( true );
			}

			bool entered = false;

			COLL::iterator i, begin = coll.begin(), end = coll.end();
			for ( i = begin ; i != end ; ++i )
			{
				if ( ifHelper( *i ) )
				{
					if ( !entered )
					{
						entered = true;
						EnterColl( key );
					}

					if ( !AdvanceCollIter() || !valHelper.Save( *this, xfer, coll, i ) )
					{
						success = false;
					}
				}
			}

			if ( entered )
			{
				LeaveColl();
			}
		}
		else
		{
			gpassert( coll.empty() );
			return ( XferColl( xfer, key, coll, valHelper ) );
		}

		// done
		return ( success );
	}

	template <typename COLL, typename VAL_HELPER, typename IF_HELPER>
	bool XferIfColl( const char* key, COLL& coll, VAL_HELPER valHelper, IF_HELPER ifHelper )
		{  return ( XferIfColl( XFER_NORMAL, key, coll, valHelper, ifHelper ) );  }

// Specialized collection xfer functions.

	// lists

	template <typename COLL>
	bool XferList( eXfer xfer, const char* key, COLL& coll )
		{  return ( XferColl( xfer, key, coll, XferCollValueHelper() ) );  }

	template <typename COLL>
	bool XferList( const char* key, COLL& coll )
		{  return ( XferList( XFER_NORMAL, key, coll ) );  }

	template <typename COLL, typename IF_HELPER>
	bool XferIfList( eXfer xfer, const char* key, COLL& coll, IF_HELPER ifHelper )
		{  return ( XferIfColl( xfer, key, coll, XferCollValueHelper(), ifHelper ) );  }

	template <typename COLL, typename IF_HELPER>
	bool XferIfList( const char* key, COLL& coll, IF_HELPER ifHelper )
		{  return ( XferIfList( XFER_NORMAL, key, coll, ifHelper ) );  }

	// vectors

	template <typename COLL>
	bool XferVector( eXfer xfer, const char* key, COLL& coll )
		{  return ( XferColl( xfer, key, coll, XferCollValueHelper() ) );  }

	template <typename COLL>
	bool XferVector( const char* key, COLL& coll )
		{  return ( XferVector( XFER_NORMAL, key, coll ) );  }

	template <typename COLL, typename IF_HELPER>
	bool XferIfVector( eXfer xfer, const char* key, COLL& coll, IF_HELPER ifHelper )
		{  return ( XferIfColl( xfer, key, coll, XferCollValueHelper(), ifHelper ) );  }

	template <typename COLL, typename IF_HELPER>
	bool XferIfVector( const char* key, COLL& coll, IF_HELPER ifHelper )
		{  return ( XferIfVector( XFER_NORMAL, key, coll, ifHelper ) );  }

	// deques

	template <typename COLL>
	bool XferDeque( eXfer xfer, const char* key, COLL& coll )
		{  return ( XferColl( xfer, key, coll, XferCollValueHelper() ) );  }

	template <typename COLL>
	bool XferDeque( const char* key, COLL& coll )
		{  return ( XferDeque( XFER_NORMAL, key, coll ) );  }

	template <typename COLL, typename IF_HELPER>
	bool XferIfDeque( eXfer xfer, const char* key, COLL& coll, IF_HELPER ifHelper )
		{  return ( XferIfColl( xfer, key, coll, XferCollValueHelper(), ifHelper ) );  }

	template <typename COLL, typename IF_HELPER>
	bool XferIfDeque( const char* key, COLL& coll, IF_HELPER ifHelper )
		{  return ( XferIfDeque( XFER_NORMAL, key, coll, ifHelper ) );  }

	// UnsafeBucketVectors

	template <typename COLL>
	bool XferUnsafeBucketVector( eXfer xfer, const char* key, COLL& coll )
		{  return ( XferColl( xfer, key, coll, XferUnsafeBucketVectorValueHelper() ) );  }

	template <typename COLL>
	bool XferUnsafeBucketVector( const char* key, COLL& coll )
		{  return ( XferUnsafeBucketVector( XFER_NORMAL, key, coll ) );  }

	template <typename COLL, typename IF_HELPER>
	bool XferIfUnsafeBucketVector( eXfer xfer, const char* key, COLL& coll, IF_HELPER ifHelper )
		{  return ( XferIfColl( xfer, key, coll, XferUnsafeBucketVectorValueHelper(), ifHelper ) );  }

	template <typename COLL, typename IF_HELPER>
	bool XferIfUnsafeBucketVector( const char* key, COLL& coll, IF_HELPER ifHelper )
		{  return ( XferIfUnsafeBucketVector( XFER_NORMAL, key, coll, ifHelper ) );  }

	// sets

	template <typename COLL>
	bool XferSet( eXfer xfer, const char* key, COLL& coll )
		{  return ( XferColl( xfer, key, coll, XferSetValueHelper() ) );  }

	template <typename COLL>
	bool XferSet( const char* key, COLL& coll )
		{  return ( XferSet( XFER_NORMAL, key, coll ) );  }

	template <typename COLL, typename IF_HELPER>
	bool XferIfSet( eXfer xfer, const char* key, COLL& coll, IF_HELPER ifHelper )
		{  return ( XferIfColl( xfer, key, coll, XferSetValueHelper(), ifHelper ) );  }

	template <typename COLL, typename IF_HELPER>
	bool XferIfSet( const char* key, COLL& coll, IF_HELPER ifHelper )
		{  return ( XferIfSet( XFER_NORMAL, key, coll, ifHelper ) );  }

	// maps

	template <typename COLL>
	bool XferMap( eXfer xfer, const char* key, COLL& coll )
		{  return ( XferColl( xfer, key, coll, XferMapValueHelper() ) );  }

	template <typename COLL>
	bool XferMap( const char* key, COLL& coll )
		{  return ( XferMap( XFER_NORMAL, key, coll ) );  }

	template <typename COLL, typename IF_HELPER>
	bool XferIfMap( eXfer xfer, const char* key, COLL& coll, IF_HELPER ifHelper )
		{  return ( XferIfColl( xfer, key, coll, XferMapValueHelper(), ifHelper ) );  }

	template <typename COLL, typename IF_HELPER>
	bool XferIfMap( const char* key, COLL& coll, IF_HELPER ifHelper )
		{  return ( XferIfMap( XFER_NORMAL, key, coll, ifHelper ) );  }

	template <typename COLL>
		bool XferMapAppend( eXfer xfer, const char* key, COLL& coll )
	{  return ( XferColl( xfer, key, coll, XferMapValueHelper(), false ) );  }

	template <typename COLL>
		bool XferMapAppend( const char* key, COLL& coll )
	{  return ( XferMapAppend( XFER_NORMAL, key, coll ) );  }

	// pairs

	template <typename PAIR>
	bool XferPair( eXfer xfer, const char* key, PAIR& pair )
	{
		bool success = EnterBlock( key );
		if ( success )
		{
			Xfer( xfer, "_first", pair.first );
			Xfer( xfer, "_second", pair.second );
		}
		LeaveBlock();
		return ( success );
	}

	template <typename PAIR>
	bool XferPair( const char* key, PAIR& pair )
		{  return ( XferPair( XFER_NORMAL, key, pair ) );  }

	// general

	template <typename ITER, typename COLL>
	bool XferIterator( const char* key, ITER& iter, COLL& coll )
	{
		size_t offset = 0;
		if ( IsSaving() )
		{
			offset = std::distance( coll.begin(), iter );
		}
		bool rc = Xfer( key, offset );
		if ( IsRestoring() )
		{
			iter = coll.begin();
			std::advance( iter, offset );
		}
		return ( rc );
	}

	template <typename ITER>
	bool XferExistingColl( const char* key, ITER begin, ITER end )
	{
		bool success = EnterColl( key );
		if ( success )
		{
			for ( ITER i = begin ; i != end ; ++i )
			{
				if ( !AdvanceCollIter() || !Xfer( VALUE_STR, *i ) )
				{
					success = false;
				}
			}
		}
		LeaveColl();
		return ( success );
	}

	struct NullCheck
	{
		template <typename T>
		bool operator () ( const T& obj )
		{
			return ( obj != NULL );
		}
	};

	struct EmptyStringCheck
	{
		template <typename T>
		bool operator () ( const T& obj )
		{
			return ( !obj.empty() );
		}
	};

	struct AutoPtrCheck
	{
		template <typename T>
		bool operator () ( const T& obj )
		{
			return ( obj.get() != NULL );
		}
	};

	template <typename ITER, typename PRED>
	bool XferSparseArray( const char* key, ITER begin, ITER end, PRED existCheck )
	{
		if ( IsSaving() )
		{
			// first look for existing entry
			for ( ITER i = begin ; i != end ; ++i )
			{
				if ( existCheck( *i ) )
				{
					EnterColl( key );
					bool first = true;
					for ( ; i != end ; ++i )
					{
						if ( first || existCheck( *i ) )
						{
							first = false;
							AdvanceCollIter();
							int index = std::distance( begin, i );
							Xfer( "_index", index );
							Xfer( VALUE_STR, *i );
						}
					}
					LeaveColl();

					// always break out of here
					break;
				}
			}
		}
		else
		{
			if ( EnterColl( key ) )
			{
				size_t sz = std::distance( begin, end );
				while ( AdvanceCollIter() )
				{
					int index;
					Xfer( "_index", index );
					if ( (index >= 0) && (index < (int)sz) )
					{
						ITER i = begin;
						std::advance( i, index );
						Xfer( VALUE_STR, *i );
					}
				}
			}
			LeaveColl();
		}

		return ( true );
	}

	template <typename ITER>
	bool XferSparseArray( const char* key, ITER begin, ITER end )
	{
		return ( XferSparseArray( key, begin, end, NullCheck() ) );
	}

// Class (schema-based) persistence methods.

	template <typename T>
	bool XferClass( eXfer xfer, const char* key, T* obj )
	{
		std::auto_ptr <FuBi::Record> record( FuBi::CreateRecord( obj ) );
		return ( Xfer( xfer, key, *record ) );
	}

private:
	bool XferRecord ( eXfer xfer, const char* key, Record& record, const Record* defRecord );
	void InitContext( gpstring* reportOutput );

	struct CollEntry
	{
		int  m_Index;
		bool m_Entered;

		CollEntry( void )
			{  m_Index = 0;  m_Entered = false;  }
	};
	typedef std::stack <CollEntry, stdx::fast_vector <CollEntry> > CollStack;
	typedef std::stack <gpstring, stdx::fast_vector <gpstring> > StringStack;

	// spec
	PersistWriter* m_Writer;
	PersistReader* m_Reader;
	bool           m_IsFullXfer;
	bool           m_IsText;
	eXMode         m_XferMode;
	gpversion      m_Version;

	// state
	CollStack              m_CollStack;
	StringStack            m_BadNest;
	bool                   m_IsFatalError;
	bool                   m_IsWatsonError;
	int                    m_SeriousErrorCount;
	const gpstring*        m_ReportOutput;
	bool                   m_ReportOutputExternal;
	my ReportSys::Context* m_ReportContext;

#	if !GP_RETAIL
	int                    m_NestDepth;
#	endif // !GP_RETAIL

	SET_NO_COPYING( PersistContext );
};

//////////////////////////////////////////////////////////////////////////////
// helper macros

#define FUBI_XFER_BITFIELD_BOOL( PERSIST, NAME, VAR )							\
	{																			\
		bool FUBI_Var = VAR;													\
		PERSIST.Xfer( NAME, FUBI_Var );											\
		VAR = FUBI_Var;															\
	}

#define FUBI_XFER_BITFIELD_BOOL_DEF( PERSIST, NAME, VAR, DEF )					\
	{																			\
		bool FUBI_Var = VAR;													\
		PERSIST.Xfer( NAME, FUBI_Var, DEF );									\
		VAR = FUBI_Var;															\
	}

#define FUBI_XFER_BITFIELD_UNSIGNED( PERSIST, NAME, VAR )						\
	{																			\
		unsigned int FUBI_Var = VAR;											\
		PERSIST.Xfer( NAME, FUBI_Var );											\
		VAR = FUBI_Var;															\
	}

#define FUBI_XFER_BITFIELD_UNSIGNED_DEF( PERSIST, NAME, VAR, DEF )				\
	{																			\
		unsigned int FUBI_Var = VAR;											\
		PERSIST.Xfer( NAME, FUBI_Var, DEF );									\
		VAR = FUBI_Var;															\
	}

#define FUBI_XFER_BITFIELD_SIGNED( PERSIST, NAME, VAR )							\
	{																			\
		int FUBI_Var = VAR;														\
		PERSIST.Xfer( NAME, FUBI_Var );											\
		VAR = FUBI_Var;															\
	}

#define FUBI_XFER_BITFIELD_SIGNED_DEF( PERSIST, NAME, VAR, DEF )				\
	{																			\
		int FUBI_Var = VAR;														\
		PERSIST.Xfer( NAME, FUBI_Var, DEF );									\
		VAR = FUBI_Var;															\
	}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace FuBi

#endif  // __FUBIPERSIST_H

//////////////////////////////////////////////////////////////////////////////
