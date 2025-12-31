//
// Flamethrower_script_helpers.cpp
//
//

#include "precomp_flamethrower.h"
#include "gpcore.h"

#include "FuBiBitPacker.h"


//
// --- Flamethrower_params
//

Flamethrower_params::Flamethrower_params( const Flamethrower_params & source )
{
	*this = source;
}


Flamethrower_params::~Flamethrower_params( void )
{
	Clear();
}


bool
Flamethrower_params::Xfer( FuBi::PersistContext &persist )
{
	persist.XferVector( "m_Params", m_Params );

	return true;
}


void
Flamethrower_params::Xfer( FuBi::BitPacker& packer )
{
	int size = m_Params.size();
	packer.XferCount( size );
	m_Params.resize( size );

	ParamColl::iterator i, ibegin = m_Params.begin(), iend = m_Params.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		i->Xfer( packer );
	}
}


#if !GP_RETAIL

gpstring Flamethrower_params::ToString( void ) const
{
	gpstring str;

	ParamColl::const_iterator i, ibegin = m_Params.begin(), iend = m_Params.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i != ibegin )
		{
			str += ',';
		}
		str.appendf( "%d:", i - ibegin );

		char buffer[ 200 ];
		int count = 0;
		if ( Output( i - ibegin, buffer, count, ELEMENT_COUNT( buffer ) ) )
		{
			str.append( buffer, count );
		}
		else
		{
			str.append( "<error>" );
		}
	}

	return ( str );
}

#endif // !GP_RETAIL


FUBI_DECLARE_CAST_TRAITS( Flamethrower_params::PARAM_TYPE, int );


FUBI_DECLARE_XFER_TRAITS( Flamethrower_params::param )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		return ( obj.Xfer( persist ) );
	}
};


bool
Flamethrower_params::param::Xfer( FuBi::PersistContext &persist )
{
	// xfer the type of param
	persist.Xfer		(	"type",			type		);
	persist.Xfer		(	"bDeferEval",	bDeferEval	);

	switch( type )
	{
		case PT_INT:
		case PT_TRACKER:
		{
			persist.Xfer(	"i",		i		);
		} break;

		case PT_UINT32:
		case PT_COMMAND_PARAM:
		case PT_SCRIPT_PARAM:
		case PT_MACRO:
		{
			persist.Xfer(	"u",		u		);
		} break;

		case PT_FLOAT:
		{
			persist.Xfer(	"f",		f		);
		} break;

		case PT_STRING:
		case PT_VARIABLE:
		case PT_SFX_PARAMS:
		{
			if( persist.IsRestoring() )
			{
				p = new gpstring;
			}
			persist.Xfer(	"string",	*((gpstring*)(p)) );
		} break;

		case PT_VECTOR:
		{
			if( persist.IsRestoring() )
			{
				p = new vector_3;
			}
			persist.Xfer(	"vector",	*((vector_3*)(p)) );
		} break;

		case PT_POSITION:
		{
			if( persist.IsRestoring() )
			{
				p = new SiegePos;
			}
			persist.Xfer(	"siegepos",	*((SiegePos*)(p)) );
		} break;
	}

	return true;
}


void
Flamethrower_params::param::Xfer( FuBi::BitPacker &packer )
{
	packer.XferRaw( type, FUBI_MAX_ENUM_BITS( PT_ ) );

	switch ( type )
	{
		case PT_INT:
		case PT_TRACKER:
		case PT_UINT32:
		case PT_COMMAND_PARAM:
		case PT_SCRIPT_PARAM:
		case PT_MACRO:
		{
			packer.XferCount( u );
		}
		break;

		case PT_FLOAT:
		{
			packer.XferIf( f );
		}
		break;

		case PT_STRING:
		case PT_VARIABLE:
		case PT_SFX_PARAMS:
		{
			if ( packer.IsRestoring() )
			{
				p = new gpstring;
			}
			gpstring& str = *(gpstring*)p;
			const char* dummy = str;
			packer.XferString( dummy, str );
		}
		break;

		case PT_VECTOR:
		{
			if ( packer.IsRestoring() )
			{
				p = new vector_3;
			}
			packer.XferIf( *(vector_3*)p );
		}
		break;

		case PT_POSITION:
		{
			if ( packer.IsRestoring() )
			{
				p = new SiegePos;
			}
			packer.XferIf( *(SiegePos*)p );
		}
		break;
	}
}


