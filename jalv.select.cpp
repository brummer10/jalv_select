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

#include <gtkmm.h>
#include <fstream>

#include <lilv/lilv.h>
#include "lv2/lv2plug.in/ns/ext/presets/presets.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"





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
    
    int write_state_to_file(Glib::ustring state);
    void on_preset_selected(Gtk::Menu *presetMenu, Glib::ustring id, Gtk::TreeModel::iterator iter, LilvWorld* world);
    void on_preset_default(Gtk::Menu *presetMenu, Glib::ustring id);
    void create_preset_menu(Glib::ustring id, LilvWorld* world);
    
    static char** uris;
    static size_t n_uris;
    static const char* unmap_uri(LV2_URID_Map_Handle handle, LV2_URID urid);
    static LV2_URID map_uri(LV2_URID_Map_Handle handle, const char* uri);

    public:
    Glib::ustring interpret;

    void create_preset_list(Glib::ustring id, const LilvPlugin* plug, LilvWorld* world);
    
    PresetList() {
        presetStore = Gtk::ListStore::create(psets);
        presetStore->set_sort_column(psets.col_label, Gtk::SORT_ASCENDING); 
    }

    ~PresetList() { 
        free(uris);
    }

};

char** PresetList::uris = NULL;
size_t PresetList::n_uris = 0;

