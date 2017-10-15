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

#include "jalv.select.h"

namespace jalv_select {


template <class T>
inline std::string to_string(const T& t) {
    std::stringstream ss;
    ss << t;
    return ss.str();
}


///*** ----------- Class Options functions ----------- ***///

Options::Options() :
    o_group("",""),
    hidden(false),
    version(false),
    w_high(0) {
        opt_hide.set_short_name('s');
        opt_hide.set_long_name("systray");
        opt_hide.set_description("start minimized in systray");

        opt_size.set_short_name('H');
        opt_size.set_long_name("high");
        opt_size.set_description("start with given high in pixel");
        opt_size.set_arg_description("HIGH");

        opt_version.set_short_name('v');
        opt_version.set_long_name("version");
        opt_version.set_description("print version string and exit");

        o_group.add_entry(opt_hide, hidden);
        o_group.add_entry(opt_size, w_high);
        o_group.add_entry(opt_version, version);
        set_main_group(o_group);
    }

Options::~Options() {}

void Options::show_version_and_exit(LV2PluginList *p) {
    fprintf(stderr, "jalv.select version \033[1;32m %s \n \033[0m" 
      "    Public Domain @ 2016 by Hermman Meyer\n", VERSION );
    p->hide();
    Glib::signal_idle().connect_once(
      sigc::ptr_fun ( Gtk::Main::quit));
}


///*** ----------- Class PresetList functions ----------- ***///

PresetList::PresetList() {
    presetStore = Gtk::ListStore::create(psets);
    presetStore->set_sort_column(psets.col_label, Gtk::SORT_ASCENDING); 
}

PresetList::~PresetList() { 
    free(uris);
}

char** PresetList::uris = NULL;
off_t PresetList::n_uris = 0;

LV2_URID PresetList::map_uri(LV2_URID_Map_Handle handle, const char* uri) {
    for (off_t i = 0; i < n_uris; ++i) {
        if (!strcmp(uris[i], uri)) {
            return i + 1;
        }
    }

    uris = (char**)realloc(uris, ++n_uris * sizeof(char*));
    uris[n_uris - 1] = const_cast<char*>(uri);
    return n_uris;
}

const char* PresetList::unmap_uri(LV2_URID_Map_Handle handle, LV2_URID urid) {
    if (urid > 0 && urid <= n_uris) {
        return uris[urid - 1];
    }
    return NULL;
}

int32_t PresetList::write_state_to_file(Glib::ustring state) {
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path("/tmp/state.ttl");
    if (file) {
        Glib::ustring prefix = "@prefix atom: <http://lv2plug.in/ns/ext/atom#> .\n"
                               "@prefix lv2: <http://lv2plug.in/ns/lv2core#> .\n"
                               "@prefix pset: <http://lv2plug.in/ns/ext/presets#> .\n"
                               "@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
                               "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
                               "@prefix state: <http://lv2plug.in/ns/ext/state#> .\n"
                               "@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .\n";
        prefix +=   state;
        Glib::RefPtr<Gio::DataOutputStream> out = Gio::DataOutputStream::create(file->replace());
        out->put_string(prefix);
        return 1;
    }
    return 0;

}

void PresetList::on_preset_selected(Gtk::Menu *presetMenu, Glib::ustring id, Gtk::TreeModel::iterator iter, LilvWorld* world) {
    Gtk::TreeModel::Row row = *iter;
    LV2_URID_Map       map           = { NULL, map_uri };
    LV2_URID_Unmap     unmap         = { NULL, unmap_uri };

    LilvNode* preset = lilv_new_uri(world, row.get_value(psets.col_uri).c_str());

    LilvState* state = lilv_state_new_from_world(world, &map, preset);
    lilv_state_set_label(state, row.get_value(psets.col_label).c_str());
    Glib::ustring st = lilv_state_to_string(world,&map,&unmap,state,row.get_value(psets.col_uri).c_str(),NULL);
    lilv_node_free(preset);
    lilv_state_free(state);
    st.replace(st.find_first_of("<"),st.find_first_of(">"),"<");
    if(!write_state_to_file(st)) return;
   
    Gtk::TreeModel::iterator it = selection->get_selected();
    if(iter) selection->unselect(*it);
    //Glib::ustring pre = row.get_value(psets.col_uri);
    //Glib::ustring com = interpret + " -p " + pre + id;
    Glib::ustring com = interpret + " -l " + "/tmp/state.ttl" + id;
    if (system(NULL)) system( com.c_str());
    presetStore->clear();
    delete presetMenu;
}

void PresetList::on_preset_default(Gtk::Menu *presetMenu, Glib::ustring id) {
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if(iter) selection->unselect(*iter);
    Glib::ustring com = interpret + id;
    if (system(NULL)) system( com.c_str());
    presetStore->clear();
    delete presetMenu;
}

void PresetList::on_preset_key(GdkEventKey *ev,Gtk::Menu *presetMenu) {
    if (ev->keyval == 0xff1b) { // GDK_KEY_Escape
        presetStore->clear();
        delete presetMenu;
    }
}

void PresetList::create_preset_menu(Glib::ustring id, LilvWorld* world) {
    Gtk::MenuItem* item;
    Gtk::Menu *presetMenu = Gtk::manage(new Gtk::Menu());
    presetMenu->signal_key_release_event().connect_notify(
          sigc::bind(sigc::mem_fun(
          *this, &PresetList::on_preset_key),presetMenu));
    item = Gtk::manage(new Gtk::MenuItem("Default", true));
    item->signal_activate().connect_notify(
          sigc::bind(sigc::bind(sigc::mem_fun(
          *this, &PresetList::on_preset_default),id),presetMenu));
    presetMenu->append(*item);
    if (presetStore->children().end() != presetStore->children().begin()) {
        Gtk::SeparatorMenuItem *hline = Gtk::manage(new Gtk::SeparatorMenuItem());
        presetMenu->append(*hline);
        for (Gtk::TreeModel::iterator i = presetStore->children().begin();
                                  i != presetStore->children().end(); i++) {
            Gtk::TreeModel::Row row = *i; 
            item = Gtk::manage(new Gtk::MenuItem(row[psets.col_label], true));
            item->signal_activate().connect(
              sigc::bind(sigc::bind(sigc::bind(sigc::bind(sigc::mem_fun(
              *this, &PresetList::on_preset_selected),world),i),id),presetMenu));
            presetMenu->append(*item);
            
        }
    }
    presetMenu->show_all();
    presetMenu->popup(0,gtk_get_current_event_time());
}

void PresetList::create_preset_list(Glib::ustring id, const LilvPlugin* plug, LilvWorld* world) {
    LilvNodes* presets = lilv_plugin_get_related(plug,
      lilv_new_uri(world,LV2_PRESETS__Preset));
    presetStore->clear();
    LILV_FOREACH(nodes, i, presets) {
        const LilvNode* preset = lilv_nodes_get(presets, i);
        lilv_world_load_resource(world, preset);
        LilvNodes* labels = lilv_world_find_nodes(
          world, preset, lilv_new_uri(world, LILV_NS_RDFS "label"), NULL);
        if (labels) {
            const LilvNode* label = lilv_nodes_get_first(labels);
            Glib::ustring set =  lilv_node_as_string(label);
            row = *(presetStore->append());
            row[psets.col_label]=set;
            row[psets.col_uri] = lilv_node_as_uri(preset);
            row[psets.col_plug] = plug;
            lilv_nodes_free(labels);
        } else {
            fprintf(stderr, "Preset <%s> has no rdfs:label\n",
                    lilv_node_as_string(lilv_nodes_get(presets, i)));
        }
    }
    lilv_nodes_free(presets);
    create_preset_menu(id, world);
}


///*** ----------- Class KeyGrabber functions ----------- ***///

KeyGrabber::KeyGrabber()  {
    start_keygrab_thread();
}

KeyGrabber::~KeyGrabber() {
    stop_keygrab_thread();
}

KeyGrabber*  KeyGrabber::get_instance() {
    static KeyGrabber instance;
    return &instance;
}

int32_t KeyGrabber::my_XErrorHandler(Display * d, XErrorEvent * e) {
    static int32_t count = 0;
    KeyGrabber *kg = KeyGrabber::get_instance();
    if (!count) {
        fprintf(stderr, "X Error: try ControlMask | ShiftMask now \n");
        XUngrabKey(kg->dpy, kg->keycode, kg->modifiers, DefaultRootWindow(kg->dpy));
        kg->modifiers = ControlMask | ShiftMask;
        XGrabKey(kg->dpy, kg->keycode, kg->modifiers, DefaultRootWindow(kg->dpy),
          0, GrabModeAsync, GrabModeAsync);
        count +=1;
    } else {
        char buffer1[1024];
        XGetErrorText(d, e->error_code, buffer1, 1024);
        fprintf(stderr, "X Error:  %s\n Global HotKey disabled\n", buffer1);
        XUngrabKey(kg->dpy, kg->keycode, kg->modifiers, DefaultRootWindow(kg->dpy));
        XFlush(kg->dpy);
        XCloseDisplay(kg->dpy);
        kg->stop_keygrab_thread();
    }
    return 0;
}

void KeyGrabber::keygrab() {
    dpy = XOpenDisplay(0);
    XSetErrorHandler(my_XErrorHandler);
    modifiers =  ShiftMask;
    keycode = XKeysymToKeycode(dpy,XK_Escape);
    XGrabKey(dpy, keycode, modifiers, DefaultRootWindow(dpy),
      0, GrabModeAsync, GrabModeAsync);
    XSync(dpy,true);
    XSelectInput(dpy,DefaultRootWindow(dpy), KeyPressMask);
    while(1) {
        XNextEvent(dpy, &ev);
        if (ev.type == KeyPress)
            Glib::signal_idle().connect_once(
              sigc::mem_fun(static_cast<LV2PluginList *>(runner), 
              &LV2PluginList::systray_hide));
    }
}

void *KeyGrabber::run_keygrab_thread(void *p) {
    KeyGrabber *kg = KeyGrabber::get_instance();
    kg->keygrab();
    return NULL;
}

void KeyGrabber::stop_keygrab_thread() {
    pthread_cancel(m_pthr);
    pthread_join(m_pthr, NULL);
}

void KeyGrabber::start_keygrab_thread() {
    pthread_attr_t      attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE );
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    if (pthread_create(&m_pthr, &attr, run_keygrab_thread, NULL)) {
        err = true;
    }
    pthread_attr_destroy(&attr);
}


