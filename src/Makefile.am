#BUILT_SOURCES = geolog_parser.h clpl_parser.h
#AM_YFLAGS=-d
bin_PROGRAMS = clp
clp_SOURCES =  geolog_parser.y clpl_parser.y prover.c common.h clpl.l geolog.l malloc.c free_vars.c rete.c atom_and_term.c axiom.c substitution.c con_dis.c theory.c rule_queue.c rete_state.c instantiate.c fresh_constants.c rete.h malloc.h substitution.h variable.h fresh_constants.h filereader.c main.c rete_insert.c predicate.h predicate.c parser.h rule_queue.h term.h atom.h conjunction.h axiom.h theory.h fact_set.h fact_set.c proof_writer.h proof_writer.c constants.c constants.h strategy.c strategy.h disjunction.h sub_alpha_queue.h sub_alpha_queue.c rete_node.h rete_net.h rete_net_state.h rule_instance_stack.h rule_instance_stack.c rule_instance_state_stack.h rule_instance_state_stack.c substitution_memory.h substitution_memory.c substitution_state_store.c substitution_state_store.h rete_state.h substitution_struct.h substitution_size_info.c substitution_size_info.h rete_state_single.h

clp.$(OBJECT): geolog_parser.h clpl_parser.h geolog_parser.c clpl_parser.c clpl.c geolog.c

geolog.c: geolog_parser.h geolog.l
	flex --fast --outfile=geolog.c $(srcdir)/geolog.l
clpl.c: clpl_parser.h clpl.l
	flex --fast --outfile=clpl.c $(srcdir)/clpl.l	

geolog_parser.h: geolog_parser.y
	bison --defines=geolog_parser.h --output=geolog_parser.c $(srcdir)/geolog_parser.y
geolog_parser.c: geolog_parser.h

clpl_parser.h: clpl_parser.y
	bison --defines=clpl_parser.h --output=clpl_parser.c $(srcdir)/clpl_parser.y
clpl_parser.c: clpl_parser.h

clp_LDADD = -ll -lfl -lm -lrt 
AM_CFLAGS=-Wall 
AM_LDFLAGS = -L. ${PTHREAD_LIBS}
#These flags (especially NDEBUG) are important for performance, and turned on here by default
clp_CFLAGS=-DNDEBUG ${PTHREAD_CFLAGS} 


# Below is setup for testing (the target "check"). 
# Testing is done  by enabling assertions, linking with DUMA,
# (Detect Unintented Memory Access) and running
# the prover on the theory anl (Newman's Lemma)

check_PROGRAMS = clpdebug clpmemdebug
clpdebug_SOURCES = $(clp_SOURCES) 
clpmemdebug_SOURCES = $(clp_SOURCES) 
check_SCRIPTS = check_clp.sh
check_DATA = $(srcdir)/anc.in  $(srcdir)/anl.in 
TESTS = $(check_SCRIPTS)
check_clp.sh:
	echo "./clpmemdebug -q $(srcdir)/anl.in" > check_clp.sh
	echo "coqc anl.v" >> check_clp.sh
	echo "rm anl.v anl.vo" >> check_clp.sh
	chmod +x check_clp.sh
CLEANFILES = $(check_SCRIPTS) geolog_parser.h clpl_parser.h geolog_parser.c clpl_parser.c geolog_parser.tab.h clpl_parser.tab.h geolog_parser.tab.c clpl_parser.tab.c y.tab.c y.tab.h clpl.c geolog.c

#Remark that DNDEBUG is not in the flags of the clpdebug / check 
clpmemdebug_CFLAGS = -lm -g -lduma -lrt ${PTHREAD_CFLAGS}
clpdebug_CFLAGS = -lm -g -lrt ${PTHREAD_CFLAGS}


