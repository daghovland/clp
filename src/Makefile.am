BUILT_SOURCES = geolog_parser.h 
AM_YFLAGS=-d
bin_PROGRAMS = prover
dist_prover_SOURCES =  geolog_parser.y prover.c geolog.l malloc.c free_vars.c rete.c atom_and_term.c axiom.c substitution.c con_dis.c theory.c rule_queue.c rete_state.c instantiate.c fresh_constants.c rete.h malloc.h substitution.h variable.h fresh_constants.h filereader.c main.c rete_insert.c predicate.h predicate.c parser.h rule_queue.h term.h atom.h conjunction.h axiom.h theory.h fact_set.h fact_set.c proof_writer.h proof_writer.c constants.c constants.h strategy.c strategy.h
# duma must only be linked for developer use
#prover_LDADD =  -lduma -ll -lfl
# This is the normal ldadd
prover_LDADD = -ll -lfl 
AM_LDFLAGS = -L. #${PTHREAD_LIBS}
#Uncomment if you want threads
#AM_CFLAGS=${PTHREAD_CFLAGS} 



