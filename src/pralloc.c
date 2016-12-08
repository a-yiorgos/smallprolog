/* pralloc.c */
/* allocation */
/* 21/12/91 
   dump_stack renamed dump_ancestors 
 */

#include <stdio.h>
#include "prtypes.h"

#define NDEBUG 1 /* turn off checking */
#define PAIRS_TOGETHER 1 
/* Only pairs are allocated on heap.
* This is useful in the case of 8086 architercture.
* as pairs take up a lot of room.
*/
/* define STATISTICS used to see how much each structure takes */
#define DEFAULTSIZE 32000 	/* default memory zone size 	*/
#define BUFFSIZE 1000		/* io buffer sizes		*/

/* error messages */
#define OVERFLOW "overflow"
#define SUBSTSPACE "substitution stack"
#define DYNSPACE "control stack"
#define TRAILSPACE "trail"
#define HEAPSPACE "heap"
#define STRINGSPACE "string zone"
#define TEMPSPACE "temp"
#define SEESTACK "Stack dump?(y/n)"
#define WILDPOINTER "stray pointer!"

/* byte alignment */
#define ALIGN(X) X>>=2,X<<=2,X+=4;

/* #define CAN_ALLOC(HOWMUCH, MAXPTR, CURRPTR)  (((MAXPTR)-(CURRPTR)) > HOWMUCH)*/
#define CAN_ALLOC(HOWMUCH, MAXPTR, CURRPTR)  ((CURRPTR + HOWMUCH) < MAXPTR)
/* This macro is used in the following circumstance:
 *  MAXPTR is the top of a zone,
 *  CURRPTR is the current pointer in the zone, it is less than MAXPTR.
 *  HOWMUCH is an integer that says how much you want to increase CURRPTR
 *  but you must stay less than MAXPTR.
*/

#define ADDRESS_DIFF(PTR1, PTR2) (char *)(PTR1) - (char *)(PTR2)
typedef char *void_ptr_t;

void alloc_err(), fatal_alloc_err(), clean_pred();

#ifdef STATISTICS
/* These numbers let you monitor how much the different 
  * structures consume.
  * It may suprise you to know that integers and variables 
  * consume nothing because they are stored directly in a node.
  */
zone_size_t Atom_consumption;
zone_size_t Pair_consumption;
zone_size_t Node_consumption;
zone_size_t Clause_consumption;
zone_size_t String_consumption;
zone_size_t Predrec_consumption;
#ifdef REAL
zone_size_t Real_consumption;
#endif
#ifdef CHARACTER
zone_size_t Char_consumption;
#endif
#endif


char *Read_buffer;		/* read tokens into this */
char *Print_buffer;		/* used by pr_string */
clause_ptr_t Bltn_pseudo_clause; /* see prlush.c  */
node_ptr_t Printing_var_nodeptr; /* see prprint.c */

static char *Heap_mem; /* bottom of heap */
static char *Heap_ptr; /* allocate from this and move this up */
static char *HighHeap_ptr; /* top of heap */

static char *Str_mem; /* this is used to allocate permanent strings 
			and perhaps other objects if you want to keep the pairs
			together
			*/
static char *Str_ptr; /* allocate from this and move this up */
static char *HighStr_ptr; /* top of zone */

dyn_ptr_t Dyn_mem; /* bottom of control stack 
		   (and zone for those temporary objects that disappear on backtrack,
			although the substitution zone could do as well) */
dyn_ptr_t Dyn_ptr; /* allocate from this and move this up */
dyn_ptr_t HighDyn_ptr; /* top of control stack */

node_ptr_t **Trail_mem; /* the trail (used to reset variable bindings) */
node_ptr_t **Trail_ptr; /* like the others */
node_ptr_t **HighTrail_ptr;/* top of zone */

subst_ptr_t Subst_mem; /* bottom of (global) variable bindings stack */
subst_ptr_t Subst_ptr; /* allocate from this and move this up */
subst_ptr_t HighSubst_ptr;/* top of zone */

