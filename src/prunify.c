/* prunify.c */
/* structure sharing unification algorithm .
 *  Occur check is a compilation option.
 * Unification is Prolog's way of passing parameters, but the comparison
 * is a little misleading.
 * When you unify a goal and a clause head you need to distiguish 
 * the variables so that even if they have the same name they are
 * not the same variable. Variables in Prolog clauses are "local"
 * after all. 
 * This is why  Prolog objects never come alone,
 * They come as pairs:
 * node and (substitution) environment . 
 * The latter says where to look up the values of the variables.
 * When a goal is unified with a head their respective environments 
 * are not the same.
 * In the structure-sharing philosophy
 * the substitutions are never really applied to modify the terms.
 * This saves time in term building.
 * Instead we use the environments to look up the values of the variables
 * of the skeletons (which are pointers to parts of the original code)
 * This adds the flesh. The disdavantage is that the algorithms may have to do
 * a lot of pointer chasing to compare two terms in their respective
 * environments. So the time we save in term building is spent in looking at 
 * terms.
 * A substitution frame consists of a sequence of substitutions.
 * The nth substitution of the frame corresponds to the nth variable
 * so the variable's offset can be used to get it directly.
 */
#include <stdio.h>
#define NDEBUG 1 /* turn off checking */
#include <assert.h>

/* #define DEBUG  */
/* #define OCCUR_CHECK  if this is defined then unification is slower but 
   checks to see that node1 is not inside node2. This occurs rarely and is
   usually checked for in prolog's.
 */

#include "prtypes.h"

extern int Trace_flag;

/* These are the globals modified by dereference() */
node_ptr_t DerefNode;
subst_ptr_t DerefSubst;

/******************************************************************************
  				unify()
 This routine tries to see if two terms (with their environments) can be
 unified, ie can a substitution be applied to make the two terms equal?
 ******************************************************************************/
/* this would be probably faster if written in a non recursive way, and with
 * in-line coding
 */
unify(node1ptr, subst1ptr, node2ptr, subst2ptr)
node_ptr_t node1ptr, node2ptr; /* skeletons */
subst_ptr_t subst1ptr, subst2ptr; /* environments */
{
	objtype_t type1, type2;

	type2 = NODEPTR_TYPE(node2ptr);
#ifdef DEBUG
	if(Trace_flag == 2){
		tty_pr_string("Enter unify with arguments\n");
		pr_node(node1ptr);
		tty_pr_string(",\n");
		pr_node(node2ptr);
		tty_pr_string(",\n");
	}
#endif

	if(type2 == VAR)
	{

		if(dereference(node2ptr, subst2ptr)) 
		/* i.e. nodeptr is a bound variable */
		{
			node2ptr = DerefNode;
			subst2ptr = DerefSubst;
			type2 = NODEPTR_TYPE(node2ptr);
			goto NODE2_NONVAR;
		}
		else /* node2ptr is free */
		node2ptr = DerefNode;
		subst2ptr = DerefSubst;



#define NODE1 DerefNode /* so as to avoid useless assignments */
#define SUBST1 DerefSubst

		if(!dereference(node1ptr, subst1ptr))/* it's free */
		{
			if (subst2ptr < SUBST1)
			{
				return(bind_var(NODE1,SUBST1, node2ptr, subst2ptr));
			}
			else	/* is it the same variable ? */
				if(SUBST1 == subst2ptr && 
				    NODEPTR_OFFSET(node2ptr) == NODEPTR_OFFSET(NODE1))
					return(TRUE);/* dont bind a var to itself */
				else
					return(bind_var(node2ptr, subst2ptr, NODE1, SUBST1));
		}
		return(bind_var(node2ptr, subst2ptr, NODE1, SUBST1));

	}
NODE2_NONVAR:
	assert(NODEPTR_TYPE(node2ptr) != VAR);
	type1 = NODEPTR_TYPE(node1ptr);

	switch(type1)
	{
	case ATOM:
		if(type1 != type2)return(FALSE);
		return(NODEPTR_ATOM(node1ptr) == NODEPTR_ATOM(node2ptr));

	case VAR:
		
		if(dereference(node1ptr, subst1ptr))
		{ /* node1 is a bound variable */
			node1ptr = DerefNode; /* what it's bound to */
			subst1ptr = DerefSubst;
			goto NODE2_NONVAR; 
		}
		else
		return(bind_var(DerefNode, DerefSubst,node2ptr,subst2ptr));

	case STRING:
		if(type1 != type2)return(FALSE);
		return(!strcmp(NODEPTR_STRING(node1ptr), NODEPTR_STRING(node2ptr)));

	case INT:
		if(type1 != type2)return(FALSE);
		return(NODEPTR_INT(node1ptr) == NODEPTR_INT(node2ptr));

	case PAIR:
		if(type1 != type2)return(FALSE);
		/* to oversimplify: 
		 unify each of the corresponding elements of the lists
			 and fail if one of them does not unify.
		*/
		while(NODEPTR_TYPE(node1ptr) == PAIR && NODEPTR_TYPE(node2ptr)== PAIR)
		{
			if(!unify(NODEPTR_HEAD(node1ptr), subst1ptr, 
			    NODEPTR_HEAD(node2ptr), subst2ptr))return(FALSE);

			dereference(NODEPTR_TAIL(node1ptr), subst1ptr);
			node1ptr = DerefNode;
			subst1ptr = DerefSubst;

			dereference(NODEPTR_TAIL(node2ptr), subst2ptr);
			node2ptr = DerefNode;
			subst2ptr = DerefSubst;
		}

		return(unify(node1ptr, subst1ptr, node2ptr, subst2ptr));

	case CLAUSE:
		if(type1 != type2)return(FALSE);
		else/* compare pointers only ! */
		return(NODEPTR_CLAUSE(node2ptr) == NODEPTR_CLAUSE(node1ptr));
#ifdef REAL
	case REAL:
		if(type1 != type2)return(FALSE);
		return(NODEPTR_REAL(node1ptr) == NODEPTR_REAL(node2ptr));
#endif	

#ifdef CHARACTER	
	case CHARACTER:
		if(type1 != type2)return(FALSE);
		else
			return(NODEPTR_CHARACTER(node2ptr) == NODEPTR_CHARACTER(node1ptr));
#endif
	default:
		INTERNAL_ERROR("unification type");
		return(FALSE);
	}

}

