#ifndef FLUX_REGISTERS_H
#define FLUX_REGISTERS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define FLUX_GP_COUNT 16
#define FLUX_FP_COUNT 16
#define FLUX_VEC_COUNT 16
#define FLUX_VEC_WIDTH 16
#define FLUX_REG_SP  11
#define FLUX_REG_FP  14
#define FLUX_REG_LR  15
typedef struct {
    int32_t  gp[FLUX_GP_COUNT];
    float    fp[FLUX_FP_COUNT];
    uint8_t  vec[FLUX_VEC_COUNT][FLUX_VEC_WIDTH];
    uint32_t pc, sp;
} FluxRegFile;
void flux_regs_init(FluxRegFile* rf);
int32_t flux_gp_read(const FluxRegFile* rf, uint8_t idx);
void    flux_gp_write(FluxRegFile* rf, uint8_t idx, int32_t val);
float   flux_fp_read(const FluxRegFile* rf, uint8_t idx);
void    flux_fp_write(FluxRegFile* rf, uint8_t idx, float val);
#ifdef __cplusplus
}
#endif
#endif
