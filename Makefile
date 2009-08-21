CFLAGS	= -I. -L. -I/ext/local/include -L/ext/local/lib -std=gnu99 -pedantic -ggdb -Wall -Wno-unused-variable -std=c99
# CFLAGS	= -O3 -I. -L. -I/ext/local/include -L/ext/local/lib -std=gnu99 -pedantic -I/gpfs/large/DSSW/redland/local/include -L/gpfs/large/DSSW/redland/local/lib -I/gpfs/large/DSSW/tokyocabinet/include -L/gpfs/large/DSSW/tokyocabinet/lib
# CFLAGS	= -I. -L. -I/ext/local/include -L/ext/local/lib -std=gnu99 -pedantic -ggdb -Wall -Wno-unused-value -Wno-unused-variable -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -DDEBUG # -Werror -DTHREADING -DDEBUG_INDEX_SELECTION
# CFLAGS		= -I. -L. -I/gpfs/large/DSSW/redland/local/include -L/gpfs/large/DSSW/redland/local/lib -I/ext/local/include -L/ext/local/lib -DDEBUG -ggdb # -Werror -DTHREADING -DDEBUG_INDEX_SELECTION
CC			= gcc $(CFLAGS)

LIBS	=	-lz -lpthread -lraptor -L/cs/willig4/local/lib -I/cs/willig4/local/include
OBJECTS	=	hexastore.o index.o store/hexastore/terminal.o store/hexastore/vector.o store/hexastore/head.o misc/avl.o misc/nodemap.o rdf/node.o engine/variablebindings.o engine/nestedloopjoin.o engine/rendezvousjoin.o engine/mergejoin.o engine/materialize.o engine/filter.o rdf/triple.o store/hexastore/btree.o parser/parser.o algebra/bgp.o algebra/expr.o SPARQLParser.o SPARQLScanner.o algebra/graphpattern.o engine/project.o misc/util.o
MPI_OBJECTS	= parallel/safealloc.o parallel/async_mpi.o parallel/async_des.o parallel/parallel.o parallel/mpi_file_iterator.o parallel/mpi_file_ntriples_iterator.o parallel/mpi_file_ntriples_node_iterator.o parallel/mpi_rdfio.o parallel/genmap/avl_tree_map.o parallel/genmap/iterator.o parallel/genmap/map.o

default: parse print optimize tests examples parse_query

all: sparql parse print optimize tests examples parse_query dumpmap assign_ids

server: cli/server.c $(OBJECTS)
	$(CC) $(INC) $(LIBS) -ldrizzle -o server cli/server.c $(OBJECTS)

assign_ids: cli/assign_ids.c $(OBJECTS)
	$(CC) $(INC) $(LIBS) -ltokyocabinet -o assign_ids cli/assign_ids.c $(OBJECTS)

parse: cli/parse.c $(OBJECTS)
	$(CC) $(INC) $(LIBS) -o parse cli/parse.c $(OBJECTS)

optimize: cli/optimize.c $(OBJECTS)
	$(CC) $(INC) $(LIBS) -o optimize cli/optimize.c $(OBJECTS)

print: cli/print.c $(OBJECTS)
	$(CC) $(INC) $(LIBS) -o print cli/print.c $(OBJECTS)

parse_query: cli/parse_query.c SPARQLParser.o SPARQLScanner.o $(OBJECTS)
	$(CC) $(INC) $(LIBS) -o parse_query cli/parse_query.c $(OBJECTS)

dumpmap: cli/dumpmap.c $(OBJECTS)
	$(CC) $(INC) $(LIBS) -o dumpmap cli/dumpmap.c $(OBJECTS)

hexastore.o: hexastore.c hexastore.h index.h store/hexastore/head.h store/hexastore/vector.h store/hexastore/terminal.h hexastore_types.h engine/variablebindings.h misc/nodemap.h
	$(CC) $(INC) -c hexastore.c

index.o: index.c index.h store/hexastore/terminal.h store/hexastore/vector.h store/hexastore/head.h hexastore_types.h
	$(CC) $(INC) -c index.c

