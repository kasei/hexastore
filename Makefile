### This is the generic Makefile for the hexastore code. It should work on most
### unix-like systems (development and testing happens on both linux and OS X).
### `make all` is what you want to run the first time through (which importantly
### drags in the `make sparql` target for running flex and bison). On subsequent
### makes, just `make` should do the trick without needing to re-generate the
### flex/bison stuff.

CDEFINES	= -DTIMING_CPU_FREQUENCY=2400000000.0 # -DDEBUG
INCPATH		= -I. # -I/usr/local/include -I/ext/local/include
LIBPATH		= -L. # -L/usr/local/lib -L/ext/local/lib
CFLAGS		= -std=gnu99 -pedantic $(INCPATH) $(LIBPATH) -ggdb $(CDEFINES)
OBJFLAGS	= -fPIC -shared
CC			= gcc $(CFLAGS)

LIBS		=	-ltokyocabinet -lpthread -lraptor

HEXASTORE_OBJECTS	= mentok/store/hexastore/hexastore.o mentok/store/hexastore/index.o mentok/store/hexastore/terminal.o mentok/store/hexastore/vector.o mentok/store/hexastore/head.o mentok/store/hexastore/btree.o
TCSTORE_OBJECTS		= mentok/store/tokyocabinet/tokyocabinet.o mentok/store/tokyocabinet/tcindex.o
STORE_OBJECTS		= mentok/store/store.o $(HEXASTORE_OBJECTS) $(TCSTORE_OBJECTS)
MISC_OBJECTS		= mentok/misc/avl.o mentok/misc/nodemap.o mentok/misc/util.o mentok/misc/idmap.o
RDF_OBJECTS			= mentok/rdf/node.o mentok/rdf/triple.o
ENGINE_OBJECTS		= mentok/engine/expr.o mentok/engine/variablebindings_iter.o mentok/engine/variablebindings_iter_sorting.o mentok/engine/nestedloopjoin.o mentok/engine/mergejoin.o mentok/engine/materialize.o mentok/engine/filter.o mentok/engine/project.o mentok/engine/hashjoin.o mentok/engine/bgp.o mentok/engine/graphpattern.o
ALGEBRA_OBJECTS		= mentok/algebra/variablebindings.o mentok/algebra/bgp.o mentok/algebra/expr.o mentok/algebra/graphpattern.o
PARSER_OBJECTS		= mentok/parser/SPARQLParser.o mentok/parser/SPARQLScanner.o mentok/parser/parser.o
OPT_OBJECTS			= mentok/optimizer/optimizer.o mentok/optimizer/plan.o
OBJECTS				= mentok/mentok.o $(STORE_OBJECTS) $(MISC_OBJECTS) $(RDF_OBJECTS) $(ENGINE_OBJECTS) $(ALGEBRA_OBJECTS) $(PARSER_OBJECTS) $(OPT_OBJECTS)
LINKOBJS			= libmentok.dylib
LINKOBJSFLAGS		= -lmentok
INSTDIR				= /usr/local

################################################################################

default: parse print optimize tests examples parse_query

all: sparql parse print optimize tests examples parse_query dumpmap assign_ids

server: cli/server.c $(LINKOBJS)
	$(CC) $(LINKOBJSFLAGS) -ldrizzle -o server cli/server.c

assign_ids: cli/assign_ids.c $(LINKOBJS)
	$(CC) $(LINKOBJSFLAGS) -ltokyocabinet -o assign_ids cli/assign_ids.c

parse: cli/parse.c $(LINKOBJS)
	$(CC) $(LINKOBJSFLAGS) -o parse cli/parse.c

optimize: cli/optimize.c $(LINKOBJS)
	$(CC) $(LINKOBJSFLAGS) -o optimize cli/optimize.c

print: cli/print.c $(LINKOBJS)
	$(CC) $(LINKOBJSFLAGS) -o print cli/print.c

parse_query: cli/parse_query.c $(LINKOBJS)
	$(CC) $(LINKOBJSFLAGS) -o parse_query cli/parse_query.c

dumpmap: cli/dumpmap.c $(LINKOBJS)
	$(CC) $(LINKOBJSFLAGS) -o dumpmap cli/dumpmap.c

###

