#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

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

  char path[PATH_MAX];
  if (realpath(argv[1], path) == NULL) {
    perror("realpath");
    exit(-3);
  }

  struct extent_t *pgc_extents;
  size_t pgc_extent_count = populate_pgc_extents(path, title, &pgc_extents);

  struct extent_t *vob_extents;
  size_t vob_extent_count = populate_vob_extents(path, title, &vob_extents);

  split(path, title,
        pgc_extents, pgc_extent_count,
        vob_extents, vob_extent_count);

  free(vob_extents);
  free(pgc_extents);
}

void usage(int argc, char **argv, FILE *f) {
  fprintf(f, "Usage: %s </path/to/VIDEO_TS> <title-index>\n", argv[0]);
}
