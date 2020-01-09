/* gok-page-keyboard.c
*
* Copyright 2004 Sun Microsystems, Inc.,
* Copyright 2004 University Of Toronto
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public
* License along with this library; if not, write to the
* Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA.
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "gok-page-keyboard.h"
#include "gok-data.h"
#include "gok-log.h"
#include "gok-settings-dialog.h"

/* backup of the page data */
static GokComposeType save_compose_type;
static gchar *save_compose_filename = NULL;
static gchar *save_aux_kbd_dirname;

/* privates */
static void gok_page_keyboard_initialize_compose_filename (const char* file);
static void gok_page_keyboard_initialize_aux_keyboard_dir (const char* file);

/**
* gok_page_keyboard_initialize
* @pWindowSettings: Pointer to the settings dialog window.
*
* Initializes this page of the gok settings dialog. This must be called
* prior to any calls on this page.
*
* returns: TRUE if the page was properly initialized, FALSE if not.
**/
gboolean gok_page_keyboard_initialize (GladeXML* xml)
{
	GtkWidget* widget;
	
	g_assert (xml != NULL);
		
	/* store the current values */
	gok_page_keyboard_backup();

	/* update the controls */
	widget = glade_xml_get_widget (xml, "XkbComposeKeyboardRadiobutton");
	g_assert (widget != NULL);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), save_compose_type == GOK_COMPOSE_XKB);

	widget = glade_xml_get_widget (xml, "AlphaComposeKeyboardRadiobutton");
	g_assert (widget != NULL);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), save_compose_type == GOK_COMPOSE_ALPHA);

	widget = glade_xml_get_widget (xml, "AlphaFrequencyComposeKeyboardRadiobutton");
	g_assert (widget != NULL);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), save_compose_type == GOK_COMPOSE_ALPHAFREQ);

	widget = glade_xml_get_widget (xml, "XmlComposeKeyboardRadiobutton");
	g_assert (widget != NULL);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), save_compose_type == GOK_COMPOSE_CUSTOM);

	gok_page_keyboard_initialize_compose_filename ((save_compose_type == GOK_COMPOSE_CUSTOM) ?
							gok_data_get_custom_compose_filename () : "");
	gok_page_keyboard_initialize_aux_keyboard_dir (gok_data_get_aux_keyboard_directory ());
	
	return TRUE;
}


/**
* gok_page_keyboard_apply
* 
* Updates the gok data with values from the controls.
*
* returns: TRUE if any of the gok data settings have changed, FALSE if not.
**/
gboolean gok_page_keyboard_apply ()
{
	GtkWidget* pWidget;
	gboolean bDataChanged;
	gchar* text;
	
	bDataChanged = FALSE;

	/* N.B. this page is already instant-apply; so 'Apply' really means 'sync saved values' */
	gok_page_keyboard_backup (); /* read the current gok-data values into our cache */

	return bDataChanged;
}

/**
* gok_page_keyboard_revert
* 
* Revert to the backup settings for this page.
*
* returns: TRUE if any of the settings have changed, FALSE 
* if they are all still the same.
**/
gboolean gok_page_keyboard_revert ()
{
	gok_data_set_compose_keyboard_type (save_compose_type);
	gok_data_set_custom_compose_filename (save_compose_filename);
	gok_data_set_aux_keyboard_directory (save_aux_kbd_dirname);
	gok_page_keyboard_initialize (gok_settingsdialog_get_xml ());

	return TRUE;
}

/**
* gok_page_keyboard_backup
* 
* Copies all the member settings to backup.
**/
void gok_page_keyboard_backup ()
{
	save_compose_type = gok_data_get_compose_keyboard_type ();
	if (save_compose_type == GOK_COMPOSE_CUSTOM) 
	    save_compose_filename = gok_data_get_custom_compose_filename ();
	save_aux_kbd_dirname = gok_data_get_aux_keyboard_directory ();
}

void 
gok_page_keyboard_update_custom_dir_from_control ()
{
	GtkWidget* pWidget; 
	gchar* text;
	
	gok_log_enter();
	pWidget = glade_xml_get_widget (gok_settingsdialog_get_xml(), "AuxKeyboardDirEntry");
	g_assert (pWidget != NULL);
	text = g_strdup (gtk_entry_get_text (GTK_ENTRY (pWidget)));
	g_free (save_aux_kbd_dirname);
	save_aux_kbd_dirname = text;

	gok_log_leave();
}

void 
gok_page_keyboard_update_custom_compose_from_control ()
{
	GtkWidget* pWidget; 
	gchar* text;
	
	gok_log_enter();
	pWidget = glade_xml_get_widget (gok_settingsdialog_get_xml(), "XmlKeyboardFileChooser");
	g_assert (pWidget != NULL);
	text = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (pWidget));
	g_free (save_compose_filename);
	save_compose_filename = text;
	gok_log_leave();
}

static void
gok_page_keyboard_initialize_aux_keyboard_dir (const char* file)
{
	GtkWidget* pWidget; 
	
	gok_log_enter();
	if (file)
	{
		pWidget = glade_xml_get_widget (gok_settingsdialog_get_xml (),
						"AuxKeyboardDirEntry");
		g_assert (pWidget != NULL);
		gtk_entry_set_text (GTK_ENTRY (pWidget), file);
	}
	gok_log_leave();
}

void
on_aux_keyboard_dir_dialog_response (GtkDialog *dialog, gint response, gpointer data)
{
	gchar *folder;

	if (response == GTK_RESPONSE_ACCEPT)
	{
		folder = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		if (folder)
		{
			GtkWidget *entry;

			entry = glade_xml_get_widget (gok_settingsdialog_get_xml (),
						      "AuxKeyboardDirEntry");
			gtk_entry_set_text (GTK_ENTRY (entry), folder);
			gok_data_set_aux_keyboard_directory (folder);
			g_free (folder);
		}
	}
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

void
on_aux_keyboard_dir_button_clicked (GtkButton *button, gpointer data)
{
	GtkWidget *dialog, *entry;
	gchar *folder;

	dialog = gtk_file_chooser_dialog_new (
		_("Enter directory to search for additional GOK keyboard files."),
		NULL,
		GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_window_set_type_hint (GTK_WINDOW (dialog),
				  GDK_WINDOW_TYPE_HINT_NORMAL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog),
					 GTK_RESPONSE_ACCEPT);
	gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
						 GTK_RESPONSE_ACCEPT,
						 GTK_RESPONSE_CANCEL,
						 -1);
	g_signal_connect (dialog, "response",
			  G_CALLBACK (on_aux_keyboard_dir_dialog_response),
			  NULL);

	entry = glade_xml_get_widget (gok_settingsdialog_get_xml (),
				      "AuxKeyboardDirEntry");
	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog),
				       gtk_entry_get_text (GTK_ENTRY (entry)));
	gtk_widget_show (dialog);
}

static void
gok_page_keyboard_initialize_compose_filename (const char* file)
{
	GtkWidget* pWidget; 
	
	gok_log_enter();
	pWidget = glade_xml_get_widget (gok_settingsdialog_get_xml(), "XmlKeyboardFileChooser");
	g_assert (pWidget != NULL);
	gtk_file_chooser_button_set_title (GTK_FILE_CHOOSER_BUTTON (pWidget),
					   _("Select the XML file defining your startup compose keyboard"));
	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (pWidget), file);
	gok_log_leave();
}

void
on_compose_keyboard_file_set (GtkFileChooserButton *button, gpointer data)
{
	gchar *file;

	file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (button));
	gok_data_set_custom_compose_filename (file ? file : "");
	g_free (file);
}
