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

#include <string.h>
#include <vips/vips.h>
#include <gtk/gtk.h>

#include "vips_display.h"
#include "gui/callbacks.h"
#include "gui/vips_operations/siril_operations.h"
#include "core/proto.h"

/* This file contains all code interacting with vips, the fast rendering
 * library. It consequently contains the code responsible for drawing images in
 * siril windows, the GTK+ `draw' callbacks for the drawing areas, the image
 * mapping functions for display purposes.
 */

static VipsImage *images[MAXGRAYVPORT];		// raw monochrome images
static VipsImage *mapped_images[MAXGRAYVPORT];	// colour-mapped monochrome images
static VipsImage *display_images[MAXVPORT];	// passing data from remap to display
                                               

static GtkWidget *drawing_area[MAXVPORT];
static gulong draw_callbacks[MAXVPORT];		// IDs for the draw callbacks

static void render_notify( VipsImage *image, VipsRect *rect, void *client );
static void vipsdisp_draw( GtkWidget *drawing_area, cairo_t *cr, VipsRegion *region );
static void commit_rendering( int vport );
static void release_data();


void initialize_vips(const char *program_name) {
	if( VIPS_INIT( program_name ) )
		vips_error_exit( "unable to start VIPS" );

	drawing_area[RED_VPORT] = lookup_widget("drawingarear");
	drawing_area[GREEN_VPORT] = lookup_widget("drawingareag");
	drawing_area[BLUE_VPORT] = lookup_widget("drawingareab");
	drawing_area[RGB_VPORT] = lookup_widget("drawingareargb");
	memset(images, 0, sizeof(VipsImage *) * MAXGRAYVPORT);
	memset(mapped_images, 0, sizeof(VipsImage *) * MAXGRAYVPORT);
	memset(display_images, 0, sizeof(VipsImage *) * MAXVPORT);
}

/* to be called when gfit has changed and should be reloaded */
void vips_reload() {
	int i;
	fprintf(stdout, "vips reload\n");
	release_data();

	/* create new images from gfit and map them to mapped_images */
	for (i = 0; i < gfit.naxes[2]; i++) {
		images[i] = vips_image_new_from_memory(gfit.pdata[i], 0,
				gfit.rx, gfit.ry, 1, VIPS_FORMAT_USHORT);
		if (!images[i]) {
			g_object_unref(images[i]);
			fprintf(stderr, "error creating vips image %d\n", i);
			return;
		}

		VipsImage *x;
		if (vips_flip(images[i], &x, VIPS_DIRECTION_VERTICAL, NULL)) {
			g_object_unref(images[i]);
			fprintf(stderr, "error flipping vips image %d\n", i);
			return;
		}
		g_object_unref(images[i]);
		images[i] = x;

		remap(i);
	}
	vips_remaprgb();
}

/* to be called when gfit data or display parameters have changed and display
 * should be refreshed with the new configuration.
 * If gfit.data has moved or changed size, use vips_reload() instead. */
void vips_remap(int vport, WORD lo, WORD hi, display_mode mode) {
	int retval;
	double scale, offset;
	VipsImage *tmprgb[3];
	scale = UCHAR_MAX_DOUBLE / (double) (hi - lo);

	//fprintf(stdout, "vips remap %d, lo: %hu, hi: %hu\n", vport, lo, hi);

	if (mapped_images[vport]) {
		g_object_unref(mapped_images[vport]);
		mapped_images[vport] = NULL;
	}

	switch (mode) {
		case LINEAR_DISPLAY:
			if (lo > hi) {
				// negative display, scale is already negative
				offset = UCHAR_MAX_DOUBLE - (double)hi*scale;
			}
			else offset = -(double)lo*scale;

			/* only linear mapping for now */
			retval = vips_linear1( images[vport], &tmprgb[0], scale, offset, "uchar", TRUE, NULL );
			if (retval) return;
			break;

		case LOG_DISPLAY:
			retval = siril_log_scaling( images[vport], &tmprgb[0], scale, NULL );
			//retval = vips_call( "siril_log", images[vport], &tmprgb[0], scale, NULL );
			if (retval) return;
			break;

		default:
			fprintf(stderr, "OPERATION NOT YET SUPPORTED\n");
			return;
	}

	mapped_images[vport] = tmprgb[0];

	/* one channel to grey RGB */
	retval = vips_copy(tmprgb[0], &tmprgb[1], NULL);
	if (retval) return;
	retval = vips_copy(tmprgb[0], &tmprgb[2], NULL);
	if (retval) { g_object_unref(tmprgb[1]); return; }

	retval = vips_bandjoin(tmprgb, &display_images[vport], 3, NULL);
	//g_object_unref(tmprgb[0]);
	g_object_unref(tmprgb[1]);
	g_object_unref(tmprgb[2]);
	if (retval) return;
	display_images[vport]->Type = VIPS_INTERPRETATION_sRGB;

	/* recreate the draw callbacks for the viewport */
	commit_rendering(vport);
}

/* from the three mapped_images, display the RGB image */
void vips_remaprgb() {
	if (!isrgb(&gfit) || !mapped_images[RED_VPORT] ||
			!mapped_images[GREEN_VPORT] || !mapped_images[BLUE_VPORT])
		return;

	vips_bandjoin(mapped_images, &display_images[RGB_VPORT], 3, NULL);
	commit_rendering(RGB_VPORT);
}

