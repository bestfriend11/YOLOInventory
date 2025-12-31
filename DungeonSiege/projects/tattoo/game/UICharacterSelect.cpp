/**************************************************************************

Filename		: UICharacterSelect.cpp

Class			: UICharacterSelect.h

Description		: Contains the classes necessary to display a rotatable
				  3D character within the frontend

Creation Date	: 1/17/99

**************************************************************************/



// Include Files
#include "precomp_game.h"

#include "GoDb.h"
#include "ContentDb.h"
#include "GoCore.h"
#include "GoData.h"
#include "GoAspect.h"
#include "GoBody.h"
#include "CameraAgent.h"
#include "Nema_AspectMgr.h"
#include "Siege_Mouse_Shadow.h"
#include "StringTool.h"
#include "UICharacterSelect.h"
#include "UIFrontend.h"
#include "World.h"
#include "UI_EditBox.h"
#include "siege_options.h"
#include "Player.h"
#include "Server.h"
#include "fuel.h"
#include "stringtool.h"
#include "config.h"
#include "WorldState.h"
#include "RapiPrimitive.h"
#include "nema_blender.h"
#include "nema_kevents.h"
#include "nema_chore.h"
#include "GoInventory.h"
#include "RapiImage.h"
#include "AppModule.h"
#include "TattooGame.h"


static const char * s_SkinSets[] =
{
	"pants",
	"shirts",
	"faces",
	"hair",
	"heads",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_SkinSets ) == SkinSet::eSkin::SKIN_COUNT );


/**************************************************************************

Function		: UICharacterSelect()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UICharacterSelect::UICharacterSelect( bool bMultiPlayer )
{
	m_DisplayActor = GOID_INVALID;

	m_bIsMulti				= bMultiPlayer;
	m_bInitialized			= false;		
	m_GoodActorIndex		= 0;	
}


/**************************************************************************

Function		: ~UICharacterSelect()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UICharacterSelect::~UICharacterSelect()
{
}

void UICharacterSelect::Initialize()
{
	if ( !m_bInitialized && m_GoodActors.size() != 0 )
	{
		GoidColl::iterator iMember;
		for ( iMember = m_GoodActors.begin(); iMember != m_GoodActors.end(); ++iMember )
		{
			GoHandle hActor( *iMember );
			if ( hActor.IsValid() == false )
			{
				return;
			}
			//hActor->GetInventory()->RSUnequip( ES_WEAPON_HAND, AO_USER );
		}

		m_GoodActorIndex = 0;
		m_SelectedActor = 0;

		SetActiveCharacter();

		PrepLights();

		FastFuelHandle hCharacterCamera( "ui:config:character_camera:character_camera" );
		if ( hCharacterCamera.IsValid() )
		{
			gpstring sTemp;
			hCharacterCamera.Get( "target_pos", sTemp );
			stringtool::GetDelimitedValue( sTemp, ',', 0, m_Camera.targetPos.pos.x );
			stringtool::GetDelimitedValue( sTemp, ',', 1, m_Camera.targetPos.pos.y );
			stringtool::GetDelimitedValue( sTemp, ',', 2, m_Camera.targetPos.pos.z );

			hCharacterCamera.Get( "distance", m_Camera.distance );
			hCharacterCamera.Get( "orbit", m_Camera.orbitAngle );
			hCharacterCamera.Get( "azimuth", m_Camera.azimuthAngle );
		}
		else
		{
			m_Camera.targetPos.pos	= vector_3( 0.0f, 0.85f, 0.0f );
			m_Camera.distance		= 7.38f;
			m_Camera.orbitAngle		= 0.12f;
			m_Camera.azimuthAngle	= 0.24799f;
		}

		// construct character skin component sets
		FastFuelHandle fhSkins( "world:global:skins" );
		gpassertm( fhSkins, "where are the skins?" );
		if ( fhSkins )
		{
			for (DWORD skintype = 0; skintype < SkinSet::eSkin::SKIN_COUNT; ++skintype)
			{
				FastFuelHandle fhSkinType = fhSkins.GetChildNamed( s_SkinSets[skintype] );

				// Collect the common items for this skin type
				FastFuelHandle fhCommon = fhSkinType.GetChildNamed( "common" );

				stdx::fast_vector<gpstring> commonlist;

				if (fhCommon)
				{
					FastFuelFindHandle iter(fhCommon);
					if ( iter.FindFirstKeyAndValue() )
					{
						gpstring dummy, value;
						while ( iter.GetNextKeyAndValue( dummy, value ) )
						{
							commonlist.push_back(value);
						}
					}
				}

				stdx::fast_vector<gpstring>::iterator cit;
				stdx::fast_vector<gpstring>::iterator cbeg = commonlist.begin();
				stdx::fast_vector<gpstring>::iterator cend = commonlist.end();

				// Collect all the specific group

				FastFuelHandleColl fhlGroups;
				FastFuelHandleColl::iterator g;
				fhSkinType.ListChildren( fhlGroups );


				for ( g = fhlGroups.begin(); g != fhlGroups.end(); ++g )
				{
					gpstring group = (*g).GetName();

					if ( !group.same_no_case("common") )
					{

						// Assume we don't what to include the common items
						cit = cend;

						// Only fetch (and create) SkinSet if needed
						SkinSet * pSkinSet = NULL;

						FastFuelFindHandle iter(*g);
						if ( iter.FindFirstKeyAndValue() )
						{
							gpstring dummy, value;
							while ( iter.GetNextKeyAndValue( dummy, value ) )
							{
								if ( value.same_no_case("include_common") )
								{
									// Include the common items
									cit = cbeg;
								}
								else
								{
									if (!pSkinSet)
									{
										pSkinSet = GetSkinSet( (SkinSet::eSkin)skintype, group );
									}
									pSkinSet->AddTexture(value);
								}
							}
						}

						// Include the common items if requested
						for (  ; cit != cend ; ++cit )
						{
							if (!pSkinSet)
							{
								pSkinSet = GetSkinSet( (SkinSet::eSkin)skintype, group );
							}
							pSkinSet->AddTexture(*cit);
						}

					}
				}
			}

			// Build the head/hair maps
			m_hairToHeads.clear();

			FastFuelHandle hHairHeads = fhSkins.GetChildNamed( "hairtoheads" );
			if ( hHairHeads.IsValid() )
			{
				FastFuelHandleColl templates;
				FastFuelHandleColl::iterator iTemplate;
				hHairHeads.ListChildren( templates, 1 );
				for ( iTemplate = templates.begin(); iTemplate != templates.end(); ++iTemplate )
				{
					TemplateHairHead hth;
					hth.sTemplate = (*iTemplate).GetName();

					FastFuelFindHandle hFind( (*iTemplate) );
					if ( hFind.FindFirstKeyAndValue() )
					{
						gpstring sHair, sHeads;
						while ( hFind.GetNextKeyAndValue( sHair, sHeads ) )
						{
							StringVec heads;
							int numHeads = stringtool::GetNumDelimitedStrings( sHeads, ',' );
							for ( int iHead = 0; iHead != numHeads; ++iHead )
							{
								gpstring sHead;
								stringtool::GetDelimitedValue( sHeads, ',', iHead, sHead );
								heads.push_back( sHead );
							}

							hth.hairToHeads.insert( std::make_pair( sHair, heads ) );
						}
					}

					m_hairToHeads.push_back( hth );
				}
			}
		}

		SkinSetColl::iterator i;
		for ( i = m_Skins.begin(); i != m_Skins.end(); ++i )
		{
			(*i).SelectDefaultTexture();
		}

		LoadCustomSelections();

		UpdateTextures();

		m_bInitialized = true;
	}
}


void UICharacterSelect::DeleteSelectionGos()
{
	GoidColl::iterator i;
	for ( i = m_GoodActors.begin(); i != m_GoodActors.end(); ++i )
	{
		gGoDb.RSMarkForDeletion( *i );
	}		
	
	m_GoodActors.clear();
}


/**************************************************************************

Function		: Update()

Purpose			: Updates the current scene

Returns			: void

**************************************************************************/
void UICharacterSelect::Update( double delta_time )
{
	if ( !m_bInitialized )
	{
		Initialize();
	}

	if ( m_bInitialized )
	{		
		GoHandle hActor;

		if ( m_DisplayActor != GOID_INVALID )
		{
			hActor = m_DisplayActor;
		}
		else
		{
			hActor = m_GoodActors[ m_GoodActorIndex ];
		}

		if ( !hActor.IsValid() )
		{
			return;
		}

		nema::HAspect hasp = hActor->GetAspect()->GetAspectHandle();

		if ( hasp.IsValid() && ( delta_time > 0.0f ))
		{
			hasp->UpdateAnimationStateMachine( (float)delta_time);
		}

		GoidColl::iterator iMember;
		for ( iMember = m_GoodActors.begin(); iMember != m_GoodActors.end(); ++iMember )
		{
			GoHandle hActor( *iMember );
			if ( hActor.IsValid() == false || !hActor->GetInventory()->GetEquipped( ES_WEAPON_HAND ) )
			{
				continue;
			}
			hActor->GetInventory()->RSUnequip( ES_WEAPON_HAND, AO_USER );
		}
	}
}

