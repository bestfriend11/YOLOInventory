#pragma once
/*========================================================================

	NeMa_Aspect.h

	"ASPECT: Appearance to the eye, especially from a specific vantage point"

	Everything that animates has an 'aspect'

	the configuration of geometry and channels that describe what an actor looks
	like at any time



  author: Mike Biddlecombe
  date: 9/13/1999
  (c)opyright 1999, Gas Powered Games

  -----

  todo:


------------------------------------------------------------------------*/
#ifndef NEMA_ASPECT_H
#define NEMA_ASPECT_H

#include "FuBiDefs.h"
#include "Nema_Types.h"
#include "SkritDefs.h"
#include "GoDefs.h"
#include "GpColl.h"
#include "space_2.h"
#include "Siege_Defs.h"
#include "choreographer.h"

#include <set>

namespace nema {

	//** Basic datatypes ***************************************************

	enum  SharedAttrFlags {
		NORMAL				= 0,
		STENCIL_ALPHA		= 1<<0,
		DISABLE_LIGHTING	= 1<<1,
		TEXTURE_UWRAP		= 1<<2,
		TEXTURE_VWRAP		= 1<<3,
		VERTEX_ALPHA		= 1<<4
	};

	enum  InstanceAttrFlags {
		HIDE_SUBMESH0			= 1 << 0,
		HIDE_SUBMESH1			= 1 << 1,
		HIDE_SUBMESH2			= 1 << 2,
		HIDE_SUBMESH3			= 1 << 3,
		STITCH_VERTS			= 1 << 4,
		RENDER_NORMALS			= 1 << 5,
		RENDER_BONES			= 1 << 6,
		RENDER_BLENDS			= 1 << 7,
		RENDER_USE_COLOR		= 1 << 8,
		RENDER_DISABLE_TEXTURE	= 1 << 9,
		RENDER_BOUNDBOXES		= 1 <<10,
		RENDER_SHIELD			= 1 <<11,
		RENDER_TRACERS			= 1 <<12,
		RENDER_FROZEN			= 1 <<13,
		INCLUDE_NORMALS		    = 1 <<14,
		HIDE_ALLSUBMESHES	    = 1 <<15
	};

	enum  AnimFlags {
		ANIMFLAG_STARTATEND			= 1 << 0
	};

	enum BoundingBoxTags {
		BBOX_CLIPPING  = 'CLIP',
		BBOX_COLLISION = 'COLL',
		BBOX_SELECTION = 'SLCT'
	};

	struct sBoneVertWeightPair {
		DWORD	VertIndex;
		float	Weight;
	};

	struct sUV {
		float u;
		float v;
	};

	typedef DWORD sVertMap;

	typedef vector_3 sVert;

	struct BoundingBox {
		vector_3 offset;
		Quat orientation;
		vector_3 halfdiag;
	};

	struct sTri {
		WORD CornerAIndex;
		WORD CornerBIndex;
		WORD CornerCIndex;
	};

	enum eBoneFreeze {
		SET_BEGIN_ENUM( BFREEZE_, 0 ),

		BFREEZE_NONE			= 0,
		BFREEZE_POSITION		= 1,
		BFREEZE_ROTATION		= 2,
		BFREEZE_BOTH			= 3,
		BFREEZE_RIGID			= 4,
		BFREEZE_POSITION_RIGID	= 5,
		BFREEZE_ROTATION_RIGID	= 6,
		BFREEZE_BOTH_RIGID		= 7,

		SET_END_ENUM( BFREEZE_ ),

	};

	const char* ToString  ( eBoneFreeze val );
	bool        FromString( const char* str, eBoneFreeze& val );

	enum eAttachMethods {

		SET_BEGIN_ENUM( ATTACHMETHOD_, 0 ),

		UNATTACHED,
		RIGID_LINKED,
		REVERSE_RIGID_LINKED,
		DEFORMABLE_OVERLAY,

		SET_END_ENUM( ATTACHMETHOD_ ),
	};

	const char* ToString  ( eAttachMethods val );
	bool        FromString( const char* str, eAttachMethods& val );

	//************************************************************
	//************************************************************
	//************************************************************

	// Feb 24/00
	/// sBoneImpl contains shared bone data
	struct sBoneImpl {

		DWORD	m_boneIndex;
		DWORD	m_parentBoneIndex;

		vector_3 m_relRestPos;
		Quat     m_relRestRot;

		vector_3 m_invRestPos; 
		Quat	 m_invRestRot; 

		bool     m_bboxExists;
		vector_3 m_bboxHalfDiag;
		vector_3 m_bboxCenter;

