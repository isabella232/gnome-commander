/**
 * @file gnome-cmd-file.cc
 * @copyright (C) 2001-2006 Marcus Bjurman\n
 * @copyright (C) 2007-2012 Piotr Eljasiak\n
 * @copyright (C) 2013-2021 Uwe Scholz\n
 *
 * @copyright This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * @copyright This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * @copyright You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <errno.h>

#include "gnome-cmd-includes.h"
#include "utils.h"
#include "gnome-cmd-owner.h"
#include "imageloader.h"
#include "gnome-cmd-data.h"
#include "gnome-cmd-plain-path.h"
#include "gnome-cmd-main-win.h"
#include "gnome-cmd-con-list.h"
#include "gnome-cmd-xfer.h"
#include "tags/gnome-cmd-tags.h"
#include "intviewer/libgviewer.h"
#include "dialogs/gnome-cmd-file-props-dialog.h"

using namespace std;


#define MAX_TYPE_LENGTH 2
#define MAX_NAME_LENGTH 128
#define MAX_OWNER_LENGTH 128
#define MAX_GROUP_LENGTH 128
#define MAX_PERM_LENGTH 10
#define MAX_DATE_LENGTH 64
#define MAX_SIZE_LENGTH 32

gint created_files_cnt = 0;
gint deleted_files_cnt = 0;
GList *all_files = nullptr;

struct GnomeCmdFile::Private
{
    Handle *dir_handle;
    GTimeVal last_update;
    gint ref_cnt;
    guint64 tree_size;
};


G_DEFINE_TYPE (GnomeCmdFile, gnome_cmd_file, GNOME_CMD_TYPE_FILE_BASE)


inline gboolean has_parent_dir (GnomeCmdFile *f)
{
    return f->priv->dir_handle && f->priv->dir_handle->ref;
}


inline GnomeCmdDir *get_parent_dir (GnomeCmdFile *f)
{
    g_return_val_if_fail (f->priv->dir_handle != nullptr, nullptr);

    return GNOME_CMD_DIR (f->priv->dir_handle->ref);
}


static void gnome_cmd_file_init (GnomeCmdFile *f)
{
    // f->info = nullptr;
    // f->collate_key = nullptr;

    f->priv = g_new0 (GnomeCmdFile::Private, 1);

    // f->priv->dir_handle = nullptr;

    // f->priv->last_update.tv_sec = 0;
    // f->priv->last_update.tv_usec = 0;

    f->priv->tree_size = -1;

    if (DEBUG_ENABLED ('c'))
    {
        all_files = g_list_append (all_files, f);
        created_files_cnt++;
    }
}


static void gnome_cmd_file_finalize (GObject *object)
{
    GnomeCmdFile *f = GNOME_CMD_FILE (object);

    delete f->metadata;

    if (f->info->name[0] != '.')
        DEBUG ('f', "file destroying 0x%p %s\n", f, f->info->name);

    g_free (f->collate_key);
    gnome_vfs_file_info_unref (f->info);
    if (f->priv->dir_handle)
        handle_unref (f->priv->dir_handle);

    if (DEBUG_ENABLED ('c'))
    {
        all_files = g_list_remove (all_files, f);
        deleted_files_cnt++;
    }

    g_free (f->priv);

    G_OBJECT_CLASS (gnome_cmd_file_parent_class)->finalize (object);
}


static void gnome_cmd_file_class_init (GnomeCmdFileClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = gnome_cmd_file_finalize;
}


/***********************************
 * Public functions
 ***********************************/

GnomeCmdFile *gnome_cmd_file_new (const gchar *local_full_path)
{
    g_return_val_if_fail (local_full_path != nullptr, nullptr);

    gchar *text_uri = gnome_vfs_get_uri_from_local_path (local_full_path);
    GnomeVFSURI *uri = gnome_vfs_uri_new (text_uri);
    GnomeCmdFile *f = gnome_cmd_file_new_from_uri (uri);

    gnome_vfs_uri_unref (uri);
    g_free (text_uri);

    return f;
}


