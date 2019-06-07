/*
 * This is free and unencumbered software released into the public domain.

 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.

 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef JALV_SELECT_H_
#define JALV_SELECT_H_

#include <gtkmm.h>
#include <fcntl.h>
#include <fstream>

#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <unistd.h> 

#include <sys/types.h>
#include <sys/stat.h>

#include <libintl.h>
#include <locale.h>

#include <lilv/lilv.h>
#include "lv2/lv2plug.in/ns/ext/presets/presets.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"

#include "config.h"

#define _(S) gettext(S)

namespace jalv_select {

///*** ----------- Class PresetList definition ----------- ***///

class PresetList {

    class Presets : public Gtk::TreeModel::ColumnRecord {
    public:
        Presets() {
            add(col_label);
            add(col_uri);
            add(col_plug);
        }
        ~Presets() {}
   
        Gtk::TreeModelColumn<Glib::ustring> col_label;
        Gtk::TreeModelColumn<Glib::ustring> col_uri;
        Gtk::TreeModelColumn<const LilvPlugin*> col_plug;
       
    };
    Presets psets;

    Glib::RefPtr<Gtk::ListStore> presetStore;
    Gtk::TreeModel::Row row ;
    
    int32_t write_state_to_file(Glib::ustring state);
    void on_preset_selected(Gtk::Menu *presetMenu, Glib::ustring id, Gtk::TreeModel::iterator iter, LilvWorld* world);
    void on_preset_default(Gtk::Menu *presetMenu, Glib::ustring id);
    void create_preset_menu(Glib::ustring id, LilvWorld* world);
    void on_preset_key(GdkEventKey *ev,Gtk::Menu *presetMenu);

    static char** uris;
    static off_t n_uris;
    static const char* unmap_uri(LV2_URID_Map_Handle handle, LV2_URID urid);
    static LV2_URID map_uri(LV2_URID_Map_Handle handle, const char* uri);

    public:
    Glib::ustring interpret;
    Glib::RefPtr<Gtk::TreeView::Selection> selection;

    void create_preset_list(Glib::ustring id, const LilvPlugin* plug, LilvWorld* world);
    
    PresetList();

    ~PresetList();

};


class LV2PluginList; // forward declaration 

///*** ----------- Class Options definition ----------- ***///

class Options : public Glib::OptionContext {
    Glib::OptionGroup o_group;
    Glib::OptionEntry opt_hide;
    Glib::OptionEntry opt_size;
    Glib::OptionEntry opt_version;
public:
    bool hidden;
    bool version;
    int32_t w_high;

    void show_version_and_exit(LV2PluginList *p);

    Options();

    ~Options();

};


///*** ----------- Singleton Class KeyGrabber definition ----------- ***///

class KeyGrabber {
private:
    Display* dpy;
    XEvent ev;
    uint32_t modifiers;
    int32_t keycode;
    pthread_t m_pthr;
    bool err;
    void stop_keygrab_thread();
    void start_keygrab_thread();
    void keygrab();

    static void *run_keygrab_thread(void* p);
    static int32_t my_XErrorHandler(Display * d, XErrorEvent * e);

    KeyGrabber();

    ~KeyGrabber();

public:
    LV2PluginList *runner;
    static KeyGrabber *get_instance();
};


///*** ----------- Singleton Class FiFoChannel definition ----------- ***///

class FiFoChannel {
private:
    int32_t read_fd;
    Glib::ustring fifo_name;
    Glib::RefPtr<Glib::IOChannel> iochannel;
    sigc::connection connect_io;
    int32_t open_fifo();
    void close_fifo();
    void re_connect_fifo();

    static bool read_fifo(Glib::IOCondition io_condition);

    FiFoChannel();

    ~FiFoChannel();

public:
    bool is_mine;
    Glib::ustring own_pid;
    LV2PluginList *runner;
    void write_fifo(Glib::IOCondition io_condition,Glib::ustring buf);
    static FiFoChannel *get_instance();
};


///*** ----------- Class LV2PluginList definition ----------- ***///

class LV2PluginList : public Gtk::Window {

    class PlugInfo : public Gtk::TreeModel::ColumnRecord {
    public:
        PlugInfo() {
            add(col_id);
            add(col_name);
            add(col_tip);
            add(col_plug);
            add(col_fav);
            add(col_bl);
        }
        ~PlugInfo() {}
   
        Gtk::TreeModelColumn<Glib::ustring> col_id;
        Gtk::TreeModelColumn<Glib::ustring> col_name;
        Gtk::TreeModelColumn<Glib::ustring> col_tip;
        Gtk::TreeModelColumn<const LilvPlugin*> col_plug;
        Gtk::TreeModelColumn<bool> col_fav;
        Gtk::TreeModelColumn<bool> col_bl;
    };
    PlugInfo pinfo;

    std::vector<Glib::ustring> cats;
    std::vector<Glib::ustring> favs;
    std::vector<Glib::ustring> bls;
    Gtk::VBox topBox;
    Gtk::HBox buttonBox;
    Gtk::ComboBoxText comboBox;
    Gtk::ScrolledWindow scrollWindow;
    Gtk::Button buttonQuit;
    Gtk::ToggleButton fav;
    Gtk::ToggleButton bl;
    Gtk::Button newList;
    Gtk::ComboBoxText textEntry;
    Gtk::TreeView treeView;
    Gtk::TreeModel::Row row;
    Gtk::Menu MenuPopup;
    Gtk::MenuItem menuQuit;
    int32_t mainwin_x;
    int32_t mainwin_y;
    int32_t valid_plugs;
    int32_t invalid_plugs;

    PresetList pstore;
    GtkIconFactory *factory;
    Glib::ustring tool_tip;
    Glib::RefPtr<Gtk::StatusIcon> status_icon;
    Glib::RefPtr<Gtk::ListStore> listStore;
    Glib::RefPtr<Gtk::ListStore> favStore;
    Glib::RefPtr<Gtk::TreeView::Selection> selection;
    Glib::ustring regex;
    bool new_world;

    LilvWorld* world;
    const LilvPlugins* lv2_plugins;
    LV2_URID_Map map;
    LV2_Feature map_feature;
    KeyGrabber *kg;
    FiFoChannel *fc;

    void get_interpreter();
    void fill_list();
    void refill_list();
    void new_list();
    void fill_class_list();
    void systray_menu(guint button, guint32 activate_time);
    void show_preset_menu();
    void copy_to_clipboard();
    void take_focus();
    void button_release_event(GdkEventButton *ev);
    bool key_release_event(GdkEventKey *ev);
    void on_fav_toggle(Glib::ustring path);
    void read_fav_list();
    bool is_fav(Glib::ustring id);
    bool fav_changed;
    void save_fav_list();
    Glib::ustring config_file;
    Glib::ustring sys_config_file;
    void on_bl_toggle(Glib::ustring path);
    void read_bl_list();
    bool is_bl(Glib::ustring id);
    bool bl_changed;
    void save_bl_list();
    Glib::ustring backlist_file;
    Glib::ustring sys_backlist_file;

    inline void fill_tooltip(Glib::ustring *tip, const LilvPlugin* plug);
    inline void truncate_name(Glib::ustring *name);

    virtual void on_combo_changed();
    virtual void on_entry_changed();
    virtual void on_fav_button();
    virtual void on_bl_button();
    virtual void on_button_quit();

public:
    Options options;
    void systray_hide();
    void come_up();
    void go_down();

    LV2PluginList();

    ~LV2PluginList();

};


} // end namespace jalv_select

#endif // JALV_SELECT_H_

