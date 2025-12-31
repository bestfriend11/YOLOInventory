#pragma once
/**********************************************************************
 *<
	FILE: PRSExp.cpp

	DESCRIPTION: Animation Exporter

	CREATED BY: biddle

	-- PRS REVISIONS:
	--   Version 4.0 : Ported to gmax/3dsmax 4.2
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

#ifndef _PRSEXP_H
#define _PRSEXP_H

#define PRS_MAJOR_VERSION 4
#define PRS_MINOR_VERSION 0

#include "ExpBase.h"

//========================================================================
class PRSExporter : public ExpBase
{
	
public:

	PRSExporter(const gpstring& maxfname, Interface* iface);
	~PRSExporter();

	bool FetchTextureFiles(std::vector<gpstring>& filenames);

	DSiegeUtils::eExpErrType ExportINode(INode *node, const gpstring& expfname, FileSys::File& outfile);
	
private:

	TimeValue m_BegTimeTicks;
	TimeValue m_EndTimeTicks;
	
	Point3 m_PositionDelta;
	Quat m_RotationDelta;
	Quat m_RotationMidDelta;

	float m_Duration;
	float m_SampleInterval;

	std::vector< std::vector<TimeValue> > m_BoneKeyTimes;

	bool VerifyRequiredGlobals();
	
	void BuildBoneKeyLists(void);

	void WriteAnimHeaderChunk(FileSys::File& outfile);
	void WriteNoteTracks(FileSys::File& outfile);
	void WriteWeaponTracers(FileSys::File& outfile);
	void WriteAnimRootKeys(FileSys::File& outfile);
	DSiegeUtils::eExpErrType  WriteKeyLists(FileSys::File& outfile);
	void WriteEndOfAnim(FileSys::File& outfile);

	void Dump(void);
	
};

#endif
	
