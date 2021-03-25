struct extent_t; 

size_t populate_pgc_extents(const char *, size_t, struct extent_t **);
size_t populate_vob_extents(const char *, size_t, struct extent_t **);
void split(const char *, size_t, const struct extent_t *, size_t, const struct extent_t *, size_t);
