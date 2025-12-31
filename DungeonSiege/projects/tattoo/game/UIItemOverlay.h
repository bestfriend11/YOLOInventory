#pragma once
/*=====================================================================

	Name	:	UIItemOverlay
	Date	:	Jan, 2001
	Author	:	Adam Swensen

	Purpose	:	Displays UI labels for items on the ground.

---------------------------------------------------------------------*/

#ifndef __UIITEMOVERLAY_H
#define __UIITEMOVERLAY_H


class UIItemOverlay
{
public:

	enum
	{
		DEFAULT_MAX_SCREEN_ITEMS = 64,
		MAX_ITEMS_PER_ROW = 32,
		ITEM_LABEL_HEIGHT = 16,
	};

	UIItemOverlay( int maxVisibleItems = DEFAULT_MAX_SCREEN_ITEMS );
	~UIItemOverlay();

	void Draw();

	bool IsVisible()						{  return m_Visible;  }
	void SetVisible( bool visible );

	int  GetMaxVisibleLabels()				{  return m_MaxVisibleLabels;  }
	void SetMaxVisibleLabels( int count );

	void SetDraggedGoid( Goid goid )		{  m_DraggedGoid = goid;  }

	void SelectItem( Goid goid, bool bSelected = true );

	void PickupSelectedItems();

	void SelectAllItemsLike( Go *pItemGo );

	bool IsItemSelected( Goid goid );

	Goid GetHoverItem();
	void SetHoverItem( Goid object ) { m_HoverGoid = object; }

	void SetHitItemOverlay( bool flag )		{  m_bHitItemOverlay = flag; m_bDragItemOverlay = false;  }
	bool GetHitItemOverlay()				{  return m_bHitItemOverlay;  }

	void SetDragItemOverlay( bool flag )	{  m_bDragItemOverlay = flag; }
	bool GetDragItemOverlay()				{  return m_bDragItemOverlay; }	

	DWORD GetColorFromItem( Go * pItem );	

private:

	enum eLabelSort
	{
		SORT_ANY,
		SORT_LEFT,
		SORT_RIGHT,
		SORT_TOP,
		SORT_BOTTOM,
	};

	bool GetValidRect( int index, int x, int y, eLabelSort sort = SORT_ANY );

	bool m_bSortAlternate;

	bool m_Visible;
	int  m_MaxVisibleLabels;	
	int  m_TextPadding;

	bool m_bHitItemOverlay;
	bool m_bDragItemOverlay;

	DWORD m_dwLabelColor;
	DWORD m_dwLabelSelectedColor;
	DWORD m_dwLabelHoverColor;

	Goid m_DraggedGoid;

	Goid m_HoverGoid;

	GoidColl	m_SelectedItems;
	
	struct ScreenItem
	{
		UIText	* m_pText;
		GRect	rect;
	};	
	ScreenItem * m_pLabels;	

	struct ItemRow
	{
		ScreenItem *m_items[ MAX_ITEMS_PER_ROW ];
		int        m_count;
	};
	ItemRow *m_Rows;
};

#endif