		DWORD				*m_numberOfPairs;
		sBoneVertWeightPair **m_vertWeights;

		char*	m_name;

	};

	struct sBoneLink
	{	
		// Setting a NEGATIVE bone value for the parent index indicates that ALL bones are mapped in pairs (default)
		// Setting a NEGATIVE bone value for the child index indicates that the child has weapon tracer points
		// Non-negative pairs indicate a one-to-one link from a single parent bone to a child bone

		// Yeah, using signal values is a little hairy, but I'm running out of time and don't want to add MORE member vars


		sBoneLink(DWORD p, DWORD c)
			: m_parentBoneIndex((short)p)
			, m_childBoneIndex((short)c)
		{};

		sBoneLink()
			: m_parentBoneIndex(-1)
			, m_childBoneIndex(0)
		{};

		bool IsFullyLinked()	{ return (m_parentBoneIndex < 0); }

		void SetHasWeaponTracerFlag()	{ if (m_childBoneIndex >=0) m_childBoneIndex = (short)~(m_childBoneIndex); }
		bool HasWeaponTracerFlag()		{ return (m_childBoneIndex<0); }

		DWORD ParentIndex()	{ return (m_parentBoneIndex < 0 ) ? 0 : m_parentBoneIndex; }
		DWORD ChildIndex()	{ return (m_childBoneIndex < 0 ) ? ~((DWORD)m_childBoneIndex) : m_childBoneIndex; }


		bool Xfer( FuBi::PersistContext& persist );

	private:
		short m_parentBoneIndex;
		short m_childBoneIndex;	

	};

	typedef stdx::linear_map<HAspect,sBoneLink>		ChildLinks;
	typedef ChildLinks::iterator					ChildLinkIter;
	typedef ChildLinks::const_iterator				ChildLinkConstIter;

	/// sBone has now stores almost all the skeleton data
	struct sBone {
		sBone	*m_parentBone;

		sBoneImpl	*m_sharedbone;

		vector_3 m_position;
		Quat    m_rotation;

		DWORD	m_frozen;
		bool	m_tweaked;

		bool XferPost( FuBi::PersistContext& persist );
	};

	// sCorner maps to the underlying D3D/GL primitive
	struct sCorner {
		float vx, vy, vz;					// Position
		DWORD color;						// Color
		sUV	  uv;							// Tex coordinate set
	};

	// sWeightedCorner
	struct sWeightedCorner {
		vector_3 v;							// Position
		float b0, b1, b2, b3;				// Bone weights
		DWORD indices;
		vector_3 n;							// Normal
		DWORD color;						// Color
		sUV	  uv;							// Tex coordinate set
	};

	struct sStitchSet {
		DWORD	m_tag;
		DWORD	m_nverts;
		DWORD*	m_verts;
	};

	#define PACK_SKIN_INDICES(i3,i2,i1,i0) \
		((DWORD) (((i3)<<24) | ((i2)<<16) | ((i1)<<8) | (i0)))

	#define UNPACK_SKIN_INDEX_I0(i) ((i)>>24) &0xff)
	#define UNPACK_SKIN_INDEX_I1(i) ((i)>>16) &0xff)
	#define UNPACK_SKIN_INDEX_I2(i) ((i)>> 8) &0xff)
	#define UNPACK_SKIN_INDEX_I3(i) ((i)    ) &0xff)


	//************************************************************
	//************************************************************
	//************************************************************

	//---------------------------------

	class AspectImpl {

	public:

		AspectImpl();
		~AspectImpl(void);

		int AddRef ( void )  {  gpassert( (m_RefCount >= 0) ); gpassert ( (m_RefCount <  1000) );  return ( ++m_RefCount );  }		// sanity checks
		int GetRef ( void )  {  gpassert( (m_RefCount >= 0) ); gpassert ( (m_RefCount <= 1000) );  return (   m_RefCount );  }		// sanity checks
		int Release( void )  {  gpassert( (m_RefCount >  0) ); gpassert ( (m_RefCount <= 1000) );  return ( --m_RefCount );  }		// sanity checks

		DWORD		m_RefCount;

		// Data that is shared by all aspects derived from the same ASP file

		DWORD		m_MajorVersion;
		DWORD		m_MinorVersion;

		DWORD		m_nbones;
		sBoneImpl	*m_restbones;

		// As of version 1.0 we support multiple sets of meshes

		DWORD		m_nmeshes;
		DWORD		m_maxverts;

		DWORD		*m_nverts;
		DWORD		*m_ncorners;
		DWORD		*m_ntriangles;

