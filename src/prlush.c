/* prlush.c */
/* Lush resolution .
 * Along with unification these are the most important routines in Prolog.
 * If you want to embed Small Prolog then you won't need query_loop as such.
 * See Chris Hogger's "An Introduction to Logic Programming" (Academic Press)
 * if you are hungry for more explanation.
 */
/* Small Prolog uses a control stack , a substitution stack and a 
 * "trail" to represent its run-time state.
 * The control stack is the stack of "activation records".
 * A clause packet is the linked list of clauses that correspond to
 * a predicate. It's prolog's equivalent of a procedure.
 * You enter a procedure when a goal is unified successfully with the
 * head of a clause. You can enter the procedure at different clauses.
 * Execution tries the clauses in the order of their occurence in the
 * list.
 * Each time a procedure (clause packet) is entered, a corresponding "frame"
 * (or what is called "activation record" in languages like C)
 * is pushed on the control stack, and a corresponding frame is pushed
 * on the substitution stack(representing the parameters).
 * However the situation is much more complicated than what occurs in 
 * familiar languages because a procedure might have to be redone
 * (with the next clause of the packet) after exit of the procedure.
 * So you cant just pop the frame on successful return.
 * Popping the frame only occurs on backtracking, unless you implement
 * an optimisation that recognises that no more clauses could be tried.
 * I have not implemented this optimisation in the current version, sorry.

*  Each frame must remember its parent: a pointer to the frame of
 * the clause which contains the goal that led to the current clause.
 * This is used when every goal of the clause has been successfully solved.
 * This is not necessarily the previous frame because the previous frame 
 * comes from the elder brother goal.
 * We need to remember also the next goal to try if the current goal
 * ends up being successful.
 * We need to remember the last "backtrack point": this is a frame
 * where there seemed to be the possibility of another solution
 * for its procedure.
 * We backtrack to that point when there is a failure.
 * There is a global variable that does this. But this variable
 * will have to be updated when we backtrack so we need to save
 * the previous values of it on the stack too. To save a little
 * space only some frames need to store the last backtrack point,
 * These are "non-deterministic frames", frames in which there
 * was a remaining candidate to the current clause.
 * Sometimes substitions are created low in the substitution stack
 * -before the environment of the current goal. This is why
 * we use a trail to be able to unset these substitutions on backtracking.
 * We have been lazy and recorded all substitutions on the 
 * trail.
 */
    
   
 
#include <stdio.h>
#include <setjmp.h> /* added this on Dec 21 1991 */
#include <assert.h>  /* added this on Dec 21 1991 */
#include "prtypes.h"
#include "prlush.h"


#define NOTVARPRED "A variable can't be used as a predicate\n"
#define NOPRED "Predicate not atom\n"
#define STACKCONTENTS "Ancestors of current goal:\n"
#define INIQUERY "Syntax error ini initial query"
#define STRINGQUERY "Syntax error ini query passed as string"

#if TRACE_CAPABILITY
/*  values of where in tracing */
#define G_BTK  		'B' /* goto bactrack */
#define G_AGA		'P' /* goto AGAIN */
#define KEEP_GOING	'K'

#define TRACE(X) if(Trace_flag > 0){\
		 if(X == 0)return ABNORMAL_LUSH_RETURN;\
		switch(where){case G_AGA:goto AGAIN;case G_BTK:goto BACKTRACK;}}
		 
#define CAN_SKIP 1
#define CANT_SKIP 0
#else
#define TRACE(X)
#endif

/* Although the Trace_flag can be set by a builtin
   actual tracing can be suspended while stepping over a call .
  The following two variables are used for this purpose.
 */
static int Copy_tracing_now;

#ifdef HUNTBUGS
	extern int Bug_hunt_flag;
#endif


extern subst_ptr_t DerefSubst; /* enviroment of last dereferenced 
				object  */