///*** ----------- Class LV2PluginList functions ----------- ***///

LV2PluginList::LV2PluginList() :
    buttonQuit("_Quit", true),
    newList("_Refresh", true),
    fav("_Fav.", true),
    textEntry(true),
    mainwin_x(-1),
    mainwin_y(-1),
    valid_plugs(0),
    invalid_plugs(0),
    tool_tip(" "),
    fav_changed(false),
    config_file(Glib::build_filename(Glib::get_user_config_dir(), "jalv.select.conf")),
    //status_icon(Gtk::StatusIcon::create_from_file(PIXMAPS_DIR "/lv2.png")),
    new_world(false) {
    set_title("LV2 plugs");
    set_default_size(350,200);
    get_interpreter();
    fc = FiFoChannel::get_instance();
    fc->runner = this;
    kg = KeyGrabber::get_instance();
    kg->runner = this;

    listStore = Gtk::ListStore::create(pinfo);
    treeView.set_model(listStore);
    treeView.append_column("Name", pinfo.col_name);
    treeView.append_column_editable("Favorit", pinfo.col_fav);
    treeView.get_column(0)->set_min_width(360);
    treeView.get_column(1)->set_max_width(60);
    Gtk::TreeViewColumn *col = treeView.get_column(1);
    Gtk::CellRendererToggle *cell = dynamic_cast<Gtk::CellRendererToggle*>(col->get_first_cell_renderer());
    treeView.set_tooltip_column(2);
    treeView.set_rules_hint(true);
    treeView.set_name("lv2_treeview" );
    listStore->set_sort_column(pinfo.col_name, Gtk::SORT_ASCENDING );
    read_fav_list();
    fill_list();
    fill_class_list();
    Gtk::RC::parse_string(
        "style 'treestyle'{ \n"
           " GtkTreeView::odd-row-color = 'gray94' \n"
           " GtkTreeView::even-row-color = 'gray100' \n"
           " GtkTreeView::allow-rules = 1 \n"
       " } \n"
       " widget '*lv2_treeview*' style 'treestyle' \n");
    scrollWindow.add(treeView);
    scrollWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scrollWindow.get_vscrollbar()->set_can_focus(true);
    topBox.pack_start(scrollWindow);
    topBox.pack_end(buttonBox,Gtk::PACK_SHRINK);
    buttonBox.pack_start(comboBox,Gtk::PACK_SHRINK);
    buttonBox.pack_start(textEntry,Gtk::PACK_EXPAND_WIDGET);
    buttonBox.pack_start(newList,Gtk::PACK_SHRINK);
    buttonBox.pack_start(fav,Gtk::PACK_SHRINK);
    buttonBox.pack_start(buttonQuit,Gtk::PACK_SHRINK);
    add(topBox);

    set_icon_from_file(PIXMAPS_DIR "/lv2_16.png");
    menuQuit.set_label("Quit");
    MenuPopup.append(menuQuit);
    status_icon = Gtk::StatusIcon::create_from_file(PIXMAPS_DIR "/lv2.png");
    status_icon->set_tooltip_text(tool_tip);

    selection = treeView.get_selection();
    pstore.selection = selection;
    cell->signal_toggled().connect(
      sigc::mem_fun(*this, &LV2PluginList::on_fav_toggle));
    treeView.signal_button_release_event().connect_notify(
      sigc::mem_fun(*this, &LV2PluginList::button_release_event));
    treeView.signal_key_release_event().connect(
      sigc::mem_fun(*this, &LV2PluginList::key_release_event));
    buttonQuit.signal_clicked().connect(
      sigc::mem_fun(*this, &LV2PluginList::on_button_quit));
    fav.signal_toggled().connect(
      sigc::mem_fun(*this, &LV2PluginList::on_fav_button));
    newList.signal_clicked().connect(
      sigc::mem_fun(*this, &LV2PluginList::new_list));
    comboBox.signal_changed().connect(
      sigc::mem_fun(*this, &LV2PluginList::on_combo_changed));
    textEntry.signal_changed().connect(
      sigc::mem_fun(*this, &LV2PluginList::on_entry_changed));
    status_icon->signal_activate().connect(
      sigc::mem_fun(*this, &LV2PluginList::systray_hide));
    status_icon->signal_popup_menu().connect(
      sigc::mem_fun(*this, &LV2PluginList::systray_menu));
    menuQuit.signal_activate().connect(
      sigc::mem_fun(*this, &LV2PluginList::on_button_quit));
    show_all();
    take_focus();
}

