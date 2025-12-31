#include "precomp_world.h"

#include "aiquery.h"
#include "GoBody.h"
#include "GoDefs.h"
#include "GoInventory.h"
#include "GoMind.h"

#include "reportsys.h"
#include "trigger_sys.h"
#include "trigger_conditions.h"
#include "services.h"
#include "worldtime.h"
#include "MCP.h"

using namespace trigger;

////////////////////////////////////////////////////////////////////////////////////
// enum eBoundaryCross support

static const char* s_BCrossStrings[] =
{
	"bcross_enter_at_time",
	"bcross_enter_on_node_change",
	"bcross_enter_force",
	"bcross_leave_at_time",
	"bcross_leave_on_node_change",
	"bcross_leave_forced"
};

COMPILER_ASSERT( ELEMENT_COUNT( s_BCrossStrings ) == BCROSS_COUNT );

static stringtool::EnumStringConverter s_BCrossConverter( s_BCrossStrings, BCROSS_BEGIN, BCROSS_END, BCROSS_ENTER_AT_TIME);

const char* ToString( eBoundaryCrossType e )
	{  return ( s_BCrossConverter.ToString( e ) );  }
bool FromString( const char* str, eBoundaryCrossType& e )
	{  return ( s_BCrossConverter.FromString( str, rcast <int&> ( e ) ) );  }

FUBI_EXPORT_ENUM( eBoundaryCrossType, BCROSS_BEGIN, BCROSS_COUNT );

////////////////////////////////////////////////////////////////////////////////////
// enum eBoundaryTest support

static const char* s_BTestStrings[] =
{
	"while_inside",
	"wait_for_first_message",
	"wait_for_every_message",
	"on_every_enter",
	"on_first_enter",
	"on_every_first_enter",
	"on_unique_enter",
	"on_every_leave",
	"on_first_leave",
	"on_last_leave",
	"on_every_last_leave",
	"on_first_message",
	"on_every_message",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_BTestStrings ) == BTEST_COUNT );

static stringtool::EnumStringConverter s_BTestConverter( s_BTestStrings, BTEST_BEGIN, BTEST_END, BTEST_ON_EVERY_ENTER);

const char* ToString( eBoundaryTest e )
	{  return ( s_BTestConverter.ToString( e ) );  }
bool FromString( const char* str, eBoundaryTest& e )
	{  return ( s_BTestConverter.FromString( str, rcast <int&> ( e ) ) );  }

FUBI_EXPORT_ENUM( eBoundaryTest, BTEST_BEGIN, BTEST_COUNT );
FUBI_DECLARE_ENUM_TRAITS( eBoundaryTest, BTEST_WHILE_INSIDE );

