#ifndef DEBUG_PRINT_DEFINES_H
#define DEBUG_PRINT_DEFINES_H

/*-------------------------------------
|               INCLUDES               |
--------------------------------------*/
#include "stdio.h"

/*-------------------------------------
|      DEBUG PRINT CONFIGURATION       |
--------------------------------------*/
#ifdef INFO_PRINT      /* Pass in -DINFO_PRINT=1 during compilation for errors, warnings, and debug/info prints */
    #define err_print(fmt, args...)     fprintf(stderr,fmt,##args)     
    #define warning_print(fmt, args...) printf(fmt,##args)
    #define info_print(fmt, args...)    printf(fmt,##args)
#elif WARNING_PRINT     /* Pass in -DWARNING_PRINT=1 during compilation for error and warning prints */
    #define err_print(fmt, args...)     fprintf(stderr,fmt,##args)     
    #define warning_print(fmt, args...) printf(fmt,##args)
    #define info_print(fmt, args...)    (void)0
#elif ERROR_PRINT       /* Pass in -DERROR_PRINT=1 during compilation for error prints */
    #define err_print(fmt, args...)     fprintf(stderr,fmt,##args)     
    #define warning_print(fmt, args...) (void)0
    #define info_print(fmt, args...)    (void)0
#else                   /* Pass in nothing extra during compilation for no prints */
    #define err_print(fmt, args...)     (void)0
    #define warning_print(fmt, args...) (void)0
    #define info_print(fmt, args...)    (void)0
#endif

#endif /* DEBUG_PRINT_DEFINES_H */