LV2PluginList::~LV2PluginList() {
    lilv_world_free(world);
}

void LV2PluginList::get_interpreter() {
    if (system(NULL) )
      system("echo $PATH | tr ':' '\n' | xargs ls  | grep jalv | gawk '{if ($0 == \"jalv\") {print \"jalv -s\"} else {print $0}}' >/tmp/jalv.interpreter" );
    std::ifstream input( "/tmp/jalv.interpreter" );
    int32_t s = 0;
    for( std::string line; getline( input, line ); ) {
        if ((line.compare("jalv.select") !=0)){
            comboBox.append(line);
            if ((line.compare("jalv.gtk") ==0))
                comboBox.set_active(s);
                s++;
        }
    }        
    pstore.interpret = comboBox.get_active_text(); 
}

void LV2PluginList::read_fav_list() {
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(config_file);
    if (!file) return;
    if (file->query_exists()) {
        Glib::RefPtr<Gio::DataInputStream> in = Gio::DataInputStream::create(file->read());    
        std::string line;
        while (in->read_line(line)) {
            favs.push_back(line);
        }
        in->close();
    }
}

void LV2PluginList::save_fav_list() {
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(config_file);
    if (!file) return;
    Glib::RefPtr<Gio::DataOutputStream> out = Gio::DataOutputStream::create(file->replace());
    typedef Gtk::TreeModel::Children type_children;
    type_children children = listStore->children();
    Glib::ustring id;
    for (std::vector<Glib::ustring>::iterator it = favs.begin() ; it != favs.end(); ++it) {
        id += (*it);
        id += "\n";
    }
    out->put_string(id);
    out->flush ();
    out->close ();
}

