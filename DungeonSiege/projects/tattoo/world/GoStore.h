//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoStore.h
// Author(s):  Scott Bilas, Chad Queen
//
// Summary  :  Contains the store component for Go's.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GOSTORE_H
#define __GOSTORE_H

//////////////////////////////////////////////////////////////////////////////

#include "Go.h"

// Forward Declarations
class UIGridbox;
class PContentReq;
enum  eQueryTrait;

//////////////////////////////////////////////////////////////////////////////
// class GoStore declaration

class GoStore : public GoComponent
{
public:
	SET_INHERITED( GoStore, GoComponent );

// Setup.

	// ctor/dtor
	GoStore( Go* parent );
	GoStore( const GoStore& source, Go* parent );
	virtual ~GoStore( void );

// Typedefs

	typedef std::vector< gpstring >					StringVec;
	typedef std::multimap< int, Go * >				PriceToGoMultiMap;
	typedef std::map< gpstring, int, istring_less > TemplateToStockMap;
	typedef std::pair< gpstring, int >				TemplateToStockPair;
	typedef std::set< gpstring, istring_less >		StringSet;		

// Interface.

	// Sell an item to the store
FEX	void RSAddToStore			( Goid item, Goid member );		
	void SAddToStore			( Goid item, Goid member );
	void SAddRefresh			( Goid member );
FEX	void RCAddToStore			( DWORD machineId );
	void AddToStore				( void );

FEX	void RSSellAllOfTrait		( eQueryTrait sellTrait, Goid member );

	// Buy an item from the store
FEX	void RSRemoveFromStore	( Goid item, Goid member, bool bAutoPlace = false );	
FEX void RCRemoveFromStore	( Goid item, DWORD machineId );
	void RemoveFromStore	( Goid item );

	// Buy a local item from the store
	void RemoveLocalItemFromStore( Goid item, bool bAutoPlace );
FEX	void RSRemoveLocalItemFromStore( const gpstring & templateName, DWORD randomSeed, 
									 Goid buyer, bool bAutoPlace );
	
	// Cleanup item from client machine
FEX void RCRemoveItemFromGrid( Goid item, DWORD machineId );

	// Insert the item into the master list
	bool InsertReceipt		( Goid item, Goid soldBy );

	// Refresh the current sellable item list
	void RecalcItemCollection( PriceToGoMultiMap & items );

	// Set this variable to true to make sure no extra work happens until the transaction is over.
	void SetTransactionPending	( bool bSet	)	{ m_bTransactionPending = bSet; }
	bool GetTransactionPending	( void		)	{ return m_bTransactionPending; }	
	
	// Track who is shopping at the store so we can refresh the window pane 
	// for their machine as items are bought and sold.
FEX	void RSAddShopper		( Goid shopper );	
FEX void RSRemoveShopper	( Goid shopper );	
	
	// Client call - Creates local go items that are specific to the character that is viewing the store.
	// this prevents hitching on the server and prevents a large amount of objects having to be loaded at the
	// same time
	void CreatePStoreItemsForTab( const gpstring & sTabType );
	bool CanCreateStoreItemsForTab( const gpstring & sTabType );
	bool DoesStoreHaveItemsForTab( const gpstring & sTabType );

	// Client call - "Shows" the store ( sets up the ui and refreshes to the proper page
	bool ShowStore			( Goid shopper, bool bResetPage = false );	

	// Refreshes the store view using the current page.  This is where all the items are placed into the grid.
FEX void RCRefreshStoreView	( DWORD machineId );
	void RefreshStoreView	( void );

	// Handles the multiple pages per tab.
	void RefreshToPage		( int page )		{ SetCurrentPage( page ); RefreshStoreView(); }
	void SetCurrentPage		( int page )		{ m_currentPage = page; }
	int  GetCurrentPage		( void )			{ return m_currentPage; }
	
	// Finds the first tab that actually has items automatically
	void		SelectDefaultTab	( void );
	gpstring	GetActiveTabType	( void );		
	void		RefreshTabs			( void );

	// Restocks any items that are marked as restockable in the store component
	void Restock			( void );
	
	// Handles the sale of a hire to a buyer ( ex. pack mule to your party )
	void TransferHireToBuyer( Go * pHire, Go * pBuyer );
	void TransferHireToBuyer( Go * pBuyer );			// For transactions where it doesn't matter which one you get.
	int  GetHireCost		( void );
FEX	int	 GetNumHires		( void );

// Helpers

	Goid		GetShopper	( void )					{ return m_shopper; }
	
	UIGridbox * GetGridbox	( void ) const				{ return m_pGridbox;		}
	void		SetGridbox	( UIGridbox * pGrid )		{ m_pGridbox = pGrid;		}

	// Retrieves the buy/sell price.  Differs from aspect value depending if it was originally sold by the user or the store.
	int			GetPrice	( Goid item, Goid shopper, bool bNoChildren = false );		

	// Lets the ui know that a transfer is happening
	void		SetAutoTransfer( bool bTransfer );

	// Differentiates between a store that sells items, and a person that sells themself ( potential hire )
	bool		IsItemStore	( void ) const;

	// Is this a potential hire for your party?
	bool		GetCanSellSelf( void ) const			{ return m_bCanSellSelf;	}

	// Multiply the markup by the aspect's value to get the sale price
FEX	float		GetItemMarkup( void )					{ return m_Markup;			}	
FEX	void		SetItemMarkup( float markup )			{ m_Markup = markup;		}	

	void		AppendToolTipString( Goid item, gpwstring & sTooltip );		

// Const Query

	// The distance the party member has to be in order to activate the store.
	float				GetActivateRange( void ) const;	
	const gpstring &	GetLocalSpec	( void ) const;

// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newParent );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );

	// event handlers
	virtual void HandleMessage ( const WorldMessage& msg );
	
private:

	FUBI_EXPORT FuBiCookie RSAddPackmule( Go * pHire, Go * pBuyer );

	static DWORD    GoStoreToNet( GoStore* x );
	static GoStore* NetToGoStore( DWORD d, FuBiCookie* cookie );		

	struct TransactionReceipt
	{
		Goid		item;
		PlayerId	soldBy;
	};
	typedef std::vector< TransactionReceipt > Receipts;

	// Items where the local objects will be kept
	struct LocalItem
	{			
		gpstring		templateName;
		DWORD			randomSeed;
		Goid			item;
		bool			instantRestock;
	};
	typedef std::vector< LocalItem > LocalItems;	

	struct CharacterLocalItems
	{		
		LocalItems			localItems;
		StringVec			usedTabs;
	};
	typedef std::map< Goid, CharacterLocalItems > CharacterLocalItemMap;	

	// Master item collection of all items bought and sold.  This list is strictly maintained on the server in order
	// to determine buy and sell values of items.  On the client, the list only keeps reciepts of transactions that concern
	// that client.
	Receipts				m_receipts;		 	

	UIGridbox * 			m_pGridbox;
	bool					m_bCanSellSelf;
	Goid					m_newMember;
	TemplateToStockMap		m_restockColl;
	StringSet				m_instantRestockColl;	
	Goid					m_shopper;
	bool					m_bTransactionPending;	
	GoidColl				m_multiShopperColl;
	int						m_currentPage;
	float					m_Markup;				
	CharacterLocalItemMap	m_CharacterLocalItemMap;	

	SET_NO_COPYING( GoStore );
	FUBI_RPC_CLASS( GoStore, GoStoreToNet, NetToGoStore, "" );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __GOSTORE_H

//////////////////////////////////////////////////////////////////////////////
