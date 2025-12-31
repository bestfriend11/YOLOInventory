
#pragma once
#ifndef __MCPDEFS_H
#define __MCPDEFS_H

namespace MCP {

	struct Position;
	struct Packet;
	struct Request;
	class Plan;
	class Manager;

	enum eOrientMode;
	enum eRID;
	enum eRequest;
	enum eReqRetCode;

};


#define gMCP      MCP::Manager::GetSingleton()


//////////////////////////////////////////////////////////////////////////////
// report streams

#if !GP_RETAIL

#endif // !GP_RETAIL

#endif  // __MCPDEFS_H
