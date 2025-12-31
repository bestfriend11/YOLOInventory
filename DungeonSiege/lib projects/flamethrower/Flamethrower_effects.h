#pragma once
#ifndef _FLAMETHROWER_EFFECTS_H_
#define _FLAMETHROWER_EFFECTS_H_
//
//	Flamethrower_effects.h
//	Copyright 1999, Gas Powered Games
//
//

class Flamethrower_effect_base;


#include "vector_3.h"
#include "RapiOwner.h"

#include "siege_pos.h"
#include "siege_database_guid.h"

//
// Fire effect
//

class Fire_Effect : public SpecialEffect
{
public:
	enum	{ MAX_FLAMES = 600 };
	enum	{ TEX_WIDTH	 = 64 };
	enum	{ TEX_HEIGHT = 64 };
	enum	{ TEX_RADIUS = 32 };

private:
	struct	Flame
	{
		SiegePos	position;
		vector_3	velocity;
		vector_3	acceleration;

		vector_3	color;
		float		alpha;
		float		s;

		bool		Xfer( FuBi::PersistContext &persist );
	};
	typedef	stdx::fast_vector< Flame >	FlameColl;

	struct	Flame_list
	{
		siege::database_guid guid;
		stdx::fast_vector< Flame >	flames;

		bool		Xfer( FuBi::PersistContext &persist );
	};
	typedef stdx::fast_vector< Flame_list >	FlameLColl;

	vector_3		m_Base_color;
	vector_3		m_Color_variance;
	vector_3		m_Acceleration;
	vector_3		m_Velocity;
	bool			m_bCheap;
	float			m_AlphaFade;

	float			m_FlameSize;

	float			m_FlameSize_min;
	float			m_FlameSize_max;
	float			m_FlameSize_inc;

	float			m_MaxFlames;
	float			m_MinFlames;
	float			m_DisplaceMin;
	float			m_DisplaceMax;

	vector_3		m_origin_dir;

	vector_3		m_MinimumCorner;
	vector_3		m_MaximumCorner;

	FlameLColl		m_Flames;
	FlameLColl		m_Insert;

	stdx::fast_vector <sVertex>
					m_DrawVerts;

	bool			m_bInstantStart;
	bool			m_bLineMode;
	bool			m_bAreaMode;
	bool			m_bIgnite;
	bool			m_bDamage;

	bool			m_bDarkBlending;
	bool			m_bYOrientOnly;
	bool			m_bGreyTexture;

	float			m_FlameCount;
	float			m_Radius;
	float			m_Radius_min;
	float			m_Radius_max;
	float			m_Radius_inc_min;
	float			m_Radius_inc_max;
	float			m_Radius_rng_min;
	float			m_Radius_rng_max;

	float			m_SinPos;
	float			m_SinSpeed;

	float			m_LinePos;
	float			m_LineSpeed;

	vector_3		m_FDamage;


public:

	Fire_Effect	( void );
	~Fire_Effect( void );

	void	RegisterDerivedParams	( EffectParams &params );

	void	InitializeDerived	( void );
	bool	ReInitialize		( void );
	bool	UnInitialize		( void );
	UINT32	InitializeTexture	( void );

	bool	OnXfer				( FuBi::PersistContext &persist );

	void	Track				( void );
	void	Put					( void ) {}
	void	UpdateDerived		( void );
	void	DrawSelf			( void );
	float	GetBoundingRadius	( void );

	inline virtual bool	GetCullPosition		( SiegePos & CullPosition );

private:
	bool	CreateNewFlame( Flame &flame, const float delta_t );

	bool	AddFlameToBounds( Flame const &flame );
};




//
// Trackball effect
//

class Trackball_Effect	: public SpecialEffect
{
	float			m_RVel_min;
	float			m_RVel_max;

	float			m_Max_velocity;
	float			m_Acceleration;
	float			m_Velocity;

	float			m_Theta;

	SiegePos		m_DisplayPos;
	vector_3		m_Heading;

	bool			m_bInit;
	bool			m_bCollided;
	bool			m_bHomingEnabled;
	bool			m_bDampingEnabled;
	float			m_DamperSetting;
	float			m_SpiralRadius;
	float			m_SpinRate;
	vector_3		m_SpiralOffset;
	vector_3		m_Base_color;
	vector_3		m_Diff_vector;
	bool			m_bInvisible;
	float			m_AfterLife;

