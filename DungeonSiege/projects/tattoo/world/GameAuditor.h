/*=======================================================================================

	---------------------------------------------------------------------------------
	GameAuditor

	author:		Adam Swensen
	creation:	8/29/2000

	purpose:	Big Dummy bean counter.

	copyright(C) 2000 Gas Powered Games
	---------------------------------------------------------------------------------


	Tracked Game Events:
	---------------------
	EVENT			  TAG		KEY
	ge_player_kill    PlayerId	template_name
	ge_player_death	  "			"

=======================================================================================*/

#pragma once
#ifndef __GAMEAUDITOR_H
#define __GAMEAUDITOR_H


///////////////////////////////////////////////////////////////
//  these events are tracked on the server

enum eGameEvent
{
	SET_BEGIN_ENUM( GE_, 0 ),

	GE_PLAYER_KILL,
	GE_PLAYER_DEATH,

//	GE_PLAYER_EXPERIENCE_GAINED,
//	GE_PLAYER_ACQUIRED_ITEM,


	SET_END_ENUM( GE_ ),
};

const char * ToString( eGameEvent Type );
bool FromString( const char* str, eGameEvent & Type );

///////////////////////////////////////////////////////////////

class AuditorDb
{
public:

	AuditorDb()  {  m_XferDb = true;  }
	~AuditorDb() {}

	// persistence
	virtual bool Xfer( FuBi::PersistContext& persist );

	void SetXferDb( bool flag )			{  m_XferDb = flag;  }
	bool GetXferDb()					{  return m_XferDb;  }

	virtual void Reset();

	// Basic audits
	bool Get( const char * key, int & value      );
	bool Get( const char * key, bool & value     );
	bool Get( const char * key, float & value    );
	bool Get( const char * key, double & value	 );
	bool Get( const char * key, gpstring & value );

	void Set( const char * key, int value,  bool bRC = false );
	void Set( const char * key, bool value, bool bRC = false );
	void Set( const char * key, float value      );
	void Set( const char * key, double value	 );
	void Set( const char * key, const gpstring & value );

	int    Increment( const char * key, int value = 1 );
	float  IncrementFloat( const char * key, float value = 1 );
	double IncrementDouble( const char * key, double value = 1 );

	// Tagged audits
	bool Get( int tag, const char * key, int & value      );
	bool Get( int tag, const char * key, bool & value     );
	bool Get( int tag, const char * key, float & value    );
	bool Get( int tag, const char * key, double & value	  );
	bool Get( int tag, const char * key, gpstring & value );

	void Set( int tag, const char * key, int value,  bool bRC = false );
	void Set( int tag, const char * key, bool value, bool bRC = false );
	void Set( int tag, const char * key, float value      );
	void Set( int tag, const char * key, double value	  );
	void Set( int tag, const char * key, const gpstring & value );

	int    Increment( int tag, const char * key, int value = 1 );
	float  IncrementFloat( int tag, const char * key, float value = 1 );
	double IncrementDouble( int tag, const char * key, double value = 1 );

	// Specialized audits
	bool Get( eGameEvent event, int tag, const char * key, int & value )			{  return ( Get( tag, gpstring( ToString( event ) ).append( key ), value ) );  }
	void Set( eGameEvent event, int tag, const char * key, int value   )			{  Set( tag, gpstring( ToString( event ) ).append( key ), value );  }
	void Increment( eGameEvent event, int tag, const char * key, int value = 1 )	{  Increment( tag, gpstring( ToString( event ) ).append( key ), value );  }

	bool PlayerGet( int playerId, int tag, gpstring key, int & value )				{  return ( Get( tag, key.appendf( "%d", playerId ), value ) );  }
	void PlayerSet( int playerId, int tag, gpstring key, int value   )				{  Set( tag, key.appendf( "%d", playerId ), value );  }
	void PlayerIncrement( int playerId, gpstring key, int tag, int value = 1 )		{  Increment( tag, key.appendf( "%d", playerId ), value );  }

