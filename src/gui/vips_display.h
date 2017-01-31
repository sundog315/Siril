#ifndef _SIRIL_VIPS_H
#define _SIRIL_VIPS_H

#include "core/siril.h"

void initialize_vips(const char *program_name);
void uninitialize_vips();

void vips_reload();
void vips_remap(int vport, WORD lo, WORD hi);
void vips_remaprgb();

#endif
