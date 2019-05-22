#include <dirent.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define DVD_SECTOR_SIZE 2048

struct extent_t {
  uint32_t first_sector;
  uint32_t last_sector;
};

int populate_pgc_extents(pgcit_t *, struct extent_t **);
int populate_vob_extents(char *, int, struct extent_t **);
void split(char *, int, struct extent_t *, int, struct extent_t *, int);
void usage(int, char **, FILE *);

int main(int argc, char **argv) {
  if (argc < 3) {
    usage(argc, argv, stdout);
    exit(-1);
  }

  char *path = realpath(argv[1], NULL);
  if (path == NULL) {
    perror("realpath");
    exit(-2);
  }

  dvd_reader_t *dvd = DVDOpen(path);
  if (dvd == NULL) {
    perror("DVDOpen");
    exit(-3);
  }

  ifo_handle_t *vmg = ifoOpen(dvd, 0);
  if (vmg == NULL) {
    perror("ifoOpen");
    exit(-4);
  }

  int title = atoi(argv[2]);
  ifo_handle_t *ifo = ifoOpen(dvd, title);
  if (ifo == NULL) {
    perror("ifoOpen");
    exit(-5);
  }

  struct extent_t *pgc_extents;

  int pgc_extent_count = populate_pgc_extents(ifo->vts_pgcit, &pgc_extents);

  struct extent_t *vob_extents;

  int vob_extent_count = populate_vob_extents(path, title, &vob_extents);

  split(path, title, pgc_extents, pgc_extent_count, vob_extents,
        vob_extent_count);

  DVDClose(dvd);
}

int populate_pgc_extents(pgcit_t *vts_pgcit, struct extent_t **pgc_extents) {
  *pgc_extents = malloc(sizeof(struct extent_t) * vts_pgcit->nr_of_pgci_srp);

  for (int i = 0; i < vts_pgcit->nr_of_pgci_srp; i++) {
    pgc_t *pgc = vts_pgcit->pgci_srp[i].pgc;
    uint32_t first_sector = pgc->cell_playback[0].first_sector;
    uint32_t last_sector = pgc->cell_playback[pgc->nr_of_cells - 1].last_sector;
    (*pgc_extents)[i] = (struct extent_t){first_sector, last_sector};
  }

  return vts_pgcit->nr_of_pgci_srp;
}

int populate_vob_extents(char *path, int title, struct extent_t **vob_extents) {
  *vob_extents = malloc(sizeof(struct extent_t) * 10);

  DIR *d;
  struct dirent *dir;
  d = opendir(path);
  if (d == NULL) {
    perror("opendir");
    exit(-6);
  }

  char match_prefix[20];
  char nomatch_prefix[20];

  snprintf(match_prefix, 20, "VTS_%02d_", title);
  snprintf(nomatch_prefix, 20, "VTS_%02d_0", title);

  uint32_t first_sector = 0;
  int index = 0;
  while ((dir = readdir(d)) != NULL) {
    if (dir->d_type == DT_REG &&
        (strstr(dir->d_name, match_prefix) == dir->d_name) &&
        (strstr(dir->d_name, nomatch_prefix) != dir->d_name)) {
      struct stat st;
      char filename[FILENAME_MAX];
      snprintf(filename, FILENAME_MAX, "%s/%s", path, dir->d_name);
      stat(filename, &st);

      uint32_t last_sector = first_sector + (st.st_size / DVD_SECTOR_SIZE);
      (*vob_extents)[index++] =
          (struct extent_t){first_sector, last_sector - 1};
      first_sector = last_sector;
    }
  }
  closedir(d);

  *vob_extents = realloc(*vob_extents, sizeof(struct extent_t) * index);
  return index;
}

void split(char *path, int title, struct extent_t *pgc_extents,
           int pgc_extent_count, struct extent_t *vob_extents,
           int vob_extent_count) {
  int vob_in_index = 0, vob_in_sector = 0;
  int vob_out_index = 0, vob_out_sector = 0;

  FILE *vob_in;
  FILE *vob_out;

  char buffer[DVD_SECTOR_SIZE];

  while (vob_in_index < vob_extent_count && vob_out_index < pgc_extent_count) {
    if (vob_in == NULL) {
      char filename[FILENAME_MAX];
      snprintf(filename, FILENAME_MAX, "%s/VTS_%02d_%d.VOB", path, title,
               vob_in_index + 1);
      printf("opening %s\n", filename);
      vob_in = fopen(filename, "r");
    }

    if (vob_out == NULL) {
      char filename[FILENAME_MAX];
      snprintf(filename, FILENAME_MAX, "out-%d.vob", vob_out_index);
      printf("opening %s\n", filename);
      vob_out = fopen(filename, "w");
    }

    if (fread(buffer, DVD_SECTOR_SIZE, 1, vob_in) < 1) {
      perror("fread");
      exit(-7);
    }
    vob_in_sector++;

    if (fwrite(buffer, DVD_SECTOR_SIZE, 1, vob_out) < 1) {
      perror("fwrite");
      exit(-8);
    }
    vob_out_sector++;

    if (vob_out_sector > pgc_extents[vob_out_index].last_sector) {
      vob_out_index++;
      fclose(vob_out);
      vob_out = NULL;
    }

    if (vob_in_sector > vob_extents[vob_in_index].last_sector) {
      vob_in_index++;
      fclose(vob_in);
      vob_in = NULL;
    }
  }

  if (vob_in != NULL) {
    fclose(vob_in);
  }

  if (vob_out != NULL) {
    fclose(vob_out);
  }
}

void usage(int argc, char **argv, FILE *f) {
  fprintf(f, "Usage: %s </path/to/VIDEO_TS> <title-index>\n", argv[0]);
}