GnomeCmdFile *gnome_cmd_file_new (GnomeVFSFileInfo *info, GnomeCmdDir *dir)
{
    auto gnomeCmdFile = static_cast<GnomeCmdFile*> (g_object_new (GNOME_CMD_TYPE_FILE, nullptr));

    gnome_cmd_file_setup (gnomeCmdFile, info, dir);

    return gnomeCmdFile;
}


GnomeCmdFile *gnome_cmd_file_new_from_uri (GnomeVFSURI *uri)
{
    g_return_val_if_fail (uri != nullptr, nullptr);
    g_return_val_if_fail (gnome_vfs_uri_is_local (uri), nullptr);

    const GnomeVFSFileInfoOptions infoOpts = (GnomeVFSFileInfoOptions) (GNOME_VFS_FILE_INFO_FOLLOW_LINKS|GNOME_VFS_FILE_INFO_GET_MIME_TYPE);
    GnomeVFSFileInfo *info = gnome_vfs_file_info_new ();

    if (gnome_vfs_get_file_info_uri (uri, info, infoOpts) != GNOME_VFS_OK)
    {
        gnome_vfs_file_info_unref (info);
        return nullptr;
    }

    GnomeVFSURI *parent = gnome_vfs_uri_get_parent (uri);
    gchar *parent_path = gnome_vfs_unescape_string (gnome_vfs_uri_get_path (parent), nullptr);
    GnomeCmdDir *dir = gnome_cmd_dir_new (get_home_con(), new GnomeCmdPlainPath(parent_path));

    g_free (parent_path);
    gnome_vfs_uri_unref (parent);

    return gnome_cmd_file_new (info, dir);
}

#if 0
static void add_file_callback(GObject *direnum,
                GAsyncResult *result,
                gpointer user_data){
    GError *error = NULL;
    GList *file_list = g_file_enumerator_next_files_finish(
                    G_FILE_ENUMERATOR(direnum),
                    result, &error);
    if( error ){
        g_critical("Unable to add files to list, error: %s", error->message);
        g_object_unref(direnum);
        g_error_free(error);
        return;
    }else if( file_list == NULL ){
        /* Done listing */
        g_object_unref(direnum);
        return;
    }else{

        GList *node = file_list;
        GFileInfo *info;
        GtkTreeIter iter;
        while(node){
            info = node->data;
            node = node->next;
            ...add to store
            g_object_unref(info);
        }
        g_file_enumerator_next_files_async(G_FILE_ENUMERATOR(direnum),
                        BLOCK_SIZE,
                        G_PRIORITY_LOW,
                        NULL,
                        add_file_callback,
                        list);
    }
    g_list_free(file_list);
}
#endif

void GnomeCmdFile::invalidate_metadata()
{
    delete metadata;
    metadata = nullptr;
}


void gnome_cmd_file_setup (GnomeCmdFile *gnomeCmdFile, GnomeVFSFileInfo *info, GnomeCmdDir *dir)
{
    g_return_if_fail (gnomeCmdFile != nullptr);

    gnomeCmdFile->info = info;

    gnomeCmdFile->is_dotdot = info->type==GNOME_VFS_FILE_TYPE_DIRECTORY && strcmp(info->name, "..")==0;    // check if file is '..'

    gchar *utf8_name;

    if (!gnome_cmd_data.options.case_sens_sort)
    {
        gchar *s = get_utf8 (info->name);
        utf8_name = g_utf8_casefold (s, -1);
        g_free (s);
    }
    else
        utf8_name = get_utf8 (info->name);

    gnomeCmdFile->collate_key = g_utf8_collate_key_for_filename (utf8_name, -1);
    g_free (utf8_name);

    if (dir)
    {
        gnomeCmdFile->priv->dir_handle = gnome_cmd_dir_get_handle (dir);
        handle_ref (gnomeCmdFile->priv->dir_handle);
    }

    gnome_vfs_file_info_ref (gnomeCmdFile->info);

    auto fUriString = gnomeCmdFile->get_path();

    if (fUriString)
    {
        GNOME_CMD_FILE_BASE (gnomeCmdFile)->gFile = g_file_new_for_path(fUriString);
        gnomeCmdFile->gFile = GNOME_CMD_FILE_BASE (gnomeCmdFile)->gFile;
        g_free(fUriString);

    }
}


