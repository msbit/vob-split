typedef struct extent_t extent_t;

size_t populate_pgc_extents(const char *, size_t, extent_t **);
size_t populate_vob_extents(const char *, size_t, extent_t **);
void split(const char *, size_t, const extent_t *, size_t, const extent_t *, size_t);
