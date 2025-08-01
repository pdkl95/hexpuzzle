/****************************************************************************
 *                                                                          *
 * gui_browser.h                                                            *
 *                                                                          *
 * This file is part of hexpuzzle.                                          *
 *                                                                          *
 * hexpuzzle is free software: you can redistribute it and/or               *
 * modify it under the terms of the GNU General Public License as published *
 * by the Free Software Foundation, either version 3 of the License,        *
 * or (at your option) any later version.                                   *
 *                                                                          *
 * hexpuzzle is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General *
 * Public License for more details.                                         *
 *                                                                          *
 * You should have received a copy of the GNU General Public License along  *
 * with hexpuzzle. If not, see <https://www.gnu.org/licenses/>.             *
 *                                                                          *
 ****************************************************************************/

#ifndef GUI_BROWSER_H
#define GUI_BROWSER_H

void init_gui_browser(void);
void cleanup_gui_browser(void);
void resize_gui_browser(void);
void draw_gui_browser(void);

void gui_browsee_reload(void);

extern int browser_active_tab;
#define BROWSER_ACTIVE_TAB_MIN 0
#define BROWSER_ACTIVE_TAB_MAX 2

#endif /*GUI_BROWSER_H*/