mentok/mentok.o: mentok/mentok.c mentok/mentok.h mentok/store/hexastore/index.h mentok/store/hexastore/head.h mentok/store/hexastore/vector.h mentok/store/hexastore/terminal.h mentok/mentok_types.h mentok/algebra/variablebindings.h mentok/misc/nodemap.h
	$(CC) $(OBJFLAGS) -c -o mentok/mentok.o mentok/mentok.c

mentok/store/hexastore/index.o: mentok/store/hexastore/index.c mentok/store/hexastore/index.h mentok/store/hexastore/terminal.h mentok/store/hexastore/vector.h mentok/store/hexastore/head.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/store/hexastore/index.o mentok/store/hexastore/index.c

mentok/store/store.o: mentok/store/store.c mentok/store/store.h
	$(CC) $(OBJFLAGS) -c -o mentok/store/store.o mentok/store/store.c

mentok/store/tokyocabinet/tokyocabinet.o: mentok/store/tokyocabinet/tokyocabinet.c mentok/store/tokyocabinet/tokyocabinet.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/store/tokyocabinet/tokyocabinet.o mentok/store/tokyocabinet/tokyocabinet.c

mentok/store/tokyocabinet/tcindex.o: mentok/store/tokyocabinet/tcindex.c mentok/store/tokyocabinet/tcindex.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/store/tokyocabinet/tcindex.o mentok/store/tokyocabinet/tcindex.c

mentok/store/hexastore/hexastore.o: mentok/store/hexastore/hexastore.c mentok/store/hexastore/hexastore.h mentok/store/hexastore/head.h mentok/store/hexastore/vector.h mentok/store/hexastore/terminal.h mentok/store/hexastore/btree.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/store/hexastore/hexastore.o mentok/store/hexastore/hexastore.c

mentok/store/hexastore/terminal.o: mentok/store/hexastore/terminal.c mentok/store/hexastore/terminal.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/store/hexastore/terminal.o mentok/store/hexastore/terminal.c

mentok/store/hexastore/vector.o: mentok/store/hexastore/vector.c mentok/store/hexastore/vector.h mentok/store/hexastore/terminal.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/store/hexastore/vector.o mentok/store/hexastore/vector.c

mentok/store/hexastore/head.o: mentok/store/hexastore/head.c mentok/store/hexastore/head.h mentok/store/hexastore/vector.h mentok/store/hexastore/terminal.h mentok/store/hexastore/btree.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/store/hexastore/head.o mentok/store/hexastore/head.c

mentok/rdf/node.o: mentok/rdf/node.c mentok/rdf/node.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/rdf/node.o mentok/rdf/node.c
	
mentok/misc/nodemap.o: mentok/misc/nodemap.c mentok/misc/nodemap.h mentok/misc/avl.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/misc/nodemap.o mentok/misc/nodemap.c

mentok/misc/idmap.o: mentok/misc/idmap.c mentok/misc/idmap.h mentok/misc/avl.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/misc/idmap.o mentok/misc/idmap.c

mentok/engine/bgp.o: mentok/engine/bgp.c mentok/engine/bgp.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/engine/bgp.o mentok/engine/bgp.c

mentok/engine/expr.o: mentok/engine/expr.c mentok/engine/expr.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/engine/expr.o mentok/engine/expr.c

mentok/engine/graphpattern.o: mentok/engine/graphpattern.c mentok/engine/graphpattern.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/engine/graphpattern.o mentok/engine/graphpattern.c

mentok/engine/mergejoin.o: mentok/engine/mergejoin.c mentok/engine/mergejoin.h mentok/mentok_types.h mentok/algebra/variablebindings.h
	$(CC) $(OBJFLAGS) -c -o mentok/engine/mergejoin.o mentok/engine/mergejoin.c

mentok/engine/nestedloopjoin.o: mentok/engine/nestedloopjoin.c mentok/engine/nestedloopjoin.h mentok/mentok_types.h mentok/algebra/variablebindings.h
	$(CC) $(OBJFLAGS) -c -o mentok/engine/nestedloopjoin.o mentok/engine/nestedloopjoin.c

mentok/engine/hashjoin.o: mentok/engine/hashjoin.c mentok/engine/hashjoin.h mentok/mentok_types.h mentok/algebra/variablebindings.h
	$(CC) $(OBJFLAGS) -c -o mentok/engine/hashjoin.o mentok/engine/hashjoin.c

