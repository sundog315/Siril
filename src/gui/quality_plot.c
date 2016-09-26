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

#include <cairo.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/siril.h"
#include "core/proto.h"
#include "gui/callbacks.h"
#include "kplot.h"

static GtkWidget *drawingPlot = NULL;
struct kpair *quality = NULL;
struct kpair ref;
gboolean is_fwhm = FALSE, export_to_PNG = FALSE;
int nb_point = 0;

static void remove_point(struct kpair *arr, int i, int N) {
	memmove(&arr[i], &arr[i + 1], (N - i - 1) * sizeof(*arr));
}

void free_drawPlot() {
	if (quality) {
		free(quality);
		quality = NULL;
	}
}

void drawPlot() {
	int i, ref_image, layer = 0;
	double *qsort;
	sequence *seq;

	if (drawingPlot == NULL) {
		drawingPlot = lookup_widget("DrawingPlot");
	}

	seq = &com.seq;

	if (!(seq->regparam))
		return;

	for (i = 0; i < seq->nb_layers; i++) {
		if (com.seq.regparam[i]) {
			layer = i;
			break;
		}
	}

	if ((!seq->regparam[layer]))
		return;

	/* loading reference frame */
	if (seq->reference_image == -1)
		ref_image = 0;
	else
		ref_image = seq->reference_image;

	if (seq->regparam[layer][ref_image].fwhm > 0.0f) {
		is_fwhm = TRUE;
	} else if (seq->regparam[layer][ref_image].quality >= 0.0) {
		is_fwhm = FALSE;
	} else
		return;

	nb_point = seq->number;

	/* building quality data array */
	if (quality) {
		free_drawPlot();
	}
	quality = calloc(nb_point, sizeof(struct kpair));
	for (i = 0; i < nb_point; i++) {
		if (!seq->imgparam[i].incl) continue;
		quality[i].x = i;
		quality[i].y = (is_fwhm == TRUE) ?
						seq->regparam[layer][i].fwhm :
						seq->regparam[layer][i].quality;
	}
	/* removing non selected points */
	for (i = 0; i < nb_point; i++) {
		if (quality[i].x == 0.0 && quality[i].y == 0.0) {
			remove_point(quality, i, nb_point);
			nb_point--;
			i--;
		}
	}

	ref.x = ref_image;
	ref.y = (is_fwhm == TRUE) ?
			seq->regparam[layer][ref_image].fwhm :
			seq->regparam[layer][ref_image].quality;

	gtk_widget_queue_draw(drawingPlot);
}

void on_ButtonSavePNG_clicked(GtkButton *button, gpointer user_data) {
	set_cursor_waiting(TRUE);
	export_to_PNG = TRUE;
	drawPlot();
	show_dialog(_("Plot.png has been saved"), _("Information"), "gtk-dialog-info");
	set_cursor_waiting(FALSE);
}

gboolean on_DrawingPlot_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
	guint width, height;
	struct kplotcfg	 cfg;
	struct kdatacfg	 cfgdata;
	struct kdata *d1, *d2;
	struct kplot *p;

	if (quality) {
		if (drawingPlot == NULL) {
			drawingPlot = lookup_widget("DrawingPlot");
		}

		d1 = d2 = NULL;
		p = NULL;

		kplotcfg_defaults(&cfg);
		kdatacfg_defaults(&cfgdata);
		cfg.xaxislabel = _("Frames");
		cfg.yaxislabel = (is_fwhm == TRUE) ? _("FWHM") : _("Quality");
		cfg.yaxislabelrot = M_PI_2 * 3.0;
		cfgdata.point.radius = 10;

		d1 = kdata_array_alloc(quality, nb_point);
		d2 = kdata_array_alloc(&ref, 1);

		p = kplot_alloc(&cfg);

		kplot_attach_data(p, d1, KPLOT_LINES, NULL);	// quality plots
		kplot_attach_data(p, d2, KPLOT_POINTS, &cfgdata);	// ref image dot

		width = gtk_widget_get_allocated_width(drawingPlot);
		height = gtk_widget_get_allocated_height(drawingPlot);

		cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
		cairo_rectangle(cr, 0.0, 0.0, width, height);
		cairo_fill(cr);
		kplot_draw(p, width, height, cr);
		if (export_to_PNG) {
			export_to_PNG = FALSE;
			cairo_surface_write_to_png(cairo_get_target(cr), "plot.png");
		}

		kplot_free(p);
		kdata_destroy(d1);
		kdata_destroy(d2);
	}
	return FALSE;
}
