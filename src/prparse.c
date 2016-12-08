/* prparse.c */
/* recursive descent parser for lisp-like syntax 
 * Makes use of scan.
 */

#include <stdio.h>
#include <ctype.h>
#include "prtypes.h"
#include "prlex.h"


#define TOOMANYVARS "too many vars"
#define PARSERRMSG "parsing error"
#define SCAN_ERRMSG "scan error"
#define EOFINEXP "EOF in expression"
#define VARSTOOLONG "the total length of the variable names is too long"
#define BADINT "bad integer"
#ifdef REAL
#define BADREAL "bad real"
#else
#define NOREALS "no reals in this version"
#endif
#define UNEXPECTED "unexpected symbol"
#define NONLISTARG "expected a list"
#define CLOSEBEXPECTED " ) expected"


extern char *Read_buffer;/* pralloc.c */
extern char *Print_buffer;/* pralloc.c */
extern char *parserr();/* error.c */
extern atom_ptr_t Nil;
extern unsigned int Inp_linecount;
extern int scan();

#ifdef CHARACTER
extern CHAR Char_scanned;
#endif

varindx Nvars;

static char *VarNames[MAX_VAR]; /*names of vars used to attribute offsets */
char *Var2Names[MAX_VAR];/* copy of VarNames used to display solution */
static char VarNameBuff[VARBUFSIZE]; /* used to allocate names */
static char *Varbufptr; /* moves along VarNameBuff */
static int Last_token;  /* used by parse so as to avoid lookahead */

/**********************************************************************
			read_list()
Main function of this file.
Reads a list and complains if not a list (and returns NULL).
Returns node_ptr_t to list parsed.
Updates VarNames, Nvars.
 **************************************************************************/
node_ptr_t read_list(status)
int status;
{
void ini_parse();
node_ptr_t nodeptr, get_node(), parse();

ini_parse();
nodeptr = get_node(status);

if(parse(FALSE, status, nodeptr) == NULL)
   return(NULL);

if(NODEPTR_TYPE(nodeptr) != PAIR)
   {
   errmsg(NONLISTARG);
   return(NULL);
   }


return(nodeptr);
}

/****************************************************************************
			ini_parse()
 ****************************************************************************/
void ini_parse()
{
register int i;

for(i = 0; i < MAX_VAR; i++)
   VarNames[i] = NULL;

Varbufptr = VarNameBuff;
Nvars = 0;
}


/****************************************************************************
			parse()
 Returns NULL if parse failed.
 ****************************************************************************/

node_ptr_t parse(use_Last_token, status, nodeptr)
int use_Last_token, /* a flag: use the global, dont start with a scan */
status; /* PERMANENT OR DYNAMIC etc */
node_ptr_t nodeptr;/* *nodeptr gets modified by this function*/
{
atom_ptr_t intern();
real_ptr_t find_real();
string_ptr_t find_string();
varindx find_offset();
pair_ptr_t the_list, parse_list();
int toktype, next_token;
objtype_t type;

if(use_Last_token == FALSE)
   do{	/* skip spaces */
   toktype = scan();

   if(toktype == EOF)
      return(NULL);

   }
while(toktype < 33 && isspace(toktype));
else 
   toktype = Last_token;
switch(toktype)
{
case TOKEN_INT:
   type = INT;
   if(!sscanf(Read_buffer, "%ld", &(NODEPTR_INT(nodeptr))))
      return (node_ptr_t)parserr(BADINT);
   break;	

case TOKEN_REAL:
#ifdef REAL
   type = REAL;
   NODEPTR_REALP(nodeptr) = find_real(Read_buffer, status);
   break;
#else
   return(node_ptr_t) parserr(NOREALS);
#endif

case TOKEN_ATOM:
   type = ATOM;
   NODEPTR_ATOM(nodeptr) = intern(Read_buffer);
   break;

case TOKEN_VAR:
   type = VAR;
   if((NODEPTR_OFFSET(nodeptr) = find_offset(Read_buffer)) == -1)
      {
      return(NULL);
      }
   break;

case TOKEN_STRING:
   type = STRING;
   NODEPTR_STRING(nodeptr) = find_string(Read_buffer, status);
   break;

#ifdef CHARACTER
case TOKEN_CHAR:
   type = CHARACTER;
   NODEPTR_CHARACTER(nodeptr) = Char_scanned;
   break;
#endif
case SCAN_ERR:
   return (node_ptr_t)parserr(UNEXPECTED);

case '(':
   next_token = scan();

   if(next_token == ')')
      {
      type = ATOM;
      NODEPTR_ATOM(nodeptr) = Nil;
      break;
      }
   else
      type = PAIR;
   Last_token = next_token;
   the_list = parse_list(status);

   if(the_list == NULL)
      {
      return(NULL);
      }

   NODEPTR_PAIR(nodeptr) = the_list;
   break;

case EOF:
   return((node_ptr_t)parserr(EOFINEXP));

default:
   return (node_ptr_t)parserr(UNEXPECTED);
} /* end switch */

NODEPTR_TYPE(nodeptr) = type;
return(nodeptr);
}