	// Skrit access
FEX	int             GetInt   ( const char * key )									{  int value;  Get( key, value );  return( value );  }
FEX	int             GetInt   ( int tag, const char * key )							{  int value;  Get( tag, key, value );  return( value );  }
FEX bool            GetBool  ( const char * key )									{  bool value;  Get( key, value );  return( value );  }
FEX bool            GetBool  ( int tag, const char * key )							{  bool value;  Get( tag, key, value );  return( value );  }
FEX float           GetFloat ( const char * key )									{  float value;  Get( key, value );  return( value );  }
FEX float           GetFloat ( int tag, const char * key )							{  float value;  Get( tag, key, value );  return( value );  }
FEX double			GetDouble( const char * key )									{  double value;  Get( key, value );  return( value );  }
FEX double			GetDouble( int tag, const char * key )							{  double value;  Get( tag, key, value );  return( value );  }
FEX const gpstring& GetString( const char * key )									{  static gpstring value;  Get( key, value );  return( value );  }
FEX const gpstring& GetString( int tag, const char * key )							{  static gpstring value;  Get( tag, key, value );  return( value );  }

FEX void SetInt   ( const char * key, int value )									{  Set( key, value );  }
FEX void SetInt   ( int tag, const char * key, int value )							{  Set( tag, key, value );  }
FEX void SetBool  ( const char * key, bool value )									{  Set( key, value );  }
FEX void SetBool  ( int tag, const char * key, bool value )							{  Set( tag, key, value );  }
FEX void SetFloat ( const char * key, float value )									{  Set( key, value );  }
FEX void SetFloat ( int tag, const char * key, float value )						{  Set( tag, key, value );  }
	void SetDouble( const char * key, double value )								{  Set( key, value );  }
	void SetDouble( int tag, const char * key, double value )						{  Set( tag, key, value );  }
FEX void SetString( const char * key, const gpstring & value )						{  Set( key, value );  }
FEX void SetString( int tag, const char * key, const gpstring & value )				{  Set( tag, key, value );  }

	// $ skrit can't speak doubles
FEX void FUBI_RENAME( SetDouble )( const char * key, float value )					{  SetDouble( key, value );  }
FEX void FUBI_RENAME( SetDouble )( int tag, const char * key, float value )			{  SetDouble( tag, key, value );  }

public:

	// GameEntry definition
	struct GameEntry
	{
		GameEntry() :	m_RC	( false ),
						m_Tag	( 0 ), 
						m_Int	( 0 ), 
						m_Bool	( 0 ),
						m_Float	( 0 ),
						m_Double( 0 )	{}
		
		bool		m_RC;

		gpstring	m_Key;
		int			m_Tag;

		int			m_Int;
		bool		m_Bool;
		float		m_Float;
		double		m_Double;
		gpstring	m_String;

		bool	Xfer( FuBi::PersistContext& persist );
	};

	typedef std::vector< GameEntry > GameEntryColl;

	// Entry maintenance
	void ClearEntries( int tag );

	void RemoveEntry( int tag, const char * key );

	GameEntryColl & GetEntries()	{  return m_Entries;  }

private:

	GameEntry * FindEntry( const char * key );
	GameEntry * FindTaggedEntry( const char * key, int tag );

	// Members
	GameEntryColl  m_Entries;

	bool		   m_XferDb;
};


///////////////////////////////////////////////////////////////

class GameAuditor : public AuditorDb, public Singleton< GameAuditor >
{
public:

	GameAuditor();
	~GameAuditor();

	// persistence
	virtual bool Xfer( FuBi::PersistContext& persist );

	virtual void Reset();

	void SSyncOnMachine( DWORD machineId );

	// auditor management
FEX	AuditorDb * AddDb( const char * name, bool xferDb = true );

FEX	AuditorDb * FindDb( const char * name );

FEX	AuditorDb * GetDb()		{  return this;  }

	// For multiplayer stats
	void SIncrement( eGameEvent event, int tag, const char * key, int value = 1 );

FEX void RCSet( const char * key, int value, DWORD machineId );
FEX void RCSet( int tag, const char * key, int value, DWORD machineId );
FEX void RCSetBool( const char * key, bool value, DWORD machineId );
FEX void RCSetBool( int tag, const char * key, bool value, DWORD machineId );

FEX void RCSet( const char * key, int value )					{  RCSet( key, value, RPC_TO_ALL );  }
FEX void RCSet( int tag, const char * key, int value )			{  RCSet( tag, key, value, RPC_TO_ALL );  }
FEX void RCSetBool( const char * key, bool value )				{  RCSetBool( key, value, RPC_TO_ALL );  }
FEX void RCSetBool( int tag, const char * key, bool value )		{  RCSetBool( tag, key, value, RPC_TO_ALL );  }


#if !GP_RETAIL
	
FEX	void Print( const char * name );

FEX void PrintDbList();

#endif


private:

	typedef std::map< gpstring, AuditorDb*, istring_less > AuditorMap;
	
	AuditorMap	m_Auditors;


	SET_NO_COPYING( GameAuditor );
	FUBI_CLASS_INHERIT( GameAuditor, AuditorDb );
	FUBI_SINGLETON_CLASS( GameAuditor, "We are game auditor." );
};

#define gGameAuditor GameAuditor::GetSingleton()


#endif
