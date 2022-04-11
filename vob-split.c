#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "split.h"

void usage(int, char **, FILE *);

int main(int argc, char **argv) {
  int exit_code = CHAR_MAX;

  if (argc < 3) {
    fprintf(stdout, "2 arguments required\n");
    usage(argc, argv, stdout);
    exit_code = -1;
    goto defer_none;
  }

  char *endptr;
  size_t title = strtoimax(argv[2], &endptr, 10);
  if (*endptr != '\0') {
    fprintf(stdout, "title-index must be numeric\n");
    usage(argc, argv, stdout);
    exit_code = -2;
    goto defer_none;
  }

  if (title == 0) {
    fprintf(stdout, "title-index must be greater than 0\n");
    usage(argc, argv, stdout);
    exit_code = -3;
    goto defer_none;
  }

  char path[PATH_MAX];
  if (realpath(argv[1], path) == NULL) {
    perror("realpath");
    exit_code = -4;
    goto defer_none;
  }

  extent_t *pgc_extents;
  int pgc_extent_count = populate_pgc_extents(path, title, &pgc_extents);
  if (pgc_extent_count < 0) {
    exit_code = pgc_extent_count;
    goto defer_none;
  }

  extent_t *vob_extents;
  int vob_extent_count = populate_vob_extents(path, title, &vob_extents);
  if (vob_extent_count < 0) {
    exit_code = vob_extent_count;
    goto defer_pgc_extents;
  }

  int result = split(path, title, pgc_extents, pgc_extent_count, vob_extents,
                     vob_extent_count);
  if (result < 0) {
    exit_code = result;
    goto defer_vob_extents;
  }

  exit_code = 0;

defer_vob_extents:
  free(vob_extents);

defer_pgc_extents:
  free(pgc_extents);

defer_none:
  return exit_code;
}

void usage(int argc, char **argv, FILE *f) {
  fprintf(f, "Usage: %s </path/to/VIDEO_TS> <title-index>\n", argv[0]);
}
