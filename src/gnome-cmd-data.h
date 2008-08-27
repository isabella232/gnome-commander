/*
    GNOME Commander - A GNOME based file manager
    Copyright (C) 2001-2006 Marcus Bjurman

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef __GNOME_CMD_DATA_H__
#define __GNOME_CMD_DATA_H__

typedef struct _GnomeCmdData GnomeCmdData;
typedef struct _GnomeCmdDataPrivate GnomeCmdDataPrivate;

#include "gnome-cmd-app.h"
#include "gnome-cmd-types.h"
#include "gnome-cmd-file-list.h"
#include "filter.h"
#include "history.h"
#include "dict.h"

struct _GnomeCmdData
{
    GnomeCmdDataPrivate *priv;
};

typedef struct _GnomeCmdConFtp GnomeCmdConFtp;

typedef enum
{
    RIGHT_BUTTON_POPUPS_MENU,
    RIGHT_BUTTON_SELECTS
} GnomeCmdRightMouseButtonMode;

#define PATTERN_HISTORY_SIZE 10

typedef struct
{
    GList *name_patterns;
    GList *content_patterns;
    GList *directories;
    gboolean recursive;
    gboolean case_sens;
    gint width, height;
} SearchDefaults;

typedef struct
{
    GList *patterns;
    History *templates;
    guint counter_start;
    guint counter_precision;
    guint counter_increment;
    gboolean auto_update;
    gint width, height;
    gint pat_col_widths;
    gint res_col_widths;
    gint sep_value;
} AdvrenameDefaults;

typedef struct
{
    gboolean file_types[8];

    gboolean hidden;
    gboolean backup;
    gboolean other;
    gchar *other_value;
} FilterSettings;


GnomeCmdData *gnome_cmd_data_new (void);
void gnome_cmd_data_free (void);

void gnome_cmd_data_save (void);
void gnome_cmd_data_load (void);
void gnome_cmd_data_load_more (void);

gpointer gnome_cmd_data_get_con_list (void);

void gnome_cmd_data_add_fav_app (GnomeCmdApp *app);
void gnome_cmd_data_remove_fav_app (GnomeCmdApp *app);
GList *gnome_cmd_data_get_fav_apps (void);
void gnome_cmd_data_set_fav_apps (GList *apps);

const gchar *gnome_cmd_data_get_ftp_anonymous_password (void);
void gnome_cmd_data_set_ftp_anonymous_password (const gchar *pw);

const gboolean gnome_cmd_data_get_use_gnome_auth_manager (void);
void gnome_cmd_data_set_use_gnome_auth_manager (gboolean use_gnome_auth_manager);

GnomeCmdSizeDispMode gnome_cmd_data_get_size_disp_mode (void);
void gnome_cmd_data_set_size_disp_mode (GnomeCmdSizeDispMode mode);

GnomeCmdPermDispMode gnome_cmd_data_get_perm_disp_mode (void);
void gnome_cmd_data_set_perm_disp_mode (GnomeCmdPermDispMode mode);

GnomeCmdDateFormat gnome_cmd_data_get_date_format (void);
void gnome_cmd_data_set_date_format (GnomeCmdDateFormat format);

GnomeCmdLayout gnome_cmd_data_get_layout (void);
void gnome_cmd_data_set_layout (GnomeCmdLayout layout);

GnomeCmdColorMode gnome_cmd_data_get_color_mode (void);
void gnome_cmd_data_set_color_mode (GnomeCmdColorMode mode);

GnomeCmdColorTheme *gnome_cmd_data_get_custom_color_theme (void);
GnomeCmdColorTheme *gnome_cmd_data_get_current_color_theme (void);

gint gnome_cmd_data_get_list_row_height (void);
void gnome_cmd_data_set_list_row_height (gint height);

GnomeCmdExtDispMode gnome_cmd_data_get_ext_disp_mode (void);
void gnome_cmd_data_set_ext_disp_mode (GnomeCmdExtDispMode mode);

FilterSettings *gnome_cmd_data_get_filter_settings (void);
gboolean gnome_cmd_data_get_type_filter (GnomeVFSFileType type);
void gnome_cmd_data_set_hidden_filter (gboolean hide);
gboolean gnome_cmd_data_get_hidden_filter (void);
gboolean gnome_cmd_data_get_backup_filter (void);
gboolean gnome_cmd_data_get_other_filter (void);

void gnome_cmd_data_get_main_win_size (gint *width, gint *height);
void gnome_cmd_data_set_main_win_size (gint width, gint height);

void gnome_cmd_data_get_sort_params (GnomeCmdFileList *fl, gint *col, gboolean *direction);
void gnome_cmd_data_set_sort_params (GnomeCmdFileList *fl, gint col, gboolean direction);

const gchar *gnome_cmd_data_get_viewer (void);
const gchar *gnome_cmd_data_get_editor (void);
const gchar *gnome_cmd_data_get_differ (void);

void gnome_cmd_data_set_viewer (const gchar *command);
void gnome_cmd_data_set_editor (const gchar *command);
void gnome_cmd_data_set_differ (const gchar *command);

gboolean gnome_cmd_data_get_case_sens_sort (void);
void gnome_cmd_data_set_case_sens_sort (gboolean value);

gboolean gnome_cmd_data_get_confirm_delete (void);
void gnome_cmd_data_set_confirm_delete (gboolean value);

GnomeCmdConfirmOverwriteMode gnome_cmd_data_get_confirm_overwrite_copy(void);
void gnome_cmd_data_set_confirm_overwrite_copy(GnomeCmdConfirmOverwriteMode value);
GnomeCmdConfirmOverwriteMode gnome_cmd_data_get_confirm_overwrite_move(void);
void gnome_cmd_data_set_confirm_overwrite_move(GnomeCmdConfirmOverwriteMode value);

const gchar *gnome_cmd_data_get_list_font (void);
void gnome_cmd_data_set_list_font (const gchar *list_font);

GnomeCmdRightMouseButtonMode gnome_cmd_data_get_right_mouse_button_mode (void);
void gnome_cmd_data_set_right_mouse_button_mode (GnomeCmdRightMouseButtonMode mode);

const gchar *gnome_cmd_data_get_term (void);
void gnome_cmd_data_set_term (const gchar *shell);

gboolean gnome_cmd_data_get_use_gcmd_block (void);

gboolean gnome_cmd_data_get_show_toolbar (void);
void gnome_cmd_data_set_show_toolbar (gboolean value);

guint gnome_cmd_data_get_icon_size (void);
void gnome_cmd_data_set_icon_size (guint size);

guint gnome_cmd_data_get_dev_icon_size (void);
GdkInterpType gnome_cmd_data_get_icon_scale_quality (void);
void gnome_cmd_data_set_icon_scale_quality (GdkInterpType quality);

const gchar *gnome_cmd_data_get_theme_icon_dir (void);
void gnome_cmd_data_set_theme_icon_dir (const gchar *dir);

const gchar *gnome_cmd_data_get_document_icon_dir (void);
void gnome_cmd_data_set_document_icon_dir (const gchar *dir);

gint gnome_cmd_data_get_fs_col_width (guint column);
void gnome_cmd_data_set_fs_col_width (guint column, gint width);

gint gnome_cmd_data_get_bookmark_dialog_col_width (guint column);
void gnome_cmd_data_set_bookmark_dialog_col_width (guint column, gint width);

gint gnome_cmd_data_get_cmdline_history_length (void);
void gnome_cmd_data_set_cmdline_history_length (gint length);
GList *gnome_cmd_data_get_cmdline_history (void);

GtkReliefStyle gnome_cmd_data_get_button_relief (void);
void gnome_cmd_data_set_button_relief (GtkReliefStyle relief);

Filter::Type gnome_cmd_data_get_filter_type (void);
void gnome_cmd_data_set_filter_type (Filter::Type type);

gboolean gnome_cmd_data_get_device_only_icon (void);
void gnome_cmd_data_set_device_only_icon (gboolean value);

gint gnome_cmd_data_get_dir_cache_size (void);
void gnome_cmd_data_set_dir_cache_size (gint size);

gboolean gnome_cmd_data_get_use_ls_colors (void);
void gnome_cmd_data_set_use_ls_colors (gboolean value);

SearchDefaults *gnome_cmd_data_get_search_defaults (void);

GnomeCmdConFtp *gnome_cmd_data_get_quick_connect (void);

GnomeCmdBookmarkGroup *gnome_cmd_data_get_local_bookmarks (void);
GList *gnome_cmd_data_get_bookmark_groups (void);

gboolean gnome_cmd_data_get_honor_expect_uris (void);
void gnome_cmd_data_set_honor_expect_uris (gboolean value);

gboolean gnome_cmd_data_get_use_internal_viewer (void);
void gnome_cmd_data_set_use_internal_viewer (gboolean value);

gboolean gnome_cmd_data_get_alt_quick_search (void);
void gnome_cmd_data_set_alt_quick_search (gboolean value);

gboolean gnome_cmd_data_get_skip_mounting (void);
void gnome_cmd_data_set_skip_mounting (gboolean value);

gboolean gnome_cmd_data_get_toolbar_visibility (void);
void gnome_cmd_data_set_toolbar_visibility (gboolean value);

gboolean gnome_cmd_data_get_buttonbar_visibility (void);
void gnome_cmd_data_set_buttonbar_visibility (gboolean value);

AdvrenameDefaults *gnome_cmd_data_get_advrename_defaults (void);

gboolean gnome_cmd_data_get_list_orientation (void);
void gnome_cmd_data_set_list_orientation (gboolean vertical);

gboolean gnome_cmd_data_get_conbuttons_visibility (void);
void gnome_cmd_data_set_conbuttons_visibility (gboolean value);

gboolean gnome_cmd_data_get_cmdline_visibility (void);
void gnome_cmd_data_set_cmdline_visibility (gboolean value);

const gchar *gnome_cmd_data_get_start_dir (gboolean fs);
void gnome_cmd_data_set_start_dir (gboolean fs, const gchar *start_dir);

const gchar *gnome_cmd_data_get_last_pattern (void);
void gnome_cmd_data_set_last_pattern (const gchar *value);

GList *gnome_cmd_data_get_auto_load_plugins ();
void gnome_cmd_data_set_auto_load_plugins (GList *plugins);

guint gnome_cmd_data_get_gui_update_rate (void);

void gnome_cmd_data_get_main_win_pos (gint *x, gint *y);
void gnome_cmd_data_set_main_win_pos (gint x, gint y);

const gchar *gnome_cmd_data_get_backup_pattern (void);
void gnome_cmd_data_set_backup_pattern (const gchar *value);

GList *gnome_cmd_data_get_backup_pattern_list (void);

GdkWindowState gnome_cmd_data_get_main_win_state (void);
void gnome_cmd_data_set_main_win_state (GdkWindowState state);

const gchar *gnome_cmd_data_get_symlink_prefix (void);
void gnome_cmd_data_set_symlink_prefix (const gchar *value);


extern DICT<guint> gdk_key_names;
extern DICT<guint> gdk_modifiers_names;

#endif // __GNOME_CMD_DATA_H__