extern subst_ptr_t my_Subst_alloc();
extern node_ptr_t DerefNode; /* skeleton of last dereferenced object */
extern node_ptr_t NilNodeptr; /* node corresponding to empty list */
extern atom_ptr_t Nil;		/* object  of empty list */
extern clause_ptr_t Bltn_pseudo_clause;
extern dyn_ptr_t HighDyn_ptr,Dyn_mem, Dyn_ptr, my_Dyn_alloc(); /* for control stack */
extern subst_ptr_t Subst_mem, Subst_ptr; /* for substitution stack */
extern node_ptr_t **Trail_mem, **Trail_ptr;
extern FILE * Curr_outfile;


void ini_lush(), reset_zones();

clause_ptr_t Curr_clause; /* current clause containing Goals */
clause_ptr_t Candidate;   /*Used when we look for a clause whose
			 head might match goal */
dyn_ptr_t QueryCframe;   /* Control frame of Query */
dyn_ptr_t LastCframe; /* most recent Cframe */
dyn_ptr_t LastBack; /* points to cframe of most recent backtrack point */
dyn_ptr_t Parent;   /* parent Cframe of current goal */
dyn_ptr_t BackFrame; /* parent frame in case of reverse tracing */
node_ptr_t Goals; /* what remains of current goals of the clause.
			The head of the list is the goal to satisfy first.
			Note that there will be (in general) other goals to 
			satisfy on completion of solution of Goals
		 */
node_ptr_t Query; /* the initial query */
node_ptr_t Arguments;/* arguments of current goal (points to a list) */
subst_ptr_t Subst_goals, /* substituion env of Goals 		*/
SubstGoal, /* can be different to Subst_goals if there is a "call" */
OldSubstTop; /* for backtracking */
atom_ptr_t Predicate; /* predicate of current goal 	*/
node_ptr_t **OldTrailTop;
int ErrorGlobal; /* used to communicate the fact there was an error */
int Deterministic_flag; /*used to minimise "more?" questions after a solution */
int ReverseTraceMode = 0; /* if set to 1 means you can trace backwards 
			  but this is greedy on control stack space 
			  because all frames are like the non-deterministic
			  frames.
			*/
integer Nunifications = 0; /* just for statistics */

/******************************/
/* Dec 21 1991 new variables: */
/******************************/

node_ptr_t ND_builtin_next_nodeptr;/* This is for 
	non deterministic builtins. When this variable is NULL it
	is the first call of the predicate. Otherwise it is the nodeptr
	 that guides the non deterministic builtin.
	*/
jmp_buf Bltin_env; /* see do_builtin() */
jmp_buf Query_jenv; /* see query_loop() */
int QJusable = 0; /* Determines if we can longjump to Query_jenv */

#if TRACE_CAPABILITY
extern int Trace_flag; /* sets trace mode */
extern int Tracing_now; /* sets trace mode */
dyn_ptr_t Skip_above;
int Unleash_flag = 0;
#endif


/*******************************************************************
		read_goals()
 Called by query_loop.
 Updates Goals, Nvars and Query. 
 *******************************************************************/
read_goals(ifp)
FILE *ifp;
{
	extern node_ptr_t read_list(), get_node();
	extern FILE * Curr_infile;

	node_ptr_t head;
	FILE * save_cif;

	ENTER("read_goals");
	save_cif = Curr_infile;
	Curr_infile = ifp;
	Goals = read_list(PERMANENT);
	if(Goals == NULL)return(0);
	copy_varnames();
	head = NODEPTR_HEAD(Goals);

	if(NODEPTR_TYPE(head) == ATOM)
	{
		pair_ptr_t pairptr, get_pair();

		pairptr = get_pair(DYNAMIC);
		NODEPTR_TYPE(PAIRPTR_HEAD(pairptr)) = PAIR;
		NODEPTR_PAIR(PAIRPTR_HEAD(pairptr)) = NODEPTR_PAIR(Goals);
		NODEPTR_TYPE(PAIRPTR_TAIL(pairptr)) = ATOM;
		NODEPTR_ATOM(PAIRPTR_TAIL(pairptr)) = Nil;
		Goals = get_node(DYNAMIC);
		NODEPTR_TYPE(Goals) = PAIR;
		NODEPTR_PAIR(Goals) = pairptr;
	}

	Query = Goals;
	Curr_infile = save_cif;

	return(1);
}


