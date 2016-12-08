/* atari.c */
/* machine dependent code for atari */

#include <stdio.h>
#include <ctype.h>
#include "prtypes.h"
#define CANTALLOCATE "cant allocate "
#define CRPLEASE "Carriage return please"
#define NOCONFIGFILE "sprolog.inf missing using default configuration"
#define CONFIG_FILE "sprolog.inf"
#define YESUPPER 'Y'
#define YESLOWER 'y'

extern FILE *Curr_outfile;

int Handle;
int contrl[12];
int intin[128];
int ptsin[128];
int intout[128];
int ptsout[128];

char *os_alloc(how_much)
zone_size_t how_much;
{
	char *ret,*lmalloc();
	if((ret = lmalloc(how_much)) == NULL)
	{
		errmsg(CANTALLOCATE);
		exit_term();
		exit(1);
		return(NULL);/* for stupid finicky compilers and lint */
	}
	else
		return(ret);
}

/* initialise terminal 
 */

ini_term()
{
int i,phys_handle;
short work_in[11],work_out[57],dummy;

appl_init();
phys_handle = graf_handle(&dummy,&dummy,&dummy,&dummy);
Handle = phys_handle;

for(i = 0; i < 10; i++)
   work_in[i]=1;

work_in[10] = 2;
v_opnvwk(work_in, &Handle, work_out);
v_clrwk(Handle);
v_enter_cur(Handle);
}

/* leave terminal */
exit_term()
{
	tty_pr_string(CRPLEASE);
	getchar();
	v_clsvwk(Handle);
	appl_exit();
}

/* output an error dialogue */
errmsg(s)
char *s;
{
/*
static char errbuffer[256];
sprintf(errbuffer,"[1][%s][OK]",s);
form_alert(1,errbuffer);	
*/
tty_pr_string(s);
tty_pr_string("\n");
}


read_config(hs, strs, ds, tp, sbs, tmps, rs, ps)
zone_size_t *hs, *strs, *ds, *tp, *sbs, *tmps;
int *rs, *ps;
{
	FILE *ifp;

	if((ifp = fopen(CONFIG_FILE, "r")) == NULL)
	{
		errmsg(NOCONFIGFILE);
		return(0);
	}

	fscanf(ifp, "%ld %ld %ld %ld %ld %ld %d %d", hs, strs, ds, tp, sbs, tmps, rs, ps);
#ifdef DEBUG
	printf("%ld %ld %ld %ld %ld %ld %d %d", *hs, *strs, *ds, *tp, *sbs, *tmps, *rs, *ps);
	getchar();
#endif
	return(1);
}


tty_pr_string(s)
char *s;
{
#if TRACE_CAPABILITY
extern FILE *Log_file;
	if(Log_file){
	  fprintf(Log_file,"%s",s);
  	  fflush(Log_file);
        }
#endif
	printf("%s",s);
	fflush(stdout);
}

tty_getc()
{
#if TRACE_CAPABILITY
extern FILE *Log_file;
	if(Log_file){
	  fprintf(Log_file,"\n");
        }
#endif
getchar();
}

tty_getche()
{
return Cconin();
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
**************************************************************************/
read_yes()
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