store/hexastore/terminal.o: store/hexastore/terminal.c store/hexastore/terminal.h hexastore_types.h
	$(CC) $(INC) -c -o store/hexastore/terminal.o store/hexastore/terminal.c

store/hexastore/vector.o: store/hexastore/vector.c store/hexastore/vector.h store/hexastore/terminal.h hexastore_types.h
	$(CC) $(INC) -c -o store/hexastore/vector.o store/hexastore/vector.c

store/hexastore/head.o: store/hexastore/head.c store/hexastore/head.h store/hexastore/vector.h store/hexastore/terminal.h store/hexastore/btree.h hexastore_types.h
	$(CC) $(INC) -c -o store/hexastore/head.o store/hexastore/head.c

rdf/node.o: rdf/node.c rdf/node.h hexastore_types.h
	$(CC) $(INC) -c -o rdf/node.o rdf/node.c
	
misc/nodemap.o: misc/nodemap.c misc/nodemap.h misc/avl.h hexastore_types.h
	$(CC) $(INC) -c -o misc/nodemap.o misc/nodemap.c

engine/mergejoin.o: engine/mergejoin.c engine/mergejoin.h hexastore_types.h engine/variablebindings.h
	$(CC) $(INC) -c -o engine/mergejoin.o engine/mergejoin.c

engine/rendezvousjoin.o: engine/rendezvousjoin.c engine/rendezvousjoin.h hexastore_types.h engine/variablebindings.h
	$(CC) $(INC) -c -o engine/rendezvousjoin.o engine/rendezvousjoin.c

engine/nestedloopjoin.o: engine/nestedloopjoin.c engine/nestedloopjoin.h hexastore_types.h engine/variablebindings.h
	$(CC) $(INC) -c -o engine/nestedloopjoin.o engine/nestedloopjoin.c

engine/variablebindings.o: engine/variablebindings.c engine/variablebindings.h hexastore_types.h rdf/node.h index.h misc/nodemap.h
	$(CC) $(INC) -c -o engine/variablebindings.o engine/variablebindings.c

engine/materialize.o: engine/materialize.c engine/materialize.h hexastore_types.h rdf/node.h index.h misc/nodemap.h
	$(CC) $(INC) -c -o engine/materialize.o engine/materialize.c

engine/filter.o: engine/filter.c engine/filter.h hexastore_types.h rdf/node.h index.h misc/nodemap.h
	$(CC) $(INC) -c -o engine/filter.o engine/filter.c

rdf/triple.o: rdf/triple.c rdf/triple.h hexastore_types.h
	$(CC) $(INC) -c -o rdf/triple.o rdf/triple.c

store/hexastore/btree.o: store/hexastore/btree.c store/hexastore/btree.h hexastore_types.h
	$(CC) $(INC) -c -o store/hexastore/btree.o store/hexastore/btree.c

parser/parser.o: parser/parser.c parser/parser.h hexastore_types.h
	$(CC) $(INC) -c -o parser/parser.o parser/parser.c

algebra/bgp.o: algebra/bgp.c algebra/bgp.h hexastore_types.h
	$(CC) $(INC) -c -o algebra/bgp.o algebra/bgp.c

algebra/expr.o: algebra/expr.c algebra/expr.h hexastore_types.h
	$(CC) $(INC) -c -o algebra/expr.o algebra/expr.c

algebra/graphpattern.o: algebra/graphpattern.c algebra/graphpattern.h hexastore_types.h
	$(CC) $(INC) -c -o algebra/graphpattern.o algebra/graphpattern.c

engine/project.o: engine/project.c engine/project.h hexastore_types.h
	$(CC) $(INC) -c -o engine/project.o engine/project.c

misc/util.o: misc/util.c misc/util.h
	$(CC) $(INC) -c -o misc/util.o misc/util.c

parallel/safealloc.o: parallel/safealloc.c parallel/safealloc.h
	$(CC) $(INC) -c -o parallel/safealloc.o parallel/safealloc.c