	vector_3		m_AngleOffset;

	bool			m_bNoCollision;
	bool			m_bNoCollisionObjects;

	bool			m_bCalcOrient;
	bool			m_bHitTargetOnly;

	SiegePos		m_TargetPos;
	SiegePos		m_LastPos;

public:

							Trackball_Effect		( void );

			void			RegisterDerivedParams	( EffectParams &params );
			void			InitializeDerived		( void );
			UINT32			InitializeTexture		( void );

			bool			OnXfer					( FuBi::PersistContext &persist );

			void			Put						( void );
			void			Track					( void );
			void			UpdateDerived			( void ) {};
			void			DrawSelf				( void );

			const SiegePos&	GetPosition				( void );
	inline	bool			UpdatesDuringDelay		( void ) const			{ return true; }
			float			GetBoundingRadius		( void );
};




//
// Lightning effect related stuff
//

class Lightning_Effect	: public SpecialEffect
{
	V3Coll							m_Points;

	vector_3						m_DefaultPosition;
									
	bool							m_bGroundCheck;
	float							m_Alpha;
	float							m_BoltLife;
									
	vector_3						m_Base_color;
	float							m_Bolt_life;
	float							m_Subd_param;
	float							m_Extra_subdivision;
									
	float							m_Min_displace;
	float							m_Max_displace;
	
	// Arcing option
	bool							m_bArc;

	// Sphere testing
	float							m_ArcRadius;
	float							m_ArcTestDelay;
	float							m_ArcTestElapsed;
	SiegePos						m_ArcTestPosition;

	// Spreading speed
	TattooTracker::TARGET_INDEX		m_ArcPosition;
	float							m_ArcSpreadDelay;
	float							m_ArcSpreadElapsed;
								
	UINT32							m_ArcBranchCount;
									
	bool							m_bDamage;
	float							m_DamageMin;
	float							m_DamageMax;
									
	bool							m_bIgnite;
									
	VertColl						m_DrawVerts;

	SiegePos						m_CullPosition;

	void	Generate3DBolt	( V3Coll &points, V3Coll &temp, int recursion );
	void	Subdivide3DBolt	( V3Coll &pointlist, int recursion, int iFirst, int iSecond );

public:

	Lightning_Effect( void );
	~Lightning_Effect( void );

	void	RegisterDerivedParams( EffectParams &params );
	void	InitializeDerived( void );
	UINT32	InitializeTexture( void );
	bool	OnXfer( FuBi::PersistContext &persist );

	void	Put( void ) {}
	void	UpdateDerived( void );
	void	DrawSelf( void );

	inline virtual bool GetCullPosition( SiegePos & CullPosition );
};




//
// Steam effect
//

class Steam_Effect	: public SpecialEffect
{
public:
	enum	{ MAX_WISPS = 600 };

private:
	struct Wisp
	{
		Wisp( void ) : alpha( 0 ) {}

		SiegePos		position;
		vector_3		velocity;
		vector_3		acceleration;
		float			alpha;

		bool			Xfer( FuBi::PersistContext &persist );
	};
	typedef	stdx::fast_vector< Wisp >		WispColl;


	struct Wisp_list
	{
		siege::database_guid	guid;
		stdx::fast_vector< Wisp >		wisps;

		bool			Xfer( FuBi::PersistContext &persist );
	};
	typedef stdx::fast_vector< Wisp_list >	WispLColl;

	WispLColl		m_Wisps;
	WispLColl		m_Insert;

	vector_3		m_Base_color;
	vector_3		m_Acceleration;
	vector_3		m_Velocity;
	float			m_AlphaFade;
	float			m_WispSize;
	float			m_MinWisps;
	float			m_MaxWisps;
	float			m_WispCount;
	float			m_AlphaStart;
	bool			m_bDarkBlending;
	bool			m_bCheap;
					
	vector_3		m_MinimumCorner;
	vector_3		m_MaximumCorner;
					
	bool			m_bInstantStart;
	bool			m_bLineMode;
	bool			m_bRectMode;

public:

	Steam_Effect( void );

