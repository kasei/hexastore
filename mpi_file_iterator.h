#ifndef _MPI_FILE_ITERATOR_H
#define _MPI_FILE_ITERATOR_H

#include "mpi.h"
#include "genmap/iterator.h"
#include "genmap/buffer.h"

iterator_t mpi_file_iterator_create(MPI_File file, MPI_Offset start, MPI_Offset amount, size_t bufsize);

#endif /* _MPI_FILE_ITERATOR_H */
