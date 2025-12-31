/**********************************************************************
 *<
	FILE: PRSExp.cpp

	DESCRIPTION: Dungeon Siege Animation Exporter

	CREATED BY: biddle

	HISTORY: 

	--   Version 4.0 : Ported to gmax/3dsmax 4.2
	==	(AnimWriter.ms)
	--   Version 3.1 : Removed root keys
	--   Version 3.0 : Separated rotation and position keys
	--   Version 2.3 : Added Weapon Tracer position & rotation
	--   Version 2.2 : Note Tracks written before any keys
	--   Version 2.1 : Added ROOT keys
	--   Version 2.0 : converted to Y is UP, Z is FACING FORWARD
	--   Version 1.1 : added support for Rotation Deltas
	--   Version 1.0 : added support for NoteTracks


 *>	Copyright (c) 2002, All Rights Reserved.
 **********************************************************************/

#include <gpcore.h>
#include <gpmath.h>
#include <algorithm>
#include "PRSExp.h"
#include "MAXhelpers.h"
#include <config.h>
#include <vector_3.h>
#include <notetrck.h>
#include "DSmxs.h"
#include <matrix_3x3.h>
#include <decomp.h>

#include <nema_iostructs.h>

PRSExporter::PRSExporter(const gpstring& maxfname, Interface* iface)
	: ExpBase(maxfname,iface)
{
}

PRSExporter::~PRSExporter()
{
}

//-----------------------------------------------------------------
void PRSExporter::BuildBoneKeyLists(void)
{
	std::vector<TimeValue> keytimes;

	for ( TimeValue keytime = m_BegTimeTicks; keytime <= m_EndTimeTicks; keytime += GetTicksPerFrame())
	{
		keytimes.push_back(keytime);
	}

	for (int b = 0; b < m_Bones.size(); ++b)
	{
		m_BoneKeyTimes.push_back(keytimes);
	}
}

//-----------------------------------------------------------------
bool PRSExporter::VerifyRequiredGlobals()
{
	// Make sure that all the globals we access from maxscript are valid

	Value* v;

	// FetchTracers
	bool bad_data = false;

	// Check the weapon tracers...
	Value* ret = RunMaxScriptCommand("dscb_CalculateTracers()");
	v = globals->get(Name::intern("dsglb_tracers"))->eval();
	if ( is_array(v) )
	{
		Array* a = (Array*)v;
		for (int i = 0; i < (*a).size ; i++)
		{
			Value* elem = (*a)[i];
			if ( is_array(elem) )
			{
				Array* sa = (Array*)elem;
				if ((*sa).size == 3)
				{
					Value* timval = (*sa)[0];
					Value* posval = (*sa)[1];
					Value* rotval = (*sa)[2];
					if ( is_time(timval) && is_point3(posval) && is_quat(rotval) )
					{
						TimeValue t = timval->to_timevalue();
						Point3 pos = posval->to_point3();
						Quat rot = rotval->to_quat();
						//gpgenericf(("%8.5f: [%8.5f,%8.5f,%8.5f] (%8.5f,%8.5f,%8.5f,%8.5f)\n",TicksToSec(t),pos.x,pos.y,pos.z,rot.x,rot.y,rot.z,rot.w));
					}
					else
					{
						bad_data = true;
					}
				}
				else
				{
					bad_data = true;
				}
			}
			else
			{
				bad_data = true;
			}
		}
	}

	return bad_data==false;
}


//-----------------------------------------------------------------
bool PRSExporter::FetchTextureFiles(std::vector<gpstring>& tn)
{
	// PRS files DO NOT return any textures. 

	tn.clear();
	return true;
}