mentok/algebra/variablebindings.o: mentok/algebra/variablebindings.c mentok/algebra/variablebindings.h mentok/mentok_types.h mentok/rdf/node.h mentok/misc/nodemap.h
	$(CC) $(OBJFLAGS) -c -o mentok/algebra/variablebindings.o mentok/algebra/variablebindings.c

mentok/engine/variablebindings_iter.o: mentok/engine/variablebindings_iter.c mentok/engine/variablebindings_iter.h mentok/mentok_types.h mentok/rdf/node.h mentok/misc/nodemap.h
	$(CC) $(OBJFLAGS) -c -o mentok/engine/variablebindings_iter.o mentok/engine/variablebindings_iter.c

mentok/engine/variablebindings_iter_sorting.o: mentok/engine/variablebindings_iter_sorting.c mentok/engine/variablebindings_iter_sorting.h mentok/mentok_types.h mentok/rdf/node.h mentok/misc/nodemap.h
	$(CC) $(OBJFLAGS) -c -o mentok/engine/variablebindings_iter_sorting.o mentok/engine/variablebindings_iter_sorting.c

mentok/engine/materialize.o: mentok/engine/materialize.c mentok/engine/materialize.h mentok/mentok_types.h mentok/rdf/node.h mentok/misc/nodemap.h
	$(CC) $(OBJFLAGS) -c -o mentok/engine/materialize.o mentok/engine/materialize.c

mentok/engine/filter.o: mentok/engine/filter.c mentok/engine/filter.h mentok/mentok_types.h mentok/rdf/node.h mentok/misc/nodemap.h
	$(CC) $(OBJFLAGS) -c -o mentok/engine/filter.o mentok/engine/filter.c

mentok/rdf/triple.o: mentok/rdf/triple.c mentok/rdf/triple.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/rdf/triple.o mentok/rdf/triple.c

mentok/store/hexastore/btree.o: mentok/store/hexastore/btree.c mentok/store/hexastore/btree.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/store/hexastore/btree.o mentok/store/hexastore/btree.c

mentok/parser/parser.o: mentok/parser/parser.c mentok/parser/parser.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/parser/parser.o mentok/parser/parser.c

mentok/algebra/bgp.o: mentok/algebra/bgp.c mentok/algebra/bgp.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/algebra/bgp.o mentok/algebra/bgp.c

mentok/algebra/expr.o: mentok/algebra/expr.c mentok/algebra/expr.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/algebra/expr.o mentok/algebra/expr.c

mentok/algebra/graphpattern.o: mentok/algebra/graphpattern.c mentok/algebra/graphpattern.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/algebra/graphpattern.o mentok/algebra/graphpattern.c

mentok/engine/project.o: mentok/engine/project.c mentok/engine/project.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/engine/project.o mentok/engine/project.c

mentok/misc/util.o: mentok/misc/util.c mentok/misc/util.h
	$(CC) $(OBJFLAGS) -c -o mentok/misc/util.o mentok/misc/util.c

mentok/misc/avl.o: mentok/misc/avl.c mentok/misc/avl.h mentok/mentok_types.h
	$(CC) $(OBJFLAGS) -c -o mentok/misc/avl.o mentok/misc/avl.c

mentok/optimizer/optimizer.o: mentok/optimizer/optimizer.c mentok/optimizer/optimizer.h
	$(CC) $(OBJFLAGS) -c -o mentok/optimizer/optimizer.o mentok/optimizer/optimizer.c

mentok/optimizer/plan.o: mentok/optimizer/plan.c mentok/optimizer/plan.h
	$(CC) $(OBJFLAGS) -c -o mentok/optimizer/plan.o mentok/optimizer/plan.c

########

# SPARQLParser.c:
# SPARQLParser.h: SPARQLParser.yy
sparql: mentok/parser/SPARQLParser.yy mentok/parser/SPARQLScanner.ll
	bison -o mentok/parser/SPARQLParser.c mentok/parser/SPARQLParser.yy
	flex -o mentok/parser/SPARQLScanner.c mentok/parser/SPARQLScanner.ll

mentok/parser/SPARQLScanner.h: mentok/parser/SPARQLScanner.c mentok/parser/SPARQLScanner.ll

mentok/parser/SPARQLScanner.c: mentok/parser/SPARQLScanner.ll

