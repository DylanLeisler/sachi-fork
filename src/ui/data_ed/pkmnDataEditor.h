#pragma once
#include <string>
#include <vector>
#include <map>

#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/dropdown.h>
#include <gtkmm/entry.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/signallistitemfactory.h>
#include <gtkmm/notebook.h>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/stringlist.h>
#include <gtkmm/textview.h>

#include "../../model.h"

namespace UI {
    class root;

    struct PokemonRecord {
        u16         m_id = 0;
        std::string m_name;
        std::string m_category;
        std::string m_flavor;

        std::string m_type1;
        std::string m_type2;
        std::string m_ability1;
        std::string m_ability2;
        std::string m_hiddenAbility;

        std::string m_hp;
        std::string m_atk;
        std::string m_def;
        std::string m_spAtk;
        std::string m_spDef;
        std::string m_speed;
        std::string m_baseExp;

        std::string m_expType;
        std::string m_eggCycles;
        std::string m_catchRate;

        std::string m_gender;
        std::string m_height;
        std::string m_weight;

        std::string m_evolutionSpec;
        std::string m_learnsetSpec;
        std::string m_formSpec;

        std::vector<std::string> m_evolutions;
        std::vector<std::string> m_forms;
        std::vector<std::string> m_learnsetPreview;
    };

    class pokemonCsvAdapter {
      public:
        static std::vector<PokemonRecord> load( const std::string& p_fsrootPath );
    };

    class pkmnDataEditor {
        struct EvolutionEntry {
            std::string m_toName;
            std::string m_methodDisplay;
            std::string m_param;
        };

        enum class LearnsetKind { LEVEL, TMHM, EGG, TUTOR };
        struct LearnsetEntry {
            LearnsetKind m_kind = LearnsetKind::LEVEL;
            std::string  m_move;
            int          m_level = 1;
        };
        struct LearnsetRowWidgets {
            Gtk::ListBoxRow* m_row = nullptr;
            Gtk::Label*      m_kindLabel = nullptr;
            Gtk::Entry*      m_levelEntry = nullptr;
            Gtk::DropDown*   m_moveDD = nullptr;
            Gtk::Button*     m_delButton = nullptr;
            size_t           m_dataIndex = size_t( -1 );
        };
        struct TmhmInfo {
            int         m_group = 0; // 0=TM/unknown, 1=HM
            int         m_number = 9999;
            std::string m_label = "TM";
        };

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

        Gtk::Grid _overviewGrid;
        Gtk::Grid _statsGrid;
        Gtk::Grid _textGrid;
        Gtk::Grid _formsGrid;

        Gtk::Entry _overviewNameValue, _overviewCategoryValue;
        Gtk::DropDown _overviewType1Value, _overviewType2Value;
        Gtk::DropDown _overviewExpTypeValue;
        Gtk::Entry _overviewEggCyclesValue, _overviewCatchRateValue;
        Gtk::Entry _overviewBaseExpValue;

        Gtk::Entry _statsHpValue, _statsAtkValue, _statsDefValue;
        Gtk::Entry _statsSpAtkValue, _statsSpDefValue, _statsSpeedValue;
        Gtk::DropDown _statsAbility1Value, _statsAbility2Value, _statsHiddenAbilityValue;
        Gtk::DropDown _statsGenderValue;
        Gtk::Entry _statsHeightValue, _statsWeightValue;

        Gtk::ScrolledWindow _textFlavorScroll;
        Gtk::TextView       _textFlavorValue;
        Gtk::Entry _formsEditValue;

        Gtk::Box            _evolutionBox{ Gtk::Orientation::VERTICAL };
        Gtk::Button         _addEvolutionButton{ "Add Evolution" };
        Gtk::ScrolledWindow _evolutionScroll;
        Gtk::ListBox        _evolutionList;

