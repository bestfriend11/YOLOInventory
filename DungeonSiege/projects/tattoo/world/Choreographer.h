#pragma once
/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		:	Choreographer.h

	Author(s)	:	Mike Biddlecombe, Bartosz Kijanka, Scott Bilas

	Purpose		:	To provide a sophisticated animation playback system that
					solves for the complex cases that can arise in a 3D game
					environment.   This system was developed because it is
					virtually impossible for  the animators to predict every
					situation that the game's characters  could find themselves
					in.

	Notes		:	Bart's original sketch of this class in included at the end
					of the file.

 //------------------------------------------------------------------------ //
 DESIGN NOTES


	Blending animation in all game situations

		Attacks

			Blending between two attacks

			Picking between two attacks
		Move

			From stand to walk to run to walk to stand

		Die

		Fidget


	Description

		The choreographer decides how to visualize what the AI has asked it to
		do.  The choreographer  takes on tasks from the AI, and does the best
		job that it can to create an accurate and smooth  visualization.  The AI
		does not concern itself with the solution the choreographer chose. For
		example, the AI asks to have a character walk between two points.  The
		choreographer looks at the environment and decides how to play the
		animations to look best.  At first the choreographer  needs to take the
		character from a stand to a walk smoothly.  It looks to see if it "has"
		the animation to make this transition, and if it does, blends the
		characters stand into a walk over time. The time that is used for the
		transition comes from the transition animation itself (the animator  has
		made this decision while creating the animation).  If the game requires
		this animation to be  sped up, a time bias can be applied to change this
		animation time (The applies to all animations  that are played in the
		system).  Once the character is walking the choreographer has to
		continue to  pay attention to the terrain beneath the characters feet.
		If the terrain is uneven and the character  needs to walk uphill for
		example, the choreographer would blend between an animation of the
		character  walking across flat ground and walking uphill.  The
		percentage would be based on the slope.


	Perfect Match Concept

		If the choreographer can find an animation that solves the requirements
		perfectly, it would use  that instead of trying to create it from
		animation components.


	Animation Data (available samples concept)

		The choreographer's goal is to make stuff look great with what it has
		available.  However, the  more animation data that is available to work
		with, the better the results.  The system does not  require that a lot
		of animation exists, but to utilize the advanced capabilities of the
		system,  a certain minimum should be available.


	Networking and the Choreographer

		$$$ this needs re-stating


	Art Required

		An animator needs to provide animations that the choreographer can use
		to solve for the situations that  arise in the game.  Here is an example
		list of animations required to take a character from a stand to  a run
		and back to a stand.

		Required Pass 1 (Abrupt transition - typical of most games today)
		Standing animation
		Walking loop

		Required Pass 2 (Smooth transition v1.0)
		Standing animation
		One step transition from stand into walk loop (straight ahead)
		Walking loop

		Required Pass 3 (Even smoother transition v1.1)
		Standing animation
		One step transition to the left, to the right, straight, and 180 deg. behind

		Required Pass 4 (Ultimate smooth transition v2.0)
		Standing animation
		One step transition in all directions
 

	Planned for Choreographer v1.1

	-	Leaning while running through a turn
	-	Supports primary and secondary focus
		Archer that can walk and keep firing at a target
		OR Archer that can stand and follow a moving target
		OR Archer can walk and keep firing at a moving target


	//------------------------------------------------------------------------
	//									IMPLEMENTATION NOTES


	Todo		:	Pass 1 - "classic"
	----			------------------

					Goal:	This pass will force us to look at most of the
							issues.  We'll have to pick conventions, define
							protocols and port the existing code to work with
							Choreographer with at least equivalent
							functionality.  I think we just have to start this
							in order to really know what we need to do.

					-	movement

						-	Interface Choreographer and NavGoals together such
							that walking, running and  stopping uses both.  The
							point is to get NavWalkGoal cooperating with the
							Choreographer.

					-	combat

						-	karate

								Basic hand-to-hand combat should use the
								choreographer, meaning things work at least as
								well as they do now.  KarateFighterGoal will
								have to  cooperate with the Choreographer.

						-	melee

								Basic melee combat works in that we have the
								single attack anim being played with the
								appropriate weapon.  Again, the point is to
								integrate with Choreographer.
								
						-	projectile
								
								Same deal as above.

  
					Pass 2 - "fancy"
					----------------

					Goal:	Once we have equivalent functionality we can plug in
							the first pass of the cool features.  Sounds like
							the biggest part of this pass is having
							Choreographer start to make basic decisions about
							playing different ( more appropriate ) animations.

					-	combat

						-	Choreographer can be querried if an attack with the 
							current weapon will fall Short, Long, or Dead On the
							current focus/target.

						-	Choreographer substitutes optimal animations for
							above attack distance estimates.


  					Pass 3 - "the other fancy"
					--------------------------

					Goal:	Make sure the work in Pass 1 and Pass two works in
							multiplayer.  This may mean extra coding, breaking
							up existing methods into multiple, and a significant
							amount of time will be spent on testing.  Since
							testing MP code always takes a  long time, so I
							would like to account for it in this separate pass.

  
					Pass 4 - "extra fancy"
					----------------------

					-	advanced actions

						-	Allow movement and action/fighting to happen in
							parallel.  This will require we solve current
							synchronization anality betweem FighterGoals and
							NavGoals.

						-	Archers aim to moving target

						-	Archers shuffle their feet to turn while aiming

						-	Archers walk and shoot

						-	Melee fighters choose appropriate attacks

					
					Pass Last - another MP adjustment pass
					--------------------------------------

					Goal:	Again, adjusting code such that all of the new
							changes work in multi player.


  					Pass 5 - "super deluxe"
					-----------------------

					-	movement
					
						-	when actors die, they align to the slope of the
							terrain

						-	runners tilt on turns

						-	improved foot placement


					Pass 6 - "bigger, badder, faster, more"
					---------------------------------------

					-	continued work on advancing Choreographer's
						improviations abilities

					-	potentially plugging in animations for actions like
						"use" or "get"  but the design doesn't call for this.
  
	Dependencies:

  
	(C)opyright 2000 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/
