----------------------------------------------------------------------------------------
--
-- Scripted Custom Attributes for Siege Nodes
--
----------------------------------------------------------------------------------------

snodef = attributes DSiegeSNO
attribID:#(0x5145da2b, 0x986e2657)
version:1000
( 

	parameters SNODATA
	(
		DoorEdgeList		type:#intTab	animatable:false tabsizevariable:true
		DoorLastEdges		type:#intTab	animatable:false tabsizevariable:true
		DoorVertList		type:#intTab	animatable:false tabsizevariable:true
		DoorLastVerts		type:#intTab	animatable:false tabsizevariable:true	
		DoorDirectionList	type:#point3Tab	animatable:false tabsizevariable:true

		FloorFaceList		type:#intTab	animatable:false tabsizevariable:true
		
		WaterFaceList		type:#intTab	animatable:false tabsizevariable:true
		
		IgnoredFaceList		type:#intTab	animatable:false tabsizevariable:true
		
		LockedNormVertList	type:#intTab	animatable:false tabsizevariable:true

		DoNotRoundVertList	type:#intTab	animatable:false tabsizevariable:true	
	)
	
	local tDoorList = #()
	local tFloorBits = #{} 
	local tWaterBits = #{}
	local tIgnoredBits = #()
	local tLockedNormBits = #()
	local tDoNotRoundBits = #()
	local tSelectedDoor = undefined

	fn RebuildInternals =
	(
		tFloorBits = #{}
		for ffl in FloorFaceList do append tFloorBits ffl
		
		tWaterBits = #{}
		for wfl in WaterFaceList do append tWaterBits wfl
		
		tIgnoredBits = #{}
		for ifl in IgnoredFaceList do append tIgnoredBits ifl
		
		tLockedNormBits = #{}
		for iln in LockedNormVertList do append tIgnoredBits iln 

		tDoNotRoundBits = #{}
		for drn in DoNotRoundVertList do append tDoNotRoundBits drn 
		
		tDoorList = #()

		for i = 1 to DoorLastEdges.count do
		(
			local vmin = if i > 1 then (DoorLastVerts[i-1]+1) else 1
			local vmax = DoorLastVerts[i]
			local doorverts = for v = vmin to vmax collect DoorVertList[v]
			
			local emin = if i > 1 then (DoorLastEdges[i-1]+1) else 1
			local emax = DoorLastEdges[i]
			local dooredges = for e = emin to emax collect DoorEdgeList[e]
			
			append tDoorList (SiegeDoor doorverts dooredges)
		)
				
	)

	on create do 
	(		
		tFloorBits = #{}
		tWaterBits = #{}
		tIgnoredBits = #{}
		tLockedNormBits = #{}
   		tDoNotRoundBits = #()
		
		tDoorList = #()
		tSelectedDoor = undefined
		
		DoorEdgeList = #()
		DoorLastEdges = #()
		DoorVertList = #()
		DoorLastVerts = #()
		
		DoorDirectionList = #()
		
		FloorFaceList = #()			
		WaterFaceList = #()		
		IgnoredFaceList = #()
		LockedNormVertList = #()	
		DoNotRoundVertList = #()
	)
		
	on update do 
	(	
		format "Updating SNODEF version %\n" version
		RebuildInternals()
	)
	
	on load do 
	(
		print "Loading SNO"
		RebuildInternals()
	)
	
) 