bool LV2PluginList::is_fav(Glib::ustring id) {
    for (std::vector<Glib::ustring>::iterator it = favs.begin() ; it != favs.end(); ++it) {
        if (id.compare(*it)==0) return true;
    }
    return false;
}

void LV2PluginList::on_fav_toggle(Glib::ustring path) {
    if(path.empty()) return;
    auto row = *listStore->get_iter(Gtk::TreeModel::Path(path));
    if(row[pinfo.col_fav] == true) {
        Glib::ustring id = row[pinfo.col_id];
        if (id.empty()) return;
        for (std::vector<Glib::ustring>::iterator it = favs.begin() ; it != favs.end(); ++it) {
            if (id.compare(*it)==0) return;
        }
        favs.push_back(id);
    } else {
        Glib::ustring id = row[pinfo.col_id];
        if (id.empty()) return;
        for (std::vector<Glib::ustring>::iterator it = favs.begin() ; it != favs.end(); ++it) {
            if (id.compare(*it)==0) {
                favs.erase(it);
                break;
            }
        }
    }
    fav_changed = true;
    if (fav.get_active()) on_fav_button();
}

void LV2PluginList::on_fav_button() {
    Glib::ustring name;
    Glib::ustring tip;
    Glib::ustring id;
    bool found = false;
    Glib::ustring tipby = " \nby ";
    LilvNode* nd;
    listStore->clear();
    for (LilvIter* it = lilv_plugins_begin(lv2_plugins);
      !lilv_plugins_is_end(lv2_plugins, it);
      it = lilv_plugins_next(lv2_plugins, it)) {
        found = false;
        const LilvPlugin* plug = lilv_plugins_get(lv2_plugins, it);
        if (plug) {
            nd = lilv_plugin_get_name(plug);
        }
        if (nd) {
            name = lilv_node_as_string(nd);
            const LilvPluginClass* cls = lilv_plugin_get_class(plug);
            tip = lilv_node_as_string(lilv_plugin_class_get_label(cls));
            id = lilv_node_as_string(lilv_plugin_get_uri(plug));
        } else {
           continue;
        }
        lilv_node_free(nd);
        if (fav.get_active()) {
            for (std::vector<Glib::ustring>::iterator it = favs.begin() ; it != favs.end(); ++it) {
                if (id.compare(*it)==0) found = true;
            }
            fav.set_label(" _All ");
        } else {
            found = true;
            fav.set_label("_Fav.");
        }
        if (found){
            row = *(listStore->append());
            row[pinfo.col_plug] = plug;
            row[pinfo.col_id] = lilv_node_as_string(lilv_plugin_get_uri(plug));
            row[pinfo.col_name] = name;
            row[pinfo.col_fav] = is_fav(lilv_node_as_string(lilv_plugin_get_uri(plug)));
            nd = lilv_plugin_get_author_name(plug);
            if (!nd) {
                nd = lilv_plugin_get_project(plug);
            }
            if (nd) {
                tip += tipby + lilv_node_as_string(nd);
            }
            lilv_node_free(nd);
            row[pinfo.col_tip] = tip;
        } 
    }
}