/******************************************************************************
			initial_query
 This executes the query from a file . It is silent if the file does not exist.
 ******************************************************************************/
initial_query(filename)
char *filename;
{
	FILE *ifp;
	
	ENTER("initial_query");
	if((ifp = fopen(filename, "r")) == NULL)
  	    return(1);		/* silent */
	reset_zones();
	Skip_above = HighDyn_ptr;
	if(!read_goals(ifp))
	{
		fatal(INIQUERY);
	}
	fclose(ifp);
	ini_lush();
	return(lush(TRUE) != FINAL_LUSH_RETURN );

}

#ifdef PROLOG_IS_CALLED_FROM_OTHER_PROGRAM
/******************************************************************************
		execute_query()
 This could be used to execute a query passed as a string.
 This means you could embed Small Prolog in another program.
 Not currently used, but why not modify main() to make use of it?
 ******************************************************************************/

execute_query(s)
char *s;
{
extern int String_input_flag;
extern char *Curr_string_input;

	ENTER("execute_query");
	String_input_flag = 1;
	Curr_string_input = s;
	reset_zones();
	Skip_above = HighDyn_ptr;

	if(!read_goals(stdin/* ignored */))
	{
		fatal(STRINGQUERY);
	}
	String_input_flag = 0;
	ini_lush();
	return(lush(1) != FINAL_LUSH_RETURN);
}
#endif


/*******************************************************************
		query_loop()
 Called by main().
 This is the interactive question-answer loop driver.
 *******************************************************************/
query_loop()
{
	int first_time = TRUE;/* have not backtracked yet */
	int stop_state;
	varindx nvar_query;
	extern varindx Nvars;

	ENTER("query_loop");
	do     {
		setjmp(Query_jenv); 
		QJusable = 1; /* indicates we can longjump to Query_jenv */
		reset_zones();
		Skip_above = HighDyn_ptr;
		prompt_user();

		if(!read_goals(stdin))continue;/* updates Goals, Nvars */
		tty_getc();/* read the carriage return */
		nvar_query = Nvars;
		ini_lush(); /* Updates Curr_clause... */

		do{
			stop_state = lush(first_time);

			switch(stop_state)
			{
			case SUCCESS_LUSH_RETURN:
				first_time = FALSE;
				pr_solution(nvar_query, BASE_SUBST);
				if(LastBack < QueryCframe)
					Deterministic_flag = 1;
				if(!Deterministic_flag && 
				    more_y_n())/*  want another solution? */
				{/* answers yes */
					break;
				}
				else
				{
					stop_state = FAIL_LUSH_RETURN;
				}
				first_time = TRUE;
				break;

			case FINAL_LUSH_RETURN:
				tty_pr_string("Bye ...\n");
				return;

			case ABNORMAL_LUSH_RETURN:
				first_time = TRUE;
				stop_state = FAIL_LUSH_RETURN;
				break;/* just a way of avoiding a goto */

			case FAIL_LUSH_RETURN:
				tty_pr_string("No\n");
				first_time = TRUE;
				break;

			default:
				INTERNAL_ERROR("lush return");
			}
		}while(stop_state != FAIL_LUSH_RETURN);
	}while(1);
}

/*******************************************************************
			ini_lush()
  Sets up the global variables, stack etc...

  Note that a pseudo-clause is created whose goals represent
  the query. Gasp, it's headless!
 *******************************************************************/
void ini_lush()
{
	clause_ptr_t get_clause();
	extern varindx Nvars;

	ENTER("ini_lush");
	Deterministic_flag = 1;
	QueryCframe = Dyn_ptr;
	Parent = QueryCframe;
	OldSubstTop =  BASE_SUBST;
	OldTrailTop =  Trail_ptr;
	LastBack = NULL;
	my_Subst_alloc((unsigned int)(Nvars * sizeof(struct subst)));
	Curr_clause = get_clause(PERMANENT); /* could be DYNAMIC !! */

	CLAUSEPTR_GOALS(Curr_clause) = Goals;
	CLAUSEPTR_HEAD(Curr_clause) = NilNodeptr; /* Who said one head
						is better than none ? */
	CLAUSEPTR_NEXT(Curr_clause) = NULL;
	/* Currclause is artificial */
	LastCframe = Dyn_ptr;
	FRAME_PARENT(LastCframe) = Parent;
	FRAME_SUBST(LastCframe) = OldSubstTop;
	FRAME_GOALS(LastCframe) = Goals;
#ifdef DFRAMES_HAVE_TYPE_FIELD 
	FRAME_TYPE(LastCframe) = D_FRAME;
#endif
	my_Dyn_alloc(SIZE_DCFRAME);
}