/**************************************************************************

Function		: Draw()

Purpose			: Draws the current scene

Returns			: void

**************************************************************************/
void UICharacterSelect::Draw()
{
	if ( m_bInitialized )
	{
		GoHandle hActor;

		if ( m_DisplayActor != GOID_INVALID )
		{
			hActor = m_DisplayActor;
		}
		else
		{
			hActor = m_GoodActors[ m_GoodActorIndex ];
		}

		if ( !hActor.IsValid() )
		{
			return;
		}

		nema::HAspect hasp = hActor->GetAspect()->GetAspectHandle();

		if ( !hasp.IsValid() ) {
			return;
		}

		// Set up viewport
		hasp->Deform();

		if ( gWorldState.GetCurrentState() == WS_MP_STAGING_AREA_SERVER || gWorldState.GetCurrentState() == WS_MP_STAGING_AREA_CLIENT )
		{
			SetTemporaryView();
			gSiegeEngine.Renderer().SetWorldMatrix( matrix_3x3() , vector_3::ZERO );
		}

		bool notexflag = false;
		gSiegeEngine.Renderer().SetTextureStageState(	0,
										notexflag ? D3DTOP_DISABLE : D3DTOP_MODULATE,
										notexflag ? D3DTOP_DISABLE : D3DTOP_SELECTARG1,
										D3DTADDRESS_CLAMP,
										D3DTADDRESS_CLAMP,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DTEXF_LINEAR,
										D3DBLEND_SRCALPHA,
										D3DBLEND_INVSRCALPHA,
										false );

		hasp->InitializeLighting( 0xFF808080, 255 );

		nema::LightSource nls;
		nls.m_Type = nema::NLS_INFINITE_LIGHT;
		nls.m_Color = m_SunLight.m_Descriptor.m_Color;
		nls.m_Intensity = m_SunLight.m_Descriptor.m_Intensity;
		nls.m_Direction = Normalize(m_SunLight.m_Direction);

		hasp->CalculateShading(  nls, gSiegeEngine.GetOptions().IsLightRaysDrawingEnabled() );

		if ( gWorldState.GetCurrentState() == WS_SP_CHARACTER_SELECT ||
			 gWorldState.GetCurrentState() == WS_SP_MAP_SELECT ||
			 gWorldState.GetCurrentState() == WS_SP_DIFFICULTY_SELECT ||
			 gWorldState.GetCurrentState() == WS_SP_MAIN_MENU )
		{
			gSiegeEngine.Renderer().SetWorldMatrix( matrix_3x3(), gUIFrontend.GetCharacterPos() );
			gSiegeEngine.Renderer().ScaleWorldMatrix( vector_3( 0.95f, 0.95f, 0.95f ) );
		}
		gSiegeEngine.Renderer().RotateWorldMatrix( m_rotateMatrix );
		hasp->Render( &gSiegeEngine.Renderer() );

		gWorld.SetViewportWorld();
	}
}


