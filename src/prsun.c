/* sun.c */
/* Machine dependent code that worked on a Sun 3.
 */
/* the input-output below is trivial */

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include "prtypes.h"

#define CANTALLOCATE "cant allocate "
#define NOCONFIGFILE "sprolog.inf missing using default configuration"
#define CONFIG_FILE "sprolog.inf"
#define YESUPPER 'Y'
#define YESLOWER 'y'

#if LOGGING_CAPABILITY
extern FILE *Log_file;
#endif

extern FILE *Curr_outfile;

/**************************** os_alloc() **********************************
	Call the system's allocation function.
 ************************************************************************/
char *os_alloc(how_much)
zone_size_t how_much;
{
	char *ret,*malloc();
	if((ret = malloc(how_much)) == NULL)
	{
		errmsg(CANTALLOCATE);
		exit_term();
		exit(1);
		return(NULL);/* for stupid finicky compilers and lint */
	}
	else
		return(ret);
}

static void die()
{
tty_pr_string("You killed me!\n");
exit(1);
}

/**************************** ini_term() **********************************
 Initialise terminal 
  This will generally involve things like initialising curses.
 ************************************************************************/

void ini_term()
{
extern void stack_dump();

signal(SIGINT, stack_dump); /* force a stack dump if user types ^C */
signal(SIGQUIT, die); /* leave if user types ^D */
}

/************************** exit_term() *********************************
 Leave terminal and clean up.
 ******************************************************************************/

exit_term()
{
}

/***************************** errmsg() *********************************
 Output error message.
 ******************************************************************************/

errmsg(s)
char *s;
{
#if LOGGING_CAPABILITY
	if(Log_file){
	  fprintf(Log_file, "%s", s);
  	  fflush(Log_file);
        }
#endif
	fprintf(stdout, "%s\n", s);
}

/************************** tty_pr_string() *********************************
 Output string to terminal.
 ******************************************************************************/

tty_pr_string(s)
char *s;
{
int len;
#if LOGGING_CAPABILITY
	if(Log_file){
	  fprintf(Log_file, "%s", s);
  	  fflush(Log_file);
        }
#endif
	len = printf("%s", s);
	fflush(stdout);
	return(len);
}

/*******************************************************************
			pr_string()
 *******************************************************************/

pr_string(s)
char *s;
{
int len;
#if LOGGING_CAPABILITY
	extern FILE *Log_file;
	if (Log_file != NULL)
	{
	fprintf(Log_file, "%s", s);
	fflush(Log_file);
	}
#endif
	len = fprintf(Curr_outfile, "%s", s);
	fflush(stdout);
return(len);
}

/************************** tty_getc() *********************************
 Read a char from terminal.
 ******************************************************************************/
tty_getc()
{
int c;
c = getchar();
#if LOGGING_CAPABILITY
	if(Log_file){
	  fprintf(Log_file, "%c", c);
        }
#endif
return(c);
}

/****************************tty_getche()***********************************
 raw read with echo (I have tested this one in the past)
 ***************************************************************************/

#include <sgtty.h>


static struct sgttyb ostate;          /* saved tty state */
static struct sgttyb nstate;          /* values for editor mode */
static struct tchars otchars;  /* Saved terminal special character set */
static struct tchars ntchars = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff };


tty_getche()
{
	int c;

	gtty(0, &ostate);/* save old state */
	gtty(0, &nstate);/* get base of new state */
	nstate.sg_flags |= RAW;
	nstate.sg_flags &= ~CRMOD;       
	stty(0, &nstate);                       /* set mode */
	ioctl(0, TIOCGETC, &otchars);           /* Save old characters */
	ioctl(0, TIOCSETC, &ntchars);           /* Place new character into K */

	c = getchar();
	stty(0, &ostate);
	ioctl(0, TIOCSETC, &otchars);
	return c;
}

/**************************** read_config() **********************************
 read the file SPROLOG.INF
 ************************************************************************/

int read_config(hs, strs, ds, tp, sbs, tmps, rs, ps)
zone_size_t *hs, *strs, *ds, *tp, *sbs, *tmps;
int *rs, *ps;
{
	FILE *ifp;

	if((ifp = fopen(CONFIG_FILE, "r")) == NULL)
	{
		errmsg(NOCONFIGFILE);
		return(0);
	}

	fscanf(ifp, "%d %d %d %d %d %d %d %d", hs, strs, ds, tp, sbs, tmps, rs, ps);
#ifdef DEBUG
	printf("%d %d %d %d %d %d %d %d", *hs, *strs, *ds, *tp, *sbs, *tmps, *rs, *ps);
	getchar();
#endif
	return(1);
}

/**************************** more_y_n() **********************************
	Ask for confirmation.
 ************************************************************************/
/* a bit crude ... */
int more_y_n()
{
	tty_pr_string("More ?");
	return(read_yes());
}

/**************************** read_yes() *********************************
		Return 1 iff user types y
**************************************************************************/
int read_yes()
{
int c;

do
 {
 c = tty_getc();
 }while(isspace(c));

while(tty_getc() != '\n');/* read rest of line */

if(c == YESLOWER || c == YESUPPER )
  return(1);
else
 return(0);
}

/* end of file */
