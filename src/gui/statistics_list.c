/*
 * This file is part of Siril, an astronomy image processor.
 * Copyright (C) 2005-2011 Francois Meyer (dulle at free.fr)
 * Copyright (C) 2012-2015 team free-astro (see more in AUTHORS file)
 * Reference site is http://free-astro.vinvin.tf/index.php/Siril
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


#include "core/siril.h"
#include "core/proto.h"
#include "gui/callbacks.h"

static GtkListStore *list_store = NULL;


enum {
	COLUMN_NAME,		// string
	COLUMN_RVALUE,		// converted to string, not pure double
	COLUMN_GVALUE,		// converted to string, not pure double
	COLUMN_BVALUE,		// converted to string, not pure double
	COLUMN_COLOR,		// string
	N_COLUMNS
};

char *statName[] = {
		"count (px)",
		"mean",
		"avgDev",
		"median",
		"sigma",
		"min",
		"max",
		"normalization"
};

void get_statlist_store() {
	if (list_store == NULL)
		list_store = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststoreStat"));
}
/* Add an statistic to the list. If imstats is NULL, the list is cleared. */
void add_stats_to_list(imstats *stat[], int nblayer, gboolean normalized) {
	static GtkTreeSelection *selection = NULL;
	GtkTreeIter iter;
	char count[20], format[6];
	char rvalue[20], gvalue[20], bvalue[20];
	double normValue[] = { 1.0, 1.0, 1.0 };

	get_statlist_store();
	if (!selection)
		selection = GTK_TREE_SELECTION(gtk_builder_get_object(builder, "treeview-selection9"));
	if (stat == NULL) {
		gtk_list_store_clear(list_store);
		return;		// just clear the list
	}

	gtk_list_store_clear(list_store);
	if (normalized) {
		normValue[RLAYER] = stat[RLAYER]->normValue;
		normValue[GLAYER] = stat[RLAYER]->normValue;
		normValue[BLAYER] = stat[RLAYER]->normValue;
		sprintf(format, "%%.9lf");
	}
	else
		sprintf(format, "%%.1lf");

	sprintf(rvalue, "%u", stat[RLAYER]->count);
	if (nblayer > 1) {
		sprintf(gvalue, "%u", stat[GLAYER]->count);
		sprintf(bvalue, "%u", stat[BLAYER]->count);
	} else {
		sprintf(gvalue, "--");
		sprintf(bvalue, "--");
	}

	gtk_list_store_append(list_store, &iter);
	gtk_list_store_set(list_store, &iter, COLUMN_NAME, statName[0],
			COLUMN_RVALUE, rvalue,
			COLUMN_GVALUE, gvalue,
			COLUMN_BVALUE, bvalue,
			COLUMN_COLOR, "White Smoke",
			-1);

	/** Mean */
	sprintf(rvalue, format, stat[RLAYER]->mean / normValue[RLAYER]);
	if (nblayer > 1) {
		sprintf(gvalue, format, stat[GLAYER]->mean / normValue[GLAYER]);
		sprintf(bvalue, format, stat[BLAYER]->mean / normValue[BLAYER]);
	} else {
		sprintf(gvalue, "--");
		sprintf(bvalue, "--");
	}

	gtk_list_store_append(list_store, &iter);
	gtk_list_store_set(list_store, &iter, COLUMN_NAME, statName[1],
			COLUMN_RVALUE, rvalue,
			COLUMN_GVALUE, bvalue,
			COLUMN_BVALUE, gvalue,
			COLUMN_COLOR, "Powder Blue",
			-1);

	/* AvgDev */
	sprintf(rvalue, format, stat[RLAYER]->avgDev / normValue[RLAYER]);
	if (nblayer > 1) {
		sprintf(gvalue, format, stat[GLAYER]->avgDev / normValue[GLAYER]);
		sprintf(bvalue, format, stat[BLAYER]->avgDev / normValue[BLAYER]);
	} else {
		sprintf(gvalue, "--");
		sprintf(bvalue, "--");
	}

	gtk_list_store_append(list_store, &iter);
	gtk_list_store_set(list_store, &iter, COLUMN_NAME, statName[2],
			COLUMN_RVALUE, rvalue,
			COLUMN_GVALUE, bvalue,
			COLUMN_BVALUE, gvalue,
			COLUMN_COLOR, "White Smoke",
			-1);

	/* median */
	sprintf(rvalue, format, stat[RLAYER]->median / normValue[RLAYER]);
	if (nblayer > 1) {
		sprintf(gvalue, format, stat[GLAYER]->median / normValue[GLAYER]);
		sprintf(bvalue, format, stat[BLAYER]->median / normValue[BLAYER]);
	} else {
		sprintf(gvalue, "--");
		sprintf(bvalue, "--");
	}

	gtk_list_store_append(list_store, &iter);
	gtk_list_store_set(list_store, &iter, COLUMN_NAME, statName[3],
			COLUMN_RVALUE, rvalue,
			COLUMN_GVALUE, bvalue,
			COLUMN_BVALUE, gvalue,
			COLUMN_COLOR, "Powder Blue",
			-1);

	/* sigma */
	sprintf(rvalue, format, stat[RLAYER]->sigma / normValue[RLAYER]);
	if (nblayer > 1) {
		sprintf(gvalue, format, stat[GLAYER]->sigma / normValue[GLAYER]);
		sprintf(bvalue, format, stat[BLAYER]->sigma / normValue[BLAYER]);
	} else {
		sprintf(gvalue, "--");
		sprintf(bvalue, "--");
	}

	gtk_list_store_append(list_store, &iter);
	gtk_list_store_set(list_store, &iter, COLUMN_NAME, statName[4],
			COLUMN_RVALUE, rvalue,
			COLUMN_GVALUE, bvalue,
			COLUMN_BVALUE, gvalue,
			COLUMN_COLOR, "White Smoke",
			-1);

	/* min */
	sprintf(rvalue, format, stat[RLAYER]->min / normValue[RLAYER]);
	if (nblayer > 1) {
		sprintf(gvalue, format, stat[GLAYER]->min / normValue[GLAYER]);
		sprintf(bvalue, format, stat[BLAYER]->min / normValue[BLAYER]);
	} else {
		sprintf(gvalue, "--");
		sprintf(bvalue, "--");
	}

	gtk_list_store_append(list_store, &iter);
	gtk_list_store_set(list_store, &iter, COLUMN_NAME, statName[5],
			COLUMN_RVALUE, rvalue,
			COLUMN_GVALUE, bvalue,
			COLUMN_BVALUE, gvalue,
			COLUMN_COLOR, "Powder Blue",
			-1);

	/* max */
	sprintf(rvalue, format, stat[RLAYER]->max / normValue[RLAYER]);
	if (nblayer > 1) {
		sprintf(gvalue, format, stat[GLAYER]->max / normValue[GLAYER]);
		sprintf(bvalue, format, stat[BLAYER]->max / normValue[BLAYER]);
	} else {
		sprintf(gvalue, "--");
		sprintf(bvalue, "--");
	}

	gtk_list_store_append(list_store, &iter);
	gtk_list_store_set(list_store, &iter, COLUMN_NAME, statName[6],
			COLUMN_RVALUE, rvalue,
			COLUMN_GVALUE, bvalue,
			COLUMN_BVALUE, gvalue,
			COLUMN_COLOR, "White Smoke",
			-1);

}

void on_statButtonClose_clicked(GtkButton *button, gpointer user_data) {
	gtk_widget_hide(lookup_widget("StatWindow"));
}

void on_statCheckButton_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
	GtkToggleButton *checkButton;
	gboolean normalized;
	int channel;
	imstats *stat[3];

	set_cursor_waiting(TRUE);

	checkButton = GTK_TOGGLE_BUTTON(lookup_widget("statCheckButton"));
	normalized = gtk_toggle_button_get_active(checkButton);

	for (channel = 0; channel < gfit.naxes[2]; channel++)
		stat[channel] = statistics(&gfit, channel, &com.selection);
	add_stats_to_list(stat, gfit.naxes[2], normalized);
	gtk_widget_show_all(lookup_widget("StatWindow"));
	for (channel = 0; channel < gfit.naxes[2]; channel++)
		free(stat[channel]);
	set_cursor_waiting(FALSE);
}