// Renders and snaps portrait, copying portrait data into the given memory image
void UICharacterSelect::GeneratePortrait()
{
	GoHandle hActor;

	if ( m_DisplayActor != GOID_INVALID )
	{
		hActor = m_DisplayActor;
	}
	else
	{
		hActor = m_GoodActors[ m_GoodActorIndex ];
	}

	if ( !hActor.IsValid() )
	{
		return;
	}

	nema::HAspect hasp = hActor->GetAspect()->GetAspectHandle();

	if ( !hasp.IsValid() )
	{
		return;
	}

	// Set up viewport
	hasp->Deform();

	vector_3 head_pos;
	Quat head_rotation;

	// Get the head bone
	hasp->GetBoneOrientation( "bip01_head", head_pos, head_rotation );

	// Setup all matrices to reflect proper head facing orientation
	matrix_3x3 mat = Transpose( head_rotation.BuildMatrix() );

	vector_3 viewer_location = vector_3( 0.5f, 3.0f, -0.5f );
	vector_3 target_pos		 = vector_3( 0.1f, 0.0f, 0.0f );
	vector_3 up_vector		 = vector_3( 1.0f, 0.0f, 0.0f );

	float orthoMatrix = 0.006f;

	gpstring sCamera;
	sCamera.assignf( "ui:config:character_camera:portrait_camera:%s", hActor->GetTemplateName() );
	FastFuelHandle hPortraitCam( sCamera.c_str() );
	if ( hPortraitCam.IsValid() )
	{
		gpstring sTemp;
		hPortraitCam.Get( "viewer_location_offset", sTemp );
		stringtool::GetDelimitedValue( sTemp, ',', 0, viewer_location.x );
		stringtool::GetDelimitedValue( sTemp, ',', 1, viewer_location.y );
		stringtool::GetDelimitedValue( sTemp, ',', 2, viewer_location.z );

		hPortraitCam.Get( "target_position_offset", sTemp );
		stringtool::GetDelimitedValue( sTemp, ',', 0, target_pos.x );
		stringtool::GetDelimitedValue( sTemp, ',', 1, target_pos.y );
		stringtool::GetDelimitedValue( sTemp, ',', 2, target_pos.z );

		hPortraitCam.Get( "up_vector", sTemp );
		stringtool::GetDelimitedValue( sTemp, ',', 0, up_vector.x );
		stringtool::GetDelimitedValue( sTemp, ',', 1, up_vector.y );
		stringtool::GetDelimitedValue( sTemp, ',', 2, up_vector.z );

		hPortraitCam.Get( "ortho_matrix", orthoMatrix );
	}

	gSiegeEngine.Renderer().SetWorldMatrix( mat, mat * -head_pos );
	gSiegeEngine.Renderer().SetOrthoMatrix( orthoMatrix );

	gSiegeEngine.Renderer().SetViewMatrix( viewer_location,	// Offset to the viewer location
										   target_pos,		// Target position offset
										   up_vector );		// Up vector

	PrepAnim();
	hasp->ForceDeformation();
	hasp->Deform(0,0);

	hasp->InitializeLighting( 0xFF808080, 255 );

	nema::LightSource nls;
	nls.m_Type = nema::NLS_INFINITE_LIGHT;
	nls.m_Color = m_SunLight.m_Descriptor.m_Color;
	nls.m_Intensity = m_SunLight.m_Descriptor.m_Intensity;
	nls.m_Direction = Normalize(m_SunLight.m_Direction);

	hasp->CalculateShading(  nls, gSiegeEngine.GetOptions().IsLightRaysDrawingEnabled() );
	hasp->Render( &gSiegeEngine.Renderer() );

	// $$ Helper Code - I'll remove this when I'm sure the portrait shot is fine and dandy
	/*
	// Generate portrait

	RECT rect;
	rect.left	= 380;
	rect.top	= 277;
	rect.right	= 444;
	rect.bottom	= 341;

	RP_DrawEmptyRect( gSiegeEngine.Renderer(), rect.left, rect.top, rect.right, rect.bottom, 0xff00ff00 );

	rect.left	= 380;
	rect.top	= 277;
	rect.right	= 419;
	rect.bottom	= 323;

	RP_DrawEmptyRect( gSiegeEngine.Renderer(), rect.left, rect.top, rect.right, rect.bottom, 0xff0000ff );
	*/
}


void UICharacterSelect::RotateY( float y )
{
	Quat quat( m_rotateMatrix );
	quat.RotateY( y );
	m_rotateMatrix = quat.BuildMatrix();
}


/**************************************************************************

Function		: PrepAnim()

Purpose			: Prepare an animation to be played

Returns			: bool

**************************************************************************/
bool UICharacterSelect::PrepAnim()
{
	GoHandle hActor;

	if ( m_DisplayActor != GOID_INVALID )
	{
		hActor = m_DisplayActor;
	}
	else
	{
		hActor = m_GoodActors[ m_GoodActorIndex ];
	}

	if ( !hActor.IsValid() )
	{
		return false;
	}

	nema::HAspect hasp = hActor->GetAspect()->GetAspectHandle();

	if ( hasp.IsValid() )
	{
		hasp->SetNextChore(CHORE_MISC, AS_PLAIN, hasp->GetBlender()->GetSubAnimIndex( CHORE_MISC, 'frtd' ) );
		hasp->ResetAnimationCompletionCallback();
		hasp->UpdateAnimationStateMachine( 0.0f );
		return true;
	}

	return false;
}



