#ifndef _PROCESSING_H_
#define _PROCESSING_H_

/* the dynamic image selection, based on various possible criteria */
typedef int (*seq_image_filter)(sequence *seq, int nb_img, double param);

struct generic_seq_args {
	sequence *seq;
	// filtering the images from the sequence, maybe we don't want them all
	seq_image_filter filtering_criterion;
	double filtering_parameter;
	// if already known, the number of images after filtering, for smoother
	// progress report. < 1 is unknown
	int nb_filtered_images;

	// function called before iterating through the sequence
	int (*prepare_hook)(struct generic_seq_args *);
	// function called for each image with image index in sequence, number
	// of image currently processed and the image.
	int (*image_hook)(struct generic_seq_args *, int, int, fits *);
	// function called after iterating through the sequence
	int (*finalize_hook)(struct generic_seq_args *);

	// idle function to register at the end. If NULL, the default ending
	// that stops the thread is queued
	GSourceFunc idle_function;
	// retval, useful for the idle_function, set by the worker
	int retval;

	// string description for progress and logs
	const char *description;

	// user data: pointer to operation-specific data
	void *user;

	// show time at the end of process
	struct timeval t_start;

	// activate parallel execution
	gboolean parallel;
};

gpointer generic_sequence_worker(gpointer p);

int seq_filter_all(sequence *seq, int nb_img, double any);
int seq_filter_included(sequence *seq, int nb_img, double any);

void start_in_new_thread(gpointer(*f)(gpointer p), gpointer p);
void stop_processing_thread();
void set_thread_run(gboolean b);
gboolean get_thread_run();
gboolean end_generic(gpointer arg);

#endif
