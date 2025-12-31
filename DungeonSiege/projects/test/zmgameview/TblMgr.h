#pragma once
#include <ZoneMatch.h>


class CTableManager
{
public:
    static void ServerDataCB( IN DWORD iRowId,
                              IN IZoneMatchTableView::UPDATE_STATUS status,
                              IN LPZONEMATCHROW pRow,
                              IN PVOID pContext );

    static void PlayerDataCB( IN DWORD iRowId,
                              IN IZoneMatchTableView::UPDATE_STATUS status,
                              IN LPZONEMATCHROW pRow,
                              IN PVOID pContext );
private:
    void DataThis( IN DWORD iRowId,
                   IN IZoneMatchTableView::UPDATE_STATUS status,
                   IN LPZONEMATCHROW pRow,
                   IN LPCSTR pszTableName );
};