void UICharacterSelect::UpdateTextures()
{
	GoHandle go( m_GoodActors[ m_GoodActorIndex ] );

	gpstring clothTexture = GetClothTexture();
	if ( !clothTexture.empty() )
	{
		go->GetAspect()->SetDynamicTexture( PS_CLOTH, clothTexture );
	}

	gpstring fleshTexture = GetFleshTexture();
	if ( !fleshTexture.empty() )
	{
		go->GetAspect()->SetDynamicTexture( PS_FLESH, fleshTexture );
	}

	gpstring headMesh = GetHeadMesh();
	if ( !headMesh.empty() )
	{
		gpassert(go->HasInventory());
		go->GetInventory()->SetCustomHead( headMesh );
	}
}



/**************************************************************************

Function		: PrepLights()

Purpose			: Initializes the lights within the scene

Returns			: Void

**************************************************************************/
void UICharacterSelect::PrepLights()
{
	m_SunLight.m_Position				= vector_3(0,4,4);
	m_SunLight.m_Direction				= vector_3(0,0.7071f,0.7071f);
	m_SunLight.m_Descriptor.m_Type		= siege::LT_DIRECTIONAL;
	m_SunLight.m_Descriptor.m_bActive	= true;
	m_SunLight.m_Descriptor.m_Color		= 0xFFFFFFFF;
	m_SunLight.m_Descriptor.m_Intensity	= 1.0f;
	m_SunLight.m_Descriptor.m_Subtype	= siege::LS_DYNAMIC;
}



/**************************************************************************

Function		: SetActiveCharacter()

Purpose			: Sets the active selected character using a fuel handle

Returns			: Void

**************************************************************************/
void UICharacterSelect::SetActiveCharacter()
{
	if ( m_GoodActors.size() != 0 )
	{
		PrepAnim();
		UpdateTextures();
	}
}


/**************************************************************************

Function		: SetNextActiveCharacter()

Purpose			: Sets the next active character to select

Returns			: Void

**************************************************************************/
void UICharacterSelect::SetNextActiveCharacter()
{
	m_GoodActorIndex++;
	if (m_GoodActorIndex >= m_GoodActors.size()) {
		m_GoodActorIndex = 0;
	}

	SetActiveCharacter();
}


void UICharacterSelect::SetPreviousActiveCharacter()
{
	if (m_GoodActorIndex == 0) {
		m_GoodActorIndex = m_GoodActors.size();
	}
	m_GoodActorIndex--;

	SetActiveCharacter();
}


void UICharacterSelect::SetSelectedCharacter( const gpstring & actorName )
{
	for ( unsigned int findChar = 0; findChar < m_GoodActors.size(); ++findChar )
	{
		if ( actorName.same_no_case( GetGo( m_GoodActors[findChar] )->GetTemplateName() ) )
		{
			m_GoodActorIndex = findChar;
			break;
		}
	}
	SetActiveCharacter();
}


/**************************************************************************

Function		: GetActiveCharacterName()

Purpose			: Gets the active character's name

Returns			: gpstring

**************************************************************************/
gpwstring	UICharacterSelect::GetActiveCharacterName()
{
	return GoHandle(m_GoodActors[m_GoodActorIndex])->GetCommon()->GetScreenName();
}

// Setup a temporary viewport
void UICharacterSelect::SetTemporaryView()
{
		// Build the camera position using the angles
	vector_3 cameraPos;

	float d;
	SINCOSF( m_Camera.azimuthAngle, cameraPos.y, d );
	SINCOSF( m_Camera.orbitAngle, cameraPos.x, cameraPos.z );

	d			*= m_Camera.distance;
	cameraPos.x	*= d;
	cameraPos.y	*= m_Camera.distance;
	cameraPos.z	*= d;

	gSiegeEngine.GetCamera().SetCameraAndTargetPosition( cameraPos, m_Camera.targetPos.pos );

	gSiegeEngine.GetCamera().UpdateCamera();
}


gpstring UICharacterSelect::GetClothTexture()
{
	gpstring clothTexture;

	SkinSetColl::iterator i;
	for ( i = m_Skins.begin(); i != m_Skins.end(); ++i )
	{
		if ( ((*i).GetType() == SkinSet::SKIN_PANTS || (*i).GetType() == SkinSet::SKIN_SHIRTS) && ( (*i).GetGroup().same_no_case( GetGo( m_GoodActors[m_GoodActorIndex] )->GetTemplateName() ) ) )
		{
			if ( (*i).GetSelectedTexture().size() > 0 )
			{
				if ( clothTexture.size() > 0 )
				{
					clothTexture.append( "," );
				}
				clothTexture.append( (*i).GetSelectedTexture() );
			}
		}
	}

	return clothTexture;
}

