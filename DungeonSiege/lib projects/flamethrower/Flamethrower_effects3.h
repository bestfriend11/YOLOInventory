#pragma once
#ifndef _FLAMETHROWER_EFFECTS_3_H_
#define _FLAMETHROWER_EFFECTS_3_H_
//
//	Flamethrower_effects3.h
//	Copyright 1999, Gas Powered Games
//
//



class	Flamethrower_effect_base;

#include "vector_3.h"
#include "RapiOwner.h"

#include "siege_pos.h"
#include "siege_database_guid.h"


//
// PolygonalExplosion effect
//

class PolygonalExplosion_Effect	:	public SpecialEffect
{
public:
	enum	{ MAX_PARTICLES	= 200 };
	enum	{ MAX_POLYSIDES = 100 };

private:
	struct ExplosionParticle
	{
		ExplosionParticle( void ) : alpha( 0 ), alpha_fade( 0 ), color( 0 ) {}

		matrix_3x3	orientation;
		vector_3	position;
		vector_3	velocity;

		vector_3	rotation;
		vector_3	rotation_delta;

		stdx::fast_vector< vector_3 >	verticies;
		UINT32		vertex_count;

		DWORD		color;

		float		alpha;
		float		alpha_fade;

		bool		Xfer( FuBi::PersistContext &persist );
	};

	stdx::fast_vector< ExplosionParticle >	m_Particles;

	float				m_RelativeGroundHeight;

	vector_3			m_Base_color;
	float				m_Fade_min;
	float				m_Fade_max;
	UINT32				m_PolySides;
	UINT32				m_PolyCount;
	float				m_Radius;
	
	bool				m_bDarkBlending;
	bool				m_bOmniDir;
	float				m_Magnitude;
	vector_3			m_RotationRange;
	vector_3			m_Color_variance;
	vector_3			m_Displace;

	stdx::fast_vector< sVertex >	m_DrawVerts;

public:

	PolygonalExplosion_Effect( void );

	void	RegisterDerivedParams( EffectParams &params );
	void	InitializeDerived( void );
	bool	OnXfer( FuBi::PersistContext &persist );

	void	UpdateDerived( void );
	void	DrawSelf( void );
	void	Put( void ) {}
	void	Track( void ) {}
	float	GetBoundingRadius( void );
};



//
// Lightsource effect
//

class Lightsource_Effect :	public SpecialEffect
{
	siege::database_guid
					m_Light_id;

	float			m_In_dur;
	float			m_Out_dur;
	float			m_InnerFalloff_radius;
	float			m_OuterFalloff_radius;

	float			m_Color0Weight;
	float			m_Color1Weight;

	float			m_Flicker;
	float			m_FlickerRate;

	vector_3        m_Color0;
	vector_3		m_Color1;

	bool			m_Visible;
	bool			m_bCastShadow;

public:

	Lightsource_Effect( void );
	~Lightsource_Effect( void );

	void	RegisterDerivedParams( EffectParams &params );
	void	InitializeDerived( void );
	bool	ReInitialize( void );
	bool	UnInitialize( void );
	bool	OnXfer( FuBi::PersistContext &persist );

	void	UpdateDerived( void );
	void	DrawSelf( void ) {};
	float	GetBoundingRadius( void )			{ return m_OuterFalloff_radius; }
};



//
// Sphere effect
//

class Sphere_Effect : public SpecialEffect
{
	enum	{ BASE_SIDES = 20 };

	bool			m_bDrawLines;
	bool			m_bCull;

	float			m_InDur;
	float			m_OutDur;
	float			m_Alpha;
	float			m_Sides;

	vector_3		m_CamDir;

	vector_3		m_RotationIncrements;


	vector_3		m_ScaleFactors;
	float			m_CurrentScaleFactor;
	UINT32			m_ScaleIndex;

	UINT32			m_Subdivision;

	vector_3		m_Base_color;
	vector_3		m_Line_color;

	sVertex			m_DrawVerts[ 4 ];

	vector_3		m_Verts[ 12 ];
	int				m_ConPts[ BASE_SIDES ][3];


	void			Subdivide( vector_3 v1, vector_3 v2, vector_3 v3, const int depth,
							const vector_3 color, const float alpha );

public:

	Sphere_Effect( void );

	void	RegisterDerivedParams( EffectParams &params );
	void	InitializeDerived( void );
	bool	OnXfer( FuBi::PersistContext &persist );
	
	void	UpdateDerived( void );
	void	DrawSelf( void );
	float	GetBoundingRadius( void )			{ return 0.5f * m_CurrentScaleFactor; }
};



//
// Flurry effect
//

