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
    return -1;
  }

  char *endptr;
  size_t title = strtoimax(argv[2], &endptr, 10);
  if (*endptr != '\0') {
    fprintf(stdout, "title-index must be numeric\n");
    usage(argc, argv, stdout);
    return -2;
  }

  if (title == 0) {
    fprintf(stdout, "title-index must be greater than 0\n");
    usage(argc, argv, stdout);
    return -3;
  }

  char path[PATH_MAX];
  if (realpath(argv[1], path) == NULL) {
    perror("realpath");
    return -4;
  }

  extent_t *pgc_extents;
  int pgc_extent_count = populate_pgc_extents(path, title, &pgc_extents);
  if (pgc_extent_count < 0) {
    return pgc_extent_count;
  }

  extent_t *vob_extents;
  int vob_extent_count = populate_vob_extents(path, title, &vob_extents);
  if (vob_extent_count < 0) {
    free(pgc_extents);
    return vob_extent_count;
  }

  int result = split(path, title, pgc_extents, pgc_extent_count, vob_extents, vob_extent_count);
  if (result < 0) {
    free(pgc_extents);
    free(vob_extents);
    return result;
  }

  free(pgc_extents);
  free(vob_extents);

  return 0;
}

void usage(int argc, char **argv, FILE *f) {
  fprintf(f, "Usage: %s </path/to/VIDEO_TS> <title-index>\n", argv[0]);
}