		sVert		**m_verts;
		sTri		**m_triangles;
		sVertMap	**m_vertmappings;

		sWeightedCorner	**m_wcorners;	// 'sWeightedCorner Corners' are an evolutionary step towards DX8 matrix palettes

		char		*m_stringtable;

		// Material components

		DWORD		m_ntextures;

		DWORD		*m_submeshmatcount;		// [NUM_SUB_MESH]
		DWORD		**m_submeshmatlookup;	// [NUM_SUB_MESH][SUBMESH_MAT_COUNT]
		DWORD		**m_submeshfacecount;	// [NUM_SUB_MESH][SUBMESH_MAT_COUNT]
		DWORD		**m_submeshcorncount;	// [NUM_SUB_MESH][SUBMESH_MAT_COUNT]
		DWORD		**m_submeshstartindex;	// [NUM_SUB_MESH][SUBMESH_MAT_COUNT]

		DWORD		*m_nstitchsets;
		sStitchSet	**m_stitchset;


		// Static Bounding Boxes
		stdx::linear_map< DWORD,BoundingBox > m_boundingboxes;

		int			m_primarybone;

		// Other flags
		DWORD		m_flags;

		vector_3	m_CenterOfGravity;

		//****************

		// New animation/choreographer functionality added by biddle

		int	m_HeadBoneIndex;
		// The list of chores that the choreographer can perform with this body

	};

	class Aspect {

		friend class AspectStorage;

	public:

		AspectImpl	*m_shared;

		Aspect( const_mem_ptr mem );

		Aspect( AspectImpl * );

		~Aspect(void);

		bool Xfer( FuBi::PersistContext& persist );
		bool XferPost( FuBi::PersistContext& persist );
		void ValidateParentLinks( bool fix_errors );

		void CommitCreation();

		bool Purge(int level=2);

		bool Reconstitute(DWORD level=0,bool immediate=false) 	{ return PrivateReconstitute(level,immediate); }

		bool ReconstituteTop(DWORD level=0,bool immediate=false);

		bool MatchPurgeStatus(Aspect* src,bool immediate);

		void CalculateWeaponTracerPoints(Aspect *kid);

		// References.

		FUBI_EXPORT const char* GetDebugName( void ) const				{  return ( m_dbgname.c_str() );  }
		FUBI_EXPORT const gpstring& GetDebugNameString( void ) const	{  return ( m_dbgname );  }

		void SetDebugName( const gpstring& n )		{   m_dbgname = n;  }

		void SetHandle( const HAspect& h ) 		{ m_handle = h ;	}
		const HAspect& GetHandle( void ) const	{ return(m_handle);	}
		FUBI_EXPORT const DWORD GetHandleValue( void ) 	{ return((DWORD)(m_handle.Get()));	}

		void SetGoid(Goid g)								{ m_owner_goid = g;				}
		void ClearGoid(void)								{ m_owner_goid = GOID_INVALID;	}
		FUBI_EXPORT const Goid GetGoid(void) const			{ return (m_owner_goid);		}
		bool ShouldPersist(void) const;

#if	!GP_RETAIL		
		float GetCost(void);
#endif  // !GP_RETAIL

		// **********************************************************

		bool Deform(DWORD CurrentFirstVert=0, DWORD ParentFirstVert=0);		// Returns true iff mesh is modified
		void Render(Rapi* hRapi, DWORD flags=0);							// Draw the mesh/normals/bones
		void RenderTracers(Rapi* hRapi, DWORD flags=0);						// Draw tracer effects on the mesh
		void RenderFrozen(Rapi* hRapi, DWORD flags=0);						// Draw 'frozen' effects on the mesh

		bool HilightDeform(float level);
		void HilightRender(Rapi* hRapi,float level);

		DWORD GetNumSubTextures(void) { return m_shared->m_ntextures; }

		void SetTextureFromFile(DWORD index, const char* texFile);
		void SetTextureCreatedByExternalSource(DWORD index, DWORD tex);

		DWORD GetTexture(DWORD index) const {
			if (index < m_shared->m_ntextures) {
				return m_currenttextures[index];
			}
			else {
				return 0;
			}
		}

		void CopyTextures( const Aspect& other );
		void CopyTexture( DWORD dsttex, const Aspect& src, DWORD srctex );

		// The default texture names are the first strings in the table
		char const * GetDefaultTextureString(DWORD index) const {
			const char* ret = m_shared->m_stringtable;
			while (ret && index) {
				while (*ret) ret++;			// Skip over string
				while (!*ret) ret++;		// Skip over separating nulls
				index--;
			}
			return ret ? ret : "";
		}