GnomeCmdFile *GnomeCmdFile::ref()
{
    priv->ref_cnt++;

    if (priv->ref_cnt == 1)
        g_object_ref (this);

    char c = GNOME_CMD_IS_DIR (this) ? 'd' : 'f';

    DEBUG (c, "refing: 0x%p %s to %d\n", this, info->name, priv->ref_cnt);

    return this;
}


void GnomeCmdFile::unref()
{
    priv->ref_cnt--;

    char c = GNOME_CMD_IS_DIR (this) ? 'd' : 'f';

    DEBUG (c, "un-refing: 0x%p %s to %d\n", this, info->name, priv->ref_cnt);
    if (priv->ref_cnt < 1)
        g_object_unref (this);
}


GnomeVFSResult GnomeCmdFile::chmod(GnomeVFSFilePermissions perm)
{
    g_return_val_if_fail (info != nullptr, GNOME_VFS_ERROR_CORRUPTED_DATA);

    info->permissions = perm;
    GnomeVFSURI *uri = get_uri();
    GnomeVFSResult ret = gnome_vfs_set_file_info_uri (uri, info, GNOME_VFS_SET_FILE_INFO_PERMISSIONS);
    gnome_vfs_uri_unref (uri);

    if (has_parent_dir (this))
    {
        GnomeCmdDir *dir = ::get_parent_dir (this);
        gchar *uri_str = get_uri_str();
        gnome_cmd_dir_file_changed (dir, uri_str);
        g_free (uri_str);
    }

    return ret;
}


GnomeVFSResult GnomeCmdFile::chown(uid_t uid, gid_t gid)
{
    g_return_val_if_fail (info != nullptr, GNOME_VFS_ERROR_CORRUPTED_DATA);

    if (uid != (uid_t)-1)
        info->uid = uid;
    info->gid = gid;

    GnomeVFSURI *uri = get_uri();
    GnomeVFSResult ret = gnome_vfs_set_file_info_uri (uri, info, GNOME_VFS_SET_FILE_INFO_OWNER);
    gnome_vfs_uri_unref (uri);

    if (has_parent_dir (this))
    {
        GnomeCmdDir *dir = ::get_parent_dir (this);
        gchar *uri_str = get_uri_str();
        gnome_cmd_dir_file_changed (dir, uri_str);
        g_free (uri_str);
    }

    return ret;
}


GnomeVFSResult GnomeCmdFile::rename(const gchar *new_name)
{
    g_return_val_if_fail (info, GNOME_VFS_ERROR_CORRUPTED_DATA);

    GnomeVFSFileInfo *new_info = gnome_vfs_file_info_new ();
    g_return_val_if_fail (new_info, GNOME_VFS_ERROR_CORRUPTED_DATA);

    new_info->name = const_cast<gchar *> (new_name);

    GError *error;
    error = nullptr;

    auto newgFile = g_file_set_display_name (this->gFile, new_name, nullptr, &error);

    if (error || newgFile == nullptr)
    {
        g_message ("rename to \"%s\" failed: %s", new_name, error->message);
        g_error_free (error);
        return GNOME_VFS_OK;
    }

    g_object_unref(this->gFile);
    this->gFile = newgFile;

    // TODO: Remove the following when GnomeVFS->GIO migration is done
    const GnomeVFSFileInfoOptions infoOpts = (GnomeVFSFileInfoOptions) (GNOME_VFS_FILE_INFO_FOLLOW_LINKS|GNOME_VFS_FILE_INFO_GET_MIME_TYPE);
    auto newUri = get_uri(new_name);
    auto result = gnome_vfs_get_file_info_uri (newUri, new_info, infoOpts);
    gnome_vfs_uri_unref (newUri);

    if (result==GNOME_VFS_OK && has_parent_dir (this))
    {
        gchar *old_uri_str = get_uri_str();

        update_info(new_info);
        gnome_cmd_dir_file_renamed (::get_parent_dir (this), this, old_uri_str);
        if (GNOME_CMD_IS_DIR (this))
            gnome_cmd_dir_update_path (GNOME_CMD_DIR (this));
    }

    return result;
}


