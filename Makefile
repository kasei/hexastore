# CFLAGS	=	-O3 -I. -L. -I/ext/local/include -L/ext/local/lib -std=gnu99 -pedantic -Wall -Wno-unused-value -Wno-unused-variable -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Werror -Wno-uninitialized # -DDEBUG_INDEX_SELECTION
CFLAGS		=	-I. -L. -I/ext/local/include -L/ext/local/lib -std=gnu99 -pedantic -ggdb -Wall -Wno-unused-value -Wno-unused-variable -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -DDEBUG # -Werror # -DDEBUG_INDEX_SELECTION
CC			=	gcc $(CFLAGS)

LIBS	=	-lpthread -lraptor -L/cs/willig4/local/lib -I/cs/willig4/local/include
OBJECTS	=	hexastore.o index.o terminal.o vector.o head.o avl.o nodemap.o node.o variablebindings.o mergejoin.o materialize.o filter.o triple.o btree.o storage.o parser.o bgp.o expr.o SPARQLParser.o SPARQLScanner.o graphpattern.o project.o

all: parse print optimize tests examples parse_query

server: server.c $(OBJECTS)
	$(CC) $(INC) $(LIBS) -ldrizzle -o server server.c $(OBJECTS)

parse: parse.c $(OBJECTS)
	$(CC) $(INC) $(LIBS) -o parse parse.c $(OBJECTS)

optimize: optimize.c $(OBJECTS)
	$(CC) $(INC) $(LIBS) -o optimize optimize.c $(OBJECTS)

print: print.c $(OBJECTS)
	$(CC) $(INC) $(LIBS) -o print print.c $(OBJECTS)

hexastore.o: hexastore.c hexastore.h index.h head.h vector.h terminal.h hexastore_types.h variablebindings.h nodemap.h
	$(CC) $(INC) -c hexastore.c

index.o: index.c index.h terminal.h vector.h head.h hexastore_types.h
	$(CC) $(INC) -c index.c

terminal.o: terminal.c terminal.h hexastore_types.h
	$(CC) $(INC) -c terminal.c

vector.o: vector.c vector.h terminal.h hexastore_types.h
	$(CC) $(INC) -c vector.c

head.o: head.c head.h vector.h terminal.h btree.h hexastore_types.h
	$(CC) $(INC) -c head.c

node.o: node.c node.h hexastore_types.h
	$(CC) $(INC) -c node.c
	
nodemap.o: nodemap.c nodemap.h avl.h hexastore_types.h
	$(CC) $(INC) -c nodemap.c

mergejoin.o: mergejoin.c mergejoin.h hexastore_types.h variablebindings.h
	$(CC) $(INC) -c mergejoin.c

variablebindings.o: variablebindings.c variablebindings.h hexastore_types.h node.h index.h nodemap.h
	$(CC) $(INC) -c variablebindings.c

materialize.o: materialize.c materialize.h hexastore_types.h node.h index.h nodemap.h
	$(CC) $(INC) -c materialize.c

filter.o: filter.c filter.h hexastore_types.h node.h index.h nodemap.h
	$(CC) $(INC) -c filter.c

triple.o: triple.c triple.h hexastore_types.h
	$(CC) $(INC) -c triple.c

btree.o: btree.c btree.h hexastore_types.h
	$(CC) $(INC) -c btree.c

parser.o: parser.c parser.h hexastore_types.h
	$(CC) $(INC) -c parser.c

bgp.o: bgp.c bgp.h hexastore_types.h
	$(CC) $(INC) -c bgp.c

expr.o: expr.c expr.h hexastore_types.h
	$(CC) $(INC) -c expr.c

storage.o: storage.c storage.h hexastore_types.h
	$(CC) $(INC) -c storage.c

graphpattern.o: graphpattern.c graphpattern.h hexastore_types.h
	$(CC) $(INC) -c graphpattern.c

project.o: project.c project.h hexastore_types.h
	$(CC) $(INC) -c project.c

########

SPARQLParser.c:
SPARQLParser.h: SPARQLParser.yy
	bison -o SPARQLParser.c SPARQLParser.yy

SPARQLScanner.h: SPARQLScanner.c SPARQLScanner.ll

SPARQLScanner.c: SPARQLScanner.ll
	flex -o SPARQLScanner.c SPARQLScanner.ll

SPARQLParser.o: SPARQLParser.yy SPARQLScanner.ll SPARQLParser.h SPARQLScanner.h
	$(CC) -DYYTEXT_POINTER=1 -W -Wall -Wextra -ansi -g -c  -o SPARQLParser.o SPARQLParser.c

SPARQLScanner.o: SPARQLScanner.c SPARQLParser.h SPARQLScanner.h
	$(CC) -DYYTEXT_POINTER=1 -Wextra -ansi -g -c  -o SPARQLScanner.o SPARQLScanner.c

parse_query: parse_query.c SPARQLParser.o SPARQLScanner.o
	$(CC) $(INC) $(LIBS) -o parse_query parse_query.c $(OBJECTS)

########