        Gtk::Box            _learnsetBox{ Gtk::Orientation::VERTICAL };
        Gtk::Box            _learnsetAddRow{ Gtk::Orientation::HORIZONTAL };
        Gtk::Button         _addLearnsetButton{ "Add Move" };
        Gtk::DropDown       _addLearnsetKindDD;
        Gtk::ScrolledWindow _learnsetScroll;
        Gtk::Box            _learnsetSections{ Gtk::Orientation::VERTICAL };
        Gtk::Label          _learnsetLevelLabel{ "Level" };
        Gtk::Label          _learnsetLevelEmpty{ "None" };
        Gtk::ListBox        _learnsetLevelList;
        Gtk::Label          _learnsetTmhmLabel{ "TM/HM" };
        Gtk::Label          _learnsetTmhmEmpty{ "None" };
        Gtk::ListBox        _learnsetTmhmList;
        Gtk::Label          _learnsetEggLabel{ "Egg" };
        Gtk::Label          _learnsetEggEmpty{ "None" };
        Gtk::ListBox        _learnsetEggList;
        Gtk::Label          _learnsetTutorLabel{ "Tutor" };
        Gtk::Label          _learnsetTutorEmpty{ "None" };
        Gtk::ListBox        _learnsetTutorList;

        Gtk::ScrolledWindow _formsScroll;
        Gtk::Label          _formsText;

        std::vector<PokemonRecord> _allRecords;
        std::vector<size_t>        _filteredIndices;
        std::string                _lastDataPath;
        bool                       _dirty = false;
        bool                       _updatingFields = false;
        bool                       _evolutionUiDirty = true;
        bool                       _learnsetUiDirty = true;
        size_t                     _selectedRecordIndex = size_t( -1 );

        std::vector<std::vector<std::string>> _pkmnNamesRows;
        std::vector<std::vector<std::string>> _pkmnCategoryRows;
        std::vector<std::vector<std::string>> _pkmnFlavorRows;
        std::vector<std::vector<std::string>> _pkmnDataRows;
        std::vector<std::vector<std::string>> _pkmnDescrRows;
        std::vector<std::vector<std::string>> _pkmnEvolvRows;
        std::vector<std::vector<std::string>> _pkmnLearnsetRows;
        std::vector<std::vector<std::string>> _pkmnFormNameRows;

        std::map<u16, size_t>                 _pkmnNamesRowById;
        std::map<u16, size_t>                 _pkmnCategoryRowById;
        std::map<u16, size_t>                 _pkmnFlavorRowById;
        std::map<u16, size_t>                 _pkmnDataRowById;
        std::map<u16, size_t>                 _pkmnDescrRowById;
        std::map<std::string, std::vector<size_t>> _pkmnEvolvRowsByFromName;
        std::map<std::string, size_t>              _pkmnLearnsetRowByName;
        std::map<u16, std::vector<size_t>>         _pkmnFormNameRowsByBaseId;

        std::vector<EvolutionEntry> _editingEvolutions;
        std::vector<LearnsetEntry>  _editingLearnset;
        std::vector<std::string>    _pokemonChoices;
        std::vector<std::string>    _moveChoices;
        std::vector<std::string>    _evoMethodChoices;
        std::map<std::string, std::string> _evoMethodRawByDisplay;
        std::map<std::string, int> _moveIdByTokenLower;
        std::map<int, std::string> _moveTokenById;
        std::map<int, std::string> _moveNameKeyById;
        std::vector<std::string> _type1Choices;
        std::vector<std::string> _typeAllChoices;
        std::vector<std::string> _type2Choices;
        std::vector<std::string> _growthChoices;
        std::vector<std::string> _ability1Choices;
        std::vector<std::string> _abilityAllChoices;
        std::vector<std::string> _abilityOptionalChoices;
        std::vector<std::string> _genderLabels;
        std::vector<std::string> _genderValues;
        std::map<std::string, int> _abilityCategoryByLower;
        size_t _type2PrevSel = 0;
        size_t _ability1PrevSel = 0;
        size_t _ability2PrevSel = 0;
        size_t _hiddenAbilityPrevSel = 0;
        std::vector<int>           _moveChoiceIdByIndex;
        std::map<int, size_t>      _moveChoiceIndexById;
        std::map<std::string, TmhmInfo> _tmhmInfoByMoveLower;
        Glib::RefPtr<Gtk::StringList> _pokemonChoicesModel;
        Glib::RefPtr<Gtk::StringList> _moveChoicesModel;
        Glib::RefPtr<Gtk::StringList> _evoMethodChoicesModel;
        Glib::RefPtr<Gtk::StringList> _learnsetKindModel;
        Glib::RefPtr<Gtk::StringList> _type1Model;
        Glib::RefPtr<Gtk::StringList> _type2Model;
        Glib::RefPtr<Gtk::StringList> _growthModel;
        Glib::RefPtr<Gtk::StringList> _ability1Model;
        Glib::RefPtr<Gtk::StringList> _abilityOptionalModel;
        Glib::RefPtr<Gtk::StringList> _genderModel;
        Glib::RefPtr<Gtk::SignalListItemFactory> _ability1ListFactory;
        Glib::RefPtr<Gtk::SignalListItemFactory> _ability2ListFactory;
        Glib::RefPtr<Gtk::SignalListItemFactory> _abilityHiddenListFactory;
        std::vector<LearnsetRowWidgets> _learnsetLevelPool;
        std::vector<LearnsetRowWidgets> _learnsetTmhmPool;
        std::vector<LearnsetRowWidgets> _learnsetEggPool;
        std::vector<LearnsetRowWidgets> _learnsetTutorPool;