/******************************************************************************
			bind_var()
 Set the "value" of node1ptr, subst1ptr to node2ptr, subst2ptr.
 node1ptr must be an unbound var in its environement subst1ptr.
 ******************************************************************************/
bind_var(node1ptr, subst1ptr, node2ptr, subst2ptr)
node_ptr_t node1ptr, node2ptr;
subst_ptr_t subst1ptr, subst2ptr;
{
	char *molec; /* yes, a char * (for efficiency) */
	node_ptr_t **my_Trail_alloc(), **trailptr;

#ifndef NDEBUG
	if(NODEPTR_TYPE(node1ptr) != VAR)INTERNAL_ERROR("non var bind");
#endif 
#ifdef OCCUR_CHECK
	if(occur_check(node1ptr, subst1ptr, node2ptr, subst2ptr))
	{
		errmsg("occur check returns true!");
		return 0;
	}
#endif
	molec = (char *)subst1ptr + NODEPTR_OFFSET(node1ptr);
	((subst_ptr_t)molec)->frame = subst2ptr;
#ifndef NDEBUG
	if(((subst_ptr_t)molec)->skel)INTERNAL_ERROR("noise in molecule");
#endif 
	((subst_ptr_t)molec)->skel = node2ptr;

	/* record the substitution on the trail so that it can be 
	   undone later
	  (this might not always be necessary) 
	*/
	trailptr = my_Trail_alloc();
	*trailptr = &(((subst_ptr_t)molec)->skel);
	return 1;
}

/******************************************************************************
			reset_trail()
Use the trail to reset the substitution stack.
 ******************************************************************************/
reset_trail(from)
node_ptr_t **from;
{
	register node_ptr_t **tp;
	extern node_ptr_t **Trail_ptr;

	for(tp = from; tp < Trail_ptr; tp++)
	{
		**tp = NULL;
	}
	Trail_ptr = from;
}

/*****************************************************************************
			dereference()
Lookup what a variable points to indirectly.
Dereferencing is weaker than instantiating, because the variables in
the dereferenced term are not replaced by their values, if you want
to know their values you have to derefence them and so on. See how
the display builtin works to give you the impression that it is
printing the instantiated term.
Returns 0 if nodeptr dereferences to (in fact instantiates to) VAR 
and 1 otherwise, ie returns 0 if (nodepr,substptr) is free
 *****************************************************************************/
/* updates DerefNode, DerefSubst */
dereference(nodeptr, substptr)
node_ptr_t nodeptr;
subst_ptr_t substptr;
{
	char *molec;/* a bit of finesse is needed here to gain speed */
	node_ptr_t skelptr;
	DerefNode = nodeptr;
	DerefSubst = substptr;

	while(NODEPTR_TYPE(DerefNode) == VAR)
	{
		molec = (char *)DerefSubst + NODEPTR_OFFSET(DerefNode);
		/* we converted to char * to avoid behind-the-scenes offset multiplication */
		skelptr = ((subst_ptr_t)molec)->skel;
		if(!skelptr)
			return(FALSE);
		else
			DerefNode = skelptr;
		DerefSubst = ((subst_ptr_t)molec)->frame;
	}
	return(TRUE);
}

/******************************************************************************
			occur_check()
 ******************************************************************************/
#ifdef OCCUR_CHECK
occur_check(node1ptr, subst1ptr, node2ptr, subst2ptr)
node_ptr_t node1ptr, node2ptr;
subst_ptr_t subst1ptr, subst2ptr;
{

	if(NODEPTR_TYPE(node2ptr) == VAR)
	{
		if(	subst1ptr == subst2ptr &&
		    (NODEPTR_OFFSET(node2ptr) == NODEPTR_OFFSET(node1ptr))
		    )return 1;
		else
			return 0;
	}
	else
		if(NODEPTR_TYPE(node2ptr) == PAIR)
		{
			dereference(NODEPTR_HEAD(node2ptr), subst2ptr);
			if(occur_check(node1ptr, subst1ptr, DerefNode, DerefSubst))
				return 1;
			else
			{
				dereference(NODEPTR_TAIL(node2ptr), subst2ptr);
				return(occur_check(node1ptr, subst1ptr, DerefNode, DerefSubst));
			}
		}
	return(0);
}
#endif

/* end of file */