/***************************************************************************
			getvarname()
****************************************************************************/
char *getvarname(s)
char *s;
{
char *ret;
int how_long;

how_long = strlen(s) + 1;
ret = Varbufptr;

if(how_long >= (VarNameBuff  + VARBUFSIZE) -ret )
   {
   return parserr(VARSTOOLONG);
   }
else
   strcpy(ret, s);
Varbufptr += how_long;
return(ret);
}

/******************************************************************
			copy_varnames()
Keep a copy of the names of the variables for an answer to a query.
 *******************************************************************/
copy_varnames()
{
int i;
char *get_string();

for(i = 0; i < Nvars; i++)
   {
   Var2Names[i] = get_string((my_alloc_size_t)(strlen(VarNames[i]) + 1), DYNAMIC);
   strcpy(Var2Names[i], VarNames[i]);
   }
}

/****************************************************************************
			find_offset()
Finds an offset for a variable.
 ****************************************************************************/
varindx find_offset(s)
char *s;
{
int i;
char *the_name;
if(!strcmp(s, "_"))
   {

   if(Nvars >= MAX_VAR)
      {
      parserr(TOOMANYVARS);
      return( -1);
      }
   else
      VarNames[Nvars] = getvarname(s);
   Nvars++;
   return((Nvars - 1) * sizeof(struct subst));
   }

for(i = 0; i < Nvars; i++)
   {
   if(VarNames[i] == NULL)break;
   if(!strcmp(s, VarNames[i]))
      {
      return(sizeof(struct subst) * i);
      }
   }

if(Nvars == MAX_VAR)
   {
   parserr(TOOMANYVARS);
   return(-1);
   }

if((the_name = getvarname(s)) == NULL)
   return(-1);

VarNames[i] = the_name; 
Nvars++;
return(sizeof(struct subst) * i);/* do this multiplication once now 
							rather than at runtime*/

}

#ifdef REAL
/****************************************************************************
			find_real()
Find a real corresponding to a string.
 ****************************************************************************/

real_ptr_t find_real(s, status)
char *s;
{
double atof();
real_ptr_t dp, get_real();


dp = get_real(status);
*dp = atof(s);
 /* on error return (real_ptr_t)parserr(BADREAL); */
return(dp);
}
#endif

/****************************************************************************
			find_string()
 Allocate a string for an input.
 ****************************************************************************/
string_ptr_t find_string(s, status)
char *s;
int status;
{
string_ptr_t s1, get_string();

if(status == PERMANENT)
   status = PERM_STRING;
s1 = get_string((my_alloc_size_t)(strlen(s) + 1), status);
strcpy(s1, s);
return(s1);
}


/****************************************************************************
			parse_list()
 Called by parse.
 ****************************************************************************/

pair_ptr_t parse_list(status)
{
pair_ptr_t the_list, pairptr, get_pair();
node_ptr_t headptr, tailptr;

int next_token;

the_list = get_pair(status);
pairptr = the_list;

   do{
   headptr = &(pairptr->head);
   tailptr = &(pairptr->tail);

   if(parse(TRUE, status, headptr) == NULL)
      {	
      return(NULL);
      }
   else
      next_token = scan();

   if(next_token == ')')
      {
      NODEPTR_TYPE(tailptr) = ATOM;
      NODEPTR_ATOM(tailptr) = Nil;
      return(the_list);
      }

   if(next_token == CONS)
      {
      if(!parse(FALSE, status, tailptr))
	 {
	 return(NULL);
	 }
      if(scan() != ')')
	 {
	 return (pair_ptr_t)parserr(CLOSEBEXPECTED);/* move past        */
	 }
      else
	 return(the_list);
      }
   else
      pairptr = get_pair(status);
   NODEPTR_TYPE(tailptr) = PAIR;
   NODEPTR_PAIR(tailptr) = pairptr;
   Last_token = next_token;
   /* continue */
   }
while (1);
return(the_list);/* for dumb lints only */
}


/* This can be used to return the name of the nth var but
 * since it relies on the global variable VarNames this
 * function must be called before the next call of ini_parse
 * as in read_goals or read_list.
 * It is used by a builtin
 */
char *var_name(i)
varindx i;
{
extern varindx Nvars;

if(i >= Nvars)
   return(NULL);
else 
   return(VarNames[i]);
}

/* end of file */
