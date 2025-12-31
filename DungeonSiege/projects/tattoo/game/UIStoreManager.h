//////////////////////////////////////////////////////////////////////////////
//
// File     :  UIStoreManager.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __UIStoreManager_H
#define __UIStoreManager_H

enum eQueryTrait;

class UIStoreManager : public Singleton <UIStoreManager>
{
public:

	UIStoreManager();

	void	Init();

	//********************* Store Management ***********************

	void	Update();

	// Checks for a store and activates it
	void	CheckForStoreAndActivate( Goid object, bool bConversing = false );
	void	ActivateStore( Go * store, Go * member );
	
	// The currently active store
	Goid	GetActiveStore()														{ return m_ActiveStore; }
	void	SetActiveStore( Goid store )											{ m_ActiveStore = store; }
	
	// The person who is buying the item
	Goid	GetActiveStoreBuyer()													{ return m_ActiveStoreBuyer; }
	void	SetActiveStoreBuyer( Goid buyer )										{ m_ActiveStoreBuyer = buyer; }

	// Sell this item(s) to the active store using the active buyer
	void	SellItemToActiveStore( Goid item );

	// Sell all of a type of item to the active store
	void	DialogStoreSellAll	( eQueryTrait = QT_ITEM );
	void	StoreSellAll		( eQueryTrait sellTrait = QT_ITEM );

	void		SetSelectedSellTrait( eQueryTrait sellTrait )						{ m_SelectedSellTrait = sellTrait;	}
	eQueryTrait GetSelectedSellTrait( void )										{ return m_SelectedSellTrait;		}

	void	SetEnableSellAllDialog( bool bEnable )									{ m_SellAllDialogEnabled = bEnable; }
	bool	GetEnableSellAllDialog()												{ return m_SellAllDialogEnabled;	}	

	// Buy this item from the active store using the active buy ( can auto place in inv if neccessary )
	void	BuyItemFromActiveStore( Goid item, bool bAutoPlace = false );

	// Lets the gridbox know if the user can purchase the store item they clicked on
	void	GridCheckBuyerGold( UIGridbox * pGrid );

	// Lets the gridbox know if there is adequate space in the destination gridbox ( only checks user, store never cares )
	void	GridCheckDestInvSpace( UIGridbox * pGrid );

	// Show a different tab of thestore
	void	TabStoreChange();

	// The user has selected the buy/sell radio button from the store
	void	ButtonHire();
	void	ButtonShop();
	void	ButtonBuyPackmule();
	void	ButtonAcceptMember();
	void	ButtonDeclineMember();
	void	ButtonHireMember();
	void	ButtonPackRehire();

	// Auto-transfer from store to user or user to store
	void	TransferInventory();

	// Disable any auto transfer modes that are active
	void	DisableAutoTransfer();

	void	SetStoreActive( bool bActive )	{ m_bStoreActive = bActive; }
	bool	GetStoreActive()				{ return m_bStoreActive; }

	void	DeactivateStore();
	void	RCDeactivateStore( PlayerId playerId );

	int		GetBuyerGold();

	void	PurchaseGuiCb( Goid member, Goid item );

	void	DisplayNextPage();
	void	DisplayPreviousPage();

	Goid	GetAutoActivator()				{ return m_AutoActivator; }
	void	SetAutoActivator( Goid object )	{ m_AutoActivator = object; }

	void	HandleStoreSwitch( Goid newMember );

private:

	Goid										m_ActiveStore;
	Goid										m_ActiveStoreBuyer;
	bool										m_bStoreActive;
	Goid										m_AutoActivator;
	eQueryTrait									m_SelectedSellTrait;
	bool										m_SellAllDialogEnabled;
};


#define gUIStoreManager UIStoreManager::GetSingleton()


#endif