parallel/async_mpi.o: parallel/async_mpi.c parallel/async_mpi.h parallel/safealloc.h async.h
	$(CC) $(INC) -c -o parallel/async_mpi.o parallel/async_mpi.c

parallel/async_des.o: parallel/async_des.c parallel/async_des.h parallel/async_mpi.h parallel/safealloc.h
	$(CC) $(INC) -c -o parallel/async_des.o parallel/async_des.c

parallel/parallel.o: parallel/parallel.c parallel/parallel.h parallel/async_des.o parallel/async_mpi.o parallel/safealloc.o
	$(CC) $(INC) -c -o parallel/parallel.o parallel/parallel.c

########

# SPARQLParser.c:
# SPARQLParser.h: SPARQLParser.yy
sparql: SPARQLParser.yy SPARQLScanner.ll
	bison -o SPARQLParser.c SPARQLParser.yy
	flex -o SPARQLScanner.c SPARQLScanner.ll

SPARQLScanner.h: SPARQLScanner.c SPARQLScanner.ll

SPARQLScanner.c: SPARQLScanner.ll

SPARQLParser.o: SPARQLParser.yy SPARQLScanner.ll SPARQLParser.h SPARQLScanner.h
	$(CC) -DYYTEXT_POINTER=1 -W -Wall -Wextra -ansi -g -c  -o SPARQLParser.o SPARQLParser.c

SPARQLScanner.o: SPARQLScanner.c SPARQLParser.h SPARQLScanner.h
	$(CC) -DYYTEXT_POINTER=1 -Wextra -ansi -g -c  -o SPARQLScanner.o SPARQLScanner.c

########
# jesse's mpi file io stuff:

parallel/mpi_file_iterator.o: parallel/mpi_file_iterator.c parallel/mpi_file_iterator.h parallel/genmap/iterator.h parallel/genmap/buffer.h
	$(CC) $(INC) -c -o parallel/mpi_file_iterator.o parallel/mpi_file_iterator.c

parallel/mpi_file_ntriples_iterator.o: parallel/mpi_file_ntriples_iterator.c parallel/mpi_file_ntriples_iterator.h parallel/mpi_file_iterator.h
	$(CC) $(INC) -c -o parallel/mpi_file_ntriples_iterator.o parallel/mpi_file_ntriples_iterator.c

parallel/mpi_file_ntriples_node_iterator.o: parallel/mpi_file_ntriples_node_iterator.c parallel/mpi_file_ntriples_node_iterator.h parallel/mpi_file_ntriples_iterator.h
	$(CC) $(INC) -c -o parallel/mpi_file_ntriples_node_iterator.o parallel/mpi_file_ntriples_node_iterator.c
	
parallel/mpi_rdfio.o: parallel/mpi_rdfio.c parallel/mpi_rdfio.h hexastore.h parallel/mpi_file_ntriples_node_iterator.h parallel/async_des.h parallel/genmap/avl_tree_map.h
	$(CC) $(INC) -c -o parallel/safealloc.o parallel/safealloc.o

parallel/genmap/avl_tree_map.o: parallel/genmap/avl_tree_map.c parallel/genmap/avl_tree_map.h parallel/genmap/map.h parallel/genmap/iterator.h misc/avl.h
	$(CC) $(INC) -c parallel/genmap/avl_tree_map.c -o parallel/genmap/avl_tree_map.o

parallel/genmap/iterator.o: parallel/genmap/iterator.c parallel/genmap/iterator.h
	$(CC) $(INC) -c parallel/genmap/iterator.c -o parallel/genmap/iterator.o

parallel/genmap/map.o: parallel/genmap/map.c parallel/genmap/map.h parallel/genmap/iterator.h
	$(CC) $(INC) -c parallel/genmap/map.c -o parallel/genmap/map.o

########

libhx.o: $(OBJECTS)
	libtool $(OBJECTS) -o libhx.o

########