#ifndef CHOREOGRAPHER_H
#define CHOREOGRAPHER_H

#include "GpCore.h"
#include "FuBiDefs.h"
#include "Nema_Types.h"
#include "SkritDefs.h"
#include <map>


class FastFuelHandle;




/*============================================================================

	Class		: Chore

	Author(s)	: biddle

	Purpose		: The simple animation tasks that can be choreographed on an
				: Actor (body?) Initialized from data in the Actors' parent
				: GAS block

----------------------------------------------------------------------------*/

enum eAnimChore
{
	SET_BEGIN_ENUM( CHORE_, 0 ),

	CHORE_INVALID,	// 
	CHORE_NONE,		// No animation playing --biddle
	CHORE_ERROR,	// Bad animation requested
	CHORE_DEFAULT,	// Some animation is "playing" but we don't know/care what it is
					// all animatable GOs must reserve these three chores --biddle

	CHORE_WALK,
	CHORE_DIE,
	CHORE_DEFEND,
	CHORE_ATTACK,
	CHORE_MAGIC,
	CHORE_FIDGET,		// $$$: some aspects have multiple fidgets

	CHORE_ROTATE,
	CHORE_OPEN,
	CHORE_CLOSE,

	CHORE_MISC,		// Misc. chores don't fit into any other category
					// or are only available in specific stances

	SET_END_ENUM( CHORE_ ),
};

// MCP packets REQUIRE chores fit into a nybble!
COMPILER_ASSERT( CHORE_COUNT < (1 << 4) );

const char* ToString       ( eAnimChore ch );
bool        FromString     ( const char* str, eAnimChore& ch );
eAnimChore  ChoreFromString( const char* str );

FUBI_EXPORT eAnimChore MakeAnimChore( DWORD x );
FUBI_EXPORT int MakeInt( eAnimChore x );

/*--------------------------------------------------------------------------*/

enum eAnimStance
{
	SET_BEGIN_ENUM( AS_, -1 ),

	AS_DEFAULT,						// -1 = default (choose based on inventory)
	AS_PLAIN,						//  0 = nothing in hands (if there are hands!)
	AS_SINGLE_MELEE,				//  1 = single-handed sword/mace/hammer/club
	AS_SINGLE_MELEE_AND_SHIELD,		//  2 = single-handed and shield
	AS_TWO_HANDED_MELEE,			//  3 = two-handed axe/hammer (NOT a sword!)
	AS_TWO_HANDED_SWORD,			//  4 = two-handed sword
	AS_STAFF,						//  5 = staff
	AS_BOW_AND_ARROW,				//  6 = bow and arrow
	AS_MINIGUN,						//  7 = minigun
	AS_SHIELD_ONLY,					//  8 = shield but no weapon

	SET_END_ENUM( AS_ ),
};

// MCP packets REQUIRE stances fit into a nybble!
COMPILER_ASSERT( AS_COUNT < (1 << 4) );

COMPILER_ASSERT( (AS_COUNT - 1) == nema::MAXIMUM_STANCES );

const char* ToString  ( eAnimStance e );
bool        FromString( const char* str, eAnimStance& e );


/*--------------------------------------------------------------------------*/

class Chore
{
public:
	static const char* ANIM_SKRIT_PATH;								// Path to animation Skrits (includes terminating backslash)

// Methods.

	Chore( void );
	Chore( const Chore& chore );
	void CommitCreation();

