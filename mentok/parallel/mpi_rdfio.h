#ifndef _MPI_RDFIO_H
#define _MPI_RDFIO_H

#include "mpi.h"
#include "mentok/mentok.h"

int mpi_rdfio_readnt(char *filename, char *mapfilename, size_t bufsize, hx_model **store, MPI_Comm comm);

#endif /* _MPI_RDFIO_H */