	void			RegisterDerivedParams( EffectParams &params );
	void			InitializeDerived( void );
	UINT32			InitializeTexture( void );
	bool			ReInitialize( void );
	bool			UnInitialize( void );

	bool			OnXfer( FuBi::PersistContext &persist );
					
	void			Put( void );
	void			UpdateDerived( void );
	void			DrawSelf( void );
	float			GetBoundingRadius( void );
					
	void			AddPointToBounds( const vector_3 &Point );
	void			AddWispToBounds( const Wisp &wisp );
};




//
// Explosion effect
//

class Explosion_Effect	: public SpecialEffect
{

public:
	enum			{ MAX_PARTICLES = 200 };

	struct Particle
	{
		Particle( void ) : elapsed(0), w(0),h(0),alpha(0),alpha_fade(0),
			splat( false ), should_collide( false ), splat_life(0) {}

		float		elapsed;

		SiegePos	position;
		vector_3	velocity;
		
		float		w, h;
									
		float		alpha;
		float		alpha_fade;
						
		vector_3	color;
		vector_3	splat_verts[3];

		bool		splat;
		bool		should_collide;
		float		splat_life;

		bool		Xfer( FuBi::PersistContext &persist );
	};

	typedef			std::list< Particle >							ParticleList;
	typedef			std::list< ParticleList::iterator >				iParticleList;
	typedef			std::map< siege::database_guid, ParticleList >	ParticleMap;

private:

	SiegePos		m_SpawnOrigin;			// Point of emission
	SiegePos		m_LastSpawnTarget;		// Point used last update to spawn particles
	SiegePos		m_LastSpawnSource;		// Point used last update to spawn particles
	matrix_3x3		m_SpawnDirection;		// General direction of particle spawning
	float			m_SpawnRate;			// Rate to spawn particles
	float			m_SpawnTime;			// Storage for timestamp

	vector_3		m_Gravity;				// Gravity vector

	bool			m_bOmniDirectional;		// Explode in all directions from the specified origin point
	bool			m_bCollide;				// Do expensive collision detection
	bool			m_bSplat;				// Splat on the ground
	bool			m_bDarkBlending;		// Blend differently using darker blending mode
	bool			m_bRandomScale;			// Use random scale on a per particle basis
	bool			m_bSmoothTexture;		// Don't use a texture that has noise in it
	bool			m_bLineMode;			// Spawn the particles from a line

	vector_3		m_origin_dir;			// Saved direction when in line mode

	UINT32			m_ParticleCount;		// The maximum number of particles this effect will use

	float			m_Restitution;			// How much a bouncy particle will rebound
	vector_3		m_Base_color;			// Base color of the partcile
	vector_3		m_Color_variance;		// How much to vary the individual particle
	vector_3		m_SplatColor;			// The color of the particle when splatted
	float			m_Fade_min;				// Fade range min
	float			m_Fade_max;				// Fade range max
	
	float			m_SplatFade;			// How long the splat till take to fade
	float			m_SplatLife;			// How long the splat will live after splatting

	float			m_Radius;				// Spawn radius
	float			m_MinVelocity;			// Min velocity of particle
	float			m_MaxVelocity;			// Max velcoity of particle

	float			m_Scale_min;			// Min scale range for random scale mode
	float			m_Scale_max;			// Max scale range for random scale mode

	vector_3		m_iVel;					// Initial velocity vector
	vector_3		m_rVel;					// Random velocity vector

	float			m_min_phi;				// For omnidirectional explosion mode spherical angle
	float			m_max_phi;
	float			m_min_theta;			// For both modes - determines spawn radius
	float			m_max_theta;

	float			m_check_freq;			// How frequently to check for collisions
	UINT32			m_check_freq_count;
	UINT32			m_check_freq_reset;

	bool			m_bUseSplatTexture;		// True if name of splat texture was specified
	gpstring		m_sSplatTexture;		// The name of the texture to use for splattered particles
	unsigned int	m_SplatTexture;			// The "name" of the texture to use for splatted particles
	float			m_SplatScaleUp;			// Amount to scale the particle up by after splatting
	UINT32			m_SplatSkip;			// Number of particles to skip before splatting
	UINT32			m_SplatSkipCount;		// counter for splat skip

	float			m_AlphaStart;			// The starting alpha for a particle

