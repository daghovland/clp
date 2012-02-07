#include "common.h"

#ifdef USE_TIMESTAMP_ARRAY
#include "timestamp_array.c"
#else 
#include "timestamp_linked_list.c"
#endif
