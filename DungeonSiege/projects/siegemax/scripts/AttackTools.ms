GLOBAL dsglb_reference_equipment = #(
	               -- Farmboy       Farmgirl           Dwarf              Skeleton			Goblin			 GoblinMech
	#("1h_melee", [ 1.0, 1.0, 1.0 ], [ 0.8, 0.8, 0.8 ], [ 0.9, 0.9, 0.9 ], [ 0.9, 0.9, 0.9 ], [ 1.0, 1.0, 1.0], [ 1.0, 1.0, 1.0]),
	#("2h_melee", [ 1.0, 1.0, 1.0 ], [ 0.8, 0.8, 0.8 ], [ 0.9, 0.9, 0.8 ], [ 1.0, 1.0, 1.0 ], [ 1.0, 1.0, 1.0], [ 1.0, 1.0, 1.0]),
	#("2h_sword", [ 1.0, 1.0, 1.0 ], [ 0.8, 0.8, 0.8 ], [ 1.0, 1.0, 0.8 ], [ 0.9, 0.9, 0.9 ], [ 1.0, 1.0, 1.0], [ 1.0, 1.0, 1.0]), 
	#("staff",    [ 1.0, 1.0, 1.0 ], [ 1.0, 1.0, 1.0 ], [ 1.0, 1.0, 0.8 ], [ 1.0, 1.0, 1.0 ], [ 1.0, 1.0, 1.0], [ 1.0, 1.0, 1.0]), 
	#("bow",      [ 1.0, 1.0, 1.0 ], [ 0.8, 0.8, 0.8 ], [ 0.8, 0.8, 0.8 ], [ 1.0, 1.0, 1.0 ], [ 1.0, 1.0, 1.0], [ 1.0, 1.0, 1.0]), 
	#("minigun",  [ 1.0, 1.0, 1.0 ], [ 0.8, 0.8, 0.8 ], [ 0.8, 0.8, 0.8 ], [ 0.9, 0.9, 0.9 ], [ 0.6, 0.6, 0.6], [ .75, .75, .75]), 	
	#("shield",   [ 1.0, 1.0, 1.0 ], [ 0.9, 0.9, 0.9 ], [ 0.8, 1.0, 0.8 ], [ 0.9, 0.9, 0.9 ], [ 1.0, 1.0, 1.0], [ 1.0, 1.0, 1.0]), 
	#("arrow",    [ 1.0, 1.0, 1.0 ], [ 0.8, 0.8, 0.8 ], [ 0.8, 0.8, 0.8 ], [ 1.0, 1.0, 1.0 ], [ 1.0, 1.0, 1.0], [ 1.0, 1.0, 1.0])) 

GLOBAL dsglb_reference_equipment_parts = #(	"grip" )
	
GLOBAL dsglb_tracers = #()
	
----------------------------------------------------------------------------------------------------
fn DetermineScale n w = (

	
	try (
	
		local nm = dsLowerCase n.name
		
		if (findstring nm "skinmesh" == 1) then
		(
			nm =  dsLowerCase (getfilenamefile maxfilename)
		)
	
		if (findstring nm "gah_fb"  != undefined) then return reference_equipment[w][2]
		if (findstring nm "gah_fg"  != undefined) then return reference_equipment[w][3]
		if (findstring nm "gan_df"  != undefined) then return reference_equipment[w][4]
		if (findstring nm "ecm_sk"  != undefined) then return reference_equipment[w][5]
		if (findstring nm "eam_ggt" != undefined) then return reference_equipment[w][6]
		if (findstring nm "edm_go" != undefined) then return reference_equipment[w][7]
		
	) catch ()
	
	return [1,1,1]
	
)
	
----------------------------------------------------------------------------------------------------
fn DetermineStance n = (

	try (
	
		local nm = dsLowerCase (getfilenamefile maxfilename)
		
		if (findstring nm "a_" != 1) then return undefined
		
		if (findstring nm "_fs0_" != undefined) then return 0
		if (findstring nm "_fs1_" != undefined) then return 1
		if (findstring nm "_fs2_" != undefined) then return 2
		if (findstring nm "_fs3_" != undefined) then return 3
		if (findstring nm "_fs4_" != undefined) then return 4
		if (findstring nm "_fs5_" != undefined) then return 5
		if (findstring nm "_fs6_" != undefined) then return 6
		if (findstring nm "_fs7_" != undefined) then return 7
		if (findstring nm "_fs8_" != undefined) then return 8
		
	)
	
	catch ()
	
	return undefined
	
)
	