/*******************************************************************
			reset_zones()
 Clean things up .
 *******************************************************************/
void reset_zones()
{
	ENTER("reset_zones");
	Dyn_ptr = BASE_CSTACK;
	Subst_ptr = BASE_SUBST;
	reset_trail(BASE_TRAIL);
}

/*******************************************************************
			do_builtin()
 Call a builtin. Notice that it uses global variables to get 
 at the arguments of the builtin.
 *******************************************************************/
do_builtin(bltn)
intfun bltn; /* a function */
{
	int ret;

	ENTER("do_builtin");

	if(setjmp(Bltin_env))
 	  return(0);/* fail */

	ret = (*bltn)();
/*	BUGHUNT(ATOMPTR_NAME(Predicate));  a good place
	to do a checksum when looking for bugs; See Robert Ward's book
	"Debugging C" 
 */
	return(ret);
}


/*******************************************************************
			determine_predicate()
Returns current predicate by dereferencing goal expression.
Updates Arguments, SubstGoal and others.
 This is more complicated than might appear at first sight because
  there are several cases involving goals that are variables.
 *******************************************************************/
atom_ptr_t determine_predicate()
{
	node_ptr_t goal, headnode;

	ENTER("determine_predicate");
	goal = NODEPTR_HEAD(Goals);

	if(!dereference(goal, Subst_goals))
	{
		errmsg(NOTVARPRED);
		ErrorGlobal = ABORT;
		return(NULL);
	}

	switch(NODEPTR_TYPE(DerefNode))
	{
	case ATOM:
		Arguments =  NilNodeptr;
		SubstGoal = DerefSubst;
		return(NODEPTR_ATOM(DerefNode));
	case PAIR:
		SubstGoal = DerefSubst;
		headnode = NODEPTR_HEAD(DerefNode);
		Arguments = NODEPTR_TAIL(DerefNode);

		if(!dereference(headnode, SubstGoal))
		{
			errmsg(NOTVARPRED);
			ErrorGlobal = ABORT;
			return(NULL);
		}
		headnode = DerefNode;

		if(NODEPTR_TYPE(headnode) != ATOM)
		{
			errmsg(NOPRED);
			ErrorGlobal = ABORT;
			return(NULL);
		}
		return(NODEPTR_ATOM(headnode));
	default:
		ErrorGlobal = ABORT;
		return(NULL);
	}
}

/****************************************************************************
			do_cut()
Implements the infamous cut. This is called by Pcut in prbuiltin.c
 As an excercise try reclaiming space on the control stack.
 ***************************************************************************/
void do_cut()
{
	ENTER("do_cut");

	if (LastBack == NULL)return;
	else
		while (LastBack >= QueryCframe && LastBack >= Parent)
			LastBack = FRAME_BACKTRACK(LastBack);
}

/****************************************************************************
			dump_ancestors()
 Lets you look at the ancestors of the call when something has gone wrong.
 Should not be called dump_ancestors because it only shows the ancestors.
 ***************************************************************************/
void dump_ancestors(cframe)
dyn_ptr_t cframe;
{
	int i;
	node_ptr_t goals;

	Curr_outfile = stdout;
	tty_pr_string(STACKCONTENTS);
	tty_pr_string("\n");
	i = 1;

	if (cframe == LastCframe)
	   {
	   i++;
           out_node(Goals, Subst_goals);
	   tty_pr_string("\n");
	   }
	
	while(cframe != QueryCframe)
	{
		goals = FRAME_GOALS( cframe );
		out_node(goals, FRAME_SUBST( cframe ));
		cframe = FRAME_PARENT( cframe );
		tty_pr_string("\n");

		if(i++ == MAX_LINES)
		{
			   i = 1;
			if (!more_y_n())
				break;
		}
	}

}