//-----------------------------------------------------------------
void PRSExporter::WriteAnimHeaderChunk(FileSys::File& outfile)
{
	nema::sNeMaAnim_Chunk chunk;

	chunk.ChunkName					= REVERSE_FOURCC('ANIM');
	chunk.MajorVersion				= PRS_MAJOR_VERSION;			
	chunk.MinorVersion				= PRS_MINOR_VERSION;			
	chunk.ExtraVersion				= 0;	
	chunk.StringTableSize			= m_StringTableSize;
	chunk.NumberOfKeyLists			= m_BoneKeyTimes.size();
	chunk.Duration					= m_Duration;
	chunk.LinearDisplacementX		= m_PositionDelta.x;
	chunk.LinearDisplacementY		= m_PositionDelta.y;
	chunk.LinearDisplacementZ		= m_PositionDelta.z;
	chunk.AngularDisplacementX		= m_RotationDelta.x;
	chunk.AngularDisplacementY		= m_RotationDelta.y;
	chunk.AngularDisplacementZ		= m_RotationDelta.z;
	chunk.AngularDisplacementW		= m_RotationDelta.w;
	chunk.HalfAngularDisplacementX	= m_RotationMidDelta.x;
	chunk.HalfAngularDisplacementY	= m_RotationMidDelta.y;
	chunk.HalfAngularDisplacementZ	= m_RotationMidDelta.z;
	chunk.HalfAngularDisplacementW	= m_RotationMidDelta.w;
	chunk.LoopAtEnd					= 0;

	outfile.WriteStruct( chunk );
};

//-----------------------------------------------------------------
void PRSExporter::WriteNoteTracks(FileSys::File& outfile)
{

	static Class_ID DefNoteTrackClassID(NOTETRACK_CLASS_ID, 0);

	nema::sAnimNoteList_Chunk chunk;

	chunk.ChunkName		= REVERSE_FOURCC('NOTE');
	chunk.MajorVersion	= PRS_MAJOR_VERSION;
	chunk.MinorVersion  = PRS_MINOR_VERSION;
	chunk.ExtraVersion	= 0;
	chunk.NumberOfNotes	= 0;

	if ( !m_ExportNode->HasNoteTracks() )
	{
		outfile.WriteStruct(chunk);
	}
	else
	{
		int numtracks = m_ExportNode->NumNoteTracks();

		// Make sure that the number of tracks is the expected number?

		//-- Count the number of events we will write out
		
		int totalevents = 0;
		
		for (int trk = 0; trk < numtracks; ++trk)
		{
	        NoteTrack *ntrk = m_ExportNode->GetNoteTrack(trk);

			if(ntrk && (ntrk->ClassID() == DefNoteTrackClassID))
			{
				DefNoteTrack* defntrk = static_cast<DefNoteTrack*>(ntrk);

				const NoteKeyTab& nk = defntrk->keys;
				
				int trackevents = 0;
				for (DWORD k = 0; k < nk.Count(); ++k)
				{
					if (nk[k]->time < m_BegTimeTicks) continue;
					if (nk[k]->time > m_EndTimeTicks) continue;
					if ((k > 0) && (nk[k-1]->time == nk[k]->time)) continue;
					trackevents += 1;
					totalevents += 1;
				}
				//gpgenericf(("Track [%d] has %d events\n",trk,trackevents));
			}

		}

		chunk.NumberOfNotes	= totalevents;
		outfile.WriteStruct(chunk);

		//gpgenericf(("Total events % \n",totalevents));

		int evtcount = 0;

		for (trk = 0; trk < numtracks; ++trk)
		{
	        NoteTrack *ntrk = m_ExportNode->GetNoteTrack(trk);

			if(ntrk && (ntrk->ClassID() == DefNoteTrackClassID))
			{
				DefNoteTrack* defntrk = static_cast<DefNoteTrack*>(ntrk);

				const NoteKeyTab& nk = defntrk->keys;
				for (DWORD k = 0; k < nk.Count(); ++k)
				{
					if (nk[k]->time < m_BegTimeTicks) continue;
					if (nk[k]->time > m_EndTimeTicks) continue;
					if ((k > 0) && (nk[k-1]->time == nk[k]->time)) continue;

					nema::sAnimNote_Chunk noteChunk;
					noteChunk.FourCC = FourCCToDWORD(nk[k]->note);
					noteChunk.Time = TicksToSec(nk[k]->time - m_BegTimeTicks)/m_Duration;

					/*dbgwart
					gpgenericf(("(%8.5f,%s)",noteChunk.Time,stringtool::MakeFourCcString(noteChunk.FourCC,true).c_str()));
					evtcount++;
					if (evtcount<totalevents)
					{
						gpgeneric(",");
					}
					*/

					outfile.WriteStruct(noteChunk);
				}
			}
			//dbgwart gpgeneric("\n");
		}
	}
};