----------------------------------------------------------------------------------------------------
fn Calculate_Weapon_Extreme show_it = (

	if ($weapon_grip == undefined) then return [0,0,0]
	
	currentweapon = $weapon_grip
	while currentweapon.children[1] != undefined do (currentweapon = currentweapon.children[1])
	
	extreme_t = 0
	min_val = 1000
	for t = animationrange.start to animationrange.end by 0.05 do at time t (
		diff = currentweapon.min
		if diff.y <  min_val then (
			min_val = diff.y
			extreme_t = t
		)
	)
	
	if (show_it) then (
		if $__Weapon_Extrema_Snapshot != undefined then delete $__Weapon_Extrema_Snapshot
		if $__Weapon_Extrema_Marker   != undefined then delete $__Weapon_Extrema_Marker
		at time extreme_t extrema_snapshot = snapshot currentweapon
		extrema_marker = sphere radius:50 pos:[0,extrema_snapshot.min.y,extrema_snapshot.center.z ]
		extrema_snapshot.name = "__Weapon_Extrema_Snapshot"
		extrema_marker.name   = "__Weapon_Extrema_Marker"
		extrema_marker.wirecolor = [255,0,0]
	)
	
	return min_val
)

----------------------------------------------------------------------------------------------------
fn FetchRightHandGrip = (

	if ($Bip01_R_Hand != undefined) then
	(
		for kid in $Bip01_R_Hand.children do (
			if (findstring kid.name "_grip" != undefined) then (
				return kid
			)
		)
	)
	else
	(
		return $weapon_grip
	)
)

----------------------------------------------------------------------------------------------------
fn myFreezeTree n = (
	for c in n.children do (
		myFreezeTree c
	)
	freeze n
)
	

----------------------------------------------------------------------------------------------------
fn AttachToRightHand i = (

	ig = FetchGrip i
	hg = FetchRightHandGrip() 
	
	if ig == undefined or hg == undefined then (
		return false
	)

	-- undo on
	in coordsys hg ig.rotation = eulerangles 0 0 0
	in coordsys hg ig.pos = [ 0, 0, 0 ]
	
	ig.parent = hg
	
	myFreezeTree ig
)

----------------------------------------------------------------------------------------------------
fn FetchLeftHandGrip = (
	
	if ($Bip01_L_Hand != undefined) then
	(
		for kid in $Bip01_L_Hand.children do (
			if (findstring kid.name "_grip" != undefined) then (
				return kid
			)
		)
	)
	else
	(
		return $shield_grip
	)
)

----------------------------------------------------------------------------------------------------
fn AttachToLeftHand i = (

	ig = FetchGrip i
	hg = FetchLeftHandGrip() 
	
	if ig == undefined or hg == undefined then (
		return false
	)

	-- undo on
	in coordsys hg ig.rotation = eulerangles 0 0 0
	in coordsys hg ig.pos = [ 0, 0, 0 ]
	
	ig.parent = hg
	
	myFreezeTree ig
	
)

		
		
----------------------------------------------------------------------------------------------------
fn MergeReferenceEquip eq = 
(
	lst = #("reference_"+dsglb_reference_equipment[eq][1])
	
	for p in dsglb_reference_equipment_parts do 
	(
		append lst (p +"_reference_"+dsglb_reference_equipment[eq][1])
	)
	
	mergeMaxFile "Reference_Scenes\GPG_RefEquip.gmax" lst #deleteOldDups
	
	n = execute ("$'"+("reference_"+dsglb_reference_equipment[eq][1])+"'")
	return n 
)

----------------------------------------------------------------------------------------------------
fn EquipOneBareHand n = 
(
	-- MessageBox "One bare hand"
)

----------------------------------------------------------------------------------------------------
fn EquipTwoBareHands n = 
(
	-- MessageBox "Two bare hands"
)

----------------------------------------------------------------------------------------------------
fn EquipOneHandedMelee n = 
(
	-- add 1h melee
	i = MergeReferenceEquip 1
	i.scale = DetermineScale n 1
	AttachToRightHand i
)

----------------------------------------------------------------------------------------------------
fn EquipTwoHandedMelee n = 
(
	i = MergeReferenceEquip 2
	i.scale = DetermineScale n 2
	AttachToLeftHand i
)

----------------------------------------------------------------------------------------------------
fn EquipTwoHandedSword n = 
(
	i = MergeReferenceEquip 3
	i.scale = DetermineScale n 3
	AttachToRightHand i
)

----------------------------------------------------------------------------------------------------
fn EquipStaff n = 
(
	i = MergeReferenceEquip 4
	i.scale = DetermineScale n 4
	AttachToRightHand i
)

----------------------------------------------------------------------------------------------------
fn EquipBow n = 
(
	i = MergeReferenceEquip 5
	i.scale = DetermineScale n 5
	AttachToLeftHand i
	i = MergeReferenceEquip 8
	i.scale = DetermineScale n 8
	AttachToRightHand i
)

