#ifndef __VIEWER_H

#include "RapiAppModule.h"
#include "Nema_AspectMgr.h"
#include "FileSys.h"
#include "MasterFileMgr.h"
#include "TankFileMgr.h"
#include "TankMgr.h"

class gpprofiler;
class GAS;
class FuelSys;
class Rapi;
class RapiOwner;
class RapiFont;
class FuelDB;
class Muffler;
class NamingKey;


namespace siege {
	class SiegeEngine;
	class Console;
}

namespace nema {
	class AspectStorage;
	class PRSKeysStorage;
}

namespace FileSys {
	class IFileMgr;
}

namespace FuBi {
	class SysExports;
}

namespace Skrit {
	class Engine;
}

class Viewer : public RapiAppModule, public Singleton <Viewer>
{
public:
	SET_INHERITED( Viewer, RapiAppModule );

	Viewer( void );
	~Viewer( void );

private:

	SET_NO_COPYING( Viewer );

	virtual bool OnInitCore( void );
	virtual bool OnExecute ( void );
	virtual bool OnPreTranslateMessage( MSG& msg );
	virtual bool OnMessage( HWND hwnd, UINT msg, WPARAM wparam,	LPARAM lparam, LRESULT& returnValue );

	virtual bool OnShutdown ( void );

	virtual bool OnFrame ( float deltaTime, float actualDeltaTime );

	virtual bool OnMouseFrameDelta( int deltaX, int deltaY );

	virtual bool OnRButtonDown    ( int /*x*/, int /*y*/, UINT /*keyFlags*/ );
	virtual bool OnRButtonUp      ( int /*x*/, int /*y*/, UINT /*keyFlags*/ );
	virtual bool OnLButtonDown    ( int /*x*/, int /*y*/, UINT /*keyFlags*/ );
	virtual bool OnLButtonUp      ( int /*x*/, int /*y*/, UINT /*keyFlags*/ );
	virtual bool OnMButtonDown    ( int /*x*/, int /*y*/, UINT /*keyFlags*/ );
	virtual bool OnMButtonUp      ( int /*x*/, int /*y*/, UINT /*keyFlags*/ );
	virtual bool OnMouseWheel     ( int /*x*/, int /*y*/, int /*z*/, UINT /*keyFlags*/ );
	
	virtual bool OnKeyDown		( UINT vkey, int repeatCount, UINT flags );
	virtual bool OnKeyUp		( UINT vkey, int repeatCount, UINT flags );

	void SetWindowTitle();

	bool PrepMesh(nema::HAspect& asp, gpstring namebuff, gpstring meshname, gpstring Title);
	bool PrepAnim(nema::HAspect& asp, gpstring namebuff, gpstring animname, gpstring Title);
	bool PrepFloor(bool preload);

	bool ParseQuickViewArg(gpstring sdata);

	bool LoadData(void);

	gpprofiler *m_GpProfiler;

	std::auto_ptr< TankMgr				> m_TankMgr;

	std::auto_ptr< FuBi::SysExports		> m_FuBiSysExports;

	std::auto_ptr< FuelSys				> m_FuelSys;
	std::auto_ptr< Skrit::Engine		> m_SkritEngine;		
	std::auto_ptr< NamingKey			> m_NamingKey;
	std::auto_ptr< nema::AspectStorage	> m_NemaAspectStorage;
	std::auto_ptr< nema::PRSKeysStorage	> m_NemaKeyMgr;

	bool m_PreviewLoad;
	bool m_QuickViewMesh;
	bool m_QuickViewAnim;
	bool m_UsingLocalData;

	gpstring m_OutDir;

	gpstring MeshPathBuff;
	gpstring AnimPathBuff;
	gpstring TexturePathBuff;

	gpstring Mesh0FName;
	gpstring Mesh1FName;
	gpstring Mesh3FName;
	gpstring MeshHeadFName;
	gpstring MeshBodyFName;
	gpstring MeshFootFName;
	gpstring MeshHandFName;

	gpstring AnimFName;

	gpstring FloorFName;

	nema::HAspect Aspect0;
	nema::HAspect Aspect1;
	nema::HAspect AspectHead;
	nema::HAspect AspectBody;
	nema::HAspect AspectFoot;
	nema::HAspect AspectHand;
	nema::HAspect Aspect3;

	vector_3	m_WorldPosition;
	Quat		m_WorldRotation;

	vector_3	m_LightPosition;
	vector_3	m_LightDirection;
	bool		m_DrawLightRayFlag;
	bool		m_DrawShadowsFlag;
	float		m_LightAngle;
	bool		m_SpinLightsFlag;

	float		m_TimeScale;

	bool		m_Jogging;
	bool		m_WireFlag;

	float		m_JogAngle;
	float		m_JogRadius;

	bool		m_NoFloorFlag;
	DWORD		m_FloorTexID[1];
	DWORD		m_NumFloorTex;
	DWORD		m_FloorTileFactor;

	bool		m_NoTexFlag;

	bool		m_DaVinciMode;

	bool		m_IgnoreMouseFlag;
	bool		m_FollowAspect0Flag;

	bool		m_RenderNormalsFlag;
	bool		m_RenderBonesFlag;
	DWORD		m_RenderSettings;

	bool		m_PausedFlag;
	bool		m_DrawRootFlag;

	DWORD		m_EnvironmentTex;

	gpstring	m_HelpFile;

};

#define gViewer Singleton <Viewer>::GetSingleton()

//////////////////////////////////////////////////////////////////////////////

#endif  // __VIEWER_H

//////////////////////////////////////////////////////////////////////////////

