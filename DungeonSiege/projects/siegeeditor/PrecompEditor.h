//////////////////////////////////////////////////////////////////////////////
//
// File     :  PrecompEditor.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the headers that will be precompiled for this project.
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __PRECOMPEDITOR_H
#define __PRECOMPEDITOR_H

//////////////////////////////////////////////////////////////////////////////

// clear these
#ifdef WINVER
#undef WINVER
#endif
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif

// we're forcing windows sdk v5 headers
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500

#pragma warning ( push, 1 )		// ignore any warnings or changes that win32 has
#include "stdafx.h"
#pragma warning ( pop )			// back to normal

#include "gpcore.h"

#include "SiegeEditorTypes.h"

#include <commctrl.h>


#include "siege_engine.h"
#include "siege_console.h"
#include "siege_database.h"
#include "siege_database_guid.h"
#include "siege_node.h"
#include "siege_logical_node.h"
#include "siege_logical_mesh.h"
#include "siege_mesh.h"
#include "siege_mouse_shadow.h"
#include "siege_walker.h"
#include "siege_frustum.h"

#include "FuBi.h"
#include "FuBiSchemaImpl.h"

#include "fuel.h"
#include "world.h"
#include "Go.h"
#include "GoAspect.h"
#include "GoBody.h"
#include "Godb.h"
#include "GoMind.h"
#include "GoDefs.h"
#include "ContentDb.h"
#include "GoCore.h"

#include "stringtool.h"

#include "SiegeEditorShell.h"
#include "GizmoManager.h"
#include "ComponentList.h"


// add other fun stuff here to speed up build...

//////////////////////////////////////////////////////////////////////////////

#endif  // __PRECOMPEDITOR_H

//////////////////////////////////////////////////////////////////////////////