tests: t/nodemap.t t/node.t t/expr.t t/index.t t/terminal.t t/vector.t t/head.t t/btree.t t/join.t t/iter.t t/bgp.t t/materialize.t t/selectivity.t t/filter.t t/graphpattern.t t/parser.t t/variablebindings.t t/project.t t/triple.t

examples: examples/lubm_q4 examples/lubm_q8 examples/lubm_q9 examples/bench examples/knows

mpi: examples/mpi

bitmat: examples/lubm7_6m examples/lubm8_6m examples/lubm16_6m

########
t/node.t: test/tap.o t/node.c rdf/node.h rdf/node.o $(OBJECTS) test/tap.o
	$(CC) $(INC) $(LIBS) -o t/node.t t/node.c $(OBJECTS) test/tap.o

t/expr.t: test/tap.o t/expr.c algebra/expr.h algebra/expr.o $(OBJECTS) test/tap.o
	$(CC) $(INC) $(LIBS) -o t/expr.t t/expr.c $(OBJECTS) test/tap.o

t/nodemap.t: test/tap.o t/nodemap.c misc/nodemap.h misc/nodemap.o $(OBJECTS) test/tap.o
	$(CC) $(INC) $(LIBS) -o t/nodemap.t t/nodemap.c $(OBJECTS) test/tap.o

t/index.t: test/tap.o t/index.c index.h index.o $(OBJECTS) test/tap.o
	$(CC) $(INC) $(LIBS) -o t/index.t t/index.c $(OBJECTS) test/tap.o

t/terminal.t: test/tap.o t/terminal.c store/hexastore/terminal.h store/hexastore/terminal.o $(OBJECTS) test/tap.o
	$(CC) $(INC) $(LIBS) -o t/terminal.t t/terminal.c $(OBJECTS) test/tap.o

t/vector.t: test/tap.o t/vector.c store/hexastore/vector.h store/hexastore/vector.o $(OBJECTS) test/tap.o
	$(CC) $(INC) $(LIBS) -o t/vector.t t/vector.c $(OBJECTS) test/tap.o

t/head.t: test/tap.o t/head.c store/hexastore/head.h store/hexastore/head.o $(OBJECTS) test/tap.o
	$(CC) $(INC) $(LIBS) -o t/head.t t/head.c $(OBJECTS) test/tap.o

t/btree.t: test/tap.o t/btree.c store/hexastore/btree.h store/hexastore/btree.o $(OBJECTS) test/tap.o
	$(CC) $(INC) $(LIBS) -o t/btree.t t/btree.c $(OBJECTS) test/tap.o

t/join.t: test/tap.o t/join.c $(OBJECTS) test/tap.o
	$(CC) $(INC) $(LIBS) -o t/join.t t/join.c $(OBJECTS) test/tap.o

t/iter.t: test/tap.o t/iter.c $(OBJECTS) test/tap.o
	$(CC) $(INC) $(LIBS) -o t/iter.t t/iter.c $(OBJECTS) test/tap.o

t/bgp.t: test/tap.o t/bgp.c $(OBJECTS) test/tap.o
	$(CC) $(INC) $(LIBS) -o t/bgp.t t/bgp.c $(OBJECTS) test/tap.o

t/filter.t: test/tap.o t/filter.c $(OBJECTS) test/tap.o
	$(CC) $(INC) $(LIBS) -o t/filter.t t/filter.c $(OBJECTS) test/tap.o

t/materialize.t: test/tap.o t/materialize.c $(OBJECTS) test/tap.o
	$(CC) $(INC) $(LIBS) -o t/materialize.t t/materialize.c $(OBJECTS) test/tap.o

t/selectivity.t: test/tap.o t/selectivity.c $(OBJECTS) test/tap.o
	$(CC) $(INC) $(LIBS) -o t/selectivity.t t/selectivity.c $(OBJECTS) test/tap.o

t/graphpattern.t: test/tap.o t/graphpattern.c $(OBJECTS) test/tap.o
	$(CC) $(INC) $(LIBS) -o t/graphpattern.t t/graphpattern.c $(OBJECTS) test/tap.o