void LV2PluginList::fill_list() {
    valid_plugs = 0;
    invalid_plugs = 0;
    Glib::ustring invalid = "";
    Glib::ustring tip;
    Glib::ustring tipby = " \nby ";
    world = lilv_world_new();
    lilv_world_load_all(world);
    lv2_plugins = lilv_world_get_all_plugins(world);        
    LilvNode* nd = NULL;

    for (LilvIter* it = lilv_plugins_begin(lv2_plugins);
      !lilv_plugins_is_end(lv2_plugins, it);
      it = lilv_plugins_next(lv2_plugins, it)) {
        const LilvPlugin* plug = lilv_plugins_get(lv2_plugins, it);
        if (plug) {
            nd = lilv_plugin_get_name(plug);
        }
        if (nd) {
            row = *(listStore->append());
            row[pinfo.col_name] = lilv_node_as_string(nd);
            row[pinfo.col_plug] = plug;
            row[pinfo.col_id] = lilv_node_as_string(lilv_plugin_get_uri(plug));
            row[pinfo.col_fav] = is_fav(lilv_node_as_string(lilv_plugin_get_uri(plug)));
            valid_plugs++;
        } else {
            invalid += "\n";
            invalid += lilv_node_as_string(lilv_plugin_get_uri(plug));
            invalid_plugs++;
            continue;
        }
        lilv_node_free(nd);
        const LilvPluginClass* cls = lilv_plugin_get_class(plug);
        tip = lilv_node_as_string(lilv_plugin_class_get_label(cls));
        cats.insert(cats.begin(),tip);
        nd = lilv_plugin_get_author_name(plug);
        if (!nd) {
            nd = lilv_plugin_get_project(plug);
        }
        if (nd) {
             tip += tipby + lilv_node_as_string(nd);
        }
        lilv_node_free(nd);
        row[pinfo.col_tip] = tip;
    }
    tool_tip = to_string(valid_plugs)+" valid plugins installed\n";
    tool_tip += to_string(invalid_plugs)+" invalid plugins found";
    tool_tip += invalid;
    if (status_icon) status_icon->set_tooltip_text(tool_tip);
}