temp_ptr_t Temp_mem; /*  For things you might want to 
			create with temp_assert...
			clean with clean_temp */
temp_ptr_t Temp_ptr; /* allocate from this and move this up */
temp_ptr_t HighTemp_ptr;/* top of zone */

int    Max_Readbuffer, Max_Printbuffer;

atom_ptr_t Nil;
node_ptr_t NilNodeptr;

/*******************************************************************************
			ini_alloc()
 Reserve zones and io buffers. 
 ******************************************************************************/
void ini_alloc()
{
	zone_size_t heap_size, str_size, dyn_size, trail_size, subst_size, temp_size;
	extern void_ptr_t os_alloc(); /* see the machine dependent file */
	extern int read_config();   /* see the machine dependent file */

	if(!read_config(&heap_size, &str_size, &dyn_size, &trail_size, &subst_size, &temp_size,
	    &Max_Readbuffer, &Max_Printbuffer))
	{
		heap_size = DEFAULTSIZE;
		str_size = DEFAULTSIZE;
		dyn_size = DEFAULTSIZE;
		trail_size = DEFAULTSIZE;
		subst_size = DEFAULTSIZE;
		temp_size = DEFAULTSIZE;
		Max_Readbuffer = BUFFSIZE;
		Max_Printbuffer =  BUFFSIZE;
	}

	Heap_mem = os_alloc(heap_size);
	Heap_ptr = Heap_mem;
	HighHeap_ptr = Heap_mem + heap_size;

	Str_mem = os_alloc(str_size);
	Str_ptr = Str_mem;
	HighStr_ptr = Str_mem + str_size;

	Dyn_mem = (dyn_ptr_t)os_alloc(dyn_size);
	Dyn_ptr = Dyn_mem;
	HighDyn_ptr = (dyn_ptr_t)((char *)Dyn_mem + dyn_size);

	Trail_mem = (node_ptr_t **)os_alloc(trail_size);
	Trail_ptr = Trail_mem;
	HighTrail_ptr = (node_ptr_t **)((char *)Trail_mem + trail_size);


	Subst_mem = (subst_ptr_t)os_alloc(subst_size);
	Subst_ptr = Subst_mem;
	HighSubst_ptr = (subst_ptr_t)((char *)Subst_mem + subst_size);
	while(Subst_ptr < HighSubst_ptr)
	{
		Subst_ptr->skel = (node_ptr_t)NULL;
		Subst_ptr++;
	}

	Subst_ptr = Subst_mem;

	Temp_mem = (temp_ptr_t)os_alloc(temp_size);
	Temp_ptr = Temp_mem;
	HighTemp_ptr = (temp_ptr_t)((char *)Temp_mem + temp_size);

	Read_buffer = os_alloc((zone_size_t)Max_Readbuffer);
	Print_buffer = os_alloc((zone_size_t)Max_Printbuffer);
}

/*******************************************************************************
			my_Heap_alloc()
 ******************************************************************************/
static void_ptr_t my_Heap_alloc(how_much)
my_alloc_size_t how_much;
{
	void_ptr_t ret;
	ALIGN(how_much);

	if(!CAN_ALLOC(how_much ,HighHeap_ptr, Heap_ptr))
	{
		fatal_alloc_err(HEAPSPACE);
	}
	else
		ret = Heap_ptr;
	Heap_ptr += how_much;
	return(ret);
}

/*******************************************************************************
			my_Str_alloc()
Allocate on permanent string space. 
 ******************************************************************************/
static void_ptr_t my_Str_alloc(how_much)
my_alloc_size_t how_much;
{
	void_ptr_t ret;
	ALIGN(how_much);

	if(!(CAN_ALLOC(how_much, HighStr_ptr, Str_ptr)))
	{
		fatal_alloc_err(STRINGSPACE);
	}
	else
		ret = Str_ptr;
	Str_ptr += how_much;
	return(ret);
}

/********************************************************************************
			my_Dyn_alloc()
Allocate on the control stack. 
This is for objects that disappear on backtracking.
 ******************************************************************************/
