//////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------
// File     :  IgnoredWarnings.h
// Author(s):  <Original Author Unknown>, Scott Bilas
//
// Summary  :  This collects ignored warnings together in one place. Lots of
//             VC++ warnings are ok to ignore for our purposes.
//
// Copyright © 1999 Gas Powered Games.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

// note: no pragma once here so that warnings will properly reset if this file
//       is #included twice

#ifndef __IGNOREDWARNINGS_H
#define __IGNOREDWARNINGS_H

//////////////////////////////////////////////////////////////////////////////
// disabled warnings

#pragma warning ( disable : 4097 ) // typedef-name 'x' used as synonym for class-name 'y'
#pragma warning ( disable : 4201 ) // nonstandard extension used : nameless struct/union
#pragma warning ( disable : 4211 ) // nonstandard extension used : redefined extern to static
#pragma warning ( disable : 4290 ) // C++ exception specification ignored
#pragma warning ( disable : 4503 ) // decorated name length exceeded, name was truncated
#pragma warning ( disable : 4511 ) // 'x' : copy constructor could not be generated
#pragma warning ( disable : 4512 ) // 'x' : assignment operator could not be generated
#pragma warning ( disable : 4514 ) // 'x' : unreferenced inline function has been removed
#pragma warning ( disable : 4530 ) // warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify -GX
#pragma warning ( disable : 4786 ) // debug symbol truncated to 255 characters
#pragma warning ( disable : 4710 ) // The given function was selected for inline expansion, but the compiler did not perform the inlining.
#pragma warning ( disable : 4711 ) // The compiler performed inlining on the given function, although it was not marked for inlining by the user.
#pragma warning ( disable : 4268 ) // 'identifier' : 'const' static/global data initialized with compiler generated default constructor fills the object with zeros
#pragma warning ( disable : 4725 ) // instruction may be inaccurate on some Pentiums (anomaly of the Pentium FDIV bug that we don't care about)


//////////////////////////////////////////////////////////////////////////////

#endif // __IGNOREDWARNINGS_H

//////////////////////////////////////////////////////////////////////////////
