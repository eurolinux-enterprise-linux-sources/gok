/* gok-page-wordcomplete.h
*
* Copyright 2002 Sun Microsystems, Inc.,
* Copyright 2002 University Of Toronto
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

gboolean gok_page_wordcomplete_initialize (GladeXML* xml);
void gok_page_wordcomplete_refresh (void);
gboolean gok_page_wordcomplete_apply (void);
gboolean gok_page_wordcomplete_revert (void);
void gok_page_wordcomplete_backup (void);
void gok_page_wordcomplete_update_auxwordlist_from_control ();
void gok_page_wordcomplete_toggle_wordlist (gboolean on);