//-----------------------------------------------------------------
void PRSExporter::WriteWeaponTracers(FileSys::File& outfile)
{

	// The tracers were calculated by the call to VerifyMaxscriptGlobals
	// so its safe to access all the data without further testing
	
	//dbgw gpgeneric("TracerDATA\n-------------\n");
	Array* a = (Array*)(globals->get(Name::intern("dsglb_tracers"))->eval());

	if ((*a).size > 0)
	{

		nema::sAnimTracerList_Chunk TracerListChunk;

		TracerListChunk.ChunkName		= REVERSE_FOURCC('TRCR');		
		TracerListChunk.MajorVersion	= PRS_MAJOR_VERSION;
		TracerListChunk.MinorVersion	= PRS_MINOR_VERSION;
		TracerListChunk.ExtraVersion	= 0;		
		TracerListChunk.NumberOfTracers	= (*a).size;

		Array* begTKey = (Array*)((*a)[0]);
		Array* endTKey = (Array*)((*a)[(*a).size-1]);
		
		TracerListChunk.StartTime		= TicksToSec((*begTKey)[0]->to_timevalue());
		TracerListChunk.EndTime			= TicksToSec((*endTKey)[0]->to_timevalue());

		outfile.WriteStruct(TracerListChunk);

		for2 (int i = 0; i < (*a).size ; i++)
		{
			Quat kidrot = (*(Array*)((*a)[i]))[2]->to_quat();
			Quat restrot =   Inverse(m_CurrRootRotation) * kidrot;

			Matrix3 restmat;
			restrot.MakeMatrix(restmat);

			Point3 row0 = restmat.GetRow(0);
			Point3 row1 = restmat.GetRow(1);
			Point3 row2 = restmat.GetRow(2);
			
			outfile.Write(&row0.x,sizeof(float));
			outfile.Write(&row0.y,sizeof(float));
			outfile.Write(&row0.z,sizeof(float));

			outfile.Write(&row1.x,sizeof(float));
			outfile.Write(&row1.y,sizeof(float));
			outfile.Write(&row1.z,sizeof(float));

			outfile.Write(&row2.x,sizeof(float));
			outfile.Write(&row2.y,sizeof(float));
			outfile.Write(&row2.z,sizeof(float));

		}

		for2 (int i = 0; i < (*a).size ; i++)
		{

			Point3 kidpos = (*(Array*)((*a)[i]))[1]->to_point3();
			Point3 restpos = m_CurrRootRotMatrix * (kidpos-m_CurrRootPosition);
	
			restpos *= 1/1000.0f;	// Convert from millimeters

			outfile.Write(&restpos.x,sizeof(float));
			outfile.Write(&restpos.y,sizeof(float));
			outfile.Write(&restpos.z,sizeof(float));

		}

	}
};

//-----------------------------------------------------------------
DWORD WINAPI progbar_dummy_fn(LPVOID arg) {
	return(0);
}

