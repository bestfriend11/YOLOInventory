#pragma once
#ifndef _SIEGE_OPTIONS_
#define _SIEGE_OPTIONS_

/*******************************************************************************
**
**		SiegeOptions class
**
**		Maintains all toggleable options for Siege and its brethren.
**
**		Author:		James Loe
**		Date:		10/14/99
**
*******************************************************************************/

namespace siege
{
	class SiegeOptions
	{
	private:

		bool			m_bFloorsFlag;
		bool			m_bWaterFlag;
		bool			m_bWireFrameFlag;		
		bool			m_bFPSFlag;
		bool			m_bPolyStatsFlag;
		bool			m_bNodeBoxes;
		bool			m_bCullEffects;
		bool			m_bDynamicLightingFlag;
		bool			m_bStaticLightingFlag;
		bool			m_bDynamicShadows;
		bool			m_bStaticShadows;
		bool			m_bDynamicShading;
		bool			m_bStaticShading;
		bool			m_bDrawDoors;
		bool			m_bSpaceTree;
		bool			m_bDrawNormals;
		bool			m_bDrawLightRays;
		bool			m_bSelectAllObjects;
		bool			m_bShowBones;
		bool			m_bExpensiveShadows;
		bool			m_bExpensivePartyShadows;
		bool			m_bDrawObjects;
		bool			m_bDrawLogicalNodes;
		bool			m_bCollisionBoxes;
		bool			m_bDrawFadedNodes;
		bool			m_bDiscoveryFog;
		bool			m_bObjectFading;
		bool			m_bDecals;
		bool			m_bShadowSpheres;
		bool			m_bErrorReporting;

	public:

		SiegeOptions()
			: m_bFloorsFlag( false )
			, m_bWaterFlag( false )
			, m_bWireFrameFlag( false )		
			, m_bPolyStatsFlag( false )
			, m_bNodeBoxes( false )
			, m_bCullEffects( true )
			, m_bDynamicLightingFlag( true )
			, m_bStaticLightingFlag( true )
#if GP_DEBUG
			, m_bFPSFlag( true )
			, m_bDynamicShadows( false )
			, m_bStaticShadows( false )
#else
			, m_bFPSFlag( false )
			, m_bDynamicShadows( true )
			, m_bStaticShadows( true )
#endif
			, m_bDynamicShading( true )
			, m_bStaticShading( true )
			, m_bDrawDoors( false )
			, m_bSpaceTree( false )
			, m_bDrawNormals( false )
			, m_bDrawLightRays( false )
			, m_bSelectAllObjects( false )
			, m_bShowBones( false )
			, m_bExpensiveShadows( false )
			, m_bExpensivePartyShadows( false )
			, m_bDrawObjects( true )
			, m_bDrawLogicalNodes( false )
			, m_bCollisionBoxes( false )
			, m_bDrawFadedNodes( false )
			, m_bDiscoveryFog( true )
			, m_bObjectFading( true )
			, m_bDecals( true )
			, m_bShadowSpheres( false )
			, m_bErrorReporting( true )
		{}

		void ToggleFloors()					{ m_bFloorsFlag			= !m_bFloorsFlag; }
		bool DrawFloors()					{ return m_bFloorsFlag; }

		void ToggleWater()					{ m_bWaterFlag			= !m_bWaterFlag; }
		bool DrawWater()					{ return m_bWaterFlag; }

		void ToggleWireframe()				{ m_bWireFrameFlag		= !m_bWireFrameFlag; }
		void SetWireframeEnabled(bool set = true)	{ m_bWireFrameFlag = set; }
		bool IsWireframeEnabled()			{ return m_bWireFrameFlag; }
		
		void ToggleFPS()					{ m_bFPSFlag			= !m_bFPSFlag; }
		void SetShowFPS(bool set = true)	{ m_bFPSFlag			= set; }
		bool ShowFPSEnabled()				{ return m_bFPSFlag; }

		void TogglePolyStats()				{ m_bPolyStatsFlag		= !m_bPolyStatsFlag; }
		bool ShowPolyStatsEnabled()			{ return m_bPolyStatsFlag; }

		void ToggleEffectsCulling()			{ m_bCullEffects		= !m_bCullEffects; }
		bool GetEffectsCulling()			{ return m_bCullEffects; }
		
		void ToggleNodeBoxes()				{ m_bNodeBoxes			= !m_bNodeBoxes; }
		bool NodeBoxes()					{ return m_bNodeBoxes; }

		void ToggleStaticLighting()			{ m_bStaticLightingFlag	= !m_bStaticLightingFlag; }
		bool IsStaticLightingEnabled()		{ return m_bStaticLightingFlag; }