LV2_URID PresetList::map_uri(LV2_URID_Map_Handle handle, const char* uri) {
    for (size_t i = 0; i < n_uris; ++i) {
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

int PresetList::write_state_to_file(Glib::ustring state) {
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
   
    //Glib::ustring pre = row.get_value(psets.col_uri);
    //Glib::ustring com = interpret + " -p " + pre + id;
    Glib::ustring com = interpret + " -l " + "/tmp/state.ttl" + id;
    if (system(NULL)) system( com.c_str());
    presetStore->clear();
    delete presetMenu;
}

void PresetList::on_preset_default(Gtk::Menu *presetMenu, Glib::ustring id) {
   Glib::ustring com = interpret + id;
   if (system(NULL)) system( com.c_str());
   presetStore->clear();
   delete presetMenu;
}

void PresetList::create_preset_menu(Glib::ustring id, LilvWorld* world) {
    Gtk::MenuItem* item;
    Gtk::Menu *presetMenu = Gtk::manage(new Gtk::Menu());
    item = Gtk::manage(new Gtk::MenuItem("Default", true));
    item->signal_activate().connect(
          sigc::bind(sigc::bind(sigc::mem_fun(
          *this, &PresetList::on_preset_default),id),presetMenu));
    presetMenu->append(*item);
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

class LV2PluginList : public Gtk::Window {

    class PlugInfo : public Gtk::TreeModel::ColumnRecord {
    public:
        PlugInfo() {
            add(col_id);
            add(col_name);
            add(col_tip);
            add(col_plug);
        }
        ~PlugInfo() {}
   
        Gtk::TreeModelColumn<Glib::ustring> col_id;
        Gtk::TreeModelColumn<Glib::ustring> col_name;
        Gtk::TreeModelColumn<Glib::ustring> col_tip;
        Gtk::TreeModelColumn<const LilvPlugin*> col_plug;
    };
    PlugInfo pinfo;
    
    std::vector<Glib::ustring> cats;
    Gtk::VBox topBox;
    Gtk::HBox buttonBox;
    Gtk::ComboBoxText comboBox;
    Gtk::ScrolledWindow scrollWindow;
    Gtk::Button buttonQuit;
    Gtk::Button newList;
    Gtk::ComboBoxEntryText textEntry;
    Gtk::TreeView treeView;
    Gtk::TreeModel::Row row ;

    PresetList pstore;
    Glib::RefPtr<Gtk::ListStore> listStore;
    Glib::RefPtr<Gtk::TreeView::Selection> selection;
    Glib::ustring regex;
    bool new_world;

    LilvWorld* world;
    const LilvPlugins* lv2_plugins;
    LV2_URID_Map map;
    LV2_Feature map_feature;
    
    void get_interpreter();
    void fill_list();
    void refill_list();
    void new_list();
    void fill_class_list();
    virtual void on_selection_changed();
    virtual void on_combo_changed();
    virtual void on_entry_changed();
    virtual void on_button_quit();

    public:
    LV2PluginList() :
        buttonQuit("Quit"),
        newList("Refresh"),
        new_world(false) {
        set_title("LV2 plugs");
        set_default_size(350,200);
        get_interpreter();

        treeView.set_model(listStore = Gtk::ListStore::create(pinfo));
        treeView.append_column("Name", pinfo.col_name);
        treeView.set_tooltip_column(2);
        treeView.set_rules_hint(true);
        fill_list();
        fill_class_list();

        scrollWindow.add(treeView);
        scrollWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
        topBox.pack_start(scrollWindow);
        topBox.pack_end(buttonBox,Gtk::PACK_SHRINK);
        buttonBox.pack_start(comboBox,Gtk::PACK_SHRINK);
        buttonBox.pack_start(textEntry,Gtk::PACK_EXPAND_WIDGET);
        buttonBox.pack_start(newList,Gtk::PACK_SHRINK);
        buttonBox.pack_start(buttonQuit,Gtk::PACK_SHRINK);
        add(topBox);

        selection = treeView.get_selection();
        selection->signal_changed().connect( sigc::mem_fun(*this, &LV2PluginList::on_selection_changed) );
        buttonQuit.signal_clicked().connect( sigc::mem_fun(*this, &LV2PluginList::on_button_quit));
        newList.signal_clicked().connect( sigc::mem_fun(*this, &LV2PluginList::new_list));
        comboBox.signal_changed().connect( sigc::mem_fun(*this, &LV2PluginList::on_combo_changed));
        textEntry.signal_changed().connect( sigc::mem_fun(*this, &LV2PluginList::on_entry_changed));
        show_all_children();
        selection->unselect_all();
        textEntry.grab_focus();
    }
    ~LV2PluginList() {
        lilv_world_free(world);
    }
};

void LV2PluginList::get_interpreter() {
    if (system(NULL) )
        system("echo $PATH | tr ':' '\n' | xargs ls  | grep jalv | gawk '{if ($0 == \"jalv\") {print \"jalv -s\"} else {print $0}}' >/tmp/jalv.interpreter" );
    std::ifstream input( "/tmp/jalv.interpreter" );
    int s = 0;
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

void LV2PluginList::fill_list() {
    Glib::ustring tip;
    Glib::ustring tipby = " \nby ";
    world = lilv_world_new();
    lilv_world_load_all(world);
    lv2_plugins = lilv_world_get_all_plugins(world);        
    LilvNode* nd = NULL;
    LilvIter* it = lilv_plugins_begin(lv2_plugins);

    for (it; !lilv_plugins_is_end(lv2_plugins, it);
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
        } else {
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
}

void LV2PluginList::refill_list() {
    Glib::ustring name;
    Glib::ustring name_search;
    Glib::ustring tip;
    Glib::ustring tipby = " \nby ";
    LilvNode* nd;
    LilvIter* it = lilv_plugins_begin(lv2_plugins);
    for (it; !lilv_plugins_is_end(lv2_plugins, it);
    it = lilv_plugins_next(lv2_plugins, it)) {
        const LilvPlugin* plug = lilv_plugins_get(lv2_plugins, it);
        if (plug) {
            nd = lilv_plugin_get_name(plug);
        }
        if (nd) {
            name = lilv_node_as_string(nd);
            const LilvPluginClass* cls = lilv_plugin_get_class(plug);
            tip = lilv_node_as_string(lilv_plugin_class_get_label(cls));
            name_search = name+tip;
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
    } else {
        new_world = false;
    }
}

void LV2PluginList::on_combo_changed() {
    pstore.interpret = comboBox.get_active_text();
}

void LV2PluginList::on_selection_changed() {
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if(iter) {  
        Gtk::TreeModel::Row row = *iter;
        const LilvPlugin* plug = row[pinfo.col_plug];
        Glib::ustring id = " " + row[pinfo.col_id] + " & " ;
        pstore.create_preset_list( id, plug, world);
        selection->unselect(*iter);
    }
}

void LV2PluginList::on_button_quit() {
    Gtk::Main::quit();
}

int main (int argc , char ** argv) {
    Gtk::Main kit (argc, argv);
    LV2PluginList lv2plugs;
    Gtk::Main::run (lv2plugs);
    return 0;
}
