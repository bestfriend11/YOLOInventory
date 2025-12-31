#include "precomp_world.h"
/*==================================================================================================

	MCP_Pipeline -- MCP support code

	purpose:	Provides support for converting PLAN information located on
				the server to the FOLLOWER on each client

	author:

  (c)opyright Gas Powered Games, 2000

--------------------------------------------------------------------------------------------------*/

#include "GoCore.h"
#include "Go.h"
#include "GoBody.h"	
#include "GoFollower.h"
#include "GoInventory.h"

#include "MCP.h"

#include "worldtime.h"		// $$ for debugging

#include "goaspect.h"		// $$ for more debugging
#include "nema_aspect.h"	// $$ for more debugging

#include "world.h"			// $$ Still more debugging

#include "FubiBitPacker.h"

#include "siege_engine.h"

using namespace MCP;

////////////////////////////////////////////////////////////////////////////////////
void Plan::SSendPackedUpdateToFollowers( double forward_time ) 
{
	CHECK_SERVER_ONLY;

	if ( !m_Follower || m_Sequencer.Empty() )
	{
		// This is a static object (like a barrel)
		return;
	}

	bool initialsend = (m_Sequencer.GetLastTimeSent() == DBL_MAX);
	
	bool all_transmitted = m_Sequencer.IsAllTransmitted();
	if ( !all_transmitted )
	{
		// Transmit all timeline node that are <= forward time
		bool send_mess = !initialsend && (m_Sequencer.GetLastReqBlock() == (eRID)m_CurrentReqBlock);

		std::vector<TimeNode> toSend;

		bool near_end = m_Sequencer.CollectTimeNodesToSend(GetGo(m_Owner), forward_time, toSend );

		if (near_end && send_mess )
		{
			WorldMessage msg( WE_MCP_SECTION_COMPLETE_WARNING, GOID_INVALID, m_Owner , m_CurrentReqBlock );
			msg.SendDelayed( 0 );
		}

		if (!toSend.empty())
		{
			std::vector<TimeNode>::iterator beg = toSend.begin();
			std::vector<TimeNode>::iterator end = toSend.end();
			std::vector<TimeNode>::iterator it = beg;

			if (gServer.IsMultiPlayer())
			{

				Go* pgo = m_Follower->GetGo();
				
				DWORD clientFrustums = (DWORD)gGoDb.GetActiveFrustumMask() & ~(DWORD)gGoDb.GetScreenPlayerFrustumMask();
				DWORD currInClients = (DWORD)pgo->GetWorldFrustumMembership() & clientFrustums;

				if ( pgo->IsAllClients() )
				{				

					// We need to so some fancy stuff to maintain the position of all-clients-gos
					// When we detect that they are about to move to a different node, we pop the 
					// go to the new position
					SiegePos waypos = m_Sequencer.GetWayPosNodeTransmitted();

					gpassert( gSiegeEngine.IsNodeValid( waypos.node ) );
					const siege::SiegeNodeHandle lhandle(gSiegeEngine.NodeCache().UseObject(waypos.node));
					const siege::SiegeNode& lnode = lhandle.RequestObject( gSiegeEngine.NodeCache() );

					// Determine which clients will get their bits set
					DWORD nextInClients = lnode.GetFrustumOwnedBitfield() & clientFrustums;
					
					DWORD newMembership = ~currInClients & nextInClients;
					if (newMembership != 0)
					{
						// If we have new membership bits, then we force an update
						// of the synch time on all clients
						m_ClientSynchTime = 0;
					}

					currInClients |= nextInClients;

					DWORD missingClients = ~currInClients & clientFrustums;

					if ( missingClients )
					{
						// Some all clients don't have this AllClientsGo in their frustum before or
						// after the move. We need to maintain the correct node.
						FuBi::BitPacker packer;
						packer.XferFloat(waypos.pos.x);
						packer.XferFloat(waypos.pos.y);
						packer.XferFloat(waypos.pos.z);
						packer.XferRaw(waypos.node);
						m_Follower->SSendPackedPositionUpdateToFollowers( packer, missingClients );
					}
				}

				if (currInClients)
				{
					FuBi::BitPacker packer;
					DWORD num_to_send = toSend.size();
					packer.XferCount(num_to_send);
					while (it!=end)
					{
						// Send to Server
						m_Follower->DecodeNode(*it);
						// Pack for Clients
						(*it).XferForSync(packer,m_ClientSynchTime);	
						++it;
					}
					m_Follower->SSendPackedUpdateToFollowers( packer, currInClients );
				}
			}

			// Send to the server itself
			while (it!=end)
			{
				m_Follower->DecodeNode(*it);
				++it;
			}
			

#if !GP_RETAIL
			gpstring buff;	
			buff.assignf("MCP-OUT [%s:g:%08x] ",
				m_Follower->GetGo()->GetTemplateName(),
				m_Follower->GetGo()->GetGoid()
				);
			it = beg;
			while (it!=end)
			{
				(*it).Crack(it-beg,buff);
				++it;
			}
			buff.appendf("\n");
			gpdevreportf( &gMCPNetLogContext, (buff.c_str()));
#endif
			
		}

		all_transmitted = m_Sequencer.IsAllTransmitted();
	}

	if (all_transmitted && m_Sequencer.CheckForCompletion(forward_time))
	{
		bool send_mess = !initialsend && ((m_Sequencer.GetLastReqBlock() == (eRID)m_CurrentReqBlock));
		if (send_mess)
		{
			// SECTION COMPLETED messages are sent with a delay so that they are recieved 
			// by the AI when the event is actually happening in the world
			double delay = m_Sequencer.GetFinalTime();
			if (delay != DBL_MAX)
			{
				delay = max_t(0.0, PreciseSubtract(delay, gWorldTime.GetTime()));
			}
			else
			{
				delay = 0.0f;
			}
			WorldMessage msg( WE_MCP_SECTION_COMPLETED, GOID_INVALID, m_Owner , m_CurrentReqBlock );
			msg.SendDelayed( (float)delay );
		}
	}

}

