/* prcnsult.c */
/* load a file of clauses */

#include <stdio.h>
#include "prtypes.h"


#define CANTLOAD "Can't load %s"
#define ERRBUILTIN "Can't redefine builtin %s"
#define HEADNOTATOM "Predicate not atom!"

extern char *Print_buffer;
extern varindx Nvars;
extern FILE *Curr_infile;
extern atom_ptr_t LastBuiltin;
extern int Trace_flag;

void add_to_end();
void record_pred();
void do_listing();

unsigned int Inp_linecount;
pred_rec_ptr_t First_pred, Last_pred;
static int Pred_count = 0;
static char Filename[80];

/******************************************************************
			make_clause()
 ******************************************************************/
clause_ptr_t make_clause(clhead, clgoals, status, predptr)
node_ptr_t clhead, clgoals;
atom_ptr_t *predptr;
{
	clause_ptr_t clauseptr, get_clause();
	ENTER("make_clause");

	clauseptr = get_clause(status);
	CLAUSEPTR_GOALS(clauseptr) = clgoals;
	CLAUSEPTR_HEAD(clauseptr) = clhead;
	CLAUSEPTR_NVARS(clauseptr) = Nvars * sizeof(struct subst);
	CLAUSEPTR_NEXT(clauseptr) = NULL;
		if(NODEPTR_TYPE(clhead) == PAIR)
		{

			clhead = NODEPTR_HEAD(clhead);
			if(NODEPTR_TYPE(clhead) != ATOM)
			  {
			  errmsg(HEADNOTATOM);
			  return(NULL);/* could also indicate which one !! */
			  }
			*predptr = NODEPTR_ATOM(clhead);
		}
	return(clauseptr);
}

/******************************************************************
			do_consult()
 ******************************************************************/

do_consult(infilename,  status)
char *infilename;
int status;
{
	extern node_ptr_t NilNodeptr;
	extern FILE *Curr_outfile;

	FILE *ifp, *save_cif,  *save_cof;
	atom_ptr_t the_pred;
	clause_ptr_t clauseptr;
	node_ptr_t the_list, the_head, read_list();

	ENTER("do_consult");
	Inp_linecount = 0;

	if((ifp = fopen(infilename, "r")) == NULL)
	{
		sprintf(Print_buffer, CANTLOAD, infilename);
		errmsg(Print_buffer);
		return(0);
	}
	else
		save_cif = Curr_infile;
	strcpy(Filename, infilename);
	Curr_infile = ifp;
	save_cof = Curr_outfile;
	Curr_outfile = stdout;
	while((the_list = read_list(status)) != NULL)
	{
		the_head = NODEPTR_HEAD(the_list);
		if(NODEPTR_TYPE(the_head) == ATOM)
		{
			clauseptr = make_clause(the_list, NilNodeptr, status,
			    &the_pred);
		}
		else
			clauseptr = make_clause(the_head, NODEPTR_TAIL(the_list), status, &the_pred);
		if(!clauseptr)
			{
			fclose(Curr_infile);
			return(0);
			}
		if(Trace_flag > 0){
			pr_string("adding \n");
			pr_clause(clauseptr);
			pr_string("\n");
		}
		add_to_end(clauseptr, the_pred);
	}
	fclose(Curr_infile);
	Curr_infile = save_cif;
	Curr_outfile = save_cof;
	return(1);
}

/******************************************************************************
 			load()
 The usual function used to load a file.
 Called in prmain.c
 ******************************************************************************/
load(s)
char *s;
{
ENTER("load");
return(do_consult(s, PERMANENT));
}

/*********************************************************************
		add_to_end()
Adds a clause to the end of its packet.
********************************************************************/
void add_to_end(clauseptr, pred)
clause_ptr_t clauseptr;
atom_ptr_t pred;
{
	clause_ptr_t clp, previous;

	ENTER("add_to_end");
	clp = ATOMPTR_CLAUSE(pred);
	record_pred(pred);

	if(clp == NULL)
	{
		ATOMPTR_CLAUSE(pred) = clauseptr;
	}
	else
	{
		previous = clp;

		while(clp != NULL)
		{
			previous = clp;
			clp = CLAUSEPTR_NEXT(clp);
		}

		CLAUSEPTR_NEXT(previous) = clauseptr;
	}
}

/**********************************************************************
			record_pred() 
Record the atom pointer as a predicate and verify we are not
redefining a primitive.
**********************************************************************/
void record_pred(atomptr)
atom_ptr_t atomptr;
{
	extern unsigned int Inp_linecount;
	pred_rec_ptr_t predptr, get_pred();

	ENTER("record_pred");
	if(atomptr < LastBuiltin)
	{
		sprintf(Print_buffer, "%s line %d ", Filename, Inp_linecount);
		errmsg(Print_buffer);
	
		fatal2("redefining builtin",  ATOMPTR_NAME(atomptr));
	}

	if((atomptr < LastBuiltin) ||
	    (atomptr > LastBuiltin && ATOMPTR_CLAUSE(atomptr) == NULL))
	{
		predptr = get_pred();
		predptr->atom = atomptr;

		if( Pred_count == 0)
		{
			First_pred = predptr;
		}
		else
		{
			Last_pred->next_pred = predptr;
		}
		Last_pred = predptr;
		Pred_count++;
	}
}

/**********************************************************************
		do_listing()
List all clauses on current output file.
**********************************************************************/
void do_listing()
{
	pred_rec_ptr_t predptr;

	ENTER("do_listing");
	for(predptr = First_pred; predptr != NULL; predptr = predptr->next_pred)
	{
		if(predptr->atom > LastBuiltin)
		{
			pr_packet(ATOMPTR_CLAUSE(predptr->atom));
			pr_string("\n");
		}
	}
}

/* end of file */
