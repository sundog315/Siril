/*
 * This file is part of Siril, an astronomy image processor.
 * Copyright (C) 2005-2011 Francois Meyer (dulle at free.fr)
 * Copyright (C) 2012-2016 team free-astro (see more in AUTHORS file)
 * Reference site is http://free-astro.vinvin.tf/index.php/Siril
 *
 * Siril is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Siril is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Siril. If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>

#include "core/siril.h"
#include "core/proto.h"
#include "io/single_image.h"
#include "algos/cosmetic_correction.h"

static WORD getMedian5x5(WORD *buf, const int xx, const int yy, const int w,
		const int h, gboolean is_cfa) {
	int step, radius, x, y;
	WORD *value, median;

	if (is_cfa) {
		step = 2;
		radius = 4;
	}
	else {
		step = 1;
		radius = 2;
	}

	int n = 0;
	int start;
	value = calloc(24, sizeof(WORD));
	for (y = yy - radius; y <= yy + radius; y += step) {
		for (x = xx - radius; x <= xx + radius; x += step) {
			if (y >= 0 && y < h) {
				if (x >= 0 && x < w) {
					if ((x != xx) || (y != yy)) {
						value[n++] = buf[x + y * w];
					}
				}
			}
		}
	}
	start = 24 - n - 1;
	quicksort_s(value, 24);
	median = round_to_WORD(get_median_value_from_sorted_word_data(value + start, n));
	free(value);
	return median;
}

static WORD *getMedian5x5Line(WORD *buf, const int yy, const int w, const int h,
		gboolean is_cfa) {
	int step, radius, x, xx, y;
	WORD *cpyline;

	if (is_cfa) {
		step = 2;
		radius = 4;
	} else {
		step = 1;
		radius = 2;
	}
	cpyline = calloc(w, sizeof(WORD));
	for (xx = 0; xx < w; ++xx) {
		int n = 0;
		int start;
		WORD *value = calloc(20, sizeof(WORD));
		/* 20 neighbours
		 * XXXXX
		 * XXXXX
		 *   o
		 * XXXXX
		 * XXXXX
		 */
		for (y = yy - radius; y <= yy + radius; y += step) {
			if (y != yy) {	// we skip the line
				for (x = xx - radius; x <= xx + radius; x += step) {
					if (y >= 0 && y < h) {
						if (x >= 0 && x < w) {
							value[n++] = buf[x + y * w];
						}
					}
				}
			}
		}
		start = 20 - n - 1;
		quicksort_s(value, 20);
		cpyline[xx] = round_to_WORD(get_median_value_from_sorted_word_data(value + start, n));
		free(value);
	}
	return cpyline;
}

/*
static WORD getAverage3x3(WORD *buf, const int xx, const int yy, const int w,
		const int h, gboolean is_cfa) {
	int step, radius, x, y;
	double value = 0;

	if (is_cfa)
		step = radius = 2;
	else
		step = radius = 1;

	int n = 0;
	for (y = yy - radius; y <= yy + radius; y += step) {
		for (x = xx - radius; x <= xx + radius; x += step) {
			if (y >= 0 && y < h) {
				if (x >= 0 && x < w) {
					if ((x != xx) || (y != yy)) {
						value += (double) buf[x + y * w];
						n++;
					}
				}
			}
		}
	}
	return round_to_WORD(value / n);
}
*/

/* Gives a list of point p containing deviant pixel coordinates
 * p MUST be freed after the call
 * if cold == -1 or hot == -1, this is a flag to not compute cold or hot
 */
point *find_deviant_pixels(fits *fit, double sig[2], long *icold, long *ihot) {
	int x, y, i;
	WORD *buf = fit->pdata[RLAYER];
	imstats *stat;
	double sigma, median, thresHot, thresCold;
	point *p;

	/** statistics **/
	stat = statistics(fit, RLAYER, NULL, STATS_BASIC);
	sigma = stat->sigma;
	median = stat->median;

	if (sig[0] == -1.0) {	// flag for no cold detection
		thresCold = -1.0;
	}
	else {
		double val = median - (sig[0] * sigma);
		thresCold = (val > 0) ? val : 0.0;
	}
	if (sig[1] == -1.0) {	// flag for no hot detection
		thresHot = USHRT_MAX_DOUBLE + 1;
	}
	else {
		double val = median + (sig[1] * sigma);
		thresHot = (val > USHRT_MAX_DOUBLE) ? USHRT_MAX_DOUBLE : val;
	}

	free(stat);

	/** First we count deviant pixels **/
	*icold = 0;
	*ihot = 0;
	for (i = 0; i < fit->rx * fit->ry; i++) {
		if (buf[i] >= thresHot) (*ihot)++;
		else if (buf[i] <= thresCold) (*icold)++;
	}

	/** Second we store deviant pixels in p*/
	int n = (*icold) + (*ihot);
	if (n <= 0) return NULL;
	p = calloc(n, sizeof(point));
	i = 0;
	for (y = 0; y < fit->ry; y++) {
		for (x = 0; x < fit->rx; x++) {
			double pixel = (double) buf[x + y * fit->rx];
			if (pixel >= thresHot || pixel <= thresCold) {
				p[i].x = x;
				p[i].y = y;
				i++;
			}
		}
	}
	return p;
}

int cosmeticCorrOnePoint(fits *fit, point p, gboolean is_cfa) {
	WORD *buf = fit->pdata[RLAYER];		// Cosmetic correction, as developed here, is only used on 1-channel images
	int width = fit->rx;
	int height = fit->ry;
	int x = (int) p.x;
	int y = (int) p.y;

	WORD mean = getMedian5x5(buf, x, y, width, height, is_cfa);

	buf[x + y * fit->rx] = mean;
	return 0;
}

int cosmeticCorrOneLine(fits *fit, point p, gboolean is_cfa) {
	WORD *buf = fit->pdata[RLAYER];
	WORD *line, *newline;
	int width = fit->rx;
	int height = fit->ry;
	int row = (int) p.y;

	line = buf + row * width;
	newline = getMedian5x5Line(buf, row, width, height, is_cfa);
	memcpy(line, newline, width * sizeof(WORD));

	free(newline);

	return 0;
}

int cosmeticCorrection(fits *fit, point *p, int size, gboolean is_cfa) {
	int i;
	WORD *buf = fit->pdata[RLAYER];		// Cosmetic correction, as developed here, is only used on 1-channel images
	int width = fit->rx;
	int height = fit->ry;

	for (i = 0; i < size; i++) {
		int xx = (int) p[i].x;
		int yy = (int) p[i].y;

		WORD median = getMedian5x5(buf, xx, yy, width, height, is_cfa);

		buf[xx + yy * width] = median;
	}
	return 0;
}