void
Flamethrower_params::operator=( const Flamethrower_params & source )
{
	// Clear existing information
	Clear();

	// Reserve space for new parameters
	m_Params.reserve( source.m_Params.size() );

	// Iterate through each parameter and add it to the collection	
	ParamColl::const_iterator it = source.m_Params.begin(), it_end = source.m_Params.end();
	for( ; it != it_end; ++it )
	{
		param* pParam = &*m_Params.push_back();

		switch( (*it).type )
		{
			case PT_INT:
			case PT_TRACKER:
			case PT_UINT32:
			case PT_COMMAND_PARAM:
			case PT_SCRIPT_PARAM:
			case PT_MACRO:
			case PT_FLOAT:
			{
				*pParam = (*it);
			} break;

			case PT_STRING:
			case PT_VARIABLE:
			case PT_SFX_PARAMS:
			{
				pParam->p			= new gpstring( *((gpstring*)(*it).p) );
				pParam->type		= (*it).type;
				pParam->bDeferEval	= (*it).bDeferEval;
			} break;

			case PT_VECTOR:
			{
				pParam->p			= new vector_3( *((vector_3*)(*it).p) );
				pParam->type		= PT_VECTOR;
			} break;

			case PT_POSITION:
			{
				pParam->p			= new SiegePos( *((SiegePos*)(*it).p) );
				pParam->type		= PT_POSITION;
			} break;

			default:
			{
				gpassert( !"Flamethrower_params: cannot copy unknown type\n" );
			}
		}
	}
}


void
Flamethrower_params::Clear( void )
{
	if( !m_Params.empty() )
	{
		ParamColl::const_iterator it = m_Params.begin(), it_end = m_Params.end();
		for( UINT32 name = 0; it != it_end; ++it, ++name )
		{
			Clear( name );
		}
		m_Params.clear();
	}
}


bool
Flamethrower_params::Clear( UINT32 name )
{
	gpassert( name < m_Params.size() );

	if( name < m_Params.size() )
	{
		param * fp = &m_Params[ name ];

		switch( fp->type )
		{
			case PT_INT:
			case PT_TRACKER:
			{
				fp->i = 0;
			} break;

			case PT_UINT32:
			case PT_COMMAND_PARAM:
			case PT_SCRIPT_PARAM:
			case PT_MACRO:
			{
				fp->u = 0;
			} break;

			case PT_FLOAT:
			{
				fp->f = 0;
			} break;

			case PT_STRING:
			case PT_VARIABLE:
			case PT_SFX_PARAMS:
			{
				gpstring *p = (gpstring *)fp->p;
				delete p;
			} break;

			case PT_VECTOR:
			{
				vector_3 *p = (vector_3 *)fp->p;
				delete p;
			} break;

			case PT_POSITION:
			{
				SiegePos *p = (SiegePos *)fp->p;
				delete p;
			} break;

			default:
			{
				fp->p = NULL;
				return false;
			}
		}

		fp->p = NULL;
		return true;
	}
	return false;
}


#define DEFINE_ADD( TYPE )															\
UINT32 Flamethrower_params::Add( TYPE val )											\
{ m_Params.push_back( param( val ) ); return (m_Params.size() - 1); }

DEFINE_ADD( int );
DEFINE_ADD( UINT32 );
DEFINE_ADD( float );
DEFINE_ADD( const gpstring & );
DEFINE_ADD( const vector_3 & );
DEFINE_ADD( const SiegePos & );

static int		dummy_int	 = 0;
static UINT32	dummy_UINT32 = 0;
static float	dummy_float	 = 0;
static gpstring dummy_gpstring;
static vector_3 dummy_vector_3;
static SiegePos dummy_SiegePos;

