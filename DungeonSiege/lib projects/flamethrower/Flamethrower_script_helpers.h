#pragma once
//
//	Flamethrower_script_helpers.h
//
//	Helper classes for script stuff
//

#ifndef _FLAMETHROWER_SCRIPT_HELPERS_H_
#define	_FLAMETHROWER_SCRIPT_HELPERS_H_

class Flamethrower_script;

//
// --- Flamethrower Params
//

class Flamethrower_params
{
public:
	struct param;
	typedef stdx::fast_vector< param >	ParamColl;


	enum PARAM_TYPE
	{
		SET_BEGIN_ENUM( PT_, 0 ),

		PT_INVALID,

		// basic types
		PT_INT,				// i
		PT_UINT32,			// u
		PT_FLOAT,			// f
		PT_STRING,			// s
		PT_VECTOR,			// v
		PT_POSITION,		// p

		// extended types
		PT_COMMAND_PARAM,	// cast not allowed
		PT_SCRIPT_PARAM,	// cast not allowed
		PT_SFX_PARAMS,		// cast not allowed
		PT_MACRO,			// cast not allowed
		PT_VARIABLE,		// cast not allowed
		PT_TRACKER,			// cast not allowed

		SET_END_ENUM( PT_ ),
	};

	Flamethrower_params		( void ) {}
	Flamethrower_params		( const Flamethrower_params & source );
	~Flamethrower_params	( void );

	bool		Xfer		( FuBi::PersistContext &persist );
	void		Xfer		( FuBi::BitPacker& packer );
	void		operator=	( const Flamethrower_params & source );

#	if !GP_RETAIL
	gpstring	ToString	( void ) const;
#	endif // !GP_RETAIL

	void		Clear		( void );
	bool		Clear		( UINT32 name );

	UINT32		Add			( int val );
	UINT32		Add			( UINT32 val );
	UINT32		Add			( float val );
	UINT32		Add			( const gpstring & val );
	UINT32		Add			( const vector_3 & val );
	UINT32		Add			( const SiegePos & val );


	int	&		_int		( UINT32 name );
	int	&		_tracker	( UINT32 name );
							
	UINT32 &	_uint32		( UINT32 name );
	UINT32 &	_cparam		( UINT32 name );
	UINT32 &	_sparam		( UINT32 name );
	UINT32 &	_macro		( UINT32 name );

	float &		_float		( UINT32 name );

	gpstring &	_gpstring	( UINT32 name );
	gpstring &	_variable	( UINT32 name );
	gpstring &	_sfxparam	( UINT32 name );

	vector_3 &	_vector_3	( UINT32 name );
							
	SiegePos &	_SiegePos	( UINT32 name );

							
	PARAM_TYPE	GetType		( UINT32 name );
	bool		SetType		( UINT32 name, PARAM_TYPE new_pt );

	bool &		DeferEval	( UINT32 name );
							
	bool		Replace		( UINT32 name, UINT32 new_name, const Flamethrower_params & new_params );
	bool		Replace		( UINT32 name, PARAM_TYPE pt, const char * pText );
							
	bool		Output		( UINT32 name, char * pOutput, int & count, UINT32 size = 1024 ) const;

	UINT32		Count		( void ) const		{ return m_Params.size(); }


	// helper struct to store an individual parameter

	struct param
	{
		PARAM_TYPE	type;

		union
		{
			int		i;
			UINT32	u;
			float	f;

			void	*p;
		};

		param( void );
		param( int value );
		param( UINT32 value );
		param( float value );
		param( const gpstring & value );
		param( const vector_3 & value );
		param( const SiegePos & value );

		bool		Xfer	( FuBi::PersistContext &persist );
		void		Xfer	( FuBi::BitPacker &packer );

		bool		bDeferEval;
	};

private:
	ParamColl	m_Params;
};


//
//  --- Flamethrower Macro
//

class Flamethrower_macro
{

protected:
	Flamethrower_macro						( const gpstring &sName ) : m_sName( sName ) {}

public:
	const gpstring&		Name				( void ) const						{ return m_sName; }
	virtual bool		Execute				( Flamethrower_script *pScript ) = 0;

	inline UINT32		GetTokenIndex		( void ) const						{ return m_TokenIndex; }
	void				SetTokenIndex		( UINT32 i )						{ m_TokenIndex = i; }

private:
	gpstring			m_sName;
	UINT32				m_TokenIndex;
};



//
// --- Flamethrower command
//

class Flamethrower_command
{

protected:

	Flamethrower_command					( const gpstring &sName ) : m_sName( sName ) {}

public:
	const gpstring&		Name				( void ) const						{ return m_sName; }

	inline UINT32		GetTokenIndex		( void ) const						{ return m_TokenIndex; }
	void				SetTokenIndex		( UINT32 i )						{ m_TokenIndex = i; }

	virtual bool		Execute				( Flamethrower_script *pScript, Flamethrower_params & params ) = 0;
	virtual bool		GetParamToken		( const char * sParam, UINT32 & token );

private:
	gpstring			m_sName;
	UINT32				m_TokenIndex;
};



//
//	--- Flamethrower line
//

struct Flamethrower_line
{
	enum LINE_TYPE
	{
		LT_INVALID,
		LT_COMMENT,

		LT_BLOCK_BEGIN,
		LT_BLOCK_END,
		LT_BLOCK_END_ELSE,

		LT_IF,
		LT_ELSE,

		LT_COMMAND,

		LT_EOF
	};


	Flamethrower_line( void ) : m_Type( LT_INVALID ), m_CommandIndex( 0 ), m_BlockSkipOffset( 0 ) {}

	bool		Xfer		( FuBi::PersistContext &persist );


	LINE_TYPE				m_Type;
	UINT32					m_CommandIndex;
	Flamethrower_params		m_Params;

	UINT32					m_BlockSkipOffset;
};



#endif	// _FLAMETHROWER_SCRIPT_HELPERS_H_