#if TRACE_CAPABILITY

/************************************************************************/
/* 		Tracing routines 					*/
/************************************************************************/
/*************************************************************************
			function trace_pause()
 *************************************************************************/

int trace_pause(pwhere, skipp)
int *pwhere, skipp;
{
	char c;

	*pwhere = KEEP_GOING; /* default */

	if(Unleash_flag){
	  tty_pr_string("\n");
	  return(1);
	  }

	c = tty_getche();
	tty_pr_string("\n");
	switch(c)
	{
	case '\n':
	case '\r':
		Skip_above = HighDyn_ptr;
		break;

	case 'n':
		Trace_flag = 0; /* stop tracing, continue execution */
		Tracing_now = 0;
		break;

	case 's':
		if(skipp == CANT_SKIP)
			return 1;
		Skip_above = Parent;
		Copy_tracing_now = Tracing_now;
		Tracing_now = 0;
		break;

	case '2':
		Trace_flag = 2;
		break;

	case '1':
		Trace_flag = 1;
		break;

	case 'a':
		Trace_flag = 0;
		Tracing_now = 0;
		return 0;

	case 'P':
		BackFrame = Parent;
		if(   (BackFrame < BASE_CSTACK)
		   || (FRAME_TYPE(BackFrame) == D_FRAME)
		   || !ReverseTraceMode
		  )
		  {
	  	  tty_pr_string("Cannot reverse step just here\n");
		  tty_pr_string("Please retype your command:");
		  return trace_pause();
	  	  }
		*pwhere = G_AGA;
		break;

	case 'B':
		*pwhere = G_BTK;
                break;

	case 'U':
		Unleash_flag = 1;
		break;
	default:
		tty_pr_string("type:\n");
		tty_pr_string("Enter to see next step\n");
		tty_pr_string("2 to increase the trace details\n");
		tty_pr_string("1 to return to normal trace details\n");
		tty_pr_string("a to abort trace and execution\n");
		tty_pr_string("n to abort trace but continue execution\n");
		tty_pr_string("s to step over current goal \n");
		tty_pr_string("P to come back to parent goal\n");
		tty_pr_string("B to backtrack\n");
		tty_pr_string("U to trace without prompt\n");
		tty_pr_string("please retype your command:");
		return trace_pause();
	}
return 1;
}


static int tr_immediate_exit(pwhere)
int *pwhere;
{
int res = 1;

     if(!Tracing_now && Skip_above >= Parent)
     	Tracing_now= Copy_tracing_now;
     if(Tracing_now){
	pr_string(" Exit ");
	out_node(NODEPTR_HEAD(Goals),SubstGoal);
	res = trace_pause(pwhere,CANT_SKIP);
	}

return (res);
}

static  tr_exit(pwhere)
int *pwhere;
{

int res = 1;

     if(!Tracing_now && Skip_above >= Parent)
     	Tracing_now = Copy_tracing_now;
     if(Tracing_now){
	pr_string("...exit ");
	out_node(NODEPTR_HEAD(Goals),FRAME_SUBST(Parent));
	res = trace_pause(pwhere,CANT_SKIP);
	}
return(res);
}

static int tr_redo(pwhere)
int *pwhere;
{
 int res = 1;
 if(!Tracing_now && Skip_above > Parent)
    Tracing_now = Copy_tracing_now;
 if(Tracing_now){
   pr_string(" Redo ");
   out_node(NODEPTR_HEAD(Goals),SubstGoal);
   res = trace_pause(pwhere,CAN_SKIP);
   }
return(res);
}

static int tr_call(pwhere)
int *pwhere;
{
int res = 1;
    if(Tracing_now){
       pr_string(" Call ");
       out_node(NODEPTR_HEAD(Goals), SubstGoal);
       res = trace_pause(pwhere,CAN_SKIP);
       }
return(res);
}

