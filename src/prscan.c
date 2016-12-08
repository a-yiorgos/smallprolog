/* prscan.c */
/* lexical analysis */
#include <stdio.h>
#include <ctype.h>

#include "prtypes.h"
#include "prlex.h"

#define EOFINCOMMENT "End of file in comment "
#define EOFINCHAR "End of file in char"

extern char *Read_buffer;
extern char *Print_buffer;
extern int Max_Readbuffer;

#if LOGGING_CAPABILITY
extern FILE *Log_file;
int Unget_flag = 0;
#endif

FILE *Curr_infile; /* initialised in pralloc.c */

static char *Rbuffptr;
CHAR Ch = '\0';
static void scan_identifier(), scan_string();
static int scan_number();


#ifdef CHARACTER
CHAR Char_scanned;
static int scan_character();
#endif

#if STRING_READ_CAPABILITY
/* the following two variables let you read from a string */

char *Curr_string_input;   /* this is where we would get the next char */
int String_input_flag = 0; /* if this is 0 then read from a file */

#endif

/* Character types - see prlex.h.
 * There is a bit of guess work once we go past 127 
 */
int Ctype[256] = {
	CC, CC, CC, CC, CC, CC, CC, CC, CC, SP,  /* O - 9 */
	SP, CC, CC, SP, CC, CC, CC, CC, CC, CC,  /* 10 - 19 */
	CC, CC, CC, CC, CC, OT, OT, OT, OT, OT,  /* 20 - 29 */
	OT, OT, SP, OT, QU, OT, OT, OT, OT, AP,  /* 30 - 39 */
	BR, BR, OT, SI, OT, SI, OT, OT, DI, DI,  /* 40 - 49 */
	DI, DI, DI, DI, DI, DI, DI, DI, OT, OT,  /* 50 - 59 */
	OT, OT, OT, QE, OT, AU, AU, AU, AU, AU,  /* 60 - 69 */
	AU, AU, AU, AU, AU, AU, AU, AU, AU, AU,  /* 70 - 79 */
	AU, AU, AU, AU, AU, AU, AU, AU, AU, AU,  /* 80 - 89 */
	AU, OT, OT, OT, OT, US, OT, AL, AL, AL,  /* 90 - 99 */
	AL, AL, AL, AL, AL, AL, AL, AL, AL, AL,  /* 100 - 109 */
	AL, AL, AL, AL, AL, AL, AL, AL, AL, AL,  /* 110 - 119 */
	AL, AL, AL, OT, BA, OT, OT, OT, OT, OT,  /* 120 - 129 */
	AL, AL, AL, AL, AL, AL, AL, AL, AL, AL,  /* 130 - 139 */
	AL, AL, AU, AU, AU, AL, OT, AL, AL, AL,  /* 140 - 149 */
	AL, AL, AL, AU, AU, OT, OT, OT, OT, OT,  /* 150 - 159 */
	OT, OT, OT, OT, OT, OT, OT, OT, OT, OT,  /* 160 - 169 */
	OT, OT, OT, OT, OT, OT, OT, OT, OT, OT,  /* 170 - 179 */
	OT, OT, OT, OT, OT, OT, OT, OT, OT, OT,  /* 180 - 189 */
	OT, OT, OT, OT, OT, OT, OT, OT, OT, OT,  /* 190 - 199 */
	OT, OT, OT, OT, OT, OT, OT, OT, OT, OT,  /* 200 - 209 */
	OT, OT, OT, OT, OT, OT, OT, OT, OT, OT,  /* 210 - 219 */
	OT, OT, OT, OT, OT, OT, OT, OT, OT, OT,  /* 220 - 229 */
	OT, OT, OT, OT, OT, OT, OT, OT, OT, OT,  /* 230 - 240 */
	OT, OT, OT, OT, OT, OT, OT, OT, OT, OT,  /* 240 - 249 */
	OT, OT, OT, OT, OT, OT
};

/******************************************************************
			ini_scan()
Called by scan().
*******************************************************************/
static void ini_scan()
{

	lookahead();/* move to next non blank */
	Rbuffptr = Read_buffer;
}