//-----------------------------------------------------------------
DSiegeUtils::eExpErrType PRSExporter::WriteKeyLists(FileSys::File& outfile)
{
	nema::sAnimKeyList_Chunk KeyListChunk;

	KeyListChunk.ChunkName			= REVERSE_FOURCC('KLST');
	KeyListChunk.MajorVersion		= PRS_MAJOR_VERSION;		
	KeyListChunk.MinorVersion		= PRS_MINOR_VERSION;		
	KeyListChunk.ExtraVersion		= 0;		
	
	// Added a progress meter for really large anims
	int totalkeys = 0;
	int completedkeys = 0;
	int completedpercent = 0;

	for2 (int b = 0 ; b < m_BoneKeyTimes.size(); ++b)
	{
		totalkeys += m_BoneKeyTimes[b].size();
	}

	LPVOID arg = 0;
	m_Interface->ProgressStart(_T("Exporting Animation Keys"), TRUE, progbar_dummy_fn, arg);

	for2 (int b = 0 ; b < m_BoneKeyTimes.size(); ++b)
	{	
		KeyListChunk.BoneIndex			= b;		
		KeyListChunk.BoneNameOffset		= m_StringTablePos[b];

		KeyListChunk.NumberOfRotKeys	= 0;
		KeyListChunk.NumberOfPosKeys	= 0;
		
		gpstring rlist_cmd = "dscb_ReduceRotKeyList #(";
		gpstring plist_cmd = "dscb_ReducePosKeyList #(";

		AffineParts ap;
		
		for2 (int k = 0; k < m_BoneKeyTimes[b].size(); ++k)
		{
			TimeValue kt = m_BoneKeyTimes[b][k];
			float ktime = TicksToSec(kt-m_BegTimeTicks)/m_Duration;
				
			DetermineRootAdjustment(kt);

			const ExpBone& bn = m_Bones[b];

			Point3 relrestpos;
			Quat relrestrot;
			
			Point3 tempscale;

			Matrix3 relrestxfrm;

			if (b == 0) 
			{
				Matrix3 kidxfrm = bn.m_INode->GetNodeTM(kt);
				relrestxfrm = kidxfrm * Inverse(m_CurrRootTransform);
				decomp_affine(relrestxfrm, &ap);
				relrestpos = ap.t;
				relrestrot = ap.q.Inverse();
			}
			else
			{

				Matrix3 parxfrm = m_Bones[bn.m_ParentIndex].m_INode->GetNodeTM(kt);

				decomp_affine(parxfrm, &ap);
				Quat parentrot = ap.q.Inverse();

				Matrix3 parentrotmat;
				parentrot.MakeMatrix(parentrotmat);

				Point3 parentpos = parxfrm.GetTrans();

				Matrix3 kidxfrm = bn.m_INode->GetNodeTM(kt);

				decomp_affine(kidxfrm, &ap);
				Quat kidrot = ap.q.Inverse();
		
				Point3 kidpos = kidxfrm.GetTrans();

				relrestpos =  parentrotmat * (kidpos-parentpos);
				relrestrot =  parentrot.Inverse() * kidrot;

			}
			
			relrestpos *= 1 / 1000.0f;	
						
			if (k == 0)
			{
				rlist_cmd.appendf("#(%f,(quat %f %f %f %f))",ktime,relrestrot.x,relrestrot.y,relrestrot.z,relrestrot.w);
				plist_cmd.appendf("#(%f,[%f,%f,%f])",ktime,relrestpos.x,relrestpos.y,relrestpos.z);
			}
			else
			{
				rlist_cmd.appendf(",#(%f,(quat %f %f %f %f))",ktime,relrestrot.x,relrestrot.y,relrestrot.z,relrestrot.w);
				plist_cmd.appendf(",#(%f,[%f,%f,%f])",ktime,relrestpos.x,relrestpos.y,relrestpos.z);
			}
			
			completedkeys++;
			int percent = (completedkeys*100)/totalkeys;
			if (completedpercent != percent)
			{
				completedpercent = percent;
				m_Interface->ProgressUpdate(completedpercent);
			}

			if (m_Interface->GetCancel()) 
			{
				int retval = MessageBox(m_Interface->GetMAXHWnd(), _T("Really Cancel?"),_T("Export Animation"), MB_ICONQUESTION | MB_YESNO);
				if (retval == IDYES)
				{
					m_Interface->ProgressEnd();
					return DSiegeUtils::EXP_ERROR_EXPORT_CANCELLED;
				}
				else 
				{
					m_Interface->SetCancel(FALSE);
				}
			}
		}

		rlist_cmd.append(") 0.001");
		plist_cmd.append(") 0.001");

		// Call the maxscript routines that will reduce the keys

		Value* reducedRotKeyListVal = RunMaxScriptCommand(rlist_cmd.begin_split());

		RotKeyList rotkeys;

		// $$ todo, write a data unpacker for parse values

		bool data_error = false;
		if ( !data_error && is_array(reducedRotKeyListVal))
		{
			Array* rkla = (Array*)reducedRotKeyListVal;
			for2 (int i=0; !data_error && (i<(*rkla).size); ++i)
			{
				Value* elem = (*rkla)[i];
				if ( is_array(elem) ) 
				{
					Array* key = (Array*)elem;
					if ( (*key).size == 2 )
					{
						if ( is_float( (*key)[0] ) && is_quat( (*key)[1] ) )
						{
							rotkeys.push_back(RotKey((*key)[0]->to_float(),(*key)[1]->to_quat()));
						}
						else
						{
							data_error = true;
						}
					}
					else
					{
						data_error = true;
					}
				}
				else
				{
					data_error = true;
				}
			}
		}

		// We don't need the Value anymore
		reducedRotKeyListVal->make_collectable();
		
		Value* reducedPosKeyListVal = RunMaxScriptCommand(plist_cmd.begin_split());

		PosKeyList poskeys;

		if ( !data_error && is_array(reducedPosKeyListVal))
		{
			Array* rkla = (Array*)reducedPosKeyListVal;
			for2 (int i=0; !data_error && (i<(*rkla).size); ++i)
			{
				Value* elem = (*rkla)[i];
				if ( is_array(elem) ) 
				{
					Array* key = (Array*)elem;
					if ( (*key).size == 2 )
					{
						if ( is_float( (*key)[0] ) && is_point3( (*key)[1] ) )
						{
							poskeys.push_back(PosKey((*key)[0]->to_float(),(*key)[1]->to_point3()));
						}
						else
						{
							data_error = true;
						}
					}
					else
					{
						data_error = true;
					}
				}
				else
				{
					data_error = true;
				}
			}
		}

		reducedPosKeyListVal->make_collectable();

		if (data_error)
		{
			// $$$ set the error code
			m_Interface->ProgressEnd();
			return DSiegeUtils::EXP_ERROR_ANIMATION_KEY_REDUCTION_FAILED;
		}
		
		KeyListChunk.NumberOfRotKeys = rotkeys.size();
		KeyListChunk.NumberOfPosKeys = poskeys.size();

		outfile.WriteStruct(KeyListChunk);
		
		for2 ( RotKeyList::iterator it = rotkeys.begin(); it != rotkeys.end(); ++it )
		{
			nema::sAnimRotKey_Chunk AnimRotKeyChunk;
			AnimRotKeyChunk.Time = (*it).time;
			AnimRotKeyChunk.RotX = (*it).rotation.x;
			AnimRotKeyChunk.RotY = (*it).rotation.y;
			AnimRotKeyChunk.RotZ = (*it).rotation.z;
			AnimRotKeyChunk.RotW = (*it).rotation.w;
			outfile.WriteStruct(AnimRotKeyChunk);
		}
		for2 ( PosKeyList::iterator it = poskeys.begin(); it != poskeys.end(); ++it )
		{
			nema::sAnimPosKey_Chunk AnimPosKeyChunk;
			AnimPosKeyChunk.Time = (*it).time;
			AnimPosKeyChunk.PosX = (*it).position.x;
			AnimPosKeyChunk.PosY = (*it).position.y;
			AnimPosKeyChunk.PosZ = (*it).position.z;
			outfile.WriteStruct(AnimPosKeyChunk);
		}

		
	}

	m_Interface->ProgressEnd();
	return DSiegeUtils::EXP_SUCCESS;
};

