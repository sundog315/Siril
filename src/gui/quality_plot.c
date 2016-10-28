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
#include "gui/quality_plot.h"
#include "kplot.h"

static GtkWidget *drawingPlot = NULL;
static pldata *plot_data;
static struct kpair ref;
static gboolean is_fwhm = FALSE, export_to_PNG = FALSE;

static void remove_point(pldata *plot, int i) {
	memmove(&plot->data[i], &plot->data[i + 1], (plot->nb - i - 1) * sizeof(*plot->data));
	plot->nb--;
}

static pldata *alloc_plot_data(int size) {
	pldata *plot = malloc(sizeof(pldata));
	if (!plot) return NULL;
	plot->data = malloc(size * sizeof(struct kpair));
	if (!plot->data) { free(plot); return NULL; }
	plot->nb = size;
	plot->next = NULL;
	return plot;
}

static void build_quality(sequence *seq, int layer, int ref_image, pldata *plot) {
	int i;

	for (i = 0; i < plot->nb; i++) {
		if (!seq->imgparam[i].incl) continue;
		plot->data[i].x = (double) i;
		plot->data[i].y = (is_fwhm == TRUE) ?
						seq->regparam[layer][i].fwhm :
						seq->regparam[layer][i].quality;
	}
	/* removing non selected points */
	for (i = 0; i < plot->nb; i++) {
		if (plot->data[i].x == 0.0 && plot->data[i].y == 0.0) {
			remove_point(plot, i);
			i--;
		}
	}

	ref.y = (is_fwhm == TRUE) ?
			seq->regparam[layer][ref_image].fwhm :
			seq->regparam[layer][ref_image].quality;

}

void free_plot_data() {
	pldata *plot = plot_data;
	while (plot) {
		pldata *next = plot->next;
		if (plot->data)
			free(plot->data);
		free(plot);
		plot = next;
	}
	plot_data = NULL;
}

void drawPlot() {
	int i, ref_image, layer = 0;
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
	} else return;

	ref.x = (double) ref_image;

	/* building data array */
	if (plot_data) {
		free_plot_data();
	}
	plot_data = alloc_plot_data(seq->number);

	ref.x = (double) ref_image;
	build_quality(seq, layer, ref_image, plot_data);

	gtk_widget_queue_draw(drawingPlot);
}

void on_ButtonSavePNG_clicked(GtkButton *button, gpointer user_data) {
	set_cursor_waiting(TRUE);
	export_to_PNG = TRUE;
	drawPlot();
	set_cursor_waiting(FALSE);
}

gboolean on_DrawingPlot_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
	guint width, height, i, j;
	double mean/*, sigma*/;
	int min, max;
	struct kpair *avg;
	struct kplotcfg	 cfgplot;
	struct kdatacfg	 cfgdata;
	struct kdata *d1, *d2, *m;
	struct kplot *p;

	if (plot_data) {

		d1 = d2 = m = NULL;
		p = NULL;

		kplotcfg_defaults(&cfgplot);
		kdatacfg_defaults(&cfgdata);
		cfgplot.xaxislabel = _("Frames");
		cfgplot.yaxislabel = (is_fwhm == TRUE) ? _("FWHM") : _("Quality");
		cfgplot.yaxislabelrot = M_PI_2 * 3.0;
//		cfgplot.y2axislabel = _("Sigma");
		cfgplot.xticlabelpad = cfgplot.yticlabelpad = 10.0;
		cfgdata.point.radius = 10;

		d1 = kdata_array_alloc(plot_data->data, plot_data->nb);
		d2 = kdata_array_alloc(&ref, 1);

		/* mean and sigma */
		mean = kdata_ymean(d1);
		//sigma = kdata_ystddev(d1);
		min = plot_data->data[0].x;
		/* if reference is ploted, we take it as maximum if it is */
		max = (plot_data->data[plot_data->nb-1].x > ref.x) ? plot_data->data[plot_data->nb-1].x + 1: ref.x + 1;

		avg = calloc(max - min, sizeof(struct kpair));

		j = min;
		for (i = 0; i < max - min; i++) {
			avg[i].x = (double) j;
			avg[i].y = mean;
			++j;
		}

		m = kdata_array_alloc(avg, max - min);

		p = kplot_alloc(&cfgplot);

		kplot_attach_data(p, d1, ((plot_data->nb <= 100) ? KPLOT_LINESPOINTS : KPLOT_LINES), NULL);	// data plot
		kplot_attach_data(p, d2, KPLOT_POINTS, &cfgdata);	// ref image dot
		kplot_attach_data(p, m, KPLOT_LINES, NULL);			// mean plot

		width = gtk_widget_get_allocated_width(widget);
		height = gtk_widget_get_allocated_height(widget);

		cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
		cairo_rectangle(cr, 0.0, 0.0, width, height);
		cairo_fill(cr);
		kplot_draw(p, width, height, cr);

		if (export_to_PNG) {
			const gchar *file;
			gchar *filename, *msg;
			GtkEntry *EntryPng;

			EntryPng = GTK_ENTRY(lookup_widget("GtkEntryPng"));
			file = gtk_entry_get_text(EntryPng);
			if (file && file[0] != '\0') {
				msg = siril_log_message("%s.png has been saved.\n", file);
				show_dialog(msg, _("Information"), "gtk-dialog-info");
				filename = g_strndup(file, strlen(file) + 5);
				g_strlcat(filename, ".png", strlen(file) + 5);
				cairo_surface_write_to_png(cairo_get_target(cr), filename);
				g_free(filename);
			}
		}

		free(avg);
		kplot_free(p);
		kdata_destroy(d1);
		kdata_destroy(d2);
		kdata_destroy(m);
	}
	export_to_PNG = FALSE;
	return FALSE;
}

void on_GtkEntryPng_changed(GtkEditable *editable, gpointer user_data) {
	const gchar *txt;

	txt = gtk_entry_get_text(GTK_ENTRY(editable));
	if (txt[0] == '\0') {
		gtk_widget_set_sensitive(lookup_widget("ButtonSavePNG"), FALSE);
	}
	else
		gtk_widget_set_sensitive(lookup_widget("ButtonSavePNG"), TRUE);
}
