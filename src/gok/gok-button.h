/* gok-button.h
*
* Copyright 2001-2009 Sun Microsystems, Inc.,
* Copyright 2001-2009 University Of Toronto
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

#ifndef __GOKBUTTON_H__
#define __GOKBUTTON_H__

#include <gtk/gtk.h>

#include "gok-key.h"

G_BEGIN_DECLS

#define GOK_TYPE_BUTTON        (gok_button_get_type ())
#define GOK_BUTTON(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), GOK_TYPE_BUTTON, GokButton))
#define GOK_BUTTON_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST (k, GOK_TYPE_BUTTON, GokButtonClass)
#define GOK_IS_BUTTON(o)       (G_TYPE_CHECK_INSTANCE_TYPE (o, GOK_TYPE_BUTTON))
#define GOK_IS_BUTTON_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE (k, GOK_TYPE_BUTTON))

typedef struct _GokButton      GokButton;
typedef struct _GokButtonClass GokButtonClass;

struct _GokButton
{
	GtkToggleButton parent;

	GtkWidget* pBox;
	GtkWidget* pLabel;
        GtkWidget* pImage;
        gchar     *indicator_type;
};

struct _GokButtonClass
{
	GtkToggleButtonClass parent;
};

GType      gok_button_get_type (void) G_GNUC_CONST;
GtkWidget* gok_button_new_with_label (const gchar *pText, GokImagePlacementPolicy align);
GtkWidget* gok_button_new_with_image (GtkWidget *image, GokImagePlacementPolicy align);
void gok_button_set_image (GokButton *button, GtkImage *image);
void gok_button_set_label (GokButton *button, GtkLabel *label);
gint gok_button_enter_notify   (GtkWidget *widget, GdkEventCrossing   *event);
gint gok_button_leave_notify   (GtkWidget *widget, GdkEventCrossing   *event);
void gok_button_state_changed   (GtkWidget *widget, GtkStateType state, gpointer user_data);

G_END_DECLS

#endif /* __GOKBUTTON_H__ */
