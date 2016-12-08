/* pp.c */
/* A trivial pretty printer for lisp-like expressions.
 * This might be useful for finding mistakes with parentheses.
 * There is a provision for quotes that wont necessarily apply.
 */

#include <stdio.h>
#include <ctype.h>

#define TOGGLE(Flag) if(Flag)Flag = 0;else Flag = 1;

pp(ifp, ofp)
FILE *ifp, *ofp;
{
	int depth = 0;
	int in_dbquotes = 0;
	int in_quotes = 0;
	int in_comments = 0;
	int line_count = 0;
	int previous;
	int c = 0;

	do{	
		previous = c;
		c = getc(ifp);
		switch(c)
		{
		case '/':
			if(previous == '*')
			{
				if(in_comments)
					in_comments = 0;
				else
					fprintf(stderr, "nested comments? line %d\n", line_count);
			}
			putc(c, ofp);
			break;

		case '*':
			if(previous == '/')
			{
				if(!in_comments )
					in_comments = 1;
				else
					fprintf(stderr, "nested comments? line %d\n", line_count);
			}
			putc(c, ofp);
			break;

		case '(':
			if(!in_dbquotes && !in_quotes)
				depth++;
			putc(c, ofp);
			break;

		case ')':
			if(depth < 0)continue;
			if(!in_dbquotes && !in_quotes)
				depth--;
			putc(c, ofp);
			break;

		case '\n':
			line_count++;

			if(in_quotes )
			{
				fprintf(stderr, "Newline in quotes, line %d\n", line_count);
			}

			if(in_dbquotes )
			{
				fprintf(stderr, "Newline in double quotes %d\n", line_count);
			}

			do{
				c = getc(ifp);
				if(c == '\n'){
					putc(c, ofp);
					line_count++;
				}
			}while(isspace(c));
			putc('\n', ofp);
			switch(c)
			{
			case ')':
/*				depth--;	*/
				indent(depth, ofp);
				break;

			case '(':
				indent(depth, ofp); 	
/*				depth++; 	*/
				break;

			default:
				indent(depth, ofp);
			}

			ungetc(c, ifp);
			break;

		case EOF:
			if(depth > 0)
				fprintf(stderr, "%d ) brackets missing\n", depth);
			if(depth < 0)
				fprintf(stderr, "%d ( brackets missing\n",-depth);
			if(ofp != stdout)fclose(ofp);
			fclose(ifp);
			return;

		case '"':
			if(!in_quotes && !in_comments)TOGGLE(in_dbquotes);
			putc(c, ofp);
			break;

		case '\'':
			if(!in_dbquotes && !in_comments)TOGGLE(in_quotes);
			putc(c, ofp);
			break;

		default:
			putc(c, ofp);
			break;
		}
	}while(1);

}

indent(d, ofp)
int d;
FILE *ofp;
{
	while(d > 0){
		d--;
		fprintf(ofp, "  ");
	}
}

#ifdef TEST
main()
{
	FILE *ifp;
	if((ifp = fopen("test.pro", "r")) == NULL)
	{
		fprintf(stderr, "Cant open %s\n", "test.pro");
		exit(1);
	}
	pp(ifp, stdout);

}

#else
main(argc, argv)
int argc;
char *argv[];
{
	int i;
	FILE *ifp;
	if(argc == 1)
	{
		printf("usage: pp files...\n");
		exit(1);
	}
	for(i = 1; i < argc; i++)
	{
		if((ifp = fopen(argv[i], "r")) == NULL)
		{
			fprintf(stderr, "Cant open %s\n", argv[i]);
			continue;
		}
		pp(ifp, stdout);
	}
}
#endif
/* end of file */
