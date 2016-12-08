/* pribmpc.c */
/* machine dependent code for pc and clones */

#include <stdio.h>
#include <ctype.h>
#include "prtypes.h"

#define CANTALLOCATE "cant allocate "
#define CRPLEASE "Carriage return please"
#define NOCONFIGFILE "sprolog.inf missing using default configuration"
#define CONFIG_FILE "sprolog.inf"
#define YESUPPER 'Y'
#define YESLOWER 'y'

extern FILE *Log_file, *Curr_outfile;
extern void exit_term();

/**************************** os_alloc() *********************************/
/* This is the function you use to get memory  from the system.		 */
/* Why not use malloc? Because malloc will only allocate up to 64K  on   */
/* some systems when there is another function around for larger 	 */
/* allocations.								 */
    
char *os_alloc(how_much)
zone_size_t how_much;
{
	char *ret, *malloc();
#ifdef DEBUG
printf("trying to allocate %u\n",how_much);
#endif
	if((ret = malloc(how_much)) == NULL)
	{
		errmsg(CANTALLOCATE);
		exit_term();
		exit(1);
		return(NULL);/* for stupid finicky compilers and lint */
	}
	else
#ifdef DEBUG 
printf("successful\n");
#endif
		return(ret);
}

/******************************************************************************
 Initialise terminal
 This is the function you use to set up your terminal for interaction with
 sprolog. It would change if you wanted to incorporate a line editor for example
 ******************************************************************************/
void ini_term()
{

}

/******************************************************************************
			function exit_term
 This restores the terminal characteristics.
 ******************************************************************************/
void exit_term()
{

}

/***************************** errmsg() *********************************
 Output error message.
 On some systems you might want to output a box-surrounded message.
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

/************************** tty_getc() *********************************
 Read a char from terminal, no matter what the other flags are.
 ******************************************************************************/

int tty_getc()
{
int c;

c = getchar();

#ifdef LOGGING_CAPABILITY
if(Log_file!=NULL)
  {
  fputc(c,Log_file);
  }
#endif

return(c);
}

/* raw read one char , with echo */
int tty_getche()
{
#ifdef MARK_WILLIAMS
return getcnb(); /* may not be on your machine */
#else
return getche(); /* may not be on your machine */
#endif
}

/************************** tty_pr_string() *********************************
 Output string to the terminal no matter what the other flags are.
 ******************************************************************************/

int tty_pr_string(s)
char *s;
{
int len;
#ifdef LOGGING_CAPABILITY
if(Log_file  != NULL)
  {
	fprintf(Log_file,"%s",s);
	fflush(Log_file);
  }
#endif
	len = printf("%s",s);
	fflush(stdout);
return (len);
}

/*******************************************************************
			pr_string()
 This will output a string to the current output file.
 It also returns the number of chars output.
 It should be the only way your functions can output a string.
 *******************************************************************/

int pr_string(s)
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

/**************************** read_config() **********************************
 this reads the file SPROLOG.INF so that the sizes of the various 
  memory zones can be set.
 ************************************************************************/

int read_config(hsp, strsp, dsp, tsp, sbsp, tmpsp, rsp, psp)
zone_size_t *hsp, *strsp, *dsp, *tsp, *sbsp, *tmpsp;
int *rsp, *psp;
{
#ifdef MARK_WILLIAMS
long  hs, strs, ds, ts, sbs, tmps;
#endif
	FILE *ifp;

	if((ifp = fopen(CONFIG_FILE, "r")) == NULL)
	{
		errmsg(NOCONFIGFILE);
		return(0);
	}
#ifdef MARK_WILLIAMS
fscanf(ifp, "%D %D %D %D %D %D %d %d", &hs, &strs, &ds, &ts, &sbs, &tmps, rsp, psp);
	*hsp = hs;
	*strsp = strs;
	*dsp = ds;
	*tsp = ts;
	*sbsp = sbs;
	*tmpsp = tmps;

#else
	fscanf(ifp, "%u %u %u %u %u %u %u %u", hsp, strsp, dsp, tsp, sbsp, tmpsp, rsp, psp);
#endif
#ifdef DEBUG
	printf("%u %u %u %u %u %u %u %u", *hsp, *strsp, *dsp, *tsp, *sbsp, *tmpsp, *rsp, *psp);
	getchar();
#endif
	return(1);
}


/**************************** more_y_n() **********************************
	Ask for confirmation.
 ************************************************************************/
/* a bit crude ... */
more_y_n()
{
	tty_pr_string("More ?");
	return(read_yes());
}

/**************************** read_yes() *********************************
		Return 1 iff user types y
Ignores characters after first non blank.
You could use a dialogue box in a windowing environment.
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

#ifdef LINE_EDITOR /* some day ... */
/* this function returns the x and y position of the cursor.
   It's not used but you might want to make use of it to extend
   the input.
 */

#ifdef __TURBOC__
getxy(px, py)
int *px, *py;
{
*px = wherex();
*py = wherey();
}
#else

#include <dos.h>
void gotoxy(X, Y)
short X, Y;
{
union REGS inregs;
union REGS outregs;
           /*
inregs.h.ah = 0x0F;
int86(16, &inregs, &outregs);
             */
inregs.h.ah = 0x02;
inregs.h.bh = 0 ;/* outregs.h.bh; the active page  */
inregs.h.dh = Y;
inregs.h.dl = X;
int86(16/* video */, &inregs, &outregs);
}

void getxy(px, py)/* get cursor position */
short *px, *py;
{
union REGS inregs, outregs;

inregs.h.ah = 0x0F;
int86(16, &inregs, &outregs);

inregs.h.bh = outregs.h.bh; 
inregs.h.ah = 0x03;

int86(16, &inregs, &outregs);
*py = outregs.h.dh;
*px = outregs.h.dl;
}
#endif
#endif

#ifdef __TURBOC__
#include <time.h>
/* in hundreths of a second */
long clock()
{
return (100* clock()) /CLK_TCK;
}
#else
long  clock()
{
errmsg(" Clock not implemented  on  PC");
/* some recent C User Journal article has a version, I think. */
return(0L);
}
#endif
/* end of file */