tests: t/nodemap.t t/node.t t/expr.t t/index.t t/terminal.t t/vector.t t/head.t t/btree.t t/join.t t/iter.t t/bgp.t t/materialize.t t/selectivity.t t/filter.t t/graphpattern.t t/parser.t t/variablebindings.t t/project.t

examples: examples/lubm_q4 examples/lubm_q8 examples/lubm_q9 examples/bench examples/knows

bitmat: examples/lubm7_6m examples/lubm8_6m examples/lubm16_6m

########
t/node.t: tap.o t/node.c node.h node.o $(OBJECTS) tap.o
	$(CC) $(INC) $(LIBS) -o t/node.t t/node.c $(OBJECTS) tap.o

t/expr.t: tap.o t/expr.c expr.h expr.o $(OBJECTS) tap.o
	$(CC) $(INC) $(LIBS) -o t/expr.t t/expr.c $(OBJECTS) tap.o

t/nodemap.t: tap.o t/nodemap.c nodemap.h nodemap.o $(OBJECTS) tap.o
	$(CC) $(INC) $(LIBS) -o t/nodemap.t t/nodemap.c $(OBJECTS) tap.o

t/index.t: tap.o t/index.c index.h index.o $(OBJECTS) tap.o
	$(CC) $(INC) $(LIBS) -o t/index.t t/index.c $(OBJECTS) tap.o

t/terminal.t: tap.o t/terminal.c terminal.h terminal.o $(OBJECTS) tap.o
	$(CC) $(INC) $(LIBS) -o t/terminal.t t/terminal.c $(OBJECTS) tap.o

t/vector.t: tap.o t/vector.c vector.h vector.o $(OBJECTS) tap.o
	$(CC) $(INC) $(LIBS) -o t/vector.t t/vector.c $(OBJECTS) tap.o

t/head.t: tap.o t/head.c head.h head.o $(OBJECTS) tap.o
	$(CC) $(INC) $(LIBS) -o t/head.t t/head.c $(OBJECTS) tap.o

t/btree.t: tap.o t/btree.c btree.h btree.o $(OBJECTS) tap.o
	$(CC) $(INC) $(LIBS) -o t/btree.t t/btree.c $(OBJECTS) tap.o

t/join.t: tap.o t/join.c $(OBJECTS) tap.o
	$(CC) $(INC) $(LIBS) -o t/join.t t/join.c $(OBJECTS) tap.o

t/iter.t: tap.o t/iter.c $(OBJECTS) tap.o
	$(CC) $(INC) $(LIBS) -o t/iter.t t/iter.c $(OBJECTS) tap.o

t/bgp.t: tap.o t/bgp.c $(OBJECTS) tap.o
	$(CC) $(INC) $(LIBS) -o t/bgp.t t/bgp.c $(OBJECTS) tap.o

t/filter.t: tap.o t/filter.c $(OBJECTS) tap.o
	$(CC) $(INC) $(LIBS) -o t/filter.t t/filter.c $(OBJECTS) tap.o

t/materialize.t: tap.o t/materialize.c $(OBJECTS) tap.o
	$(CC) $(INC) $(LIBS) -o t/materialize.t t/materialize.c $(OBJECTS) tap.o

t/selectivity.t: tap.o t/selectivity.c $(OBJECTS) tap.o
	$(CC) $(INC) $(LIBS) -o t/selectivity.t t/selectivity.c $(OBJECTS) tap.o

t/graphpattern.t: tap.o t/graphpattern.c $(OBJECTS) tap.o
	$(CC) $(INC) $(LIBS) -o t/graphpattern.t t/graphpattern.c $(OBJECTS) tap.o

t/parser.t: tap.o t/parser.c $(OBJECTS) tap.o
	$(CC) $(INC) $(LIBS) -o t/parser.t t/parser.c $(OBJECTS) tap.o

t/variablebindings.t: tap.o t/variablebindings.c $(OBJECTS) tap.o
	$(CC) $(INC) $(LIBS) -o t/variablebindings.t t/variablebindings.c $(OBJECTS) tap.o

t/project.t: tap.o t/project.c $(OBJECTS) tap.o
	$(CC) $(INC) $(LIBS) -o t/project.t t/project.c $(OBJECTS) tap.o

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

########

avl.o: avl.c avl.h hexastore_types.h
	$(CC) $(INC) -c avl.c

tap.o: tap.c tap.h
	$(CC) $(INC) -c tap.c

clean:
	rm -f examples/lubm_q[489] examples/bench examples/knows
	rm -rf examples/lubm7_6m examples/lubm7_6m.dSYM
	rm -rf examples/lubm8_6m examples/lubm8_6m.dSYM
	rm -rf examples/lubm16_6m examples/lubm16_6m.dSYM
	rm -rf examples/lubm_q[489].dSYM examples/bench.dSYM examples/knows.dSYM
	rm -f test parse print optimize a.out server parse_query
	rm -f *.o
	rm -rf *.dSYM t/*.dSYM
	rm -f t/*.t
	rm -f SPARQL SPARQLParser.o SPARQLScanner.o SPARQLParser.c SPARQLScanner.c SPARQLParser.h
	rm -f stack.h position.h location.h
