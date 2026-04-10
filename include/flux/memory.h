#ifndef FLUX_MEMORY_H
#define FLUX_MEMORY_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#define FLUX_MAX_REGIONS 256
#define FLUX_DEFAULT_MEM_SIZE 65536
typedef struct { char name[64]; uint8_t* data; size_t size, capacity; char owner[64]; int borrower_count; } FluxMemRegion;
typedef struct { FluxMemRegion regions[FLUX_MAX_REGIONS]; int count; } FluxMemManager;
int flux_mem_init(FluxMemManager* mm);
int flux_mem_create(FluxMemManager* mm, const char* name, size_t size, const char* owner);
int flux_mem_destroy(FluxMemManager* mm, const char* name);
FluxMemRegion* flux_mem_get(FluxMemManager* mm, const char* name);
void flux_mem_read(FluxMemRegion* r, size_t off, void* buf, size_t len);
void flux_mem_write(FluxMemRegion* r, size_t off, const void* data, size_t len);
int32_t flux_mem_read_i32(FluxMemRegion* r, size_t off);
void    flux_mem_write_i32(FluxMemRegion* r, size_t off, int32_t val);
uint8_t flux_mem_read_u8(FluxMemRegion* r, size_t off);
void    flux_mem_write_u8(FluxMemRegion* r, size_t off, uint8_t val);
int flux_mem_transfer(FluxMemManager* mm, const char* src, const char* dst, const char* owner);
void flux_mem_free(FluxMemManager* mm);
#ifdef __cplusplus
}
#endif
#endif