//-----------------------------------------------------------------
void PRSExporter::WriteEndOfAnim(FileSys::File& outfile)
{
	DWORD fourcc = REVERSE_FOURCC('AEND');
	outfile.Write(&fourcc,sizeof(fourcc));
};

//-----------------------------------------------------------------
DSiegeUtils::eExpErrType 
PRSExporter::ExportINode(INode *node, const gpstring& expfname, FileSys::File& outfile)
{

	if (!VerifyRequiredGlobals())
	{
		return DSiegeUtils::EXP_ERROR_BUSTED_MAXSCRIPT_GLOBALS;
	}

	m_BoneKeyTimes.clear();

	m_ExportNode = node;

	InitializeStringTable();

	DetermineRootAdjustment(0);

//	DetermineStance();

	Interval interv = m_Interface->GetAnimRange();

	m_BegTimeTicks = interv.Start();
	m_EndTimeTicks = interv.End();
	
	m_Duration = TicksToSec(m_EndTimeTicks-m_BegTimeTicks);
	m_SampleInterval = 1.0/GetFrameRate();

	Quat RootBegRot, RootMidRot, RootEndRot;
	Point3 RootBegPos, RootEndPos;
	
	if (m_RootPosNode)
	{
		Matrix3 RootBegTransform = m_RootPosNode->GetNodeTM(m_BegTimeTicks);
		Matrix3 RootEndTransform = m_RootPosNode->GetNodeTM(m_EndTimeTicks);
		Matrix3 RootMidTransform = m_RootPosNode->GetNodeTM((m_BegTimeTicks+m_EndTimeTicks)*0.5f);

		RootBegPos = RootBegTransform.GetTrans();
		RootEndPos = RootEndTransform.GetTrans();

		if (m_RootPosNode != m_ExportNode)
		{
			RootBegRot = Inverse(Quat(RootBegTransform));
			RootMidRot = Inverse(Quat(RootMidTransform));
			RootEndRot = Inverse(Quat(RootEndTransform));
		}
		else
		{
			RootBegRot.Identity();
			RootMidRot.Identity();
			RootEndRot.Identity();
		}

	}

	m_PositionDelta = (RootEndPos - RootBegPos) / 1000.0f;

	m_RotationDelta = RootBegRot.Inverse() * RootEndRot;
	m_RotationMidDelta = RootBegRot.Inverse() * RootMidRot;

	BuildBoneList();

	if ( m_Bones.empty() ) 
	{
		// Print an error message
		/* 
		if (g_Mesh.parent == $bip01) then (
			if g_Verbose then (MessageBox "The SKINMESH object is attached to the BIPO1 and not the BIP01_PELVIS" title:"Bad News")
			format "ERROR: The SKINMESH object is attached to the BIPO1 and not the BIP01_PELVIS\n"
		) else (
			if g_Verbose then (MessageBox "You need at least one bone attached to the object\n\nPerhaps you are missing a 'Dummy Root'?" title:"Bad News")
			format "ERROR: There is no bone attached to the object. Perhaps you are missing a 'Dummy Root'?\n"
		)
		*/
		return DSiegeUtils::EXP_ERROR_NO_BONES;
	}

	BuildBoneStringTable();

	BuildBoneKeyLists();

	WriteAnimHeaderChunk(outfile);

	WriteStringTable(outfile);

	WriteNoteTracks(outfile);

	WriteWeaponTracers(outfile);
	
	DSiegeUtils::eExpErrType retval = WriteKeyLists(outfile);
	if (DSiegeUtils::EXP_SUCCESS != retval)
	{
		return retval;
	}

	WriteEndOfAnim(outfile);
	
	WriteInfoChunk(outfile); 
	
	// Dump();

	return DSiegeUtils::EXP_SUCCESS;
}


