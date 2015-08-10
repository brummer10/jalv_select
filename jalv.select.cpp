#include <gtkmm.h>
#include <lilv/lilv.h>

class LV2PluginList : public Gtk::Window {

    class Columns : public Gtk::TreeModel::ColumnRecord {
    public:
        Columns() {
            add(col_id);
            add(col_name);
        }
        ~Columns() {}
   
        Gtk::TreeModelColumn<Glib::ustring> col_id;
        Gtk::TreeModelColumn<Glib::ustring> col_name;
    };
    Columns cols;

    Gtk::VBox topBox;
    Gtk::HBox buttonBox;
    Gtk::ComboBoxEntryText comboBox;
    Gtk::ScrolledWindow scrollWindow;
    Gtk::Button buttonQuit;
    Gtk::Button newList;
    Gtk::Entry textEntry;
    Gtk::TreeView treeView;
    Gtk::TreeModel::Row row ;

    Glib::RefPtr<Gtk::ListStore> listStore;
    Glib::RefPtr<Gtk::TreeView::Selection> selection;
    Glib::ustring interpret;
    Glib::ustring regex;
    bool new_world;

    LilvWorld* world;
    const LilvPlugins* lv2_plugins;
    
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
        comboBox.append_text("jalv.gtk ");
        comboBox.append_text("jalv.gtkmm ");
        comboBox.append_text("jalv.gtk3 ");
        comboBox.append_text("jalv.qt ");
        comboBox.append_text("jalv ");
        comboBox.set_active(0);
        interpret = "jalv.gtk ";

        treeView.set_model(listStore = Gtk::ListStore::create(cols));
        treeView.append_column("Name", cols.col_name);
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
        comboBox.signal_changed().connect(sigc::mem_fun(*this, &LV2PluginList::on_combo_changed));
        textEntry.signal_changed().connect(sigc::mem_fun(*this, &LV2PluginList::on_entry_changed));
        show_all_children();
    }
    ~LV2PluginList() {
        lilv_world_free(world);        
    }
};

void LV2PluginList::fill_list()
{
    world = lilv_world_new();
    lilv_world_load_all(world);
    lv2_plugins = lilv_world_get_all_plugins(world);        
    LilvIter* it = lilv_plugins_begin(lv2_plugins);

    for (it; !lilv_plugins_is_end(lv2_plugins, it);
    it = lilv_plugins_next(lv2_plugins, it)) {
        row = *(listStore->append());
        const LilvPlugin* plug = lilv_plugins_get(lv2_plugins, it);
        row[cols.col_id] = lilv_node_as_string(lilv_plugin_get_uri(plug));
        row[cols.col_name] = lilv_node_as_string(lilv_plugin_get_name(plug));
    }
}

void LV2PluginList::refill_list()
{
    Glib::ustring name;
    LilvIter* it = lilv_plugins_begin(lv2_plugins);
    for (it; !lilv_plugins_is_end(lv2_plugins, it);
    it = lilv_plugins_next(lv2_plugins, it)) {
        const LilvPlugin* plug = lilv_plugins_get(lv2_plugins, it);
        name = lilv_node_as_string(lilv_plugin_get_name(plug));
        Glib::ustring::size_type found = name.lowercase().find(regex.lowercase());
        if (found!=Glib::ustring::npos){
            row = *(listStore->append());
            row[cols.col_id] = lilv_node_as_string(lilv_plugin_get_uri(plug));
            row[cols.col_name] = name;
	    }
    }
}

void LV2PluginList::new_list()
{
    new_world = true;
    listStore->clear();
    lilv_world_free(world);
    world = NULL;
    textEntry.set_text("");
    fill_list();
}

void LV2PluginList::on_entry_changed()
{
    if(! new_world) {
    regex = textEntry.get_text();
    listStore->clear();
    refill_list();
    } else {
        new_world = false;
    }
}

void LV2PluginList::on_combo_changed()
{
    interpret = comboBox.get_entry()->get_text();
}

void LV2PluginList::on_selection_changed()
{
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if(iter) {  
        Gtk::TreeModel::Row row = *iter;
        Glib::ustring n = row[cols.col_name];
        Glib::ustring id = interpret;
        id += row[cols.col_id];
        id += " & " ;
        if (system(NULL)) system( id.c_str());
        selection->unselect(*iter);
    }       
}

void LV2PluginList::on_button_quit()
{
    Gtk::Main::quit();
}

int main (int argc , char ** argv) {
    Gtk::Main kit (argc, argv);
    LV2PluginList lv2plugs;
    Gtk::Main::run (lv2plugs);
    return 0;
}
