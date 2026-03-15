#pragma once
#include <string>
#include <vector>
#include <map>

#include <gtkmm/box.h>
#include <gtkmm/entry.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/notebook.h>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>

#include "../../model.h"

namespace UI {
    class root;

    struct ItemRecord {
        u16         m_id = 0;
        std::string m_name;
        std::string m_flavor;

        std::string m_itemType;
        std::string m_sellPrice;
        std::string m_effect;
        std::string m_param1;
        std::string m_param2;
        std::string m_param3;

        std::string m_tmhmType;
        std::string m_tmhmNumber;
        std::string m_tmhmMove;

        std::string m_medicineEffect;
        std::string m_medicineParam1;
        std::string m_medicineParam2;
        std::string m_medicineParam3;

        std::string m_formeTargetSpecies;
        std::string m_formeTargetForme;
    };

    class itemCsvAdapter {
      public:
        static std::vector<ItemRecord> load( const std::string& p_fsrootPath );
    };

    class itemDataEditor {
        model& _model;
        root&  _rootWindow;

        Gtk::Box      _mainBox{ Gtk::Orientation::VERTICAL };
        Gtk::Label    _title;
        Gtk::Paned    _splitPane{ Gtk::Orientation::HORIZONTAL };
        Gtk::Notebook _tabs;

        Gtk::Box            _browserBox{ Gtk::Orientation::VERTICAL };
        Gtk::Entry          _search;
        Gtk::ScrolledWindow _listScroll;
        Gtk::ListBox        _recordList;

        Gtk::Grid _baseGrid;
        Gtk::Grid _behaviorGrid;
        Gtk::Grid _textGrid;

        Gtk::Entry _baseNameValue, _baseTypeValue, _baseSellPriceValue;
        Gtk::Entry _baseEffectValue, _baseParam1Value, _baseParam2Value, _baseParam3Value;

        Gtk::Entry _behaviorTmTypeValue, _behaviorTmNumberValue, _behaviorTmMoveValue;
        Gtk::Entry _behaviorMedicineEffectValue, _behaviorMedicineP1Value;
        Gtk::Entry _behaviorMedicineP2Value, _behaviorMedicineP3Value;
        Gtk::Entry _behaviorFormeSpeciesValue, _behaviorFormeFormeValue;

        Gtk::Entry _textNameValue, _textFlavorValue;

        std::vector<ItemRecord> _allRecords;
        std::vector<size_t>     _filteredIndices;
        std::string             _lastDataPath;
        bool                    _dirty = false;
        bool                    _updatingFields = false;
        size_t                  _selectedRecordIndex = size_t( -1 );

        std::vector<std::vector<std::string>> _itemDataRows;
        std::vector<std::vector<std::string>> _itemNameRows;
        std::vector<std::vector<std::string>> _itemFlavorRows;
        std::vector<std::vector<std::string>> _itemTmhmRows;
        std::vector<std::vector<std::string>> _itemMedicineRows;
        std::vector<std::vector<std::string>> _itemFormeRows;
        std::map<u16, size_t>                 _itemDataRowById;
        std::map<u16, size_t>                 _itemNameRowById;
        std::map<u16, size_t>                 _itemFlavorRowById;
        std::map<u16, size_t>                 _itemTmhmRowById;
        std::map<u16, size_t>                 _itemMedicineRowById;
        std::map<u16, size_t>                 _itemFormeRowById;

        static std::string toLower( const std::string& p_text );
        static void        setupGrid( Gtk::Grid& p_grid );
        static void        attachRow( Gtk::Grid& p_grid, int p_row, const std::string& p_label,
                                      Gtk::Entry& p_value );
        static std::vector<std::vector<std::string>> readDelimitedCsv( const std::string& p_path,
                                                                        char p_delim );
        static bool writeDelimitedCsv( const std::string&                            p_path,
                                       const std::vector<std::vector<std::string>>& p_rows,
                                       char p_delim );

        void reloadIfNeeded( );
        void rebuildList( );
        void selectByFilteredPosition( size_t p_pos );
        void showRecord( const ItemRecord& p_record );
        void handleFieldEdited( );
        void loadRowsFromDisk( const std::string& p_dataPath );
        void applyFieldEditsToSelectedRecord( );
        bool writeCurrentRowsToDisk( const std::string& p_dataPath );

      public:
        itemDataEditor( model& p_model, root& p_root );

        inline operator Gtk::Widget&( ) {
            return _mainBox;
        }

        inline void hide( ) {
            _mainBox.hide( );
        }

        inline void show( ) {
            _mainBox.show( );
        }

        inline bool isVisible( ) {
            return _mainBox.is_visible( );
        }

        void redraw( );
        bool saveToCsv( );
        bool isDirty( ) const;
    };
} // namespace UI
