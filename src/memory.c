#include "flux/memory.h"
#include <stdlib.h>
#include <string.h>
static FluxMemRegion* find(FluxMemManager* m, const char* n) {
    for (int i = 0; i < m->count; i++) if (!strcmp(m->regions[i].name, n)) return &m->regions[i];
    return NULL;
}
int flux_mem_init(FluxMemManager* m) { memset(m, 0, sizeof(*m)); return 0; }
int flux_mem_create(FluxMemManager* m, const char* n, size_t s, const char* o) {
    if (m->count >= 256 || find(m, n)) return -1;
    if (!s) s = 65536;
    FluxMemRegion* r = &m->regions[m->count++];
    strncpy(r->name, n, 63); strncpy(r->owner, o, 63);
    r->size = r->capacity = s; r->borrower_count = 0;
    r->data = calloc(s, 1);
    if (!r->data) { m->count--; return -2; }
    return 0;
}
int flux_mem_destroy(FluxMemManager* m, const char* n) {
    for (int i = 0; i < m->count; i++) if (!strcmp(m->regions[i].name, n)) {
        free(m->regions[i].data); m->regions[i] = m->regions[--m->count]; return 0;
    }
    return -1;
}
FluxMemRegion* flux_mem_get(FluxMemManager* m, const char* n) { return find(m, n); }
void flux_mem_read(FluxMemRegion* r, size_t o, void* b, size_t l) { if (r && r->data && o + l <= r->size) memcpy(b, r->data + o, l); }
void flux_mem_write(FluxMemRegion* r, size_t o, const void* d, size_t l) { if (r && r->data && o + l <= r->size) memcpy(r->data + o, d, l); }
int32_t flux_mem_read_i32(FluxMemRegion* r, size_t o) { int32_t v = 0; flux_mem_read(r, o, &v, 4); return v; }
void flux_mem_write_i32(FluxMemRegion* r, size_t o, int32_t v) { flux_mem_write(r, o, &v, 4); }
uint8_t flux_mem_read_u8(FluxMemRegion* r, size_t o) { uint8_t v = 0; flux_mem_read(r, o, &v, 1); return v; }
void flux_mem_write_u8(FluxMemRegion* r, size_t o, uint8_t v) { flux_mem_write(r, o, &v, 1); }
int flux_mem_transfer(FluxMemManager* m, const char* s, const char* d, const char* o) {
    FluxMemRegion* sr = find(m, s); if (!sr) return -1;
    if (flux_mem_create(m, d, sr->size, o ? o : sr->owner)) return -2;
    FluxMemRegion* dr = find(m, d); if (!dr) return -3;
    memcpy(dr->data, sr->data, sr->size); return 0;
}
void flux_mem_free(FluxMemManager* m) { for (int i = 0; i < m->count; i++) { free(m->regions[i].data); m->regions[i].data = NULL; } m->count = 0; }