void LV2PluginList::refill_list() {
    Glib::ustring name;
    Glib::ustring name_search;
    Glib::ustring tip;
    Glib::ustring tip1;
    Glib::ustring tipby = " \nby ";
    LilvNode* nd;

    for (LilvIter* it = lilv_plugins_begin(lv2_plugins);
      !lilv_plugins_is_end(lv2_plugins, it);
      it = lilv_plugins_next(lv2_plugins, it)) {
        const LilvPlugin* plug = lilv_plugins_get(lv2_plugins, it);
        if (plug) {
            nd = lilv_plugin_get_name(plug);
        }
        if (nd) {
            name = lilv_node_as_string(nd);
            const LilvPluginClass* cls = lilv_plugin_get_class(plug);
            tip = lilv_node_as_string(lilv_plugin_class_get_label(cls));
            tip1 = lilv_node_as_string(lilv_plugin_get_uri(plug));
            name_search = name+tip+tip1;
        } else {
            continue;
        }
        lilv_node_free(nd);
        Glib::ustring::size_type found = name_search.lowercase().find(regex.lowercase());
        if (found!=Glib::ustring::npos){
            row = *(listStore->append());
            row[pinfo.col_plug] = plug;
            row[pinfo.col_id] = lilv_node_as_string(lilv_plugin_get_uri(plug));
            row[pinfo.col_name] = name;
            row[pinfo.col_fav] = is_fav(lilv_node_as_string(lilv_plugin_get_uri(plug)));
            nd = lilv_plugin_get_author_name(plug);
            if (!nd) {
                nd = lilv_plugin_get_project(plug);
            }
            if (nd) {
                tip += tipby + lilv_node_as_string(nd);
            }
            lilv_node_free(nd);
            row[pinfo.col_tip] = tip;
        }
    }
}

void LV2PluginList::new_list() {
    new_world = true;
    listStore->clear();
    lilv_world_free(world);
    world = NULL;
    textEntry.get_entry()->set_text("");
    fill_list();
}

void LV2PluginList::fill_class_list() {
    sort(cats.begin(), cats.end());
    cats.erase( unique(cats.begin(), cats.end()), cats.end());
    for (std::vector<Glib::ustring>::iterator it = cats.begin() ; it != cats.end(); ++it)
        textEntry.append_text(*it);
}

void LV2PluginList::on_entry_changed() {
    if(! new_world) {
        regex = textEntry.get_entry()->get_text();
        listStore->clear();
        refill_list();
        if (fav.get_active()) on_fav_button();
    } else {
        new_world = false;
    }
}

void LV2PluginList::on_combo_changed() {
    pstore.interpret = comboBox.get_active_text();
}

void LV2PluginList::show_preset_menu() {
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if(iter) {  
        Gtk::TreeModel::Row row = *iter;
        const LilvPlugin* plug = row[pinfo.col_plug];
        Glib::ustring id = " " + row[pinfo.col_id] + " & " ;
        pstore.create_preset_list( id, plug, world);
    }
}