        static std::string toLower( const std::string& p_text );
        static void        setupGrid( Gtk::Grid& p_grid );
        static void        attachRow( Gtk::Grid& p_grid, int p_row, const std::string& p_label,
                                      Gtk::Entry& p_value );
        static void        attachRow( Gtk::Grid& p_grid, int p_row, const std::string& p_label,
                                      Gtk::DropDown& p_value );
        static std::string joinLines( const std::vector<std::string>& p_lines );
        static std::string trim( const std::string& p_text );
        static std::string decodeEscapedNewlines( const std::string& p_text );
        static std::string encodeEscapedNewlines( const std::string& p_text );
        static std::vector<std::vector<std::string>> readDelimitedCsv( const std::string& p_path,
                                                                        char p_delim );
        static bool writeDelimitedCsv( const std::string&                            p_path,
                                       const std::vector<std::vector<std::string>>& p_rows,
                                       char p_delim );

        void reloadIfNeeded( );
        void rebuildList( );
        void selectByFilteredPosition( size_t p_pos );
        void showRecord( const PokemonRecord& p_record );
        void clearDetails( );
        void handleFieldEdited( );
        void handleDropDownEdited( int p_source );
        void enforceSelectorRules( int p_source );
        void refreshAbilitySelectorTint( );
        void rebuildType2Choices( const std::string& p_excludedType,
                                  const std::string& p_preferredSelection );
        void rebuildAbilityOptionalChoices( const std::string& p_excludedAbility,
                                            const std::string& p_preferredAbility2,
                                            const std::string& p_preferredHidden );
        void installAbilityListFactory( Gtk::DropDown& p_dd,
                                        Glib::RefPtr<Gtk::SignalListItemFactory>& p_factoryOut );
        void captureAbilityPreviousSelections( );
        bool isAbilitySentinelChoice( const std::string& p_value ) const;
        std::string selectedDropDownValue( const Gtk::DropDown&            p_dd,
                                           const std::vector<std::string>& p_values ) const;
        void setDropDownByValue( Gtk::DropDown&                  p_dd,
                                 const std::vector<std::string>& p_values,
                                 const std::string&              p_value,
                                 size_t                          p_default = 0 );
        void loadRowsFromDisk( const std::string& p_dataPath );
        void applyFieldEditsToSelectedRecord( );
        bool writeCurrentRowsToDisk( const std::string& p_dataPath );
        void buildEditorChoiceLists( );
        void rebuildEvolutionUi( );
        void rebuildLearnsetUi( );
        void syncSpecsFromEditorState( );
        static LearnsetKind decodeLearnsetKind( const std::string& p_code );
        static int          parseLearnsetLevel( const std::string& p_code );

      public:
        pkmnDataEditor( model& p_model, root& p_root );

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
