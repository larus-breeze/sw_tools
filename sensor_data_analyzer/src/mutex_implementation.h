#ifndef MUTEX_IMPLEMENTATION_H_
#define MUTEX_IMPLEMENTATION_H_

#include "stdio.h"

class Mutex_Wrapper_Type
{
public:
  void lock( void)
  {
    if( lock_count == 0)
      printf("lock ... ");
    ++lock_count;
  }
  void unlock( void)
  {
    --lock_count;
    if( lock_count == 0)
      printf("release\n");
  }
private:
  unsigned lock_count;
};

#include "scoped_lock.h"

extern Mutex_Wrapper_Type my_mutex;

#define LOCK_SECTION() ScopedLock lock( my_mutex)

#endif /* MUTEX_IMPLEMENTATION_H_ */
