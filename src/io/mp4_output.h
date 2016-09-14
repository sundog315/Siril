#ifndef _MP4_OUTPUT_H
#define _MP4_OUTPUT_H

#include "core/siril.h"

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>

struct mp4_struct {
	AVOutputFormat *fmt;
	AVFormatContext *oc;
	AVStream *st;
	AVCodecContext *enc;

	/* pts of the next frame that will be generated */
	int64_t next_pts;
	int samples_count;

	AVFrame *frame;
	AVFrame *tmp_frame;

	struct SwsContext *sws_ctx;
	struct SwrContext *swr_ctx;

};

struct mp4_struct *mp4_create(const char *filename, int w, int h, int fps);
int mp4_add_frame(struct mp4_struct *, fits *);
int mp4_close(struct mp4_struct *);

#endif
