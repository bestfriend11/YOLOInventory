#include "pch.h"
#include "TblMgr.h"

#include "misc.h"


void
CTableManager::ServerDataCB(
        IN DWORD iRowId,
        IN IZoneMatchTableView::UPDATE_STATUS status,
        IN LPZONEMATCHROW pRow,
        IN PVOID pContext )
{
    CTableManager* pThis=(CTableManager*)pContext;

    pThis->DataThis( iRowId, status, pRow, "Server" );
}
        

void
CTableManager::PlayerDataCB(
        IN DWORD iRowId,
        IN IZoneMatchTableView::UPDATE_STATUS status,
        IN LPZONEMATCHROW pRow,
        IN PVOID pContext )
{
    CTableManager* pThis=(CTableManager*)pContext;

    pThis->DataThis( iRowId, status, pRow, "Player" );
}
        

void
CTableManager::DataThis(
        IN DWORD iRowId,
        IN IZoneMatchTableView::UPDATE_STATUS status,
        IN LPZONEMATCHROW pRow,
        IN LPCSTR pszTable )
{
    DWORD i;

    printf("%s RowId=%d %s", pszTable, iRowId, pszUPDATE_STATUS( status ) );
    if( IZoneMatchTableView::ROW_UPDATE == status )
    {
        printf(":");
        for(i=0; i<pRow->Count(); i++)
        {
            if( NULL == pRow->Strings(i) )
                printf(" ''");
            else
                printf(" '%s'", pRow->Strings(i));
        }
    }
    printf("\n");
}