dyn_ptr_t my_Dyn_alloc(how_much)
my_alloc_size_t how_much; /* in bytes */
{
	dyn_ptr_t ret;
	ALIGN(how_much);
	if(!(CAN_ALLOC(how_much ,HighDyn_ptr, Dyn_ptr)))
	{
		alloc_err(DYNSPACE);
	}
	else
		ret = Dyn_ptr;
	Dyn_ptr += how_much;
	return(ret);
}

/*******************************************************************************
			my_Trail_alloc()
 Allocate one trail element.
 ******************************************************************************/
node_ptr_t ** my_Trail_alloc()
{
	node_ptr_t ** ret;

	if( Trail_ptr >= HighTrail_ptr)
	{
		alloc_err(TRAILSPACE);
	}
	else
		ret = Trail_ptr;
	Trail_ptr ++;
	return(ret);
}

/*******************************************************************************
			my_Subst_alloc()
Allocate how_much bytes on the substitution stack.
This is more speed-efficient than allocating struct substs on an array of 
 structures, as there is no multiplication.
 ******************************************************************************/
subst_ptr_t my_Subst_alloc(how_much)
my_alloc_size_t how_much; /* in bytes */
{
	subst_ptr_t ret;
#ifndef  NDEBUG
	if(how_much % sizeof(struct subst))INTERNAL_ERROR("alignment");
#endif	
	if(! CAN_ALLOC(how_much ,HighSubst_ptr, Subst_ptr))
	{
		alloc_err(SUBSTSPACE);
	}
	else
		ret = Subst_ptr;
	Subst_ptr += how_much;
	return(ret);
}

/********************************************************************************
			my_Temp_alloc()
Allocate in temporary memory.
This is for objects that disappear
when you clean that zone.
 ******************************************************************************/
temp_ptr_t my_Temp_alloc(how_much)
my_alloc_size_t how_much; /* in bytes */
{
	temp_ptr_t ret;
	ALIGN(how_much);
	if(!(CAN_ALLOC(how_much, HighTemp_ptr, Temp_ptr)))
	{
		fatal_alloc_err(TEMPSPACE);
	}
	else
		ret = Temp_ptr;
	Temp_ptr += how_much;
	return(ret);
}

/*******************************************************************************
			my_alloc()
 Allocate anywhere.
 ******************************************************************************/
