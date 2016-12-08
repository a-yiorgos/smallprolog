/* prdebug.c */
/* Functions that might help you to find a bug with a source-level debugger.
 12/21/91 dump_stack renamed dump_ancestors
 */
#include <stdio.h>
#ifndef MEGAMAX
#include <assert.h>
#endif
#include "prtypes.h"
#include "prlex.h"


#ifndef NDEBUG
#define CHK(X) if(!check_object(X))INTERNAL_ERROR("wild pointer");
#endif 

/* Print the offset of a variable */

void pr_offset(nodeptr)
node_ptr_t nodeptr;
{
	if(NODEPTR_TYPE(nodeptr)!= VAR)
	{
		pr_string("non var\n");
	}
	else
		printf("%d",NODEPTR_OFFSET(nodeptr));
}

/* print all open files */
void pr_ofiles()
{
	extern struct named_ofile Open_ofiles[];
	int i;
	char *name;

	for(i = 0; i < MAXOPEN; i++)
	{
		name =    (Open_ofiles[i].o_filename);
		if(*name){
			tty_pr_string(name);
			tty_pr_string("\n");
		}
	}
}

void stack_dump()
{
extern dyn_ptr_t LastCframe; 
if (LastCframe == NULL)
   return;
dump_ancestors( LastCframe );
}

#ifdef HUNTBUGS

/* The function below was used to look for a bug 
 *   - bughunt(__LINE__,__FILE__,..) was inserted in various parts of the program 
 */

atom_ptr_t Mischievous, hash_search();
extern atom_ptr_t Predicate;
int Bug_hunt_flag = 0;  /* see do_builtin() in prlush */
static unsigned long Sum;

unsigned long sum_bytes()
{
unsigned long result;
char *p;
char *start_watch_zone, *end_watch_zone;

start_watch_zone = (char *)hash_search("eq");/* last predicate in sprolog.ini */
end_watch_zone = (char *)hash_search("log-session");/* last predicate in sprolog.ini */
assert(start_watch_zone != NULL);
assert(end_watch_zone != NULL);
assert(start_watch_zone != end_watch_zone)
for(result = 0L , p = start_watch_zone; p <= end_watch_zone;  p++)
	result += *p;
return(result);
}

bughunt(line, file, msg)
char *file,*msg;
{
static int first_time = 1;

if(Bug_hunt_flag == 0)return 1;
if(first_time){
	Sum = sum_bytes();
	first_time = 0;
	 return(1);
	}
	else
	if(Sum != sum_bytes())
		{
		printf("bughunt error %s line %d %s\n", file, line, msg);
		getchar();
		stack_dump();
		Sum = sum_bytes();

		return(0);
		}
/* printf("Sum = %lu\n", Sum); */
	return (1);
}

#endif

