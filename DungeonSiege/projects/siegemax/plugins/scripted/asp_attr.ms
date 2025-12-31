----------------------------------------------------------------------------------------
--
-- Scripted Custom Attributes for ASPECTS
--
----------------------------------------------------------------------------------------

----------------------------------------------------------------------------------------
ASPdef = attributes DSiegeASP
attribID:#(0x6d33b5d2, 0x888aa7c1)
version:1000
( 

	local tHeadBits = #{} 
	local tHandBits = #{}
	local tFeetBits = #()
	
	local STENCIL_ALPHA_BIT		=  1
	local DISABLE_LIGHTING_BIT	=  2
	local VERTEX_ALPHA_BIT		= 16
	
	parameters ASPDATA
	(
		RenderFlags			type:#integer	animatable:false	
		HeadFaceList		type:#intTab	animatable:false tabsizevariable:true	
		HandFaceList		type:#intTab	animatable:false tabsizevariable:true
		FeetFaceList		type:#intTab	animatable:false tabsizevariable:true
		StitchVertList		type:#intTab	animatable:false tabsizevariable:true
		StitchTagList		type:#stringTab	animatable:false tabsizevariable:true
	)
		
	fn RebuildInternals =
	(
		tHeadBits = #{}
		for i in HeadFaceList do append tHeadBits i
		
		tHandBits = #{}
		for i in HandFaceList do append tHandBits i
		
		tFeetBits = #{}
		for i in FeetFaceList do append tFeetBits i						
	)
		
	on create do 
	(		
		tHeadBits = #{}
		tHandBits = #{}
		tFeetBits = #{}
	
		RenderFlags = 0
			
		HeadFaceList = #()			
		HandFaceList = #()		
		FeetFaceList = #()
		StitchTagList = #()
		StitchVertList = #()
	)
		
	on update do 
	(	
		format "Updating ASPDEF version %\n" version
		RebuildInternals()
	)
	
	on load do 
	(
		RebuildInternals()
	)
	
) 

