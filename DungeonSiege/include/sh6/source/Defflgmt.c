/* defflgmt.c -- SmartHeap (tm) Default pool flags (multi-threaded).
 *
 * Copyright (C) 1991-1996 Compuware Corporation.
 * All Rights Reserved.
 *
 * No part of this source code may be copied, modified or reproduced
 * in any form without retaining the above copyright notice.
 * This source code, or source code derived from it, may not be redistributed
 * without express written permission of the author.
 *
 */

/*** Header Files ***/

#include "smrtheap.h"

unsigned MemDefaultPoolFlags = MEM_POOL_DEFAULT|MEM_POOL_SERIALIZE;