t/parser.t: test/tap.o t/parser.c $(OBJECTS) test/tap.o
	$(CC) $(INC) $(LIBS) -o t/parser.t t/parser.c $(OBJECTS) test/tap.o

t/variablebindings.t: test/tap.o t/variablebindings.c $(OBJECTS) test/tap.o
	$(CC) $(INC) $(LIBS) -o t/variablebindings.t t/variablebindings.c $(OBJECTS) test/tap.o

t/project.t: test/tap.o t/project.c $(OBJECTS) test/tap.o
	$(CC) $(INC) $(LIBS) -o t/project.t t/project.c $(OBJECTS) test/tap.o

t/triple.t: test/tap.o t/triple.c $(OBJECTS) test/tap.o
	$(CC) $(INC) $(LIBS) -o t/triple.t t/triple.c $(OBJECTS) test/tap.o

########

examples/lubm_q4: examples/lubm_q4.c $(OBJECTS)
	$(CC) $(INC) $(LIBS) -o examples/lubm_q4 examples/lubm_q4.c $(OBJECTS)

examples/lubm_q8: examples/lubm_q8.c $(OBJECTS)
	$(CC) $(INC) $(LIBS) -o examples/lubm_q8 examples/lubm_q8.c $(OBJECTS)

examples/lubm_q9: examples/lubm_q9.c $(OBJECTS)
	$(CC) $(INC) $(LIBS) -o examples/lubm_q9 examples/lubm_q9.c $(OBJECTS)

examples/bench: examples/bench.c $(OBJECTS)
	$(CC) $(INC) $(LIBS) -o examples/bench examples/bench.c $(OBJECTS)

examples/knows: examples/knows.c $(OBJECTS)
	$(CC) $(INC) $(LIBS) -o examples/knows examples/knows.c $(OBJECTS)

examples/lubm7_6m: examples/lubm7_6m.c $(OBJECTS)
	$(CC) $(INC) $(LIBS) -o examples/lubm7_6m examples/lubm7_6m.c $(OBJECTS)

examples/lubm8_6m: examples/lubm8_6m.c $(OBJECTS)
	$(CC) $(INC) $(LIBS) -o examples/lubm8_6m examples/lubm8_6m.c $(OBJECTS)

examples/lubm16_6m: examples/lubm16_6m.c $(OBJECTS)
	$(CC) $(INC) $(LIBS) -o examples/lubm16_6m examples/lubm16_6m.c $(OBJECTS)

examples/mpi: examples/mpi.c $(OBJECTS) $(MPI_OBJECTS)
	$(CC) -lmpi $(INC) $(LIBS) -o examples/mpi examples/mpi.c $(OBJECTS) $(MPI_OBJECTS)

########

misc/avl.o: misc/avl.c misc/avl.h hexastore_types.h
	$(CC) $(INC) -c -o misc/avl.o misc/avl.c

test/tap.o: test/tap.c test/tap.h
	$(CC) $(INC) -c -o test/tap.o test/tap.c

clean:
	rm -f examples/lubm_q[489] examples/bench examples/knows examples/mpi
	rm -rf examples/lubm7_6m examples/lubm7_6m.dSYM
	rm -rf examples/lubm8_6m examples/lubm8_6m.dSYM
	rm -rf examples/lubm16_6m examples/lubm16_6m.dSYM
	rm -rf examples/lubm_q[489].dSYM examples/bench.dSYM examples/knows.dSYM examples/mpi.dSYM
	rm -f parse print optimize a.out server parse_query dumpmap assign_ids
	rm -f *.o */*.o */*/*.o parallel/genmap/*.o
	rm -rf *.dSYM t/*.dSYM
	rm -f t/*.t
	rm -f SPARQL SPARQLParser.o SPARQLScanner.o SPARQLParser.c SPARQLScanner.c SPARQLParser.h
	rm -f stack.h position.h location.h