/****************************************************************
		get_nc_char
get non commented character
get next char out of comments and not in string
*****************************************************************/
get_nc_char()
{
	CHAR c;
	static prevc = ' ';
	static inside_comment=0;

	for(;;){
		c =  getc(Curr_infile);
		if(inside_comment)
		{
			if(prevc == '*' && c == '/')
			{
				inside_comment = 0;
				prevc = ' ';
				continue;
			}
			if(c == '\n' || c == '\r')
			{
				return(c);
			}
			if(c == '*')
			{
				c =  getc(Curr_infile);
				if(c == '/')
				{
					inside_comment = 0;
				}
				prevc = c;
				continue;
			}
			else
				if(c == EOF)
				{
					fatal(EOFINCOMMENT);
					return(EOF);
				}
			prevc = c;
			continue;
		}
		else	if/* not inside comments */
		(c == '/')
		{
			c =  getc(Curr_infile);
			if( c == '*')
			{
				inside_comment = 1;
				continue;
			}
			else
			{
				ungetc(c, Curr_infile);
				return((short)'/');
			}
		}
		else
			if(c == EOF)
				return(EOF);
			else
				if(c ==  '\n')
				{
					return(c);
				}
		return(c);
	}
}


/******************************************************************
			getachar()
The only routine you should use for reading a char.
*******************************************************************/

/* too crude */
CHAR getachar()
{
	extern unsigned int Inp_linecount;
#if STRING_READ_CAPABILITY
	if(String_input_flag)
	{
		Ch = *Curr_string_input++;
		if(Ch == '\0')
		{
			Curr_string_input--;
			Ch = EOF;
		}
	}
	else
#endif
		Ch = get_nc_char();

	if(Ch == '\n')
		Inp_linecount ++;
#if LOGGING_CAPABILITY /* new version */

	if(Ch != '\0' && Log_file != NULL && !Unget_flag)
	  {
	  fprintf( Log_file,"%c", Ch);
	  }
	Unget_flag = 0;
#endif
	return(Ch);
}

/******************************************************************
			ungetachar()
Put just one char back on input stream.
Cannot be used without a prior call to getachar().
*******************************************************************/

ungetachar()
{
#if STRING_READ_CAPABILITY
	if(String_input_flag)
		Curr_string_input--;
	else
#endif
		ungetc(Ch, Curr_infile);
Unget_flag = 1;
}
/******************************************************************
			lookahead()
 Peek at next character, 
 but this character can be read by getachar()
 ******************************************************************/
CHAR lookahead()
{
	do{
		getachar();
	}  while(isspace(Ch));

	ungetachar();
	return(Ch);
}



/******************************************************************
			scan()
See prlex.h for return values other than characters (i.e. > 256)
*******************************************************************/
int scan()
{
	ini_scan();
	getachar();
	if(Ch == EOF)
		return(EOF);
	switch(Ctype[Ch])
	{

	case DI:
		MY_ASSERT(isdigit(Ch)); /* double check */
	case SI:
		MY_ASSERT(isdigit(Ch) || Ch == '-' || Ch == '+');
		return(scan_number(Ch));

	case QU:
		MY_ASSERT(Ch == '"');
		scan_string();
		return(TOKEN_STRING);

	case BR:
		MY_ASSERT(Ch == ')' || Ch == '(');
		return(Ch);

#ifdef CLIPS_SYNTAX
	case QE:
		scan_identifier(?);
		return(TOKEN_VAR);
	case AL:
	case AU:
	case OT:
		scan_identifier(Ch);
		return(TOKEN_ATOM);
#else
	case AL:
		MY_ASSERT(islower(Ch));
		scan_identifier(Ch);
		return(TOKEN_ATOM);

	case US:
		MY_ASSERT(Ch == '_');

	case AU:
		scan_identifier(Ch);
		return(TOKEN_VAR);
#endif
	case CC:
		return(SCAN_ERR);
#ifdef  CHARACTER
	case AP:
		return(scan_character());
#endif
	default:
		return(Ch);
	}
}