/* recreate the draw callbacks with the new region */
void commit_rendering(int vport) {
	VipsImage *x;
	int retval;

	/* manage zoom */
	double zoom = get_zoom_val();
	if (zoom < 0.0) {
		// zoom to fit, not yet implemented
		// use vips_resize()
	}
	else if (zoom < 1.0) {
		// zoom out
		int factor = round_to_int(1.0 / zoom);
		retval = vips_subsample( display_images[vport], &x, factor, factor, NULL );
		g_object_unref(display_images[vport]);
		if (retval) return;
		display_images[vport] = x;
	}
	else if (zoom > 1.0) {
		// zoom in
		int factor = round_to_int(zoom);
		retval = vips_zoom( display_images[vport], &x, factor, factor, NULL );
		g_object_unref(display_images[vport]);
		if (retval) return; 
		display_images[vport] = x;
	}

	if (draw_callbacks[vport])
		g_signal_handler_disconnect(drawing_area[vport], draw_callbacks[vport]);
	/* start processing the display */
	x = vips_image_new();
	retval = vips_sink_screen( display_images[vport], x, NULL, 128, 128, 400, 0, 
				render_notify, drawing_area[vport] );
	g_object_unref( display_images[vport] );
	if (retval) { g_object_unref( x ); display_images[vport] = NULL; return; }
	display_images[vport] = x;

	VipsRegion *region = vips_region_new(display_images[vport]);
	draw_callbacks[vport] = g_signal_connect(drawing_area[vport], "draw", 
			G_CALLBACK( vipsdisp_draw ), region);
	//fprintf(stdout, "vips rendering updated\n");
}

typedef struct _Update {
	GtkWidget *drawing_area;
	VipsRect rect;
} Update;

/* The main GUI thread runs this when it's idle and there are tiles that need
 * painting. */
	static gboolean
render_cb( Update *update )
{
	gtk_widget_queue_draw_area( update->drawing_area,
			update->rect.left, update->rect.top,
			update->rect.width, update->rect.height );

	g_free( update );

	return FALSE;
}

/* Come here from the vips_sink_screen() background thread when a tile has been
 * calculated. We can't paint the screen directly since the main GUI thread
 * might be doing something. Instead, we add an idle callback which will be run
 * by the main GUI thread when it next hits the mainloop.
 */
	static void
render_notify( VipsImage *image, VipsRect *rect, void *client )
{
	GtkWidget *drawing_area = GTK_WIDGET( client );
	Update *update = g_new( Update, 1 );

	update->rect = *rect;
	update->drawing_area = drawing_area;

	g_idle_add( (GSourceFunc) render_cb, update );
}

	static void
vipsdisp_draw_rect( GtkWidget *drawing_area, 
		cairo_t *cr, VipsRegion *region, VipsRect *expose )
{
	VipsRect image;
	VipsRect clip;
	unsigned char *cairo_buffer;
	int x, y;
	cairo_surface_t *surface;

	/*printf( "vipsdisp_draw_rect: left = %d, top = %d, width = %d, height = %d\n",
			expose->left, expose->top,
			expose->width, expose->height );
			*/

	/* Clip against the image size ... we don't want to try painting 
	 * outside the image area. */
	image.left = 0;
	image.top = 0;
	image.width = region->im->Xsize;
	image.height = region->im->Ysize;
	vips_rect_intersectrect( &image, expose, &clip );
	if( vips_rect_isempty( &clip ) ||
			vips_region_prepare( region, &clip ) )
		return;

	/* libvips is RGB, cairo is ARGB, we have to repack the data. */
	cairo_buffer = g_malloc( clip.width * clip.height * 4 );

	for( y = 0; y < clip.height; y++ ) {
		VipsPel *p = 
			VIPS_REGION_ADDR( region, clip.left, clip.top + y );
		unsigned char *q = cairo_buffer + clip.width * 4 * y;

		for( x = 0; x < clip.width; x++ ) {
			q[0] = p[2];
			q[1] = p[1];
			q[2] = p[0];
			q[3] = 0;

			p += 3;
			q += 4;
		}
	}

	surface = cairo_image_surface_create_for_data( cairo_buffer, 
			CAIRO_FORMAT_RGB24, clip.width, clip.height, clip.width * 4 );

	cairo_set_source_surface( cr, surface, clip.left, clip.top );

	cairo_paint( cr );

	g_free( cairo_buffer ); 

	cairo_surface_destroy( surface ); 
}

	static void
vipsdisp_draw( GtkWidget *drawing_area, cairo_t *cr, VipsRegion *region )
{
	cairo_rectangle_list_t *rectangle_list = 
		cairo_copy_clip_rectangle_list( cr );

	//printf( "vipsdisp_draw\n" ); 

	if( rectangle_list->status == CAIRO_STATUS_SUCCESS ) { 
		int i;

		for( i = 0; i < rectangle_list->num_rectangles; i++ ) {
			VipsRect expose;

			expose.left = rectangle_list->rectangles[i].x;
			expose.top = rectangle_list->rectangles[i].y;
			expose.width = rectangle_list->rectangles[i].width;
			expose.height = rectangle_list->rectangles[i].height;

			vipsdisp_draw_rect( drawing_area, cr, region, &expose );
		}
	}

	cairo_rectangle_list_destroy( rectangle_list );

	// paint_overlays(drawing_area, cr);
}

static void release_data() {
	/* release previous data */
	int vport;
	for (vport = 0; vport < MAXGRAYVPORT; vport++) {
		if (images[vport])
			g_object_unref(images[vport]);
		if (mapped_images[vport])
			g_object_unref(mapped_images[vport]);
		if (display_images[vport])
			g_object_unref(display_images[vport]);
	}
	if (display_images[RGB_VPORT])
		g_object_unref(display_images[RGB_VPORT]);

	memset(images, 0, sizeof(VipsImage *) * MAXGRAYVPORT);
	memset(mapped_images, 0, sizeof(VipsImage *) * MAXGRAYVPORT);
	memset(display_images, 0, sizeof(VipsImage *) * MAXVPORT);
}

void uninitialize_vips() {
	// release all vips-associated data if needed
	release_data();
}

