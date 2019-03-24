#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

struct extent_t {
  uint32_t first_sector;
  uint32_t last_sector;
};

#define DVD_SECTOR_SIZE 2048

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
  while((dir = readdir(d)) != NULL) {
    if (dir->d_type == DT_REG
      && (strstr(dir->d_name, match_prefix) == dir->d_name)
      && (strstr(dir->d_name, nomatch_prefix) != dir->d_name)) {
      struct stat st;
      char filename[1024];
      snprintf(filename, 1024, "%s/%s", path, dir->d_name);
      stat(filename, &st);

      uint32_t last_sector = first_sector + (st.st_size / DVD_SECTOR_SIZE);
      (*vob_extents)[index++] = (struct extent_t){first_sector, last_sector - 1};
      first_sector = last_sector;
    }
  }
  closedir(d);
  return index;
}

int main(int argc, char **argv) {
  if (argc < 3) {
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

  for (int i = 0; i < pgc_extent_count; i++) {
    printf("%d %u %u\n", i, pgc_extents[i].first_sector, pgc_extents[i].last_sector);
  }

  for (int i = 0; i < vob_extent_count; i++) {
    printf("%d %u %u\n", i, vob_extents[i].first_sector, vob_extents[i].last_sector);
  }

  DVDClose(dvd);
}