	iParticleList	m_TransferList;			// Particles that need to be transferred
	ParticleMap		m_Particles;			// Where the moving particles live
	ParticleMap		m_SplatParticles;		// Where the splatted particles live

	stdx::fast_vector <sVertex>
					m_DrawVerts;			// Storage for where the actual vertices are drawn from

	bool	CreateParticle( Particle &particle );
	void	InsertParticle( Particle &particle, ParticleMap &map );
	// using the initial time will update the particle using that time
	// otherwise will use frame sync.
	bool	UpdateParticle( Particle &particle, float initialTime = 0.0f );

public:
	Explosion_Effect( void );
	~Explosion_Effect( void );

	void	RegisterDerivedParams( EffectParams &params );
	void	InitializeDerived( void );
	bool	ReInitialize( void );
	UINT32	InitializeTexture( void );
	void	InitializeSplatTexture( void );
	bool	OnXfer( FuBi::PersistContext &persist );

	void	Track( void ) {}
	void	Put( void ) {}

	void	UpdateDerived( void );
	void	DrawSelf( void );
	float	GetBoundingRadius( void );
};




//
// Ye Olde Spatiotemporal Pattern Effect
//
//

class SPE_Effect : public SpecialEffect
{
	UINT32			m_Max_sparkles;

	vector_3		m_Base_color;
	float			m_Alpha_fade;
	float			m_In_dur;
	float			m_Out_dur;

	float			m_wh;
	float			m_Alpha;
	float			m_Radius;

	// Pattern signature
	float			m_XIndex[ 2 ];
	float			m_YIndex[ 2 ];
	float			m_ZIndex[ 2 ];

	float			m_XSpeed[ 2 ];
	float			m_YSpeed[ 2 ];
	float			m_ZSpeed[ 2 ];
	
	float			m_XSpace[ 2 ];
	float			m_YSpace[ 2 ];
	float			m_ZSpace[ 2 ];

	// temp storage
	float			m_OldXIndex[ 2 ];
	float			m_OldYIndex[ 2 ];
	float			m_OldZIndex[ 2 ];

	VertColl		m_Verts;

	UINT32			m_IndexCount;

public:

			SPE_Effect				( void );

	void	RegisterDerivedParams	( EffectParams &params );
	void	InitializeDerived		( void );
	UINT32	InitializeTexture		( void );
	bool	OnXfer					( FuBi::PersistContext &persist );

	void	UpdateDerived			( void );
	void	DrawSelf				( void );
	float	GetBoundingRadius		( void );
};




//
//	Orbiter effect
//
//

class Orbiter_Effect : public SpecialEffect
{
	enum			{ TEX_WIDTH = 64 };
	enum			{ TEX_HEIGHT = 64 };
	enum			{ TEX_RADIUS = 32 };

	vector_3		m_Base_color;
	float			m_Radius;
	float			m_iRadius;
	float			m_Theta;
	float			m_iTheta;
	float			m_Phi;
	float			m_iPhi;
	float			m_Vdisplace;

	gpstring		m_sModelName;
	Goid			m_ModelGoid;
	
	bool			m_bSmooth;
	float			m_Smooth_history;

	bool			m_bFreeSpin;
	bool			m_bOrientRot;

	float			m_Vpos;
	bool			m_bInvisible;

	float			m_wh;
	float			m_Alpha;
	float			m_In_dur;
	float			m_Out_dur;

	vector_3		m_RotationInc;
	float			m_Model_scale_original;
	float			m_Model_scale;
	float			m_Model_sin;
	float			m_Model_sout;
	float			m_Model_smax;
	matrix_3x3		m_ModelOrient;

	SiegePos		m_LastPos;

	std::list< SiegePos >	
					m_PosHistory;

public:
	Orbiter_Effect( void );
	~Orbiter_Effect( void );

	void	RegisterDerivedParams( EffectParams &params );
	void	InitializeDerived( void );
	UINT32	InitializeTexture( void );
	bool	OnXfer( FuBi::PersistContext &persist );

	void	UpdateDerived( void );
	void	DrawSelf( void );
	void	Track( void );

	void	SetFinishing( void );

	float	GetBoundingRadius( void );
};



#endif	// _FLAMETHROWER_EFFECTS_H_