char *my_alloc(how_much, status)
my_alloc_size_t how_much;
int status;
{
#ifdef DEBUG
char buff[80];
sprintf(buff, "my_alloc allocating %d of type %d\n",
#endif
	switch(status)
	{
	case PERMANENT:
		return(my_Heap_alloc(how_much));
	case DYNAMIC:
		return(my_Dyn_alloc(how_much));
	case TEMPORARY:
		return(my_Temp_alloc(how_much));
	case PERM_STRING:
		return(my_Str_alloc(how_much));
	default:
		INTERNAL_ERROR("status");
		return(NULL);/* for lint */
	}
}

/*******************************************************************************
			offset_subst()
 Check your compiler on this !! This should return the difference
 between two far pointers 
 ******************************************************************************/

long offset_subst(substptr)
subst_ptr_t substptr;
{
	return((long)(substptr - (subst_ptr_t)Subst_mem));
}

/*******************************************************************************
			get_string()
Allocate a string of length (how_much - 1)
 ******************************************************************************/
string_ptr_t get_string(how_much, status)
my_alloc_size_t how_much;
{
#ifdef STATISTICS
	String_consumption += how_much;
#endif
	return((string_ptr_t)my_alloc(how_much, status));
}

#ifdef REAL
/*******************************************************************************
			get_real()
 There is no need for a get_integer, as integers fit in a node.
 ******************************************************************************/
real_ptr_t get_real(status)
{
#ifdef STATISTICS
	Real_consumption += sizeof(real);
#endif
#ifdef PAIRS_TOGETHER
	if(status == PERMANENT)status = PERM_STRING;
#endif
	return((real_ptr_t)my_alloc((my_alloc_size_t)sizeof(real), status));
}
#endif

/*******************************************************************************
			get_atom();
 ******************************************************************************/
atom_ptr_t get_atom(status)
{
#ifdef STATISTICS
	Atom_consumption += sizeof(struct atom);
#endif
#ifdef PAIRS_TOGETHER
	if(status == PERMANENT)status = PERM_STRING;
#endif
	return((atom_ptr_t)my_alloc((my_alloc_size_t)sizeof(struct atom), status));
}

/*******************************************************************************
			get_pair()
 ******************************************************************************/
pair_ptr_t get_pair(status)
{
#ifdef STATISTICS
	Pair_consumption += sizeof(struct pair);
#endif
	return((pair_ptr_t)my_alloc((my_alloc_size_t)sizeof(struct pair), status));
}

/*******************************************************************************
			get_node()
It might be an idea to actually allocate a pair and return the pointer 
to the head. This would simplify garbage collection.
 ******************************************************************************/
node_ptr_t get_node(status)
{
#ifdef STATISTICS
	Node_consumption += sizeof(struct node);
#endif
#ifdef PAIRS_TOGETHER
	if(status == PERMANENT)status = PERM_STRING;
#endif
	return((node_ptr_t)my_alloc((my_alloc_size_t)sizeof(struct node), status));
}

/*******************************************************************************
			 get_clause();
 Allocate a clause.
 ******************************************************************************/
clause_ptr_t get_clause(status)
{
clause_ptr_t result;
#ifdef STATISTICS
	Clause_consumption += sizeof(struct clause);
#endif
#ifdef PAIRS_TOGETHER
	if(status == PERMANENT)status = PERM_STRING;
#endif
result = (clause_ptr_t) my_alloc((my_alloc_size_t)sizeof(struct clause), status);
if(status == TEMPORARY)
  CLAUSEPTR_FLAGS(result) = 0x1;
else
  CLAUSEPTR_FLAGS(result) = 0x0;
	return( result );
}

/*******************************************************************************
				get_pred()
This is called whenever a new predicate is defined.
 ******************************************************************************/
pred_rec_ptr_t get_pred()
{
	pred_rec_ptr_t ret;
	int status = PERMANENT;

#ifdef STATISTICS
	Predrec_consumption += sizeof(struct predicate_record);
#endif
#ifdef PAIRS_TOGETHER
	if(status == PERMANENT)status = PERM_STRING;
#endif
	ret = (pred_rec_ptr_t)my_alloc((my_alloc_size_t)sizeof(struct predicate_record), status);
	ret->next_pred = NULL;
	ret->atom = Nil;
	return(ret);
}

/*******************************************************************************
				alloc_err()
 ******************************************************************************/
void alloc_err(s)
char *s;
{
	extern dyn_ptr_t Parent;
	char msg[100];

	sprintf(msg, "%s %s\n", s, OVERFLOW);
	errmsg(msg);
	tty_pr_string(SEESTACK);
	if(read_yes())
	  dump_ancestors(Parent);
	exit_term();
	exit(1);
}
/*******************************************************************************
			fatal_alloc_err()
 ******************************************************************************/
void fatal_alloc_err(s)
char *s;
{
	extern dyn_ptr_t Parent;
	char msg[100];

	sprintf(msg, "%s %s\n", s, OVERFLOW);
	errmsg(msg);
	exit_term();
	exit(1);
}
/*******************************************************************************
			check_object()
This should be used to check the internal consistency of the interpreter.
It doesnt make much sense on an IBM PC, because you can only meaningfully 
compare pointers from the same segment.
 ******************************************************************************/
int check_object(objptr)
char *objptr;
{
	if(((objptr >= HighHeap_ptr)||(objptr < Heap_mem))
	    && ((objptr >= HighDyn_ptr)||(objptr < Dyn_mem))
	    && ((objptr >= HighTemp_ptr)||(objptr < Temp_mem))
	    && ((objptr >= HighStr_ptr)||(objptr < Str_mem))
	    )
	{
		errmsg(WILDPOINTER);

#ifndef HUNTBUGS
		exit_term();
                exit(1);
#endif
		return(0);
	}
	else
	return(1);
}


/*********************************************************************
			clean_temp()
Clean the temporary zone.
***************************************************************************************************************************************/
void clean_temp()
{
	extern pred_rec_ptr_t First_pred;
	extern atom_ptr_t LastBuiltin;
	pred_rec_ptr_t predptr;

	for(predptr = First_pred; predptr != NULL; predptr = predptr->next_pred)
	{
		if(predptr->atom > LastBuiltin)
			clean_pred(predptr->atom);
	}/* for */
	Temp_ptr = Temp_mem;
}

/*********************************************************************
			clean_pred()
Clean all temporary clauses from an atom.
***************************************************************************************************************************************/
void clean_pred(atomptr)
atom_ptr_t atomptr;
{
	clause_ptr_t previous, first, clauseptr;
#ifdef DEBUG
	tty_pr_string(ATOMPTR_NAME (atomptr));
	tty_pr_string(" cleaned \n");
#endif
	clauseptr = ATOMPTR_CLAUSE(atomptr);
	first = clauseptr;
	while(first != NULL && IS_TEMPORARY_CLAUSE(first))
	{
		first = CLAUSEPTR_NEXT(first);
	}
	ATOMPTR_CLAUSE(atomptr) = first;
	if(first == NULL)return;
	else
		clauseptr = first;
	while(clauseptr)
	{
		while(clauseptr && !(IS_TEMPORARY_CLAUSE(clauseptr)))
		{
			previous = clauseptr;
			clauseptr = CLAUSEPTR_NEXT(clauseptr);
		}
		if(clauseptr == NULL)return;
		else
			while(clauseptr && IS_TEMPORARY_CLAUSE(clauseptr))
			{
				clauseptr = CLAUSEPTR_NEXT(clauseptr);
			}
		CLAUSEPTR_NEXT(previous) = clauseptr;
	}
}

/*********************************************************************************
			space_left()
 
 ********************************************************************************/
void space_left(ph, pstr, pd, ps, ptr, pte)
zone_size_t *ph, *pstr, *pd, *ps, *ptr, *pte;
{
	*ph = ADDRESS_DIFF(HighHeap_ptr, Heap_ptr);
	*pstr = ADDRESS_DIFF(HighStr_ptr, Str_ptr);
	*pd = ADDRESS_DIFF(HighDyn_ptr, Dyn_ptr);
	*ps = ADDRESS_DIFF(HighTrail_ptr, Trail_ptr);
	*ptr = ADDRESS_DIFF(HighSubst_ptr, Subst_ptr);
	*pte = ADDRESS_DIFF(HighTemp_ptr, Temp_ptr);
}

/*******************************************************************************
			ini_globals().
This is where global structures that are used everywhere are allocated 
 ******************************************************************************/
void ini_globals()
{
	extern FILE * Curr_infile, *Curr_outfile;
	extern node_ptr_t Printing_var_nodeptr;
	atom_ptr_t intern();

	Nil = intern("()");

	Curr_infile = stdin; /* Initially all input is from terminal */
	Curr_outfile = stdout;/* Initially all output is to terminal */

	NilNodeptr = get_node(PERMANENT);
	NODEPTR_TYPE(NilNodeptr) = ATOM;
	NODEPTR_ATOM(NilNodeptr) = Nil;

	Bltn_pseudo_clause = get_clause(PERMANENT);/* see prlush.c */
	CLAUSEPTR_GOALS(Bltn_pseudo_clause) = NilNodeptr;
	CLAUSEPTR_HEAD(Bltn_pseudo_clause) = NilNodeptr;

	Printing_var_nodeptr = get_node(PERMANENT); /* used by pr_solution */
	NODEPTR_TYPE(Printing_var_nodeptr) = VAR;
}

/* end of file */
 
