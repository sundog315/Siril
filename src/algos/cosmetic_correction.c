/*
 * This file is part of Siril, an astronomy image processor.
 * Copyright (C) 2005-2011 Francois Meyer (dulle at free.fr)
 * Copyright (C) 2012-2015 team free-astro (see more in AUTHORS file)
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

#include "core/siril.h"
#include "core/proto.h"
#include "algos/cosmetic_correction.h"

static WORD getMedian5x5(WORD *buf, const int xx, const int yy, const int w,
		const int h, gboolean is_cfa) {
	int step, radius, x, y;
	int width, height;
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
	median = get_median_value_from_sorted_word_data(value + start, n);
	free(value);
	return median;
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
 */
point *find_deviant_pixels(fits *fit, double k, int *count) {
	int x, y, i;
	WORD *buf = fit->pdata[RLAYER];
	imstats *stat;
	double sigma, median, threshold;
	point *p;

	/** statistics **/
	stat = statistics(fit, RLAYER, NULL, STATS_SIGMA);
	sigma = stat->sigma;
	median = stat->median;
	threshold = (k * sigma) + median;
	free(stat);

	/** First we count hot pixels **/
	*count = 0;
	for (i = 0; i < fit->rx * fit->ry; i++)
		if (buf[i] > threshold) (*count)++;

	/** Second we store hot pixels in p*/
	int n = *count;
	if (n <= 0) return NULL;
	p = calloc(n, sizeof(point));
	i = 0;
	for (y = 0; y < fit->ry; y++) {
		for (x = 0; x < fit->rx; x++) {
			double pixel = (double) buf[x + y * fit->rx];
			if (pixel > threshold) {
				p[i].x = x;
				p[i].y = y;
				i++;
			}
		}
	}
	return p;
}

int cosmeticCorrection(fits *fit, point *p, int size, gboolean is_cfa) {
	int i;
	WORD *buf = fit->pdata[RLAYER];		// Cosmetic correction, as developed here, is only used on 1-channel images
	int width = fit->rx;
	int height = fit->ry;

	for (i = 0; i < size; i++) {
		int xx = (int) p[i].x;
		int yy = (int) p[i].y;

		WORD mean = getMedian5x5(buf, xx, yy, width, height, is_cfa);

		buf[xx + yy * fit->rx] = mean;
	}
	return 0;
}