		eAttachMethods GetAttachType(void) const	{ return m_attachtype; }
	
		// TODO -- might want to allow an offset rotation/translation when we rigid link a child...

		bool AttachRigidLinkedChild(
			HAspect hchild,
			const char* parentbone,
			const char* childbone,
			const Quat& offsetrot,
			const vector_3& offsetpos,
			bool hastracers
			);

FEX		bool AttachRigidLinkedChild(
			HAspect hchild,
			const char* parentbone,
			const char* childbone,
			const Quat& offsetrot,
			const vector_3& offsetpos
			)
		{
			return AttachRigidLinkedChild( hchild, parentbone, childbone, offsetrot, offsetpos, false );
		}

FEX		bool AttachRigidLinkedChild(
			HAspect hchild,
			const char* parentbone,
			const char* childbone
			) 
		{
			return AttachRigidLinkedChild( hchild, parentbone, childbone, Quat::IDENTITY, vector_3::ZERO, false );
		}

		bool AttachRigidLinkedChild(
			HAspect hchild,
			const char* parentbone,
			const char* childbone,
			bool hastracers
			) 
		{
			return AttachRigidLinkedChild( hchild, parentbone, childbone, Quat::IDENTITY, vector_3::ZERO, hastracers );
		}

FEX		bool AttachReverseLinkedChild(
			HAspect hchild,
			const char* parentbone,
			const char* childbone,
			const Quat& offsetrot,
			const vector_3& offsetpos );

FEX		bool AttachReverseLinkedChild(	HAspect hchild,
										const char* parentbone,
										const char* childbone )
		{
			return AttachReverseLinkedChild( hchild, parentbone, childbone, Quat::IDENTITY, vector_3::ZERO );
		}

FEX		bool AttachDeformableChild(	HAspect hchild );

FEX		int	 GetNumberOfChildren(void)						{ return m_childlinks.size();	}
		ChildLinks& GetChildren(void)						{ return m_childlinks;			}

FEX		bool DetachChild(HAspect hchild);
FEX		bool DetachAllChildren(void);

FEX		const vector_3&	GetLinkOffsetPosition(void) const	{ return m_linkOffsetPosition;	}
FEX		void SetLinkOffsetPosition(const vector_3& v)		{ m_linkOffsetPosition = v;		}

FEX		const Quat&	GetLinkOffsetRotation(void) const		{ return m_linkOffsetRotation;	}
FEX		void SetLinkOffsetRotation(const Quat& q)			{ m_linkOffsetRotation = q;		}

		//-- CORNERS ----------------------

FEX		int GetNumCorners(DWORD sm)							{ return (sm < m_shared->m_nmeshes) ? m_shared->m_ncorners[sm] : 0; }

FEX		bool GetIndexedCornerPosNorm(
			DWORD sm,
			DWORD index,
			vector_3& pos,
			vector_3& norm
			) const;

		//-- BONES ------------------------

		// Locking the primary bone of an object forces the object to the
		// orientation/position provided
		bool LockPrimaryBone(
			const vector_3& bone_position = vector_3::ZERO,
			const Quat& bone_rotation = Quat::IDENTITY);

		// Unlocking the primary bone allows the object derive its orient/pos
		// from its parent bone
		bool UnlockPrimaryBone(void);

		// Is the primary bone locked
		bool IsPrimaryBoneLocked(void)	{ return (m_PrimaryBoneIsLocked); }

FEX		HAspect GetParent(void);
FEX		HAspect GetChild(DWORD k);

FEX		bool GetBoneOrientation(
		 	char const * bone_name,
 			vector_3& bone_position,
 			Quat& bone_rotation
			) const;

FEX		bool GetIndexedBoneOrientation(
			int bone_index,
 			vector_3& bone_position,
 			Quat& bone_rotation
			) const;

FEX		bool GetPrimaryBoneOrientation(
 			vector_3& bone_position,
 			Quat& bone_rotation
			) const;

FEX		bool GetIndexedBonePosition(
			int bone_index,
			vector_3& bone_position
			) const;

FEX		bool GetBoneIndex(
			char const * bone_name,
			int& bone_index) const;

FEX		int GetBoneIndex(
			const gpstring& bone_name) const;

		DWORD GetNearestBoneIndex(const vector_3& pos);

		DWORD GetNumBones() const { return m_shared->m_nbones; }

		sBone* GetBone(int bone_index);

		char const * const  GetBoneName(int bone_index);

		// BONE IK and tweaks