void PRSExporter::Dump(void)
{
	int b;

	gpgenericf(("There are %d sorted bones\n",m_Bones.size()));
	
	for (b = 0; b < m_Bones.size(); ++b)
	{
		INode* bnode = m_Bones[b].m_INode;
		gpstring bname = (bnode) ? bnode->GetName() : "<NULL>";
		stringtool::RemoveBorderingWhiteSpace(bname);
		
		int p = m_Bones[b].m_ParentIndex;

		INode* pnode = (p>=0) ? m_Bones[p].m_INode : NULL;
		gpstring pname = (pnode) ? pnode->GetName() : "<NULL>";
		stringtool::RemoveBorderingWhiteSpace(pname);

		gpgenericf(("\tBone [%d:%s] -> Parent [%d:%s]\n",b,bname.c_str(),p,pname.c_str()));
	}

	for (b = 0; b < m_Bones.size(); ++b)
	{
		gpgenericf(( "Bone %d: %d keys (", b, m_BoneKeyTimes[b].size() ));
		for (int k = 0; k < m_BoneKeyTimes[b].size(); ++k)
		{
			gpgenericf(( "%8.5f", TicksToSec(m_BoneKeyTimes[b][k])) );
			if (k < m_BoneKeyTimes[b].size()-1)
			{
				gpgeneric(",");
			}
		}
		gpgeneric(")\n");
	}

}
