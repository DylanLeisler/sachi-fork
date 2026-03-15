#pragma once
#include <string>
#include <map>
#include <vector>

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

    struct MoveRecord {
        u16         m_id = 0;
        std::string m_displayName;
        std::string m_description;

        std::string m_type;
        std::string m_category;
        std::string m_power;
        std::string m_accuracy;
        std::string m_pp;
        std::string m_priority;
        std::string m_target;
        std::string m_flags;

        std::string m_effectCode;
        std::string m_effectChance;
        std::string m_secondaryStatus;
        std::string m_secondaryBoosts;
    };

    class moveCsvAdapter {
      public:
        static std::vector<MoveRecord> load( const std::string& p_fsrootPath );
    };

    class moveDataEditor {
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

        Gtk::Grid _mechanicsGrid;
        Gtk::Grid _effectsGrid;
        Gtk::Grid _textGrid;

        Gtk::Entry _mechanicsNameValue, _mechanicsTypeValue, _mechanicsCategoryValue;
        Gtk::Entry _mechanicsPowerValue, _mechanicsAccuracyValue, _mechanicsPpValue;
        Gtk::Entry _mechanicsPriorityValue, _mechanicsTargetValue, _mechanicsFlagsValue;

        Gtk::Entry _effectsCodeValue, _effectsChanceValue, _effectsStatusValue, _effectsBoostValue;

        Gtk::Entry _textNameValue;
        Gtk::Entry _textDescriptionValue;

        std::vector<MoveRecord> _allRecords;
        std::vector<size_t>     _filteredIndices;
        std::string             _lastDataPath;
        bool                    _dirty = false;
        bool                    _updatingFields = false;
        size_t                  _selectedRecordIndex = size_t( -1 );

        std::vector<std::vector<std::string>> _moveDataRows;
        std::vector<std::vector<std::string>> _moveNameRows;
        std::vector<std::vector<std::string>> _moveDescrRows;
        std::map<u16, size_t>                 _moveDataRowById;
        std::map<u16, size_t>                 _moveNameRowById;
        std::map<u16, size_t>                 _moveDescrRowById;

        static std::string toLower( const std::string& p_text );
        static void        setupGrid( Gtk::Grid& p_grid );
        static void attachRow( Gtk::Grid& p_grid, int p_row, const std::string& p_label,
                               Gtk::Entry& p_value );

        static std::vector<std::vector<std::string>> readDelimitedCsv( const std::string& p_path,
                                                                        char p_delim );
        static bool writeDelimitedCsv( const std::string&                            p_path,
                                       const std::vector<std::vector<std::string>>& p_rows,
                                       char p_delim );
        void reloadIfNeeded( );
        void rebuildList( );
        void selectByFilteredPosition( size_t p_pos );
        void showRecord( const MoveRecord& p_record );
        void handleFieldEdited( );
        void loadRowsFromDisk( const std::string& p_dataPath );
        void applyFieldEditsToSelectedRecord( );
        bool writeCurrentRowsToDisk( const std::string& p_dataPath );

      public:
        moveDataEditor( model& p_model, root& p_root );

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
