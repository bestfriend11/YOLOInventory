/**************************************************************************

Filename		: UICharacterSelect.h

Class			: UICharacterSelect

Author			: Chad Queen

Description		: This class handles the loading and display of the 
				  characters the user may select.

Creation Date	: 1/17/99

**************************************************************************/



#pragma once
#ifndef _UICHARACTERSELECT_H_
#define _UICHARACTERSELECT_H_


// Include Files
#include "GoDefs.h"

#include "CameraPosition.h"
#include "nema_types.h"
#include "siege_node.h"

// Skin handler class
class SkinSet
{
public:
	
	// Types

	enum eSkin
	{
		SET_BEGIN_ENUM( SKIN_, 0 ),

		SKIN_PANTS,
		SKIN_SHIRTS,
		SKIN_FACES,
		SKIN_HAIR,
		SKIN_HEADS,

		SET_END_ENUM( SKIN_ ),
	};

	typedef std::vector< gpstring > SkinTextureColl;

	// Setup

	SkinSet( eSkin type, gpstring group )	{ m_GroupType = type; m_GroupName = group;
											  m_Texture = -1; m_SelectedTexture = -1; }

	eSkin	 GetType()						{ return ( m_GroupType ); }
	gpstring GetGroup()						{ return ( m_GroupName ); }
	gpstring GetTypeString();

	void AddTexture( gpstring texture )		{ m_Textures.push_back( texture ); }

	void SelectDefaultTexture();
	void SelectNextTexture();
	void SelectPreviousTexture();
	gpstring GetVisibleTexture();
	gpstring GetSelectedTexture();
	void SelectTexture( gpstring texture );

	int GetTextureCount()					{ return m_Textures.size(); }

	void AcceptSelection()					{ m_Texture = m_SelectedTexture; }
	void CancelSelection()					{ m_SelectedTexture = m_Texture; }

private:

	eSkin		m_GroupType;
	gpstring	m_GroupName;

	SkinTextureColl	m_Textures;

	int			m_Texture;
	int			m_SelectedTexture;
};


class UICharacterSelect : public Singleton <UICharacterSelect>
{
public:

	// Constructor
	UICharacterSelect( bool bMultiPlayer = false );

	// Destructor
	~UICharacterSelect();

	// Draws the current scene
	void Draw();

	// Renders portrait shot to center 32x32 area of the current viewport
	void GeneratePortrait();

	// Update the current scene
	void Update( double secondsElapsed );

	void Deform( double secondsElapsed );

	void Initialize();

	// Deletes all the gos
	void DeleteSelectionGos();

	bool IsInitialized()					{ return m_bInitialized; }

	// Set the currently active character
	void SetActiveCharacter();
	void SetNextActiveCharacter();	
	void SetPreviousActiveCharacter();

	void SetSelectedCharacter( const gpstring & actorName );

	// Get character information
	const Goid	GetActiveCharacter()		{ return m_GoodActors[m_GoodActorIndex]; }
	gpwstring	GetActiveCharacterName();

	void SetRotationX( float x ) { m_Camera.orbitAngle += x; }

	void RotateY( float y );

	void SelectNextShirtTexture();
	void SelectNextPantsTexture();
	void SelectPreviousShirtTexture();
	void SelectPreviousPantsTexture();

	void SelectNextFaceTexture();
	void SelectNextHairTexture();
	void SelectPreviousFaceTexture();
	void SelectPreviousHairTexture();

	void SelectPreviousHeadTexture();
	void SelectNextHeadTexture();

	int GetTextureCount( SkinSet::eSkin skin );

	gpstring GetClothTexture();
	gpstring GetFleshTexture();
	gpstring GetHeadMesh();

	void SaveCustomSelections();
	void LoadCustomSelections();

	void AddAllCharacters();

FEX FuBiCookie RSGeneratePortrait( PlayerId playerId );
FEX FuBiCookie RCGeneratePortrait( PlayerId playerId, DWORD machineId );
	FUBI_MEMBER_SET( RCGeneratePortrait, +SERVER );

	// Character selection
	void AcceptCharacterSelection();
	void CancelCharacterSelection();

	void SetDisplayActor( Goid actor );
	
private:

	// Setup a temporary viewport
	void SetTemporaryView();

	// Setup the lights
	void PrepLights();

	// Prepare and animation to be played
	bool PrepAnim();

	// Update textures on all clients
	void UpdateTextures();

	bool CanUseHairTexture( gpstring sHair );
	bool CanUseHeadMesh( gpstring sHead );
	bool CanUseHeadHair( gpstring sHair, gpstring sHead );

	siege::SiegeNodeLight	m_SunLight;
	CameraEulerPosition		m_Camera;

	GoidColl				m_GoodActors;
	DWORD					m_GoodActorIndex;
	DWORD					m_SelectedActor;

	Goid					m_DisplayActor; // display this actor instead of the selected actor.

	bool					m_bIsMulti;
	
	matrix_3x3				m_rotateMatrix;	
	
	// Skin texture selection
	typedef std::vector< SkinSet > SkinSetColl;

	SkinSetColl				m_Skins;

	bool					m_bInitialized;	
	bool					m_bCharactersAdded;

	typedef std::map< gpstring, StringVec, istring_less > HairHeadsMap;
	struct TemplateHairHead
	{
		gpstring sTemplate;
		HairHeadsMap hairToHeads;		
	};

	typedef std::vector< TemplateHairHead > HairToHeadColl;
	HairToHeadColl m_hairToHeads;

	SkinSet * GetSkinSet( SkinSet::eSkin skinType, gpstring skinGroup );

	FUBI_SINGLETON_CLASS( UICharacterSelect, "This is the UICharacterSelect singleton" );
};

#define gUICharacterSelect UICharacterSelect::GetSingleton()


#endif


