#include <assert.h>
#include <string.h>
#include <glib.h>

#include "siril.h"
#include "processing.h"
#include "proto.h"
#include "gui/callbacks.h"

// called in start_in_new_thread only
// set_cursor_waiting(FALSE); is not included, but the TRUE is
gpointer generic_sequence_worker(gpointer p) {
	struct generic_seq_args *args = (struct generic_seq_args *)p;
	struct timeval t_end;
	char filename[256], msg[256];
	float nb_framesf, progress = 0.f; // 0 to nb_framesf, for progress
	int frame;	// the current frame, sequence index
	int current;	// number of processed frames so far

	assert(args);
	assert(args->seq);
	assert(args->image_hook);
	set_progress_bar_data(NULL, PROGRESS_RESET);
	if (args->nb_filtered_images > 0)
		nb_framesf = (float)args->nb_filtered_images;
	else 	nb_framesf = (float)args->seq->number;
	args->retval = 0;

	if (args->prepare_hook && args->prepare_hook(args)) {
		siril_log_message("Preparing sequence processing failed.\n");
		args->retval = 1;
		goto the_end;
	}

	current = 0;
	// to enable soon after having removed the breaks
//#pragma omp parallel for num_threads(com.max_thread) private(frame) schedule(static) if(args->parallel && fits_is_reentrant())
	for (frame = 0; frame < args->seq->number; frame++) {
		if (!args->retval) {
			fits fit;
			memset(&fit, 0, sizeof(fits));

			if (!get_thread_run()) {
				args->retval = 1;
				clearfits(&fit);
				continue;
			}
			if (args->filtering_criterion
					&& !args->filtering_criterion(args->seq, frame,
							args->filtering_parameter))
				continue;
			if (!seq_get_image_filename(args->seq, frame, filename)) {
				args->retval = 1;
				clearfits(&fit);
				continue;
			}

			snprintf(msg, 256, "Processing image %d (%s)", frame, filename);
			progress =
					(float) (args->nb_filtered_images <= 0 ? frame : current);
			set_progress_bar_data(msg, progress / nb_framesf);

			if (seq_read_frame(args->seq, frame, &fit)) {
				args->retval = 1;
				clearfits(&fit);
				continue;
			}

			if (args->image_hook(args, frame, current, &fit)) {
				args->retval = 1;
				clearfits(&fit);
				continue;
			}

#pragma omp atomic
			current++;
			clearfits(&fit);
		}
	}


the_end:
	if (args->retval) {
		set_progress_bar_data("Sequence processing failed. Check the log.", PROGRESS_RESET);
		siril_log_message("Sequence processing failed.\n");
	}
	else {
		set_progress_bar_data("Sequence processing succeeded.", PROGRESS_RESET);
		siril_log_message("Sequence processing succeeded.\n");
		gettimeofday(&t_end, NULL);
			show_time(args->t_start, t_end);
	}

	if (args->finalize_hook && args->finalize_hook(args)) {
		siril_log_message("Preparing sequence processing failed.\n");
		args->retval = 1;
	}

	if (args->idle_function)
		gdk_threads_add_idle(args->idle_function, args);
	else gdk_threads_add_idle(end_generic, args);
	return NULL;
}



/*****************************************************************************
 *      P R O C E S S I N G      T H R E A D      M A N A G E M E N T        *
 ****************************************************************************/

// This function is reentrant
void start_in_new_thread(gpointer (*f)(gpointer p), gpointer p) {
	g_mutex_lock(&com.mutex);
	if (com.run_thread || com.thread != NULL) {
		fprintf(stderr, "The processing thread is busy, stop it first.\n");
		g_mutex_unlock(&com.mutex);
		return;
	}

	com.run_thread = TRUE;
	g_mutex_unlock(&com.mutex);
	com.thread = g_thread_new("processing", f, p);
}

void stop_processing_thread() {
	if (com.thread == NULL) {
		fprintf(stderr,
				"The processing thread is not running, cannot stop it.\n");
		return;
	}

	set_thread_run(FALSE);

	g_thread_join(com.thread);
	com.thread = NULL;
}

void set_thread_run(gboolean b) {
	g_mutex_lock(&com.mutex);
	com.run_thread = b;
	g_mutex_unlock(&com.mutex);
}

gboolean get_thread_run() {
	gboolean retval;
	g_mutex_lock(&com.mutex);
	retval = com.run_thread;
	g_mutex_unlock(&com.mutex);
	return retval;
}

/* should be called in a threaded function if nothing special has to be done at the end.
 * gdk_threads_add_idle(end_generic, NULL);
 */
gboolean end_generic(gpointer arg) {
	stop_processing_thread();
	update_used_memory();
	set_cursor_waiting(FALSE);
	return FALSE;
}

void on_processes_button_cancel_clicked(GtkButton *button, gpointer user_data) {
	if (com.thread != NULL)
		siril_log_color_message("Process aborted by user\n", "red");
	stop_processing_thread();
}

int seq_filter_all(sequence *seq, int nb_img, double any) {
	return 1;
}

int seq_filter_included(sequence *seq, int nb_img, double any) {
	return (seq->imgparam[nb_img].incl);
}