mentok/parser/SPARQLParser.o: mentok/parser/SPARQLParser.yy mentok/parser/SPARQLScanner.ll mentok/parser/SPARQLParser.h mentok/parser/SPARQLScanner.h
	$(CC) -DYYTEXT_POINTER=1 -W -Wall -Wextra -ansi -g -c  -o mentok/parser/SPARQLParser.o mentok/parser/SPARQLParser.c

mentok/parser/SPARQLScanner.o: mentok/parser/SPARQLScanner.c mentok/parser/SPARQLParser.h mentok/parser/SPARQLScanner.h
	$(CC) -DYYTEXT_POINTER=1 -Wextra -ansi -g -c  -o mentok/parser/SPARQLScanner.o mentok/parser/SPARQLScanner.c

########

tests: t/nodemap.t t/node.t t/expr.t t/index.t t/terminal.t t/vector.t t/head.t t/btree.t t/join.t t/iter.t t/bgp.t t/materialize.t t/selectivity.t t/filter.t t/graphpattern.t t/parser.t t/variablebindings.t t/project.t t/triple.t t/hash.t t/store-hexastore.t t/store-tokyocabinet.t t/tokyocabinet.t t/optimizer.t

examples: examples/lubm_q4 examples/lubm_q8 examples/lubm_q9 examples/bench examples/knows

########
t/node.t: test/tap.o t/node.c mentok/rdf/node.h $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/node.t t/node.c test/tap.o

t/expr.t: test/tap.o t/expr.c mentok/algebra/expr.h $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/expr.t t/expr.c test/tap.o

t/nodemap.t: test/tap.o t/nodemap.c mentok/misc/nodemap.h $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/nodemap.t t/nodemap.c test/tap.o

t/index.t: test/tap.o t/index.c mentok/store/hexastore/index.h $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/index.t t/index.c test/tap.o

t/terminal.t: test/tap.o t/terminal.c mentok/store/hexastore/terminal.h $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/terminal.t t/terminal.c test/tap.o

t/vector.t: test/tap.o t/vector.c mentok/store/hexastore/vector.h $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/vector.t t/vector.c test/tap.o

t/head.t: test/tap.o t/head.c mentok/store/hexastore/head.h $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/head.t t/head.c test/tap.o

t/btree.t: test/tap.o t/btree.c mentok/store/hexastore/btree.h $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/btree.t t/btree.c test/tap.o

t/join.t: test/tap.o t/join.c $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/join.t t/join.c test/tap.o

t/iter.t: test/tap.o t/iter.c $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/iter.t t/iter.c test/tap.o

t/bgp.t: test/tap.o t/bgp.c $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/bgp.t t/bgp.c test/tap.o

t/filter.t: test/tap.o t/filter.c $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/filter.t t/filter.c test/tap.o

t/materialize.t: test/tap.o t/materialize.c $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/materialize.t t/materialize.c test/tap.o

t/selectivity.t: test/tap.o t/selectivity.c $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/selectivity.t t/selectivity.c test/tap.o

t/graphpattern.t: test/tap.o t/graphpattern.c $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/graphpattern.t t/graphpattern.c test/tap.o

t/parser.t: test/tap.o t/parser.c $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/parser.t t/parser.c test/tap.o

t/variablebindings.t: test/tap.o t/variablebindings.c $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/variablebindings.t t/variablebindings.c test/tap.o

t/project.t: test/tap.o t/project.c $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/project.t t/project.c test/tap.o

t/triple.t: test/tap.o t/triple.c $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/triple.t t/triple.c test/tap.o

t/hash.t: test/tap.o t/hash.c $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/hash.t t/hash.c test/tap.o

t/optimizer.t: test/tap.o t/optimizer.c $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/optimizer.t t/optimizer.c test/tap.o

t/store-hexastore.t: test/tap.o t/store-hexastore.c mentok/store/hexastore/hexastore.h $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/store-hexastore.t t/store-hexastore.c test/tap.o

t/store-tokyocabinet.t: test/tap.o t/store-tokyocabinet.c mentok/store/tokyocabinet/tokyocabinet.h $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/store-tokyocabinet.t t/store-tokyocabinet.c test/tap.o

