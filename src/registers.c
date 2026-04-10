#include "flux/registers.h"
#include <string.h>
void flux_regs_init(FluxRegFile* rf) { memset(rf, 0, sizeof(*rf)); }
int32_t flux_gp_read(const FluxRegFile* rf, uint8_t i) { return i < 16 ? rf->gp[i] : 0; }
void flux_gp_write(FluxRegFile* rf, uint8_t i, int32_t v) { if (i < 16) { rf->gp[i] = v; if (i == 11) rf->sp = (uint32_t)v; } }
float flux_fp_read(const FluxRegFile* rf, uint8_t i) { return i < 16 ? rf->fp[i] : 0.0f; }
void flux_fp_write(FluxRegFile* rf, uint8_t i, float v) { if (i < 16) rf->fp[i] = v; }