/******************************************************************
			scan_identifier()
Read an identifier.
******************************************************************/
static void  scan_identifier(c)
int c;
{
	int i;
	*Rbuffptr++ = c;

	for(i = 0; i < Max_Readbuffer; i++)
	{
		*Rbuffptr++ = getachar();
		switch(Ctype[Ch])
		{
		case SP:
		case BR:
		case QU:
		case BA:
			Rbuffptr --;
			ungetachar();
			break;
		default:
			continue;
		}
		break;
	}
	*Rbuffptr = '\0';
}


/******************************************************************
			scan_number()
*******************************************************************/
static int scan_number(c)
char c;
{
	int met_dot = 0;
	int i;

	*Rbuffptr++ = c;
	for(i = 0; i < MAXREALLENGTH; i++)
	{
		*Rbuffptr++ = getachar();
		if(Ch == '.')
			met_dot++;
		if(!isdigit(Ch) && Ch != '.')
		{
			Rbuffptr --;
			ungetachar();
			break;
		}
	}

	*Rbuffptr = '\0';

	switch(met_dot)
	{
	case 0:
		return(TOKEN_INT);
	case 1:
		return(TOKEN_REAL);
	default:
		return(SCAN_ERR);
	}
}

/******************************************************************
			scan_string()
Read a string and only store the characters between the quotes.
To handle embeded quotes double them up.
*******************************************************************/
/* a bit crude ... */
static void scan_string()
{
	int i, c;

	i = 0;
	do{
		c = getachar();
		if(c == EOF)break;
		if(c == STRING_QUOTE)
		{
			c = getachar();
			if(c != '"')/* it really is the end of the string */
			{
				ungetachar();
				break;/* dont store the quote */
			}
		}
		*Rbuffptr++ = c;
	}while(++i < Max_Readbuffer);

	*Rbuffptr = '\0';
}

#ifdef CHARACTER
#define APOSTROPHE '\''

/* scan character */
static int scan_character()
{
	int c;

	*Rbuffptr++ = '\'';
	c = getachar();
	*Rbuffptr++ = c;

	if (c == '\\')
		return(scan_escape());
	    else
		Char_scanned = c;
	if(c == EOF){
		sprintf(Print_buffer, EOFINCHAR );
		errmsg(Print_buffer);
		return(SCAN_ERR);
	}
	else
		c = getachar();
	*Rbuffptr++ = c;
	*Rbuffptr = 0;

	if(c != APOSTROPHE)
		return (SCAN_ERR);
	return(TOKEN_CHAR);
}

/* have just read a \ */
scan_escape(){
int c;
	c = getachar();
	*Rbuffptr++ = c;

	switch(c)
	{
	case 't':
		Char_scanned = '\t';
		break;

	case 'n':
		Char_scanned = '\n';
		break;

	case 'r':
		Char_scanned = '\r';
		break;

	case '\\':
		Char_scanned = '\\';
		break;

	case '\'':
		Char_scanned = '\'';
		break;

	case 'b':
		Char_scanned = '\b';
		break;

	case '"':
		Char_scanned = '\"';
		break;

	case 'f':
		Char_scanned = '\f';
		break;

	case 'v':
		Char_scanned = '\v';
		break;
	default:
		if(isdigit(c))
			return(scan_nescape(c));
		*Rbuffptr = 0;
		return(SCAN_ERR);
	}/* end switch */

c = getachar();
*Rbuffptr = c;
*Rbuffptr = 0;

if(c != '\'')
  return(SCAN_ERR);

return(TOKEN_CHAR);
}

/******************************************************************************
 			scan_nescape()
 Scan rest of character that looks like '\123'
 ******************************************************************************/
scan_nescape(c)
char c; /* first digit */
{
	int i = 1;

	Char_scanned = c - '0';
	do{
		c = getachar();
		i++;
		if(c == APOSTROPHE)
		{
			*Rbuffptr++ = c;
			*Rbuffptr = 0;
			return TOKEN_CHAR;
		
		}
		if((c > '7') || (c < '0'))
		{

			*Rbuffptr = 0;
			return(SCAN_ERR);
		}
		else
		{
			Char_scanned = 8*Char_scanned + c - '0';
			*Rbuffptr++ = c;
		}
	}while(i <= 3);
	if (i > 3)
	{
		*Rbuffptr = 0;
		return(SCAN_ERR);
	}
	else	
		return(SCAN_ERR);
}
#endif

/* end of file */

 
