#ifndef _MPI_FILE_NTRIPLES_ITERATOR_H
#define _MPI_FILE_NTRIPLES_ITERATOR_H

#include "parallel/mpi_file_iterator.h"

iterator_t mpi_file_ntriples_iterator_create(MPI_File file, MPI_Offset start, MPI_Offset amount, size_t bufsize, MPI_Comm comm);

#endif /* _MPI_FILE_NTRIPLES_ITERATOR_H */