		FUBI_EXPORT void RotateBoneX   (const char* bonename, float anginradians, bool relative);
		FUBI_EXPORT void RotateBoneY   (const char* bonename, float anginradians, bool relative);
		FUBI_EXPORT void RotateBoneZ   (const char* bonename, float anginradians, bool relative);
		FUBI_EXPORT void TranslateBoneX(const char* bonename, float offset, bool relative);
		FUBI_EXPORT void TranslateBoneY(const char* bonename, float offset, bool relative);
		FUBI_EXPORT void TranslateBoneZ(const char* bonename, float offset, bool relative);

		FUBI_EXPORT void RotateIndexedBoneX   (int bonenum, float anginradians, bool relative);
		FUBI_EXPORT void RotateIndexedBoneY   (int bonenum, float anginradians, bool relative);
		FUBI_EXPORT void RotateIndexedBoneZ   (int bonenum, float anginradians, bool relative);
		FUBI_EXPORT void TranslateIndexedBoneX(int bonenum, float offset, bool relative);
		FUBI_EXPORT void TranslateIndexedBoneY(int bonenum, float offset, bool relative);
		FUBI_EXPORT void TranslateIndexedBoneZ(int bonenum, float offset, bool relative);

		FUBI_EXPORT void RotateBoneX   (const char* b, float a)	{ RotateBoneX   ( b, a, false ); }
		FUBI_EXPORT void RotateBoneY   (const char* b, float a)	{ RotateBoneY   ( b, a, false ); }
		FUBI_EXPORT void RotateBoneZ   (const char* b, float a)	{ RotateBoneZ   ( b, a, false ); }
		FUBI_EXPORT void TranslateBoneX(const char* b, float o)	{ TranslateBoneX( b, o, false ); } 
		FUBI_EXPORT void TranslateBoneY(const char* b, float o)	{ TranslateBoneY( b, o, false ); }
		FUBI_EXPORT void TranslateBoneZ(const char* b, float o)	{ TranslateBoneZ( b, o, false ); }

		FUBI_EXPORT void RotateIndexedBoneX   (int b, float a)	{ RotateIndexedBoneX   ( b, a, false ); }
		FUBI_EXPORT void RotateIndexedBoneY   (int b, float a)	{ RotateIndexedBoneY   ( b, a, false ); }
		FUBI_EXPORT void RotateIndexedBoneZ   (int b, float a)	{ RotateIndexedBoneZ   ( b, a, false ); }
		FUBI_EXPORT void TranslateIndexedBoneX(int b, float o)	{ TranslateIndexedBoneX( b, o, false ); }
		FUBI_EXPORT void TranslateIndexedBoneY(int b, float o)	{ TranslateIndexedBoneY( b, o, false ); }
		FUBI_EXPORT void TranslateIndexedBoneZ(int b, float o)	{ TranslateIndexedBoneZ( b, o, false ); }

		FUBI_EXPORT DWORD SetBoneFrozenState(const char* bonename, eBoneFreeze f, bool recursive);
		FUBI_EXPORT DWORD GetBoneFrozenState(const char* bonename) const;

		FUBI_EXPORT DWORD SetIndexedBoneFrozenState(DWORD bone_index, eBoneFreeze f, bool recursive);
		FUBI_EXPORT DWORD GetIndexedBoneFrozenState(DWORD bone_index) const;

		//---------------------------------

		void ForceDeformation(void)			{ m_DeformationRequested = m_IsDeformable; }
		bool IsDeformationRequested() const	{ return m_DeformationRequested; }
		void ForceTweak(void)				{ m_TweakRequested = true; }
		bool IsTweakRequested() const		{ return m_TweakRequested; }

		bool IsDeformable() const	{ return m_IsDeformable; }

		bool IsPurged(DWORD minlev) const	{ return m_PurgeLevel >= minlev; }

		DWORD GetNumMeshes()		{ return m_shared->m_nmeshes; }

		void GetStats(const DWORD i, int& tris,int& verts) {
				if (i < m_shared->m_nmeshes) {
					tris = m_shared->m_ntriangles[i];
					verts = m_shared->m_nverts[i];
				} else {
					tris = 0;
					verts = 0;
				}
		}

		DWORD GetAlpha() const						{ return m_Alpha; }
		DWORD GetAmbience() const					{ return m_Ambience; }

		DWORD GetLightVisit() const					{ return m_lightVisit; }
		void SetLightVisit( DWORD lightVisit )		{ m_lightVisit = lightVisit; }

		FUBI_EXPORT DWORD GetDiffuseColor()			{ return m_DiffuseColor; }
		FUBI_EXPORT void SetDiffuseColor( DWORD c )	{ m_DiffuseColor = c; }

