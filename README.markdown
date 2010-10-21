# Mentok
## An RDF storage and query framework

Mentok is a library for storing and querying RDF data. Its main purpose at this
point is to provide a bases for testing new approaches to query evaluation and
not meant to be a fully functional or supported system. There's not any
documentation to speak of, but there are some small example programs included
that can hopefully provide a starting point for anyone wanting to dive into the
code.

*NOTE*: Despite it's name, this project is not the Hexastore described by
Weiss, et. al (VLDB08). It started as a re-implementation of the Hexastore
system as described in their publication, but has no official connection with
that work.

### Requirements

* [Raptor](http://librdf.org/raptor/)
* [Tokyo Cabinet](http://1978th.net/tokyocabinet/)
* [MPI](http://www.open-mpi.org/) (for cluster support)
* [Bison](http://www.gnu.org/software/bison/)
* [Flex](http://flex.sourceforge.net/)
* [Libtool](http://www.gnu.org/software/libtool/)

### Contributors

* [Gregory Todd Williams](mailto:greg@evilfunhouse.com)
* [Jesse Weaver](http://www.cs.rpi.edu/~weavej3/)

