BUILT_SOURCES = geolog_parser.h 
AM_YFLAGS=-d
bin_PROGRAMS = clp
clp_SOURCES =  geolog_parser.y prover.c common.h geolog.l malloc.c free_vars.c rete.c atom_and_term.c axiom.c substitution.c con_dis.c theory.c rule_queue.c rete_state.c instantiate.c fresh_constants.c rete.h malloc.h substitution.h variable.h fresh_constants.h filereader.c main.c rete_insert.c predicate.h predicate.c parser.h rule_queue.h term.h atom.h conjunction.h axiom.h theory.h fact_set.h fact_set.c proof_writer.h proof_writer.c constants.c constants.h strategy.c strategy.h disjunction.h 
	
# duma must only be linked for developer use
#prover_LDADD =  -lduma -ll -lfl

# This is the normal ldadd
clp_LDADD = -ll -lfl 

AM_LDFLAGS = -L. #${PTHREAD_LIBS}
#These flags (especially NDEBUG) are important for performance, and turned on here by default
clp_CFLAGS=-DNDEBUG #${PTHREAD_CFLAGS} 



check_PROGRAMS = clpdebug
clpdebug_SOURCES = $(clp_SOURCES)
TESTS = ./clpdebug < echo "p->q.p.q->goal."
#Remark that DNDEBUG is not in the flags of the clpdebug / check 
clpdebug_CFLAGS = -g