/////////////////////////////////////////////////////////////
void Plan::SSendPackedClipToFollowers(double clip_time, SiegePos clip_pos)
{

	CHECK_SERVER_ONLY;

	if (!m_Follower)
	{
		// This is a static object (like a barrel)
		return;
	}

	m_Sequencer.UpdateAfterClipIsSent(clip_time);

	m_Follower->ClipAllQueues(clip_time,clip_pos);

	if (gServer.IsMultiPlayer())
	{
		FuBi::BitPacker packer;
		packer.XferRaw(clip_time);
		packer.XferFloat(clip_pos.pos.x);
		packer.XferFloat(clip_pos.pos.y);
		packer.XferFloat(clip_pos.pos.z);
		packer.XferRaw(clip_pos.node);
		m_Follower->SSendPackedClipToFollowers( packer );
	}

#if !GP_RETAIL
	gpstring buff;	
	buff.assignf("MCPCLIP-OUT [%s:g:%08x] %f [%f,%f,%f,%08x]\n",
		m_Follower->GetGo()->GetTemplateName(),
		m_Follower->GetGo()->GetGoid(),
		clip_time,
		clip_pos.pos.x,
		clip_pos.pos.y,
		clip_pos.pos.z,
		clip_pos.node
		);
	gpdevreportf( &gMCPNetLogContext, (buff.c_str()));
#endif

}

/////////////////////////////////////////////////////////////
bool Plan::NeedsRefresh( )
{
	if (!gServer.IsMultiPlayer())
	{
		return false;
	}
	if (!m_Follower || m_Follower->GetGo()->GetAspect()->GetAspectPtr()->GetNextChore() == CHORE_NONE)
	{
		// You can't refresh something that hasn't been animated to begin with....
		return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////
void Plan::BuildPackedRefresh( FuBi::BitPacker& packer )
{

	CHECK_SERVER_ONLY;

	m_Sequencer.PackRefresh( packer, m_Follower, m_ClientSynchTime );
	
}
