/*
HEADER:		;
TITLE:		Small Prolog;
VERSION:	2.O;

DESCRIPTION:	"Interactive Prolog interpreter with lisp-like syntax.
		Will run on IBM-PC, Atari ST and Sun-3, and should be very
		easy to port to other machines.
		Requires sprolog.ini and sprolog.inf on the same directory."
KEYWORDS:	Programming language, Prolog.
SYSTEM:		MS-DOS v2+, TOS, SUN-OS;
FILENAME:	prmain.c;
WARNINGS:	Better to compile this with compact or large model on the PC.

SEE-ALSO:	pr*.*
AUTHORS:	Henri de Feraudy
COMPILERS:	Turbo C V1.5, Mark Williams Let's C V4,	Quick C 
	        on PC compatibles.
		DJ Delorie's GCC386 on 386 based PC compatibles.
		Mark Williams C V3.0 for the Atari ST ,
		Megamax Laser C on the Atari
		cc on the Sun-OS v3.5
		gcc on a Sun
*/
/* prmain.c */
/* SPROLOG - a public domain prolog interpreter.
 * Design goals: portability, small size, embedability and hopefully 
 * educational.
 * You must add the builtins you need (and remove the ones you don't).
 * Input-output has been left to the trivial minimum.
 * You are encouraged to modify prsun.c to adapt it to your machine.
 * The syntax is LISPish, for reasons of simplicity and small code,
 * but this does have the advantage that it encourages meta-programming
 * (unlike Turbo-Prolog).
 * Very little in the way of space saving techniques have been used (in the
 * present version). There is not yet any tail recursion optimisation or
 * garbage collection.
 */

#include <stdio.h>

#define CRPLEASE "Press Return"

extern void ini_alloc(), ini_term(), ini_hash(), ini_builtin(), ini_globals();

void init_prolog()/* call this once in your application */
{
/* initialise I-O */
	ini_term(); 	/* in machine dependent file 	*/
/* allocate memory zones */
	ini_alloc();	/* in pralloc.c 		*/
/* initialise symbol table */
	ini_hash();	/* in prhash.c	 		*/
/* make builtin predicates */
	ini_builtin();	/* in prbuiltin.c		*/
/* intialise global variables */
	ini_globals();  /* in pralloc.c 		*/
}

main(argc,argv)
int argc;
char *argv[];
{
	int i;
	init_prolog();
	pr_string("SMALL PROLOG 2.O  \n");
	pr_string("by Henri de Feraudy\n");
/* load initial clauses */
	load("sprolog.ini"); /* see prconsult.c for load */

/* load files passed as command line arguments */
	for(i = 1; i < argc; i++)
	   {
	   load(argv[i]);
	   }

/* execute initial query from file query.ini */
	if(initial_query("query.ini"))/* interact with user */
	query_loop();	/* in prlush.c			*/
/* clean up I-O */
	exit_term();	/* in machine dependent file 	*/

/* normal exit */
	exit(0);
}

/* end of file */
