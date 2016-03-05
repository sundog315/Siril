#ifndef COSMETIC_CORRECTION_H_
#define COSMETIC_CORRECTION_H_

#include "core/siril.h"

point *find_deviant_pixels(fits *fit, double sig[2], long *icold, long *ihot);
int cosmeticCorrection(fits *fit, point *p, int size, gboolean is_CFA);
int cosmeticCorrOneLine(fits *fit, point p, gboolean is_cfa);
int cosmeticCorrOnePoint(fits *fit, point p, gboolean is_cfa);

#endif /* COSMETIC_CORRECTION_H_ */