		void InitializeLighting( const DWORD ambience, const DWORD alpha = 255 );
		void CalculateShading(const LightSource& source, const bool dbgdrawlightvect );
		void RenderShadow( Rapi& renderer, vector_3& light, float colorScale );
		void RenderSingleShadow( Rapi& renderer, vector_3& light, float colorScale );

		// Bounding Box and Center Of Gravity methods

		void GetBoundingSphere(vector_3& center,float& radius) const;

		float    GetBoundingSphereRadius(void)	{ return m_BSphereRadius; }

		vector_3 GetBoundingSphereCenter(void) const;

		void     GetBoundingBox(vector_3& center,vector_3& half_diag) const;

		vector_3 GetBoundingBoxCenter(void) const;

		const vector_3& GetBoundingBoxHalfDiag(void) const	{ return m_BBoxHalfDiag; }

		void GetBoundingSphereIncludingChildren(vector_3& center,float& radius, float scalefactor) const;

		void GetOrientedBoundingBox( Quat& orient, vector_3& center, vector_3& half_diag) const;
		void GetOrientedBoundingBoxCenter( vector_3& center) const;

		bool GetOrientedBoneBoundingBox( DWORD boneIndex, Quat& orient, vector_3& center, vector_3& half_diag) const;
		bool GetOrientedBoneBoundingBoxCenter( DWORD boneIndex, vector_3& center) const;

		void GetTaggedBBox(Quat& orient, vector_3& center, vector_3& half_diag, DWORD tag) const;
		void GetTaggedBBoxCenter(vector_3& center, DWORD tag) const;

		void GetCollisionBox(Quat& orient, vector_3& center, vector_3& half_diag) const
			{ GetTaggedBBox( orient, center, half_diag, 'COLL'); }
		void GetCollisionBoxCenter(vector_3& center) const
			{ GetTaggedBBoxCenter( center,'COLL'); }

		void GetSelectionBox(Quat& orient, vector_3& center, vector_3& half_diag) const
			{ GetTaggedBBox( orient, center, half_diag, 'SLCT'); }
		void GetSelectionBoxCenter(vector_3& center) const
			{ GetTaggedBBoxCenter( center,'SLCT'); }

		void GetClippingBox(Quat& orient, vector_3& center, vector_3& half_diag) const
			{ GetTaggedBBox( orient, center, half_diag, 'CLIP'); }
		void GetClippingBoxCenter(vector_3& center) const
			{ GetTaggedBBoxCenter( center,'CLIP'); }

		const vector_3& GetCenterOfGravity(void) { return m_shared->m_CenterOfGravity; };

		// Registering a callback on the entire aspect merely attaches the callback to bone 0;
		void RegisterAnimationCompletionCallback(EventCallback const &CBInit) ;
		void ResetAnimationCompletionCallback(void) ;
		EventCallback GetAnimationCompletionCallback(void) const ;
		FUBI_EXPORT void AnimationCallback(DWORD msg);

		FUBI_EXPORT DWORD GetSharedAttrFlags(void)				{ return m_shared->m_flags; }

		FUBI_EXPORT DWORD GetInstanceAttrFlags(void)			{ return m_instflags; }
		FUBI_EXPORT void SetInstanceAttrFlags(DWORD newval)		{ m_instflags |= newval; m_Ambience = UINT_MAX; }
		FUBI_EXPORT void ClearInstanceAttrFlags(DWORD newval)	{ m_instflags &= ~newval; m_Ambience = UINT_MAX; }

FEX		void SetHideMeshFlag(bool flag);
FEX		bool GetHideMeshFlag(void)								{ return ((m_instflags & HIDE_ALLSUBMESHES) != 0); }

FEX		void SetFreezeMeshFlag(bool flag);
FEX		bool GetFreezeMeshFlag(void)							{ return ((m_instflags & RENDER_FROZEN) != 0); }

FEX		void SetRenderTracersFlag(bool flag);
FEX		bool GetRenderTracersFlag(void)							{ return ((m_instflags & RENDER_TRACERS) != 0); }

FEX		void SetLockMeshFlag(bool flag);
FEX		bool GetLockMeshFlag(void)								{ return (!m_UpdateEnabled); }

		void temporarySetSpinning(float rpm);	/// TODO Need to set up a 'real' calling routine --biddle

		// Need a routine that will copy the animation components from one aspect to another
		bool temporaryTransferInternals(HAspect hdest);

#		if !GP_RETAIL
		void DebugGetInfo(int spacing, gpstring& output );
#		endif // !GP_RETAIL