class Flurry_Effect : public SpecialEffect
{
	enum			{ TEX_WIDTH = 64 };
	enum			{ TEX_HEIGHT = 64 };
	enum			{ TEX_RADIUS = 32 };

	enum			{ MAX_PARTICLES = 200 };

	struct	Particle
	{
		Particle( void ) : alpha( 0 ), sin_amplitude( 0 ), theta( 0 ), phi( 0 ), sin( 0 ),
			itheta( 0 ), iphi( 0 ), isin( 0 ) {}

		float		alpha;

		float		sin_amplitude;

		float		theta;
		float		phi;
		float		sin;

		float		itheta;
		float		iphi;
		float		isin;

		vector_3	position;

		bool		Xfer( FuBi::PersistContext &persist );
	};

	typedef stdx::fast_vector< Particle >	PColl;

	PColl			m_Particles;

	vector_3		m_Base_color;
	float			m_Radius;

	UINT32			m_ParticleCount;
	float			m_Size;
	float			m_Alpha;
	float			m_In_dur;
	float			m_Out_dur;

	float			m_PhiSpeed;
	float			m_ThetaSpeed;
	float			m_SinAmp;
	float			m_SinSpeed;

	vector_3		m_GrowthFactors;
	float			m_CurrentGrowthFactor;
	UINT32			m_GrowthIndex;

	DWORD			m_Color0, m_Color1, m_Color2;


public:
	Flurry_Effect( void );

	void	RegisterDerivedParams( EffectParams &params );
	void	InitializeDerived( void );
	UINT32	InitializeTexture( void );
	bool	OnXfer( FuBi::PersistContext &persist );
	
	void	UpdateDerived( void );
	void	DrawSelf( void );

	float	GetBoundingRadius( void );
};



//
// Charge effect
//

class	Charge_Effect : public SpecialEffect
{
	enum			{ TEX_WIDTH = 64 };
	enum			{ TEX_HEIGHT = 64 };
	enum			{ TEX_RADIUS = 32 };

	enum			{ MAX_PARTICLES = 200 };

	struct	Particle
	{
		Particle( void ) : active( false ), delay( 0 ), alpha( 0 ), ialpha( 0 ) {}
		bool		active;

		float		delay;

		float		alpha;
		float		ialpha;

		vector_3	position;
		vector_3	velocity;
		vector_3	accel;

		bool		Xfer( FuBi::PersistContext &persist );
	};

	stdx::fast_vector< Particle >	m_Particles;
	sVertex			m_DrawVerts[ 4 ];

	float			m_CenterIntensity;
	UINT32			m_ActiveCount;

	vector_3		m_Base_color;
	float			m_Radius;
	UINT32			m_ParticleCount;
	float			m_Size;
	float			m_Speed;
	float			m_CenterSize;
	float			m_Alpha;
	float			m_IAlpha;
	float			m_Out_dur;
	float			m_OutElapsed;

public:
	Charge_Effect( void );

	void	RegisterDerivedParams( EffectParams &params );
	void	InitializeDerived( void );
	UINT32	InitializeTexture( void );
	bool	OnXfer( FuBi::PersistContext &persist );

	void	UpdateDerived( void );
	void	DrawSelf( void );

	float	GetBoundingRadius( void );
};



//
// Sparkles effect
//
class Sparkles_Effect : public SpecialEffect
{
public:
	enum			{ MAX_PARTICLES = 200 };

private:
	struct	Particle
	{
		Particle( void ) : active( false ), fading_in( false ), alpha( 0 ), ialpha( 0 ),
			delay( 0 ) {}

		SiegePos	position;
		bool		active;
		bool		fading_in;
		float		alpha;
		float		ialpha;
		float		delay;

		bool		Xfer( FuBi::PersistContext &persist );
	};

	typedef std::list< Particle >	PColl;

	struct	Particle_list
	{
		siege::database_guid	guid;
		PColl					particles;

		bool		Xfer( FuBi::PersistContext &persist );
	};

	typedef std::list< Particle_list >	PLColl;

	PLColl			m_Particles;
	PLColl			m_InsertList;

	vector_3		m_Base_color;
	float			m_Radius;
	float			m_Size;
	UINT32			m_ParticleCount;

	float			m_PSize;
	float			m_YVelocity;

	bool			m_bDone;

	sVertex			m_DrawVerts[ 4 ];

	void	CreateParticle( Particle &p, float delay = 0 );

public:
	Sparkles_Effect( void );


	void	RegisterDerivedParams( EffectParams &params );
	void	InitializeDerived( void );
	UINT32	InitializeTexture( void );
	bool	OnXfer( FuBi::PersistContext &persist );

	void	UpdateDerived( void );
	void	DrawSelf( void );

	void	Put( void ){}

	float	GetBoundingRadius( void );
};



