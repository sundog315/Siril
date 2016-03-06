#ifndef COSMETIC_CORRECTION_H_
#define COSMETIC_CORRECTION_H_

#include "core/siril.h"

typedef struct deviant_struct deviant_pixel;

typedef enum {
	COLD_PIXEL, HOT_PIXEL
} typeOfDeviant;

struct deviant_struct {
	point p;
	typeOfDeviant type;
};

deviant_pixel *find_deviant_pixels(fits *fit, double sig[2], long *icold, long *ihot);
int cosmeticCorrection(fits *fit, deviant_pixel *dev, int size, gboolean is_CFA);
int cosmeticCorrOneLine(fits *fit, deviant_pixel dev, gboolean is_cfa);
int cosmeticCorrOnePoint(fits *fit, deviant_pixel dev, gboolean is_cfa);

#endif /* COSMETIC_CORRECTION_H_ */
