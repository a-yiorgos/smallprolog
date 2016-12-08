/* prlush.h */
/* used by prlush.c */
/* 12/21/91 
  since we now have non deterministic builtins
   a Nd frame can be used for nodeterministic builtins now 
   a frame type is used to distinguish between the two
  12/31/91 
  Frame types have been added so that we can have nondeterministic
  builtins.
 */

#if TRACE_CAPABILITY
#define DFRAMES_HAVE_TYPE_FIELD /* for deterministic frames */
#endif

#define BASE_TRAIL  	Trail_mem
#define BASE_CSTACK 	Dyn_mem
#define BASE_SUBST 	Subst_mem

/* lush return values */
#define SUCCESS_LUSH_RETURN 	1
#define FAIL_LUSH_RETURN 	2
#define ABNORMAL_LUSH_RETURN 	3
#define FINAL_LUSH_RETURN 	4


typedef int frame_type_t;
/* values for frame_type_t args : */
#define ND_CLAUSE  1	 /* via clause */
#define ND_BUILTIN 2	 /* via builtin */
#define D_FRAME    0     /* deterministic */

#define STRUCT_ASSUMPTION 

/*	12/31/91 now a tested way at getting at the fields of those
	stack frames. It would be inefficient to define a stack frame as the
	union of non-deterministic and deterministic frames. 
        You might want to do more with the type field.
*/

#ifdef STRUCT_ASSUMPTION
/* you cant expect all compilers to accept these as lvalues ! 
   i.e. FRAME_PARENT(p) = Parent (for example) is not garanteed
   to pass.
   if not you will need explicit intermediate variables.
 */
#define FRAME_PARENT(C) ((dfp_t)C)->cf_parent /* points to parent frame.
					Used to continue exection
					on successful completion
					of rule.			
					 */
#define FRAME_SUBST(C) ((dfp_t)C)->cf_subst   /* represents parameters of
					the clause
					*/
#define FRAME_GOALS(C) ((dfp_t)C)->cf_goals   /* so we know which is the
					next goal to try if the
					current goal is successful
					*/
#define FRAME_CLAUSE(C) ((ndfp_t)C)->cf_next.cf_clause  /* so we know which is the next
					rule to try if the goal fails
					*/
#define FRAME_ND_BLTIN_NEXT(C) ((ndfp_t)C)->cf_next.cf_ndb_next
#define FRAME_BACKTRACK(C) ((ndfp_t)C)->cf_backtrack /* so we know which frame
					to come back to on failure 
					*/
#define FRAME_TRAIL(C) ((ndfp_t)C)->cf_trail	/* indicates part of trail
					which will be used to reset
					substitutions on bactracking
					*/
#define FRAME_TYPE(C) ((ndfp_t)C)->cf_type	/* distinguish between
					   nd frames due to builtins 
					   and those due to clauses 
					*/
typedef union 
	{struct d_cframe *df; struct nd_cframe *ndf;} cframe_ptr_t;

typedef struct d_cframe {
		dyn_ptr_t    cf_parent;
		node_ptr_t   cf_goals;
		subst_ptr_t  cf_subst;
#ifdef DFRAMES_HAVE_TYPE_FIELD
		frame_type_t cf_type; /* only used rarely */
#endif
		}*dfp_t;/* deterministic control frame */

typedef struct nd_cframe {
/* first four fields same */
		dyn_ptr_t    cf_parent;
		node_ptr_t   cf_goals;
		subst_ptr_t  cf_subst;
		frame_type_t cf_type;
		union {
		clause_ptr_t cf_clause;
		node_ptr_t   cf_ndb_next;
		}cf_next;
		dyn_ptr_t  cf_backtrack;
		node_ptr_t ** cf_trail;
		}*ndfp_t; /* non deterministic control frame */

#define SIZE_NDCFRAME sizeof(struct nd_cframe)
#define SIZE_DCFRAME  sizeof(struct d_cframe)

#else
/* not wonderful 
 * because it relies on the assumption that all pointers have 
 * the same size : 
 * This version makes the type flag take up a whole pointer
 */

#define  FRAME_PARENT(C) *((dyn_ptr_t *)C)
#define  FRAME_SUBST(C) *((subst_ptr_t *)(C + sizeof(subst_ptr_t)))
#define  FRAME_GOALS(C) *((node_ptr_t *)(C + 2*sizeof(char *)))
#define  FRAME_TYPE(C) *((frame_type_t *)(C + 3*sizeof(char *)))
#define  FRAME_CLAUSE(C) *((clause_ptr_t *)(C + 4*sizeof(char *)))
#define  FRAME_BACKTRACK(C) *((dyn_ptr_t *)(C + 5*sizeof(char *)))
#define  FRAME_TRAIL(C) *((node_ptr_t ***)(C + 6*sizeof(char *)))
#define  FRAME_ND_BLTIN_NEXT(C) *((node_ptr_t *)(C + 4*sizeof(char *)))

#define SIZE_NDCFRAME 8*sizeof(char *)
#define SIZE_DCFRAME  4*sizeof(char *)
#endif

#define IS_BUILTIN(A) (A <= LastBuiltin)

extern atom_ptr_t LastBuiltin;

