#ifndef COSMETIC_CORRECTION_H_
#define COSMETIC_CORRECTION_H_

#include "core/siril.h"

point *find_deviant_pixels(fits *fit, double k, int *count);
int cosmeticCorrection(fits *fit, point *p, int size, gboolean is_CFA);
int cosmeticCorrOnePoint(fits *fit, point p, gboolean is_cfa);

#endif /* COSMETIC_CORRECTION_H_ */
