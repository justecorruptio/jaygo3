#ifndef COMMON_H

#include <stdio.h>
#include <stdlib.h>


#ifdef TESTING

#define ASSERT(test) \
    if(!(test)){ \
        fprintf(stderr, "ASSERT FAIL: %s:%d\n", __FILE__, __LINE__); \
        exit(1); \
    }

#else

#define ASSERT(test)

#endif


#define COMMON_H
#endif
