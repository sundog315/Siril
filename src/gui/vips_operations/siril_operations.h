#ifndef _SIRIL_VIPS_LOG
#define _SIRIL_VIPS_LOG

#include <vips/vips.h>

//extern GType siril_log_get_type( void );

int siril_log_scaling( VipsImage *in, VipsImage **out, ... );

#endif
