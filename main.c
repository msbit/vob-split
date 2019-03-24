#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include <stdio.h>
#include <stdlib.h>

struct pgc_extent_t {
  uint32_t first_sector;
  uint32_t last_sector;
};

int main(int argc, char **argv) {
  if (argc < 3) {
    return -1;
  }

  char *path = realpath(argv[1], NULL);
  if (path == NULL) {
    perror("realpath");
    return -2;
  }

  dvd_reader_t *dvd = DVDOpen(path);
  if (dvd == NULL) {
    perror("DVDOpen");
    return -3;
  }

  ifo_handle_t *vmg = ifoOpen(dvd, 0);
  if (vmg == NULL) {
    perror("ifoOpen");
    return -4;
  }

  int title = atoi(argv[2]);
  ifo_handle_t *ifo = ifoOpen(dvd, title);
  if (ifo == NULL) {
    perror("ifoOpen");
    return -5;
  }

  struct pgc_extent_t *pgc_extents = malloc(sizeof(struct pgc_extent_t) * ifo->vts_pgcit->nr_of_pgci_srp);

  for (int i = 0; i < ifo->vts_pgcit->nr_of_pgci_srp; i++) {
    pgc_t *pgc = ifo->vts_pgcit->pgci_srp[i].pgc;
    uint32_t first_sector = pgc->cell_playback[0].first_sector;
    uint32_t last_sector = pgc->cell_playback[pgc->nr_of_cells - 1].last_sector;
    pgc_extents[i] = (struct pgc_extent_t){first_sector, last_sector};
  }

  for (int i = 0; i < ifo->vts_pgcit->nr_of_pgci_srp; i++) {
    printf("%d %d %d\n", i, pgc_extents[i].first_sector, pgc_extents[i].last_sector);
  }

  DVDClose(dvd);
}