----------------------------------------------------------------------------------------------------
fn EquipMinigun n = 
(
	i = MergeReferenceEquip 6
	i.scale = DetermineScale n 6
	AttachToRightHand i
)

----------------------------------------------------------------------------------------------------
fn EquipShield n = 
(
	i = MergeReferenceEquip 7
	i.scale = DetermineScale n 7
	AttachToLeftHand i
)

----------------------------------------------------------------------------------------------------
fn EquipWeaponUsingStance n verbose = (

	local Stance = DetermineStance n
	
	case Stance of (
		0: EquipTwoBareHands n
		1: EquipOneHandedMelee n
		2: EquipOneHandedMelee n
		3: EquipTwoHandedMelee n
		4: EquipTwoHandedSword n
		5: EquipStaff n
		6: EquipBow n
		7: EquipMinigun n
		8: EquipOneBareHand n
		undefined: 
		(
			if verbose then (
				msg = "Weapon not allowed in undefined stance"
				MessageBox msg
			)
		)
		default: 
		(
			if verbose then (
				msg = "Weapon not allowed in stance [FS" + (Stance as string) + "]"
				MessageBox msg
			)
		)
	)
	
)
		
----------------------------------------------------------------------------------------------------
fn EquipShieldUsingStance n verbose = (

	local Stance = DetermineStance n
	
	case Stance of (
		2: EquipShield n
		8: EquipShield n
		undefined: 
		(
			if verbose then (
				msg = "Shield not allowed in undefined stance"
				MessageBox msg
			)
		)
		default: (
			if verbose then (
				msg = "Shield not allowed in stance [FS" + (Stance as string) + "]"
				MessageBox msg
			)
		)
	)
	
)
	
----------------------------------------------------------------------------------------------------
fn myDeleteKids n = (
	for c in n.children do (
		myDeleteKids c
		delete c
	)
)
	
----------------------------------------------------------------------------------------------------
fn UnEquipEverything verbose = (

	if ($weapon_grip != undefined) then myDeleteKids $weapon_grip
	if ($shield_grip != undefined) then myDeleteKids $shield_grip
)

----------------------------------------------------------------------------------------------------
fn EquipCallback = (
	try (
	
		if (UnEquipEverything == undefined) then return false
		if (EquipWeaponUsingStance == undefined) then return false
		if (EquipShieldUsingStance == undefined) then return false
		if (DetermineStance == undefined) then return false

		local msh
		
		if ($skinmesh == undefined) and ($selection.count != 1) then return false

		if ($skinmesh != undefined) then 
		(
			msh = $skinmesh
		)
		else
		(
			msh = $selection[1]
		)			
		
		if ((DetermineStance msh) == undefined) then return false
		
		UnEquipEverything false 
		EquipWeaponUsingStance msh false 
		EquipShieldUsingStance msh false

	) catch ()
	
	return true
)

----------------------------------------------------------------------------------------------------
fn dscb_CalculateTracers = (

	dsglb_tracers = #()
	
	local msh
	
	if ($skinmesh == undefined) and ($selection.count != 1) then return 0

	if ($skinmesh != undefined) then 
	(
		msh = $skinmesh
	)
	else
	(
		msh = $selection[1]
	)
			
	local Stance = DetermineStance msh
	
	grip = case Stance of (
		1: FetchRightHandGrip()		-- EquipOneHandedMelee 
		2: FetchRightHandGrip()		-- EquipOneHandedMelee 
		3: FetchLeftHandGrip() 		-- EquipTwoHandedMelee 
		4: FetchRightHandGrip()		-- EquipTwoHandedSword 
		5: FetchRightHandGrip()		-- EquipStaff 
	)
	
	if (grip == undefined) then return 0
	
	i = (finditem dsglb_CriticalEventFourCC "BSWG")
	if i == 0 then return  0
	if (i > numnotetracks msh) then return 0
	nt = getNoteTrack msh i
	if (nt == undefined) then return 0
	if (nt.keys.count == 0) then return 0
	blurOn = nt.keys[1].time
	
	i = (finditem dsglb_CriticalEventFourCC "ESWG")
	if i == 0 then return 0
	if (i > numnotetracks msh) then return 0
	nt = getNoteTrack msh i
	if (nt == undefined) then return 0
	if (nt.keys.count == 0) then return 0
	blurOff = nt.keys[ nt.keys.count].time
	
	fr = framerate
	framerate = 240
	tk = for t = blurOn to blurOff collect at time t 
	(
		#(t,grip.pos,grip.rotation)
	)
	framerate = fr
	
	dsglb_tracers = tk
	
	return dsglb_tracers.count
)

