/*
 * This file is part of Siril, an astronomy image processor.
 * Copyright (C) 2005-2011 Francois Meyer (dulle at free.fr)
 * Copyright (C) 2012-2016 team free-astro (see more in AUTHORS file)
 * Reference site is https://free-astro.org/index.php/Siril
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
#include "algos/PSF.h"

static GtkWidget *drawingPlot = NULL, *combo = NULL;
static pldata *plot_data;
static struct kpair ref;
static gboolean is_fwhm = FALSE, export_to_PNG = FALSE;
static char *ylabel = NULL;
static enum photmetry_source selected_source = ROUNDNESS;

static void update_ylabel();

static pldata *alloc_plot_data(int size) {
	pldata *plot = malloc(sizeof(pldata));
	if (!plot) return NULL;
	plot->data = malloc(size * sizeof(struct kpair));
	if (!plot->data) { free(plot); return NULL; }
	plot->nb = size;
	plot->next = NULL;
	return plot;
}

static void build_registration_dataset(sequence *seq, int layer, int ref_image, pldata *plot) {
	int i, j;

	for (i = 0, j = 0; i < plot->nb; i++) {
		if (!seq->imgparam[i].incl) continue;
		plot->data[j].x = (double) i;
		plot->data[j].y = is_fwhm ?
						seq->regparam[layer][i].fwhm :
						seq->regparam[layer][i].quality;
		j++;
	}
	plot->nb = j;

	ref.x = (double) ref_image;
	ref.y = is_fwhm ?
			seq->regparam[layer][ref_image].fwhm :
			seq->regparam[layer][ref_image].quality;

}

static void build_photometry_dataset(sequence *seq, int dataset, int size, int ref_image, pldata *plot) {
	int i, j;
	fitted_PSF **psfs = seq->photometry[dataset];

	for (i = 0, j = 0; i < size; i++) {
		if (!seq->imgparam[i].incl) continue;
		if (psfs[i]) {
			plot->data[j].x = (double)i;
			switch (selected_source) {
				case ROUNDNESS:
					plot->data[j].y = psfs[i]->fwhmy / psfs[i]->fwhmx;
					break;
				case FWHM:
					plot->data[j].y = psfs[i]->fwhmx;
					break;
				case AMPLITUDE:
					plot->data[j].y = psfs[i]->A;
					break;
				case MAGNITUDE:
					plot->data[j].y = psfs[i]->mag;
					if (com.magOffset > 0.0)
						plot->data[j].y += com.magOffset;
					break;
				case BACKGROUND:
					plot->data[j].y = psfs[i]->B;
					break;
			}
		}

		/* we'll just take the reference image point from the last data set rendered */
		if (i == ref_image) {
			ref.x = (double) ref_image;
			ref.y = plot->data[j].y;
		}

		j++;
	}
	plot->nb = j;
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
		combo = lookup_widget("plotCombo");
	}

	seq = &com.seq;
	if (plot_data)
		free_plot_data();

	if (seq->reference_image == -1)
		ref_image = 0;
	else ref_image = seq->reference_image;

	/* XXX include a new way of selecting data to plot here */
	if (seq->photometry[0]) {
		pldata *plot;
		gtk_widget_set_visible(combo, TRUE);
		update_ylabel();

		plot = alloc_plot_data(seq->number);
		plot_data = plot;
		for (i = 0; i < MAX_SEQPSF && seq->photometry[i]; i++) {
			if (i > 0) {
				plot->next = alloc_plot_data(seq->number);
				plot = plot->next;
			}
			
			build_photometry_dataset(seq, i, seq->number, ref_image, plot);
		}
	} else {
		// fallback to registration graph display
		gtk_widget_set_visible(combo, FALSE);
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

		if (seq->regparam[layer][ref_image].fwhm > 0.0f) {
			is_fwhm = TRUE;
			ylabel = _("FWHM");
		} else if (seq->regparam[layer][ref_image].quality >= 0.0) {
			is_fwhm = FALSE;
			ylabel = _("Quality");
		} else return;

		/* building data array */
		plot_data = alloc_plot_data(seq->number);

		build_registration_dataset(seq, layer, ref_image, plot_data);
	}

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
	int min, max, nb_graphs = 0;
	struct kpair *avg;
	struct kplotcfg	 cfgplot;
	struct kdatacfg	 cfgdata;
	struct kdata *d1, *ref_d, *mean_d;
	struct kplot *p;

	if (plot_data) {
		pldata *plot = plot_data;
		d1 = ref_d = mean_d = NULL;

		kplotcfg_defaults(&cfgplot);
		kdatacfg_defaults(&cfgdata);
		cfgplot.xaxislabel = _("Frames");
		cfgplot.yaxislabel = ylabel;
		cfgplot.yaxislabelrot = M_PI_2 * 3.0;
		//cfgplot.y2axislabel = _("Sigma");
		cfgplot.xticlabelpad = cfgplot.yticlabelpad = 10.0;
		cfgdata.point.radius = 10;

		p = kplot_alloc(&cfgplot);

		// data plots
		while (plot) {
			d1 = kdata_array_alloc(plot->data, plot->nb);
			kplot_attach_data(p, d1, ((plot_data->nb <= 100) ? KPLOT_LINESPOINTS : KPLOT_LINES), NULL);
			plot = plot->next;
			nb_graphs++;
		}

		/* mean and sigma */
		mean = kdata_ymean(d1);
		//sigma = kdata_ystddev(d1);
		min = plot_data->data[0].x;
		/* if reference is ploted, we take it as maximum if it is */
		max = (plot_data->data[plot_data->nb-1].x > ref.x) ? plot_data->data[plot_data->nb-1].x + 1: ref.x + 1;

		if (nb_graphs == 1) {
			avg = calloc(max - min, sizeof(struct kpair));
			j = min;
			for (i = 0; i < max - min; i++) {
				avg[i].x = (double) j;
				avg[i].y = mean;
				++j;
			}

			mean_d = kdata_array_alloc(avg, max - min);
			kplot_attach_data(p, mean_d, KPLOT_LINES, NULL);	// mean plot
			free(avg);
		}

		ref_d = kdata_array_alloc(&ref, 1);

		kplot_attach_data(p, ref_d, KPLOT_POINTS, &cfgdata);	// ref image dot

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
				msg = siril_log_message(_("%s.png has been saved.\n"), file);
				show_dialog(msg, _("Information"), "gtk-dialog-info");
				filename = g_strndup(file, strlen(file) + 5);
				g_strlcat(filename, ".png", strlen(file) + 5);
				cairo_surface_write_to_png(cairo_get_target(cr), filename);
				g_free(filename);
			}
		}

		kplot_free(p);
		kdata_destroy(d1);
		kdata_destroy(ref_d);
		if (mean_d)
			kdata_destroy(mean_d);
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

void on_plotCombo_changed(GtkComboBox *box, gpointer user_data) {
	drawPlot();
}

static void update_ylabel() {
	selected_source = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
	switch (selected_source) {
		case ROUNDNESS:
			ylabel = _("Star roundness (1 is round)");
			break;
		case FWHM:
			ylabel = _("FWHM");
			break;
		case AMPLITUDE:
			ylabel = _("Amplitude");
			break;
		case MAGNITUDE:
			if (com.magOffset > 0.0)
				ylabel = _("Star magnitude (absolute)");
			else ylabel = _("Star magnitude (relative, use setmag)");
			break;
		case BACKGROUND:
			ylabel = _("Background value");
			break;
	}
}

