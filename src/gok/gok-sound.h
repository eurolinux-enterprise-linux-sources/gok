/*
 * gok-sound.h
 *
 * Copyright 2002-2009 Sun Microsystems, Inc.,
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

#ifndef __GOK_SOUND_H__
#define __GOK_SOUND_H__

G_BEGIN_DECLS

void gok_sound_initialize (void);
gint gok_sound_play       (const gchar *soundfile);

G_END_DECLS

#endif /* __GOK_SOUND_H__ */
