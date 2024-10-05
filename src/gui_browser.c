/****************************************************************************
 *                                                                          *
 * gui_browser.c                                                            *
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

#include "common.h"
#include "gui_browser.h"

Rectangle browser_panel_rect;

char browser_panel_text[] = "Browser";

void init_gui_browser(void)
{
    resize_gui_browser();
}

void resize_gui_browser(void)
{
    browser_panel_rect.width  = window_size.x * 0.7;
    browser_panel_rect.height = window_size.y * 0.8;

    MINVAR(browser_panel_rect.width,  400);
    MINVAR(browser_panel_rect.height, 450);

    browser_panel_rect.x = (window_size.x / 2) - (browser_panel_rect.width  / 2);
    browser_panel_rect.y = (window_size.y / 2) - (browser_panel_rect.height / 2);

}

void draw_gui_browser(void)
{
    GuiPanel(browser_panel_rect, browser_panel_text);
}

