fn RangeROOTtoTopBone picked =
(
	local RootBoneNode = dsFetchTopmostBone picked 
	
	if ($ROOT == undefined) or (RootBoneNode == undefined) then
	(
		return false
	)
	
	animate on (
	
		at time animationrange.start (
			if (classof RootBoneNode.controller) == Link_Control then
			(
				local lt = dsGetLinkControlledParentAtTime RootBoneNode animationrange.start
				if (lt != undefined and lt != false) then
				(
					$ROOT.position.x = lt.pos.x
					$ROOT.position.y = lt.pos.y
				)
			-- Look for a biped	controller (if CStudio is around...)
			--) else if (dsValidBipedBody RootBoneNode) then (
			--	$ROOT.position.x = RootBoneNode.controller.horizontal.translation.x
			--	$ROOT.position.y = RootBoneNode.controller.horizontal.translation.y
			--) else if ((RootBoneNode.parent != undefined) and (dsValidBipedBody RootBoneNode.parent)) then (
			--	$ROOT.position.x = (RootBoneNode.parent).controller.horizontal.translation.x
			--	$ROOT.position.y = (RootBoneNode.parent).controller.horizontal.translation.y
			) 
			else
			(
				$ROOT.position.x = RootBoneNode.pos.x
				$ROOT.position.y = RootBoneNode.pos.y
			)
		)
		
		at time animationrange.end (
			if (classof RootBoneNode.controller) == Link_Control then
			(
				local lt = dsGetLinkControlledParentAtTime RootBoneNode animationrange.end
				if (lt != undefined and lt != false) then
				(
					$ROOT.position.x = lt.pos.x
					$ROOT.position.y = lt.pos.y
				)
			)
			-- Look for a biped	controller (if CStudio is around...)
			-- else if (doaValidBody RootBoneNode) then
			-- (
			-- 	$ROOT.position.x = RootBoneNode.controller.horizontal.translation.x
			-- 	$ROOT.position.y = RootBoneNode.controller.horizontal.translation.y
			-- )
			-- else if ((RootBoneNode.parent != undefined) and (doaValidBody RootBoneNode.parent)) then 
			-- (
			-- 	$ROOT.position.x = (RootBoneNode.parent).controller.horizontal.translation.x
			-- 	$ROOT.position.y = (RootBoneNode.parent).controller.horizontal.translation.y
			-- )
			else
			(
				$ROOT.position.x = RootBoneNode.pos.x
				$ROOT.position.y = RootBoneNode.pos.y
			)
		)
	
	)
	
	return true
)

-------------------------------------------------------------------------
fn ResetRootPosMarker =
(
	try
	(	
		if $ROOT != undefined then 
		(
			delete $ROOT
		)
		
		newroot = LoadReferenceObject "__ROOT"
		
		if (newroot != undefined) then
		(
			newroot.name = "ROOT"
			newroot.rotation.controller = linear_rotation()
			newroot.position.controller = linear_position()
			newroot.scale.controller = linear_scale()
		)
		
	)	
	catch ()
)
	
-------------------------------------------------------------------------
fn AddFirstLastRootPosMarkerKeys picked = 
(
	try
	(
		RangeROOTtoTopBone picked
	)
	catch ()
)
