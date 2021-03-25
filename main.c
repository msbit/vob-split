#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <dvdread/ifo_read.h>

#include "split.h"

void usage(int, char **, FILE *);

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stdout, "2 arguments required\n");
    usage(argc, argv, stdout);
    exit(-1);
  }

  char *endptr;
  size_t title = strtoimax(argv[2], &endptr, 10);
  if (*endptr != '\0') {
    fprintf(stdout, "title-index must be numeric\n");
    usage(argc, argv, stdout);
    exit(-2);
  }

  char *path = realpath(argv[1], NULL);
  if (path == NULL) {
    perror("realpath");
    exit(-3);
  }

  dvd_reader_t *dvd = DVDOpen(path);
  if (dvd == NULL) {
    perror("DVDOpen");
    exit(-4);
  }

  ifo_handle_t *vmg = ifoOpen(dvd, 0);
  if (vmg == NULL) {
    perror("ifoOpen");
    exit(-5);
  }

  ifo_handle_t *ifo = ifoOpen(dvd, title);
  if (ifo == NULL) {
    perror("ifoOpen");
    exit(-6);
  }

  struct extent_t *pgc_extents;

  size_t pgc_extent_count = populate_pgc_extents(ifo->vts_pgcit, &pgc_extents);

  struct extent_t *vob_extents;

  size_t vob_extent_count = populate_vob_extents(path, title, &vob_extents);

  split(path, title, pgc_extents, pgc_extent_count, vob_extents,
        vob_extent_count);

  DVDClose(dvd);

  free(vob_extents);
  free(pgc_extents);
}

void usage(int argc, char **argv, FILE *f) {
  fprintf(f, "Usage: %s </path/to/VIDEO_TS> <title-index>\n", argv[0]);
}
