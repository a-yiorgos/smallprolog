/* prolog.h */
/* environment specific headers */

/* #define STATISTICS *//* just to check consumption */

#define STRING_READ_CAPABILITY 1 /* can read from a string */
#define TRACE_CAPABILITY 1 /* the system can trace */
#define LOGGING_CAPABILITY 1 /* the system can log */
#define CLOCK 1 /* the clock predicate works */
#define MAXOPEN 15 /* maximum number of files simultaneously open (see prbltin.c) in a mode */

/* adjust this to your environment */
#ifdef ATARI
typedef unsigned long zone_size_t;
#endif

#ifdef MSDOS 
typedef unsigned int zone_size_t;
#define SEGMENTED_ARCHITECTURE /* can't compare pointers from different 
				  arrays */
#endif
#ifdef SUN
typedef unsigned int zone_size_t;
#endif

#ifdef GCC
typedef unsigned int zone_size_t;
#endif
#define MAX_LINES 25 /* default number of lines per page */

/**********************************************************************
 * Looking for bugs.
 * This can help debugging but you should probably #undef these
 * and #define (replace) them by your own according to the file.
 **********************************************************************/

#ifdef DEBUG

extern char Debug_buffer[128];
#define HUNTBUGS
/* #define ENTER(X) sprintf(Debug_buffer,"Enter %s\n",X);tty_pr_string(Debug_buffer) */
#define ENTER(X) 
#define BUGHUNT(S) bughunt(__LINE__,__FILE__,S)

#else

#define ENTER(X) 
#define BUGHUNT(S) 

#endif
 