	bool Load      ( FastFuelHandle hchore, const gpstring& chorePrefix = gpstring::EMPTY );
	void LoadStance( FastFuelHandle hanims, eAnimStance stance, const gpstring& chorePrefix = gpstring::EMPTY );

	static DWORD MakeStanceMask( eAnimStance stance )  {  gpassert( stance != AS_DEFAULT );  return ( 1 << stance );  }

	void SetStanceMask  ( eAnimStance stance )  {  m_lStanceMask |= MakeStanceMask( stance );  }
	void ClearStanceMask( eAnimStance stance )  {  m_lStanceMask &= ~MakeStanceMask( stance );  }

	Chore& operator = ( const Chore& chore );

// Data.

	// $ note: skrit objects in chores are not instanced, they are ref-counted
	//         (they all share the same skrit). when the chore is converted to
	//         an ActiveChore, then the skrit is cloned to become its own
	//         instance.

	// $$$ TODO: should store ref-counted handles to PRS keys, not to strings.

	eAnimChore					m_Verb;									// Which chore am I?

	Skrit::HAutoObject			m_AnimSkritObject;						// The skrit that is used to drive this chore

	stdx::fast_vector<gpstring>	m_vsAnimFiles[nema::MAXIMUM_STANCES];	// The names of the simple animations used by
																		// the skrit when performing the chore

	stdx::fast_vector<DWORD>	m_vlAnimFourCC;							// Map FourCCs of the simple animations to their indices
																		// in vcAnimFiles

	stdx::fast_vector<float>	m_vfAnimDurations;						// The length of the base anim for each stance
																		// Currently, this vector is only filled for the attack & magic chores


	DWORD					m_lStanceMask;								// Bit that are 'set' indicate valid stances for this
																		// chore. What each bit represents is up the
																		// the chore designer/coder
};

/*--------------------------------------------------------------------------*/

class ChoreDictionary : public std::map <eAnimChore, Chore>
{
public:
	bool Load( FastFuelHandle hdict );
};

/*--------------------------------------------------------------------------*/

#endif // CHOREOGRAPHER_H

/*--------------------------------------------------------------------------*/







#if 0

/*
--= OLD NOTES FOLLOW =--


choreograph ( from www.dictionary.com ):
1. To create the choreography of: choreograph a ballet. 
2. To plan out or oversee the movement, development, or details of; orchestrate:
advance people who choreographed the candidate's whistle-stop tour. 


Assumptions:
------------

	- The idea of content creators making custom AI is a High Ideal.  We must
	accept the posibility that all AI will be programmed by the  AI programmer.
	The #1 priority is that -we- make a cool AI; that the AI programmer be able
	to quickly create/modify new AIs.  Content creators and end-users playing
	with the AI is cool, but a secondary priority.

	- Any GO who wants to animate, must have ChoreographerStorage


Requirements:
-------------

- for animation client side: ( ai, and other systems ):
	- anyone calling the animation may or may not want to know about events with this animation ( via callback/notify interface ).  Providing
	  a notify interface should be optional.
	- the chor client notify should include such info as keys being reached in the anim, as well as begin, end, loop ... but they could also just be default keys
	- the chor client notify should be easy to debug - therefore I'd like to shy away from blindcallback, it's a pain in the ass.

- the choreographer itself will need:
	? Mike, what are your thoughts here?
	- need some amount of historical AND look-ahead information as to animations being requested.
	- need to keep track of all the requests and the current state for each GO which can animate
	- Q: what is the resolution of this information?  How large are these atomic pieces...

- multiplayer:
	- we should be easily able to duplicate the animation state of a GO on a remote machine.

*/




/*

What might the master Choreographer class look like?

*/