//
// DiscPile effect
//

class DiscPile_Effect : public SpecialEffect
{
	enum			{ TEX_WIDTH = 32 };
	enum			{ TEX_HEIGHT = 32 };
	enum			{ TEX_RADIUS = 16 };

	enum			{ MAX_DISCS = 100 };
	enum			{ MAX_SIDES = 32*2 };

	struct	Disc
	{
		vector_3	position;
		vector_3	velocity;
		vector_3	rotation;
		vector_3	irotation;

		bool		Xfer( FuBi::PersistContext &persist );
	};

	Disc			m_Discs[ MAX_DISCS ];

	sVertex			m_SideVerts[ 2*(MAX_SIDES+1) ];
	sVertex			m_TopVerts[ 1+MAX_SIDES ];
	sVertex			m_BottomVerts[ 1+MAX_SIDES ];

	vector_3		m_SideNorms[ 2*(MAX_SIDES+1) ];
	vector_3		m_TopNorms[ 1+MAX_SIDES ];
	vector_3		m_BottomNorms[ 1+MAX_SIDES ];

	bool			m_bHeightIndexed;
	float			m_RelativeGroundHeight;
	vector_3		m_Base_color;
	float			m_Radius;
	float			m_DiscRadius;
	float			m_ThicknessFactor;
	float			m_Size;

	UINT32			m_Sides;
	UINT32			m_DiscCount;

	vector_3		m_MinVel;
	vector_3		m_MaxVel;

public:
	DiscPile_Effect( void );

	void	RegisterDerivedParams( EffectParams &params );
	void	InitializeDerived( void );
	bool	OnXfer( FuBi::PersistContext &persist );

	void	Put( void ){}
	void	Track( void ){}
	void	UpdateDerived( void );
	void	DrawSelf( void );
	float	GetBoundingRadius( void );
};



//
// Cylinder effect
//

class Cylinder_Effect : public SpecialEffect
{
	enum			{ MAX_SEGMENTS = 48 };

	vector_3		m_Base_color;
	UINT32			m_Segments;

	float			m_Upper_radius;
	float			m_Upper_radius_limit;
	float			m_Upper_radius_inc;

	float			m_Lower_radius;
	float			m_Lower_radius_limit;
	float			m_Lower_radius_inc;

	float			m_Upper_height;
	float			m_Upper_height_limit;
	float			m_Upper_height_inc;

	float			m_Lower_height;
	float			m_Lower_height_limit;
	float			m_Lower_height_inc;

	float			m_SpinRate;
	float			m_Alpha;
	float			m_MaxAlpha;
	float			m_AlphaFade;

	float			m_In_dur;
	float			m_Out_dur;

	float			m_Theta;
	float			m_ThetaI;
	float			m_SpinPos;

	bool			m_bDarkBlending;

	matrix_3x3		m_Rotation;
	vector_3		m_iRotation;
	vector_3		m_sRotation;

	matrix_3x3		m_RenderRotation;


	stdx::fast_vector< sVertex >	m_DrawVerts;

public:
	Cylinder_Effect( void );

	void	RegisterDerivedParams( EffectParams &params );
	void	InitializeDerived( void );
	bool	OnXfer( FuBi::PersistContext &persist );

	void	UpdateDerived( void );
	void	DrawSelf( void );

	float	GetBoundingRadius( void );
};



//
// PointTracer effect
//

class PointTracer_Effect : public SpecialEffect
{
public:
	enum			{ MAX_TRACERS = 400 };

	struct	Tracer
	{
		Tracer( void ) : time_index( 0 ), alpha( 0 ) {}
		bool		Xfer( FuBi::PersistContext &persist );

		float		time_index;
		SiegePos	position;
		float		alpha;
	};

private:

	stdx::fast_vector< Tracer >	m_Tracers;

	vector_3		m_Base_color;
	float			m_Fade_rate;
	float			m_Decay_rate;
	float			m_Velocity;
	float			m_Frequency;
	float			m_Period;
	float			m_Impact_t;

	SiegePos		m_Target_position;
	SiegePos		m_Spawn_position;

	UINT32			m_Tracer_limit;
	UINT32			m_Tracer_count;
	UINT32			m_Tracer_index;
	int				m_Last_index;

	sVertex			m_DrawVerts[ 4 ];

public:
	PointTracer_Effect( void );

	void	RegisterDerivedParams( EffectParams &params );
	void	InitializeDerived( void );
	bool	OnXfer( FuBi::PersistContext &persist );

	void	Put( void ){}
	void	Track( void ){}
	void	UpdateDerived( void );
	void	DrawSelf( void );

	float	GetBoundingRadius( void );
};



#endif //_FLAMETHROWER_EFFECTS_3_H_