gchar *GnomeCmdFile::get_quoted_name()
{
    g_return_val_if_fail (info != nullptr, nullptr);

    return quote_if_needed (info->name);
}


gchar *GnomeCmdFile::get_path()
{
    g_return_val_if_fail (info != nullptr, nullptr);

    if (strcmp (info->name, G_DIR_SEPARATOR_S) == 0)
        return g_strdup (G_DIR_SEPARATOR_S);

    GnomeCmdPath *path;
    gchar *path_str;

    if (!has_parent_dir (this))
    {
        if (GNOME_CMD_IS_DIR (this))
        {
            path = gnome_cmd_dir_get_path (GNOME_CMD_DIR (this));
            return g_strdup (path->get_path());
        }
        g_assert ("Non directory file without owning directory");
    }

    path = gnome_cmd_dir_get_path (::get_parent_dir (this))->get_child(info->name);
    path_str = g_strdup (path->get_path());
    delete path;

    return path_str;
}


gchar *GnomeCmdFile::get_real_path()
{
    GnomeVFSURI *uri = get_uri();
    gchar *path = gnome_vfs_unescape_string (gnome_vfs_uri_get_path (uri), nullptr);
    gnome_vfs_uri_unref (uri);

    return path;
}


gchar *GnomeCmdFile::get_quoted_real_path()
{
    gchar *path = get_real_path();
    gchar *ret = quote_if_needed (path);

    g_free (path);

    return ret;
}


gchar *GnomeCmdFile::get_dirname()
{
    GnomeVFSURI *uri = get_uri();
    gchar *path = gnome_vfs_uri_extract_dirname (uri);
    gnome_vfs_uri_unref (uri);

    return path;
}


gchar *GnomeCmdFile::get_unescaped_dirname()
{
    GnomeVFSURI *uri = get_uri();
    gchar *path = gnome_vfs_uri_extract_dirname (uri);
    gnome_vfs_uri_unref (uri);
    gchar *unescaped_path = gnome_vfs_unescape_string (path, nullptr);
    g_free (path);

    return unescaped_path;
}


GAppInfo *GnomeCmdFile::GetAppInfoForContentType()
{
    auto contentTypeString = this->GetGfileAttributeString(G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);
    auto appInfo = g_app_info_get_default_for_type (contentTypeString, false);
    g_free(contentTypeString);

    return appInfo;
}


gchar *GnomeCmdFile::GetGfileAttributeString(const char *attribute)
{
    GError *error;
    error = nullptr;
    auto gcmdFileInfo = g_file_query_info(this->gFile,
                                   attribute,
                                   G_FILE_QUERY_INFO_NONE,
                                   nullptr,
                                   &error);
    if (gcmdFileInfo && error)
    {
        g_message ("retrieving file info failed: %s", error->message);
        g_error_free (error);
        return nullptr;
    }

    auto gFileAttributeString = g_strdup(g_file_info_get_attribute_string (gcmdFileInfo, attribute));
    g_object_unref(gcmdFileInfo);

    return gFileAttributeString;
}


guint32 GnomeCmdFile::GetGfileAttributeUInt32(const char *attribute)
{
    GError *error;
    error = nullptr;
    guint32 gFileAttributeUInt32 = 0;

    auto gcmdFileInfo = g_file_query_info(this->gFile,
                                   attribute,
                                   G_FILE_QUERY_INFO_NONE,
                                   nullptr,
                                   &error);
    if (error)
    {
        g_message ("retrieving file info failed: %s", error->message);
        g_error_free (error);
    }
    else
    {
        gFileAttributeUInt32 = g_file_info_get_attribute_uint32 (gcmdFileInfo, attribute);
        g_object_unref(gcmdFileInfo);
    }

    return gFileAttributeUInt32;
}


