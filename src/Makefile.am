## Process this file with automake to produce Makefile.in

bin_PROGRAMS = siril

AM_CPPFLAGS = -DPACKAGE_DATA_DIR='"$(datadir)/${PACKAGE}"' -DPACKAGE_DOC_DIR='"${datarootdir}/doc/${PACKAGE}"' $(GTK_CFLAGS) $(LIBTIFF_CFLAGS) $(LIBPNG_CFLAGS) $(FFTW_CFLAGS) $(CFITSIO_CFLAGS) $(FFMPEG_CFLAGS) $(GSL_CFLAGS) $(FFMS2_CFLAGS) $(LIBCONFIGXX_CFLAGS)

siril_SOURCES = \
	siril.c main.c command.c command.h \
	io/conversion.c io/conversion.h io/ser.c io/ser.h io/films.c io/films.h \
	io/image_formats_libraries.c io/image_formats_internal.c io/image_format_fits.c \
	siril.h core/proto.h \
	callbacks.c callbacks.h \
	registration/registration.c registration/registration.h registration/registration_preview.c \
	registration/matching/match.c registration/matching/atpmatch.c registration/matching/misc.c registration/matching/match.h registration/matching/atpmatch.h registration/matching/misc.h \
	stacking/stacking.c stacking/stacking.h \
	algos/gradient.c algos/gradient.h algos/quality.c algos/quality.h \
	algos/fft.c algos/fft.h \
	algos/colors.c algos/colors.h \
	algos/demosaicing.c algos/demosaicing.h \
	sequence.c seqfile.c sequence_list.c \
	single_image.c single_image.h \
	core/histogram.c core/histogram.h core/undo.c core/undo.h core/utils.c \
	algos/pave.c algos/transform.c algos/io_wave.c algos/reconstr.c algos/Def_Math.h algos/Def_Wavelet.h algos/Def_Mem.h \
	algos/PSF.c algos/PSF.h algos/PSF_list.c algos/PSF_list.h algos/star_finder.c algos/star_finder.h \
	compositing.c compositing.h filters.c filters.h
	
siril_LDADD = $(GTK_LIBS) $(LIBRAW_LIBS) $(LIBTIFF_LIBS) $(LIBPNG_LIBS) $(FFTW_LIBS) $(CFITSIO_LIBS) $(GSL_LIBS) $(FFMS2_LIBS) $(LIBCONFIGXX_LIBS)

if HAVE_OPENCV
siril_SOURCES += opencv.cpp opencv.h 
siril_LDADD += $(OPENCV_LIBS)
siril_LINK = $(CXXLINK)
else
siril_LINK = $(LINK)
endif
