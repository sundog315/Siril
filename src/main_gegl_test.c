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
 * and the link command but replacing siril by main_gegl_test and main.o by main_gegl_test.o
 */


#include <stdio.h>
#include <unistd.h>
#include <gegl.h>
#include <gegl-gtk.h>
#include <gtk/gtk.h>

#include "core/siril.h"
#include "core/proto.h"	// for readfits()

/* the global variables of the whole project (from main.c) */
cominfo com;	// the main data struct
fits gfit;	// currently loaded image
fits wfit[5];	// used for temp files, can probably be replaced by local variables
GtkBuilder *builder;	// get widget references anywhere

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s file.fit\n", *argv);
		exit(1);
	}

	if (readfits(argv[1], &gfit, NULL)) {
		exit(1);
	}

	GeglNode *gegl;
	GeglRectangle rect;
	gegl_init(&argc, &argv);
	gegl_rectangle_set(&rect, 0, 0, gfit.rx, gfit.ry);
	gegl = gegl_node_new();

	/* create the gegl buffer from gfit */
	const Babl* format = babl_format("Y u16");
	GeglBuffer *buf = gegl_buffer_new(&rect, format);
	gegl_buffer_set(buf, &rect, 1, format, &gfit.pdata[0], GEGL_AUTO_ROWSTRIDE);
	/* create the graph that displays this buffer */
	GeglNode *bufnode = gegl_node_new_child(gegl,
			"operation", "gegl:buffer-source", "buffer", buf, NULL);
	//gegl_node_process(bufnode);

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