guint64 GnomeCmdFile::GetGfileAttributeUInt64(const char *attribute)
{
    GError *error;
    error = nullptr;
    guint64 gFileAttributeUInt64 = 0;

    auto gcmdFileInfo = g_file_query_info(this->gFile,
                                   attribute,
                                   G_FILE_QUERY_INFO_NONE,
                                   nullptr,
                                   &error);
    if (error)
    {
        g_message ("retrieving file info failed: %s", error->message);
        g_error_free (error);
    }
    else
    {
        gFileAttributeUInt64 = g_file_info_get_attribute_uint64 (gcmdFileInfo, attribute);
        g_object_unref(gcmdFileInfo);
    }

    return gFileAttributeUInt64;
}


gchar *GnomeCmdFile::get_default_application_name_string()
{
    auto contentType = GetGfileAttributeString (G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);

    auto appInfo = g_app_info_get_default_for_type (contentType, false);

    g_free(contentType);
    return g_strdup(g_app_info_get_name (appInfo));
}


gchar *GnomeCmdFile::get_default_application_action_label(GAppInfo *gAppInfo)
{
    gchar *escaped_app_name = get_default_application_name (gAppInfo);
    if (escaped_app_name == nullptr)
    {
        return g_strdup (_("_Open"));
    }

    gchar *retval = g_strdup_printf (_("_Open with “%s”"), escaped_app_name);
    g_free (escaped_app_name);

    return retval;
}


gchar *GnomeCmdFile::get_default_application_name(GAppInfo *gAppInfo)
{
    gchar *escaped_app_name = string_double_underscores (g_app_info_get_name (gAppInfo));

    return escaped_app_name;
}


GnomeVFSMimeApplication *GnomeCmdFile::get_default_gnome_vfs_app_for_mime_type()
{
    g_return_val_if_fail (GNOME_CMD_IS_FILE (this), nullptr);

    auto uri_str = this->get_uri_str();
    auto *app = gnome_vfs_mime_get_default_application_for_uri (uri_str, this->info->mime_type);

    g_free (uri_str);
    return app;
}


GnomeVFSURI *GnomeCmdFile::get_uri(const gchar *name)
{
    if (!has_parent_dir (this))
    {
        if (GNOME_CMD_IS_DIR (this))
        {
            GnomeCmdPath *path = gnome_cmd_dir_get_path (GNOME_CMD_DIR (this));
            GnomeCmdCon *con = gnome_cmd_dir_get_connection (GNOME_CMD_DIR (this));
            return gnome_cmd_con_create_uri (con, path);
        }
        else
            g_assert ("Non directory file without owning directory");
    }

    return gnome_cmd_dir_get_child_uri (::get_parent_dir (this), name ? name : info->name);
}


gchar *GnomeCmdFile::get_uri_str(GnomeVFSURIHideOptions hide_options)
{
    GnomeVFSURI *uri = get_uri();
    gchar *uri_str = gnome_vfs_uri_to_string (uri, hide_options);
    gnome_vfs_uri_unref (uri);

    return uri_str;
}


const gchar *GnomeCmdFile::get_extension()
{
    g_return_val_if_fail (info != nullptr, nullptr);

    if (GetGfileAttributeUInt32(G_FILE_ATTRIBUTE_STANDARD_TYPE) == G_FILE_TYPE_DIRECTORY)
        return nullptr;

    const char *s = strrchr (info->name, '.');        // does NOT work on UTF-8 strings, should be (MUCH SLOWER):
    // const char *s = g_utf8_strrchr (info->name, -1, '.');

    return s ? s+1 : nullptr;
}


const gchar *GnomeCmdFile::get_owner()
{
    g_return_val_if_fail (info != nullptr, nullptr);

    if (GNOME_VFS_FILE_INFO_LOCAL (info))
        return gcmd_owner.get_name_by_uid(info->uid);
    else
    {
        static gchar owner_str[MAX_OWNER_LENGTH];
        g_snprintf (owner_str, MAX_OWNER_LENGTH, "%d", info->uid);
        return owner_str;
    }
}


