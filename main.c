#include <dirent.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define DVD_SECTOR_SIZE 2048
#define MAX_VOB_PER_VTS 10

typedef struct {
  uint32_t first;
  uint32_t last;
} extent_t;

size_t populate_pgc_extents(pgcit_t *, extent_t **);
size_t populate_vob_extents(char *, size_t, extent_t **);
void split(char *, size_t, extent_t *, size_t, extent_t *, size_t);
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

  extent_t *pgc_extents;

  size_t pgc_extent_count = populate_pgc_extents(ifo->vts_pgcit, &pgc_extents);

  extent_t *vob_extents;

  size_t vob_extent_count = populate_vob_extents(path, title, &vob_extents);

  split(path, title, pgc_extents, pgc_extent_count, vob_extents,
        vob_extent_count);

  DVDClose(dvd);

  free(vob_extents);
  free(pgc_extents);
}

size_t populate_pgc_extents(pgcit_t *pgcit, extent_t **extents) {
  *extents = malloc(sizeof(extent_t) * pgcit->nr_of_pgci_srp);

  for (size_t i = 0; i < pgcit->nr_of_pgci_srp; i++) {
    pgc_t *pgc = pgcit->pgci_srp[i].pgc;
    uint32_t first = pgc->cell_playback[0].first_sector;
    uint32_t last = pgc->cell_playback[pgc->nr_of_cells - 1].last_sector;
    (*extents)[i] = (extent_t){first, last};
  }

  return pgcit->nr_of_pgci_srp;
}

size_t populate_vob_extents(char *path, size_t title, extent_t **extents) {
  *extents = malloc(sizeof(extent_t) * MAX_VOB_PER_VTS);

  DIR *d;
  struct dirent *dir;
  d = opendir(path);
  if (d == NULL) {
    perror("opendir");
    exit(-7);
  }

  char match_prefix[20];
  char nomatch_prefix[20];

  snprintf(match_prefix, 20, "VTS_%02zu_", title);
  snprintf(nomatch_prefix, 20, "VTS_%02zu_0", title);

  uint32_t first = 0;
  size_t index = 0;
  while ((dir = readdir(d)) != NULL) {
    if (dir->d_type == DT_REG &&
        (strstr(dir->d_name, match_prefix) == dir->d_name) &&
        (strstr(dir->d_name, nomatch_prefix) != dir->d_name)) {
      struct stat st;
      char filename[FILENAME_MAX];
      snprintf(filename, FILENAME_MAX, "%s/%s", path, dir->d_name);
      stat(filename, &st);

      uint32_t last = first + (st.st_size / DVD_SECTOR_SIZE);
      (*extents)[index++] = (extent_t){first, last - 1};
      first = last;
    }
  }
  closedir(d);

  *extents = realloc(*extents, sizeof(extent_t) * index);
  return index;
}

void split(char *path, size_t title,
           extent_t *pgc_extents, size_t pgc_extent_count,
           extent_t *vob_extents, size_t vob_extent_count) {
  size_t in_index = 0, out_index = 0;
  uint32_t in_sector = 0, out_sector = 0;

  FILE *in = NULL;
  FILE *out = NULL;

  char buffer[DVD_SECTOR_SIZE];

  while (in_index < vob_extent_count && out_index < pgc_extent_count) {
    if (in == NULL) {
      char filename[FILENAME_MAX];
      snprintf(filename, FILENAME_MAX, "%s/VTS_%02zu_%zu.VOB", path, title, in_index + 1);
      printf("opening %s\n", filename);
      in = fopen(filename, "r");
    }

    if (out == NULL) {
      char filename[FILENAME_MAX];
      snprintf(filename, FILENAME_MAX, "out-%02zu-%zu.vob", title, out_index);
      printf("opening %s\n", filename);
      out = fopen(filename, "w");
    }

    if (fread(buffer, DVD_SECTOR_SIZE, 1, in) < 1) {
      perror("fread");
      exit(-8);
    }
    in_sector++;

    if (fwrite(buffer, DVD_SECTOR_SIZE, 1, out) < 1) {
      perror("fwrite");
      exit(-9);
    }
    out_sector++;

    if (out_sector > pgc_extents[out_index].last) {
      out_index++;
      fclose(out);
      out = NULL;
    }

    if (in_sector > vob_extents[in_index].last) {
      in_index++;
      fclose(in);
      in = NULL;
    }
  }

  if (in != NULL) {
    fclose(in);
  }

  if (out != NULL) {
    fclose(out);
  }
}

void usage(int argc, char **argv, FILE *f) {
  fprintf(f, "Usage: %s </path/to/VIDEO_TS> <title-index>\n", argv[0]);
}
