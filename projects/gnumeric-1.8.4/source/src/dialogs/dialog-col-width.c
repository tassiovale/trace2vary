/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * dialog-col-width.c:  Sets the magnification factor
 *
 * Author:
 *        Andreas J. Guelzow <aguelzow@taliesin.ca>
 *
 * (c) Copyright 2002 Andreas J. Guelzow <aguelzow@taliesin.ca>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <gnumeric-config.h>
#include <glib/gi18n-lib.h>
#include <gnumeric.h>
#include "dialogs.h"
#include "help.h"

#include <gui-util.h>
#include <commands.h>
#include <workbook-control.h>
#include <workbook.h>
#include <wbc-gtk.h>
#include <sheet.h>
#include <sheet-view.h>
#include <application.h>
#include <workbook-cmd-format.h>

#include <glade/glade.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtklabel.h>

#define COL_WIDTH_DIALOG_KEY "col-width-dialog"

typedef struct {
	GladeXML           *gui;
	WBCGtk *wbcg;
	Sheet              *sheet;
	SheetView	   *sv;
	GtkWidget          *dialog;
	GtkWidget          *ok_button;
	GtkWidget          *apply_button;
	GtkWidget          *cancel_button;
	GtkWidget          *default_check;
	GtkWidget          *description;
	GtkSpinButton      *spin;

	gboolean           set_default_value;

	gnm_float         orig_value;
	gboolean           orig_is_default;
	gboolean           orig_some_default;
	gboolean           orig_all_equal;
	gboolean           adjusting;
} ColWidthState;

static void
dialog_col_width_button_sensitivity (ColWidthState *state)
{
	gnm_float value = gtk_spin_button_get_value (state->spin);
	gboolean use_default = gtk_toggle_button_get_active
		(GTK_TOGGLE_BUTTON (state->default_check));
	gboolean changed_info;

	if (state->set_default_value) {
		changed_info = (state->orig_value != value);
	} else {
		changed_info = (((!state->orig_all_equal || (state->orig_value != value)
				  || state->orig_some_default) && !use_default)
				|| (use_default && !state->orig_is_default));

	}
	gtk_widget_set_sensitive (state->ok_button, changed_info);
	gtk_widget_set_sensitive (state->apply_button, changed_info);
}

static void
cb_dialog_col_width_destroy (ColWidthState *state)
{
	if (state->gui != NULL)
		g_object_unref (G_OBJECT (state->gui));
	g_free (state);
}

static void
cb_dialog_col_width_cancel_clicked (G_GNUC_UNUSED GtkWidget *button,
				    ColWidthState *state)
{
	gtk_widget_destroy (state->dialog);
	return;
}


static void
dialog_col_width_set_value (gnm_float value, ColWidthState *state)
{
	gtk_spin_button_set_value (state->spin, value);
}

static void
dialog_col_width_load_value (ColWidthState *state)
{
	GSList *l;
	gnm_float value = 0.0;
	state->orig_is_default = TRUE;
	state->orig_some_default = FALSE;
	state->orig_all_equal = TRUE;

	state->adjusting = TRUE;
	if (state->set_default_value) {
		value = sheet_col_get_default_size_pts (state->sheet);
	} else {
		for (l = state->sv->selections; l; l = l->next){
			GnmRange *ss = l->data;
			int col;

			for (col = ss->start.col; col <= ss->end.col; col++){
				ColRowInfo const *ri = sheet_col_get_info (state->sheet, col);
				if (ri->hard_size)
					state->orig_is_default = FALSE;
				else
					state->orig_some_default = TRUE;
				if (value == 0.0)
					value = ri->size_pts;
				else if (value != ri->size_pts){
					/* Values differ, so let the user enter the data */
					state->orig_all_equal = FALSE;
				}
			}
		}
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (state->default_check),
					      state->orig_is_default);
	}
	state->orig_value = value;
	dialog_col_width_set_value (value, state);
	dialog_col_width_button_sensitivity (state);
	state->adjusting = FALSE;
}

static void
cb_dialog_col_width_value_changed (G_GNUC_UNUSED GtkSpinButton *spinbutton,
				   ColWidthState *state)
{
	if (!state->adjusting) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (state->default_check), FALSE);
		dialog_col_width_button_sensitivity (state);
	}
}

static void
cb_dialog_col_width_default_check_toggled (GtkToggleButton *togglebutton, ColWidthState *state)
{
	if (!state->adjusting) {
		if (gtk_toggle_button_get_active (togglebutton)) {
			state->adjusting = TRUE;
			dialog_col_width_set_value (sheet_col_get_default_size_pts (state->sheet),
						     state);
			state->adjusting = FALSE;
		}
		dialog_col_width_button_sensitivity (state);
	}
}

