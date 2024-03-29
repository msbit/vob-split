#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <dvdread/ifo_read.h>

#include "split.h"

#define MAX_VOB_PER_VTS 10

struct extent_t {
  uint32_t first;
  uint32_t last;
};

int populate_pgc_extents(const char *path, size_t title, extent_t **extents) {
  int result = INT_MIN;

  dvd_reader_t *dvd = DVDOpen(path);
  if (dvd == NULL) {
    perror("DVDOpen");
    result = -1;
    goto defer_none;
  }

  ifo_handle_t *ifo = ifoOpen(dvd, 0);
  if (ifo == NULL) {
    perror("ifoOpen");
    result = -2;
    goto defer_dvd;
  }
  ifoClose(ifo);

  ifo = ifoOpen(dvd, title);
  if (ifo == NULL) {
    perror("ifoOpen");
    result = -3;
    goto defer_dvd;
  }

  pgcit_t *pgcit = ifo->vts_pgcit;
  size_t count = pgcit->nr_of_pgci_srp;

  *extents = calloc(count, sizeof(extent_t));
  if (*extents == NULL) {
    perror("calloc");
    result = -4;
    goto defer_ifo;
  }

  for (size_t i = 0; i < count; i++) {
    pgc_t *pgc = pgcit->pgci_srp[i].pgc;
    uint32_t first = pgc->cell_playback[0].first_sector;
    uint32_t last = pgc->cell_playback[pgc->nr_of_cells - 1].last_sector;
    (*extents)[i] = (extent_t){first, last};
  }

  result = count;

defer_ifo:
  ifoClose(ifo);

defer_dvd:
  DVDClose(dvd);

defer_none:
  return result;
}

int populate_vob_extents(const char *path, size_t title, extent_t **extents) {
  int result = INT_MIN;

  *extents = calloc(MAX_VOB_PER_VTS, sizeof(extent_t));
  if (*extents == NULL) {
    perror("calloc");
    result = -1;
    goto defer_none;
  }

  DIR *d;
  struct dirent *dir;
  d = opendir(path);
  if (d == NULL) {
    perror("opendir");
    result = -2;
    goto defer_none;
  }

  char match_prefix[20];
  char nomatch_prefix[20];

  snprintf(match_prefix, 20, "VTS_%02zu_", title);
  snprintf(nomatch_prefix, 20, "VTS_%02zu_0", title);

  uint32_t first = 0;
  size_t index = 0;
  while ((dir = readdir(d)) != NULL) {
    if (dir->d_type != DT_REG) {
      continue;
    }
    if (strstr(dir->d_name, match_prefix) != dir->d_name) {
      continue;
    }
    if (strstr(dir->d_name, nomatch_prefix) == dir->d_name) {
      continue;
    }

    char filename[FILENAME_MAX];
    snprintf(filename, FILENAME_MAX, "%s/%s", path, dir->d_name);

    struct stat st;
    stat(filename, &st);

    uint32_t last = first + (st.st_size / DVD_VIDEO_LB_LEN);
    (*extents)[index++] = (extent_t){first, last - 1};
    first = last;
  }
  closedir(d);

  *extents = realloc(*extents, sizeof(extent_t) * index);
  if (*extents == NULL) {
    perror("realloc");
    result = -3;
    goto defer_none;
  }

  result = index;

defer_none:
  return result;
}

int split(const char *path, size_t title, const extent_t *pgc_extents,
          size_t pgc_extent_count, const extent_t *vob_extents,
          size_t vob_extent_count) {
  int result = INT_MIN;

  size_t in_index = 0, out_index = 0;
  uint32_t in_sector = 0, out_sector = 0;

  FILE *in = NULL;
  FILE *out = NULL;

  char buffer[DVD_VIDEO_LB_LEN];

  while (in_index < vob_extent_count && out_index < pgc_extent_count) {
    if (in == NULL) {
      char filename[FILENAME_MAX];
      snprintf(filename, FILENAME_MAX, "%s/VTS_%02zu_%zu.VOB", path, title,
               in_index + 1);
      fprintf(stdout, "opening %s\n", filename);
      in = fopen(filename, "r");
    }

    if (out == NULL) {
      char filename[FILENAME_MAX];
      snprintf(filename, FILENAME_MAX, "out-%02zu-%zu.vob", title, out_index);
      fprintf(stdout, "opening %s\n", filename);
      out = fopen(filename, "w");
    }

    if (fread(buffer, DVD_VIDEO_LB_LEN, 1, in) < 1) {
      perror("fread");
      result = -1;
      goto defer_files;
    }
    in_sector++;

    if (fwrite(buffer, DVD_VIDEO_LB_LEN, 1, out) < 1) {
      perror("fwrite");
      result = -2;
      goto defer_files;
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

  result = 0;

defer_files:
  if (out != NULL) {
    fclose(out);
  }

  if (in != NULL) {
    fclose(in);
  }

  return result;
}