static int tr_again(pwhere)
int *pwhere;
{
int res = 1;
    if(Tracing_now){
       pr_string(" Again ");
       out_node(NODEPTR_HEAD(Goals),SubstGoal);
       res = trace_pause(pwhere,CAN_SKIP);
       }
return(res);
}

static void tr_fail()
{
 if(!Tracing_now)
   {
  while( Parent > LastBack && Parent > QueryCframe)
      {
	Goals = FRAME_GOALS(Parent);
	Parent = FRAME_PARENT(Parent);
	if(Skip_above >= Parent){
	   Tracing_now = Copy_tracing_now;
	   SubstGoal = FRAME_SUBST(Parent);
           break;	
	  }
       }/* while */
     }/* if */
if(!Tracing_now && (Parent <= Skip_above))
   Tracing_now = Copy_tracing_now;
if(Tracing_now)
  {
  if( ! IS_NIL(Goals)){
     pr_string(" Fail ");
     out_node(NODEPTR_HEAD(Goals),SubstGoal);
     pr_string("\n");
     }

/* print fail of Parent goals at this point */
   while( Parent >  LastBack  && Parent > QueryCframe)
	{
	Goals = FRAME_GOALS(Parent);
	Parent = FRAME_PARENT(Parent);
	if( ! IS_NIL(Goals)){
	     pr_string(" Fail ");
	     out_node(NODEPTR_HEAD(Goals),FRAME_SUBST(Parent));
	     pr_string("\n");
	    }
	}
   }
}


/******************************************************************************
			function trace_clause()
 ******************************************************************************/
void trace_clause(clauseptr)
clause_ptr_t clauseptr;
{
pr_string("Trying rule ");
pr_clause(clauseptr);
}

#endif /* #if TRACE_CAPABILITY */

/*******************************************************************
			lush()
 Lush resolution algorithm.
 This routine tries to solve the query and sets the stacks aworking.
 Probably the most important routine.
 Warning: 
 If you add lots of features this becomes a big function,
 you may have to set a switch in your compiler to handle the size 
 of the function or it will runout of space.
 Microsoft and Zortech and Mark Williams have this.
 Turbo C seems not to need a special option .
 *******************************************************************/