		// ***************************************************
		// Choreographer info
		// ***************************************************

		// Hande messages for the animation skrit.

		void HandleMessage( const WorldMessage& msg );

		// New format for animation interface **********************

		bool UpdateAnimationStateMachine(float delta_time);
		bool RestartAnimationStateMachine(void);
		bool RefreshAnimationStateMachine(float elapsed);	// Called when an aspect re-enters a frustum

		void SetCurrReqBlock(DWORD rb)						{ m_CurrReqBlock = rb; }

		bool SetNextChore(
			eAnimChore  reqchore,
			eAnimStance reqstance = AS_DEFAULT,
			int			reqsubanim = 0,
			DWORD		reqflags = 0
			);

		bool SetNextStance(
			eAnimStance reqstance
			);

		bool ChangeCurrentChore(void);
		bool SkipChoreChange(void);

		// Animation blending support

		FUBI_EXPORT void ResetBlender(void);
		FUBI_EXPORT DWORD UpdateBlender(float deltat);

		bool AnimateBones(float deltat, bool reconstituting=false);	// move the skeleton

		void CopyBonesPRS(HAspect hsource);		// copy the source's bones' prs to my bones

		void DisableUpdate();		// Skrits should now use SetLockMeshFlag(state) to modify
		void EnableUpdate();
		bool IsUpdateEnabled()	{ return m_UpdateEnabled;	}

		void AddChore(const Chore* nc, bool shouldPersist = false);

		// We can allows for 31 different stances per Aspect
		// See the notes on m_ChoreStanceMask --biddle

		FUBI_EXPORT eAnimStance GetChoreStance(void)			{ return m_CurrStance; }

		FUBI_EXPORT bool		HasBlender(void) const			{ return m_Blender != NULL;	}
		FUBI_EXPORT Blender*	GetBlender()					{ return m_Blender;			}
		FUBI_EXPORT Blender*	GetOrCreateBlender();
		FUBI_EXPORT bool		BlenderIsIdle();

		FUBI_EXPORT eAnimChore  GetPreviousChore(void) const	{ return m_PrevChore;    }
		FUBI_EXPORT eAnimStance GetPreviousStance(void) const	{ return m_PrevStance;   }
		FUBI_EXPORT int         GetPreviousSubAnim(void) const	{ return m_PrevSubAnim;  }
		FUBI_EXPORT int         GetPreviousFlags(void) const	{ return m_PrevFlags;    }

		FUBI_EXPORT DWORD       GetCurrentReqBlock(void) const  { return m_CurrReqBlock; }
		FUBI_EXPORT eAnimChore  GetCurrentChore(void) const		{ return m_CurrChore;    }
		FUBI_EXPORT eAnimStance GetCurrentStance(void) const	{ return m_CurrStance;   }
		FUBI_EXPORT int         GetCurrentSubAnim(void) const	{ return m_CurrSubAnim;  }
		FUBI_EXPORT int         GetCurrentFlags(void) const		{ return m_CurrFlags;    }

		FUBI_EXPORT eAnimChore  GetNextChore(void) const		{ return m_NextChore;    }
		FUBI_EXPORT eAnimStance GetNextStance(void) const		{ return m_NextStance;   }
		FUBI_EXPORT int         GetNextSubAnim(void) const		{ return m_NextSubAnim;  }
		FUBI_EXPORT int         GetNextFlags(void) const		{ return m_NextFlags;    }
		
		FUBI_EXPORT bool		GetChoreWasRepeated()			{ return ((m_PrevChore == m_CurrChore) &&
																		  (m_PrevStance == m_CurrStance)); }

		void SetCurrentScale(const vector_3 &s)			{ m_Scale = s;		}
		void GetCurrentScale(vector_3 &s)				{ s = m_Scale;		}
		void GetCombinedScale(vector_3 &s) const;

		// Queries

		FUBI_EXPORT const float GetCurrentVelocity(void) const
			{
				return IsPositive(m_Velocity,0.01f) ? m_Velocity: 0.0f;
			}

		const vector_3& GetCurrentScale() const			{ return m_Scale;	}
		
	private:

		bool PrivateReconstitute(int level, bool immediate);
		
		bool PrivateCreateTexture(DWORD t, const char* texFile );
		bool PrivateDestroyTexture(DWORD t );
		bool PrivatePurgeTexture(DWORD t);
		bool PrivateReconstituteTexture(DWORD t, bool immediate);

		bool PrivateSetTexture( DWORD index, DWORD tex );	
		

		void SetParent(HAspect hnewkid);