gpstring UICharacterSelect::GetFleshTexture()
{
	gpstring faceTexture;
	gpstring hairTexture;

	SkinSetColl::iterator i;
	for ( i = m_Skins.begin(); i != m_Skins.end(); ++i )
	{
		if ( ((*i).GetType() == SkinSet::SKIN_FACES) && ( (*i).GetGroup().same_no_case( GetGo( m_GoodActors[m_GoodActorIndex] )->GetTemplateName() ) ) )
		{
			if ( (*i).GetSelectedTexture().size() > 0 )
			{
				faceTexture = (*i).GetSelectedTexture();
			}
		}
		if ( ((*i).GetType() == SkinSet::SKIN_HAIR) && ( (*i).GetGroup().same_no_case( GetGo( m_GoodActors[m_GoodActorIndex] )->GetTemplateName() ) ) )
		{
			if ( (*i).GetSelectedTexture().size() > 0 )
			{
				hairTexture = (*i).GetSelectedTexture();
			}
		}
	}


	if ( !faceTexture.empty() && !hairTexture.empty() )
	{
		return faceTexture.append( "," ).append( hairTexture );
	}
	else if ( !faceTexture.empty() )
	{
		return faceTexture;
	}

	return hairTexture;

}

gpstring UICharacterSelect::GetHeadMesh()
{
	gpstring headMesh;

	SkinSetColl::iterator i;
	for ( i = m_Skins.begin(); i != m_Skins.end(); ++i )
	{
		if ( ((*i).GetType() == SkinSet::SKIN_HEADS) && ( (*i).GetGroup().same_no_case( GetGo( m_GoodActors[m_GoodActorIndex] )->GetTemplateName() ) ) )
		{
			if ( (*i).GetSelectedTexture().size() > 0 )
			{
				headMesh.assign( (*i).GetSelectedTexture() );
			}
		}
	}

	return headMesh;
}

void UICharacterSelect::SelectNextFaceTexture()
{
	SkinSetColl::iterator i;
	for ( i = m_Skins.begin(); i != m_Skins.end(); ++i )
	{
		if ( ((*i).GetType() == SkinSet::SKIN_FACES) && ((*i).GetGroup().same_no_case( GetGo( m_GoodActors[m_GoodActorIndex] )->GetTemplateName() )) )
		{
			(*i).SelectNextTexture();
		}
	}

	UpdateTextures();
}

void UICharacterSelect::SelectPreviousFaceTexture()
{
	SkinSetColl::iterator i;
	for ( i = m_Skins.begin(); i != m_Skins.end(); ++i )
	{
		if ( ((*i).GetType() == SkinSet::SKIN_FACES) && ((*i).GetGroup().same_no_case( GetGo( m_GoodActors[m_GoodActorIndex] )->GetTemplateName() )) )
		{
			(*i).SelectPreviousTexture();
		}
	}

	UpdateTextures();
}

void UICharacterSelect::SelectNextHairTexture()
{
	SkinSetColl::iterator i;
	for ( i = m_Skins.begin(); i != m_Skins.end(); ++i )
	{
		if ( ((*i).GetType() == SkinSet::SKIN_HAIR) && ((*i).GetGroup().same_no_case( GetGo( m_GoodActors[m_GoodActorIndex] )->GetTemplateName() )) )
		{
			(*i).SelectNextTexture();
			while ( !CanUseHairTexture( (*i).GetSelectedTexture() ) )
			{
				(*i).SelectNextTexture();
			}
		}
	}

	UpdateTextures();
}

void UICharacterSelect::SelectPreviousHairTexture()
{
	SkinSetColl::iterator i;
	for ( i = m_Skins.begin(); i != m_Skins.end(); ++i )
	{
		if ( ((*i).GetType() == SkinSet::SKIN_HAIR) && ((*i).GetGroup().same_no_case( GetGo( m_GoodActors[m_GoodActorIndex] )->GetTemplateName() )) )
		{
			(*i).SelectPreviousTexture();
			while ( !CanUseHairTexture( (*i).GetSelectedTexture() ) )
			{
				(*i).SelectPreviousTexture();
			}
		}
	}

	UpdateTextures();
}

void UICharacterSelect::SelectNextShirtTexture()
{
	SkinSetColl::iterator i;
	for ( i = m_Skins.begin(); i != m_Skins.end(); ++i )
	{
		if ( ((*i).GetType() == SkinSet::SKIN_SHIRTS) && ((*i).GetGroup().same_no_case( GetGo( m_GoodActors[m_GoodActorIndex] )->GetTemplateName() )) )
		{
			(*i).SelectNextTexture();
		}
	}

	UpdateTextures();
}

void UICharacterSelect::SelectNextPantsTexture()
{
	SkinSetColl::iterator i;
	for ( i = m_Skins.begin(); i != m_Skins.end(); ++i )
	{
		if ( ((*i).GetType() == SkinSet::SKIN_PANTS) && ((*i).GetGroup().same_no_case( GetGo( m_GoodActors[m_GoodActorIndex] )->GetTemplateName() )) )
		{
			(*i).SelectNextTexture();
		}
	}

	UpdateTextures();
}

void UICharacterSelect::SelectNextHeadTexture()
{
	SkinSetColl::iterator i;
	for ( i = m_Skins.begin(); i != m_Skins.end(); ++i )
	{
		if ( ((*i).GetType() == SkinSet::SKIN_HEADS) && ((*i).GetGroup().same_no_case( GetGo( m_GoodActors[m_GoodActorIndex] )->GetTemplateName() )) )
		{
			(*i).SelectNextTexture();
			if ( !CanUseHeadMesh( (*i).GetSelectedTexture() ) )
			{
				SelectNextHairTexture();
			}
		}
	}

	UpdateTextures();
}

void UICharacterSelect::SelectPreviousShirtTexture()
{
	SkinSetColl::iterator i;
	for ( i = m_Skins.begin(); i != m_Skins.end(); ++i )
	{
		if ( ((*i).GetType() == SkinSet::SKIN_SHIRTS) && ((*i).GetGroup().same_no_case( GetGo( m_GoodActors[m_GoodActorIndex] )->GetTemplateName() )) )
		{
			(*i).SelectPreviousTexture();
		}
	}

	UpdateTextures();
}