#define DEFINE_PARAM_REF_V( NAME, TYPE, PT_TYPE, MEMBER )					\
	TYPE & Flamethrower_params::##NAME ( UINT32 name )						\
	{gpassert( m_Params[ name ].type == PT_TYPE );							\
	if( (name < m_Params.size()) && (m_Params[ name ].type == PT_TYPE) )	\
	{return m_Params[ name ].##MEMBER;} return dummy_##TYPE;}

#define DEFINE_PARAM_REF_P( NAME, TYPE, PT_TYPE )							\
	TYPE & Flamethrower_params::##NAME ( UINT32 name )						\
	{gpassert( m_Params[ name ].type == PT_TYPE );							\
	if( (name < m_Params.size()) && (m_Params[ name ].type == PT_TYPE) )	\
	{return *((TYPE*)(m_Params[ name ].p)); }return dummy_##TYPE;}

DEFINE_PARAM_REF_V( _int,		int,		PT_INT,				i );
DEFINE_PARAM_REF_V( _tracker,	int,		PT_TRACKER,			i );
DEFINE_PARAM_REF_V( _uint32,	UINT32,		PT_UINT32,			u );
DEFINE_PARAM_REF_V( _cparam,	UINT32,		PT_COMMAND_PARAM,	u );
DEFINE_PARAM_REF_V( _sparam,	UINT32,		PT_SCRIPT_PARAM,	u );
DEFINE_PARAM_REF_V( _macro,		UINT32,		PT_MACRO,			u );
DEFINE_PARAM_REF_V( _float,		float,		PT_FLOAT,			f );

DEFINE_PARAM_REF_P( _gpstring,	gpstring,	PT_STRING		);
DEFINE_PARAM_REF_P( _variable,	gpstring,	PT_VARIABLE		);
DEFINE_PARAM_REF_P( _sfxparam,	gpstring,	PT_SFX_PARAMS	);
DEFINE_PARAM_REF_P( _vector_3,	vector_3,	PT_VECTOR		);
DEFINE_PARAM_REF_P( _SiegePos,	SiegePos,	PT_POSITION		);


Flamethrower_params::PARAM_TYPE
Flamethrower_params::GetType( UINT32 name )
{
	gpassert( name < m_Params.size() );

	if( name < m_Params.size() )
	{
		return m_Params[ name ].type;
	}

	return PT_INVALID;
}


bool
Flamethrower_params::SetType( UINT32 name, Flamethrower_params::PARAM_TYPE new_pt )
{
	gpassert( name < m_Params.size() );

	if( name < m_Params.size() )
	{
		param * fp = &m_Params[ name ];

		PARAM_TYPE old_pt = fp->type;

		bool bCanCast = false;

		if( old_pt == new_pt )
		{
			return true;
		}
		else if( ((old_pt == PT_INT) || (old_pt == PT_TRACKER)) &&
				 ((new_pt == PT_INT) || (new_pt == PT_TRACKER)) )
		{
			bCanCast = true;
		}
		else if( ((old_pt == PT_UINT32) || (old_pt == PT_COMMAND_PARAM) || (old_pt == PT_SCRIPT_PARAM) || (old_pt == PT_MACRO)) &&
				 ((new_pt == PT_UINT32) || (new_pt == PT_COMMAND_PARAM) || (new_pt == PT_SCRIPT_PARAM) || (new_pt == PT_MACRO)) )
		{
			bCanCast = true;
		}
		else if( old_pt == PT_FLOAT )
		{
			if( (new_pt == PT_UINT32) || (new_pt == PT_COMMAND_PARAM) || (new_pt == PT_SCRIPT_PARAM) || (new_pt == PT_MACRO) )
			{
				// float to UINT32
				fp->u = (UINT32)_float(name);
				bCanCast = true;
			}
			else if( (new_pt == PT_INT) || (new_pt == PT_TRACKER) )
			{
				// float to int
				fp->i = (UINT32)_float(name);
				bCanCast = true;
			}
		}
		else if( (old_pt == PT_UINT32) || (old_pt == PT_COMMAND_PARAM) || (old_pt == PT_SCRIPT_PARAM) || (old_pt == PT_MACRO) )
		{
			if( new_pt == PT_FLOAT )
			{
				// UINT32 to float
				fp->f = (float)_uint32(name);
				bCanCast = true;
			}
			else if( (new_pt == PT_INT) || (new_pt == PT_TRACKER) )
			{
				// UINT32 to int
				fp->i = (int)_uint32(name);
				bCanCast = true;
			}
		}
		else if( (old_pt == PT_INT) || (old_pt == PT_TRACKER) )
		{
			if( (new_pt == PT_UINT32) || (new_pt == PT_COMMAND_PARAM) || (new_pt == PT_SCRIPT_PARAM) || (new_pt == PT_MACRO) )
			{
				// int to UINT32
				fp->u = (UINT32)_int(name);
				bCanCast = true;
			}
			else if(new_pt == PT_FLOAT) 
			{
				// int to float
				fp->f = (float)_int(name);
				bCanCast = true;
			}
		}
		else if( ((old_pt == PT_STRING) || (old_pt == PT_VARIABLE) || (old_pt == PT_SFX_PARAMS)) && 
				 ((new_pt == PT_STRING) || (new_pt == PT_VARIABLE) || (new_pt == PT_SFX_PARAMS)) )
		{
			bCanCast = true;
		}
		else if( ((old_pt == PT_VECTOR) ) &&
				 ((new_pt == PT_VECTOR) ) )
		{
			bCanCast = true;
		}
		else if( ((old_pt == PT_POSITION) ) &&
				 ((new_pt == PT_POSITION) ) )
		{
			bCanCast = true;
		}

		if( bCanCast )
		{
			fp->type = new_pt;
			return true;
		}
	}

	return false;
}


static bool dummy_bool = false;

bool &
Flamethrower_params::DeferEval( UINT32 name )
{
	gpassert( name < m_Params.size() );

	if( name < m_Params.size() )
	{
		if( m_Params[ name ].type == PT_STRING )
		{
			return m_Params[ name ].bDeferEval;
		}
	}

	dummy_bool = false;
	return dummy_bool;
}


bool
Flamethrower_params::Replace( UINT32 name, UINT32 new_name, const Flamethrower_params & new_params )
{
	gpassert( new_name < new_params.m_Params.size() );

	if( new_name < new_params.m_Params.size() )
	{
		if( Clear( name ) )
		{
			param * op = &m_Params[ name ];
			const param * np = &new_params.m_Params[ new_name ];

			switch( np->type )
			{
				case PT_INT:
				case PT_TRACKER:
				{
					op->i = np->i;
				} break;

				case PT_UINT32:
				case PT_COMMAND_PARAM:
				case PT_SCRIPT_PARAM:
				case PT_MACRO:
				{
					op->u = np->u;
				} break;

				case PT_FLOAT:
				{
					op->f = np->f;
				} break;

				case PT_STRING:
				case PT_VARIABLE:
				case PT_SFX_PARAMS:
				{
					if( (op->type == PT_STRING) || (op->type == PT_VARIABLE) || (op->type == PT_SFX_PARAMS) )
					{
						delete op->p;
					}
					op->p = new gpstring( *((gpstring*)(np->p)) );
				} break;

				case PT_VECTOR:
				{
					op->p = new vector_3( *((vector_3*)(np->p)) );
				} break;

				case PT_POSITION:
				{
					op->p = new SiegePos( *((SiegePos*)(np->p)) );
				} break;

				default:
				{
					op->p = NULL;
					return false;
				}
			}
		
			op->type = np->type;
			return true;
		}
	}
	return false;
}


bool
Flamethrower_params::Replace( UINT32 name, Flamethrower_params::PARAM_TYPE pt, const char * pText )
{
	gpassert( name < m_Params.size() );

	if( name < m_Params.size() )
	{
		param * pVal = &m_Params[ name ];

		if ( !SetType( name, pt ) )
		{
			Clear( name );
		}

		switch( pt )
		{
			case PT_INT:
			case PT_TRACKER:
			{
				if( !FromString( pText, pVal->i ) )
				{
					return false;
				}
			} break;

			case PT_UINT32:
			case PT_COMMAND_PARAM:
			case PT_SCRIPT_PARAM:
			case PT_MACRO:
			{
				if( !FromString( pText, pVal->u ) )
				{
					return false;
				}
			} break;

			case PT_FLOAT:
			{
				if( !FromString( pText, pVal->f ) )
				{
					return false;
				}
			} break;

			case PT_STRING:
			case PT_VARIABLE:
			case PT_SFX_PARAMS:
			{
				if( (pVal->type == PT_STRING) || (pVal->type == PT_VARIABLE) || (pVal->type == PT_SFX_PARAMS) )
				{
					gpstring copy( pText );
					((gpstring*)(pVal->p))->assign( copy );
				}
				else
				{
					pVal->p	= new gpstring( pText );
				}

			} break;

			case PT_VECTOR:
			{
				if( pVal->type == PT_VECTOR )
				{
					vector_3 * pVect = (vector_3*)(pVal->p);

					if( EOF == ::sscanf( pText, "%g %g %g", &pVect->x, &pVect->y, &pVect->z ) )
					{
						return false;
					}
				}
				else
				{
					float x = 0, y = 0, z = 0;

					if( EOF == ::sscanf( pText, "%g %g %g", &x, &y, &z ) )
					{
						return false;
					}

					pVal->p = new vector_3( x, y, z );
				}

			} break;

			case PT_POSITION:
			{
				if( pVal->type == PT_POSITION )
				{
					SiegePos * pPos = (SiegePos*)(pVal->p);
					UINT32 guid = 0;

					if( EOF == ::sscanf( pText, "%g %g %g %u", &pPos->pos.x, &pPos->pos.y, &pPos->pos.z, &guid ) )
					{
						return false;
					}

					siege::database_guid node( guid );
					pPos->node = node;
				}
				else
				{
					vector_3 pos;
					UINT32 guid = 0;

					if( EOF == ::sscanf( pText, "%g %g %g %u", &pos.x, &pos.y, &pos.z, &guid ) )
					{
						return false;
					}

					siege::database_guid node( guid );
					pVal->p = new SiegePos( pos, node );
				}
			} break;

			default:
			{
				pVal->p = NULL;
				return false;
			}
		}

		pVal->type = pt;
		return true;
	}
	return false;
}


bool
Flamethrower_params::Output( UINT32 name, char * pOutput, int & count, UINT32 size ) const
{
	gpassert( name < m_Params.size() );

	if( name < m_Params.size() )
	{
		const param * fp = &m_Params[ name ];

		switch( fp->type )
		{
			case PT_INVALID:
			{
				return false;
			};

			case PT_INT:
			case PT_TRACKER:
			{
				count = ::_snprintf( pOutput, size, "%d", fp->i );
			} goto good;

			case PT_UINT32:
			case PT_COMMAND_PARAM:
			case PT_SCRIPT_PARAM:
			case PT_MACRO:
			{
				count = ::_snprintf( pOutput, size, "%u", fp->u );
			} goto good;

			case PT_FLOAT:
			{
				count = ::_snprintf( pOutput, size, "%g", fp->f );
			} goto good;

			case PT_STRING:
			case PT_VARIABLE:
			case PT_SFX_PARAMS:
			{
				count = ::_snprintf( pOutput, size, "%s", (*((gpstring*)(fp->p))).c_str() );
			} goto good;

			case PT_VECTOR:
			{
				const vector_3 & val = *(vector_3*)fp->p;

				count = ::_snprintf( pOutput, size, "v<%g %g %g>", val.x, val.y, val.z );
			} goto good;

			case PT_POSITION:
			{
				const SiegePos & val = *(SiegePos*)fp->p;

				count = ::_snprintf( pOutput, size, "p<%g %g %g %d>", val.pos.x, val.pos.y, val.pos.z, val.node.GetValue() );
			} goto good;
		}
		gpassert( !"Flamethrower_params : cannot output - unknown parameter type\n" );
good:

		// negative means overflow
		if ( count < 0 )
		{
			count = size - 1;
		}
		else if ( count == (int)size )
		{
			// can't have same size, must terminate with something
			--count;
		}

		// remove the null termination
		pOutput[ count ] = ' ';

		return true;
	}
	return false;
}


Flamethrower_params::param::param( void )
{
	type		= PT_INVALID;
	p			= NULL;
	bDeferEval	= false;
}


Flamethrower_params::param::param( int value )
{
	type		= Flamethrower_params::PT_INT;
	i			= value;
	bDeferEval	= false;
}


Flamethrower_params::param::param( UINT32 value )
{
	type		= Flamethrower_params::PT_UINT32;
	u			= value;
	bDeferEval	= false;
}


Flamethrower_params::param::param( float value )
{
	type		= Flamethrower_params::PT_FLOAT;
	f			= value; 
	bDeferEval	= false;
}


Flamethrower_params::param::param( const gpstring & value )
{
	type		= Flamethrower_params::PT_STRING;
	p			= new gpstring( value ); 
	bDeferEval	= false;
}


Flamethrower_params::param::param( const vector_3 & value )
{
	type		= Flamethrower_params::PT_VECTOR;
	p			= new vector_3( value );
	bDeferEval	= false;
}


Flamethrower_params::param::param( const SiegePos & value )
{
	type		= Flamethrower_params::PT_POSITION;
	p			= new SiegePos( value );
	bDeferEval	= false;
}




//
// --- Flamethrower_command
//

bool
Flamethrower_command::GetParamToken( const char * sParam, UINT32 & token )
{
	UNREFERENCED_PARAMETER( sParam );
	UNREFERENCED_PARAMETER( token );

	return false;
}


//
// --- Flamethrower_line
//

FUBI_DECLARE_SELF_TRAITS( Flamethrower_params );
FUBI_DECLARE_CAST_TRAITS( Flamethrower_line::LINE_TYPE, int );

bool
Flamethrower_line::Xfer( FuBi::PersistContext &persist )
{
	persist.Xfer	( "m_Type",				m_Type				);
	persist.Xfer	( "m_CommandIndex",		m_CommandIndex		);
	persist.Xfer	( "m_Params",			m_Params			);

	persist.Xfer	( "m_BlockSkipOffset",	m_BlockSkipOffset	);

	return true;
}