const gchar *GnomeCmdFile::get_group()
{
    g_return_val_if_fail (info != nullptr, nullptr);

    if (GNOME_VFS_FILE_INFO_LOCAL (info))
        return gcmd_owner.get_name_by_gid(info->gid);
    else
    {
        static gchar group_str[MAX_GROUP_LENGTH];
        g_snprintf (group_str, MAX_GROUP_LENGTH, "%d", info->gid);
        return group_str;
    }
}


inline const gchar *date2string (time_t date, gboolean overide_disp_setting)
{
    return time2string (date, overide_disp_setting?"%c":gnome_cmd_data.options.date_format);
}


const gchar *GnomeCmdFile::get_adate(gboolean overide_disp_setting)
{
    g_return_val_if_fail (info != nullptr, nullptr);

    return date2string (info->atime, overide_disp_setting);
}


const gchar *GnomeCmdFile::get_mdate(gboolean overide_disp_setting)
{
    g_return_val_if_fail (info != nullptr, nullptr);

    return date2string (info->mtime, overide_disp_setting);
}


const gchar *GnomeCmdFile::get_size()
{
    static gchar dir_indicator[] = "<DIR> ";

    g_return_val_if_fail (info != nullptr, nullptr);

    if (GetGfileAttributeUInt32(G_FILE_ATTRIBUTE_STANDARD_TYPE) == G_FILE_TYPE_DIRECTORY)
        return dir_indicator;

    auto size = GetGfileAttributeUInt64(G_FILE_ATTRIBUTE_STANDARD_SIZE);
    return size2string (size, gnome_cmd_data.options.size_disp_mode);
}


guint64 GnomeCmdFile::get_tree_size()
{
    if (GetGfileAttributeUInt32(G_FILE_ATTRIBUTE_STANDARD_TYPE) != G_FILE_TYPE_DIRECTORY)
        return info->size;

    if (is_dotdot)
        return 0;

    if (priv->tree_size != (guint64)-1)
        return priv->tree_size;

    priv->tree_size = calc_tree_size (nullptr);

    return priv->tree_size;
}


guint64 GnomeCmdFile::calc_tree_size (gulong *count)
{
    g_return_val_if_fail (this->gFile != NULL, -1);

    guint64 size = 0;

    if (GetGfileAttributeUInt32(G_FILE_ATTRIBUTE_STANDARD_TYPE) == G_FILE_TYPE_DIRECTORY)
    {
        GError *error;
        error = nullptr;

        g_file_measure_disk_usage (this->gFile,
                           G_FILE_MEASURE_NONE,
                           nullptr,
                           nullptr,
                           nullptr,
                           &size,
                           nullptr,
                           nullptr,
                           &error);

        if (error)
        {
            g_message ("calc_tree_size: closing gFileEnumerator failed: %s", error->message);
            g_error_free (error);
        }
    }
    else
    {
        size += GetGfileAttributeUInt32(G_FILE_ATTRIBUTE_STANDARD_SIZE);
        if (count!=NULL) {
            (*count)++;
        }
    }
    return size;
}


const gchar *GnomeCmdFile::get_tree_size_as_str()
{
    g_return_val_if_fail (info != nullptr, nullptr);

    if (GetGfileAttributeUInt32(G_FILE_ATTRIBUTE_STANDARD_TYPE) != G_FILE_TYPE_DIRECTORY)
        return get_size();

    if (is_dotdot)
        return get_size();

    return size2string (get_tree_size(), gnome_cmd_data.options.size_disp_mode);
}


const gchar *GnomeCmdFile::get_perm()
{
    static gchar perm_str[MAX_PERM_LENGTH];

    perm2string (GetGfileAttributeUInt32(G_FILE_ATTRIBUTE_UNIX_MODE) & 0xFFF, perm_str, MAX_PERM_LENGTH);
    return perm_str;
}


const gchar *GnomeCmdFile::get_type_string()
{
    static gchar type_str[MAX_TYPE_LENGTH];

    type2string (GetGfileAttributeUInt32(G_FILE_ATTRIBUTE_STANDARD_TYPE), type_str, MAX_TYPE_LENGTH);
    return type_str;
}


