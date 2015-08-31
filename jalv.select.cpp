#include <gtkmm.h>

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
    
    void on_preset_selected(Gtk::Menu *presetMenu, Glib::ustring id, Gtk::TreeModel::iterator iter, LilvWorld* world);
    void on_preset_default(Gtk::Menu *presetMenu, Glib::ustring id);
    void create_preset_menu(Glib::ustring id, LilvWorld* world);

    public:
    Glib::ustring interpret;

    void create_preset_list(Glib::ustring id, const LilvPlugin* plug, LilvWorld* world);
    
    PresetList() {
        presetStore = Gtk::ListStore::create(psets);
        presetStore->set_sort_column(psets.col_label, Gtk::SORT_ASCENDING); 
    }

    ~PresetList() {}

};

void PresetList::on_preset_selected(Gtk::Menu *presetMenu, Glib::ustring id, Gtk::TreeModel::iterator iter, LilvWorld* world) {
    Gtk::TreeModel::Row row = *iter;
    Glib::ustring pre = row.get_value(psets.col_uri);
    Glib::ustring com = interpret + " -p " + pre + id;
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

    Gtk::VBox topBox;
    Gtk::HBox buttonBox;
    Gtk::ComboBoxText comboBox;
    Gtk::ScrolledWindow scrollWindow;
    Gtk::Button buttonQuit;
    Gtk::Button newList;
    Gtk::Entry textEntry;
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
    
    void fill_list();
    void refill_list();
    void new_list();
    virtual void on_selection_changed();
    virtual void on_combo_changed();
    virtual void on_entry_changed();
    virtual void on_button_quit();

    public:
    LV2PluginList() :
        buttonQuit("Quit"),
        newList("New"),
        new_world(false) {
        set_title("LV2 plugs");
        set_default_size(350,200);
        comboBox.append("jalv.gtk ");
        comboBox.append("jalv.gtkmm ");
        comboBox.append("jalv.gtk3 ");
        comboBox.append("jalv.qt ");
        //comboBox.append("jalv.extui ");
        comboBox.set_active(0);
        pstore.interpret = "jalv.gtk ";

        treeView.set_model(listStore = Gtk::ListStore::create(pinfo));
        treeView.append_column("Name", pinfo.col_name);
        treeView.set_tooltip_column(2);
        treeView.set_rules_hint(true);
        fill_list();

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
    }
    ~LV2PluginList() {
        lilv_world_free(world);
    }
};

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
        row = *(listStore->append());
        const LilvPlugin* plug = lilv_plugins_get(lv2_plugins, it);
        if (plug) {
            nd = lilv_plugin_get_name(plug);
        }
        if (nd) {
            row[pinfo.col_name] = lilv_node_as_string(nd);
            row[pinfo.col_plug] = plug;
            row[pinfo.col_id] = lilv_node_as_string(lilv_plugin_get_uri(plug));
        } else {
            continue;
        }
        lilv_node_free(nd);
        const LilvPluginClass* cls = lilv_plugin_get_class(plug);
        tip = lilv_node_as_string(lilv_plugin_class_get_label(cls));
        
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
        } else {
            continue;
        }
        lilv_node_free(nd);
        Glib::ustring::size_type found = name.lowercase().find(regex.lowercase());
        if (found!=Glib::ustring::npos){
            row = *(listStore->append());
            row[pinfo.col_plug] = plug;
            row[pinfo.col_id] = lilv_node_as_string(lilv_plugin_get_uri(plug));
            row[pinfo.col_name] = name;
            const LilvPluginClass* cls = lilv_plugin_get_class(plug);
            tip = lilv_node_as_string(lilv_plugin_class_get_label(cls));
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
    textEntry.set_text("");
    fill_list();
}

void LV2PluginList::on_entry_changed() {
    if(! new_world) {
        regex = textEntry.get_text();
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