void FetchBoundaryTestFormat(Parameter::format& p)
{
	p.clear();
	p.sName			= "Boundary Check";
	p.isString		= true;
	p.isRequired	= true;
	p.s_default		= ToString(BTEST_ON_EVERY_ENTER);
	for (eBoundaryTest bt = BTEST_BEGIN; bt != BTEST_END; bt++)
	{
		if (   (bt != BTEST_ON_FIRST_MESSAGE) 
			&& (bt != BTEST_ON_EVERY_MESSAGE)
			)
		{
			p.sValues.push_back( ToString(bt) );
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////
#ifdef USE_PARANOID_TESTS
#define PARANOID_IN_WORLD_TEST																			\
	gpassert(m_pTrigger && m_pTrigger->GetOwner() && m_pTrigger->GetOwner()->IsInActiveWorldFrustum()); \
	if (!(m_pTrigger && m_pTrigger->GetOwner() && m_pTrigger->GetOwner()->IsInActiveWorldFrustum()))    \
	{																									\
		return false;																					\
	}																									
#else
#define PARANOID_IN_WORLD_TEST
#endif

#define PARTY_MEMBER_FILTER (OF_HUMAN_CONTROLLED | OF_ACTORS ) //| OF_PARTY_MEMBERS | OF_ALIVE_OR_GHOST


///&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
inline bool DirectedSegmentIntersectsSphere(const vector_3& p0,
											const vector_3& p1,
											const float sphere_radius,
											float& tEnter,
											float& tLeave)
{
	//$$$$ Polish up so that co-incidental spheres are handled

	// This is the 3D version of the 2D penetration algorithm
	
	// Modified from Eberly's Magic Software 3D library and Foley van Dam
    // set up quadratic Q(t) = a*t^2 + 2*b*t + c

	// where 
	// a = v.v
	// b = v.u
	// c = (u.u) - (r*r)
	

	// u = (p0-co) -- here the center CO is the origin <0,0,0> so u == p0
	// v = (p1-p0)

	vector_3 v = p1 - p0;

    float a = v.Length2();

    if ( IsZero(a) )
	{
		// The segment is zero length
		tEnter = -2.0f;
		tLeave =  2.0f;
		return false;
	}
	
    float b = p0.DotProduct(v);

    float c = p0.Length2() - Square(sphere_radius);

    if ( IsPositive(b) )
	{
		// We are pointing away from the sphere origin
		if (c > 0.0f) 
		{
			// We are outside the sphere, facing away
			tEnter = -2.0f;
			tLeave =  2.0f;
			return false;
		}
	}

	float discrim = Square(b)-a*c;

    if ( IsNegative(discrim)  )
    {
		tEnter = -2.0f;
		tLeave =  2.0f;
		return false;
	}
	else if (IsZero(discrim))
	{
		float tmp = -b;
		if  (tmp > a) 
		{
			tEnter = -2.0f;
			tLeave =  2.0f;
			return false;
		}
		tEnter = -2.0f;
		tLeave = tmp/a;
		return true;
	}

	float discroot = SQRTF(discrim);

	float root0 = -b - discroot;
	float root1 = -b + discroot;

	if (IsPositive( root0 - a ) || IsNegative( root1 ))
	{
		// Entirely outside
		tEnter = -2.0f;
		tLeave =  2.0f;
		return false;
	}

	if (!IsPositive( root0 ) && !IsNegative( root1 - a ))
	{
		// Entirely inside
		tEnter = 0.5f;
		tLeave = 0.5f;
		return false;
	}

	if (IsPositive(root0))
	{
		if (IsEqual(root0,a))
		{
			tEnter = 1.0f;
		}
		else
		{
			tEnter = root0/a;
		}
	}
	else
	{
		tEnter = -2.0f;
	}
	
	if (IsPositive( root1 - a ))
	{
		tLeave = 2.0f;
	}
	else
	{
		if (IsEqual(root1,a))
		{
			tLeave = 1.0f;
		}
		tLeave = root1/a;
	}

	bool ret = false;
	
	if ((tEnter>-0.0001f) && (tEnter<0.9999f))
	{
		if (tEnter<0.0f)
		{
			tEnter = 0.0f;
		}
		ret = true;
	}
	else
	{
		tEnter = -2.0f;
	}

	if (tLeave>0.0001f && tLeave<1.0001f)
	{
		if (tLeave>1.0f)
		{
			tLeave = 1.0f;
		}
		ret = true;
	}
	else
	{
		tLeave = 2.0f;
	}

	return ret;

}

///&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
bool CheckSphereCollision( const Go*		in_Mover,
						   const SiegePos&	in_Center,
						   float			in_Radius,
						   eOccupantFilter	in_Filter, 
						   const SiegePos&	in_PosA, 
						   const SiegePos&	in_PosB, 
						   float&			out_tEnter, 
						   float&			out_tLeave,
						   bool&			out_AlreadyInside,
						   bool&			out_OutsideAfterwards) 

{						   
	bool hit = false;
	out_AlreadyInside = false;
	out_OutsideAfterwards = true;

	if ( (in_Filter == OF_NONE) || gAIQuery.Filter( in_Mover, 0, in_Filter ) )
	{
		vector_3 p0 = gSiegeEngine.GetDifferenceVector(in_Center,in_PosA);
		vector_3 p1 = gSiegeEngine.GetDifferenceVector(in_Center,in_PosB);

		out_AlreadyInside     = p0.Length2() <= (in_Radius*in_Radius);
		out_OutsideAfterwards = p1.Length2() >  (in_Radius*in_Radius);

		if (out_AlreadyInside && !out_OutsideAfterwards)
		{
			// We remain entirely inside the sphere
			out_tEnter = -2.0f;
			out_tLeave =  2.0f;
			hit = false;
		}
		else
		{
			hit = DirectedSegmentIntersectsSphere( p0, p1, in_Radius, out_tEnter, out_tLeave );

#if !GP_RETAIL
			if (gWorldOptions.GetShowTriggerSysHits() || gWorldOptions.GetShowTriggerSys())
			{
				if (hit)
				{
					SiegePos s0 = in_Center;

					static float angle = 0;
					
					if (out_tEnter>=0.0f)
					{
						angle+=(PI/8.0f);
						s0.pos += p0 + ((p1-p0) * out_tEnter);
						gWorld.DrawDebugBoxStack(s0, 0.1f, 0xffffffff, 5.0f, YRotationColumns(angle));
					}

					if (out_tLeave<=1.0f)
					{
						angle+=(PI/8.0f);
						s0.pos = in_Center.pos + (p0 + ((p1-p0) * out_tLeave));
						gWorld.DrawDebugBoxStack(s0, 0.1f, 0xffffffff, 5.0f, YRotationColumns(angle));
					}
				}
			}
#endif
		}
	}

	return hit;
}

///&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
bool CheckInsideSphere( const Go*		in_Mover,
						const SiegePos&	in_Center,
						float			in_Radius,
						eOccupantFilter	in_Filter, 
						const SiegePos&	in_Pos) 

{						   
	bool hit = false;

	if ( (in_Filter == OF_NONE) || gAIQuery.Filter( in_Mover, 0, in_Filter ) )
	{
		vector_3 p0 = gSiegeEngine.GetDifferenceVector(in_Center,in_Pos);
		hit = p0.Length2() <= (in_Radius*in_Radius);
	}
#if !GP_RETAIL
	if (gWorldOptions.GetShowTriggerSysHits() || gWorldOptions.GetShowTriggerSys())
	{
		if (hit)
		{
			static float angle = 1.0f;
			angle+=(PI/8.0f);
			gWorld.DrawDebugBoxStack(in_Pos, 0.1f, 0xffffff00, 5.0f);
		}
	}
#endif
	return hit;
}

///&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
bool CLIPt (float denom, float num, float& tE, float& tL)
{
	float t;
	if (denom>0.0001f)
	{
		t = num/denom;
		if (t > tL)
		{
			return false;
		} 
		else if (t > tE)
		{
			tE = t;
		}
	}
	else if (denom<-0.0001f)
 	{
		t = num/denom;
		if (t < tE)
		{
			return false;
		} 
		else if (t < tL)
		{
			tL = t;
		}
	}
	else if (num > 0.0001f)
	{
		return false;
	}
	return true;
}


///&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
bool DirectedSegmentIntersectsBox( const vector_3&	hd,			/*box halfdiag */
								   const vector_3&	ip,			/*initial point */
								   const vector_3&	rd,			/*ray direction */
								   float&			out_tEnter, 
								   float&			out_tLeave 
								 )

{
	bool ret = false;

	// Liang-Barsky parametric line clipper
	if (!IsZero(rd))
	{
		out_tEnter = 0.0f;
		out_tLeave = 1.0f;

		if (CLIPt(rd.x,-hd.x-ip.x,out_tEnter,out_tLeave))
			if (CLIPt(-rd.x,ip.x-hd.x,out_tEnter,out_tLeave))
				if (CLIPt(rd.y,-hd.y-ip.y,out_tEnter,out_tLeave))
					if (CLIPt(-rd.y,ip.y-hd.y,out_tEnter,out_tLeave))
						if (CLIPt(rd.z,-hd.z-ip.z,out_tEnter,out_tLeave))
							if (CLIPt(-rd.z,ip.z-hd.z,out_tEnter,out_tLeave))
							{
								ret = true;
							}
	}

	return ret;
}


///&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
bool CheckBoxCollision( const Go*			in_Mover,
						const SiegePos&		in_Center,
						const matrix_3x3&	in_InvOrient,
						const vector_3&		in_HalfDiag,
						eOccupantFilter		in_Filter, 
						const SiegePos&		in_PosA, 
						const SiegePos&		in_PosB, 
						float&				out_tEnter, 
						float&				out_tLeave,
						bool&				out_AlreadyInside,
						bool&				out_OutsideAfterwards) 
{
	bool hit = false;
	out_AlreadyInside = false;
	out_OutsideAfterwards = true;

	if ( (in_Filter == OF_NONE) || gAIQuery.Filter( in_Mover, 0, in_Filter ) )
	{

		// Calculate the local origin and direction and run a basic axis-aligned test on the ray

		vector_3 local_orig = in_InvOrient * gSiegeEngine.GetDifferenceVector(in_Center,in_PosA);
		vector_3 local_dest = in_InvOrient * gSiegeEngine.GetDifferenceVector(in_Center,in_PosB);

		out_AlreadyInside =		(local_orig.x <= in_HalfDiag.x) && (local_orig.x >= -in_HalfDiag.x) &&
								(local_orig.y <= in_HalfDiag.y) && (local_orig.y >= -in_HalfDiag.y) &&
								(local_orig.z <= in_HalfDiag.z) && (local_orig.z >= -in_HalfDiag.z);

		out_OutsideAfterwards =	(local_dest.x >  in_HalfDiag.x) || (local_dest.x <  -in_HalfDiag.x) ||
								(local_dest.y >  in_HalfDiag.y) || (local_dest.y <  -in_HalfDiag.y) ||
								(local_dest.z >  in_HalfDiag.z) || (local_dest.z <  -in_HalfDiag.z);
			
		if (out_AlreadyInside && !out_OutsideAfterwards)
		{
			// We remain entirely inside the box
			out_tEnter = -2.0f;
			out_tLeave =  2.0f;
			hit = false;
		}
		else
		{
			vector_3 coord( DoNotInitialize );
			hit = DirectedSegmentIntersectsBox( in_HalfDiag,
												local_orig,
												local_dest-local_orig,
												out_tEnter,
												out_tLeave );
#if !GP_RETAIL
			if (gWorldOptions.GetShowTriggerSysHits() || gWorldOptions.GetShowTriggerSys())
			{
				if (hit)
				{
					SiegePos s0 = in_Center;

					static float angle = 0.0f;
					if (!out_AlreadyInside && (out_tEnter >= 0.0f))
					{
						angle+=(PI/8.0f);
						s0.pos += Transpose(in_InvOrient) * (local_orig + ((local_dest-local_orig) * out_tEnter));
						gWorld.DrawDebugBoxStack(s0, 0.1f, 0xffffffff, 5.0f);
					}

					if (out_OutsideAfterwards && (out_tLeave <= 1.0f))
					{
						angle+=(PI/8.0f);
						s0.pos = in_Center.pos + (Transpose(in_InvOrient) * (local_orig + ((local_dest-local_orig) * out_tLeave)));
						gWorld.DrawDebugBoxStack(s0, 0.1f, 0xffffffff, 5.0f);
					}
				}
			}
#endif
		}
	}

	return hit;

}
///&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
bool CheckInsideBox( const Go*			in_Mover,
					 const SiegePos&	in_Center,
 					 const matrix_3x3&	in_InvOrient,
					 const vector_3&	in_HalfDiag,
					 eOccupantFilter	in_Filter, 
					 const SiegePos&	in_Pos) 

{						   
	bool hit = false;

	if ( (in_Filter == OF_NONE) || gAIQuery.Filter( in_Mover, 0, in_Filter ) )
	{
		vector_3 local_pos = in_InvOrient * gSiegeEngine.GetDifferenceVector(in_Center,in_Pos);
		hit = (local_pos.x <= in_HalfDiag.x) && (local_pos.x >= -in_HalfDiag.x) &&
			  (local_pos.y <= in_HalfDiag.y) && (local_pos.y >= -in_HalfDiag.y) &&
			  (local_pos.z <= in_HalfDiag.z) && (local_pos.z >= -in_HalfDiag.z);
	}
#if !GP_RETAIL
	if (gWorldOptions.GetShowTriggerSysHits() || gWorldOptions.GetShowTriggerSys())
	{
		if (hit)
		{
			static float angle = 1.0f;
			angle+=(PI/8.0f);
			gWorld.DrawDebugBoxStack(in_Pos, 0.1f, 0xffffff00, 5.0f);
		}
	}
#endif
	return hit;
}


FUBI_DECLARE_SELF_TRAITS( trigger::MessageHeader );

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//
// Conditions
//
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
//
// Condition base class methods
//

bool Condition::EvaluateSatisfaction()
{
	bool retval = false;

	if (IsMessageHandler())
	{
		if (m_BoundaryTest == BTEST_ON_EVERY_MESSAGE)
		{
			retval = !m_CurrentSatisfyingMessages.empty();
			m_bIsReset = true;
		}
		else if (m_BoundaryTest == BTEST_ON_FIRST_MESSAGE)
		{
			// This cases are only satisfied once, they never get reset
			if (m_bIsReset)
			{
				if (!m_CurrentSatisfyingMessages.empty())
				{
					m_bIsReset = false;
					retval = true;
				}
			}
			else
			{
				// NOT reset, so it must have been satisfied
				return false;
			}
		}
	}
	else
	{
		if ((m_BoundaryTest == BTEST_WHILE_INSIDE) ||
			(m_BoundaryTest == BTEST_WAIT_FOR_EVERY_MESSAGE)
			)
		{
			retval = !m_CurrentSatisfyingOccupants.empty();
		}
		else if ( (m_BoundaryTest == BTEST_ON_EVERY_ENTER) ||
			      (m_BoundaryTest == BTEST_ON_EVERY_FIRST_ENTER) ||
			      (m_BoundaryTest == BTEST_ON_EVERY_LEAVE) ||
			      (m_BoundaryTest == BTEST_ON_EVERY_LAST_LEAVE) ||
				  (m_BoundaryTest == BTEST_ON_UNIQUE_ENTER) 
				  )
		{
			retval = !m_CurrentSatisfyingOccupants.empty();
			m_bIsReset = !retval;
		}
		else if ( (m_BoundaryTest == BTEST_WAIT_FOR_FIRST_MESSAGE) ||
			      (m_BoundaryTest == BTEST_ON_FIRST_ENTER) ||
				  (m_BoundaryTest == BTEST_ON_FIRST_LEAVE) ||
				  (m_BoundaryTest == BTEST_ON_LAST_LEAVE)
				  )
		{
			// This cases are only satisfied once, they never get reset
			if (m_bIsReset)
			{
				if (!m_CurrentSatisfyingOccupants.empty())
				{
					m_bIsReset = false;
					retval = true;
				}
			}
			else
			{
				// And it they are NOT reset, they must be satisfied
				retval = false;
			}
		}
	}
	return retval;
}

bool Condition::Reset()
{
	m_bIsReset = true;
	if (IsMessageHandler())
	{
		m_CurrentSatisfyingMessages.clear();
	}
	else
	{
		m_CurrentSatisfyingOccupants.clear();
		if (m_BoundaryTest == BTEST_ON_UNIQUE_ENTER)
		{
			m_CurrentInsideBoundary.clear();
		}
		else
		{
			GoOccupantMapIter it = m_CurrentInsideBoundary.begin();
			for (it; it != m_CurrentInsideBoundary.end(); ++it)
			{
				OnEnterBoundary((*it).first);
			}
		}
	}
	return true;
}

void Condition::OnEnterBoundary(const Goid in_Goid)
{
	switch (m_BoundaryTest)
	{
		case BTEST_ON_EVERY_FIRST_ENTER :
		{
			if (m_CurrentInsideBoundary.size() != 1)
			{
				// Bail if we already have an occupant
				break;
			}
			// Fall through to next case
		}
		case BTEST_ON_EVERY_ENTER :
		case BTEST_WHILE_INSIDE :
		case BTEST_WAIT_FOR_EVERY_MESSAGE :
		case BTEST_WAIT_FOR_FIRST_MESSAGE :
		{
			GoOccupantMapIter it = m_CurrentSatisfyingOccupants.find(in_Goid);
			if (it == m_CurrentSatisfyingOccupants.end())
			{
				m_CurrentSatisfyingOccupants[in_Goid] = 1;
			}
			else
			{
				(*it).second++;
			}
			break;
		}
		case BTEST_ON_FIRST_ENTER :
		{
			if (m_bIsReset)
			{
				m_CurrentSatisfyingOccupants[in_Goid] = 1;
			}
			break;
		}
		case BTEST_ON_UNIQUE_ENTER :
		{
			m_CurrentSatisfyingOccupants[in_Goid] = 1;
			break;
		}
/*
		case BTEST_ON_EVERY_LAST_LEAVE :
		case BTEST_ON_EVERY_LEAVE :
		{
			if (m_bIsReset && IsGroupWatcher())
			{
				GoOccupantMapIter it = m_CurrentSatisfyingOccupants.find(in_Goid);
				if (it != m_CurrentSatisfyingOccupants.end())
				{
					// This is a problem with some wacky grouped triggers
					// The go has re-entered the boundary BEFORE it was evaluated
					// we will remove this entry
					if ((*it).second > 1)
					{
						(*it).second--;
					}
					else
					{
						m_CurrentSatisfyingOccupants.erase(it);
					}
				}
			}
		}
*/
	}
}

void Condition::OnLeaveBoundary(const Goid in_Goid)
{
	switch (m_BoundaryTest)
	{
/*		case BTEST_ON_EVERY_ENTER :
		{
			if (!m_bIsReset && IsGroupWatcher())
			{
				break;
			}
			// This is a problem with some wacky grouped triggers
			// The go has left the boundary BEFORE it was evaluated
			// we will fall through and remove it!
		}
*/	
		case BTEST_WHILE_INSIDE :
		case BTEST_WAIT_FOR_EVERY_MESSAGE :
		case BTEST_WAIT_FOR_FIRST_MESSAGE :
		{
			GoOccupantMapIter it = m_CurrentSatisfyingOccupants.find(in_Goid);
			if (it != m_CurrentSatisfyingOccupants.end())
			{
				if ((*it).second > 1)
				{
					(*it).second--;
				}
				else
				{
					m_CurrentSatisfyingOccupants.erase(it);
				}
			}
			break;
		}
		case BTEST_ON_EVERY_LAST_LEAVE :
		{
			if (!m_CurrentInsideBoundary.empty())
			{
				// Bail if we still have an occupant
				break;
			}
			m_CurrentSatisfyingOccupants.clear();
			m_bIsReset = true;
			m_CurrentSatisfyingOccupants[in_Goid] = 1;
			break;
		}
		case BTEST_ON_LAST_LEAVE :
		{
			if (!m_CurrentInsideBoundary.empty())
			{
				// Bail if we still have an occupant
				break;
			}
			if (m_CurrentSatisfyingOccupants.empty())
			{
				m_CurrentSatisfyingOccupants[in_Goid] = 1;
			}
			break;
		}
		case BTEST_ON_EVERY_LEAVE :
		{
			GoOccupantMapIter it = m_CurrentSatisfyingOccupants.find(in_Goid);
			if (it == m_CurrentSatisfyingOccupants.end())
			{
				m_CurrentSatisfyingOccupants[in_Goid] = 1;
			}
			else
			{
				(*it).second++;
			}
			break;
		}
		case BTEST_ON_FIRST_LEAVE :
		{
			if (m_bIsReset)
			{
				m_CurrentSatisfyingOccupants[in_Goid] = 1;
			}
			break;
		}
	}
}

bool Condition::AddOccupantIntoBoundary(const Goid in_EnteringGoid)
{
	gpassert(!IsGroupWatcher());

	if (IsDormant()) return false;

	bool ret = true;

	GoOccupantMapIter it = m_CurrentInsideBoundary.find(in_EnteringGoid);
	if (it == m_CurrentInsideBoundary.end())
	{
		m_CurrentInsideBoundary[in_EnteringGoid] = 1;
		OnEnterBoundary(in_EnteringGoid);
	}
	else
	{
		if (m_BoundaryTest != BTEST_ON_UNIQUE_ENTER)
		{
			Go* entergo = GetGo(in_EnteringGoid);
			gpdevreportf( &gTriggerSysContext,(
				"[TRIGGERSYS: %s:0x%08x] attempted to enter a condition boundary "
				"more than once.\n\tCondition [%s:0x%08x:0x%04x] Trigger Owner [%s:0x%08x]\n",
				entergo ? entergo->GetTemplateName() : "unknown",
				entergo ? MakeInt(entergo->GetScid()) : -1,
				GetName(),
				(m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetGoid()) : -1,
				GetID(),
				(m_pTrigger && m_pTrigger->GetOwner()) ? m_pTrigger->GetOwner()->GetTemplateName() : "unknown",
				(m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1
				));
			ret = false;
		}
	}
	return ret;
}

bool Condition::RemoveOccupantFromBoundary(const Goid in_ExitingGoid, bool isDeactivating, GoOccupantMapIter& outIter)
{
	gpassert(!IsGroupWatcher());

	if (IsDormant())
	{
		outIter = m_CurrentInsideBoundary.end();
		return true;
	};

	bool ret = true;
	outIter = m_CurrentInsideBoundary.find(in_ExitingGoid);
	if (outIter != m_CurrentInsideBoundary.end())
	{
		gpassert(m_CurrentInsideBoundary[in_ExitingGoid] == 1);

		if (isDeactivating || (m_BoundaryTest != BTEST_ON_UNIQUE_ENTER))
		{
			outIter = m_CurrentInsideBoundary.erase(outIter);
			OnLeaveBoundary(in_ExitingGoid);
		}
		else
		{
			++outIter;
		}
	}
	else
	{
		Go* exitgo = GetGo(in_ExitingGoid);
		gpdevreportf( &gTriggerSysContext,(
			"TRIGGERSYS: [%s:0x%08x] attempted to leave a condition boundary "
			"it has never entered. Condition [%s:0x%08x:0x%04x] Trigger Owner [%s:0x%08x]\n",
			exitgo ? exitgo->GetTemplateName() : "unknown",
			exitgo ? MakeInt(exitgo->GetScid()) : -1,
			GetName(),
			(m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetGoid()) : -1,
			GetID(),
			(m_pTrigger && m_pTrigger->GetOwner()) ? m_pTrigger->GetOwner()->GetTemplateName() : "unknown",
			(m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1
			));
		ret = false;
	}
	return ret;
}

bool Condition::Xfer( FuBi::PersistContext& persist )
{
	persist.XferHex ( "m_wId",                        m_wId       );
	persist.Xfer    ( "m_iSubGroup",                  m_iSubGroup );
	persist.Xfer    ( "m_bIsReset",                   m_bIsReset  );
	persist.Xfer    ( "m_sDoc",                       m_sDoc      );
	persist.Xfer    ( "m_BoundaryTest",               m_BoundaryTest               );
	persist.XferMap ( "m_CurrentInsideBoundary",      m_CurrentInsideBoundary      );
	persist.XferMap ( "m_CurrentSatisfyingOccupants", m_CurrentSatisfyingOccupants );
	persist.XferList( "m_CurrentSatisfyingMessages",  m_CurrentSatisfyingMessages  );
	return true;
}

#if !GP_RETAIL
bool Condition::Validate( const Trigger & trigger, gpstring& out_errmsg ) const
{
	if (trigger.IsOneShot() && (m_BoundaryTest == BTEST_ON_UNIQUE_ENTER))
	{
		out_errmsg.appendf( "DATA Error: You have an ON_UNIQUE_ENTER condition attached a one-shot trigger. Trigger owner Scid 0x%08x\n",
			trigger.GetOwner() ? MakeInt(trigger.GetOwner()->GetScid()) : -1 );
		return false;
	}
	return true;
}
#endif

///////////////////////////////////////////////////////////////////////////////////
//
// receive_world_message( "event type" );
//
receive_world_message::receive_world_message()
{
	m_WorldEventExpected = WE_REQ_ACTIVATE;
	m_WorldEventIsQualified = false;
	m_WorldEventQualifier = 0;
	m_BoundaryTest = BTEST_ON_EVERY_MESSAGE;
}

void receive_world_message::SetTrigger(Trigger* trig) 
{

	if (m_pTrigger != trig)
	{
		m_pTrigger = trig;

		if (m_pTrigger && m_WorldEventExpected == WE_CONSTRUCTED )
		{
			m_pTrigger->SetWaitsForConstruction(true);
		}

		if (trig &&
			trig->GetOwner() &&
			trig->GetOwner()->HasPlacement())
		{
			m_Center = m_pTrigger->GetOwner()->GetPlacement()->GetPosition();

#if !GP_RETAIL

			if( gTriggerSysContext.IsEnabled() )
			{
				const char *lbl = (m_pTrigger && m_pTrigger->GetOwner()) ?
					m_pTrigger->GetOwner()->GetTemplateName() : "owner-STILL-not-known";
				int ts = (m_pTrigger && m_pTrigger->GetOwner()) ? (int)m_pTrigger->GetOwner()->GetGoid() : 0;
				int tn = (m_pTrigger && m_pTrigger->GetOwner()) ? m_pTrigger->GetTruid().GetTrigNum() : 0;
				int sc = (m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : 0;

				gpdevreportf( &gTriggerSysContext,("trigger condition [%s:0x%08x:%04x:0x%08x] waiting for [%s]\n",lbl,ts,tn,sc,ToString(m_WorldEventExpected)));
			}
# endif
		}
	}
}

bool receive_world_message::IsComplicated() const
{
	return  (m_BoundaryTest == BTEST_ON_FIRST_MESSAGE) && HasSatisfyingMessages();
}

void receive_world_message::FetchParameterList(Parameter &parameterlist)
{
	// Define the format of the parameter list (for SiegeEditor)

	// Parameter 0
	Parameter::format p0;
	p0.sName		= "World Message Event";
	p0.isString		= true;
	p0.isRequired	= true;
	p0.s_default	= ::ToString( m_WorldEventExpected );

	for( UINT32 i = 0; i != WE_COUNT; ++i )
	{
		p0.sValues.push_back( ToString( eWorldEvent( i ) ) );
	}

	parameterlist.Add( p0 );

	// Parameter 1
	Parameter::format p1;
	p1.sName		= "Qualifier";
	p1.isInt		= true;
	p1.isRequired	= m_WorldEventIsQualified;
	p1.i_default	= m_WorldEventQualifier;

	parameterlist.Add( p1 );

	// Special 'boundary' tests
	Parameter::format px;
	px.sName		= "Message Check";
	px.isString		= true;
	px.isRequired	= true;
	px.s_default	= ToString(BTEST_ON_EVERY_MESSAGE);
	px.sValues.push_back( ToString(BTEST_ON_EVERY_MESSAGE) );
	px.sValues.push_back( ToString(BTEST_ON_FIRST_MESSAGE) );
	parameterlist.Add( px );
}

bool receive_world_message::StoreParameterValues(const Params &parms)
{
	gpassert(m_wId == parms.ParamID);
	m_sDoc = parms.Doc;
	m_iSubGroup = parms.SubGroup;
	// Convert the messages set by SiegeEditor to useable values

	if (!parms.strings.empty())
	{
		FromString(parms.strings[ 0 ],m_WorldEventExpected);

		if (parms.strings.size() > 1)
		{
			FromString(parms.strings[ 1 ],m_BoundaryTest);
		}
		else
		{
			m_BoundaryTest = BTEST_ON_EVERY_MESSAGE;
		}
		
		if (!parms.ints.empty() && parms.ints[0] != 0)
		{
			m_WorldEventIsQualified = true;
			m_WorldEventQualifier = parms.ints[ 0 ];
		}
		else
		{
			m_WorldEventIsQualified = false;
			m_WorldEventQualifier = 0;
		}

		if ( m_pTrigger && (m_WorldEventExpected == WE_CONSTRUCTED) )
		{
			m_pTrigger->SetWaitsForConstruction(true);
		}

	}
	else
	{
		m_WorldEventExpected = WE_INVALID;
		m_WorldEventIsQualified = false;
		m_WorldEventQualifier = 0;
	}

	if (m_WorldEventExpected==WE_INVALID)
	{
		gperrorf(("CONTENT ERROR: Attempted to construct a condition that receives an invalid message [%s]\n",
			parms.strings[0]));
		gpdevreportf( &gTriggerSysContext,("CONTENT ERROR: Attempted to construct a condition that receives an invalid message [%s]\n",
			parms.strings[0]));
		return false;
	}

	return true;
}

bool receive_world_message::FetchParameterValues( Params &parms )
{
	parms.Clear();
	parms.Doc = m_sDoc;
	parms.SubGroup = m_iSubGroup;
	parms.ParamID = m_wId; 
	parms.strings.push_back(::ToString(m_WorldEventExpected));
	if (m_WorldEventIsQualified)
	{
		parms.ints.push_back(m_WorldEventQualifier);
	}
	else
	{
		parms.ints.push_back(0);
	}
	parms.strings.push_back(ToString(m_BoundaryTest));

	return true;
}

bool receive_world_message::HandleMessage( Trigger &trigger, const WorldMessage &message )
{
	UNREFERENCED_PARAMETER(trigger);

	bool mess_ok =
		   ( m_WorldEventExpected == message.GetEvent() )
		&& ( !m_WorldEventIsQualified || ( m_WorldEventQualifier == (int)message.GetData1() ) );

	if (mess_ok)
	{

		if (m_BoundaryTest == BTEST_ON_EVERY_MESSAGE)
		{
			if (!m_bIsReset)
			{
				m_bIsReset = true;
				m_CurrentSatisfyingMessages.clear();
			}
		}

		if (m_bIsReset)
		{
#if !GP_RETAIL
			if( gTriggerSysContext.IsEnabled() )
			{
				const char *lbl = (m_pTrigger && m_pTrigger->GetOwner()) ?
					m_pTrigger->GetOwner()->GetTemplateName() : "owner-STILL-not-known";
				int ts = (m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetGoid()) : 0;
				int tn = (m_pTrigger && m_pTrigger->GetOwner()) ? m_pTrigger->GetTruid().GetTrigNum() : 0;
				int sc = (m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : 0;

				gpdevreportf( &gTriggerSysContext,("trigger condition [%s:0x%08x:%04x:0x%08x] detected [%s:0x%08x]\n",lbl,ts,tn,sc,ToString(m_WorldEventExpected),m_WorldEventQualifier));
			}
#endif
			m_CurrentSatisfyingMessages.push_back(message);
		}
		else
		{
#if !GP_RETAIL
			if( gTriggerSysContext.IsEnabled() )
			{
				const char *lbl = (m_pTrigger && m_pTrigger->GetOwner()) ?
					m_pTrigger->GetOwner()->GetTemplateName() : "owner-STILL-not-known";

				int ts = (m_pTrigger && m_pTrigger->GetOwner()) ? (int)m_pTrigger->GetOwner()->GetGoid() : 0;
				int tn = (m_pTrigger && m_pTrigger->GetOwner()) ? m_pTrigger->GetTruid().GetTrigNum() : 0;
				int sc = (m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : 0;

				gpdevreportf( &gTriggerSysContext,("trigger condition [%s:0x%08x:%04x:0x%08x] received duplicate [%s:0x%08x]\n",lbl,ts,tn,sc,ToString(m_WorldEventExpected),m_WorldEventQualifier));
			}
#endif
		}
	}

	return true;
}

bool receive_world_message::Evaluate()
{
	bool ret = EvaluateSatisfaction();

#if !GP_RETAIL
	if( gTriggerSysContext.IsEnabled() )
	{
		if (ret)
		{
			const char *lbl = (m_pTrigger && m_pTrigger->GetOwner()) ?
				m_pTrigger->GetOwner()->GetTemplateName() : "owner-STILL-not-known";
			int ts = (m_pTrigger && m_pTrigger->GetOwner()) ? (int)m_pTrigger->GetOwner()->GetGoid()   : 0;
			int tn = (m_pTrigger && m_pTrigger->GetOwner()) ? m_pTrigger->GetTruid().GetTrigNum() : 0;
			int sc = (m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : 0;

			gpdevreportf( &gTriggerSysContext,("trigger condition [%s:0x%08x:%04x:0x%08x] has acknowledged receipt of [%s:0x%08x]\n",lbl,ts,tn,sc,ToString(m_WorldEventExpected),m_WorldEventQualifier));
		}
	}
# endif

	return ( ret );
}

bool receive_world_message::Xfer( FuBi::PersistContext& persist )
{
	Condition::Xfer(persist);
	persist.Xfer( "m_WorldEventExpected",    m_WorldEventExpected    );
	persist.Xfer( "m_WorldEventIsQualified", m_WorldEventIsQualified );
	persist.Xfer( "m_WorldEventQualifier",   m_WorldEventQualifier   );
	return true;
}

#if !GP_RETAIL
void receive_world_message::Draw( const ConditionDb& cond )
{
	if ((gWorldOptions.TestDebugHudOptions( DHO_LABELS ) && !gGoDb.IsEditMode()) &&
		m_pTrigger &&
		m_pTrigger->GetOwner() &&
		m_pTrigger->GetOwner()->HasPlacement())
	{
		SiegePos LabelPos;
		gpstring lbl,lblsuffix;
		if (m_pTrigger->GetOwner() && m_pTrigger->GetOwner()->HasCommon())
		{
			m_pTrigger->GetOwner()->GetCommon()->PadDebugLabelHelper(m_pTrigger,"\n\n\n\n",lbl,lblsuffix);
		}
		Truid tr = m_pTrigger->GetTruid();
		ConditionConstIter iCondition = cond.begin();
		for(; iCondition != cond.end(); ++iCondition )
		{
			if (m_wId == (*iCondition).second->GetID())
			{
				double et = (m_pTrigger->GetResetTimeout() <= 0.0f) ? 0 : PreciseSubtract(m_pTrigger->GetResetTimeout(), gWorldTime.GetTime());
				lbl.appendf("%s:receive_world_message:%5.2f",m_pTrigger->GetCurrentActive() ? (IsDormant()?"X":"A"):"D", et );
				int sc = m_pTrigger && m_pTrigger->GetOwner() ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1;
				int expsig = m_pTrigger->GetNumExpectedSignals(m_wId);
				lbl.appendf("\n%08x:%04x:%04x:%08x (%d)",tr.GetOwnGoid(),tr.GetTrigNum(),m_wId,sc,expsig);
				lbl.appendf("\n%s:0x%08x [%s]",ToString(m_WorldEventExpected),m_WorldEventQualifier,ToString(m_BoundaryTest));
				lbl.appendf("\n%d",m_CurrentSatisfyingMessages.size());
			}
			else
			{
				lbl.appendf("\n\n\n\n");
			}
			LabelPos = (*iCondition).second->GetLabelPos();
		}
		lbl.append(lblsuffix);
		if (LabelPos.node.IsValidSlow() && !gGoDb.IsEditMode())
		{
			gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), LabelPos.ScreenPos(), lbl );
		}
	}

}
#endif

///////////////////////////////////////////////////////////////////////////////////
//
// party_member_within_node( region, section, level, object );
//
party_member_within_node::party_member_within_node()
{
	m_Region	= -1;
	m_Section	= -1;
	m_Level		= -1;
	m_Object	= -1;
	m_BoundaryTest = BTEST_ON_EVERY_ENTER;
}

void party_member_within_node::SetTrigger(Trigger* trig) 
{
	m_pTrigger = trig;
	if (trig &&
		trig->GetOwner() &&
		trig->GetOwner()->HasPlacement())
	{
		m_Center = trig->GetOwner()->GetPlacement()->GetPosition();
	}
	else
	{
		m_Center = SiegePos::INVALID;
	}
}

bool party_member_within_node::IsComplicated() const
{
	return HasBoundaryOccupants() || HasSatisfyingOccupants();
};

void party_member_within_node::FetchParameterList(Parameter &parameterlist)
{
	// Define the format of the parameter list (for SiegeEditor)
	
	Parameter::format p0;
	p0.sName		= "Region ID";
	p0.isInt		= true;
	p0.isRequired	= true;
	p0.i_default	= m_Region;
	p0.bSaveAsHex	= true;
	parameterlist.Add( p0 );

	Parameter::format p1;
	p1.sName		= "Section";
	p1.isInt		= true;
	p1.isRequired	= true;
	p1.i_default	= m_Section;
	parameterlist.Add( p1 );

	Parameter::format p2;
	p2.sName		= "Level";
	p2.isInt		= true;
	p2.isRequired	= true;
	p2.i_default	= m_Level;
	parameterlist.Add( p2 );

	Parameter::format p3;
	p3.sName		= "Object";
	p3.isInt		= true;
	p3.isRequired	= true;
	p3.i_default	= m_Object;
	parameterlist.Add( p3 );

	Parameter::format px;
	FetchBoundaryTestFormat(px);
	parameterlist.Add( px );
}

bool party_member_within_node::StoreParameterValues(const Params &parms)
{
	gpassert(m_wId == parms.ParamID);
	m_sDoc = parms.Doc;
	m_iSubGroup = parms.SubGroup;
	// Convert the messages set by SiegeEditor to useable values
	if( parms.ints.size() == 4 )
	{
		m_Region	= parms.ints[0];
		m_Section	= parms.ints[1];
		m_Level		= parms.ints[2];
		m_Object	= parms.ints[3];

		if (!parms.strings.empty())
		{
			FromString(parms.strings[0].c_str(),m_BoundaryTest);
		}

		return true;
	}
	else
	{
		gpwarningf(("Invalid trigger parameter supplied for party_member_within_node"));
		return false;
	}
}

bool party_member_within_node::FetchParameterValues( Params &parms )
{
	parms.Clear();
	parms.Doc = m_sDoc;
	parms.SubGroup = m_iSubGroup;
	parms.ParamID = m_wId; 
	parms.ints.push_back(m_Region);
	parms.ints.push_back(m_Section);
	parms.ints.push_back(m_Level);
	parms.ints.push_back(m_Object);
	parms.strings.push_back(ToString(m_BoundaryTest));
	return true;
}

bool party_member_within_node::CheckForInsideBoundary( const Go* in_Mover, const SiegePos& in_Pos )
{
	if (IsDormant()) return false;

	if (!in_Mover->IsAnyHumanPartyMember())
	{
		return false;
	}

	siege::eLogicalNodeFlags walkPerms = in_Mover->GetBody()->GetTerrainMovementPermissions();

	SiegePos ip = in_Pos;

	bool hit = false;

	if ( (gSiegeEngine.IsPositionAllowed(ip,walkPerms,0.5f)) )
	{
	
		SiegeNode* pNode = gSiegeEngine.IsNodeValid( ip.node );

		if ( pNode)
		{
			hit = ( pNode->GetRegionID() == m_Region )
				&&  ((m_Section == -1) || (pNode->GetSection() == m_Section ))
				&&  ((m_Level   == -1) || (pNode->GetLevel()   == m_Level   ))
				&&  ((m_Object  == -1) || (pNode->GetObject()  == m_Object  ));
		}
	}

	return hit;
}

bool party_member_within_node::CollectInsideBoundary( GopColl& in_Occupants )
{
	return (0 != gAIQuery.GetHumanPartyMembersInNodes(m_Region,
													m_Section,
													m_Level,
													m_Object,
													in_Occupants));
}

bool party_member_within_node::CheckForBoundaryCollision( const Go*		in_Mover, 
								const SiegePos&	in_PosA, 
								const SiegePos&	in_PosB, 
								float&			out_tEnter, 
								float&			out_tLeave,
								bool&			out_AlreadyInside,
								bool&			out_OutsideAfterwards) 
{
	out_AlreadyInside = false;
	out_OutsideAfterwards = true;

	if (IsDormant())
	{
		out_tEnter = -2.0f;
		out_tLeave =  2.0f;
		return false;
	}

	if (!in_Mover->IsAnyHumanPartyMember())
	{
		return false;
	}

	bool hit = false;

	SiegeNode* pNodeA = gSiegeEngine.IsNodeValid( in_PosA.node ) ;
	SiegeNode* pNodeB = gSiegeEngine.IsNodeValid( in_PosB.node );

	if ( pNodeA && pNodeB )
	{
		out_AlreadyInside = (pNodeA->GetRegionID() == m_Region )
			&&  ((m_Section == -1) || (pNodeA->GetSection() == m_Section ))
			&&  ((m_Level   == -1) || (pNodeA->GetLevel()   == m_Level   ))
			&&  ((m_Object  == -1) || (pNodeA->GetObject()  == m_Object  ));

		out_OutsideAfterwards = (pNodeB->GetRegionID() != m_Region )
			||  !((m_Section == -1) || (pNodeB->GetSection() == m_Section ))
			||  !((m_Level   == -1) || (pNodeB->GetLevel()   == m_Level   ))
			||  !((m_Object  == -1) || (pNodeB->GetObject()  == m_Object  ));

		if (out_AlreadyInside == out_OutsideAfterwards)
		{
			// Approximate when/where we enter node 'B'

			const int splits = 8;
			float splits_inc =  1.0f/(float)splits;

			vector_3 delta = gSiegeEngine.GetDifferenceVector(in_PosA,in_PosB);
			vector_3 delta_inc = delta * splits_inc;

			SiegePos probe = in_PosA;
			probe.pos += delta_inc;

			float ip_ratio = splits_inc;	// How far along AB until we switch to node B?

			siege::eLogicalNodeFlags walkPerms = in_Mover->GetBody()->GetTerrainMovementPermissions();
			for (int i= 1; i<splits; ++i)
			{
				if (gSiegeEngine.IsPositionAllowed(probe,walkPerms,0.5f))
				{
					if (probe.node == in_PosB.node)
					{
						break;
					}
				}
				ip_ratio += splits_inc;
				probe.pos += delta_inc;
			}

			// We are interested in the last time that we were in Node A
			// The GoFollower will signal when the node changes from A to B
			ip_ratio -= splits_inc;

			if (out_OutsideAfterwards)
			{
				out_tEnter = -2.0;
				out_tLeave = ip_ratio;
			}
			else
			{
				out_tEnter = ip_ratio;
				out_tLeave = 2.0f;
			}

			hit = true;
		}
		else
		{
			out_tEnter = -2.0f;
			out_tLeave =  2.0f;
		}

	}

	return hit;
}

bool party_member_within_node::Xfer( FuBi::PersistContext& persist )
{
	Condition::Xfer(persist);
	persist.XferHex( "m_Region",  m_Region  );
	persist.Xfer(    "m_Section", m_Section );
	persist.Xfer(    "m_Level",   m_Level   );
	persist.Xfer(    "m_Object",  m_Object  );
	return true;
}

#if !GP_RETAIL
void party_member_within_node::Draw( const ConditionDb& cond )
{

	if ( !gWorldOptions.TestDebugHudOptions( DHO_LABELS ))
	{
		return;
	}

	GoOccupantMapIter it;
	int bcount = 0;
	for (it = m_CurrentInsideBoundary.begin(); it != m_CurrentInsideBoundary.end(); ++it)
	{
		bcount += (*it).second;	
	}

	int scount = 0;
	for (it = m_CurrentSatisfyingOccupants.begin(); it != m_CurrentSatisfyingOccupants.end(); ++it)
	{
		scount += (*it).second;	
	}

	if (m_pTrigger &&
		m_pTrigger->GetOwner() &&
		m_pTrigger->GetOwner()->HasPlacement())
	{
		SiegePos LabelPos;
		gpstring lbl,lblsuffix;
		if (m_pTrigger->GetOwner() && m_pTrigger->GetOwner()->HasCommon())
		{
			m_pTrigger->GetOwner()->GetCommon()->PadDebugLabelHelper(m_pTrigger,"\n\n\n\n",lbl,lblsuffix);
		}
		Truid tr = m_pTrigger->GetTruid();
		ConditionConstIter iCondition = cond.begin();
		for(; iCondition != cond.end(); ++iCondition )
		{
			if (m_wId == (*iCondition).second->GetID())
			{
				double et = (m_pTrigger->GetResetTimeout() <= 0.0f) ? 0 : PreciseSubtract(m_pTrigger->GetResetTimeout(), gWorldTime.GetTime());
				lbl.appendf("%s:party_member_within_node:%5.2f",m_pTrigger->GetCurrentActive() ? (IsDormant()?"X":"A"):"D",et);
				int sc = m_pTrigger && m_pTrigger->GetOwner() ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1;
				int expsig = m_pTrigger->GetNumExpectedSignals(m_wId);
				lbl.appendf("\n%08x:%04x:%04x:%08x (%d)",tr.GetOwnGoid(),tr.GetTrigNum(),m_wId,sc,expsig);
				lbl.appendf("\n%s r:0x%08x s:%d l:%d o:%d",ToString(m_BoundaryTest),m_Region,m_Section,m_Level,m_Object);
				lbl.appendf("\n%d(%d):%d(%d)",bcount,m_CurrentInsideBoundary.size(),scount, m_CurrentSatisfyingOccupants.size());
			}
			else
			{
				lbl.appendf("\n\n\n\n");
			}
			LabelPos = (*iCondition).second->GetLabelPos();
		}
		lbl.append(lblsuffix);
		if (LabelPos.node.IsValidSlow() && !gGoDb.IsEditMode())
		{
			gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), LabelPos.ScreenPos(), lbl );
		}
	}
	
}
#endif

///////////////////////////////////////////////////////////////////////////////////
//
// party_member_within_sphere( radius );
//

party_member_within_sphere::party_member_within_sphere()
{
	m_Radius = 2.5f;
	m_Center = SiegePos::INVALID;
	m_BoundaryTest = BTEST_ON_EVERY_ENTER;
}

void party_member_within_sphere::SetTrigger(Trigger* trig) 
{
	m_pTrigger = trig;
	if (trig &&
		trig->GetOwner() &&
		trig->GetOwner()->HasPlacement())
	{
		m_Center = trig->GetOwner()->GetPlacement()->GetPosition();
	}
	else
	{
		m_Center = SiegePos::INVALID;
	}
}

bool party_member_within_sphere::IsComplicated() const
{
	return HasBoundaryOccupants() || HasSatisfyingOccupants();
};

void party_member_within_sphere::FetchParameterList(Parameter &parameterlist)
{
	// Define the format of the parameter list (for SiegeEditor)

	// Parameter 0
	Parameter::format p0;
	p0.sName		= "Radius";
	p0.isFloat		= true;
	p0.isRequired	= true;
	p0.f_min		= 0.0f;
	p0.f_max		= 1000.0f;
	p0.f_default	= m_Radius;

	parameterlist.Add( p0 );

	Parameter::format px;
	FetchBoundaryTestFormat(px);
	parameterlist.Add( px );

}

bool party_member_within_sphere::StoreParameterValues(const Params &parms)
{
	gpassert(m_wId == parms.ParamID);
	m_sDoc = parms.Doc;
	m_iSubGroup = parms.SubGroup;
	// Convert the messages set by SiegeEditor to useable values
	if( !parms.floats.empty() )
	{
		m_Radius = parms.floats[0];
		if (!parms.strings.empty())
		{
			FromString(parms.strings[0].c_str(),m_BoundaryTest);
		}
		return true;
	}
	else
	{
		gpwarningf(("Invalid trigger parameter supplied for party_member_within_sphere"));
		return false;
	}
}

bool party_member_within_sphere::FetchParameterValues( Params &parms )
{
	parms.Clear();
	parms.Doc = m_sDoc;
	parms.SubGroup = m_iSubGroup;
	parms.ParamID = m_wId; 
	parms.floats.push_back(m_Radius);
	parms.strings.push_back(ToString(m_BoundaryTest));
	return true;
}

bool party_member_within_sphere::CheckForInsideBoundary( const Go* in_Mover, const SiegePos& in_Pos )
{
	PARANOID_IN_WORLD_TEST

 	if (IsDormant()) return false;

	bool hit = false;
	if (in_Mover->IsAnyHumanPartyMember())
	{
		hit = CheckInsideSphere( in_Mover,
								 m_Center,
								 m_Radius,
								 OF_CAN_MOVE_SELF,
								 in_Pos);
	}
	return hit;
}

bool party_member_within_sphere::CollectInsideBoundary( GopColl& in_Occupants )
{
	PARANOID_IN_WORLD_TEST

	if (IsDormant()) return false;

	GopColl possibles;
	in_Occupants.clear();

	if ( gAIQuery.GetOccupantsInSphere(
					m_Center,
					m_Radius+5.0f,
					NULL,
					NULL,
					0,
					OF_CAN_MOVE_SELF|PARTY_MEMBER_FILTER,
					&possibles,
					false))
	{
		Go* pown = m_pTrigger->GetOwner();
		for (GopColl::iterator git=possibles.begin(); git!=possibles.end(); ++git)
		{
			if ((*git) == pown)
			{
				continue;
			}

			SiegePos pos;
			bool ok=false;
			if ((*git)->HasFollower())
			{	
				MCP::Plan* jplan = gMCP.FindPlan((*git)->GetGoid(),false);
				if (jplan)
				{
					// get the earliest clip time to test if we are inside the boudary
					// this will give us the most accurate view of the trigger's occupants
					double clipTime = gWorldTime.GetTime();
					jplan->GetEarliestClipOrTransmittedTime( clipTime );
					
					pos = jplan->GetPosition(clipTime,true,true);
					ok = true;
				}
			}
			if (!ok)
			{
				pos = (*git)->GetPlacement()->GetPosition();
			}

			if (CheckForInsideBoundary(*git,pos))
			{
				in_Occupants.push_back(*git);
			}
		}
	}
	return (!in_Occupants.empty());
}

bool party_member_within_sphere::CheckForBoundaryCollision( const Go*		in_Mover, 
								const SiegePos&	in_PosA, 
								const SiegePos&	in_PosB, 
								float&			out_tEnter, 
								float&			out_tLeave,
								bool&			out_AlreadyInside,
								bool&			out_OutsideAfterwards) 
{
	PARANOID_IN_WORLD_TEST

	if (IsDormant())
	{
		out_tEnter = -2.0f;
		out_tLeave =  2.0f;
		out_AlreadyInside = false;
		out_OutsideAfterwards = true;
		return false;
	}

	// This method seems to be mis-named, the original REALLY only wanted human controlled actors
	bool hit = false;
	out_AlreadyInside = false;
	out_OutsideAfterwards = true;
	if (in_Mover->IsAnyHumanPartyMember())
	{
		hit = CheckSphereCollision(in_Mover,
									m_Center,
									m_Radius,
									OF_NONE,
									in_PosA,
									in_PosB,
									out_tEnter,
									out_tLeave,
									out_AlreadyInside,
									out_OutsideAfterwards);
	}

	return hit;
}

bool party_member_within_sphere::Xfer( FuBi::PersistContext& persist )
{
	Condition::Xfer(persist);
	persist.Xfer( "m_Radius", m_Radius );
	return true;
}

#if !GP_RETAIL
SiegePos party_member_within_sphere::GetLabelPos()
{
	SiegePos p = m_Center;
	p.pos.y +=  min_t(5.0f,m_Radius*1.25f);
	return p;
}

void party_member_within_sphere::Draw( const ConditionDb& cond )
{
	GoOccupantMapIter it;
	int bcount = 0;
	for (it = m_CurrentInsideBoundary.begin(); it != m_CurrentInsideBoundary.end(); ++it)
	{
		bcount += (*it).second;	
	}

	int scount = 0;
	for (it = m_CurrentSatisfyingOccupants.begin(); it != m_CurrentSatisfyingOccupants.end(); ++it)
	{
		scount += (*it).second;	
	}

	SiegePos LabelPos;
	gpstring lbl,lblsuffix;
	if (m_pTrigger->GetOwner() && m_pTrigger->GetOwner()->HasCommon())
	{
		m_pTrigger->GetOwner()->GetCommon()->PadDebugLabelHelper(m_pTrigger,"\n\n\n\n",lbl,lblsuffix);
	}
	Truid tr = m_pTrigger->GetTruid();
	ConditionConstIter iCondition = cond.begin();
	for(; iCondition != cond.end(); ++iCondition )
	{
		if (m_wId == (*iCondition).second->GetID())
		{
			double et = (m_pTrigger->GetResetTimeout() <= 0.0f) ? 0 : PreciseSubtract(m_pTrigger->GetResetTimeout(), gWorldTime.GetTime());
			lbl.appendf("%s:party_member_within_sphere:%5.2f",m_pTrigger->GetCurrentActive() ? (IsDormant()?"X":"A"):"D", et );
			int sc = (m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1;
			int expsig = m_pTrigger->GetNumExpectedSignals(m_wId);
			lbl.appendf("\n%08x:%04x:%04x:%08x (%d)",tr.GetOwnGoid(),tr.GetTrigNum(),m_wId,sc,expsig);
			lbl.appendf("\n%s",ToString(m_BoundaryTest));
			lbl.appendf("\n%d(%d):%d(%d)",bcount,m_CurrentInsideBoundary.size(),scount, m_CurrentSatisfyingOccupants.size());
		}
		else
		{
			lbl.appendf("\n\n\n\n");
		}
		LabelPos = (*iCondition).second->GetLabelPos();
	}
	lbl.append(lblsuffix);
	gWorld.DrawDebugSphere( m_Center, m_Radius, COLOR_CHERRY, "" );
	if (LabelPos.node.IsValidSlow() && !gGoDb.IsEditMode())
	{
		gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), LabelPos.ScreenPos(), lbl );
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////////
//
// party_member_within_bounding_box
//

party_member_within_bounding_box::party_member_within_bounding_box() 
{
	m_HalfDiag = vector_3(1.0f,0.5f,1.0f);
	m_Center = SiegePos::INVALID;
	m_InvOrient = matrix_3x3::IDENTITY;
	m_BoundaryTest = BTEST_ON_EVERY_ENTER;
}

void party_member_within_bounding_box::SetTrigger(Trigger* trig) 
{
	m_pTrigger = trig;
	if (trig &&
		trig->GetOwner() &&
		trig->GetOwner()->HasPlacement())
	{
		m_Center = trig->GetOwner()->GetPlacement()->GetPosition();
		m_InvOrient = Transpose(trig->GetOwner()->GetPlacement()->GetOrientation().BuildMatrix());
	}
	else
	{
		m_Center = SiegePos::INVALID;
	}
}

bool party_member_within_bounding_box::IsComplicated() const
{
	return HasBoundaryOccupants() || HasSatisfyingOccupants();
};

void party_member_within_bounding_box::FetchParameterList(Parameter &parameterlist)
{
	// Define the format of the parameter list (for SiegeEditor)
	Parameter::format p0;
	p0.sName		= "Half diag X";
	p0.isFloat		= true;
	p0.isRequired	= false;
	p0.f_min		= 0.0f;
	p0.f_max		= 1000.0f;
	p0.f_default	= m_HalfDiag.x;

	parameterlist.Add( p0 );

	Parameter::format p1;
	p1.sName		= "Half diag Y";
	p1.isFloat		= true;
	p1.isRequired	= false;
	p1.f_min		= 0.0f;
	p1.f_max		= 1000.0f;
	p1.f_default	= m_HalfDiag.y;

	parameterlist.Add( p1 );

	Parameter::format p2;
	p2.sName		= "Half diag Z";
	p2.isFloat		= true;
	p2.isRequired	= false;
	p2.f_min		= 0.0f;
	p2.f_max		= 1000.0f;
	p2.f_default	= m_HalfDiag.z;

	parameterlist.Add( p2 );

	Parameter::format px;
	FetchBoundaryTestFormat(px);
	parameterlist.Add( px );

}

bool party_member_within_bounding_box::StoreParameterValues(const Params &parms)
{
	gpassert(m_wId == parms.ParamID);
	m_sDoc = parms.Doc;
	m_iSubGroup = parms.SubGroup;
	// Convert the messages set by SiegeEditor to useable values
	if( parms.floats.size() == 3 )
	{
		m_HalfDiag = vector_3(parms.floats[0],parms.floats[1],parms.floats[2]);
		if (!parms.strings.empty())
		{
			FromString(parms.strings[0].c_str(),m_BoundaryTest);
		}
		return true;
	}
	else
	{
		gpwarningf(( "Invalid trigger parameter supplied for party_member_within_bounding_box" ));
		return false;
	}
}

bool party_member_within_bounding_box::FetchParameterValues( Params &parms )
{
	parms.Clear();
	parms.Doc = m_sDoc;
	parms.SubGroup = m_iSubGroup;
	parms.ParamID = m_wId; 
	parms.floats.push_back(m_HalfDiag.x);
	parms.floats.push_back(m_HalfDiag.y);
	parms.floats.push_back(m_HalfDiag.z);
	parms.strings.push_back(ToString(m_BoundaryTest));
	return true;
}

bool party_member_within_bounding_box::CheckForInsideBoundary( const Go* in_Mover, const SiegePos& in_Pos )
{
	PARANOID_IN_WORLD_TEST

	if (IsDormant()) return false;

	bool hit = false;
	if (in_Mover->IsAnyHumanPartyMember())
	{
		hit = CheckInsideBox( in_Mover,
						   m_Center, m_InvOrient, m_HalfDiag,
						   OF_CAN_MOVE_SELF, 
						   in_Pos );
	}
	return hit;
}

bool party_member_within_bounding_box::CollectInsideBoundary( GopColl& in_Occupants )
{
	PARANOID_IN_WORLD_TEST

	if (IsDormant()) return false;

	GopColl possibles;
	in_Occupants.clear();

	if ( gAIQuery.GetOccupantsInSphere(
					m_Center,
					m_HalfDiag.Length()+5.0f,	// Overestimate initial query
					NULL,
					NULL,
					0,
					OF_CAN_MOVE_SELF|PARTY_MEMBER_FILTER,
					&possibles,
					false ) )
	{
		// cull the possibles that passed the sphere test
		Go* pown = m_pTrigger->GetOwner();
		for (GopColl::iterator git=possibles.begin(); git!=possibles.end(); ++git)
		{
			if ((*git) == pown)
			{
				continue;
			}
			gpassert((*git)->HasPlacement());

			SiegePos pos;
			bool ok=false;
			if ((*git)->HasFollower())
			{	
				MCP::Plan* jplan = gMCP.FindPlan((*git)->GetGoid(),false);
				if (jplan)
				{
					// get the earliest clip time to test if we are inside the boudary
					// this will give us the most accurate view of the trigger's occupants 
					// send in the current time to attempt to clip too
					double clipTime = gWorldTime.GetTime();
					jplan->GetEarliestClipOrTransmittedTime( clipTime );
					
					pos = jplan->GetPosition(clipTime,true,true);					
					ok = true;
				}
			}
			if (!ok)
			{
				pos = (*git)->GetPlacement()->GetPosition();
			}

			if (CheckForInsideBoundary(*git,pos))
			{
				in_Occupants.push_back(*git);
			}
		}
	}

	return !in_Occupants.empty();
}

bool party_member_within_bounding_box::CheckForBoundaryCollision( const Go*		in_Mover, 
								const SiegePos&	in_PosA, 
								const SiegePos&	in_PosB, 
								float&			out_tEnter, 
								float&			out_tLeave,
								bool&			out_AlreadyInside,
								bool&			out_OutsideAfterwards ) 
{
	PARANOID_IN_WORLD_TEST

	if (IsDormant())
	{
		out_tEnter = -2.0f;
		out_tLeave =  2.0f;
		out_AlreadyInside = false;
		out_OutsideAfterwards = true;
		return false;
	}

	// This method seems to be mis-named, the original REALLY only wanted human controlled actors
	bool hit = false;
	out_AlreadyInside = false;
	out_OutsideAfterwards = true;
	if (in_Mover->IsAnyHumanPartyMember())
	{
		hit = CheckBoxCollision(in_Mover,
								 m_Center, m_InvOrient, m_HalfDiag,
								 OF_NONE, 
								 in_PosA,
								 in_PosB,
								 out_tEnter,
								 out_tLeave,
								 out_AlreadyInside,
								 out_OutsideAfterwards
								 );
	}

	return hit;
}

bool party_member_within_bounding_box::Xfer( FuBi::PersistContext& persist )
{
	Condition::Xfer(persist);
	persist.Xfer( "m_HalfDiag", m_HalfDiag );
	return true;
}

#if !GP_RETAIL
SiegePos party_member_within_bounding_box::GetLabelPos()
{
	SiegePos p = m_Center;
	p.pos.y +=  min_t(5.0f,m_HalfDiag.y*1.25f);
	return p;
}
void party_member_within_bounding_box::Draw( const ConditionDb& cond )
{
	GoOccupantMapIter it;
	int bcount = 0;
	for (it = m_CurrentInsideBoundary.begin(); it != m_CurrentInsideBoundary.end(); ++it)
	{
		bcount += (*it).second;	
	}

	int scount = 0;
	for (it = m_CurrentSatisfyingOccupants.begin(); it != m_CurrentSatisfyingOccupants.end(); ++it)
	{
		scount += (*it).second;	
	}

	SiegePos LabelPos;
	gpstring lbl,lblsuffix;
	if (m_pTrigger->GetOwner() && m_pTrigger->GetOwner()->HasCommon())
	{
		m_pTrigger->GetOwner()->GetCommon()->PadDebugLabelHelper(m_pTrigger,"\n\n\n\n",lbl,lblsuffix);
	}
	Truid tr = m_pTrigger->GetTruid();
	ConditionConstIter iCondition = cond.begin();
	for(; iCondition != cond.end(); ++iCondition )
	{
		if (m_wId == (*iCondition).second->GetID())
		{
			double et = (m_pTrigger->GetResetTimeout() <= 0.0f) ? 0 : PreciseSubtract(m_pTrigger->GetResetTimeout(), gWorldTime.GetTime());
			lbl.appendf("%s:party_member_within_bounding_box:%5.2f",m_pTrigger->GetCurrentActive() ? (IsDormant()?"X":"A"):"D", et );
			int sc = (m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1;
			int expsig = m_pTrigger->GetNumExpectedSignals(m_wId);
			lbl.appendf("\n%08x:%04x:%04x:%08x (%d)",tr.GetOwnGoid(),tr.GetTrigNum(),m_wId,sc,expsig);
			lbl.appendf("\n%s",ToString(m_BoundaryTest));
			lbl.appendf("\n%d(%d):%d(%d)",bcount,m_CurrentInsideBoundary.size(),scount, m_CurrentSatisfyingOccupants.size());
		}
		else
		{
			lbl.appendf("\n\n\n\n");
		}
		LabelPos = (*iCondition).second->GetLabelPos();
	}
	lbl.append(lblsuffix);
	gWorld.DrawDebugBox( m_Center, Transpose(m_InvOrient), m_HalfDiag, 0xFF9999E0, 1.0f/60.0f, false, false, "" );
	if (LabelPos.node.IsValidSlow() && !gGoDb.IsEditMode())
	{
		gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), LabelPos.ScreenPos(), lbl );
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////////
//
// actor_within_sphere
//
actor_within_sphere::actor_within_sphere()
{
	m_Radius = 2.5f;
	m_Center = SiegePos::INVALID;
	m_BoundaryTest = BTEST_ON_EVERY_ENTER;
}

void actor_within_sphere::SetTrigger(Trigger* trig) 
{
	m_pTrigger = trig;
	if (trig &&
		trig->GetOwner() &&
		trig->GetOwner()->HasPlacement())
	{
		m_Center = trig->GetOwner()->GetPlacement()->GetPosition();
	}
	else
	{
		m_Center = SiegePos::INVALID;
	}
}

bool actor_within_sphere::IsComplicated() const
{
	return HasBoundaryOccupants() || HasSatisfyingOccupants();
};

void actor_within_sphere::FetchParameterList(Parameter &parameterlist)
{
	// Define the format of the parameter list (for SiegeEditor)
	Parameter::format p0;
	p0.sName		= "Radius";
	p0.isFloat		= true;
	p0.isRequired	= true;
	p0.f_min		= 0.0f;
	p0.f_max		= 1000.0f;
	p0.f_default	= m_Radius;
	parameterlist.Add( p0 );

	Parameter::format px;
	FetchBoundaryTestFormat(px);
	parameterlist.Add( px );

}

bool actor_within_sphere::StoreParameterValues(const Params &parms)
{
	gpassert(m_wId == parms.ParamID);
	m_sDoc = parms.Doc;
	m_iSubGroup = parms.SubGroup;
	// Convert the messages set by SiegeEditor to useable values
	if( parms.floats.size() == 1 )
	{
		m_Radius = parms.floats[0];
		if (!parms.strings.empty())
		{
			FromString(parms.strings[0].c_str(),m_BoundaryTest);
		}
		return true;
	}
	else
	{
		gpwarningf(( "Invalid trigger parameter supplied for actor_within_sphere" ));
		return false;
	}
}

bool actor_within_sphere::FetchParameterValues( Params &parms )
{
	parms.Clear();
	parms.Doc = m_sDoc;
	parms.SubGroup = m_iSubGroup;
	parms.ParamID = m_wId; 
	parms.floats.push_back(m_Radius);
	parms.strings.push_back(ToString(m_BoundaryTest));
	return true;
}

bool actor_within_sphere::CheckForInsideBoundary( const Go* in_Mover, const SiegePos& in_Pos )
{
	PARANOID_IN_WORLD_TEST

	if (IsDormant()) return false;

	return CheckInsideSphere(in_Mover,
							 m_Center,
							 m_Radius,
							 OF_ACTORS | OF_ALIVE_OR_GHOST,
							 in_Pos);
}

bool actor_within_sphere::CollectInsideBoundary( GopColl& in_Occupants )
{
	PARANOID_IN_WORLD_TEST

	if (IsDormant()) return false;

	GopColl possibles;
	in_Occupants.clear();

	if ( gAIQuery.GetOccupantsInSphere(
					m_Center,
					m_Radius+5.0f,	// Overestimate initial query
					NULL,
					NULL,
					0,
					OF_CAN_MOVE_SELF|OF_ACTORS|OF_ALIVE_OR_GHOST,
					&possibles,
					false ) )
	{
		Go* pown = m_pTrigger->GetOwner();
		for (GopColl::iterator git=possibles.begin(); git!=possibles.end(); ++git)
		{
			if ((*git) == pown)
			{
				continue;
			}
			// cull the possibles that passed the sphere test
			SiegePos pos;
			bool ok=false;
			if ((*git)->HasFollower())
			{	
				MCP::Plan* jplan = gMCP.FindPlan((*git)->GetGoid(),false);
				if (jplan)
				{
					// get the earliest clip time to test if we are inside the boudary
					// this will give us the most accurate view of the trigger's occupants 
					double clipTime = gWorldTime.GetTime();
					jplan->GetEarliestClipOrTransmittedTime( clipTime );
					
					pos = jplan->GetPosition(clipTime,true,true);
					ok = true;
				}
			}
			if (!ok)
			{
				pos = (*git)->GetPlacement()->GetPosition();
			}

			if (CheckForInsideBoundary(*git,pos))
			{
				in_Occupants.push_back(*git);
			}
		}
	}

	return (!in_Occupants.empty());
}

bool actor_within_sphere::CheckForBoundaryCollision(
								const Go*		in_Mover, 
								const SiegePos&	in_PosA, 
								const SiegePos&	in_PosB, 
								float&			out_tEnter, 
								float&			out_tLeave,
								bool&			out_AlreadyInside,
								bool&			out_OutsideAfterwards) 
{
	PARANOID_IN_WORLD_TEST

	if (IsDormant())
	{
		out_tEnter = -2.0f;
		out_tLeave =  2.0f;
		out_AlreadyInside = false;
		out_OutsideAfterwards = true;
		return false;
	}

	bool hit = CheckSphereCollision(in_Mover,
									m_Center,
									m_Radius,
									OF_ACTORS | OF_ALIVE_OR_GHOST,
									in_PosA,
									in_PosB,
									out_tEnter,
									out_tLeave,
									out_AlreadyInside,
									out_OutsideAfterwards);
	return hit;
}

bool actor_within_sphere::Xfer( FuBi::PersistContext& persist )
{
	Condition::Xfer(persist);
	persist.Xfer( "m_Radius", m_Radius );
	return true;
}

#if !GP_RETAIL
SiegePos actor_within_sphere::GetLabelPos()
{
	SiegePos p = m_Center;
	p.pos.y +=  min_t(5.0f,m_Radius*1.25f);
	return p;
}
void actor_within_sphere::Draw( const ConditionDb& cond )
{
	GoOccupantMapIter it;
	int bcount = 0;
	for (it = m_CurrentInsideBoundary.begin(); it != m_CurrentInsideBoundary.end(); ++it)
	{
		bcount += (*it).second;	
	}

	int scount = 0;
	for (it = m_CurrentSatisfyingOccupants.begin(); it != m_CurrentSatisfyingOccupants.end(); ++it)
	{
		scount += (*it).second;	
	}

	SiegePos LabelPos;
	gpstring lbl,lblsuffix;
	if (m_pTrigger->GetOwner() && m_pTrigger->GetOwner()->HasCommon())
	{
		m_pTrigger->GetOwner()->GetCommon()->PadDebugLabelHelper(m_pTrigger,"\n\n\n\n",lbl,lblsuffix);
	}
	Truid tr = m_pTrigger->GetTruid();
	ConditionConstIter iCondition = cond.begin();
	for(; iCondition != cond.end(); ++iCondition )
	{
		if (m_wId == (*iCondition).second->GetID())
		{
			double et = (m_pTrigger->GetResetTimeout() <= 0.0f) ? 0 : PreciseSubtract(m_pTrigger->GetResetTimeout(), gWorldTime.GetTime());
			lbl.appendf("%s:actor_within_sphere:%5.2f",m_pTrigger->GetCurrentActive() ? (IsDormant()?"X":"A"):"D", et );
			int sc = (m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1;
			int expsig = m_pTrigger->GetNumExpectedSignals(m_wId);
			lbl.appendf("\n%08x:%04x:%04x:%08x (%d)",tr.GetOwnGoid(),tr.GetTrigNum(),m_wId,sc,expsig);
			lbl.appendf("\n%s",ToString(m_BoundaryTest));
			lbl.appendf("\n%d(%d):%d(%d)",bcount,m_CurrentInsideBoundary.size(),scount, m_CurrentSatisfyingOccupants.size());
		}
		else
		{
			lbl.appendf("\n\n\n\n");
		}
		LabelPos = (*iCondition).second->GetLabelPos();
	}
	lbl.append(lblsuffix);
	gWorld.DrawDebugSphere( m_Center, m_Radius, COLOR_CHERRY, "" );
	if (LabelPos.node.IsValidSlow() && !gGoDb.IsEditMode())
	{
		gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), LabelPos.ScreenPos(), lbl );
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////////
//
// actor_within_bounding_box
//

actor_within_bounding_box::actor_within_bounding_box() 
{
	m_HalfDiag = vector_3(1.0f,0.5f,1.0f);
	m_Center = SiegePos::INVALID;
	m_InvOrient = matrix_3x3::IDENTITY;
	m_BoundaryTest = BTEST_ON_EVERY_ENTER;
}

void actor_within_bounding_box::SetTrigger(Trigger* trig) 
{
	m_pTrigger = trig;
	if (trig &&
		trig->GetOwner() &&
		trig->GetOwner()->HasPlacement())
	{
		m_Center = trig->GetOwner()->GetPlacement()->GetPosition();
		m_InvOrient = Transpose(trig->GetOwner()->GetPlacement()->GetOrientation().BuildMatrix());
	}
	else
	{
		m_Center = SiegePos::INVALID;
	}
}

bool actor_within_bounding_box::IsComplicated() const
{
	return HasBoundaryOccupants() || HasSatisfyingOccupants();
};

void actor_within_bounding_box::FetchParameterList(Parameter &parameterlist)
{
	// Define the format of the parameter list (for SiegeEditor)
	Parameter::format p0;
	p0.sName		= "Half diag X";
	p0.isFloat		= true;
	p0.isRequired	= false;
	p0.f_min		= 0.0f;
	p0.f_max		= 1000.0f;
	p0.f_default	= 1.0f;
	parameterlist.Add( p0 );

	Parameter::format p1;
	p1.sName		= "Half diag Y";
	p1.isFloat		= true;
	p1.isRequired	= false;
	p1.f_min		= 0.0f;
	p1.f_max		= 1000.0f;
	p1.f_default	= 0.5f;
	parameterlist.Add( p1 );

	Parameter::format p2;
	p2.sName		= "Half diag Z";
	p2.isFloat		= true;
	p2.isRequired	= false;
	p2.f_min		= 0.0f;
	p2.f_max		= 1000.0f;
	p2.f_default	= 1.0f;
	parameterlist.Add( p2 );

	Parameter::format px;
	FetchBoundaryTestFormat(px);
	parameterlist.Add( px );

}

bool actor_within_bounding_box::StoreParameterValues(const Params &parms)
{
	gpassert(m_wId == parms.ParamID);
	m_sDoc = parms.Doc;
	m_iSubGroup = parms.SubGroup;
	// Convert the messages set by SiegeEditor to useable values
	if( parms.floats.size() == 3 )
	{
		m_HalfDiag = vector_3(parms.floats[0],parms.floats[1],parms.floats[2]);
		if (!parms.strings.empty())
		{
			FromString(parms.strings[0].c_str(),m_BoundaryTest);
		}
		return true;
	}
	else
	{
		gpwarningf(( "Invalid trigger parameter supplied for actor_within_bounding_box" ));
		return false;
	}
}

bool actor_within_bounding_box::FetchParameterValues( Params &parms )
{
	parms.Clear();
	parms.Doc = m_sDoc;
	parms.SubGroup = m_iSubGroup;
	parms.ParamID = m_wId; 
	parms.floats.push_back(m_HalfDiag.x);
	parms.floats.push_back(m_HalfDiag.y);
	parms.floats.push_back(m_HalfDiag.z);
	parms.strings.push_back(ToString(m_BoundaryTest));
	return true;
}

bool actor_within_bounding_box::CheckForInsideBoundary( const Go* in_Mover, const SiegePos& in_Pos )
{
	PARANOID_IN_WORLD_TEST

	if (IsDormant()) return false;

	return CheckInsideBox( in_Mover,
						   m_Center, m_InvOrient, m_HalfDiag,
						   OF_ACTORS | OF_ALIVE_OR_GHOST, 
						   in_Pos );
}

bool actor_within_bounding_box::CollectInsideBoundary( GopColl& in_Occupants )
{
	PARANOID_IN_WORLD_TEST

	if (IsDormant()) return false;

	GopColl possibles;
	in_Occupants.clear();

	if ( gAIQuery.GetOccupantsInSphere(
					m_Center,
					m_HalfDiag.Length()+5.0f,	// Overestimate initial query
					NULL,
					NULL,
					0,
					OF_CAN_MOVE_SELF|OF_ACTORS|OF_ALIVE_OR_GHOST,
					&possibles,
					false ) )
	{
		Go* pown = m_pTrigger->GetOwner();
		for (GopColl::iterator git=possibles.begin(); git!=possibles.end(); ++git)
		{
			if ((*git) == pown)
			{
				continue;
			}
			// cull the possibles that passed the sphere test
			SiegePos pos;
			bool ok=false;
			if ((*git)->HasFollower())
			{	
				MCP::Plan* jplan = gMCP.FindPlan((*git)->GetGoid(),false);
				if (jplan)
				{
					// get the earliest clip time to test if we are inside the boudary
					// this will give us the most accurate view of the trigger's occupants 
					double clipTime = gWorldTime.GetTime();
					jplan->GetEarliestClipOrTransmittedTime( clipTime );
					
					pos = jplan->GetPosition(clipTime,true,true);
					ok = true;
				}
			}
			if (!ok)
			{
				pos = (*git)->GetPlacement()->GetPosition();
			}

			if (CheckForInsideBoundary(*git,pos))
			{
				in_Occupants.push_back(*git);
			}
		}
	}

	return !in_Occupants.empty();
}

bool actor_within_bounding_box::CheckForBoundaryCollision(
								const Go*		in_Mover, 
								const SiegePos&	in_PosA, 
								const SiegePos&	in_PosB, 
								float&			out_tEnter, 
								float&			out_tLeave,
								bool&			out_AlreadyInside,
								bool&			out_OutsideAfterwards ) 
{
	PARANOID_IN_WORLD_TEST

	if (IsDormant())
	{
		out_tEnter = -2.0f;
		out_tLeave =  2.0f;
		out_AlreadyInside = false;
		out_OutsideAfterwards = true;
		false;
	}

	bool hit = CheckBoxCollision(in_Mover,
								 m_Center, m_InvOrient, m_HalfDiag,
								 OF_ACTORS | OF_ALIVE_OR_GHOST,
								 in_PosA,
								 in_PosB,
								 out_tEnter,
								 out_tLeave,
								 out_AlreadyInside,
								 out_OutsideAfterwards);

	return hit;
}

bool actor_within_bounding_box::Xfer( FuBi::PersistContext& persist )
{
	Condition::Xfer(persist);
	persist.Xfer( "m_HalfDiag", m_HalfDiag );
	return true;
}

#if !GP_RETAIL
SiegePos actor_within_bounding_box::GetLabelPos()
{
	SiegePos p = m_Center;
	p.pos.y +=  min_t(5.0f,m_HalfDiag.y*1.25f);
	return p;
}
void actor_within_bounding_box::Draw( const ConditionDb& cond )
{
	GoOccupantMapIter it;
	int bcount = 0;
	for (it = m_CurrentInsideBoundary.begin(); it != m_CurrentInsideBoundary.end(); ++it)
	{
		bcount += (*it).second;	
	}

	int scount = 0;
	for (it = m_CurrentSatisfyingOccupants.begin(); it != m_CurrentSatisfyingOccupants.end(); ++it)
	{
		scount += (*it).second;	
	}

	SiegePos LabelPos;
	gpstring lbl,lblsuffix;
	if (m_pTrigger->GetOwner() && m_pTrigger->GetOwner()->HasCommon())
	{
		m_pTrigger->GetOwner()->GetCommon()->PadDebugLabelHelper(m_pTrigger,"\n\n\n\n",lbl,lblsuffix);
	}
	Truid tr = m_pTrigger->GetTruid();
	ConditionConstIter iCondition = cond.begin();
	for(; iCondition != cond.end(); ++iCondition )
	{
		if (m_wId == (*iCondition).second->GetID())
		{
			double et = (m_pTrigger->GetResetTimeout() <= 0.0f) ? 0 : PreciseSubtract(m_pTrigger->GetResetTimeout(), gWorldTime.GetTime());
			lbl.appendf("%s:actor_within_bounding_box:%5.2f",m_pTrigger->GetCurrentActive() ? (IsDormant()?"X":"A"):"D", et );
			int sc = (m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1;
			int expsig = m_pTrigger->GetNumExpectedSignals(m_wId);
			lbl.appendf("\n%08x:%04x:%04x:%08x (%d)",tr.GetOwnGoid(),tr.GetTrigNum(),m_wId,sc,expsig);
			lbl.appendf("\n%s",ToString(m_BoundaryTest));
			lbl.appendf("\n%d(%d):%d(%d)",bcount,m_CurrentInsideBoundary.size(),scount, m_CurrentSatisfyingOccupants.size());
		}
		else
		{
			lbl.appendf("\n\n\n\n");
		}
		LabelPos = (*iCondition).second->GetLabelPos();
	}
	lbl.append(lblsuffix);
	gWorld.DrawDebugBox( m_Center, Transpose(m_InvOrient), m_HalfDiag, 0xFF9999E0, 1.0f/60.0f, false, false, "" );
	if (LabelPos.node.IsValidSlow() && !gGoDb.IsEditMode())
	{
		gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), LabelPos.ScreenPos(), lbl );
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////////
//
// go_within_sphere
//

go_within_sphere::go_within_sphere()
{
	m_Scid = MakeScid(0);
	m_TemplateNameFilter = "";
	m_Radius = 2.5f;
	m_Center = SiegePos::INVALID;
	m_BoundaryTest = BTEST_ON_EVERY_ENTER;
	m_UpdateTimeout = 0;
}

void go_within_sphere::SetTrigger(Trigger* trig) 
{
	m_pTrigger = trig;
	if (trig &&
		trig->GetOwner() &&
		trig->GetOwner()->HasPlacement())
	{
		m_Center = trig->GetOwner()->GetPlacement()->GetPosition();
	}
	else
	{
		m_Center = SiegePos::INVALID;
	}
}

bool go_within_sphere::IsComplicated() const
{
	return HasBoundaryOccupants() || HasSatisfyingOccupants();
};

void go_within_sphere::FetchParameterList(Parameter &parameterlist)
{	
	// Define the format of the parameter list (for SiegeEditor)
	Parameter::format p0;
	p0.sName		= "Radius";
	p0.isFloat		= true;
	p0.isRequired	= true;
	p0.f_min		= 0.0f;
	p0.f_max		= 1000.0f;
	p0.f_default	= m_Radius;
	parameterlist.Add( p0 );

	Parameter::format p1;
	p1.sName		= "Scid index";
	p1.isInt		= true;
	p1.isRequired	= true;
	p1.bSaveAsHex	= true;
	p1.i_default	= (int)m_Scid;
	parameterlist.Add( p1 );

	Parameter::format p2;
	p2.sName		= "Template name filter";
	p2.isString		= true;
	p2.isRequired	= false;
	p2.s_default	= m_TemplateNameFilter;
	parameterlist.Add( p2 );

	Parameter::format px;
	FetchBoundaryTestFormat(px);
	parameterlist.Add( px );

}

bool go_within_sphere::StoreParameterValues(const Params &parms)
{
	gpassert(m_wId == parms.ParamID);
	m_sDoc = parms.Doc;
	m_iSubGroup = parms.SubGroup;
	// Convert the messages set by SiegeEditor to useable values
	if (!parms.floats.empty() && (!parms.ints.empty() || !parms.strings.empty()))
	{
		m_Radius = parms.floats[0];

		if (!parms.ints.empty() && (parms.ints[0] != 0))
		{
			m_Scid = MakeScid(parms.ints[0]);
		} 

		if ( !parms.strings.empty())
		{
			if (m_Scid == MakeScid(0))
			{
				m_TemplateNameFilter = parms.strings[ 0 ];
			}
			else
			{
				m_TemplateNameFilter = "";
			}

			if ( parms.strings.size()>1 )
			{
				FromString(parms.strings[1].c_str(),m_BoundaryTest);
			}
		}
		else
		{
			m_TemplateNameFilter = "";
			m_BoundaryTest = BTEST_ON_EVERY_ENTER;
		}


		return true;
	}
	else 
	{
		gpwarningf(( "Invalid trigger parameter(s) supplied for go_within_sphere" ));
		return false;
	}
}

bool go_within_sphere::FetchParameterValues( Params &parms )
{
	parms.Clear();
	parms.Doc = m_sDoc;
	parms.SubGroup = m_iSubGroup;
	parms.ParamID = m_wId; 
	parms.floats.push_back(m_Radius);
	parms.strings.push_back(m_TemplateNameFilter);
	parms.ints.push_back(MakeInt(m_Scid));
	parms.strings.push_back(ToString(m_BoundaryTest));
	return true;
}

bool go_within_sphere::Evaluate( )
{
	// All "go_within..." condition needs to be periodically updated to look for non-moving gos
	double wt = gWorldTime.GetTime();

	if (m_UpdateTimeout < wt)
	{
		m_UpdateTimeout = PreciseAdd(wt, Random(0.25f,0.75f));	// Poll again after random delay

		GopColl possibles;

		// Mark all "items" inside the boundary as stale
		GoOccupantMapIter it;
		for (it = m_CurrentInsideBoundary.begin(); it != m_CurrentInsideBoundary.end();)
		{
			Go* i = GetGo((*it).first);	
			if (i)
			{
				if (!i->HasFollower())
				{
					(*it).second = -1;
				}
				++it;
			}
			else
			{
				gpwarningf(("TRIGGER ERROR: [%s:0x%08x:0x%04x] Can't resolve GO [0x%08x] for the occupant of bounding sphere\n",
					(m_pTrigger && m_pTrigger->GetOwner()) ? m_pTrigger->GetOwner()->GetTemplateName() : "unknown",
					(m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1,
					m_wId,
					(*it).first
					));
				it = m_CurrentInsideBoundary.erase(it);
			}
		}

		if ( gAIQuery.GetOccupantsInSphere(
						m_Center,
						m_Radius,
						NULL,
						NULL,
						0,
						OF_NONE,
						&possibles,
						false ) )
		{
			Go* pown = m_pTrigger->GetOwner();
			if (m_TemplateNameFilter.empty())
			{
				for (GopColl::iterator git=possibles.begin(); git!=possibles.end(); ++git)
				{
					if ((*git) == pown)
					{
						continue;
					}
					if ((m_Scid == MakeScid(0)) || ((*git)->GetScid() == m_Scid))
					{
						SiegePos pos = (*git)->GetPlacement()->GetPosition();
						if (!(*git)->HasFollower() && CheckInsideSphere((*git), m_Center, m_Radius, OF_NONE, pos))
						{
							Goid occgoid = (*git)->GetGoid();
							GoOccupantMapIter occit = m_CurrentInsideBoundary.find(occgoid);
							if (occit != m_CurrentInsideBoundary.end())
							{
								// Mark it as not stale
								(*occit).second = 1;
							}
							else
							{
								AddOccupantIntoBoundary(occgoid);

								// I HATE GO_WITHIN.... TRIGGERS!!!!
								// Set the owner's bucket dirty so we can be sure that the 
								// clients all get the updated occupants when the trigger is refreshed
								m_pTrigger->GetOwner()->SetBucketDirty(true);

							}
						}
					}
				}
			}
			else
			{
				for (GopColl::iterator git=possibles.begin(); git!=possibles.end(); ++git)
				{
					if ((*git) == pown)
					{
						continue;
					}
					if (m_TemplateNameFilter.find( (*git)->GetTemplateName() ) != gpstring::npos)
					{
						gpassert((*git)->HasPlacement());
						SiegePos pos = (*git)->GetPlacement()->GetPosition();
						if (!(*git)->HasFollower() && CheckInsideSphere((*git), m_Center, m_Radius, OF_NONE, pos))
						{
							Goid occgoid = (*git)->GetGoid();
							GoOccupantMapIter occit = m_CurrentInsideBoundary.find(occgoid);
							if (occit != m_CurrentInsideBoundary.end())
							{
								// Mark it as not stale
								(*occit).second = 1;
							}
							else
							{
								AddOccupantIntoBoundary(occgoid);
								// I HATE GO_WITHIN.... TRIGGERS!!!!
								// Set the owner's bucket dirty so we can be sure that the 
								// clients all get the updated occupants when the trigger is refreshed
								m_pTrigger->GetOwner()->SetBucketDirty(true);
							}
						}
					}
				}
			}
		}

		// Delete all stale entries
		for (it = m_CurrentInsideBoundary.begin(); it != m_CurrentInsideBoundary.end();)
		{
			if ((*it).second == -1)
			{
				(*it).second = 1;  
				RemoveOccupantFromBoundary((*it).first,false,it);
			}
			else
			{
				++it;
			}
		}
	}

	return EvaluateSatisfaction();

}

bool go_within_sphere::CheckForInsideBoundary( const Go* in_Mover, const SiegePos& in_Pos )
{
	PARANOID_IN_WORLD_TEST

	if (IsDormant()) return false;

	// Is this a Go we are looking for?
	bool hit;
	if (m_TemplateNameFilter.empty())
	{
		hit = (m_Scid == MakeScid(0)) || (in_Mover->GetScid() == m_Scid);
	}
	else
	{
		hit = m_TemplateNameFilter.find( in_Mover->GetTemplateName() ) != gpstring::npos;
	}
	
	if (hit) 
	{
		hit = CheckInsideSphere(in_Mover,
							 m_Center,
							 m_Radius,
							 OF_CAN_MOVE_SELF,
							 in_Pos);
	}
	return hit;
}

bool go_within_sphere::CollectInsideBoundary( GopColl& in_Occupants )
{
	PARANOID_IN_WORLD_TEST

	if (IsDormant()) return false;

	// Force the next update to look for static items too
	m_UpdateTimeout = 0;

	GopColl possibles;
	in_Occupants.clear();

	if ( gAIQuery.GetOccupantsInSphere(
					m_Center,
					m_Radius+5.0f,	// Overestimate initial query
					NULL,
					NULL,
					0,
					OF_CAN_MOVE_SELF,
					&possibles,
					false ) )
	{
		in_Occupants.clear();
		Go* pown = m_pTrigger->GetOwner();
		if (m_TemplateNameFilter.empty())
		{
			for (GopColl::iterator git=possibles.begin(); git!=possibles.end(); ++git)
			{
				if ((*git) == pown)
				{
					continue;
				}
				if ((m_Scid == MakeScid(0)) || ((*git)->GetScid() == m_Scid))
				{
					SiegePos pos;
					bool ok=false;
					if ((*git)->HasFollower())
					{	
						MCP::Plan* jplan = gMCP.FindPlan((*git)->GetGoid(),false);
						if (jplan)
						{
							// get the earliest clip time to test if we are inside the boudary
							// this will give us the most accurate view of the trigger's occupants 
							double clipTime = gWorldTime.GetTime();
							jplan->GetEarliestClipOrTransmittedTime( clipTime );
							
							pos = jplan->GetPosition(clipTime,true,true);
							ok = true;
						}
					}
					if (!ok)
					{
						pos = (*git)->GetPlacement()->GetPosition();
					}

					if (CheckForInsideBoundary(*git,pos))
					{
						in_Occupants.push_back(*git);
					}
				}
			}
		}
		else
		{
			for (GopColl::iterator git=possibles.begin(); git!=possibles.end(); ++git)
			{
				if ((*git) == pown)
				{
					continue;
				}
				if (m_TemplateNameFilter.find( (*git)->GetTemplateName() ) != gpstring::npos)
				{
					SiegePos pos;
					bool ok=false;
					if ((*git)->HasFollower())
					{	
						MCP::Plan* jplan = gMCP.FindPlan((*git)->GetGoid(),false);
						if (jplan)
						{
							// get the earliest clip time to test if we are inside the boudary
							// this will give us the most accurate view of the trigger's occupants
							double clipTime = gWorldTime.GetTime();
							jplan->GetEarliestClipOrTransmittedTime( clipTime );
							
							pos = jplan->GetPosition(clipTime,true,true);
							ok = true;
						}
					}
					if (!ok)
					{
						pos = (*git)->GetPlacement()->GetPosition();
					}
					if (CheckForInsideBoundary(*git,pos))
					{
						in_Occupants.push_back(*git);
					}
				}
			}
		}
	}

	return !in_Occupants.empty();
}

bool go_within_sphere::CheckForBoundaryCollision( const Go*		in_Mover, 
								const SiegePos&	in_PosA, 
								const SiegePos&	in_PosB, 
								float&			out_tEnter, 
								float&			out_tLeave,
								bool&			out_AlreadyInside,
								bool&			out_OutsideAfterwards ) 
{
	PARANOID_IN_WORLD_TEST

	if (IsDormant())
	{
		out_tEnter = -2.0f;
		out_tLeave =  2.0f;
		out_AlreadyInside = false;
		out_OutsideAfterwards = true;
		return false;
	}


	bool hit;
	out_AlreadyInside = false;
	out_OutsideAfterwards = true;
	
	// Is this a Go we are looking for?
	if (m_TemplateNameFilter.empty())
	{
		hit = (m_Scid == MakeScid(0)) || (in_Mover->GetScid() == m_Scid);
	}
	else
	{
		hit = m_TemplateNameFilter.find( in_Mover->GetTemplateName() ) != gpstring::npos;
	}

	if (hit)
	{
		hit = CheckSphereCollision(	in_Mover,
									m_Center,
									m_Radius,
									OF_NONE,
									in_PosA,
									in_PosB,
									out_tEnter,
									out_tLeave,
									out_AlreadyInside,
									out_OutsideAfterwards);
	}

	return hit;
}

bool go_within_sphere::Xfer( FuBi::PersistContext& persist )
{
	Condition::Xfer(persist);
	persist.Xfer( "m_Scid",               m_Scid               );
	persist.Xfer( "m_TemplateNameFilter", m_TemplateNameFilter );
	persist.Xfer( "m_Radius",             m_Radius             );
	return true;
}

#if !GP_RETAIL
SiegePos go_within_sphere::GetLabelPos()
{
	SiegePos p = m_Center;
	p.pos.y +=  min_t(5.0f,m_Radius*1.25f);
	return p;
}
void go_within_sphere::Draw( const ConditionDb& cond )
{
	GoOccupantMapIter it;
	int bcount = 0;
	for (it = m_CurrentInsideBoundary.begin(); it != m_CurrentInsideBoundary.end(); ++it)
	{
		bcount += (*it).second;	
	}

	int scount = 0;
	for (it = m_CurrentSatisfyingOccupants.begin(); it != m_CurrentSatisfyingOccupants.end(); ++it)
	{
		scount += (*it).second;	
	}

	SiegePos LabelPos;
	gpstring lbl,lblsuffix;
	if (m_pTrigger->GetOwner() && m_pTrigger->GetOwner()->HasCommon())
	{
		m_pTrigger->GetOwner()->GetCommon()->PadDebugLabelHelper(m_pTrigger,"\n\n\n\n",lbl,lblsuffix);
	}
	Truid tr = m_pTrigger->GetTruid();
	ConditionConstIter iCondition = cond.begin();
	for(; iCondition != cond.end(); ++iCondition )
	{
		if (m_wId == (*iCondition).second->GetID())
		{
			double et = (m_pTrigger->GetResetTimeout() <= 0.0f) ? 0 : PreciseSubtract(m_pTrigger->GetResetTimeout(), gWorldTime.GetTime());
			lbl.appendf("%s:go_within_sphere:%5.2f",m_pTrigger->GetCurrentActive() ? (IsDormant()?"X":"A"):"D", et );
			int sc = (m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1;
			int expsig = m_pTrigger->GetNumExpectedSignals(m_wId);
			lbl.appendf("\n%08x:%04x:%04x:%08x (%d)",tr.GetOwnGoid(),tr.GetTrigNum(),m_wId,sc,expsig);
			lbl.appendf("\n%s",ToString(m_BoundaryTest));
			lbl.appendf("\n%d(%d):%d(%d)",bcount,m_CurrentInsideBoundary.size(),scount, m_CurrentSatisfyingOccupants.size());
		}
		else
		{
			lbl.appendf("\n\n\n\n");
		}
		LabelPos = (*iCondition).second->GetLabelPos();
	}
	lbl.append(lblsuffix);
	gWorld.DrawDebugSphere( m_Center, m_Radius, COLOR_CHERRY, "" );
	if (LabelPos.node.IsValidSlow() && !gGoDb.IsEditMode())
	{
		gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), LabelPos.ScreenPos(), lbl );
	}
}
#endif


///////////////////////////////////////////////////////////////////////////////////
//
// go_within_bounding_box
//

go_within_bounding_box::go_within_bounding_box() 
{
	m_Scid = (Scid)0;
	m_TemplateNameFilter = "";
	m_HalfDiag = vector_3(1.0f,0.5f,1.0f);
	m_Center = SiegePos::INVALID;
	m_InvOrient = matrix_3x3::IDENTITY;
	m_BoundaryTest = BTEST_ON_EVERY_ENTER;
	m_UpdateTimeout = 0;
}

void go_within_bounding_box::SetTrigger(Trigger* trig) 
{
	m_pTrigger = trig;
	if (trig &&
		trig->GetOwner() &&
		trig->GetOwner()->HasPlacement())
	{
		m_Center = trig->GetOwner()->GetPlacement()->GetPosition();
		m_InvOrient = Transpose(trig->GetOwner()->GetPlacement()->GetOrientation().BuildMatrix());
	}
	else
	{
		m_Center = SiegePos::INVALID;
	}
}

bool go_within_bounding_box::IsComplicated() const
{
	return HasBoundaryOccupants() || HasSatisfyingOccupants();
};

void go_within_bounding_box::FetchParameterList(Parameter &parameterlist)
{
	// Define the format of the parameter list (for SiegeEditor)
	Parameter::format p0;
	p0.sName		= "Half diag X";
	p0.isFloat		= true;
	p0.isRequired	= false;
	p0.f_min		= 0.0f;
	p0.f_max		= 1000.0f;
	p0.f_default	= m_HalfDiag.x;
	parameterlist.Add( p0 );

	Parameter::format p1;
	p1.sName		= "Half diag Y";
	p1.isFloat		= true;
	p1.isRequired	= false;
	p1.f_min		= 0.0f;
	p1.f_max		= 1000.0f;
	p1.f_default	= m_HalfDiag.y;
	parameterlist.Add( p1 );

	Parameter::format p2;
	p2.sName		= "Half diag Z";
	p2.isFloat		= true;
	p2.isRequired	= false;
	p2.f_min		= 0.0f;
	p2.f_max		= 1000.0f;
	p2.f_default	= m_HalfDiag.z;
	parameterlist.Add( p2 );

	Parameter::format p3;
	p3.sName		= "Scid index";
	p3.isInt		= true;
	p3.isRequired	= true;
	p3.bSaveAsHex	= true;
	p3.i_default	= (int)m_Scid;
	parameterlist.Add( p3 );

	Parameter::format p4;
	p4.sName		= "Template name filter";
	p4.isString		= true;
	p4.isRequired	= false;
	p4.s_default	= m_TemplateNameFilter;
	parameterlist.Add( p4 );

	Parameter::format px;
	FetchBoundaryTestFormat(px);
	parameterlist.Add( px );

}

bool go_within_bounding_box::StoreParameterValues(const Params &parms)
{
	gpassert(m_wId == parms.ParamID);
	m_sDoc = parms.Doc;
	m_iSubGroup = parms.SubGroup;
	// Convert the messages set by SiegeEditor to useable values
	if ((parms.floats.size() == 3) && (!parms.ints.empty() || !parms.strings.empty()))
	{
		m_HalfDiag = vector_3(parms.floats[0],parms.floats[1],parms.floats[2]);

		if (!parms.ints.empty() && parms.ints[0] != 0) 
		{
			m_Scid = MakeScid(parms.ints[0]);
		}

		if ( !parms.strings.empty())
		{
			if (m_Scid == MakeScid(0))
			{
				m_TemplateNameFilter = parms.strings[ 0 ];
			}
			else
			{
				m_TemplateNameFilter = "";
			}

			if ( parms.strings.size()>1 )
			{
				FromString(parms.strings[1].c_str(),m_BoundaryTest);
			}
		}
		else
		{
			m_TemplateNameFilter = "";
			m_BoundaryTest = BTEST_ON_EVERY_ENTER;
		}

		return true;
	}
	else
	{
		gpwarningf(( "Invalid trigger parameter supplied for go_within_bounding_box" ));
		return false;
	}
}

bool go_within_bounding_box::FetchParameterValues( Params &parms )
{
	parms.Clear();
	parms.Doc = m_sDoc;
	parms.SubGroup = m_iSubGroup;
	parms.ParamID = m_wId; 
	parms.floats.push_back(m_HalfDiag.x);
	parms.floats.push_back(m_HalfDiag.y);
	parms.floats.push_back(m_HalfDiag.z);
	parms.ints.push_back(MakeInt(m_Scid));
	parms.strings.push_back(m_TemplateNameFilter);
	parms.strings.push_back(ToString(m_BoundaryTest));
	return true;
}

bool go_within_bounding_box::Evaluate( )
{
	// All "go_within..." condition needs to be periodically updated to look for non-moving gos
	double wt = gWorldTime.GetTime();

	if (m_UpdateTimeout < wt)
	{
		m_UpdateTimeout = PreciseAdd(wt, Random(0.25f,0.75f));	// Poll again after random delay

		GopColl possibles;

		// Mark all "items" inside the boundary as stale
		GoOccupantMapIter it;
		for (it = m_CurrentInsideBoundary.begin(); it != m_CurrentInsideBoundary.end();)
		{
			Go* i = GetGo((*it).first);	
			if (i)
			{
				if (!i->HasFollower())
				{
					(*it).second = -1;
				}
				++it;
			}
			else
			{
/*				gpwarningf(("TRIGGER ERROR: [%s:0x%08x:0x%04x] Can't resolve GO [0x%08x] for the occupant of bounding box\n",
					(m_pTrigger && m_pTrigger->GetOwner()) ? m_pTrigger->GetOwner()->GetTemplateName() : "unknown",
					(m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1,
					m_wId,
					(*it).first
					));
*/
				(*it).second = 1;  
				RemoveOccupantFromBoundary((*it).first,false,it);
			}
		}

		if ( gAIQuery.GetOccupantsInSphere(
						m_Center,
						m_HalfDiag.Length(),
						NULL,
						NULL,
						0,
						OF_NONE,
						&possibles,
						false ) )
		{
			Go* pown = m_pTrigger->GetOwner();
			if (m_TemplateNameFilter.empty())
			{
				for (GopColl::iterator git=possibles.begin(); git!=possibles.end(); ++git)
				{
					if ((*git) == pown)
					{
						continue;
					}
					if (((m_Scid == MakeScid(0)) || ((*git)->GetScid() == m_Scid)))
					{
						SiegePos pos = (*git)->GetPlacement()->GetPosition();
						if (!(*git)->HasFollower() && CheckInsideBox( *git, m_Center, m_InvOrient, m_HalfDiag, OF_NONE, pos ))
						{
							Goid occgoid = (*git)->GetGoid();
							GoOccupantMapIter occit = m_CurrentInsideBoundary.find(occgoid);
							if (occit != m_CurrentInsideBoundary.end())
							{
								// Mark it as not stale
								(*occit).second = 1;
							}
							else
							{
								AddOccupantIntoBoundary(occgoid);
								// I HATE GO_WITHIN.... TRIGGERS!!!!
								// Set the owner's bucket dirty so we can be sure that the 
								// clients all get the updated occupants when the trigger is refreshed
								m_pTrigger->GetOwner()->SetBucketDirty(true);
							}
						}
					}
				}
			}
			else
			{
				for (GopColl::iterator git=possibles.begin(); git!=possibles.end(); ++git)
				{
					if ((*git) == pown)
					{
						continue;
					}
					if (m_TemplateNameFilter.find( (*git)->GetTemplateName() ) != gpstring::npos)
					{
						gpassert((*git)->HasPlacement());
						SiegePos pos = (*git)->GetPlacement()->GetPosition();
						if (!(*git)->HasFollower() && CheckInsideBox( *git, m_Center, m_InvOrient, m_HalfDiag, OF_NONE, pos ))
						{
							Goid occgoid = (*git)->GetGoid();
							GoOccupantMapIter occit = m_CurrentInsideBoundary.find(occgoid);
							if (occit != m_CurrentInsideBoundary.end())
							{
								// Mark it as not stale
								(*occit).second = 1;
							}
							else
							{
								AddOccupantIntoBoundary(occgoid);
								// I HATE GO_WITHIN.... TRIGGERS!!!!
								// Set the owner's bucket dirty so we can be sure that the 
								// clients all get the updated occupants when the trigger is refreshed
								m_pTrigger->GetOwner()->SetBucketDirty(true);
							}
						}
					}
				}
			}
		}

		// Delete all stale entries
		for (it = m_CurrentInsideBoundary.begin(); it != m_CurrentInsideBoundary.end();)
		{
			if ((*it).second == -1)
			{
				(*it).second = 1;  
				RemoveOccupantFromBoundary((*it).first,false,it);
			}
			else
			{
				++it;
			}
		}
	}

	return EvaluateSatisfaction();

}

bool go_within_bounding_box::CheckForInsideBoundary( const Go* in_Mover, const SiegePos& in_Pos )
{
	PARANOID_IN_WORLD_TEST

	if (IsDormant()) return false;

	bool hit;
	
	// Is this a Go we are looking for?
	if (m_TemplateNameFilter.empty())
	{
		hit = (m_Scid == MakeScid(0)) || (in_Mover->GetScid() == m_Scid);
	}
	else
	{
		hit = m_TemplateNameFilter.find( in_Mover->GetTemplateName() ) != gpstring::npos;
	}

	if (hit)
	{
		hit = CheckInsideBox( in_Mover,
						   m_Center, m_InvOrient, m_HalfDiag,
						   OF_CAN_MOVE_SELF, 
						   in_Pos );
	}

	return hit;
}

bool go_within_bounding_box::CollectInsideBoundary( GopColl& in_Occupants )
{
	PARANOID_IN_WORLD_TEST

	if (IsDormant()) return false;

	// Force the next update to look for static items too
	m_UpdateTimeout = 0;

	GopColl possibles;
	in_Occupants.clear();

	if ( gAIQuery.GetOccupantsInSphere(
					m_Center,
					m_HalfDiag.Length()+5.0f,	// Overestimate initial query
					NULL,
					NULL,
					0,
					OF_CAN_MOVE_SELF,
					&possibles,
					false ) )
	{
		Go* pown = m_pTrigger->GetOwner();
		for (GopColl::iterator git=possibles.begin(); git!=possibles.end(); ++git)
		{
			if ((*git) == pown)
			{
				continue;
			}

			SiegePos pos;
			bool ok=false;
			if ((*git)->HasFollower())
			{	
				MCP::Plan* jplan = gMCP.FindPlan((*git)->GetGoid(),false);
				if (jplan)
				{
					// get the earliest clip time to test if we are inside the boudary
					// this will give us the most accurate view of the trigger's occupants 
					double clipTime = gWorldTime.GetTime();
					jplan->GetEarliestClipOrTransmittedTime( clipTime );
					
					pos = jplan->GetPosition(clipTime,true,true);
					ok = true;
				}
			}
			if (!ok)
			{
				pos = (*git)->GetPlacement()->GetPosition();
			}

			if (CheckForInsideBoundary(*git,pos))
			{
				in_Occupants.push_back(*git);
			}
		}
	}

	// cull the in_Occupants that passed the sphere test
	return !in_Occupants.empty();
}

bool go_within_bounding_box::CheckForBoundaryCollision( const Go*		in_Mover, 
								const SiegePos&	in_PosA, 
								const SiegePos&	in_PosB, 
								float&			out_tEnter, 
								float&			out_tLeave,
								bool&			out_AlreadyInside,
								bool&			out_OutsideAfterwards ) 
{

	if (IsDormant())
	{
		out_tEnter = -2.0f;
		out_tLeave =  2.0f;
		out_AlreadyInside = false;
		out_OutsideAfterwards = true;
		return false;
	}


	bool hit;
	out_AlreadyInside = false;
	out_OutsideAfterwards = true;
	
	// Is this a Go we are looking for?
	if (m_TemplateNameFilter.empty())
	{
		hit = (m_Scid == MakeScid(0)) || (in_Mover->GetScid() == m_Scid);
	}
	else
	{
		hit = m_TemplateNameFilter.find( in_Mover->GetTemplateName() ) != gpstring::npos;
	}

	if (hit)
	{
		hit = CheckBoxCollision(in_Mover,
								m_Center, m_InvOrient, m_HalfDiag,
								OF_NONE,
								in_PosA,
								in_PosB,
								out_tEnter,
								out_tLeave,
								out_AlreadyInside,
								out_OutsideAfterwards);
	}

	return hit;
}

bool go_within_bounding_box::Xfer( FuBi::PersistContext& persist )
{
	Condition::Xfer(persist);
	persist.Xfer ( "m_Scid",               m_Scid               );
	persist.Xfer ( "m_TemplateNameFilter", m_TemplateNameFilter );
	persist.Xfer ( "m_HalfDiag",           m_HalfDiag           );
	return true;
}

#if !GP_RETAIL
SiegePos go_within_bounding_box::GetLabelPos()
{
	SiegePos p = m_Center;
	p.pos.y += min_t(5.0f,m_HalfDiag.y*1.25f);
	return p;
}
void go_within_bounding_box::Draw( const ConditionDb& cond )
{
	GoOccupantMapIter it;
	int bcount = 0;
	for (it = m_CurrentInsideBoundary.begin(); it != m_CurrentInsideBoundary.end(); ++it)
	{
		bcount += (*it).second;	
	}

	int scount = 0;
	for (it = m_CurrentSatisfyingOccupants.begin(); it != m_CurrentSatisfyingOccupants.end(); ++it)
	{
		scount += (*it).second;	
	}

	SiegePos LabelPos;
	gpstring lbl,lblsuffix;
	if (m_pTrigger->GetOwner() && m_pTrigger->GetOwner()->HasCommon())
	{
		m_pTrigger->GetOwner()->GetCommon()->PadDebugLabelHelper(m_pTrigger,"\n\n\n\n",lbl,lblsuffix);
	}
	Truid tr = m_pTrigger->GetTruid();
	ConditionConstIter iCondition = cond.begin();
	for(; iCondition != cond.end(); ++iCondition )
	{
		if (m_wId == (*iCondition).second->GetID())
		{
			double et = (m_pTrigger->GetResetTimeout() <= 0.0f) ? 0 : PreciseSubtract(m_pTrigger->GetResetTimeout(), gWorldTime.GetTime());
			lbl.appendf("%s:go_within_bounding_box:%5.2f",m_pTrigger->GetCurrentActive() ? (IsDormant()?"X":"A"):"D", et );
			int sc = (m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1;
			int expsig = m_pTrigger->GetNumExpectedSignals(m_wId);
			lbl.appendf("\n%08x:%04x:%04x:%08x (%d)",tr.GetOwnGoid(),tr.GetTrigNum(),m_wId,sc,expsig);
			lbl.appendf("\n%s",ToString(m_BoundaryTest));
			lbl.appendf("\n%d(%d):%d(%d)",bcount,m_CurrentInsideBoundary.size(),scount, m_CurrentSatisfyingOccupants.size());
		}
		else
		{
			lbl.appendf("\n\n\n\n");
		}
		LabelPos = (*iCondition).second->GetLabelPos();
	}
	lbl.append(lblsuffix);
	gWorld.DrawDebugBox( m_Center, Transpose(m_InvOrient), m_HalfDiag, 0xFF9999E0, 1.0f/60.0f, false, false, "" );
	if (LabelPos.node.IsValidSlow() && !gGoDb.IsEditMode())
	{
		gSiegeEngine.GetLabels().DrawScreenLabel( &gServices.GetConsoleFont(), LabelPos.ScreenPos(), lbl );
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////////
//
// has_go_in_inventory
//

has_go_in_inventory::has_go_in_inventory() 
{
	m_LocalInventory = true;
	m_Scid = (Scid)0;
	m_TemplateNameFilter = "";
	m_BoundaryTest = BTEST_ON_EVERY_ENTER;
}

void has_go_in_inventory::SetTrigger(Trigger* trig) 
{
	m_pTrigger = trig;
	if (trig &&
		trig->GetOwner() &&
		trig->GetOwner()->HasPlacement())
	{
		m_Center = trig->GetOwner()->GetPlacement()->GetPosition();
	}
	else
	{
		m_Center = SiegePos::INVALID;
	}
}

bool has_go_in_inventory::IsComplicated() const
{
	return HasBoundaryOccupants() || HasSatisfyingOccupants();
};

void has_go_in_inventory::FetchParameterList(Parameter &parameterlist)
{
	// Define the format of the parameter list (for SiegeEditor)
	Parameter::format p0;
	p0.sName		= "Player inventory";
	p0.isString		= true;
	p0.isRequired	= false;
	p0.sValues.push_back( "local" );
	p0.sValues.push_back( "any" );
	p0.s_default = "any";
	parameterlist.Add( p0 );

	Parameter::format p1;
	p1.sName		= "Scid index";
	p1.isInt		= true;
	p1.isRequired	= true;
	p1.bSaveAsHex	= true;
	p1.i_default	= int(m_Scid);
	parameterlist.Add( p1 );

	Parameter::format p2;
	p2.sName		= "Template name filter";
	p2.isString		= true;
	p2.isRequired	= false;
	p2.s_default	= m_TemplateNameFilter;
	parameterlist.Add( p2 );
}

bool has_go_in_inventory::StoreParameterValues(const Params &parms)
{
	gpassert(m_wId == parms.ParamID);
	m_sDoc = parms.Doc;
	m_iSubGroup = parms.SubGroup;
	// Convert the messages set by SiegeEditor to useable values
	if (!parms.strings.empty() && (!parms.ints.empty() || (parms.strings.size() > 1) ) )
	{
		if (parms.strings[0].same_no_case( "local" ))
		{
			m_LocalInventory = true;
		}
		else 
		{
			m_LocalInventory = false;
		}
		
		if (!parms.ints.empty() && (parms.ints[0] != 0))
		{
			m_Scid = MakeScid(parms.ints[0]);
		}

		if ( !parms.strings.empty())
		{
			if (m_Scid == MakeScid(0))
			{
				m_TemplateNameFilter = parms.strings[ 0 ];
			}
			else
			{
				m_TemplateNameFilter = "";
			}
		}
		
		if ( (m_Scid == MakeScid(0)) && !parms.strings.empty() && (parms.strings[ 1 ] != 0) )
		{
			m_TemplateNameFilter = parms.strings[ 1 ];
		}
		else
		{
			m_TemplateNameFilter = "";
		}
		return true;
	}
	else
	{
		gpwarningf(( "Invalid trigger parameter supplied for has_go_in_inventory" ));
		return false;
	}
}

bool has_go_in_inventory::FetchParameterValues( Params &parms )
{
	parms.Clear();
	parms.Doc = m_sDoc;
	parms.SubGroup = m_iSubGroup;
	parms.ParamID = m_wId; 
	parms.strings.push_back( (m_LocalInventory) ? "local" : "any" );
	parms.strings.push_back(m_TemplateNameFilter);
	parms.ints.push_back(MakeInt(m_Scid));
	return true;
}

void has_go_in_inventory::SearchInventoryHelper( const GopColl& partyColl, const gpstring& filter, const Scid& scid )
{
	GopColl::const_iterator iPartyMember;
	GopColl::const_iterator iBegMember = partyColl.begin();
	GopColl::const_iterator iEndMember = partyColl.end();

	for (iPartyMember = iBegMember ; iPartyMember != iEndMember; ++iPartyMember )
	{
		if ( !(*iPartyMember)->HasInventory() )
		{
			continue;
		}

		bool found_in_inventory = false;

		GopColl storedItems;
		(*iPartyMember)->GetInventory()->GetInventoryDb( storedItems );

		if ( filter.empty() )
		{
			for ( GopColl::iterator i = storedItems.begin(); i != storedItems.end(); ++i )
			{
				if ( scid == (*i)->GetScid() )
				{
					found_in_inventory = true;
					break;
				}
			}
		}
		else 
		{
			for ( GopColl::iterator i = storedItems.begin(); i != storedItems.end(); ++i )
			{
				if ( filter.same_no_case( (*i)->GetTemplateName() ) )
				{
					found_in_inventory = true;
					break;
				}
			}
		}
	
		Goid memgoid = (*iPartyMember)->GetGoid();
		bool already_in = m_CurrentSatisfyingOccupants.find(memgoid) != m_CurrentSatisfyingOccupants.end();

		if (found_in_inventory)
		{
			if (!already_in)
			{
				 m_CurrentSatisfyingOccupants[memgoid] = 1;
			}
		}
		else
		{
			if (!already_in)
			{
				 m_CurrentSatisfyingOccupants.erase(memgoid);
			}
		}
	}
}

bool has_go_in_inventory::Evaluate( )
{
	if ( m_LocalInventory && gServer.GetLocalHumanPlayer() )
	{
		Go * pParty = gServer.GetLocalHumanPlayer()->GetParty();
		if ( pParty )
		{
			SearchInventoryHelper(pParty->GetChildren(),m_TemplateNameFilter,m_Scid);
		}
	}
	else
	{
		for ( Server::PlayerColl::const_iterator iPlayer = gServer.GetPlayers().begin(); iPlayer != gServer.GetPlayers().end(); ++iPlayer )
		{
			if ( IsValid(*iPlayer) && !(*iPlayer)->IsComputerPlayer() )
			{
				Go * pParty = (*iPlayer)->GetParty();
				if ( pParty )
				{
					SearchInventoryHelper(pParty->GetChildren(),m_TemplateNameFilter,m_Scid);
				}
			}
		}

	}
	
	return EvaluateSatisfaction();
}

bool has_go_in_inventory::Xfer( FuBi::PersistContext& persist )
{
	Condition::Xfer(persist);
	persist.Xfer( "m_LocalInventory",     m_LocalInventory     );
	persist.Xfer( "m_Scid",               m_Scid               );
	persist.Xfer( "m_TemplateNameFilter", m_TemplateNameFilter );
	return true;
}

#if !GP_RETAIL
void has_go_in_inventory::Draw( const ConditionDb& /*cond*/ )
{
}
#endif

///////////////////////////////////////////////////////////////////////////////////
//
// party_member_within_trigger_group
//

party_member_within_trigger_group::party_member_within_trigger_group() 
{
	m_Center = SiegePos::INVALID;
	m_BoundaryTest = BTEST_ON_EVERY_ENTER;
}

party_member_within_trigger_group::~party_member_within_trigger_group() 
{
	if (m_pTrigger && (m_pTrigger->GetTruid() != Truid()))
	{
		gTriggerSys.RemoveGroupWatcher(	m_GroupName, Cuid(m_pTrigger->GetTruid(), m_wId) );
	}
}

void party_member_within_trigger_group::SetTrigger(Trigger* trig) 
{
	if (m_pTrigger != trig)
	{
		m_pTrigger = trig;
		if (trig &&
			trig->GetOwner() &&
			trig->GetOwner()->HasPlacement())
		{
			m_Center = m_pTrigger->GetOwner()->GetPlacement()->GetPosition();
			gTriggerSys.AddGroupWatcher( m_GroupName, Cuid(m_pTrigger->GetTruid(), m_wId) );
		}
		else
		{
			m_Center = SiegePos::INVALID;
		}
	}
}

bool party_member_within_trigger_group::IsComplicated() const
{
	return HasBoundaryOccupants() || HasSatisfyingOccupants();
};

void party_member_within_trigger_group::FetchParameterList(Parameter &parameterlist)
{
	// Define the format of the parameter list (for SiegeEditor)
	Parameter::format p0;
	p0.sName			= "Group Name";
	p0.isString			= true;
	p0.isRequired		= true;
	parameterlist.Add( p0 );

	Parameter::format px;
	FetchBoundaryTestFormat(px);
	parameterlist.Add( px );
}

bool party_member_within_trigger_group::StoreParameterValues(const Params &parms)
{
	gpassert(m_wId == parms.ParamID);
	m_sDoc = parms.Doc;
	m_iSubGroup = parms.SubGroup;
	// Convert the messages set by SiegeEditor to useable values
	if ( !parms.strings.empty() && !parms.strings[0].empty() ) 
	{
		m_GroupName = parms.strings[0];
		if (parms.strings.size()>1)
		{
			FromString(parms.strings[1].c_str(),m_BoundaryTest);
		}
		else
		{
			m_BoundaryTest = BTEST_ON_EVERY_ENTER;
		}
		return true;
	}
	else
	{
		gpwarningf(( "Invalid trigger parameter supplied for party_member_within_trigger_group" ));
		return false;
	}
}

bool party_member_within_trigger_group::FetchParameterValues( Params &parms )
{
	parms.Clear();
	parms.Doc = m_sDoc;
	parms.SubGroup = m_iSubGroup;
	parms.ParamID = m_wId; 
	parms.strings.push_back(m_GroupName);
	parms.strings.push_back(ToString(m_BoundaryTest));
	return true;
}

bool party_member_within_trigger_group::CheckForInsideBoundary( const Go* in_Mover, const SiegePos& in_Pos )
{
	if (IsDormant()) return false;

	UNREFERENCED_PARAMETER(in_Mover);
	UNREFERENCED_PARAMETER(in_Pos);
	return false;
}

bool party_member_within_trigger_group::CollectInsideBoundary( GopColl& in_Occupants )
{
	if (IsDormant()) return false;

	GroupLink::MemberSet*  members = NULL;

	if (gTriggerSys.GetGroupMembers(m_GroupName,members))
	{
		for ( GroupLink::MemberIter i = members->begin(); i != members->end(); ++i )
		{
			Trigger* trig;
			if ( gTriggerSys.FetchTriggerByTruid(*i,trig) )
			{
				for (ConditionConstIter mcit = trig->GetConditions().begin(); mcit != trig->GetConditions().end(); ++mcit)
				{
					if ((*mcit).second->HasBoundaryOccupants())
					{
						GoOccupantMap* occs = (*mcit).second->GetBoundaryOccupants();
						GoOccupantMapIter it = occs->begin();
						for (;it!=occs->end();++it)
						{
							Goid occugoid = (*it).first;
							in_Occupants.push_back(GetGo(occugoid));
						}
					}
				}
			}
		}
	}

	return !in_Occupants.empty();
}

bool party_member_within_trigger_group::CheckForBoundaryCollision(
								const Go*		in_Mover, 
								const SiegePos&	in_PosA, 
								const SiegePos&	in_PosB, 
								float&			out_tEnter, 
								float&			out_tLeave,
								bool&			out_AlreadyInside,
								bool&			out_OutsideAfterwards ) 
{

	if (IsDormant())
	{
		out_tEnter = -2.0f;
		out_tLeave =  2.0f;
		out_AlreadyInside = false;
		out_OutsideAfterwards = true;
		return false;
	}

	UNREFERENCED_PARAMETER(in_PosA);
	UNREFERENCED_PARAMETER(in_PosB);
	UNREFERENCED_PARAMETER(out_tEnter);
	UNREFERENCED_PARAMETER(out_tLeave);

	// The mover is crossing a group member boundary, all we need to determine
	// is whether or not is is a party member. Sweet!

	out_AlreadyInside = false;
	out_OutsideAfterwards = !in_Mover->IsAnyHumanPartyMember();
	return !out_OutsideAfterwards;
}

bool party_member_within_trigger_group::AddOccupantIntoBoundary(const Goid in_EnteringGoid)
{
	gpassert (IsGroupWatcher());

	if (IsDormant()) return false;

	bool ret = true;

	GoOccupantMapIter it = m_CurrentInsideBoundary.find(in_EnteringGoid);
	if (it == m_CurrentInsideBoundary.end())
	{
		m_CurrentInsideBoundary[in_EnteringGoid] = 1;
		OnEnterBoundary(in_EnteringGoid);
	}
	else
	{
		if (m_BoundaryTest != BTEST_ON_UNIQUE_ENTER)
		{
			// Make sure that the number of times that this occupant appears does not exceed the number 
			// of members in the group
			if (m_GroupSize <= m_CurrentInsideBoundary[in_EnteringGoid])
			{
#if !GP_RETAIL
				Go* entergo = GetGo(in_EnteringGoid);
					gpdevreportf( &gTriggerSysContext,(
						"[TRIGGERSYS: %s g:%08x s:%08x] attempted to enter a group watcher while already at the max entry count!\n"
						"\tThere are %d references, and only %d are allowed!\n"
						"\tCondition [%s:0x%08x:0x%04x] Trigger Owner [%s:0x%08x]\n",
						entergo ? entergo->GetTemplateName() : "unknown",
						entergo ? MakeInt(entergo->GetGoid()) : -1,
						entergo ? MakeInt(entergo->GetScid()) : -1,
						m_CurrentInsideBoundary[in_EnteringGoid],
						m_GroupSize,
						GetName(),
						(m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetGoid()) : -1,
						GetID(),
						(m_pTrigger && m_pTrigger->GetOwner()) ? m_pTrigger->GetOwner()->GetTemplateName() : "unknown",
						(m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1
						));
#endif
				ret = false;
				// Error!
			}
			else
			{
				m_CurrentInsideBoundary[in_EnteringGoid]++;
			}
		}
	}
	return ret;

}
bool party_member_within_trigger_group::RemoveOccupantFromBoundary(const Goid in_ExitingGoid, bool isDeactivating, GoOccupantMapIter& outIter)
{

	gpassert (IsGroupWatcher());
	
	if (IsDormant())
	{
		outIter = m_CurrentInsideBoundary.end();
		return true;
	};

	bool ret = true;
	outIter = m_CurrentInsideBoundary.find(in_ExitingGoid);

	if (outIter != m_CurrentInsideBoundary.end())
	{
		if (!isDeactivating && (*outIter).second>1)
		{
			(*outIter).second--;
			++outIter;
		}
		else
		{
#if !GP_RETAIL
			Go* exitgo = GetGo(in_ExitingGoid);
			if ( !((*outIter).second>=0) )
			{
				gpwarningf((
					"TRIGGERSYS: [%s:0x%08x] attempted to leave a group watcher but the entry count is %d.\n"
					"\tCondition [%s:%08x:%08x:%04x:%04x]\n"
					"\tTrigger Owner [%s:%08x]\n",
					(*outIter).second,
					exitgo ? exitgo->GetTemplateName() : "unknown",
					exitgo ? MakeInt(exitgo->GetScid()) : -1,
					GetName(),
					(m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetGoid()) : -1,
					(m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1,
					(m_pTrigger && m_pTrigger->GetOwner()) ? m_pTrigger->GetTruid().GetTrigNum() : -1,
					m_wId,
					(m_pTrigger && m_pTrigger->GetOwner()) ? m_pTrigger->GetOwner()->GetTemplateName() : "unknown",
					(m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1
					));
			}
#endif
			if (isDeactivating || (m_BoundaryTest != BTEST_ON_UNIQUE_ENTER))
			{
				outIter = m_CurrentInsideBoundary.erase(outIter);
				OnLeaveBoundary(in_ExitingGoid);
			}
			else
			{
				++outIter;
			}
		}
	}
	else
	{
#if !GP_RETAIL
		Go* exitgo = GetGo(in_ExitingGoid);
		gpwarningf((
			"TRIGGERSYS: [%s:0x%08x] attempted to leave a group watcher it is not inside.\n"
			"\tCondition [%s:%08x:%08x:%04x:%04x]\n"
			"\tTrigger Owner [%s:%08x]\n",
			exitgo ? exitgo->GetTemplateName() : "unknown",
			exitgo ? MakeInt(exitgo->GetScid()) : -1,
			GetName(),
			(m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetGoid()) : -1,
			(m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1,
			(m_pTrigger && m_pTrigger->GetOwner()) ? m_pTrigger->GetTruid().GetTrigNum() : -1,
			m_wId,
			(m_pTrigger && m_pTrigger->GetOwner()) ? m_pTrigger->GetOwner()->GetTemplateName() : "unknown",
			(m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1
			));
#endif
		ret = false;
	}
	return ret;

}

void party_member_within_trigger_group::AddIllegalOccupantCrossing(const double& badtime, const Goid badgoid)
{
	m_Illegals.insert(std::make_pair(badtime,badgoid));
}

bool party_member_within_trigger_group::IsIllegalOccupantCrossing(const double& badtime, const Goid badgoid)
{
	if (m_Illegals.empty())
	{
		return false;
	}

	IllegalCrossingDb::iterator i = m_Illegals.begin();
	IllegalCrossingDb::iterator iend = m_Illegals.end();

	while( i != iend )
	{
		if ((*i).second == badgoid)
		{
			float rel_time = (float)PreciseSubtract(badtime,(*i).first);
			if ( IsPositive(rel_time,2.0f) )
			{
				// The is a busted entry in the m_Illegals list, we should have processed it by now!
				gpdevreportf( &gTriggerSysContext,("[%s:0x%08x:0x%04x] Ignoring a totally bogus illegal boundary crossing for [%s g:0x%08x s:0x%08x]\n  Illegal Time: %f, Check Time: %f (Diff: %f)\n",
					(m_pTrigger && m_pTrigger->GetOwner()) ? m_pTrigger->GetOwner()->GetTemplateName() : "<UNKNOWN>",
					(m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1,
					m_wId,
					GetGo(badgoid) ? GetGo(badgoid)->GetTemplateName() : "<UNKNOWN>",
					badgoid,
					GetGo(badgoid) ? MakeInt(GetGo(badgoid)->GetScid()) : -1,
					(*i).first,
					badtime,
					rel_time
					));
				i = m_Illegals.erase(i);
			}
			else if ( IsNegative(rel_time,0.01f) )
			{
				++i;
			}
			else 
			{
				if ( IsPositive(rel_time,0.01f) )
				{
					gpdevreportf( &gTriggerSysContext,("[%s:0x%08x:0x%04x] Almost missed an illegal boundary crossing for [%s g:0x%08x s:0x%08x]\n  Illegal Time: %f, Check Time: %f (Diff: %f)\n",
						(m_pTrigger && m_pTrigger->GetOwner()) ? m_pTrigger->GetOwner()->GetTemplateName() : "<UNKNOWN>",
						(m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1,
						m_wId,
						GetGo(badgoid) ? GetGo(badgoid)->GetTemplateName() : "<UNKNOWN>",
						badgoid,
						GetGo(badgoid) ? MakeInt(GetGo(badgoid)->GetScid()) : -1,
						badtime,
						rel_time
						));
				}
				else
				{
					gpdevreportf( &gTriggerSysContext,("[%s:0x%08x:0x%04x] Detected illegal boundary crossing for [%s g:0x%08x s:0x%08x]\n  Illegal Time: %f, Check Time: %f (Diff: %f)\n",
						(m_pTrigger && m_pTrigger->GetOwner()) ? m_pTrigger->GetOwner()->GetTemplateName() : "<UNKNOWN>",
						(m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1,
						m_wId,
						GetGo(badgoid) ? GetGo(badgoid)->GetTemplateName() : "<UNKNOWN>",
						badgoid,
						GetGo(badgoid) ? MakeInt(GetGo(badgoid)->GetScid()) : -1,
						badtime,
						rel_time
						));
				}
				i = m_Illegals.erase(i);
				return true;
			}
		}
		else
		{
			++i;
		}
	}
	return false;
}

bool party_member_within_trigger_group::Xfer( FuBi::PersistContext& persist )
{
	Condition::Xfer(persist);
	persist.Xfer(    "m_GroupName", m_GroupName );
	persist.XferMap( "m_Illegals",  m_Illegals );
	return true;
}

#if !GP_RETAIL
void party_member_within_trigger_group::Draw( const ConditionDb& cond )
{
	GoOccupantMapIter it;
	int bcount = 0;
	for (it = m_CurrentInsideBoundary.begin(); it != m_CurrentInsideBoundary.end(); ++it)
	{
		bcount += (*it).second;	
	}

	int scount = 0;
	for (it = m_CurrentSatisfyingOccupants.begin(); it != m_CurrentSatisfyingOccupants.end(); ++it)
	{
		scount += (*it).second;	
	}

	if (m_Center != SiegePos::INVALID )
	{
		gpstring lbl,lblsuffix;
		SiegePos LabelPos;
		Truid tr = m_pTrigger->GetTruid();
		int sc = (m_pTrigger && m_pTrigger->GetOwner()) ? MakeInt(m_pTrigger->GetOwner()->GetScid()) : -1;
		int expsig = m_pTrigger->GetNumExpectedSignals(m_wId);
		if (m_pTrigger->GetOwner() && m_pTrigger->GetOwner()->HasCommon())
		{
			m_pTrigger->GetOwner()->GetCommon()->PadDebugLabelHelper(m_pTrigger,"\n\n\n\n",lbl,lblsuffix);
		}
		// Leave blank lines where other conditions will appear
		// The group draw helper will get called for each watcher(UGH!)

		ConditionConstIter iCondition = cond.begin();
		for(; iCondition != cond.end(); ++iCondition )
		{
			if (m_wId == (*iCondition).second->GetID())
			{
				lbl.appendf("%s",m_GroupName);
				lbl.appendf("\n%s:[%08x:%04x:%04x:%08x](%d)",
					m_pTrigger->GetCurrentActive() ? (IsDormant()?"X":"A"):"D",
					tr.GetOwnGoid(),tr.GetTrigNum(),m_wId,sc,expsig);
				lbl.appendf("\n%s",ToString(m_BoundaryTest));
				lbl.appendf("\n%d(%d):%d(%d)",bcount,m_CurrentInsideBoundary.size(),scount, m_CurrentSatisfyingOccupants.size());
			}
			else
			{
				lbl.appendf("\n\n\n\n");
			}
			LabelPos = (*iCondition).second->GetLabelPos();
		}
		lbl.append(lblsuffix);
		gTriggerSys.GroupDrawHelper(LabelPos,m_GroupName,lbl);
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////////
//
// party_member_entered_trigger_group
//

party_member_entered_trigger_group::party_member_entered_trigger_group()
	: party_member_within_trigger_group()
{
	m_BoundaryTest = BTEST_ON_EVERY_ENTER ;
}

void party_member_entered_trigger_group::FetchParameterList(Parameter &parameterlist)
{
	// Define the format of the parameter list (for SiegeEditor)
	Parameter::format p0;
	p0.sName			= "Group Name";
	p0.isString			= true;
	p0.isRequired		= true;
	parameterlist.Add( p0 );

	Parameter::format px;

	// Special boundary test restrictions
	px.sName		= "Boundary Check";
	px.isString		= true;
	px.isRequired	= true;
	px.s_default	= ToString(BTEST_ON_EVERY_ENTER);
	px.sValues.push_back(ToString(BTEST_ON_FIRST_ENTER));
	px.sValues.push_back(ToString(BTEST_ON_EVERY_ENTER));

	parameterlist.Add( px );

}

bool party_member_entered_trigger_group::StoreParameterValues(const Params &parms)
{
	gpassert(m_wId == parms.ParamID);
	m_sDoc = parms.Doc;
	m_iSubGroup = parms.SubGroup;
	// Convert the messages set by SiegeEditor to useable values
	if ( !parms.strings.empty() && !parms.strings[0].empty() ) 
	{
		m_GroupName = parms.strings[0];
		if (parms.strings.size()>1)
		{
			FromString(parms.strings[1].c_str(),m_BoundaryTest);
		}
		else
		{
			m_BoundaryTest = BTEST_ON_EVERY_ENTER;
		}
		return true;
	}
	else
	{
		gpwarningf(( "Invalid trigger parameter supplied for party_member_entered_trigger_group" ));
		return false;
	}
}

///////////////////////////////////////////////////////////////////////////////////
//
// party_member_left_trigger_group
//

party_member_left_trigger_group::party_member_left_trigger_group()
	: party_member_within_trigger_group()
{
	m_BoundaryTest = BTEST_ON_EVERY_LEAVE ;
}

void party_member_left_trigger_group::FetchParameterList(Parameter &parameterlist)
{
	// Define the format of the parameter list (for SiegeEditor)
	Parameter::format p0;
	p0.sName			= "Group Name";
	p0.isString			= true;
	p0.isRequired		= true;
	parameterlist.Add( p0 );

	// Special boundary test restrictions
	Parameter::format px;
	px.sName		= "Boundary Check";
	px.isString		= true;
	px.isRequired	= true;
	px.s_default	= ToString(BTEST_ON_EVERY_LEAVE);
	px.sValues.push_back(ToString(BTEST_ON_FIRST_LEAVE));
	px.sValues.push_back(ToString(BTEST_ON_EVERY_LEAVE));

	parameterlist.Add( px );

}

bool party_member_left_trigger_group::StoreParameterValues(const Params &parms)
{
	gpassert(m_wId == parms.ParamID);
	m_sDoc = parms.Doc;
	m_iSubGroup = parms.SubGroup;
	// Convert the messages set by SiegeEditor to useable values
	if ( !parms.strings.empty() && !parms.strings[0].empty() ) 
	{
		m_GroupName = parms.strings[0];
		if (parms.strings.size()>1)
		{
			FromString(parms.strings[1].c_str(),m_BoundaryTest);
		}
		else
		{
			m_BoundaryTest = BTEST_ON_EVERY_LEAVE;
		}
		return true;
	}
	else
	{
		gpwarningf(( "Invalid trigger parameter supplied for party_member_left_trigger_group" ));
		return false;
	}
}