gboolean GnomeCmdFile::get_type_pixmap_and_mask(GdkPixmap **pixmap, GdkBitmap **mask)
{
    g_return_val_if_fail (info != nullptr, FALSE);

    return IMAGE_get_pixmap_and_mask (GetGfileAttributeUInt32(G_FILE_ATTRIBUTE_STANDARD_TYPE),
                                      info->mime_type, info->symlink_name != nullptr, pixmap, mask);
}


gboolean GnomeCmdFile::has_mime_type(const gchar *mime_type)
{
    g_return_val_if_fail (info != nullptr, FALSE);
    g_return_val_if_fail (info->mime_type != nullptr, FALSE);
    g_return_val_if_fail (mime_type != nullptr, FALSE);

    return strcmp (info->mime_type, mime_type) == 0;
}


gboolean GnomeCmdFile::mime_begins_with(const gchar *mime_type_start)
{
    g_return_val_if_fail (info != nullptr, FALSE);
    g_return_val_if_fail (info->mime_type != nullptr, FALSE);
    g_return_val_if_fail (mime_type_start != nullptr, FALSE);

    return strncmp (info->mime_type, mime_type_start, strlen(mime_type_start)) == 0;
}


void gnome_cmd_file_show_properties (GnomeCmdFile *f)
{
    GtkWidget *dialog = gnome_cmd_file_props_dialog_create (f);
    if (!dialog) return;

    g_object_ref (dialog);
    gtk_widget_show (dialog);
}


inline void do_view_file (GnomeCmdFile *f, gint internal_viewer=-1)
{
    if (internal_viewer==-1)
        internal_viewer = gnome_cmd_data.options.use_internal_viewer;

    switch (internal_viewer)
    {
        case TRUE : {
                        GtkWidget *viewer = gviewer_window_file_view (f);
                        gtk_widget_show (viewer);
                        gdk_window_set_icon (viewer->window, nullptr,
                                             IMAGE_get_pixmap (PIXMAP_INTERNAL_VIEWER),
                                             IMAGE_get_mask (PIXMAP_INTERNAL_VIEWER));
                    }
                    break;

        case FALSE: {
                        gchar *filename = f->get_quoted_real_path();
#if defined (__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
                        gchar *command = g_strdup_printf (gnome_cmd_data.options.viewer, filename);
#if defined (__GNUC__)
#pragma GCC diagnostic pop
#endif
                        run_command (command);
                        g_free (filename);
                    }
                    break;

        default: break;
    }
}


static void on_file_downloaded_for_view (GnomeVFSURI *uri)
{
    GnomeCmdFile *f = gnome_cmd_file_new_from_uri (uri);

    if (!f)
        return;

    do_view_file (f);
    f->unref();
}


void gnome_cmd_file_view (GnomeCmdFile *f, gint internal_viewer)
{
    g_return_if_fail (f != nullptr);
    g_return_if_fail (has_parent_dir (f));

    // If the file is local there is no need to download it
    if (f->is_local())
    {
        do_view_file (f, internal_viewer);
        return;
    }

    // The file is remote, let's download it to a temporary file first
    gchar *path_str = get_temp_download_filepath (f->get_name());
    if (!path_str)  return;

    GnomeCmdPlainPath path(path_str);
    GnomeVFSURI *src_uri = f->get_uri();
    GnomeVFSURI *dest_uri = gnome_cmd_con_create_uri (get_home_con (), &path);

    g_printerr ("Copying to: %s\n", path_str);
    g_free (path_str);

    gnome_cmd_xfer_tmp_download (src_uri,
                                 dest_uri,
                                 GNOME_VFS_XFER_FOLLOW_LINKS,
                                 GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
                                 GTK_SIGNAL_FUNC (on_file_downloaded_for_view),
                                 dest_uri);
}


void gnome_cmd_file_edit (GnomeCmdFile *f)
{
    g_return_if_fail (f != nullptr);

    if (!f->is_local())
        return;

    gchar *fpath = f->get_quoted_real_path();
    gchar *dpath = f->get_unescaped_dirname();
#if defined (__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
    gchar *command = g_strdup_printf (gnome_cmd_data.options.editor, fpath);
#if defined (__GNUC__)
#pragma GCC diagnostic pop
#endif

    run_command_indir (command, dpath, FALSE);

    g_free (command);
    g_free (dpath);
    g_free (fpath);
}