		void ToggleDynamicLighting()		{ m_bDynamicLightingFlag= !m_bDynamicLightingFlag; }
		bool IsDynamicLightingEnabled()		{ return m_bDynamicLightingFlag; }

		void ToggleStaticShadows()			{ m_bStaticShadows		= !m_bStaticShadows; }
		bool IsStaticShadowsEnabled()		{ return m_bStaticShadows; }

		void ToggleDynamicShadows()			{ m_bDynamicShadows		= !m_bDynamicShadows; }
		bool IsDynamicShadowsEnabled()		{ return m_bDynamicShadows; }

		void ToggleExpensiveShadows()		{ m_bExpensiveShadows	= !m_bExpensiveShadows; }
		bool IsExpensiveShadows()			{ return m_bExpensiveShadows; }
		bool UseExpensiveShadows()			{ return m_bExpensiveShadows && gDefaultRapi.IsMultiTexBlend(); }

		void ToggleExpensivePartyShadows()	{ m_bExpensivePartyShadows	= !m_bExpensivePartyShadows; }
		bool IsExpensivePartyShadows()		{ return m_bExpensivePartyShadows; }
		bool UseExpensivePartyShadows()		{ return m_bExpensivePartyShadows && gDefaultRapi.IsMultiTexBlend(); }

		void ToggleStaticShading()			{ m_bStaticShading		= !m_bStaticShading; }
		bool IsStaticShadingEnabled()		{ return m_bStaticShading; }

		void ToggleDynamicShading()			{ m_bDynamicShading		= !m_bDynamicShading; }
		bool IsDynamicShadingEnabled()		{ return m_bDynamicShading; }

		void ToggleDoorDrawing()			{ m_bDrawDoors			= !m_bDrawDoors; }
		bool IsDrawDoorsEnabled()			{ return m_bDrawDoors; }

		void ToggleSpaceTree()				{ m_bSpaceTree			= !m_bSpaceTree; }
		bool IsSpaceTreeEnabled()			{ return m_bSpaceTree; }

		void ToggleNormalDrawing()			{ m_bDrawNormals		= !m_bDrawNormals; }
		bool IsNormalDrawingEnabled()		{ return m_bDrawNormals; }

		void ToggleLightRaysDrawing()		{ m_bDrawLightRays		= !m_bDrawLightRays; }
		bool IsLightRaysDrawingEnabled()	{ return m_bDrawLightRays; }

		void ToggleSelectAllObjects()		{ m_bSelectAllObjects	= !m_bSelectAllObjects; }
		bool SelectAllObjects()				{ return m_bSelectAllObjects; }

		void ToggleShowBones()				{ m_bShowBones			= !m_bShowBones; }
		bool ShowBones()					{ return m_bShowBones; }

		void ToggleDrawObjects()			{ m_bDrawObjects		= !m_bDrawObjects; }
		bool DrawObjects()					{ return m_bDrawObjects; }

		void ToggleDrawLogicalNodes()		{ m_bDrawLogicalNodes	= !m_bDrawLogicalNodes; }
		bool IsDrawLogicalNodes()			{ return m_bDrawLogicalNodes; }

		void ToggleCollisionBoxes()			{ m_bCollisionBoxes		= !m_bCollisionBoxes; }
		bool IsCollisionBoxes()				{ return m_bCollisionBoxes; }

		void ToggleFadedNodeDrawing()		{ m_bDrawFadedNodes		= !m_bDrawFadedNodes; }
		bool DrawFadedNodes()				{ return m_bDrawFadedNodes; }

		void ToggleDiscoveryFog()			{ m_bDiscoveryFog		= !m_bDiscoveryFog; }
		void SetDrawDiscoveryFog( bool set = true )	{ m_bDiscoveryFog = set; }
		bool DrawDiscoveryFog()				{ return m_bDiscoveryFog; }

		void ToggleObjectFading()			{ m_bObjectFading		= !m_bObjectFading; }
		bool IsObjectFading()				{ return m_bObjectFading; }

		void ToggleDecals()					{ m_bDecals				= !m_bDecals; }
		bool DrawDecals()					{ return m_bDecals; }

		void ToggleShadowSpheres()			{ m_bShadowSpheres		= !m_bShadowSpheres; }
		bool ShadowSpheres()				{ return m_bShadowSpheres; }

		void ToggleErrorReporting()			{ m_bErrorReporting		= !m_bErrorReporting; }
		bool ErrorReporting()				{ return m_bErrorReporting; }
	};
}


#endif
