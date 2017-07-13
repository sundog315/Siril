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

/* This is the vips operation implementing the log scaling for display of images.
 * Input can be UCHAR, USHORT, FLOAT, DOUBLE; Output is always UCHAR.
 * The input and ouput images should have only one band (channel). The output
 * image is then used with other channels images to create the RGB view.
 *
 * The actual operation is: round_to_BYTE(logf((float) i / 10.f) * pente);
 * 10.f is arbitry: good matching with ds9
 *
 * It has two parameters, the slope (pente) and the constant. Since we will not
 * probably change this constant, it is fixed as an inverted #define
 * PIXEL_VALUE_FACTOR, inverted because it's faster to multiply by 0.1 than
 * divide by 10, but the slope is an argument to the operation.
 *
 * TODO:
 *	evaluate if we can change the operation type to TYPE_UNARY
 */

#define PIXEL_VALUE_FACTOR 0.1f

#include <vips/vips.h>
#include "siril_operations.h"

typedef struct _Siril_log {
	VipsOperation parent_instance;

	VipsImage *in;
	VipsImage *out;

	double slope;

} Siril_log;

typedef struct _Siril_logClass {
	VipsOperationClass parent_class;

	/* No new class members needed for this op.
	*/

} Siril_logClass;

/* TODO: evaluate if we can change the operation type to TYPE_UNARY */
G_DEFINE_TYPE( Siril_log, siril_log, VIPS_TYPE_OPERATION );

	static void
siril_log_init( Siril_log *siril_log )
{
	siril_log->slope = 0.0;
}

/* Adapted from round_to_BYTE in utils.c */
static unsigned char round_to_UCHAR(float x) {
        if (x <= 0.0)
                return (unsigned char) 0;
        if (x > (float)UCHAR_MAX)
                return UCHAR_MAX;
        return (unsigned char) (x + 0.5);
}

#define LOOP( ITYPE ) { \
	ITYPE *p = (ITYPE *) VIPS_REGION_ADDR( ir, r->left, r->top + y ); \
	unsigned char *q = (unsigned char *) VIPS_REGION_ADDR( or, r->left, r->top + y ); \
	\
	for( x = 0; x < line_size; x++ ) { \
		/* q[x] = round_to_UCHAR(logf((float)p[x] * PIXEL_VALUE_FACTOR) * siril_log->slope); */ \
		q[x] = round_to_UCHAR((float)p[x]); \
	} \
}

	static int
siril_log_generate( VipsRegion *or, 
		void *vseq, void *a, void *b, gboolean *stop )
{
	/* The area of the output region we have been asked to make.
	*/
	VipsRect *r = &or->valid;

	/* The sequence value ... the thing returned by vips_start_one().
	*/
	VipsRegion *ir = (VipsRegion *) vseq;

	VipsImage *in = (VipsImage *) a;
	Siril_log *siril_log = (Siril_log *) b;
	int line_size = r->width * siril_log->in->Bands; 

	int x, y;

	/* Request matching part of input region.
	*/
	if( vips_region_prepare( ir, r ) )
		return( -1 );

	for( y = 0; y < r->height; y++ ) {
		switch( vips_image_get_format( in )) {
			case VIPS_FORMAT_UCHAR:
				LOOP( unsigned char ); break;
			case VIPS_FORMAT_USHORT:
				LOOP( unsigned short ); break;
			case VIPS_FORMAT_FLOAT:
				LOOP( float ); break;
			case VIPS_FORMAT_DOUBLE:
				LOOP( double ); break;
			default:
				g_assert_not_reached();
		}
	}

	return( 0 );
}

	static int
siril_log_build( VipsObject *object )
{
	VipsObjectClass *class = VIPS_OBJECT_GET_CLASS( object );
	Siril_log *siril_log = (Siril_log *) object;

	if( VIPS_OBJECT_CLASS( siril_log_parent_class )->build( object ) )
		return( -1 );

	VipsBandFormat fmt = vips_image_get_format( siril_log->in );
	if( vips_check_uncoded( class->nickname, siril_log->in ) ||
			( fmt != VIPS_FORMAT_UCHAR &&
			  fmt != VIPS_FORMAT_USHORT &&
			  fmt != VIPS_FORMAT_FLOAT &&
			  fmt != VIPS_FORMAT_DOUBLE ) ) {
		vips_error( "siril", "unsupported image format in log scaling %d", fmt );
		return( -1 );
	}

	/* creating the output image: always 1 band and UCHAR */
	g_object_set( object, "out", vips_image_new(), NULL ); 
	siril_log->out->Bands = 1;
	siril_log->out->BandFmt = VIPS_FORMAT_UCHAR;

	if( vips_image_pipelinev( siril_log->out,
				VIPS_DEMAND_STYLE_THINSTRIP, siril_log->in, NULL ) )
		return( -1 );

	if( vips_image_generate(siril_log->out, 
				vips_start_one, 
				siril_log_generate, 
				vips_stop_one, 
				siril_log->in, siril_log) )
		return( -1 );

	return( 0 );
}

	static void
siril_log_class_init( Siril_logClass *class )
{
	GObjectClass *gobject_class = G_OBJECT_CLASS( class );
	VipsObjectClass *object_class = VIPS_OBJECT_CLASS( class );

	gobject_class->set_property = vips_object_set_property;
	gobject_class->get_property = vips_object_get_property;

	object_class->nickname = "siril_log";
	object_class->description = "siril display log scaling";
	object_class->build = siril_log_build;

	VIPS_ARG_IMAGE( class, "in", 1, 
			"Input", 
			"Input image",
			VIPS_ARGUMENT_REQUIRED_INPUT,
			G_STRUCT_OFFSET( Siril_log, in ) );

	VIPS_ARG_IMAGE( class, "out", 2, 
			"Output", 
			"Output image",
			VIPS_ARGUMENT_REQUIRED_OUTPUT, 
			G_STRUCT_OFFSET( Siril_log, out ) );

	VIPS_ARG_DOUBLE( class, "slope", 3, 
			"Slope", 
			"Slope associated to the scaling",
			VIPS_ARGUMENT_REQUIRED_INPUT,
			G_STRUCT_OFFSET( Siril_log, slope ),
		        -DBL_MAX, DBL_MAX, 0.0);
}

	int
siril_log_scaling( VipsImage *in, VipsImage **out, ... )
{
	va_list ap;
	int result;

	va_start( ap, out );
	result = vips_call_split( "siril_log", ap, in, out );
	va_end( ap );

	return( result );
}

