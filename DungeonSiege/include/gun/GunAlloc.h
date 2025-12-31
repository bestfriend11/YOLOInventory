/*-------------------------------------------------------------------------
  GunAlloc.hpp
  
  Interface for memory allocations, so Gun consumer can own all memory
  
  Owner: curtc
  
  Copyright 1986-2000 Microsoft Corporation, All Rights Reserved
 *-----------------------------------------------------------------------*/
#pragma once

namespace Gun2 // namespace all gun reuseable library components use
{

interface 
__declspec(uuid("{AD27F0E2-CCB6-4a1d-B0C3-879DF73F8C0F}"))
IGUNAllocator : public IMalloc
{
  // for certain users of IGUNAllocatorEvents, they may have a common allocation size,
  // which if known in advance can allow for efficient pools
  STDMETHOD_(void, AllocSizeHint)(DWORD cbytes) = 0;
};


// if you don't care, just use the default
STDMETHODIMP CreateDefaultGUNAllocator(
        OUT IGUNAllocator ** ppGunAllocator);

} // namespace Gun2