void GnomeCmdFile::update_info(GnomeVFSFileInfo *file_info)
{
    g_return_if_fail (file_info != nullptr);

    g_free (collate_key);
    gnome_vfs_file_info_unref (this->info);
    gnome_vfs_file_info_ref (file_info);
    this->info = file_info;

    gchar *utf8_name;

    if (!gnome_cmd_data.options.case_sens_sort)
    {
        gchar *s = get_utf8 (file_info->name);
        utf8_name = g_utf8_casefold (s, -1);
        g_free (s);
    }
    else
        utf8_name = get_utf8 (file_info->name);

    collate_key = g_utf8_collate_key_for_filename (utf8_name, -1);
    g_free (utf8_name);
}


gboolean GnomeCmdFile::is_local()
{
    return gnome_cmd_dir_is_local (::get_parent_dir (this));
}


gboolean GnomeCmdFile::is_executable()
{
    if (GetGfileAttributeUInt32(G_FILE_ATTRIBUTE_STANDARD_TYPE) != G_FILE_TYPE_REGULAR)
        return FALSE;

    if (!is_local())
        return FALSE;

    if (gcmd_owner.uid() == info->uid && info->permissions & GNOME_CMD_PERM_USER_EXEC)
        return TRUE;

    if (gcmd_owner.gid() == info->gid && info->permissions & GNOME_CMD_PERM_GROUP_EXEC)
        return TRUE;

    if (info->permissions & GNOME_CMD_PERM_OTHER_EXEC)
        return TRUE;

    return FALSE;
}


void GnomeCmdFile::is_deleted()
{
    if (has_parent_dir (this))
    {
        gchar *uri_str = get_uri_str();
        gnome_cmd_dir_file_deleted (::get_parent_dir (this), uri_str);
        g_free (uri_str);
    }
}


void GnomeCmdFile::execute()
{
    gchar *fpath = get_real_path();
    gchar *dpath = g_path_get_dirname (fpath);
    gchar *cmd = g_strdup_printf ("./%s", this->get_quoted_name());

    run_command_indir (cmd, dpath, app_needs_terminal (this));

    g_free (fpath);
    g_free (dpath);
    g_free (cmd);
}


/******************************************************************************
*
*   Function: gnome_cmd_file_list_copy
*
*   Purpose: Refs all files in the passed list and return a copy of that list
*
*   Params:
*
*   Returns: A copy of the passed list with all files ref'ed
*
*   Statuses:
*
******************************************************************************/

GList *gnome_cmd_file_list_copy (GList *files)
{
    g_return_val_if_fail (files != nullptr, nullptr);

    gnome_cmd_file_list_ref (files);
    return g_list_copy (files);
}


/******************************************************************************
*
*   Function: gnome_cmd_file_list_free
*
*   Purpose: Unrefs all files in the passed list and then frees the list
*
*   Params:
*
*   Returns:
*
*   Statuses:
*
******************************************************************************/

void gnome_cmd_file_list_free (GList *files)
{
    if (!files) return;

    gnome_cmd_file_list_unref (files);
    g_list_free (files);
}


GnomeCmdDir *GnomeCmdFile::get_parent_dir()
{
    return ::get_parent_dir (this);
}


inline gulong tv2ms (const GTimeVal &t)
{
    return t.tv_sec*1000 + t.tv_usec/1000;
}


gboolean GnomeCmdFile::needs_update()
{
    GTimeVal t;

    g_get_current_time (&t);

    if (tv2ms (t) - tv2ms (priv->last_update) > gnome_cmd_data.gui_update_rate)
    {
        priv->last_update = t;
        return TRUE;
    }

    return FALSE;
}


void GnomeCmdFile::invalidate_tree_size()
{
    priv->tree_size = -1;
}


gboolean GnomeCmdFile::has_tree_size()
{
    return priv->tree_size != (GnomeVFSFileSize)-1;
}
