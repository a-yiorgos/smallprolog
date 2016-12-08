/* prerror.c */
/* handling errors */

#include <stdio.h>
#include "prtypes.h"

#define CRPLEASE "Press Return"

extern char *Print_buffer;
extern atom_ptr_t Predicate; /* from prlush.c */

/****************************************************************************
			parserr()
 A bit crude.
 Parse error messages.
 ****************************************************************************/
char * parserr(s)
char *s;
{
	extern char *Read_buffer;
	extern FILE * Curr_infile;
	extern unsigned int Inp_linecount;

	Read_buffer[80] = '\0'; /* dont print too much rubbish */
	if(Curr_infile != stdin){
		sprintf(Print_buffer, "Parse error,  line %d: %s %s\n", 
					Inp_linecount, s, Read_buffer);
		}
	else
		sprintf(Print_buffer, "Parse error: %s %s\n", s, Read_buffer);
	errmsg(Print_buffer);/* see machine dependent file */
	return(NULL);
}

/************************************************************************
			fatal()
 Deadly error().
 Make sure  that the user has time to see this!
 ************************************************************************/
void fatal(s)
char *s;
{
	sprintf(Print_buffer, "Fatal error %s\n", s);
	errmsg(Print_buffer);
	exit_term();
	puts(CRPLEASE);
	tty_getc();
	exit(1);
}

/************************************************************************
			fatal2()
 Deadly error().
 Make sure  that the user has time to see this!
 ************************************************************************/
void fatal2(s,  s2)
char *s, *s2;
{
	sprintf(Print_buffer, "Fatal error %s %s\n", s, s2);
	errmsg(Print_buffer);
	exit_term();
	puts(CRPLEASE);
	tty_getc();
	exit(1);
}

/************************************************************************
			internal_error().
If this gets called then you (or I) blew it in the C code.
This is called by the macro INTERNAL_ERROR. 
 ************************************************************************/
void internal_error(filename, linenumber, s)
char *filename, *s;
int linenumber;
{
	sprintf(Print_buffer, "Internal Error in source file %s line %d %s\n", 
	    filename, linenumber, s);
	errmsg(Print_buffer);
	exit_term();
	exit(2);
}

/************************************************************************
 			argerr()
 Called by builtins.
 ************************************************************************/
void argerr(narg, msg)
int narg;
char *msg;
{
sprintf(Print_buffer, "argument %d of %s bad should be %s\n", narg, ATOMPTR_NAME(Predicate), msg);
errmsg(msg);
}

/************************************************************************
			nargerr()
 Used by builtins.
 ************************************************************************/
nargerr(narg)
int narg;
{
sprintf(Print_buffer, "Argument %d of %s expected; is missing\n",  narg,  ATOMPTR_NAME(Predicate));
errmsg(Print_buffer);
return(CRASH);
}

/************************************************************************
			typerr()
 Used by builtins.
 ************************************************************************/
/* verify that this is in the the same order as in prtypes.h 
 * or do something more complicated
 */
char *Typenames[] = 
{ "atom", "variable", "string", "integer", "pair", "clause"
#ifdef REAL
	,"real"
#endif
#ifdef CHARACTER
	,"character"
#endif 
};

/* display a message saying the user has made a type error */

int typerr(narg, type)
objtype_t type;
{
if(type < 0 || type > 5)
  INTERNAL_ERROR("illegal type");

sprintf(Print_buffer, "argument %d of %s should be of type %s\n", 
	narg, ATOMPTR_NAME(Predicate), Typenames[type]);
errmsg(Print_buffer);
return(CRASH);
}

/* end of file */