t/tokyocabinet.t: test/tap.o t/tokyocabinet.c mentok/store/hexastore/hexastore.h $(LINKOBJS) test/tap.o
	$(CC) $(LINKOBJSFLAGS) -o t/tokyocabinet.t t/tokyocabinet.c test/tap.o

########

examples/lubm_q4: examples/lubm_q4.c $(LINKOBJS)
	$(CC) $(LINKOBJSFLAGS) -o examples/lubm_q4 examples/lubm_q4.c

examples/lubm_q8: examples/lubm_q8.c $(LINKOBJS)
	$(CC) $(LINKOBJSFLAGS) -o examples/lubm_q8 examples/lubm_q8.c

examples/lubm_q9: examples/lubm_q9.c $(LINKOBJS)
	$(CC) $(LINKOBJSFLAGS) -o examples/lubm_q9 examples/lubm_q9.c

examples/bench: examples/bench.c $(LINKOBJS)
	$(CC) $(LINKOBJSFLAGS) -o examples/bench examples/bench.c

examples/knows: examples/knows.c $(LINKOBJS)
	$(CC) $(LINKOBJSFLAGS) -o examples/knows examples/knows.c

########

test/tap.o: test/tap.c test/tap.h
	$(CC) $(OBJFLAGS) -c -o test/tap.o test/tap.c

########

distclean:
	rm -f SPARQL mentok/parser/SPARQLParser.o mentok/parser/SPARQLScanner.o mentok/parser/SPARQLParser.c mentok/parser/SPARQLScanner.c mentok/parser/SPARQLParser.h
	rm -f examples/lubm_q[489] examples/bench examples/knows examples/mpi
	rm -rf examples/lubm_q[489].dSYM examples/bench.dSYM examples/knows.dSYM examples/mpi.dSYM
	rm -f parse print optimize a.out server parse_query dumpmap assign_ids
	rm -f mentok/*.o mentok/*/*.o mentok/*/*/*.o
	rm -f libmentok.dylib
	rm -rf *.dSYM t/*.dSYM
	rm -f t/*.t
	rm -f stack.h position.h location.h
	
clean:
	rm -f examples/lubm_q[489] examples/bench examples/knows examples/mpi
	rm -rf examples/lubm_q[489].dSYM examples/bench.dSYM examples/knows.dSYM examples/mpi.dSYM
	rm -f parse print optimize a.out server parse_query dumpmap assign_ids
	rm -f mentok/*.o mentok/*/*.o mentok/*/*/*.o
	rm -f libmentok.dylib
	rm -rf *.dSYM t/*.dSYM
	rm -f t/*.t
	rm -f stack.h position.h location.h

########

install: libmentok.dylib $(PUBLIC_HEADERS)
	mkdir -p $(INSTDIR)/include/mentok
	mkdir -p $(INSTDIR)/include/mentok/algebra
	mkdir -p $(INSTDIR)/include/mentok/engine
	mkdir -p $(INSTDIR)/include/mentok/rdf
	mkdir -p $(INSTDIR)/include/mentok/misc
	mkdir -p $(INSTDIR)/include/mentok/parser
	mkdir -p $(INSTDIR)/include/mentok/store
	mkdir -p $(INSTDIR)/include/mentok/store/hexastore
	mkdir -p $(INSTDIR)/include/mentok/store/tokyocabinet
	cp mentok/algebra/*.h $(INSTDIR)/include/mentok/algebra/
	cp mentok/engine/*.h $(INSTDIR)/include/mentok/engine/
	cp mentok/rdf/*.h $(INSTDIR)/include/mentok/rdf/
	cp mentok/parser/*.h $(INSTDIR)/include/mentok/parser/
	cp mentok/store/store.h $(INSTDIR)/include/mentok/store/
	cp mentok/misc/*.h $(INSTDIR)/include/mentok/misc/
	cp mentok/store/hexastore/*.h $(INSTDIR)/include/mentok/store/hexastore/
	cp mentok/store/tokyocabinet/*.h $(INSTDIR)/include/mentok/store/tokyocabinet/
	cp mentok/*.h $(INSTDIR)/include/mentok/
	cp libmentok.dylib $(INSTDIR)/lib/

libmentok.dylib: $(OBJECTS)
	libtool -dynamic -flat_namespace -install_name $(INSTDIR)/lib/libmentok.dylib -current_version 0.1 $(LIBS) -o libmentok.dylib  $(OBJECTS)

