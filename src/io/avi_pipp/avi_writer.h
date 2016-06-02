/*
 * avi_writer.h
 *
 *  Created on: 2 juin 2016
 *      Author: cyril
 */

#ifndef SRC_IO_AVI_WRITER_H_
#define SRC_IO_AVI_WRITER_H_


#include <stdint.h>

#define AVI_WRITER_INPUT_FORMAT_MONOCHROME  0
#define AVI_WRITER_INPUT_FORMAT_COLOUR      1
#define AVI_WRITER_CODEC_DIB       0
#define AVI_WRITER_CODEC_UT_VIDEO  1

#ifdef __cplusplus
extern "C" {
#endif

    // Create a new AVI file
int32_t avi_file_create(
        char *filename,
        int32_t  width,          // Frame width in pixels
        int32_t  height,         // Frame height in pixels
        int32_t  input_format,   // 0: Monochrome LLL, 1: Colour BGRBGRBGR
        int32_t  codec,          // 0: DIB, 1: Ut Video
        int32_t  fps);           // Frames per second value


    // Write frame to video file
int32_t avi_file_write_frame(int32_t file_id, uint8_t *data);

    // Close the AVI file
int32_t avi_file_close(int32_t file_id);


#ifdef __cplusplus
}
#endif

#endif /* SRC_IO_AVI_WRITER_H_ */
