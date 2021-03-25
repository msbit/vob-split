typedef struct extent_t extent_t;

int populate_pgc_extents(const char *, size_t, extent_t **);
int populate_vob_extents(const char *, size_t, extent_t **);
int split(const char *, size_t, const extent_t *, size_t, const extent_t *, size_t);