void UICharacterSelect::SelectPreviousPantsTexture()
{
	SkinSetColl::iterator i;
	for ( i = m_Skins.begin(); i != m_Skins.end(); ++i )
	{
		if ( ((*i).GetType() == SkinSet::SKIN_PANTS) && ((*i).GetGroup().same_no_case( GetGo( m_GoodActors[m_GoodActorIndex] )->GetTemplateName() )) )
		{
			(*i).SelectPreviousTexture();
		}
	}

	UpdateTextures();
}


void UICharacterSelect::SelectPreviousHeadTexture()
{
	SkinSetColl::iterator i;
	bool bReselect = false;		
	for ( i = m_Skins.begin(); i != m_Skins.end(); )
	{		
		if ( ((*i).GetType() == SkinSet::SKIN_HEADS) && ((*i).GetGroup().same_no_case( GetGo( m_GoodActors[m_GoodActorIndex] )->GetTemplateName() )) )
		{
			if ( !bReselect )
			{
				(*i).SelectPreviousTexture();
			}
			if ( !CanUseHeadMesh( (*i).GetSelectedTexture() ) )
			{				
				SelectNextHairTexture();
				bReselect = true;
			}
			else
			{
				bReselect = false;
			}
		}

		if ( !bReselect )
		{
			++i;
		}
	}

	UpdateTextures();
}

int UICharacterSelect::GetTextureCount( SkinSet::eSkin skin )
{
	SkinSetColl::iterator i;
	for ( i = m_Skins.begin(); i != m_Skins.end(); ++i )
	{
		if ( ((*i).GetType() == skin) && ((*i).GetGroup().same_no_case( GetGo( m_GoodActors[m_GoodActorIndex] )->GetTemplateName() )) )
		{
			return ( (*i).GetTextureCount() );
		}
	}
	return 0;
}

void UICharacterSelect::SaveCustomSelections()
{
	FuelHandle hCharacter = gTattooGame.GetPrefsConfigFuel( true );
	if ( !hCharacter.IsValid() )
	{
		return;
	}
	
	if ( m_bIsMulti )
	{
		hCharacter = hCharacter->GetChildBlock( "multiplayer", true );
		if ( !hCharacter.IsValid() )
		{
			return;
		}
	}

	hCharacter = hCharacter->GetChildBlock( "character", true );
	if ( !hCharacter.IsValid() )
	{
		return;
	}

	for ( SkinSetColl::iterator skinSet = m_Skins.begin(); skinSet != m_Skins.end(); ++skinSet )
	{
		if ( !skinSet->GetSelectedTexture().empty() )
		{
			hCharacter->Set( gpstringf( "texture_%s_%s", skinSet->GetGroup().c_str(), skinSet->GetTypeString().c_str() ), skinSet->GetSelectedTexture() );
		}
	}

	hCharacter->Set( "selected_character", GetGo( m_GoodActors[m_GoodActorIndex] )->GetTemplateName() );

	if ( !m_bIsMulti )
	{
		UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "name_edit_box", "character_select" );
		if ( pEditBox )
		{
			hCharacter->Set( "character_name", pEditBox->GetText() );
		}
	}

	hCharacter->GetDB()->SaveChanges();
}


void UICharacterSelect::LoadCustomSelections()
{
	AppModule::AutoFreezeTime freezeTime;

	FuelHandle hCharacter = gTattooGame.GetPrefsConfigFuel();
	
	if ( hCharacter.IsValid() )
	{
		if ( m_bIsMulti )
		{
			hCharacter = hCharacter->GetChildBlock( "multiplayer:character" );
		}
		else
		{
			hCharacter = hCharacter->GetChildBlock( "character" );
		}
	}

	if ( !hCharacter.IsValid() )
	{
		if ( !m_bIsMulti )
		{
			FastFuelHandle hCharacterDefaults( "config:character_defaults" );
			if ( hCharacterDefaults )
			{
				gpstring sTexture;

				for ( SkinSetColl::iterator skinSet = m_Skins.begin(); skinSet != m_Skins.end(); ++skinSet )
				{
					sTexture = hCharacterDefaults.GetString( gpstringf( "texture_%s_%s", skinSet->GetGroup().c_str(), skinSet->GetTypeString().c_str() ) );
					if ( !sTexture.empty() )
					{
						skinSet->SelectTexture( sTexture );
						skinSet->AcceptSelection();
					}
				}

				gpstring selectChar = hCharacterDefaults.GetString( "selected_character" );
				if ( selectChar.empty() )
				{
					const char* randHero = gContentDb.GetRandomHeroName();
					if ( randHero != NULL )
					{
						selectChar = randHero;
					}
				}
				for ( unsigned int SelChar = 0; SelChar < m_GoodActors.size(); ++SelChar )
				{
					if ( selectChar.same_no_case( GetGo( m_GoodActors[SelChar] )->GetTemplateName() ) )
					{
						m_GoodActorIndex = SelChar;
						m_SelectedActor = SelChar;
						break;
					}
				}
				SetActiveCharacter();
			}
		}
		return;
	}

	char *charPath = "player";
	if ( m_bIsMulti )
	{
		charPath = "multiplayer\\character";
	}

	gpstring sTexture;

	for ( SkinSetColl::iterator skinSet = m_Skins.begin(); skinSet != m_Skins.end(); ++skinSet )
	{
		sTexture = hCharacter->GetString( gpstringf( "texture_%s_%s", skinSet->GetGroup().c_str(), skinSet->GetTypeString().c_str() ) );
		if ( !sTexture.empty() )
		{
			skinSet->SelectTexture( sTexture );
			skinSet->AcceptSelection();
		}
	}

	gpstring selectChar = hCharacter->GetString( "selected_character" );
	if ( selectChar.empty() )
	{
		const char* randHero = gContentDb.GetRandomHeroName();
		if ( randHero != NULL )
		{
			selectChar = randHero;
		}
	}
	for ( unsigned int SelChar = 0; SelChar < m_GoodActors.size(); ++SelChar )
	{
		if ( selectChar.same_no_case( GetGo( m_GoodActors[SelChar] )->GetTemplateName() ) )
		{
			m_GoodActorIndex = SelChar;
			m_SelectedActor = SelChar;
			break;
		}
	}
	SetActiveCharacter();

	if ( !m_bIsMulti )
	{
		UIEditBox * pEditBox = (UIEditBox *)gUIShell.FindUIWindow( "name_edit_box", "character_select" );
		if ( pEditBox )
		{
			gpwstring charName;
			hCharacter->Get( "character_name", charName );
			pEditBox->SetText( charName );
		}
	}
}