void LV2PluginList::copy_to_clipboard() {
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if(iter) {  
        Gtk::TreeModel::Row row = *iter;
        const LilvPlugin* plug = row[pinfo.col_plug];
        Glib::ustring id = row[pinfo.col_id];
        Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get();
        clipboard->set_text(id.c_str());
        clipboard->set_can_store();
    }
}

void LV2PluginList::button_release_event(GdkEventButton *ev) {
    if (ev->type == GDK_BUTTON_RELEASE && ev->button == 1) {
        Gtk::TreeModel::Path path;
        Gtk::TreeViewColumn* col;
        treeView.get_cursor(path, col);
        if ( col == treeView.get_column(0)) show_preset_menu();
    } else if (ev->type == GDK_BUTTON_RELEASE && ev->button == 3) {
        copy_to_clipboard();
    }
}

bool LV2PluginList::key_release_event(GdkEventKey *ev) {
    if (ev->keyval == 0xff0d || ev->keyval == 0x020 ) { // GDK_KEY_Return || GDK_KEY_space
        Gtk::TreeModel::Path path;
        Gtk::TreeViewColumn* col;
        treeView.get_cursor(path, col);
        if ( col == treeView.get_column(0)) show_preset_menu();
    } else if ((ev->state & GDK_CONTROL_MASK) &&
           ((ev->keyval ==  0x063) || (ev->keyval ==  0x043))) { // GDK_KEY_c || GDK_KEY_C
        copy_to_clipboard();
    } else if ((ev->state & GDK_CONTROL_MASK) &&
           ((ev->keyval ==  0x071) || (ev->keyval  ==  0x051))) { // GDK_KEY_q || GDK_KEY_Q
        on_button_quit();
    } else if ((ev->state & GDK_CONTROL_MASK) &&
           ((ev->keyval ==  0x072) || (ev->keyval ==  0x052))) { // GDK_KEY_r || GDK_KEY_R 
        new_list();
    } else if ((ev->state & GDK_CONTROL_MASK) &&
           ((ev->keyval ==  0x077) || (ev->keyval ==  0x057))) { // GDK_KEY_w || GDK_KEY_W 
        systray_hide();
    }
    return true;
}

void LV2PluginList::take_focus() { 
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if(iter) selection->unselect(*iter);
    treeView.grab_focus();
}

void LV2PluginList::systray_menu(guint button, guint32 activate_time) {
    MenuPopup.show_all();
    MenuPopup.popup(2, gtk_get_current_event_time());
}

void LV2PluginList::systray_hide() {
    if (get_window()->get_state()
     & (Gdk::WINDOW_STATE_ICONIFIED|Gdk::WINDOW_STATE_WITHDRAWN)) {
        if(!options.hidden) {
            move(mainwin_x, mainwin_y);
        } else {
            options.hidden = false;
        }
        present();
        take_focus();
    } else {
        get_window()->get_root_origin(mainwin_x, mainwin_y);
        hide();
    }
}

void LV2PluginList::come_up() {
    if (get_window()->get_state()
     & (Gdk::WINDOW_STATE_ICONIFIED|Gdk::WINDOW_STATE_WITHDRAWN)) {
        if ((!options.hidden)&&(mainwin_x + mainwin_y >1))
            move(mainwin_x, mainwin_y);
    } else {
        get_window()->get_root_origin(mainwin_x, mainwin_y);
    }
    present();
    take_focus();
}

void LV2PluginList::go_down() {
    if (get_window()->get_state()
     & (Gdk::WINDOW_STATE_ICONIFIED|Gdk::WINDOW_STATE_WITHDRAWN)) {
        return;
    } else {
        get_window()->get_root_origin(mainwin_x, mainwin_y);
    }
    hide();
}

void LV2PluginList::on_button_quit() {
    if (fav_changed) save_fav_list();
    Gtk::Main::quit();
}


///*** ----------- Class FiFoChannel functions ----------- ***///

FiFoChannel::FiFoChannel() {
    open_fifo();
}

FiFoChannel::~FiFoChannel() {
    close_fifo();
}

FiFoChannel*  FiFoChannel::get_instance() {
    static FiFoChannel instance;
    return &instance;
}