		HAspect		m_handle;			// The handle used to get at this aspect

		gpstring	m_dbgname;			// Name printed for debugging

		Goid		m_owner_goid;		// Goid that ultimately owns this ASPECT, need for messaging

		DWORD		m_instflags;		// Attributes from the exporter (alpha_stencil, nolight, etc...)

		DWORD		m_lightVisit;		// Visit tag for lighting calcs

		// Geometry components

		vector_3	m_Scale;

		// As of version 1.0 we support multiple sets of meshes

		DWORD				*m_currenttextures;
		siege::LoadOrderId	*m_textureloadids;

		char*		m_memblock;
#if GP_DEBUG
		DWORD		m_memsize;	// Track the memsize in debug mode
#endif

		sBone		*m_bones;
		sCorner		**m_corners;
		DWORD		**m_cornercolors;

		vector_3	**m_normals;

		// Animation components

		HAspect		m_parent;
		ChildLinks	m_childlinks;

		eAttachMethods	m_attachtype;
		DWORD			m_linkBone;		// When a child is rigidly linked, this is the index of the
										// first bone that we need to evaluate
										// When we are unlinked, it will be 0

		Quat			m_linkOffsetRotation;		// Offset of the link bone relative it parent
		vector_3		m_linkOffsetPosition;


		DWORD			m_PurgeLevel;	// 0 - loaded, 1 - textures unloaded, 2 - textures & data unloaded

		bool			m_UpdateEnabled;

		// TODO: This flag is used to decide if we
		// need to do a fully deformation update
		// Will probably want to explore how best to handle
		// totally rigid objects. This flag may not be enough
		bool m_IsDeformable;

		// Deformation requested only when the bones have moved
		bool m_DeformationRequested;

		// Tweak requested if the bones have been adjusted directly
		// and you need to get them updated on the next call to AnimateBones
		bool m_TweakRequested;

		// Maintain a flag to indicate if the bones have been modified from the 
		// initial rest pose
		bool m_BonesInRestPose;

		bool m_PrimaryBoneIsLocked;

		// TODO:: allow for multiple bones driving separate rigid meshes - biddle
		//        This is the BAM break-apart-mesh

		// Bounding region information
		vector_3 m_BBoxCenter;
		vector_3 m_BBoxHalfDiag;
		float    m_BSphereRadius;

		// Currently calculated ambient lighting value
		DWORD	 m_Ambience;

		// Current alpha value
		DWORD	 m_Alpha;

		// Color used when rendering with RENDER_USE_COLOR flag set
		DWORD	 m_DiffuseColor;

		float    m_Velocity;

		//************* New animation support members

		eAnimChore		m_PrevChore;
		eAnimStance		m_PrevStance;
		int				m_PrevSubAnim;
		int				m_PrevFlags;

		DWORD			m_CurrReqBlock;
		eAnimChore		m_CurrChore;
		eAnimStance		m_CurrStance;
		int				m_CurrSubAnim;
		int				m_CurrFlags;

		eAnimChore		m_NextChore;
		eAnimStance		m_NextStance;
		int				m_NextSubAnim;
		int				m_NextFlags;

		bool			m_ChoreChangeRequested;

		Blender			*m_Blender;

		DWORD			m_ChoreStanceMask;		// Bits for animation chores stances.
												// A set bit indicates that the aspect
												// has support for the stance.
												// 31 possible stances with the												//
												// MSB bit set indicating special meaning
												//
												// -1  This is custom created chore
												//     The full PRS name to use is embedded in
												//     the chore, we don't need to look it up

		int				m_DeformNormCounter;

		// Render sub components of the aspect separately
		void RenderMesh(Rapi* hRapi);
		void RenderNormals(Rapi* hRapi,float gizmoscale);
		void RenderBones(Rapi* hRapi,float gizmoscale);
		void RenderBoundBoxes(Rapi* hRapi);
		void RenderShield(Rapi* hRapi);

		EventCallback	m_CompletionCallback;

		SET_NO_COPYING( Aspect );

		FUBI_EXPORT static bool    __cdecl FUBI_DoesHandleMgrExist( void );
		FUBI_EXPORT static UINT    __cdecl FUBI_HandleAddRef( HAspect aspect );
		FUBI_EXPORT static UINT    __cdecl FUBI_HandleRelease( HAspect aspect );
		FUBI_EXPORT static bool    __cdecl FUBI_HandleIsValid( HAspect aspect );
		FUBI_EXPORT static Aspect* __cdecl FUBI_HandleGet( HAspect aspect );
	};

};

using nema::ToString;
using nema::FromString;

#endif