gpstring SkinSet::GetTypeString()
{
	return ( gpstring( s_SkinSets[ (int)m_GroupType ] ) );
}

SkinSet * UICharacterSelect::GetSkinSet( SkinSet::eSkin skinType, gpstring skinGroup )
{
	SkinSetColl::iterator i;

	for ( i = m_Skins.begin(); i != m_Skins.end(); ++i )
	{
		if ( ((*i).GetType() == skinType) && (skinGroup.same_no_case( (*i).GetGroup() )) )
		{
			return (SkinSet *)i;
		}
	}

	m_Skins.push_back( SkinSet( skinType, skinGroup ) );

	return ( &m_Skins.back() );
}


void UICharacterSelect::AddAllCharacters()
{
	GoidColl sources;

	if ( !m_bIsMulti )
	{
		gContentDb.GetAllHeroes( sources );
	}
	else
	{
		FastFuelHandle hCharacters( "config:multiplayer:characters" );

		if ( hCharacters.IsValid() )
		{
			FastFuelFindHandle hFindCharacters( hCharacters );

			if ( hFindCharacters.IsValid() )
			{
				if ( hFindCharacters.FindFirstKeyAndValue() )
				{
					gpstring key, value;
					while ( hFindCharacters.GetNextKeyAndValue( key, value ) )
					{
						Goid goid = gGoDb.FindCloneSource( value );
						if ( goid != GOID_INVALID )
						{
							sources.push_back( goid );
						}
					}
				}
			}
		}
	}

	GoidColl::iterator iHero;
	for ( iHero = sources.begin(); iHero != sources.end(); ++iHero )
	{
		GoCloneReq cloneReq( (*iHero) );
		cloneReq.m_OmniGo = true;
		Goid actor = gGoDb.CloneLocalGo( cloneReq );
		if ( actor != GOID_INVALID )
		{
			m_GoodActors.push_back( actor );
			gpgenericf(( "AddAllCharacters : 0x%08X\n", MakeInt( actor ) ));
		}
	}	

	gGoDb.CommitAllRequests();
}


FuBiCookie UICharacterSelect::RSGeneratePortrait( PlayerId playerId )
{
	FUBI_RPC_CALL_RETRY( RSGeneratePortrait, RPC_TO_SERVER );

	CHECK_SERVER_ONLY;

	Player * pPlayer = gServer.GetPlayer( playerId );
	if ( pPlayer )
	{
		return ( RCGeneratePortrait( playerId, pPlayer->GetMachineId() ) );
	}

	return RPC_FAILURE;
}


FuBiCookie UICharacterSelect::RCGeneratePortrait( PlayerId playerId, DWORD machineId )
{
	FUBI_RPC_CALL_RETRY( RCGeneratePortrait, machineId );

	Player *pPlayer = gServer.GetPlayer( playerId );
	if ( !pPlayer )
	{
		return RPC_FAILURE;
	}

	const DWORD prevGoodActorIndex = m_GoodActorIndex;

	bool bFoundActor = false;

	for ( DWORD findChar = 0; findChar < m_GoodActors.size(); ++findChar )
	{
		if ( pPlayer->GetHeroCloneSourceTemplate().same_no_case( GetGo( m_GoodActors[ findChar ] )->GetTemplateName() ) )
		{
			m_GoodActorIndex = findChar;
			bFoundActor = true;
			break;
		}
	}

	if ( !bFoundActor )
	{
		m_GoodActorIndex = prevGoodActorIndex;
		return RPC_FAILURE;
	}

	// Set the character's attributes
	GoHandle go( m_GoodActors[ m_GoodActorIndex ] );
	go->GetAspect()->SetDynamicTexture( PS_CLOTH, pPlayer->GetHeroSkin( PS_CLOTH ) );
	go->GetAspect()->SetDynamicTexture( PS_FLESH, pPlayer->GetHeroSkin( PS_FLESH ) );

	gpassert(go->HasInventory())
	go->GetInventory()->SetCustomHead( pPlayer->GetHeroHead() );

	// Start a new scene
	gDefaultRapi.Begin3D();

	// Take the shot
	GeneratePortrait();

	// End the scene without copying to primary
	gDefaultRapi.End3D( false, true );

	// Generate portrait
	RECT rect;
	rect.left	= 380;
	rect.top	= 277;
	rect.right	= 444;
	rect.bottom	= 341;

	// Create image
	RapiMemImage* pImage = gSiegeEngine.Renderer().CreateSubsectionImage( &rect );

	// Set all of the alpha bits to full on
	DWORD* pImageBits = pImage->GetBits();
	for( int i = 0; i < (pImage->GetWidth() * pImage->GetHeight()); ++i )
	{
		pImageBits[ i ]	|= 0xFF000000;
	}

	// Set the player's portrait
	DWORD portrait_tex = gDefaultRapi.CreateAlgorithmicTexture( "portrait", pImage, false, 0, TEX_LOCKED, false, true );
	pPlayer->SetHeroPortrait( portrait_tex, false );

	// Restore our textures
	UpdateTextures();
	m_GoodActorIndex = prevGoodActorIndex;

	return RPC_SUCCESS;
}