bool FiFoChannel::read_fifo(Glib::IOCondition io_condition)
{
    FiFoChannel *fc = FiFoChannel::get_instance();
    if ((io_condition & Glib::IO_IN) == 0) {
        return false;
    } else {
        Glib::ustring buf;
        fc->iochannel->read_line(buf);
        if (buf.compare("quit\n") == 0){
            Gtk::Main::quit ();
        } else if (buf.compare("exit\n") == 0) {
            fc->is_mine = false;
            fc->runner->hide();
            Glib::signal_idle().connect_once(
              sigc::ptr_fun ( Gtk::Main::quit));
        } else if (buf.compare("show\n") == 0) {
            fc->runner->come_up();
        } else if (buf.compare("hide\n") == 0) {
            fc->runner->go_down();
        } else if (buf.compare("systray action\n") == 0) {
            fc->runner->systray_hide();
        } else if (buf.find("PID: ") != Glib::ustring::npos) {
            fc->own_pid +="\n";
            if(buf.compare(fc->own_pid) != 0) {
                fc->connect_io.disconnect();
                fc->iochannel->write("exit\n");
                fc->iochannel->flush();
                Glib::signal_timeout().connect_once(
                  sigc::mem_fun (fc, &FiFoChannel::re_connect_fifo), 5);
            }
            fc->runner->come_up();
            fc->is_mine = true;
        } else {
            fprintf(stderr,"jalv.select * Unknown Message\n %2s\n", buf.c_str()) ;
        }
    }
    return true;
}

void FiFoChannel::re_connect_fifo() {
    connect_io = Glib::signal_io().connect(
      sigc::ptr_fun(read_fifo), read_fd, Glib::IO_IN);
}

void FiFoChannel::write_fifo(Glib::IOCondition io_condition, Glib::ustring buf)
{
    if ((io_condition & Glib::IO_OUT) == 0) {
        return;
    } else {
        connect_io.disconnect();
        iochannel->write(buf);
        iochannel->write("\n");
        iochannel->flush();
        re_connect_fifo();
    }
}

int32_t FiFoChannel::open_fifo() {
    fifo_name = "/tmp/jalv.select.fifo"+to_string(getuid());
    is_mine = false;
    if (access(fifo_name.c_str(), F_OK) == -1) {
        is_mine = true;
        if (mkfifo(fifo_name.c_str(), 0666) != 0) {
            is_mine = false;
            return -1;
        }
    }
    read_fd = open(fifo_name.c_str(), O_RDWR | O_NONBLOCK);
    if (read_fd == -1) return -1;
    connect_io = Glib::signal_io().connect(
      sigc::ptr_fun(read_fifo), read_fd, Glib::IO_IN);
    iochannel = Glib::IOChannel::create_from_fd(read_fd);
    return 0;
}

void FiFoChannel::close_fifo() {
    if (is_mine) unlink(fifo_name.c_str());
}


} // end namespace jalv_select

// NULL handler for gtk warnings 
static void null_handler(const char *log_domain, GLogLevelFlags log_level,
               const gchar *msg, gpointer user_data ) {
    return ;
}

///*** ----------- main ----------- ***///

int32_t main (int32_t argc , char ** argv) {
    Gtk::Main kit (argc, argv);
    jalv_select::LV2PluginList lv2plugs;

    // disable anoing gtk warnings
    g_log_set_handler("Gtk", G_LOG_LEVEL_WARNING, null_handler, NULL);

    try {
        lv2plugs.options.parse(argc, argv);
    } catch (Glib::OptionError& error) {
        fprintf(stderr,"%s\n",error.what().c_str()) ;
    }
    
    if(lv2plugs.options.hidden) lv2plugs.hide();
    if(lv2plugs.options.w_high) lv2plugs.resize(1, lv2plugs.options.w_high);
    if(lv2plugs.options.version) lv2plugs.options.show_version_and_exit(&lv2plugs);

    jalv_select::FiFoChannel *fc = jalv_select::FiFoChannel::get_instance();
    fc->own_pid = "PID: ";
    fc->own_pid += jalv_select::to_string(getpid());

    if (!fc->is_mine) {
        fc->write_fifo(Glib::IO_OUT,fc->own_pid);
    }

    Gtk::Main::run();
    return 0;
}