int lush(first_time)
int first_time;
{
	int retval;
	int where;

	ENTER("lush");
	Unleash_flag = 0;
	if(first_time != TRUE)
		goto BACKTRACK; /* e.g. if you want to see a 2nd solution */
SELECT_GOAL:
	if(IS_FACT(Curr_clause)) /* no goals - came to leaf of proof tree */
	{
		Goals = FRAME_GOALS(LastCframe);

		TRACE(tr_immediate_exit(&where));
		Goals = NODEPTR_TAIL(Goals);
		while(Parent != QueryCframe && IS_NIL(Goals))
		{
			Goals = FRAME_GOALS(Parent);
			Parent = FRAME_PARENT(Parent);
			TRACE(tr_exit(&where));
			Goals = NODEPTR_TAIL(Goals);
		}
		Subst_goals = FRAME_SUBST(Parent);
		if(IS_NIL(Goals))
		{
			return(SUCCESS_LUSH_RETURN);
		}
	}
	else /* the clause just entered has conditions, euh, goals */
	{
		Goals = CLAUSEPTR_GOALS(Curr_clause);
		Parent = LastCframe; 
		Subst_goals = OldSubstTop;
	}
  	/* Now you've  got the goal (via Goals)
	  determine what the predicate is : */
	Predicate = determine_predicate();/* argument list updated here */

	TRACE(tr_call(&where));

        ND_builtin_next_nodeptr = NULL;

	if(Predicate == NULL)
	{
		retval = ErrorGlobal;
		goto BUILTIN_ACTION;
	}

	if(!IS_BUILTIN(Predicate))
	{
		Candidate = ATOMPTR_CLAUSE(Predicate);
		/* - this was the FIRST clause */
#ifdef HUNTBUGS
		if(Candidate!=NULL && !check_object(Candidate))
		  {
		Curr_outfile = stdout;
		pr_string(Predicate->name);
		pr_string(" is the predicate \n****************************************\n");
		fatal("error1 in code");
		  }
#endif
#if TRACE_CAPABILITY
		if(Trace_flag > 0 && Candidate == NULL)
		{
			pr_string(" Undefined ");
			pr_string(Predicate->name);
			pr_string("\n");
		}

	}
#endif
SELECT_CLAUSE: /* we can come here from BACTRACK and Candidate can be 
				  set in the backtrack section too */
	if(IS_BUILTIN(Predicate))
	{
/*		OldSubstTop = Subst_ptr;
		OldTrailTop = Trail_ptr;
		Let the builtin do this if necessary
 */
		retval = do_builtin(ATOMPTR_BUILTIN(Predicate));
BUILTIN_ACTION:
		switch( retval)
		{
		case FALSE:
			goto BACKTRACK;

		case TRUE:
			Curr_clause = Bltn_pseudo_clause;
			LastCframe = Dyn_ptr; /* top of stack */
			goto DFRAME;

		case ABORT:
			reset_zones();
			return(ABNORMAL_LUSH_RETURN);

		case CRASH:
			dump_ancestors( LastCframe );
			reset_zones();
			return(ABNORMAL_LUSH_RETURN);

		case QUIT:
			return(FINAL_LUSH_RETURN);

		case ND_SUCCESS:
			Curr_clause = Bltn_pseudo_clause;
			LastCframe = Dyn_ptr;

		my_Dyn_alloc(SIZE_NDCFRAME);

		Deterministic_flag = 0; /* maybe there is a choice point */
		FRAME_PARENT(LastCframe) = Parent;
		FRAME_SUBST(LastCframe) = OldSubstTop;
		FRAME_GOALS(LastCframe) = Goals;
		FRAME_ND_BLTIN_NEXT(LastCframe) = ND_builtin_next_nodeptr;
		/* It is the builtin's responsability to update 
		ND_builtin_next_nodeptr
		*/
		FRAME_BACKTRACK(LastCframe) = LastBack;
		FRAME_TRAIL(LastCframe) = OldTrailTop;
		FRAME_TYPE(LastCframe) = ND_BUILTIN;

		LastBack = LastCframe;
		goto SELECT_GOAL;
		}
	}
	else /* It's not builtin so look amongst candidate clauses for
		the one whose head will unify with the goal.
		*/
	{
		while (Candidate != NULL)
		{
#if TRACE_CAPABILITY
		if(Trace_flag >= 2 && Tracing_now)
		   trace_clause(Candidate);
#endif
			OldSubstTop = Subst_ptr; /* save for backtrack */
			OldTrailTop = Trail_ptr; /* ditto 		*/
			my_Subst_alloc((unsigned int)
					CLAUSEPTR_NVARS(Candidate));

			if(!unify(NODEPTR_TAIL(CLAUSEPTR_HEAD(Candidate)),
			    (subst_ptr_t)OldSubstTop,
			    Arguments, (subst_ptr_t)SubstGoal))
			{/* shallow backtrack, reset substitution and
			  try next clause
			 */
				reset_trail(OldTrailTop);
				Subst_ptr = OldSubstTop;
				Candidate = CLAUSEPTR_NEXT(Candidate);
			}
			else
			{/* successful unification */
				Nunifications++; /* statistics only */
				Curr_clause = Candidate;
				goto CFRAME_CREATION;
			}
		}/* end while */
		goto BACKTRACK; /* no candidate found */
	}/* end it's not builtin */
CFRAME_CREATION:
	LastCframe = Dyn_ptr;
	if(CLAUSEPTR_NEXT(Candidate) == NULL 
#if TRACE_CAPABILITY
	   && !ReverseTraceMode
#endif
	   )/* it's not a backtrack point.
		Mild bug: This ignores possible 
		assertz(Predicate(... calls in the clause 
		To fix this bug I guess I would need a flag.
	     */
	{/* deterministic frame */
DFRAME:
		my_Dyn_alloc(SIZE_DCFRAME);
		FRAME_PARENT(LastCframe) = Parent;
		FRAME_SUBST(LastCframe) = OldSubstTop;
		FRAME_GOALS(LastCframe) = Goals;
#ifdef DFRAMES_HAVE_TYPE_FIELD 
		FRAME_TYPE(LastCframe) = D_FRAME;
#endif

	}
	else
	{ /* non deterministic frame */
		my_Dyn_alloc(SIZE_NDCFRAME);
		Deterministic_flag = 0; /* maybe there is a choice point */
		FRAME_PARENT(LastCframe) = Parent;
		FRAME_SUBST(LastCframe) = OldSubstTop;
		FRAME_GOALS(LastCframe) = Goals;
		FRAME_CLAUSE(LastCframe) = Curr_clause;
		FRAME_BACKTRACK(LastCframe) = LastBack;
		FRAME_TRAIL(LastCframe) = OldTrailTop;
		FRAME_TYPE(LastCframe) = ND_CLAUSE;
		LastBack = LastCframe;
	}
	goto SELECT_GOAL;

BACKTRACK:

#if TRACE_CAPABILITY 
	
	if(Trace_flag > 0)
	{
	tr_fail();
	}
#endif
	if(LastBack < BASE_CSTACK)
	{
		return(FAIL_LUSH_RETURN);
	}
	Parent = FRAME_PARENT(LastBack);
	Subst_goals = FRAME_SUBST(Parent);
	Goals = FRAME_GOALS(LastBack);
	Predicate = determine_predicate();
	Dyn_ptr = LastBack;
	Subst_ptr = FRAME_SUBST(LastBack);
	OldTrailTop =  FRAME_TRAIL(LastBack);
	reset_trail(OldTrailTop);
	if(FRAME_TYPE(LastBack) == ND_CLAUSE)
	  {
 	  Candidate = FRAME_CLAUSE(LastBack);
	  Candidate = CLAUSEPTR_NEXT(Candidate);
	  }
	else
	   {
	   assert(FRAME_TYPE(LastBack) == ND_BUILTIN);
	   ND_builtin_next_nodeptr = FRAME_ND_BLTIN_NEXT(LastBack);
	   }
	LastBack =  FRAME_BACKTRACK(LastBack);
	TRACE(tr_redo(&where));
#ifdef HUNTBUGS
		if(Candidate!=NULL && !check_object(Candidate))
		  {
 		  dump_ancestors( LastCframe );
		  fatal("error2 in code");
		  }
#endif
	goto SELECT_CLAUSE;
#if TRACE_CAPABILITY
/* 
 25/12/91
 This is for debugging only: 
 When tracing you may have gone too far, so you might want
 to step back to the parent by answering 'P' during trace.
  In that case this is where execution goes to. 
  This is different from backtracking because the latter tries 
  the next clause.
 */
AGAIN:
	assert(ReverseTraceMode);
	Parent = FRAME_PARENT(BackFrame);
	Subst_goals = FRAME_SUBST(Parent);
	Goals = FRAME_GOALS(BackFrame);
	Predicate = determine_predicate();
	Dyn_ptr = BackFrame;
	Subst_ptr = FRAME_SUBST(BackFrame);
	OldTrailTop =  FRAME_TRAIL(BackFrame);
 	reset_trail(OldTrailTop); 

	if(FRAME_TYPE(BackFrame) == ND_CLAUSE)
	  {
 	  Candidate = FRAME_CLAUSE(BackFrame);
	/* Candidate = CLAUSEPTR_NEXT(Candidate); */
	  }
	else
	if(FRAME_TYPE(BackFrame) == ND_BUILTIN)
	   {
	   ND_builtin_next_nodeptr = FRAME_ND_BLTIN_NEXT(BackFrame);
	   }
        LastBack = FRAME_BACKTRACK(BackFrame);
	TRACE(tr_again(&where));
	goto SELECT_CLAUSE;
#endif
}

/* new functions 12/21/91 */


/******************************************************************************
		function abandon_query()
 noone uses this!!
 ******************************************************************************/
void abandon_query()
{
if(QJusable)longjmp(Query_jenv, 0);
else
  {
  exit_term();
  exit(1);
  }
}

/* end of file */
