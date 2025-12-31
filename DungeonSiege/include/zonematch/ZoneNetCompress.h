#ifndef _ZONENETCOMPRESS_H
#define _ZONENETCOMPRESS_H

#include "zonenet.h"

// {3DB68F88-CD52-4c3b-9428-4BEA839D84CE}
DEFINE_GUID(CLSID_ZoneNetCompress, 0x3db68f88, 0xcd52, 0x4c3b, 0x94, 0x28, 0x4b, 0xea, 0x83, 0x9d, 0x84, 0xce);

// {83892FE0-55D5-47b5-8911-1B8534D1C356}
DEFINE_GUID(IID_IZoneNetCompress, 0x83892fe0, 0x55d5, 0x47b5, 0x89, 0x11, 0x1b, 0x85, 0x34, 0xd1, 0xc3, 0x56);

interface IZoneNetCompress : public IUnknown
{
    //
    // Compress
    //
    // Compresss an Compressed ZONETICKET for use
    //
    virtual HRESULT STDMETHODCALLTYPE CompressData
                     ( IN     const LPBYTE          pSrcData,	// buffer containing data
                       IN           DWORD           cbSrcData,	// sizeof data pointed to by pBuffer
                       IN OUT       LPBYTE*			pDestData,	// Compressed data
                       IN OUT       DWORD*          pcbDestData	// size of Compressed data
                     ) = 0;

    virtual HRESULT STDMETHODCALLTYPE Free(
                       IN     const LPBYTE          pSrcData,   // pointer to data to be freed
                       IN     const DWORD           cbData      // amount of data to be freed
                     ) = 0;
};

#endif //ndef _ZONENETCOMPRESS_H
