#pragma once
#ifndef _FLAMETHROWER_EFFECTS_2_H_
#define _FLAMETHROWER_EFFECTS_2_H_
//
//	Flamethrower_effects2.h
//	Copyright 1999, Gas Powered Games
//
//



class Flamethrower_effect_base;

#include "vector_3.h"
#include "RapiOwner.h"

#include "siege_pos.h"
#include "siege_database_guid.h"


class LineTracer_Effect : public SpecialEffect
{
	struct	Trace_element
	{
		Trace_element	( void ) : alpha( 1.0f ) {}
		Trace_element	( const SiegePos &pos_a, const SiegePos &pos_b )
						: alpha( 1.0f ), pos_a( pos_a ), pos_b( pos_b ) {}

		bool	Xfer	( FuBi::PersistContext &persist );

		float			alpha;

		SiegePos		pos_a;
		SiegePos		pos_b;
	};

	typedef stdx::fast_vector< Trace_element >	TraceColl;

	vector_3			m_Base_color;
	float				m_Fade_rate;
	float				m_fade_in;
	float				m_fade_out;
	float				m_Alpha_scalar;
	bool				m_bLockToTarget;

	SiegePos			m_RefPos,	m_RefPosNext;

	DWORD				m_color0, m_color1;

	VertColl			m_DrawVerts;
	TraceColl			m_Segments;

	SiegePos			m_CullPosition;

public:

	LineTracer_Effect		( void );

	void	RegisterDerivedParams	( EffectParams &params );
	void	InitializeDerived		( void );
	bool	OnXfer					( FuBi::PersistContext &persist );

	void	Put				( void ){}
	void	Track			( void ){}
	void	UpdateDerived	( void );
	void	DrawSelf		( void );

	inline virtual bool	GetCullPosition( SiegePos & CullPosition );
};


class FireB_Effect : public SpecialEffect
{
	enum	{ MAX_FLAMES = 600 };
	enum	{ TEX_WIDTH	 = 64 };
	enum	{ TEX_HEIGHT = 64 };
	enum	{ TEX_RADIUS = 32 };

	vector_3		m_Base_color;
	vector_3		m_Acceleration;
	vector_3		m_Velocity;
	float			m_AlphaFade;
	float			m_FlameSize;
	float			m_MinFlames;
	float			m_MaxFlames;
	bool			m_bCheap;
	bool			m_bDarkBlending;
	bool			m_bConstantVelocity;
	float			m_Velocity_scalar;

	float			m_Lower_r0;
	float			m_Lower_r1;	
	float			m_Upper_r0;	
	float			m_Upper_r1;
	
	float			m_DisplaceMin;
	float			m_DisplaceMax;
	float			m_BurnStartRate;
	float			m_BurnEndRate;

	vector_3		m_FDamage;

	float			m_FlameSize_min;
	float			m_FlameSize_max;
	float			m_FlameSize_inc;

	float			m_OBB_freq;
	float			m_OBB_time;
	
	vector_3		m_OBB_half_diag;
	SiegePos		m_OBB_center;

	SiegePos		m_LastSpawnTarget;

	vector_3		m_MinimumCorner;
	vector_3		m_MaximumCorner;

	bool			m_bOBBReady;

	struct	Flame
	{
		Flame( void ) : alpha( 0 ), s( 0 ), light_particle( false ) {}

		SiegePos	position;
		vector_3	velocity;
		vector_3	acceleration;

		float		alpha;
		float		s;

		bool		light_particle;
		siege::database_guid
					light_id;

		bool		Xfer( FuBi::PersistContext &persist );
	};
	typedef	stdx::fast_vector< Flame >		def_flame;


	struct	Flame_list
	{
		siege::database_guid guid;
		stdx::fast_vector< Flame >	flames;

		bool		Xfer( FuBi::PersistContext &persist );
	};
	typedef stdx::fast_vector< Flame_list >	def_flame_list;


	def_flame_list		m_Flames;
	def_flame_list		m_Insert;


	stdx::fast_vector< sVertex>	m_DrawVerts;

	float				m_FlameCount;
	bool				m_bIgnite;
	bool				m_bDamage;

	bool				m_bLightSpawn;
	UINT32				m_Light_particle_frequency;
	UINT32				m_Light_particle_count;
	float				m_light_radius;


	bool				ComputeOBB( const matrix_3x3 &EigenVec, SiegePos &center, vector_3 &half_diag );

	inline virtual bool	GetCullPosition( SiegePos & CullPosition );

public:

	FireB_Effect	( void );
	~FireB_Effect	( void );
	
	void	RegisterDerivedParams	( EffectParams &params );
	void	InitializeDerived		( void );
	UINT32	InitializeTexture		( void );
	bool	ReInitialize			( void );
	bool	UnInitialize			( void );

	bool	OnXfer( FuBi::PersistContext &persist );

	void	Track				( void );
	void	Put					( void ) {}
	void	UpdateDerived		( void );
	void	DrawSelf			( void );

