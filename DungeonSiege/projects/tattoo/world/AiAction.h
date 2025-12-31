#pragma once
/*=======================================================================================

  AIQ - "AI Queries"

  purpose	:	This class contains interfaces which serve as short-cuts for assigning some
				ai commands to GROUPS of objects.  If you want to tell a singular actor 
				what to do, just talk to his Mind directly.

  author	:	Bartosz Kijanka

  date		:		11/16/1999

  Conventions:

				These methods don't do excessive parameter or context validation.  These are util
				methods and it's not their job to determine weather they should have really
				been called or not.  As long as their parameters are valid GOs, they will
				execute.  This means they will do exactly what you ask for, which could mean
				attacking dead people or unconscious people, trying to walk to silly places etc.
				It's not their job to validate context.  That's left up to the User Interface or 
				the higher-level AI functions like the brains.

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/
#ifndef __AIACTION_H
#define __AIACTION_H


enum eActiveInventorySlot;
enum eEquipSlot;
enum eJobAbstractType;
enum eJobQ;
enum eQPlace;

#include "GoDefs.h"
#include <map>


/////////////////////////////////////////////////////////////
// JobReq

struct JobReq
{
	FUBI_POD_CLASS( JobReq );

	JobReq( void );
	JobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin );

	JobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin,	eEquipSlot slot );
	JobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin,	Goid item, eEquipSlot slot );
																				
	JobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin,	Goid goal );
	JobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin,	SiegePos const & pos );
	JobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin,	Goid goal, SiegePos const & pos );
	JobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin,	Goid goal, Goid modifier );
	JobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin,	Goid goal, Goid modifier, SiegePos const & pos );

	void Init( void );

	FUBI_VARIABLE		( eJobAbstractType, m_, Jat,		   	 		"Job Abstract Type" 						);
	FUBI_VARIABLE		( eJobQ,            m_, Q,			   	 		"Job queue"   								);
	FUBI_VARIABLE		( eQPlace,          m_, QPlace,       	 		"Job queue placement" 						);      
	FUBI_VARIABLE		( eActionOrigin,	m_, Origin,    	 			"Job assignment origin"   					);
	FUBI_VARIABLE		( DWORD,			m_, PlaceBefore,			"Job placement relative to other job."		);
	FUBI_VARIABLE		( DWORD,			m_, PlaceAfter,				"Job placement relative to other job."		);      
	FUBI_VARIABLE		( Goid,             m_, Notify,       	 		"Goid to notify on completion"  			);      

	FUBI_VARIABLE		( Goid,             m_, GoalObject,   	 		"Job param - goal object"  					);      
	FUBI_VARIABLE		( Goid,             m_, GoalModifier, 	 		"Job param - goal modifier"  				);      
	FUBI_VARIABLE_BYREF	( SiegePos,         m_, GoalPosition, 	 		"Job param - goal position"  				);
	FUBI_VARIABLE_BYREF	( Quat,				m_, GoalOrientation,		"Job param - goal orientation"				);
	FUBI_VARIABLE		( eEquipSlot,       m_, Slot,         	 		"Job param - slot"   						);

	FUBI_VARIABLE		( int,              m_, Int1,        	 		"Job param - int1"   						);
	FUBI_VARIABLE		( int,              m_, Int2,        	 		"Job param - int2"   						);
	FUBI_VARIABLE		( Scid,             m_, SECommandScid, 	 		"Job param - Which SE command is asking"	);
	FUBI_VARIABLE		( float,            m_, Float1,        	 		"Job param - float1"   						);

	void Xfer( FuBi::BitPacker& packer );

#	if !GP_RETAIL
	void RpcToString( gpstring& appendHere ) const;
#	endif // !GP_RETAIL
};

FEX JobReq & MakeJobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin );
FEX JobReq & MakeJobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin, eEquipSlot slot );
FEX JobReq & MakeJobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin, Goid item, eEquipSlot slot );
FEX JobReq & MakeJobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin, Goid goal );
FEX JobReq & MakeJobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin, SiegePos const & pos );
FEX JobReq & MakeJobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin, SiegePos const & pos, Goid modifier );
FEX JobReq & MakeJobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin, Goid goal, SiegePos const & pos );
FEX JobReq & MakeJobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin, Goid goal, Goid modifier );
FEX JobReq & MakeJobReq( eJobAbstractType type, eJobQ q, eQPlace place, eActionOrigin origin, Goid goal, Goid modifier, SiegePos const & pos );


struct JobCacheSlot
{
	gpstring			m_SkritPath;
	Skrit::HAutoObject	m_hSkrit;
	DWORD				m_RefCount;
};

struct JobReqClientsXfer
{
	JobReq   m_JobReq;
	GoidColl m_Clients;

	void Xfer( FuBi::BitPacker& packer );

#	if !GP_RETAIL
	void RpcToString( gpstring& appendHere ) const;
#	endif // !GP_RETAIL
};

typedef std::map< gpstring, JobCacheSlot *, istring_less > JobCloneSourceMap;




class AIAction : public Singleton <AIAction>
{

public:

	////////////////////////////////////////
	//	types

	AIAction();
	~AIAction();
	void Shutdown();

	void HandleBroadcast( WorldMessage & msg );

	////////////////////////////////////////
	//	group command assignment

	void RSDoJob( JobReq const & req, const GoidColl& Clients );
	void RSCollectLoot( const GoidColl& Clients );

	////////////////////////////////////////
	//	magic item usage

	void RSDrinkLifeHealingPotions( GoidColl & gos, eActionOrigin jo );
	void RSDrinkManaHealingPotions( GoidColl & gos, eActionOrigin jo );
	void RSCastLifeHealingSpells(   GoidColl & gos, eActionOrigin jo );

	////////////////////////////////////////
	//	cache

	JobCloneSourceMap::iterator RegisterJobCloneSource( gpstring const & str, bool delayJobInit );
	void RegisterReference( JobCloneSourceMap::iterator slot );
	void UnregisterReference( JobCloneSourceMap::iterator slot );
	DWORD GetNumJobCloneSources()	{ return m_JobCloneSourceMap.size(); }

	
private:

	////////////////////////////////////////
	//	methods
	
FEX	FuBiCookie RSDoJobPacker( const_mem_ptr packola );
FEX FuBiCookie RSCollectLoot( const_mem_ptr ptr );
FEX FuBiCookie RSCollectLootMp( Goid member, const_mem_ptr ptr );
	void	   PackageAndSendLootMp( Goid member );
	void	   SCollectLoot( GoidColl & members );
FEX	void	   RCCollectLoot( int numLeftover, DWORD machineId );


FEX FuBiCookie RSDrinkLifeHealingPotions( const_mem_ptr ptr, eActionOrigin jo );
FEX FuBiCookie RSDrinkManaHealingPotions( const_mem_ptr ptr, eActionOrigin jo );
FEX FuBiCookie RSCastLifeHealingSpells(   const_mem_ptr ptr, eActionOrigin jo );

	////////////////////////////////////////
	//	data

	JobCloneSourceMap m_JobCloneSourceMap;	

	FUBI_SINGLETON_CLASS( AIAction, "AI action services." );
	SET_NO_COPYING( AIAction );
};


#define gAIAction AIAction::GetSingleton()


#endif

