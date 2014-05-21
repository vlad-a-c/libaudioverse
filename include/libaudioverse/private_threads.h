/**Copyright (C) Austin Hicks, 2014
This file is part of Libaudioverse, a library for 3D and environmental audio simulation, and is released under the terms of the Gnu General Public License Version 3 or (at your option) any later version.
A copy of the GPL, as well as other important copyright and licensing information, may be found in the file 'LICENSE' in the root of the Libaudioverse repository.  Should this file be missing or unavailable to you, see <http://www.gnu.org/licenses/>.*/
#pragma once
#include "libaudioverse.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*LavThreadCapableFunction)(void* param);

Lav_PUBLIC_FUNCTION LavError threadRun(LavThreadCapableFunction fn, void* param, void** destination);
Lav_PUBLIC_FUNCTION LavError threadJoinAndFree(void* t);
Lav_PUBLIC_FUNCTION LavError createMutex(void **destination);
Lav_PUBLIC_FUNCTION LavError freeMutex(void *m);
Lav_PUBLIC_FUNCTION LavError mutexLock(void *m);
Lav_PUBLIC_FUNCTION LavError mutexUnlock(void *m);

/**The following functions are threadsafe ringbuffers*/
typedef struct Lav_CrossThreadRingBuffer_s LavCrossThreadRingBuffer;
Lav_PUBLIC_FUNCTION LavError createCrossThreadRingBuffer(int length, int elementSize, LavCrossThreadRingBuffer **destination);
Lav_PUBLIC_FUNCTION int CTRBGetAvailableWrites(LavCrossThreadRingBuffer* buffer);
Lav_PUBLIC_FUNCTION int CTRBGetAvailableReads(LavCrossThreadRingBuffer *buffer);
Lav_PUBLIC_FUNCTION void CTRBGetItems(LavCrossThreadRingBuffer *buffer, int count, void* destination);
Lav_PUBLIC_FUNCTION void CTRBWriteItems(LavCrossThreadRingBuffer *buffer, int count, void* data);

/**These are utilities that operate on the current thread.*/
Lav_PUBLIC_FUNCTION void sleepFor(unsigned int milliseconds); //sleep.
Lav_PUBLIC_FUNCTION void yield(); //yield this thread.

/**An atomic flag that can be either cleared or set.

The point of this is to allow for thraed killing and other such communications.  It is guaranteed to be lock-free on all architectures.*/
Lav_PUBLIC_FUNCTION LavError createAFlag(void** destination);
Lav_PUBLIC_FUNCTION int aFlagTestAndSet(void* flag);
Lav_PUBLIC_FUNCTION void aFlagClear(void* flag);
Lav_PUBLIC_FUNCTION void freeAFlag(void* flag);


/**The following three macros abstract returning error codes, and make the cleanup logic for locks manageable.
They exist because goto is a bad thing for clarity, and because they can.*/

#define WILL_RETURN(type) type return_value; int did_already_lock = 0

#define RETURN(value) do {\
return_value = value;\
goto do_return_and_cleanup;\
} while(0)

#define BEGIN_CLEANUP_BLOCK do_return_and_cleanup:

#define DO_ACTUAL_RETURN return return_value

#define STANDARD_CLEANUP_BLOCK(mutex) BEGIN_CLEANUP_BLOCK \
if(did_already_lock) mutexUnlock((mutex));\
DO_ACTUAL_RETURN

#define LOCK(lock_expression) mutexLock((lock_expression));\
did_already_lock = 1;

#ifdef __cplusplus
}
#endif