	float	GetBoundingRadius	( void );

private:
	bool	CreateNewFlame		( Flame &flame );
	bool	AddFlameToBounds	( Flame const &flame );
};



class Curve_Effect : public SpecialEffect
{
	vector_3	m_Base_color;
	DWORD		m_color_0;
	float		m_Size;
	float		m_Step;
	float		m_Curvature;
	UINT32		m_Model;
	float		m_TrailLength;
	float		m_Velocity;
	float		m_CurvePos;
	float		m_Spacing;

	bool		m_bPointsChosen;

	SiegePos	m_RefPos;
	vector_3	m_P0, m_P1, m_P2, m_P3;

	sVertex		m_DrawVerts[ 4 ];

	stdx::fast_vector< vector_3 >	m_Points;

	// do not xfer
	SiegePos	m_LastStartPos;
	SiegePos	m_EndPos;

public:

	Curve_Effect( void );

	void	RegisterDerivedParams( EffectParams &params );
	void	InitializeDerived( void );
	UINT32	InitializeTexture( void );
	bool	OnXfer( FuBi::PersistContext &persist );

	void	Put( void ){}
	void	Track( void ){}
	void	UpdateDerived( void );
	void	DrawSelf( void );
};



class Spawn_Effect : public SpecialEffect
{
	// Const parameters
	gpstring		m_sModelName;
	float			m_SpawnInterval;
	float			m_SpawnCount;
	float			m_MaxObjects;

	float			m_RadiusStartMin;
	float			m_RadiusStartMax;
	float			m_RadiusEndMin;
	float			m_RadiusEndMax;

	vector_3		m_LinearVelocity;
	vector_3		m_LinearVelocityVar;
	vector_3		m_LinearAcceleration;
	vector_3		m_LinearAccelerationVar;
	vector_3		m_AngularVelocity;
	vector_3		m_AngularVelocityVar;
	vector_3		m_AngularAcceleration;
	vector_3		m_AngularAccelerationVar;
	
	// Non-Const variables
	float			m_SpawnElapsed;
	UINT32			m_TotalSpawned;

public:
	Spawn_Effect( void );

	void	RegisterDerivedParams( EffectParams &params );
	void	InitializeDerived( void );
	bool	OnXfer( FuBi::PersistContext &persist );

	void	UpdateDerived( void );
	void	DrawSelf( void ){}

	float	GetBoundingRadius( void );

	void	SpawnObject( void );
};



class SRay_Effect : public SpecialEffect
{
	struct			SRay;

	enum			{ MAX_RAY_COUNT = 200 };
	typedef			stdx::fast_vector< SRay >	RayColl;

	vector_3		m_Start_color;
	vector_3		m_End_color;
	vector_3		m_Vari_color;

	float			m_Radius;
	UINT32			m_Count;
	float			m_RayLen_min;
	float			m_RayLen_max;

	float			m_RayW_start_min;
	float			m_RayW_start_max;
	float			m_RayW_end_min;
	float			m_RayW_end_max;

	vector_3		m_ThetaSettings;
	vector_3		m_PhiSettings;
	vector_3		m_AlphaSettings;

	bool			m_bInstant;

	float			m_SpawnRate;
	float			m_SpawnTime;

	bool			m_bNoTexture;


	struct	SRay
	{
		SRay( void ) {}

		bool		Xfer( FuBi::PersistContext &persist );

		float		alpha;
		float		alpha_fade;

		vector_3	start_color;
		vector_3	end_color;

		float		length;

		float		width_start;
		float		width_end;

		float		spin_theta;
		float		spin_phi;

		float		spin_itheta;
		float		spin_iphi;

		vector_3	start_pos;
		vector_3	end_pos;
	};

	RayColl			m_Rays;

	sVertex			*m_pDrawVerts;

	void	CreateRay( SRay &ray );

public:
	SRay_Effect( void );
	~SRay_Effect( void );

	void	RegisterDerivedParams( EffectParams &params );
	void	InitializeDerived( void );
	UINT32	InitializeTexture( void );
	bool	OnXfer( FuBi::PersistContext &persist );

	
	bool	ReInitialize( void );
	void	UpdateDerived( void );
	void	DrawSelf( void );

	float	GetBoundingRadius( void );
};



class Decal_Effect : public SpecialEffect
{
	gpstring		m_sTexName;
	float			m_In_dur;
	float			m_Out_dur;
	float			m_near;
	float			m_far;
	float			m_MaxAlpha;
	float			m_Alpha;
	bool			m_bRemoveTexture;

	unsigned int	m_DecalID;

public:
	Decal_Effect( void );
	~Decal_Effect( void );

	void	RegisterDerivedParams( EffectParams &params );
	void	InitializeDerived( void );
	bool	OnXfer( FuBi::PersistContext &persist );

	bool	ReInitialize( void );

	void	UpdateDerived( void );
	void	DrawSelf( void ) {}
};


#endif	// _FLAMETHROWER_EFFECTS_2_H_
