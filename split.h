#include <dvdread/ifo_read.h>

struct extent_t; 

size_t populate_pgc_extents(pgcit_t *, struct extent_t **);
size_t populate_vob_extents(char *, size_t, struct extent_t **);
void split(char *, size_t, struct extent_t *, size_t, struct extent_t *, size_t);
