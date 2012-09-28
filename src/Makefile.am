bin_PROGRAMS = clp 
#BUILT_SOURCES = geolog.c clpl.c tptp.c geolog_parser.h clpl_parser.h tptp_parser.h geolog_parser.c clpl_parser.c tptp_parser.c 
#AM_YFLAGS=-d

clp_SOURCES =  geolog_parser.y clpl_parser.y tptp_parser.y common.h clpl.l geolog.l tptp.l malloc.c free_vars.c rete.c atom_and_term.c axiom.c substitution.c con_dis.c theory.c instantiate.c fresh_constants.c rete.h malloc.h substitution.h variable.h fresh_constants.h filereader.c main.c predicate.h predicate.c parser.h rule_queue.h term.h atom.h conjunction.h axiom.h theory.h fact_set.h fact_set.c proof_writer.h proof_writer.c constants.c constants.h strategy.c strategy.h disjunction.h  rete_node.h rete_net.h rete_net_state.h rule_instance_stack.h rule_instance_stack.c logger.h logger.c rule_instance_state_stack.h rule_instance_state_stack.c substitution_store_mt.h substitution_store_mt.c substitution_store.c substitution_store.h rete_state.h substitution_struct.h substitution_size_info.c substitution_size_info.h rete_state_single.h prover_single.c rule_instance.h rule_queue_single.h rule_queue_single.c rete_state_struct.h rule_queue_state.h rete_state_single_struct.h rete_state_single.c rete_insert_single.h rete_insert_single.c rule_instance.c fact_store.h fact_store.c error_handling.c error_handling.h rete_worker_queue.c rete_worker_queue.h substitution_store_array.c substitution_store_array.h rete_worker.c rete_worker.h proof_branch.h proof_branch.c timestamp.h timestamps.h timestamp.c timestamps.c timestamp_store.c ParseTPTP.c ParseTPTP.h Parsing.c Parsing.h Utilities.c Utilities.h FileUtilities.c Tokenizer.c Examine.c List.c List.h Signature.c Signature.h PrintTSTP.c PrintTSTP.h ParseTSTP.h ParseTSTP.c Compare.c Compare.h Modify.h Modify.c PrintDFG.h PrintDFG.c PrintOtter.h PrintOtter.c PrintSUMO.h PrintSUMO.c PrintXML.h PrintXML.c PrintKIF.c PrintKIF.h 

clp.$(OBJECT): geolog_parser.h clpl_parser.h tptp_parser.h geolog_parser.c clpl_parser.c tptp_parser.c clpl.c geolog.c tptp.c

geolog.c: geolog_parser.h geolog.l
	flex --fast --outfile=geolog.c $(srcdir)/geolog.l
clpl.c: clpl_parser.h clpl.l
	flex --fast --outfile=clpl.c $(srcdir)/clpl.l	
tptp.c: tptp_parser.h tptp.l 
	flex --fast --outfile=tptp.c $(srcdir)/tptp.l	

geolog_parser.h: geolog_parser.y
	bison --defines=geolog_parser.h --output=geolog_parser.c $(srcdir)/geolog_parser.y
geolog_parser.c: geolog_parser.h

clpl_parser.h: clpl_parser.y
	bison --defines=clpl_parser.h --output=clpl_parser.c $(srcdir)/clpl_parser.y
clpl_parser.c: clpl_parser.h

tptp_parser.h: tptp_parser.y
	bison --defines=tptp_parser.h --output=tptp_parser.c $(srcdir)/tptp_parser.y
tptp_parser.c: tptp_parser.h

#nodist_clp_SOURCES = geolog.c clpl.c tptp.c geolog_parser.h clplp_parser.h tptp_parser.h geolog_parser.c clplp_parser.c tptp_parser.c


clp_LDADD = -ll -lfl -lm -lrt 
AM_CFLAGS=-Wall -O0
AM_LDFLAGS = -L. ${PTHREAD_LIBS}
#These flags (especially NDEBUG) are important for performance, and turned on here by default
clp_CFLAGS=-DNDEBUG ${PTHREAD_CFLAGS} 
#clp_CFLAGS= -g ${PTHREAD_CFLAGS} 


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
	echo "./clpdebug -q $(srcdir)/nl.in" > $(check_SCRIPTS)
	echo "coqc nl.v" >> $(check_SCRIPTS)
	echo "rm nl.v nl.vo" >> $(check_SCRIPTS)
	echo "./clpmemdebug -q $(srcdir)/anc.in" >> $(check_SCRIPTS)
	echo "coqc anc.v" >> $(check_SCRIPTS)	
	echo "rm anc.v anc.vo" >> $(check_SCRIPTS)
	echo "./clpdebug $(srcdir)/nl.in" >> $(check_SCRIPTS)
	chmod +x $(check_SCRIPTS)
CLEANFILES = $(check_SCRIPTS) geolog_parser.h clpl_parser.h geolog_parser.c clpl_parser.c geolog_parser.tab.h clpl_parser.tab.h geolog_parser.tab.c clpl_parser.tab.c y.tab.c y.tab.h clpl.c geolog.c

#Remark that DNDEBUG is not in the flags of the clpdebug / check 
clpdebug_LDADD = ${clp_LDADD}
clpmemdebug_LDADD = ${clpdebug_LDADD} -lduma
clpmemdebug_CFLAGS = -g  ${PTHREAD_CFLAGS}
clpdebug_CFLAGS =  -g  ${PTHREAD_CFLAGS}