Chor
{

/*
	I like the sentence strucrue/natural language approach because it seems so
	intuitive to the user.  So, I imagine that the interface will at least let
	us ask for a chore to be done by identifying the chore by a "verb."  What
	that verbs maps onto, I don't know.  Maybe it maps directly to a Scrit,
	maybe it's a key to a map which then will map it to something else. ???

	So, we could ask the Choreographer to perform a chore by giving it a verb.
	Most of the time, but not all of the time, we will want to be informed by
	the Choreographer about the events related to this Chore.  So, in request,
	along with the verb we pass a pointer to a ChoreographerNotify.  ( This of
	course could be NULL, if we're not interested in the events. )

	The ChoreographerNotify could be any sort of callback mechanism.  It could
	be a blind callback, or an inherited interface. My personal preference at
	the moment is that we use an inherited interface instead of a callback.
	Back in the day, before proper precompiled headers, using blind callbacks
	saved on compile times.  Compile times are down, so blind callbacks don't
	buy us much.  They compilcate tracing or stepping through a callback with
	all the template BS, they involve more work when a callback interface
	changes, and they are limited by the max number of arguments that you can
	pass.  If we just use inheritance to get the callback interface, we're back
	to good old readable C++.  Thoughts? */

/*
	So, we could have a bunch of overloads for all the different variations of Chore requests:
*/

	bool AddChore( GOChoreographerStorage *, ChoreNotify *, gpstring const & sVerb );
	bool AddChore( GOChoreographerStorage *, ChoreNotify *, gpstring const & sVerb, SiegePos );
	bool AddChore( GOChoreographerStorage *, ChoreNotify *, gpstring const & sVerb, SiegePos, GO * pTargetObject );

/*
	Since we don't know how much stuff we will ultimately want to include in a
	Chore request this could be a problem.  We do know that this could a lot of
	parameters, and we know for sure that whatever interface we choose now will
	grow in the future.  This would make maintaining the above interface a
	problem.

	So... we could take the approach that we Add a Chore and get a new Chore
	object pointer back, off of which we can access a rich interface for setting
	the Chore parameters.

	A couple variations come to mind.  First, the butt-simple approach: just add
	a Chore, get a pointer and set the parameters:
*/

	void AddChore( GOChoreographerStorage *, ChoreNotify *, Chore * & pAction );

	-- or --

	void AddChore( GO *, ChoreNotify *, ChorAcrion * & pAction );

/*
	So, we would use the above like:

	// make a new chore:
	Chore * pChore;
	gChor.AddChore( MyGO, MyNotify, pChore );

	// set chore params:
	gChor.pChore->SetVerb( "walk" );
	gChor.pChore->SetAdverb( "tiredly" );
	gChor.pChore->SetGoalPosition( TargetPosition );

	So, here comes a question: How do we communicate to the client that the
	above Chore can't be done?  If, for whatever reason it just can't happen...?

	Since the above interface doesn't immediately return some sort success bool,
	and since the paramters aren't set until after the Chore's creation, the
	Chore will not be evaluated until the Choreographer gets to crunch it.  At
	that time, if the Chore is impossible, the Choreographer will use the passed
	ChorNotify to call the client with some sort of error or result code.

	While on the one hand this seems reasonable, there might be instance where
	the client code might want to know immediately if a Chore can be performed
	-now-.

	So, we could do something like:
*/

	bool AddChore( GO *, ChoreNotify *, gpstring const & sVerb, Chore * & pChore );

/*
	In the above request, we would pass the most critical part of the Chore; the
	verb, in as part of the initial Chore creation request.  Working with the
	assumption that the verb is the most critical part of the Chore definition,
	and that all other Chore paramters just "dress it up", the Choreograpger
	could check for any immediate conflicts with existing chores and return a
	bool as to this new Chore being created or not ( if Chore creation fails
	when there is a Chore collision ).

	So, we could write:

	Chore * pChore;

	if( gChor.AddChore( MyGO, MyNotify, "jump", pChore ) ) {
		pChore->SetDirection( DirectionVector );
	}
	else {
		// Can't do this Chore right now...
	}

	Thoughts?


	Issues:

	Asking for Chores to be done NOW vs. later, OR after a certain other Chore.
	( Accomodating look-ahead )...  We might ask for a Chore to be done NOW, and
	if we do we might want to know right then and there if that's possible.
	Also, we might just add a number of Chores to the queue, and then just treat
	the success/failure/progress messages as the come in.  How do we accomodate
	this?  Do we want to accomodate this?

	Thoughts?

	We could have:

	// This would evaluate immediately if the Chore can be started right NOW alone, or in parallel with whatever other Chores are running.
	// It would return a failure code if the Chore was not possible now.
	bool AddImmediateChore( GO *, ChoreNotify *, gpstring const & sVerb, Chore * & pChore );
	
	// this would add the new chode to be done after an existing Chore
	void AddChoreAfter( GO *, ChoreNotify *, gpstring const & sVerb, Chore * pAfterThisChore, Chore * & pNewChore );

	Thoughts?
*/

}




// ???
class GOChoreographerStorage
{
};




// This is the callback interface that a Chore client could use to receive Chore-related event messages.  Heck, maybe the
// message it receives is of WorldMessage class type.
class ChoreNotify
{
	OnChorNotify( unsigned int ChoreMessage );
};




/*
Some verbs we might have:
-------------------------

- walk/move				- each character will move differently... differs based on speed
- takeoff				- for flying creatures
- land					- for flying creatures
- fly					- 
- swing at something	- swing anim will vary based on what you're holding
- load projectile
- aim
- shoot					- shoot the projecile weapon; animations will vary based on weapon being held
- gesture for magic		-
- change weapon			- maybe? must adapt as to what weapons are being involved
- ambient				- hmm... almost infinite variety here
- look at something		- anythinig with a head should be able to turn it to face something.
*/




#endif
