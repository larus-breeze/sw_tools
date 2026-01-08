#ifndef MUTEX_IMPLEMENTATION_H_
#define MUTEX_IMPLEMENTATION_H_

#include "stdio.h"

class Mutex_Wrapper_Type
{
public:
  void lock( void)
  {
    printf("lock ... ");
  }
  void unlock( void)
  {
    printf("release\n");
  }
};

#include "scoped_lock.h"

extern Mutex_Wrapper_Type my_mutex;

#define LOCK_SECTION() ScopedLock lock( my_mutex)

#endif /* MUTEX_IMPLEMENTATION_H_ */
