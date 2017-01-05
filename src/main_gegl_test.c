/*
 * This file is part of Siril, an astronomy image processor.
 * Copyright (C) 2005-2011 Francois Meyer (dulle at free.fr)
 * Copyright (C) 2012-2017 team free-astro (see more in AUTHORS file)
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

/* This is a sample for gegl-gtk that loads a FITS file and displays the first
 * channel from the USHORT data.
 * Compile siril then use the following command in src to compile the sample:
 * $ make main_gegl_test.o
 * $ $(CC) -o main_gegl_test main_gegl_test.o ../deps/gegl-gtk/gegl-gtk/.libs/libgegl-gtk3-0.1.a `pkg-config gtk+-3.0 gegl-0.3 cfitsio --libs`
 */


#include <stdio.h>
#include <unistd.h>
#include <gegl.h>
#include <gegl-gtk.h>
#include <gtk/gtk.h>
#include <fitsio.h>

typedef unsigned char BYTE;		// default type for image display data
typedef unsigned short WORD;		// default type for internal image data
#define RLAYER		0
#define GLAYER		1
#define BLAYER		2

/* from core/siril.h */
typedef struct ffit {
	unsigned int rx;	// image width	(naxes[0])
	unsigned int ry;	// image height	(naxes[1])
	int bitpix;
	int naxis;		// number of dimensions of the image
	long naxes[3];		// size of each dimension
	WORD lo;	// MIPS-LO key in FITS file, which is "Lower visualization cutoff"
	WORD hi;	// MIPS-HI key in FITS file, which is "Upper visualization cutoff"
	unsigned short min[3];	// min for all layers
	unsigned short max[3];	// max for all layers
	unsigned short maxi;	// max of the max[3]
	unsigned short mini;	// min of the min[3]
	fitsfile *fptr;		// file descriptor. Only used for file read and write.
	WORD *data;		// 16-bit image data (depending on image type)
	WORD *pdata[3];		// pointers on data, per layer data access (RGB)
} fits;

static int readfits(const char *filename, fits *fit);

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s file.fit\n", *argv);
		exit(1);
	}

	fits fit;
	if (readfits(argv[1], &fit)) {
		exit(1);
	}

	GeglNode *gegl;
	GeglRectangle rect;
	/* Init */
	gtk_init(&argc, &argv);
	gegl_init(&argc, &argv);
	gegl_rectangle_set(&rect, 0, 0, fit.rx, fit.ry);
	gegl = gegl_node_new();

	/* create the gegl buffer from fit */
	const Babl* format = babl_format("Y u16");
	GeglBuffer *buf = gegl_buffer_new(&rect, format);
	gegl_buffer_set(buf, &rect, 0, format, fit.pdata[0], GEGL_AUTO_ROWSTRIDE);
	/* create the graph that displays this buffer */
	GeglNode *bufnode = gegl_node_new_child(gegl,
			"operation", "gegl:buffer-source", "buffer", buf, NULL);
	gegl_node_process(bufnode);

	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "GEGL-GTK basic example");
	GtkWidget *view = GTK_WIDGET(gegl_gtk_view_new_for_node(bufnode));
	gtk_container_add(GTK_CONTAINER(window), view);

	g_signal_connect(window, "destroy",
			G_CALLBACK(gtk_main_quit), NULL);
	gtk_widget_show_all(window);

	/* Run */
	gtk_main();

	/* Cleanup */
	g_object_unref(gegl);
	gegl_exit();

	return 0;
}


static void report_fits_error(int status) {
	if (status) {
		char errmsg[FLEN_ERRMSG];
		while (fits_read_errmsg(errmsg)) {
			fprintf(stderr, "FITS error: %s\n", errmsg);
		}
	}
}

static int readfits(const char *filename, fits *fit) {
	int status;
	long orig[3] = { 1L, 1L, 1L };
	// orig ^ gives the coordinate in each dimension of the first pixel to be read
	int zero = 0;
	int i;
	unsigned int nbdata;
	BYTE *data8;

	fit->naxes[2] = 1;

	status = 0;
	fits_open_diskfile(&(fit->fptr), filename, READONLY, &status);
	if (status) {
		report_fits_error(status);
		return status;
	}

	status = 0;
	fits_get_img_param(fit->fptr, 3, &(fit->bitpix), &(fit->naxis), fit->naxes,
			&status);
	if (status) {
		report_fits_error(status);
		status = 0;
		fits_close_file(fit->fptr, &status);
		return status;
	}

	fit->rx = fit->naxes[0];
	fit->ry = fit->naxes[1];
	nbdata = fit->rx * fit->ry;

	if (fit->naxis == 3 && fit->naxes[2] != 3) {
		status = 0;
		fits_close_file(fit->fptr, &status);
		return -1;
	}
	if (fit->naxis == 2 && fit->naxes[2] == 0) {
		fit->naxes[2] = 1;
	}
	if (fit->bitpix == LONGLONG_IMG) {
		status = 0;
		fits_close_file(fit->fptr, &status);
		return -1;
	}

	/* realloc fit->data to the image size */
	if ((fit->data = malloc(nbdata * fit->naxes[2] * sizeof(WORD))) == NULL) {
		fprintf(stderr, "readfits: error malloc\n");
		status = 0;
		fits_close_file(fit->fptr, &status);
		return -1;
	}

	if (fit->naxis == 3) {
		fit->pdata[RLAYER] = fit->data;
		fit->pdata[GLAYER] = fit->data + nbdata;
		fit->pdata[BLAYER] = fit->data + nbdata * 2;
	} else {
		fit->pdata[RLAYER] = fit->data;
		fit->pdata[GLAYER] = fit->data;
		fit->pdata[BLAYER] = fit->data;
	}

	status = 0;
	switch (fit->bitpix) {
	case SBYTE_IMG:
	case BYTE_IMG:
		data8 = calloc(fit->rx * fit->ry * fit->naxes[2], sizeof(BYTE));
		fits_read_pix(fit->fptr, TBYTE, orig, nbdata * fit->naxes[2], &zero,
				data8, &zero, &status);
		for (i=0; i < fit->rx * fit->ry * fit->naxes[2]; i++)
				fit->data[i] = (WORD)data8[i];
		free(data8);
		break;
	case USHORT_IMG:
		fits_read_pix(fit->fptr, TUSHORT, orig, nbdata * fit->naxes[2], &zero,
				fit->data, &zero, &status);
		break;
	case SHORT_IMG:
		fits_read_pix(fit->fptr, TSHORT, orig, nbdata * fit->naxes[2], &zero,
				fit->data, &zero, &status);
		break;
	default:
		fprintf(stderr, "Unsupported FITS image format.\n");
		status = -1;
	}
	if (status) {
		report_fits_error(status);
	}

	status = 0;
	fits_close_file(fit->fptr, &status);
	fprintf(stdout, "Reading FITS: file %s, %ld layer(s), %ux%u pixels\n", filename, fit->naxes[2], fit->rx, fit->ry);
	return 0;
}

