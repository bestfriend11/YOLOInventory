#pragma once
#ifndef _SIEGE_DATABASE_GUID_
#define _SIEGE_DATABASE_GUID_
/* ========================================================================
   Header File: siege_database_guid.h
   Description: Declares siege::database_guid
   ======================================================================== */


/* ************************************************************************
   Class: database_guid
   Description: Uniquely identifies an object in a database with a
                32-bit number
   ************************************************************************ */

namespace siege
{
	class database_guid
	{
	public:
		// Existence
				 database_guid( void )					{  GetRawValue() = 0;  }
		explicit database_guid( const char* str );
		explicit database_guid( unsigned int GUID )		{  SetValue( GUID );  }

		// Information
		bool	 IsValid( void ) const					{  return ( GetRawValue() != 0 );  }
		bool	 IsValidSlow( void ) const;
		UINT32	 GetValue( void ) const;
		void	 SetValue( UINT32 GUID );

		gpstring ToString( void ) const;
		bool     FromString( const char* str );

	private:
		UINT32&       GetRawValue( void )				{  return ( *(UINT32*)m_Guid );  }
		const UINT32& GetRawValue( void ) const			{  return ( *(const UINT32*)m_Guid );  }

		friend bool operator == ( database_guid const &GUID0, database_guid const &GUID1 );
		friend bool operator != ( database_guid const &GUID0, database_guid const &GUID1 );
		friend bool operator <  ( database_guid const &GUID0, database_guid const &GUID1 );

		UINT8 m_Guid[4];
	};

	inline UINT32 database_guid::GetValue( void ) const
	{
		return( (m_Guid[0]<<24) + (m_Guid[1]<<16) + (m_Guid[2]<<8) + m_Guid[3] );
	}

	inline void database_guid::SetValue( UINT32 GUID )
	{
		m_Guid[0] = (UINT8)(( GUID & 0xff000000 ) >> 24);
		m_Guid[1] = (UINT8)(( GUID & 0x00ff0000 ) >> 16);
		m_Guid[2] = (UINT8)(( GUID & 0x0000ff00 ) >> 8 );
		m_Guid[3] = (UINT8)(( GUID & 0x000000ff )      );
	}

	// Comparison
	inline bool operator==(database_guid const &GUID0, database_guid const &GUID1)
		{  return ( GUID0.GetRawValue() == GUID1.GetRawValue() );  }

	inline bool operator!=(database_guid const &GUID0, database_guid const &GUID1)
		{  return ( GUID0.GetRawValue() != GUID1.GetRawValue() );  }

	inline bool operator<(database_guid const &GUID0, database_guid const &GUID1)
		{  return ( GUID0.GetRawValue() < GUID1.GetRawValue() );  }


	extern const database_guid UNDEFINED_GUID;
};

#endif