static void
cb_dialog_col_width_apply_clicked (G_GNUC_UNUSED GtkWidget *button,
				   ColWidthState *state)
{
	gnm_float value = gtk_spin_button_get_value (state->spin);
	double const scale =
		state->sheet->last_zoom_factor_used *
		gnm_app_display_dpi_get (FALSE) / 72.;
	int size_pixels = (int)(value * scale + 0.5);
	gboolean use_default = gtk_toggle_button_get_active
		(GTK_TOGGLE_BUTTON (state->default_check));

	if (state->set_default_value) {
		cmd_colrow_std_size (WORKBOOK_CONTROL (state->wbcg),
				     state->sheet, TRUE, value);
		dialog_col_width_load_value (state);
	} else {
		if (use_default)
			size_pixels = 0;

		workbook_cmd_resize_selected_colrow (WORKBOOK_CONTROL (state->wbcg),
			state->sheet, TRUE, size_pixels);
		dialog_col_width_load_value (state);
	}

	return;
}

static void
cb_dialog_col_width_ok_clicked (GtkWidget *button, ColWidthState *state)
{
	cb_dialog_col_width_apply_clicked (button, state);
	gtk_widget_destroy (state->dialog);
	return;
}


static void
dialog_col_width_set_mode (gboolean set_default, ColWidthState *state)
{
	state->set_default_value = set_default;

	if (set_default) {
		gtk_widget_hide (state->default_check);
		gtk_label_set_text (GTK_LABEL (state->description),
				    _("Set standard/default column width"));
	} else {
		char *text;
		char *name = g_markup_escape_text (state->sheet->name_unquoted, -1);
		gtk_widget_show (state->default_check);
		text = g_strdup_printf (_("Set column width of selection on "
					  "<span style='italic' weight='bold'>%s</span>"),
					name);
		gtk_label_set_markup (GTK_LABEL (state->description), text);
		g_free (text);
		g_free (name);
	}

}

void
dialog_col_width (WBCGtk *wbcg, gboolean use_default)
{
	GladeXML *gui;
	ColWidthState *state;

	g_return_if_fail (wbcg != NULL);

	if (gnumeric_dialog_raise_if_exists (wbcg, COL_WIDTH_DIALOG_KEY))
		return;
	gui = gnm_glade_xml_new (GO_CMD_CONTEXT (wbcg),
		"col-width.glade", NULL, NULL);
	if (gui == NULL)
		return;

	state = g_new (ColWidthState, 1);
	state->wbcg  = wbcg;
	state->sv = wb_control_cur_sheet_view (WORKBOOK_CONTROL (wbcg));
	state->sheet = sv_sheet (state->sv);
	state->adjusting = FALSE;
	state->gui    = gui;
	state->dialog = glade_xml_get_widget (state->gui, "dialog");

	state->description = GTK_WIDGET (glade_xml_get_widget (state->gui, "description"));

	state->spin  = GTK_SPIN_BUTTON (glade_xml_get_widget (state->gui, "spin"));
	gtk_spin_button_get_adjustment (state->spin)->lower =
		GNM_COL_MARGIN + GNM_COL_MARGIN;
	g_signal_connect (G_OBJECT (state->spin),
		"value-changed",
		G_CALLBACK (cb_dialog_col_width_value_changed), state);

	state->default_check  = GTK_WIDGET (glade_xml_get_widget (state->gui, "default_check"));
	g_signal_connect (G_OBJECT (state->default_check),
		"clicked",
		G_CALLBACK (cb_dialog_col_width_default_check_toggled), state);

	state->ok_button = glade_xml_get_widget (state->gui, "ok_button");
	g_signal_connect (G_OBJECT (state->ok_button),
		"clicked",
		G_CALLBACK (cb_dialog_col_width_ok_clicked), state);
	state->apply_button = glade_xml_get_widget (state->gui, "apply_button");
	g_signal_connect (G_OBJECT (state->apply_button),
		"clicked",
		G_CALLBACK (cb_dialog_col_width_apply_clicked), state);

	state->cancel_button = glade_xml_get_widget (state->gui, "cancel_button");
	g_signal_connect (G_OBJECT (state->cancel_button),
		"clicked",
		G_CALLBACK (cb_dialog_col_width_cancel_clicked), state);

	gnumeric_init_help_button (
		glade_xml_get_widget (state->gui, "help_button"),
		GNUMERIC_HELP_LINK_COL_WIDTH);

	dialog_col_width_set_mode (use_default, state);
	dialog_col_width_load_value (state);

	wbc_gtk_attach_guru (state->wbcg, state->dialog);
	g_object_set_data_full (G_OBJECT (state->dialog),
		"state", state, (GDestroyNotify) cb_dialog_col_width_destroy);

	gnumeric_keyed_dialog (wbcg, GTK_WINDOW (state->dialog),
			       COL_WIDTH_DIALOG_KEY);
	gtk_widget_show (state->dialog);
}