void UICharacterSelect::AcceptCharacterSelection()
{
	m_SelectedActor = m_GoodActorIndex;

	for ( SkinSetColl::iterator i = m_Skins.begin(); i != m_Skins.end(); ++i )
	{
		i->AcceptSelection();
	}

	GoHandle selectedActor( m_GoodActors[ m_GoodActorIndex ] );
	gServer.GetLocalHumanPlayer()->RSSetHeroCloneSourceTemplate( selectedActor->GetTemplateName() );
	gServer.GetLocalHumanPlayer()->RSSetStashCloneSourceTemplate( gContentDb.GetDefaultStashTemplate() );

	gpstring clothTexture = GetClothTexture();
	if ( !clothTexture.empty() )
	{
		gServer.GetLocalHumanPlayer()->RSSetHeroSkin( PS_CLOTH, clothTexture );
	}

	gpstring fleshTexture = GetFleshTexture();
	if ( !fleshTexture.empty() )
	{
		gServer.GetLocalHumanPlayer()->RSSetHeroSkin( PS_FLESH, fleshTexture );
	}

	gpstring headMesh = GetHeadMesh();
	if ( !headMesh.empty() )
	{
		gServer.GetLocalHumanPlayer()->RSSetHeroHead( headMesh );
	}

	RSGeneratePortrait( gServer.GetLocalHumanPlayer()->GetId() );

	SaveCustomSelections();
}


void UICharacterSelect::CancelCharacterSelection()
{
	m_GoodActorIndex = m_SelectedActor;

	for ( SkinSetColl::iterator i = m_Skins.begin(); i != m_Skins.end(); ++i )
	{
		i->CancelSelection();
	}
}


void UICharacterSelect::SetDisplayActor( Goid actor )
{
	// delete old to prevent leak
	gGoDb.SMarkForDeletion( m_DisplayActor );

	// take new
	m_DisplayActor = actor;
	PrepAnim();
}


bool UICharacterSelect::CanUseHairTexture( gpstring sHair )
{
	gpstring sHead;
	SkinSetColl::iterator i;
	for ( i = m_Skins.begin(); i != m_Skins.end(); ++i )
	{
		if ( ((*i).GetType() == SkinSet::SKIN_HEADS) && ((*i).GetGroup().same_no_case( GetGo( m_GoodActors[m_GoodActorIndex] )->GetTemplateName() )) )
		{
			sHead = (*i).GetSelectedTexture();
		}
	}

	return CanUseHeadHair( sHair, sHead );
}


bool UICharacterSelect::CanUseHeadMesh( gpstring sHead )
{
	gpstring sHair;
	SkinSetColl::iterator i;
	for ( i = m_Skins.begin(); i != m_Skins.end(); ++i )
	{
		if ( ((*i).GetType() == SkinSet::SKIN_HAIR) && ((*i).GetGroup().same_no_case( GetGo( m_GoodActors[m_GoodActorIndex] )->GetTemplateName() )) )
		{
			sHair = (*i).GetSelectedTexture();
		}
	}

	return CanUseHeadHair( sHair, sHead );
}


bool UICharacterSelect::CanUseHeadHair( gpstring sHair, gpstring sHead )
{
	bool bTemplateNotFound = true;
	HairToHeadColl::iterator iHth;
	for ( iHth = m_hairToHeads.begin(); iHth != m_hairToHeads.end(); ++iHth )
	{
		if ( (*iHth).sTemplate.same_no_case( GetGo( m_GoodActors[m_GoodActorIndex] )->GetTemplateName() ) )
		{
			bTemplateNotFound = false;

			HairHeadsMap::iterator iMap = (*iHth).hairToHeads.find( sHair );
			if ( iMap != (*iHth).hairToHeads.end() )
			{
				StringVec::iterator iHeads;
				for ( iHeads = (*iMap).second.begin(); iHeads != (*iMap).second.end(); ++iHeads )
				{
					if ( (*iHeads).same_no_case( sHead ) )
					{
						return true;
					}
				}
			}
		}
	}

	if ( bTemplateNotFound )
	{
		return true;
	}

	return false;
}


///////////////////////////////////////////////////////////////

void SkinSet::SelectDefaultTexture()
{
	if ( m_Textures.size() > 0 )
	{
		m_SelectedTexture = 0;
		m_Texture = 0;
	}
	else
	{
		m_SelectedTexture = -1;
		m_Texture = -1;
	}	
}

void SkinSet::SelectNextTexture()
{
	if ( m_SelectedTexture < 0 )
	{
		return;
	}

	++m_SelectedTexture;

	if ( m_SelectedTexture > (int)m_Textures.size() - 1 )
	{
		m_SelectedTexture = 0;
	}
}

void SkinSet::SelectPreviousTexture()
{
	if ( m_SelectedTexture < 0 )
	{
		return;
	}	

	--m_SelectedTexture;	

	if ( m_SelectedTexture < 0 )
	{
		m_SelectedTexture = (int)m_Textures.size() - 1;
	}	
}

gpstring SkinSet::GetSelectedTexture()
{
	if ( m_SelectedTexture < 0 )
	{
		return gpstring();
	}
	
	return m_Textures[m_SelectedTexture];
}

void SkinSet::SelectTexture( gpstring texture )
{
	int TextureCount = 0;

	SkinTextureColl::iterator i;
	for ( i = m_Textures.begin(); i != m_Textures.end(); ++i, ++TextureCount )
	{
		if ( texture.same_no_case( (*i) ) )
		{
			m_SelectedTexture = TextureCount;			
			return;
		}
	}
}
