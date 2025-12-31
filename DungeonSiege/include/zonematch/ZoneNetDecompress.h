#ifndef _ZONENETDECOMPRESS_H
#define _ZONENETDECOMPRESS_H

#include "zonenet.h"

// {3A8CDF72-28EA-4f1a-A01B-970FFCA41C14}
DEFINE_GUID(CLSID_ZoneNetDecompress, 0x3a8cdf72, 0x28ea, 0x4f1a, 0xa0, 0x1b, 0x97, 0xf, 0xfc, 0xa4, 0x1c, 0x14);

// {E462A498-082D-4ab7-B6BE-F56E8A849AD5}
DEFINE_GUID(IID_IZoneNetDecompress, 0xe462a498, 0x82d, 0x4ab7, 0xb6, 0xbe, 0xf5, 0x6e, 0x8a, 0x84, 0x9a, 0xd5);


interface IZoneNetDecompress : public IUnknown
{
    //
    // Decompress
    //
    // Compresss an Compressed ZONETICKET for use
    //
    virtual HRESULT STDMETHODCALLTYPE DecompressData
                     ( IN     const LPBYTE          pSrcData,	// buffer containing data
                       IN           DWORD           cbSrcData,	// sizeof data pointed to by pBuffer
                       IN OUT       LPBYTE*			pDestData,	// Decompressed data
                       IN OUT       DWORD*          pcbDestData	// size of Compressed data
                     ) = 0;

    virtual HRESULT STDMETHODCALLTYPE Free(
                       IN     const LPBYTE          pSrcData,   // pointer to data to be freed
                       IN     const DWORD           cbData      // amount of data to be freed
                     ) = 0;
};

#endif //ndef _ZONENETDECOMPRESS_H
