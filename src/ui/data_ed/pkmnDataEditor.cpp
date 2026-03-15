#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <set>
#include <unordered_set>
#include <sstream>
#include <tuple>
#include <utility>
#include <filesystem>

#include <gtkmm/box.h>
#include <gtkmm/listitem.h>
#include <gtkmm/separator.h>
#include <gtkmm/stringobject.h>

#include "../../defines.h"
#include "../root.h"
#include "pkmnDataEditor.h"

namespace UI {
    namespace fs = std::filesystem;

    namespace {
        enum SelectorSource : int {
            SEL_NONE = 0,
            SEL_TYPE1,
            SEL_TYPE2,
            SEL_GROWTH,
            SEL_ABILITY1,
            SEL_ABILITY2,
            SEL_ABILITY_HIDDEN,
            SEL_GENDER,
        };

        constexpr const char* ABILITY_SENTINEL_NEVER_HIDDEN = "Never Hidden";
        constexpr const char* ABILITY_SENTINEL_ONLY_HIDDEN  = "Only Hidden";
        constexpr const char* ABILITY_SENTINEL_HIDDEN_OPTIONAL = "Hidden Optional";
        constexpr const char* ABILITY_SENTINEL_DIVIDER = "----------------";

        std::string normalizeTypeToken( const std::string& p_type ) {
            size_t first = 0;
            while( first < p_type.size( )
                   && std::isspace( static_cast<unsigned char>( p_type[ first ] ) ) ) {
                ++first;
            }
            size_t last = p_type.size( );
            while( last > first
                   && std::isspace( static_cast<unsigned char>( p_type[ last - 1 ] ) ) ) {
                --last;
            }
            auto t = p_type.substr( first, last - first );
            auto l = t;
            std::transform( l.begin( ), l.end( ), l.begin( ),
                            []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
            if( l == "fight" ) { return "Fighting"; }
            return t;
        }

        std::vector<std::string> splitLine( const std::string& p_line, char p_delim ) {
            std::vector<std::string> out;
            std::string              cur;
            std::istringstream       stream( p_line );
            while( std::getline( stream, cur, p_delim ) ) { out.push_back( cur ); }
            if( !p_line.empty( ) && p_line.back( ) == p_delim ) { out.push_back( "" ); }
            return out;
        }

        std::string safeGet( const std::vector<std::string>& p_cols, size_t p_idx ) {
            if( p_idx >= p_cols.size( ) ) { return ""; }
            return p_cols[ p_idx ];
        }

        u16 parseId( const std::string& p_text ) {
            try {
                return static_cast<u16>( std::stoul( p_text ) );
            } catch( ... ) { return 0; }
        }

        std::string normalizeLine( const std::string& p_line ) {
            auto out = p_line;
            while( !out.empty( ) && ( out.back( ) == '\n' || out.back( ) == '\r' ) ) {
                out.pop_back( );
            }
            return out;
        }

        std::string normalizeLearnsetToken( const std::string& p_token ) {
            auto trimText = []( const std::string& p_text ) {
                size_t first = 0;
                while( first < p_text.size( )
                       && std::isspace( static_cast<unsigned char>( p_text[ first ] ) ) ) {
                    ++first;
                }
                size_t last = p_text.size( );
                while( last > first
                       && std::isspace( static_cast<unsigned char>( p_text[ last - 1 ] ) ) ) {
                    --last;
                }
                return p_text.substr( first, last - first );
            };

            auto token = trimText( p_token );
            if( token.empty( ) ) { return ""; }
            auto parts = splitLine( token, ';' );
            if( parts.size( ) < 2 ) { return ""; }
            auto move = trimText( parts[ 0 ] );
            auto code = trimText( parts[ 1 ] );
            if( move.empty( ) || code.empty( ) ) { return ""; }
            return move + ";" + code;
        }

        std::string canonicalMoveKey( const std::string& p_text ) {
            std::string out;
            out.reserve( p_text.size( ) );
            for( unsigned char c : p_text ) {
                if( std::isalnum( c ) ) { out.push_back( static_cast<char>( std::tolower( c ) ) ); }
            }
            return out;
        }

        std::string lowerText( const std::string& p_text ) {
            std::string out = p_text;
            std::transform( out.begin( ), out.end( ), out.begin( ),
                            []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
            return out;
        }

        u16 parseFormBaseId( const std::string& p_formKey ) {
            auto pos = p_formKey.find( '_' );
            return parseId( pos == std::string::npos ? p_formKey : p_formKey.substr( 0, pos ) );
        }

        std::vector<Glib::ustring> toUStrings( const std::vector<std::string>& p_values ) {
            std::vector<Glib::ustring> out;
            out.reserve( p_values.size( ) );
            for( const auto& v : p_values ) { out.push_back( v ); }
            return out;
        }
    } // namespace

    std::vector<PokemonRecord> pokemonCsvAdapter::load( const std::string& p_fsrootPath ) {
        auto fsrootPath = fs::path( p_fsrootPath );
        auto pneoRoot
            = fsrootPath.filename( ) == "FSROOT" ? fsrootPath.parent_path( ) : fsrootPath;
        auto dataPath = pneoRoot / "tools" / "fsdata" / "data";

        auto namesPath         = dataPath / "pkmnnames.csv";
        auto categoryPath      = dataPath / "pkmncategory.csv";
        auto flavorPath        = dataPath / "pkmnflavor.csv";
        auto pkmnDataPath      = dataPath / "pkmndata.csv";
        auto pkmnDescrPath     = dataPath / "pkmndescr.csv";
        auto pkmnEvolvPath     = dataPath / "pkmnevolv.csv";
        auto pkmnLearnsetPath  = dataPath / "pkmnlearnsets.csv";
        auto pkmnFormPath      = dataPath / "pkmnformes.csv";
        auto pkmnFormNamesPath = dataPath / "pkmnformnames.csv";

        std::map<u16, PokemonRecord> records;
        std::map<std::string, u16>   idByName;

        {
            std::ifstream f( namesPath );
            std::string   line;
            while( std::getline( f, line ) ) {
                auto cols = splitLine( normalizeLine( line ), ',' );
                if( cols.empty( ) ) { continue; }
                auto id   = parseId( safeGet( cols, 0 ) );
                auto& rec = records[ id ];
                rec.m_id  = id;
                rec.m_name = safeGet( cols, 1 );
                idByName[ lowerText( rec.m_name ) ] = id;
            }
        }

        {
            std::ifstream f( categoryPath );
            std::string   line;
            while( std::getline( f, line ) ) {
                auto cols = splitLine( normalizeLine( line ), ',' );
                if( cols.empty( ) ) { continue; }
                auto id   = parseId( safeGet( cols, 0 ) );
                auto& rec = records[ id ];
                rec.m_id  = id;
                rec.m_category = safeGet( cols, 1 );
            }
        }

        {
            std::ifstream f( flavorPath );
            std::string   line;
            while( std::getline( f, line ) ) {
                auto cols = splitLine( normalizeLine( line ), ';' );
                if( cols.empty( ) ) { continue; }
                auto id   = parseId( safeGet( cols, 0 ) );
                auto& rec = records[ id ];
                rec.m_id  = id;
                rec.m_flavor = safeGet( cols, 1 );
            }
        }

        {
            std::ifstream f( pkmnDataPath );
            std::string   line;
            while( std::getline( f, line ) ) {
                auto cols = splitLine( normalizeLine( line ), ',' );
                if( cols.empty( ) ) { continue; }
                auto id   = parseId( safeGet( cols, 0 ) );
                auto& rec = records[ id ];
                rec.m_id  = id;
                rec.m_expType = safeGet( cols, 1 );
                rec.m_eggCycles = safeGet( cols, 2 );
                rec.m_catchRate = safeGet( cols, 3 );
            }
        }

        {
            std::ifstream f( pkmnDescrPath );
            std::string   line;
            while( std::getline( f, line ) ) {
                auto cols = splitLine( normalizeLine( line ), ',' );
                if( cols.empty( ) ) { continue; }
                auto id   = parseId( safeGet( cols, 0 ) );
                auto& rec = records[ id ];
                rec.m_id  = id;
                rec.m_type1 = safeGet( cols, 1 );
                rec.m_type2 = safeGet( cols, 2 );
                rec.m_ability1 = safeGet( cols, 3 );
                rec.m_ability2 = safeGet( cols, 4 );
                rec.m_hiddenAbility = safeGet( cols, 5 );

                rec.m_hp = safeGet( cols, 8 );
                rec.m_atk = safeGet( cols, 9 );
                rec.m_def = safeGet( cols, 10 );
                rec.m_spAtk = safeGet( cols, 11 );
                rec.m_spDef = safeGet( cols, 12 );
                rec.m_speed = safeGet( cols, 13 );
                rec.m_baseExp = safeGet( cols, 14 );

                rec.m_gender = safeGet( cols, 15 );
                rec.m_height = safeGet( cols, 16 );
                rec.m_weight = safeGet( cols, 17 );
            }
        }

        {
            std::ifstream f( pkmnEvolvPath );
            std::string   line;
            while( std::getline( f, line ) ) {
                auto cols = splitLine( normalizeLine( line ), ',' );
                if( cols.size( ) < 3 ) { continue; }
                auto fromName = safeGet( cols, 1 );
                auto toName   = safeGet( cols, 2 );
                auto method   = safeGet( cols, 4 );

                auto fromIt = idByName.find( lowerText( fromName ) );
                if( fromIt != idByName.end( ) ) {
                    records[ fromIt->second ].m_evolutions.push_back(
                        fromName + " -> " + toName + ( method.empty( ) ? "" : " (" + method + ")" ) );
                }
            }
        }

        {
            std::ifstream f( pkmnLearnsetPath );
            std::string   line;
            while( std::getline( f, line ) ) {
                auto cols = splitLine( normalizeLine( line ), ',' );
                if( cols.empty( ) ) { continue; }
                auto nameIt = idByName.find( lowerText( safeGet( cols, 0 ) ) );
                if( nameIt == idByName.end( ) ) { continue; }
                auto& learnset = records[ nameIt->second ].m_learnsetPreview;
                learnset.clear( );
                for( size_t i = 1; i < cols.size( ) && learnset.size( ) < 12; ++i ) {
                    if( cols[ i ].empty( ) ) { continue; }
                    learnset.push_back( cols[ i ] );
                }
            }
        }

        {
            std::ifstream f( pkmnFormPath );
            std::string   line;
            while( std::getline( f, line ) ) {
                auto cols = splitLine( normalizeLine( line ), ',' );
                if( cols.empty( ) ) { continue; }
                auto formKey = safeGet( cols, 0 );
                auto baseId  = parseFormBaseId( formKey );
                if( !baseId ) { continue; }
                auto summary = formKey + " [" + safeGet( cols, 1 ) + "/" + safeGet( cols, 2 ) + "]";
                records[ baseId ].m_forms.push_back( summary );
            }
        }

        {
            std::ifstream f( pkmnFormNamesPath );
            std::string   line;
            while( std::getline( f, line ) ) {
                auto cols = splitLine( normalizeLine( line ), ',' );
                if( cols.empty( ) ) { continue; }
                auto formKey = safeGet( cols, 0 );
                auto baseId  = parseFormBaseId( formKey );
                if( !baseId ) { continue; }
                auto name = safeGet( cols, 1 );
                if( !name.empty( ) ) { records[ baseId ].m_forms.push_back( formKey + " - " + name ); }
            }
        }

        std::vector<PokemonRecord> out;
        out.reserve( records.size( ) );
        for( auto& [ _, rec ] : records ) { out.push_back( std::move( rec ) ); }
        std::sort( out.begin( ), out.end( ),
                   []( const PokemonRecord& p_lhs, const PokemonRecord& p_rhs ) {
                       return p_lhs.m_id < p_rhs.m_id;
                   } );
        return out;
    }

    std::string pkmnDataEditor::toLower( const std::string& p_text ) {
        std::string out = p_text;
        std::transform( out.begin( ), out.end( ), out.begin( ),
                        []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
        return out;
    }

    std::string pkmnDataEditor::trim( const std::string& p_text ) {
        size_t first = 0;
        while( first < p_text.size( ) && std::isspace( static_cast<unsigned char>( p_text[ first ] ) ) ) {
            ++first;
        }
        size_t last = p_text.size( );
        while( last > first && std::isspace( static_cast<unsigned char>( p_text[ last - 1 ] ) ) ) {
            --last;
        }
        return p_text.substr( first, last - first );
    }

    std::vector<std::vector<std::string>>
    pkmnDataEditor::readDelimitedCsv( const std::string& p_path, char p_delim ) {
        std::vector<std::vector<std::string>> rows;
        std::ifstream                         f( p_path );
        std::string                           line;
        while( std::getline( f, line ) ) { rows.push_back( splitLine( normalizeLine( line ), p_delim ) ); }
        return rows;
    }

    bool pkmnDataEditor::writeDelimitedCsv( const std::string& p_path,
                                            const std::vector<std::vector<std::string>>& p_rows,
                                            char p_delim ) {
        std::ofstream f( p_path, std::ios::trunc );
        if( !f.good( ) ) { return false; }

        for( const auto& row : p_rows ) {
            for( size_t i = 0; i < row.size( ); ++i ) {
                if( i ) { f << p_delim; }
                f << row[ i ];
            }
            f << '\n';
        }
        return true;
    }

    void pkmnDataEditor::setupGrid( Gtk::Grid& p_grid ) {
        p_grid.set_row_spacing( MARGIN );
        p_grid.set_column_spacing( MARGIN * 2 );
        p_grid.set_margin( MARGIN );
    }

    void pkmnDataEditor::attachRow( Gtk::Grid& p_grid, int p_row, const std::string& p_label,
                                    Gtk::Entry& p_value ) {
        auto* key = Gtk::make_managed<Gtk::Label>( p_label );
        key->set_halign( Gtk::Align::START );
        key->set_xalign( 0.0f );
        p_grid.attach( *key, 0, p_row, 1, 1 );

        p_value.set_hexpand( true );
        p_grid.attach( p_value, 1, p_row, 1, 1 );
    }

    void pkmnDataEditor::attachRow( Gtk::Grid& p_grid, int p_row, const std::string& p_label,
                                    Gtk::DropDown& p_value ) {
        auto* key = Gtk::make_managed<Gtk::Label>( p_label );
        key->set_halign( Gtk::Align::START );
        key->set_xalign( 0.0f );
        p_grid.attach( *key, 0, p_row, 1, 1 );

        p_value.set_hexpand( true );
        p_grid.attach( p_value, 1, p_row, 1, 1 );
    }

    std::string pkmnDataEditor::joinLines( const std::vector<std::string>& p_lines ) {
        if( p_lines.empty( ) ) { return ""; }
        std::string out;
        for( size_t i = 0; i < p_lines.size( ); ++i ) {
            if( i ) { out += "\n"; }
            out += p_lines[ i ];
        }
        return out;
    }

    std::string pkmnDataEditor::decodeEscapedNewlines( const std::string& p_text ) {
        std::string out;
        out.reserve( p_text.size( ) );
        for( size_t i = 0; i < p_text.size( ); ++i ) {
            if( p_text[ i ] == '\\' && i + 1 < p_text.size( ) && p_text[ i + 1 ] == 'n' ) {
                out.push_back( '\n' );
                ++i;
            } else {
                out.push_back( p_text[ i ] );
            }
        }
        return out;
    }

    std::string pkmnDataEditor::encodeEscapedNewlines( const std::string& p_text ) {
        std::string out;
        out.reserve( p_text.size( ) + 8 );
        for( char c : p_text ) {
            if( c == '\r' ) { continue; }
            if( c == '\n' ) {
                out += "\\n";
            } else {
                out.push_back( c );
            }
        }
        return out;
    }

    pkmnDataEditor::pkmnDataEditor( model& p_model, root& p_root )
        : _model( p_model ), _rootWindow( p_root ) {
        _learnsetKindModel = Gtk::StringList::create( toUStrings( { "Level", "TM/HM", "Egg", "Tutor" } ) );
        _mainBox.set_margin( MARGIN );
        _mainBox.set_spacing( MARGIN );
        _mainBox.set_expand( );

        _title.set_markup( "<span size=\"x-large\">Pokemon Editor</span>" );
        _title.set_halign( Gtk::Align::START );
        _mainBox.append( _title );

        _splitPane.set_wide_handle( true );
        _splitPane.set_position( 320 );
        _splitPane.set_expand( true );
        _mainBox.append( _splitPane );

        _browserBox.set_spacing( MARGIN );
        _browserBox.set_margin( MARGIN );
        _search.set_placeholder_text( "Search pokemon..." );
        _browserBox.append( _search );

        _listScroll.set_policy( Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC );
        _listScroll.set_expand( true );
        _listScroll.set_child( _recordList );
        _browserBox.append( _listScroll );
        _splitPane.set_start_child( _browserBox );

        _tabs.set_expand( true );
        setupGrid( _overviewGrid );
        setupGrid( _statsGrid );
        setupGrid( _textGrid );
        setupGrid( _formsGrid );

        attachRow( _overviewGrid, 0, "Name", _overviewNameValue );
        attachRow( _overviewGrid, 1, "Category", _overviewCategoryValue );
        attachRow( _overviewGrid, 2, "Type 1", _overviewType1Value );
        attachRow( _overviewGrid, 3, "Type 2", _overviewType2Value );
        attachRow( _overviewGrid, 4, "Growth", _overviewExpTypeValue );
        attachRow( _overviewGrid, 5, "Egg Cycles", _overviewEggCyclesValue );
        attachRow( _overviewGrid, 6, "Catch Rate", _overviewCatchRateValue );
        attachRow( _overviewGrid, 7, "Base Exp", _overviewBaseExpValue );

        attachRow( _statsGrid, 0, "HP", _statsHpValue );
        attachRow( _statsGrid, 1, "Atk", _statsAtkValue );
        attachRow( _statsGrid, 2, "Def", _statsDefValue );
        attachRow( _statsGrid, 3, "SpAtk", _statsSpAtkValue );
        attachRow( _statsGrid, 4, "SpDef", _statsSpDefValue );
        attachRow( _statsGrid, 5, "Speed", _statsSpeedValue );
        attachRow( _statsGrid, 6, "Ability 1", _statsAbility1Value );
        attachRow( _statsGrid, 7, "Ability 2", _statsAbility2Value );
        attachRow( _statsGrid, 8, "Hidden Ability", _statsHiddenAbilityValue );
        attachRow( _statsGrid, 9, "Gender", _statsGenderValue );
        attachRow( _statsGrid, 10, "Height", _statsHeightValue );
        attachRow( _statsGrid, 11, "Weight", _statsWeightValue );
        _statsAbility1Value.set_enable_search( true );
        _statsAbility2Value.set_enable_search( true );
        _statsHiddenAbilityValue.set_enable_search( true );
        installAbilityListFactory( _statsAbility1Value, _ability1ListFactory );
        installAbilityListFactory( _statsAbility2Value, _ability2ListFactory );
        installAbilityListFactory( _statsHiddenAbilityValue, _abilityHiddenListFactory );

        _evolutionBox.set_margin( MARGIN );
        _evolutionBox.set_spacing( MARGIN );
        _addEvolutionButton.set_halign( Gtk::Align::START );
        _evolutionBox.append( _addEvolutionButton );
        _evolutionScroll.set_policy( Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC );
        _evolutionScroll.set_expand( true );
        _evolutionScroll.set_child( _evolutionList );
        _evolutionBox.append( _evolutionScroll );

        _learnsetBox.set_margin( MARGIN );
        _learnsetBox.set_spacing( MARGIN );
        _learnsetAddRow.set_spacing( MARGIN );
        _addLearnsetKindDD.set_model( _learnsetKindModel );
        _addLearnsetKindDD.set_selected( 0 );
        _learnsetAddRow.append( _addLearnsetKindDD );
        _addLearnsetButton.set_halign( Gtk::Align::START );
        _learnsetAddRow.append( _addLearnsetButton );
        _learnsetBox.append( _learnsetAddRow );
        _learnsetScroll.set_policy( Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC );
        _learnsetScroll.set_expand( true );
        _learnsetSections.set_spacing( MARGIN );
        _learnsetSections.append( _learnsetLevelLabel );
        _learnsetLevelEmpty.add_css_class( "dim-label" );
        _learnsetLevelEmpty.set_margin_start( MARGIN );
        _learnsetSections.append( _learnsetLevelEmpty );
        _learnsetSections.append( _learnsetLevelList );
        _learnsetSections.append( _learnsetTmhmLabel );
        _learnsetTmhmEmpty.add_css_class( "dim-label" );
        _learnsetTmhmEmpty.set_margin_start( MARGIN );
        _learnsetSections.append( _learnsetTmhmEmpty );
        _learnsetSections.append( _learnsetTmhmList );
        _learnsetSections.append( _learnsetEggLabel );
        _learnsetEggEmpty.add_css_class( "dim-label" );
        _learnsetEggEmpty.set_margin_start( MARGIN );
        _learnsetSections.append( _learnsetEggEmpty );
        _learnsetSections.append( _learnsetEggList );
        _learnsetSections.append( _learnsetTutorLabel );
        _learnsetTutorEmpty.add_css_class( "dim-label" );
        _learnsetTutorEmpty.set_margin_start( MARGIN );
        _learnsetSections.append( _learnsetTutorEmpty );
        _learnsetSections.append( _learnsetTutorList );
        _learnsetScroll.set_child( _learnsetSections );
        _learnsetBox.append( _learnsetScroll );

        attachRow( _formsGrid, 0, "Form Names", _formsEditValue );
        _formsEditValue.set_placeholder_text( "formKey|Display Name;formKey2|Display Name" );

        auto* flavorLabel = Gtk::make_managed<Gtk::Label>( "Flavor" );
        flavorLabel->set_halign( Gtk::Align::START );
        flavorLabel->set_xalign( 0.0f );
        _textGrid.attach( *flavorLabel, 0, 0, 1, 1 );
        _textFlavorValue.set_wrap_mode( Gtk::WrapMode::WORD_CHAR );
        _textFlavorValue.set_vexpand( true );
        _textFlavorValue.set_hexpand( true );
        _textFlavorScroll.set_policy( Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC );
        _textFlavorScroll.set_expand( true );
        _textFlavorScroll.set_child( _textFlavorValue );
        _textGrid.attach( _textFlavorScroll, 1, 0, 1, 1 );

        _formsText.set_wrap( true );
        _formsText.set_xalign( 0.0f );
        _formsText.set_yalign( 0.0f );
        _formsText.set_selectable( true );
        _formsText.set_margin( MARGIN );
        _formsScroll.set_child( _formsText );
        _formsScroll.set_policy( Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC );

        _tabs.append_page( _overviewGrid, "Overview" );
        _tabs.append_page( _statsGrid, "Stats" );
        _tabs.append_page( _evolutionBox, "Evolutions" );
        _tabs.append_page( _learnsetBox, "Learnset" );
        _tabs.append_page( _formsGrid, "Forms" );
        _tabs.append_page( _textGrid, "Flavor" );
        _splitPane.set_end_child( _tabs );

        _search.signal_changed( ).connect( [ this ]( ) { rebuildList( ); } );
        _recordList.signal_row_selected( ).connect( [ this ]( Gtk::ListBoxRow* p_row ) {
            if( p_row == nullptr ) { return; }
            const auto idx = static_cast<size_t>( p_row->get_index( ) );
            selectByFilteredPosition( idx );
        } );
        _tabs.signal_switch_page( ).connect( [ this ]( Gtk::Widget*, guint p_pageNum ) {
            if( p_pageNum == 2 && _evolutionUiDirty ) {
                rebuildEvolutionUi( );
                _evolutionUiDirty = false;
            }
            if( p_pageNum == 3 && _learnsetUiDirty ) {
                rebuildLearnsetUi( );
                _learnsetUiDirty = false;
            }
        } );

        auto connectField = [ this ]( Gtk::Entry& p_entry ) {
            p_entry.signal_changed( ).connect( [ this ]( ) { handleFieldEdited( ); } );
        };
        auto connectSelector = [ this ]( Gtk::DropDown& p_dd, int p_source ) {
            p_dd.property_selected( ).signal_changed( ).connect(
                [ this, p_source ]( ) { handleDropDownEdited( p_source ); } );
        };
        connectField( _overviewNameValue );
        connectField( _overviewCategoryValue );
        connectSelector( _overviewType1Value, SEL_TYPE1 );
        connectSelector( _overviewType2Value, SEL_TYPE2 );
        connectSelector( _overviewExpTypeValue, SEL_GROWTH );
        connectField( _overviewEggCyclesValue );
        connectField( _overviewCatchRateValue );
        connectField( _overviewBaseExpValue );
        connectField( _statsHpValue );
        connectField( _statsAtkValue );
        connectField( _statsDefValue );
        connectField( _statsSpAtkValue );
        connectField( _statsSpDefValue );
        connectField( _statsSpeedValue );
        connectSelector( _statsAbility1Value, SEL_ABILITY1 );
        connectSelector( _statsAbility2Value, SEL_ABILITY2 );
        connectSelector( _statsHiddenAbilityValue, SEL_ABILITY_HIDDEN );
        connectSelector( _statsGenderValue, SEL_GENDER );
        connectField( _statsHeightValue );
        connectField( _statsWeightValue );
        connectField( _formsEditValue );
        if( auto buffer = _textFlavorValue.get_buffer( ) ) {
            buffer->signal_changed( ).connect( [ this ]( ) { handleFieldEdited( ); } );
        }

        _addEvolutionButton.signal_clicked( ).connect( [ this ]( ) {
            _editingEvolutions.push_back( { "", "Level", "1" } );
            _evolutionUiDirty = true;
            if( _tabs.get_current_page( ) == 2 ) {
                rebuildEvolutionUi( );
                _evolutionUiDirty = false;
            }
            syncSpecsFromEditorState( );
            _dirty = true;
        } );
        _addLearnsetButton.signal_clicked( ).connect( [ this ]( ) {
            auto sel = _addLearnsetKindDD.get_selected( );
            auto kind = sel == 1   ? LearnsetKind::TMHM
                      : sel == 2   ? LearnsetKind::EGG
                      : sel == 3   ? LearnsetKind::TUTOR
                                   : LearnsetKind::LEVEL;
            _editingLearnset.push_back( { kind, "", 1 } );
            _learnsetUiDirty = true;
            if( _tabs.get_current_page( ) == 3 ) {
                rebuildLearnsetUi( );
                _learnsetUiDirty = false;
            }
            syncSpecsFromEditorState( );
            _dirty = true;
        } );
    }

    void pkmnDataEditor::loadRowsFromDisk( const std::string& p_dataPath ) {
        _pkmnNamesRows = readDelimitedCsv( p_dataPath + "/pkmnnames.csv", ',' );
        _pkmnCategoryRows = readDelimitedCsv( p_dataPath + "/pkmncategory.csv", ',' );
        _pkmnFlavorRows = readDelimitedCsv( p_dataPath + "/pkmnflavor.csv", ';' );
        _pkmnDataRows = readDelimitedCsv( p_dataPath + "/pkmndata.csv", ',' );
        _pkmnDescrRows = readDelimitedCsv( p_dataPath + "/pkmndescr.csv", ',' );
        _pkmnEvolvRows = readDelimitedCsv( p_dataPath + "/pkmnevolv.csv", ',' );
        _pkmnLearnsetRows = readDelimitedCsv( p_dataPath + "/pkmnlearnsets.csv", ',' );
        _pkmnFormNameRows = readDelimitedCsv( p_dataPath + "/pkmnformnames.csv", ',' );

        _pkmnNamesRowById.clear( );
        _pkmnCategoryRowById.clear( );
        _pkmnFlavorRowById.clear( );
        _pkmnDataRowById.clear( );
        _pkmnDescrRowById.clear( );
        _pkmnEvolvRowsByFromName.clear( );
        _pkmnLearnsetRowByName.clear( );
        _pkmnFormNameRowsByBaseId.clear( );

        for( size_t i = 0; i < _pkmnNamesRows.size( ); ++i ) {
            _pkmnNamesRowById[ parseId( safeGet( _pkmnNamesRows[ i ], 0 ) ) ] = i;
        }
        for( size_t i = 0; i < _pkmnCategoryRows.size( ); ++i ) {
            _pkmnCategoryRowById[ parseId( safeGet( _pkmnCategoryRows[ i ], 0 ) ) ] = i;
        }
        for( size_t i = 0; i < _pkmnFlavorRows.size( ); ++i ) {
            _pkmnFlavorRowById[ parseId( safeGet( _pkmnFlavorRows[ i ], 0 ) ) ] = i;
        }
        for( size_t i = 0; i < _pkmnDataRows.size( ); ++i ) {
            _pkmnDataRowById[ parseId( safeGet( _pkmnDataRows[ i ], 0 ) ) ] = i;
        }
        for( size_t i = 0; i < _pkmnDescrRows.size( ); ++i ) {
            _pkmnDescrRowById[ parseId( safeGet( _pkmnDescrRows[ i ], 0 ) ) ] = i;
        }
        for( size_t i = 0; i < _pkmnEvolvRows.size( ); ++i ) {
            _pkmnEvolvRowsByFromName[ toLower( safeGet( _pkmnEvolvRows[ i ], 1 ) ) ].push_back( i );
        }
        for( size_t i = 0; i < _pkmnLearnsetRows.size( ); ++i ) {
            _pkmnLearnsetRowByName[ toLower( safeGet( _pkmnLearnsetRows[ i ], 0 ) ) ] = i;
        }
        for( size_t i = 0; i < _pkmnFormNameRows.size( ); ++i ) {
            auto baseId = parseFormBaseId( safeGet( _pkmnFormNameRows[ i ], 0 ) );
            if( !baseId ) { continue; }
            _pkmnFormNameRowsByBaseId[ baseId ].push_back( i );
        }

        buildEditorChoiceLists( );
    }

    void pkmnDataEditor::reloadIfNeeded( ) {
        if( _model.m_fsdata.m_fsrootPath.empty( ) ) { return; }

        auto fsrootPath = fs::path( _model.m_fsdata.m_fsrootPath );
        auto pneoRoot
            = fsrootPath.filename( ) == "FSROOT" ? fsrootPath.parent_path( ) : fsrootPath;
        auto dataPath = ( pneoRoot / "tools" / "fsdata" / "data" ).string( );
        if( dataPath == _lastDataPath && !_allRecords.empty( ) ) { return; }

        _allRecords = pokemonCsvAdapter::load( _model.m_fsdata.m_fsrootPath );
        _lastDataPath = dataPath;
        loadRowsFromDisk( dataPath );
        _dirty = false;
    }

    void pkmnDataEditor::rebuildList( ) {
        u16 previouslySelectedId = 0;
        if( _selectedRecordIndex < _allRecords.size( ) ) {
            previouslySelectedId = _allRecords[ _selectedRecordIndex ].m_id;
        }

        while( auto* row = _recordList.get_row_at_index( 0 ) ) { _recordList.remove( *row ); }

        _filteredIndices.clear( );
        auto query = toLower( _search.get_text( ) );
        size_t selectedFilteredPos = size_t( -1 );
        for( size_t i = 0; i < _allRecords.size( ); ++i ) {
            const auto& rec = _allRecords[ i ];
            auto labelText  = std::to_string( rec.m_id ) + " - " + rec.m_name;
            auto haystack   = toLower( labelText + " " + rec.m_category + " " + rec.m_type1 + " "
                                     + rec.m_type2 );
            if( !query.empty( ) && haystack.find( query ) == std::string::npos ) { continue; }

            auto* rowBox = Gtk::make_managed<Gtk::Box>( Gtk::Orientation::HORIZONTAL );
            auto* label  = Gtk::make_managed<Gtk::Label>( labelText );
            label->set_halign( Gtk::Align::START );
            label->set_xalign( 0.0f );
            rowBox->append( *label );

            auto* row = Gtk::make_managed<Gtk::ListBoxRow>( );
            row->set_child( *rowBox );
            _recordList.append( *row );
            _filteredIndices.push_back( i );
            if( rec.m_id == previouslySelectedId ) { selectedFilteredPos = _filteredIndices.size( ) - 1; }
        }

        if( !_filteredIndices.empty( ) ) {
            size_t targetPos = 0;
            if( selectedFilteredPos != size_t( -1 ) ) { targetPos = selectedFilteredPos; }
            if( auto* target = _recordList.get_row_at_index( static_cast<int>( targetPos ) ) ) {
                _recordList.select_row( *target );
            }
        } else {
            clearDetails( );
            _selectedRecordIndex = size_t( -1 );
        }
    }

    void pkmnDataEditor::selectByFilteredPosition( size_t p_pos ) {
        if( p_pos >= _filteredIndices.size( ) ) { return; }
        auto idx = _filteredIndices[ p_pos ];
        if( idx >= _allRecords.size( ) ) { return; }
        _selectedRecordIndex = idx;
        showRecord( _allRecords[ idx ] );
    }

    void pkmnDataEditor::showRecord( const PokemonRecord& p_record ) {
        _updatingFields = true;
        _overviewNameValue.set_text( p_record.m_name );
        _overviewCategoryValue.set_text( p_record.m_category );
        setDropDownByValue( _overviewType1Value, _type1Choices, normalizeTypeToken( p_record.m_type1 ), 0 );
        setDropDownByValue( _overviewType2Value, _type2Choices,
                            p_record.m_type2.empty( ) || toLower( p_record.m_type2 ) == "none"
                                ? "-"
                                : normalizeTypeToken( p_record.m_type2 ),
                            0 );
        setDropDownByValue( _overviewExpTypeValue, _growthChoices, p_record.m_expType, 0 );
        _overviewEggCyclesValue.set_text( p_record.m_eggCycles );
        _overviewCatchRateValue.set_text( p_record.m_catchRate );
        _overviewBaseExpValue.set_text( p_record.m_baseExp );

        _statsHpValue.set_text( p_record.m_hp );
        _statsAtkValue.set_text( p_record.m_atk );
        _statsDefValue.set_text( p_record.m_def );
        _statsSpAtkValue.set_text( p_record.m_spAtk );
        _statsSpDefValue.set_text( p_record.m_spDef );
        _statsSpeedValue.set_text( p_record.m_speed );
        setDropDownByValue( _statsAbility1Value, _ability1Choices, p_record.m_ability1, 0 );
        setDropDownByValue( _statsAbility2Value, _abilityOptionalChoices,
                            p_record.m_ability2.empty( ) || toLower( p_record.m_ability2 ) == "none"
                                ? "-"
                                : p_record.m_ability2,
                            0 );
        setDropDownByValue( _statsHiddenAbilityValue, _abilityOptionalChoices,
                            p_record.m_hiddenAbility.empty( )
                                        || toLower( p_record.m_hiddenAbility ) == "none"
                                ? "-"
                                : p_record.m_hiddenAbility,
                            0 );
        setDropDownByValue( _statsGenderValue, _genderValues, p_record.m_gender, 0 );
        enforceSelectorRules( SEL_NONE );
        refreshAbilitySelectorTint( );
        _statsHeightValue.set_text( p_record.m_height );
        _statsWeightValue.set_text( p_record.m_weight );

        _editingEvolutions.clear( );
        auto evoIt = _pkmnEvolvRowsByFromName.find( toLower( p_record.m_name ) );
        if( evoIt != _pkmnEvolvRowsByFromName.end( ) ) {
            for( auto rowIdx : evoIt->second ) {
                if( rowIdx >= _pkmnEvolvRows.size( ) ) { continue; }
                auto toName = trim( safeGet( _pkmnEvolvRows[ rowIdx ], 2 ) );
                auto rawMethod = trim( safeGet( _pkmnEvolvRows[ rowIdx ], 4 ) );
                if( toName.empty( ) ) { continue; }
                EvolutionEntry entry{ toName, rawMethod, "" };
                if( rawMethod.rfind( "level:", 0 ) == 0 ) {
                    entry.m_methodDisplay = "Level";
                    entry.m_param         = trim( rawMethod.substr( 6 ) );
                } else if( rawMethod.rfind( "item:", 0 ) == 0 ) {
                    auto itemName = trim( rawMethod.substr( 5 ) );
                    if( _evoMethodRawByDisplay.count( itemName ) ) {
                        entry.m_methodDisplay = itemName;
                    } else {
                        entry.m_methodDisplay = "ItemOther";
                        entry.m_param         = itemName;
                    }
                } else {
                    if( _evoMethodRawByDisplay.count( rawMethod ) ) {
                        entry.m_methodDisplay = rawMethod;
                    } else {
                        entry.m_methodDisplay = "ItemOther";
                        entry.m_param         = rawMethod;
                    }
                }
                _editingEvolutions.push_back( std::move( entry ) );
            }
        }
        std::sort( _editingEvolutions.begin( ), _editingEvolutions.end( ),
                   []( const EvolutionEntry& p_lhs, const EvolutionEntry& p_rhs ) {
                       return pkmnDataEditor::toLower( p_lhs.m_toName )
                            < pkmnDataEditor::toLower( p_rhs.m_toName );
                   } );
        _evolutionUiDirty = true;
        if( _tabs.get_current_page( ) == 2 ) {
            rebuildEvolutionUi( );
            _evolutionUiDirty = false;
        }

        _editingLearnset.clear( );
        auto learnsetIt = _pkmnLearnsetRowByName.find( toLower( p_record.m_name ) );
        if( learnsetIt != _pkmnLearnsetRowByName.end( ) && learnsetIt->second < _pkmnLearnsetRows.size( ) ) {
            const auto& row = _pkmnLearnsetRows[ learnsetIt->second ];
            for( size_t i = 1; i < row.size( ); ++i ) {
                auto token = normalizeLearnsetToken( row[ i ] );
                if( token.empty( ) ) { continue; }
                auto parts = splitLine( token, ';' );
                if( parts.size( ) < 2 ) { continue; }
                LearnsetEntry entry;
                entry.m_move = trim( parts[ 0 ] );
                auto code    = trim( parts[ 1 ] );
                entry.m_kind = decodeLearnsetKind( code );
                entry.m_level = parseLearnsetLevel( code );
                _editingLearnset.push_back( std::move( entry ) );
            }
        }
        _learnsetUiDirty = true;
        if( _tabs.get_current_page( ) == 3 ) {
            rebuildLearnsetUi( );
            _learnsetUiDirty = false;
        }

        std::string formSpec;
        auto formsIt = _pkmnFormNameRowsByBaseId.find( p_record.m_id );
        if( formsIt != _pkmnFormNameRowsByBaseId.end( ) ) {
            for( auto rowIdx : formsIt->second ) {
                if( rowIdx >= _pkmnFormNameRows.size( ) ) { continue; }
                auto key = trim( safeGet( _pkmnFormNameRows[ rowIdx ], 0 ) );
                auto val = trim( safeGet( _pkmnFormNameRows[ rowIdx ], 1 ) );
                if( key.empty( ) ) { continue; }
                if( !formSpec.empty( ) ) { formSpec += ";"; }
                formSpec += key + ( val.empty( ) ? "" : "|" + val );
            }
        }
        _formsEditValue.set_text( formSpec );

        if( auto buffer = _textFlavorValue.get_buffer( ) ) {
            buffer->set_text( decodeEscapedNewlines( p_record.m_flavor ) );
        }
        _formsText.set_text( joinLines( p_record.m_forms ) );
        _updatingFields = false;
        auto t2sel = _overviewType2Value.get_selected( );
        _type2PrevSel = t2sel == GTK_INVALID_LIST_POSITION ? 0 : t2sel;
        captureAbilityPreviousSelections( );
    }

    void pkmnDataEditor::clearDetails( ) {
        _updatingFields = true;
        _overviewNameValue.set_text( "" );
        _overviewCategoryValue.set_text( "" );
        setDropDownByValue( _overviewType1Value, _type1Choices, "???", 0 );
        setDropDownByValue( _overviewType2Value, _type2Choices, "-", 0 );
        setDropDownByValue( _overviewExpTypeValue, _growthChoices, "", 0 );
        _overviewEggCyclesValue.set_text( "" );
        _overviewCatchRateValue.set_text( "" );
        _overviewBaseExpValue.set_text( "" );

        _statsHpValue.set_text( "" );
        _statsAtkValue.set_text( "" );
        _statsDefValue.set_text( "" );
        _statsSpAtkValue.set_text( "" );
        _statsSpDefValue.set_text( "" );
        _statsSpeedValue.set_text( "" );
        setDropDownByValue( _statsAbility1Value, _ability1Choices, "", 0 );
        setDropDownByValue( _statsAbility2Value, _abilityOptionalChoices, "-", 0 );
        setDropDownByValue( _statsHiddenAbilityValue, _abilityOptionalChoices, "-", 0 );
        setDropDownByValue( _statsGenderValue, _genderValues, "genderless", 0 );
        refreshAbilitySelectorTint( );
        _statsHeightValue.set_text( "" );
        _statsWeightValue.set_text( "" );

        _formsEditValue.set_text( "" );
        if( auto buffer = _textFlavorValue.get_buffer( ) ) { buffer->set_text( "" ); }
        _formsText.set_text( "" );
        _editingEvolutions.clear( );
        _editingLearnset.clear( );
        _evolutionUiDirty = true;
        if( _tabs.get_current_page( ) == 2 ) {
            rebuildEvolutionUi( );
            _evolutionUiDirty = false;
        }
        _learnsetUiDirty = true;
        if( _tabs.get_current_page( ) == 3 ) {
            rebuildLearnsetUi( );
            _learnsetUiDirty = false;
        }
        _updatingFields = false;
        auto t2sel = _overviewType2Value.get_selected( );
        _type2PrevSel = t2sel == GTK_INVALID_LIST_POSITION ? 0 : t2sel;
        captureAbilityPreviousSelections( );
    }

    std::string pkmnDataEditor::selectedDropDownValue( const Gtk::DropDown&            p_dd,
                                                        const std::vector<std::string>& p_values ) const {
        auto sel = p_dd.get_selected( );
        if( sel == GTK_INVALID_LIST_POSITION || sel >= p_values.size( ) ) { return ""; }
        return p_values[ sel ];
    }

    void pkmnDataEditor::setDropDownByValue( Gtk::DropDown&                  p_dd,
                                             const std::vector<std::string>& p_values,
                                             const std::string&              p_value,
                                             size_t p_default ) {
        if( p_values.empty( ) ) {
            p_dd.set_selected( GTK_INVALID_LIST_POSITION );
            return;
        }
        auto targetLower = toLower( trim( p_value ) );
        for( size_t i = 0; i < p_values.size( ); ++i ) {
            if( toLower( p_values[ i ] ) == targetLower ) {
                p_dd.set_selected( i );
                return;
            }
        }
        p_dd.set_selected( std::min( p_default, p_values.size( ) - 1 ) );
    }

    void pkmnDataEditor::handleDropDownEdited( int p_source ) {
        if( _updatingFields || _selectedRecordIndex >= _allRecords.size( ) ) { return; }
        if( p_source == SEL_TYPE2 ) {
            auto type1 = normalizeTypeToken( selectedDropDownValue( _overviewType1Value, _type1Choices ) );
            auto type2 = normalizeTypeToken( selectedDropDownValue( _overviewType2Value, _type2Choices ) );
            if( !type1.empty( ) && toLower( type1 ) == toLower( type2 ) ) {
                auto restore = _type2PrevSel < _type2Choices.size( ) ? _type2PrevSel : 0;
                _updatingFields = true;
                _overviewType2Value.set_selected( restore );
                _updatingFields = false;
                return;
            }
        }
        if( p_source == SEL_ABILITY1 || p_source == SEL_ABILITY2 || p_source == SEL_ABILITY_HIDDEN ) {
            auto rejectAndRestore = [ this ]( Gtk::DropDown& p_dd, const std::vector<std::string>& p_vals,
                                             size_t& p_prevSel ) {
                auto cur = p_dd.get_selected( );
                if( cur != GTK_INVALID_LIST_POSITION && cur < p_vals.size( )
                    && isAbilitySentinelChoice( p_vals[ cur ] ) ) {
                    auto restore = p_prevSel < p_vals.size( ) ? p_prevSel : 0;
                    if( restore < p_vals.size( ) && isAbilitySentinelChoice( p_vals[ restore ] ) ) {
                        restore = 0;
                        while( restore < p_vals.size( ) && isAbilitySentinelChoice( p_vals[ restore ] ) ) {
                            ++restore;
                        }
                        if( restore >= p_vals.size( ) ) { restore = 0; }
                    }
                    _updatingFields = true;
                    p_dd.set_selected( restore );
                    _updatingFields = false;
                    refreshAbilitySelectorTint( );
                    return true;
                }
                return false;
            };
            if( p_source == SEL_ABILITY1
                && rejectAndRestore( _statsAbility1Value, _ability1Choices, _ability1PrevSel ) ) {
                return;
            }
            if( p_source == SEL_ABILITY2
                && rejectAndRestore( _statsAbility2Value, _abilityOptionalChoices, _ability2PrevSel ) ) {
                return;
            }
            if( p_source == SEL_ABILITY_HIDDEN
                && rejectAndRestore( _statsHiddenAbilityValue, _abilityOptionalChoices,
                                     _hiddenAbilityPrevSel ) ) {
                return;
            }
        }
        enforceSelectorRules( p_source );
        refreshAbilitySelectorTint( );
        auto t2sel = _overviewType2Value.get_selected( );
        _type2PrevSel = t2sel == GTK_INVALID_LIST_POSITION ? 0 : t2sel;
        captureAbilityPreviousSelections( );
        handleFieldEdited( );
    }

    void pkmnDataEditor::enforceSelectorRules( int p_source ) {
        auto wasUpdating = _updatingFields;
        _updatingFields  = true;

        auto type1 = normalizeTypeToken( selectedDropDownValue( _overviewType1Value, _type1Choices ) );
        auto type2 = normalizeTypeToken( selectedDropDownValue( _overviewType2Value, _type2Choices ) );
        if( type1 == "-" && _type1Choices.size( ) > 1 ) {
            _overviewType1Value.set_selected( 1 );
            type1 = selectedDropDownValue( _overviewType1Value, _type1Choices );
        }
        if( p_source == SEL_TYPE1 || p_source == SEL_NONE ) { rebuildType2Choices( type1, type2 ); }
        type2 = normalizeTypeToken( selectedDropDownValue( _overviewType2Value, _type2Choices ) );
        if( !type1.empty( ) && toLower( type2 ) == toLower( type1 ) ) {
            setDropDownByValue( _overviewType2Value, _type2Choices, "-", 0 );
        }

        auto ability1 = selectedDropDownValue( _statsAbility1Value, _ability1Choices );
        auto ability2 = selectedDropDownValue( _statsAbility2Value, _abilityOptionalChoices );
        auto hidden   = selectedDropDownValue( _statsHiddenAbilityValue, _abilityOptionalChoices );

        if( ability1 == "-" && _ability1Choices.size( ) > 1 ) {
            _statsAbility1Value.set_selected( 1 );
            ability1 = selectedDropDownValue( _statsAbility1Value, _ability1Choices );
        }
        rebuildAbilityOptionalChoices( ability1, ability2, hidden );
        ability2 = selectedDropDownValue( _statsAbility2Value, _abilityOptionalChoices );
        hidden   = selectedDropDownValue( _statsHiddenAbilityValue, _abilityOptionalChoices );

        if( !ability1.empty( ) && ability2 == ability1 ) {
            setDropDownByValue( _statsAbility2Value, _abilityOptionalChoices, "-" );
            ability2 = "-";
        }
        if( !ability1.empty( ) && hidden == ability1 ) {
            setDropDownByValue( _statsHiddenAbilityValue, _abilityOptionalChoices, "-" );
            hidden = "-";
        }

        if( ability2 != "-" && hidden != "-" && ability2 == hidden ) {
            if( p_source == SEL_ABILITY_HIDDEN ) {
                setDropDownByValue( _statsAbility2Value, _abilityOptionalChoices, "-" );
            } else {
                setDropDownByValue( _statsHiddenAbilityValue, _abilityOptionalChoices, "-" );
            }
        }

        _updatingFields = wasUpdating;
    }

    void pkmnDataEditor::rebuildType2Choices( const std::string& p_excludedType,
                                              const std::string& p_preferredSelection ) {
        _type2Choices.clear( );
        _type2Choices.push_back( "-" );
        for( const auto& t : _typeAllChoices ) {
            _type2Choices.push_back( t );
        }
        _type2Model = Gtk::StringList::create( toUStrings( _type2Choices ) );
        _overviewType2Value.set_model( _type2Model );
        auto preferred = normalizeTypeToken( p_preferredSelection );
        auto excluded = normalizeTypeToken( p_excludedType );
        if( toLower( preferred ) == toLower( excluded ) ) { preferred = "-"; }
        if( preferred.empty( ) || toLower( preferred ) == "none" ) { preferred = "-"; }
        setDropDownByValue( _overviewType2Value, _type2Choices, preferred, 0 );
    }

    void pkmnDataEditor::rebuildAbilityOptionalChoices( const std::string& p_excludedAbility,
                                                         const std::string& p_preferredAbility2,
                                                         const std::string& p_preferredHidden ) {
        _abilityOptionalChoices = { ABILITY_SENTINEL_NEVER_HIDDEN, ABILITY_SENTINEL_ONLY_HIDDEN,
                                    ABILITY_SENTINEL_HIDDEN_OPTIONAL, ABILITY_SENTINEL_DIVIDER,
                                    "-" };
        for( const auto& a : _abilityAllChoices ) {
            if( toLower( a ) == toLower( p_excludedAbility ) ) { continue; }
            _abilityOptionalChoices.push_back( a );
        }
        _abilityOptionalModel = Gtk::StringList::create( toUStrings( _abilityOptionalChoices ) );
        _statsAbility2Value.set_model( _abilityOptionalModel );
        _statsHiddenAbilityValue.set_model( _abilityOptionalModel );

        auto pref2 = p_preferredAbility2;
        if( pref2.empty( ) || toLower( pref2 ) == "none"
            || toLower( pref2 ) == toLower( p_excludedAbility ) ) {
            pref2 = "-";
        }
        auto prefH = p_preferredHidden;
        if( prefH.empty( ) || toLower( prefH ) == "none"
            || toLower( prefH ) == toLower( p_excludedAbility ) ) {
            prefH = "-";
        }
        setDropDownByValue( _statsAbility2Value, _abilityOptionalChoices, pref2, 0 );
        setDropDownByValue( _statsHiddenAbilityValue, _abilityOptionalChoices, prefH, 0 );
    }

    void pkmnDataEditor::refreshAbilitySelectorTint( ) {
        auto applyClass = [ this ]( Gtk::DropDown& p_dd, const std::vector<std::string>& p_values ) {
            p_dd.remove_css_class( "ability-never-hidden" );
            p_dd.remove_css_class( "ability-hidden-only" );
            p_dd.remove_css_class( "ability-both" );
            auto value = selectedDropDownValue( p_dd, p_values );
            auto key   = toLower( trim( value ) );
            if( key.empty( ) || key == "-" || key == "none" ) { return; }
            if( isAbilitySentinelChoice( value ) ) {
                if( value == ABILITY_SENTINEL_NEVER_HIDDEN ) {
                    p_dd.add_css_class( "ability-never-hidden" );
                } else if( value == ABILITY_SENTINEL_ONLY_HIDDEN ) {
                    p_dd.add_css_class( "ability-hidden-only" );
                } else if( value == ABILITY_SENTINEL_HIDDEN_OPTIONAL ) {
                    p_dd.add_css_class( "ability-both" );
                }
                return;
            }
            auto it = _abilityCategoryByLower.find( key );
            if( it == _abilityCategoryByLower.end( ) ) { return; }
            switch( it->second ) {
            case 1: p_dd.add_css_class( "ability-never-hidden" ); break;
            case 2: p_dd.add_css_class( "ability-hidden-only" ); break;
            case 3: p_dd.add_css_class( "ability-both" ); break;
            default: break;
            }
        };
        applyClass( _statsAbility1Value, _ability1Choices );
        applyClass( _statsAbility2Value, _abilityOptionalChoices );
        applyClass( _statsHiddenAbilityValue, _abilityOptionalChoices );
    }

    void pkmnDataEditor::captureAbilityPreviousSelections( ) {
        auto firstValid = [ this ]( const std::vector<std::string>& p_vals ) {
            size_t idx = 0;
            while( idx < p_vals.size( ) && isAbilitySentinelChoice( p_vals[ idx ] ) ) { ++idx; }
            return idx < p_vals.size( ) ? idx : size_t( 0 );
        };
        auto chooseValid = [ this, &firstValid ]( const Gtk::DropDown& p_dd,
                                                  const std::vector<std::string>& p_vals ) {
            auto sel = p_dd.get_selected( );
            if( sel == GTK_INVALID_LIST_POSITION || sel >= p_vals.size( )
                || isAbilitySentinelChoice( p_vals[ sel ] ) ) {
                return firstValid( p_vals );
            }
            return size_t( sel );
        };
        _ability1PrevSel      = chooseValid( _statsAbility1Value, _ability1Choices );
        _ability2PrevSel      = chooseValid( _statsAbility2Value, _abilityOptionalChoices );
        _hiddenAbilityPrevSel = chooseValid( _statsHiddenAbilityValue, _abilityOptionalChoices );
    }

    bool pkmnDataEditor::isAbilitySentinelChoice( const std::string& p_value ) const {
        return p_value == ABILITY_SENTINEL_NEVER_HIDDEN || p_value == ABILITY_SENTINEL_ONLY_HIDDEN
            || p_value == ABILITY_SENTINEL_HIDDEN_OPTIONAL || p_value == ABILITY_SENTINEL_DIVIDER;
    }

    void pkmnDataEditor::installAbilityListFactory(
        Gtk::DropDown& p_dd, Glib::RefPtr<Gtk::SignalListItemFactory>& p_factoryOut ) {
        p_factoryOut = Gtk::SignalListItemFactory::create( );
        p_factoryOut->signal_setup( ).connect( []( const Glib::RefPtr<Gtk::ListItem>& p_item ) {
            auto* label = Gtk::make_managed<Gtk::Label>( );
            label->set_xalign( 0.0f );
            label->set_hexpand( true );
            p_item->set_child( *label );
        } );
        p_factoryOut->signal_bind( ).connect( [ this ]( const Glib::RefPtr<Gtk::ListItem>& p_item ) {
            auto* label = dynamic_cast<Gtk::Label*>( p_item->get_child( ) );
            if( !label ) { return; }

            label->remove_css_class( "ability-row-never-hidden" );
            label->remove_css_class( "ability-row-hidden-only" );
            label->remove_css_class( "ability-row-both" );
            label->remove_css_class( "ability-row-disabled" );

            auto str = std::dynamic_pointer_cast<Gtk::StringObject>( p_item->get_item( ) );
            if( !str ) {
                label->set_text( "" );
                return;
            }
            auto text = str->get_string( );
            label->set_text( text );

            auto key = toLower( trim( text ) );
            if( key.empty( ) || key == "-" || key == "none" ) { return; }
            if( text == ABILITY_SENTINEL_NEVER_HIDDEN ) {
                label->add_css_class( "ability-row-never-hidden" );
                label->add_css_class( "ability-row-disabled" );
                return;
            }
            if( text == ABILITY_SENTINEL_ONLY_HIDDEN ) {
                label->add_css_class( "ability-row-hidden-only" );
                label->add_css_class( "ability-row-disabled" );
                return;
            }
            if( text == ABILITY_SENTINEL_HIDDEN_OPTIONAL ) {
                label->add_css_class( "ability-row-both" );
                label->add_css_class( "ability-row-disabled" );
                return;
            }
            if( text == ABILITY_SENTINEL_DIVIDER ) {
                label->add_css_class( "ability-row-disabled" );
                return;
            }
            auto it = _abilityCategoryByLower.find( key );
            if( it == _abilityCategoryByLower.end( ) ) { return; }
            switch( it->second ) {
            case 1: label->add_css_class( "ability-row-never-hidden" ); break;
            case 2: label->add_css_class( "ability-row-hidden-only" ); break;
            case 3: label->add_css_class( "ability-row-both" ); break;
            default: break;
            }
        } );
        p_dd.set_list_factory( p_factoryOut );
    }

    void pkmnDataEditor::handleFieldEdited( ) {
        if( _updatingFields || _selectedRecordIndex >= _allRecords.size( ) ) { return; }

        auto& rec      = _allRecords[ _selectedRecordIndex ];
        rec.m_name     = _overviewNameValue.get_text( );
        rec.m_category = _overviewCategoryValue.get_text( );
        rec.m_type1    = normalizeTypeToken( selectedDropDownValue( _overviewType1Value, _type1Choices ) );
        rec.m_type2    = normalizeTypeToken( selectedDropDownValue( _overviewType2Value, _type2Choices ) );
        if( rec.m_type2 == "-" || rec.m_type2.empty( ) ) { rec.m_type2 = "none"; }

        rec.m_expType   = selectedDropDownValue( _overviewExpTypeValue, _growthChoices );
        rec.m_eggCycles = _overviewEggCyclesValue.get_text( );
        rec.m_catchRate = _overviewCatchRateValue.get_text( );
        rec.m_baseExp   = _overviewBaseExpValue.get_text( );

        rec.m_hp     = _statsHpValue.get_text( );
        rec.m_atk    = _statsAtkValue.get_text( );
        rec.m_def    = _statsDefValue.get_text( );
        rec.m_spAtk  = _statsSpAtkValue.get_text( );
        rec.m_spDef  = _statsSpDefValue.get_text( );
        rec.m_speed  = _statsSpeedValue.get_text( );

        rec.m_ability1
            = selectedDropDownValue( _statsAbility1Value, _ability1Choices );
        rec.m_ability2
            = selectedDropDownValue( _statsAbility2Value, _abilityOptionalChoices );
        rec.m_hiddenAbility
            = selectedDropDownValue( _statsHiddenAbilityValue, _abilityOptionalChoices );
        if( rec.m_ability2 == "-" || rec.m_ability2.empty( ) ) { rec.m_ability2 = "none"; }
        if( rec.m_hiddenAbility == "-" || rec.m_hiddenAbility.empty( ) ) { rec.m_hiddenAbility = "none"; }

        rec.m_gender = selectedDropDownValue( _statsGenderValue, _genderValues );
        rec.m_height = _statsHeightValue.get_text( );
        rec.m_weight = _statsWeightValue.get_text( );

        rec.m_formSpec      = _formsEditValue.get_text( );
        rec.m_flavor        = encodeEscapedNewlines( _textFlavorValue.get_buffer( )->get_text( ) );

        syncSpecsFromEditorState( );

        _dirty = true;
    }

    void pkmnDataEditor::applyFieldEditsToSelectedRecord( ) {
        if( _selectedRecordIndex >= _allRecords.size( ) ) { return; }
        auto& rec = _allRecords[ _selectedRecordIndex ];

        auto writeAt = []( std::vector<std::string>& p_row, size_t p_idx, const std::string& p_val ) {
            if( p_row.size( ) <= p_idx ) { p_row.resize( p_idx + 1 ); }
            p_row[ p_idx ] = p_val;
        };

        std::string oldName = rec.m_name;
        if( _pkmnNamesRowById.count( rec.m_id ) ) {
            oldName = safeGet( _pkmnNamesRows[ _pkmnNamesRowById[ rec.m_id ] ], 1 );
        }
        auto oldNameLower = toLower( oldName );

        if( _pkmnNamesRowById.count( rec.m_id ) ) {
            auto& row = _pkmnNamesRows[ _pkmnNamesRowById[ rec.m_id ] ];
            writeAt( row, 1, rec.m_name );
        }
        if( _pkmnCategoryRowById.count( rec.m_id ) ) {
            auto& row = _pkmnCategoryRows[ _pkmnCategoryRowById[ rec.m_id ] ];
            writeAt( row, 1, rec.m_category );
        }
        if( _pkmnFlavorRowById.count( rec.m_id ) ) {
            auto& row = _pkmnFlavorRows[ _pkmnFlavorRowById[ rec.m_id ] ];
            writeAt( row, 1, rec.m_flavor );
        }
        if( _pkmnDataRowById.count( rec.m_id ) ) {
            auto& row = _pkmnDataRows[ _pkmnDataRowById[ rec.m_id ] ];
            writeAt( row, 1, rec.m_expType );
            writeAt( row, 2, rec.m_eggCycles );
            writeAt( row, 3, rec.m_catchRate );
        }
        if( _pkmnDescrRowById.count( rec.m_id ) ) {
            auto& row = _pkmnDescrRows[ _pkmnDescrRowById[ rec.m_id ] ];
            writeAt( row, 1, rec.m_type1 );
            writeAt( row, 2, rec.m_type2 );
            writeAt( row, 3, rec.m_ability1 );
            writeAt( row, 4, rec.m_ability2 );
            writeAt( row, 5, rec.m_hiddenAbility );

            writeAt( row, 8, rec.m_hp );
            writeAt( row, 9, rec.m_atk );
            writeAt( row, 10, rec.m_def );
            writeAt( row, 11, rec.m_spAtk );
            writeAt( row, 12, rec.m_spDef );
            writeAt( row, 13, rec.m_speed );
            writeAt( row, 14, rec.m_baseExp );

            writeAt( row, 15, rec.m_gender );
            writeAt( row, 16, rec.m_height );
            writeAt( row, 17, rec.m_weight );
        }

        u16 nextEvoId = 0;
        for( const auto& row : _pkmnEvolvRows ) {
            nextEvoId = std::max<u16>( nextEvoId, static_cast<u16>( parseId( safeGet( row, 0 ) ) + 1 ) );
        }

        std::vector<std::vector<std::string>> newEvolvRows;
        newEvolvRows.reserve( _pkmnEvolvRows.size( ) + 4 );
        for( const auto& row : _pkmnEvolvRows ) {
            if( toLower( safeGet( row, 1 ) ) == oldNameLower ) { continue; }
            newEvolvRows.push_back( row );
        }
        {
            for( const auto& evo : _editingEvolutions ) {
                auto toName = trim( evo.m_toName );
                if( toName.empty( ) ) { continue; }
                std::string method;
                auto display = trim( evo.m_methodDisplay );
                if( display == "Level" ) {
                    auto levelText = trim( evo.m_param );
                    if( levelText.empty( ) ) { levelText = "1"; }
                    method = "level:" + levelText;
                } else if( display == "ItemOther" ) {
                    auto itemText = trim( evo.m_param );
                    if( itemText.empty( ) ) { continue; }
                    method = "item:" + itemText;
                } else if( _evoMethodRawByDisplay.count( display ) ) {
                    method = _evoMethodRawByDisplay[ display ];
                } else {
                    method = display;
                }

                std::vector<std::string> row;
                row.push_back( std::to_string( nextEvoId++ ) );
                row.push_back( rec.m_name );
                row.push_back( toName );
                row.push_back( "0" );
                row.push_back( method );
                newEvolvRows.push_back( std::move( row ) );
            }
        }
        _pkmnEvolvRows = std::move( newEvolvRows );

        std::vector<std::vector<std::string>> newLearnsetRows;
        newLearnsetRows.reserve( _pkmnLearnsetRows.size( ) + 1 );
        for( const auto& row : _pkmnLearnsetRows ) {
            if( toLower( safeGet( row, 0 ) ) == oldNameLower ) { continue; }
            newLearnsetRows.push_back( row );
        }
        {
            std::vector<std::string> row;
            row.push_back( rec.m_name );
            for( const auto& lrn : _editingLearnset ) {
                auto move = trim( lrn.m_move );
                if( move.empty( ) ) { continue; }
                std::string code;
                switch( lrn.m_kind ) {
                case LearnsetKind::TMHM: code = "200"; break;
                case LearnsetKind::EGG: code = "202"; break;
                case LearnsetKind::TUTOR: code = "201"; break;
                case LearnsetKind::LEVEL:
                default:
                    code = std::to_string( std::max( 1, lrn.m_level ) );
                    break;
                }
                row.push_back( move + ";" + code );
            }
            if( row.size( ) > 1 ) { newLearnsetRows.push_back( std::move( row ) ); }
        }
        _pkmnLearnsetRows = std::move( newLearnsetRows );

        std::vector<std::vector<std::string>> newFormNameRows;
        newFormNameRows.reserve( _pkmnFormNameRows.size( ) + 4 );
        for( const auto& row : _pkmnFormNameRows ) {
            if( parseFormBaseId( safeGet( row, 0 ) ) == rec.m_id ) { continue; }
            newFormNameRows.push_back( row );
        }
        {
            auto chunks = splitLine( rec.m_formSpec, ';' );
            for( const auto& rawChunk : chunks ) {
                auto chunk = trim( rawChunk );
                if( chunk.empty( ) ) { continue; }
                auto pipePos = chunk.find( '|' );
                auto formKey = trim( pipePos == std::string::npos ? chunk : chunk.substr( 0, pipePos ) );
                auto formName = trim( pipePos == std::string::npos ? "" : chunk.substr( pipePos + 1 ) );
                if( formKey.empty( ) || parseFormBaseId( formKey ) != rec.m_id ) { continue; }
                std::vector<std::string> row;
                row.push_back( formKey );
                row.push_back( formName );
                newFormNameRows.push_back( std::move( row ) );
            }
        }
        _pkmnFormNameRows = std::move( newFormNameRows );
    }

    pkmnDataEditor::LearnsetKind pkmnDataEditor::decodeLearnsetKind( const std::string& p_code ) {
        if( p_code == "200" ) { return LearnsetKind::TMHM; }
        if( p_code == "201" ) { return LearnsetKind::TUTOR; }
        if( p_code == "202" ) { return LearnsetKind::EGG; }
        return LearnsetKind::LEVEL;
    }

    int pkmnDataEditor::parseLearnsetLevel( const std::string& p_code ) {
        try {
            auto val = std::stoi( p_code );
            return std::max( 1, val );
        } catch( ... ) { return 1; }
    }

    void pkmnDataEditor::buildEditorChoiceLists( ) {
        std::set<std::string> typeSet;
        for( const auto& row : _pkmnDescrRows ) {
            auto t1 = normalizeTypeToken( safeGet( row, 1 ) );
            auto t2 = normalizeTypeToken( safeGet( row, 2 ) );
            if( !t1.empty( ) && toLower( t1 ) != "none" ) { typeSet.insert( t1 ); }
            if( !t2.empty( ) && toLower( t2 ) != "none" ) { typeSet.insert( t2 ); }
        }
        typeSet.insert( "???" );
        _type1Choices.assign( typeSet.begin( ), typeSet.end( ) );
        _typeAllChoices = _type1Choices;
        _type2Choices = { "-" };
        _type2Choices.insert( _type2Choices.end( ), _typeAllChoices.begin( ), _typeAllChoices.end( ) );
        _type1Model = Gtk::StringList::create( toUStrings( _type1Choices ) );
        _type2Model = Gtk::StringList::create( toUStrings( _type2Choices ) );
        _overviewType1Value.set_model( _type1Model );
        _overviewType2Value.set_model( _type2Model );

        std::set<std::string> growthSet;
        for( const auto& row : _pkmnDataRows ) {
            auto growth = trim( safeGet( row, 1 ) );
            if( !growth.empty( ) ) { growthSet.insert( growth ); }
        }
        _growthChoices.assign( growthSet.begin( ), growthSet.end( ) );
        _growthModel = Gtk::StringList::create( toUStrings( _growthChoices ) );
        _overviewExpTypeValue.set_model( _growthModel );

        std::set<std::string> abilitySet;
        auto abilityRows = readDelimitedCsv( _lastDataPath + "/abtynames.csv", ',' );
        for( const auto& row : abilityRows ) {
            auto name = trim( safeGet( row, 1 ) );
            if( !name.empty( ) ) { abilitySet.insert( name ); }
        }
        if( abilitySet.empty( ) ) {
            for( const auto& row : _pkmnDescrRows ) {
                for( size_t i = 3; i <= 5; ++i ) {
                    auto name = trim( safeGet( row, i ) );
                    if( !name.empty( ) && toLower( name ) != "none" ) { abilitySet.insert( name ); }
                }
            }
        }
        _abilityAllChoices.assign( abilitySet.begin( ), abilitySet.end( ) );
        _ability1Choices = _abilityAllChoices;
        _ability1Choices.insert( _ability1Choices.begin( ),
                                 { ABILITY_SENTINEL_NEVER_HIDDEN, ABILITY_SENTINEL_ONLY_HIDDEN,
                                   ABILITY_SENTINEL_HIDDEN_OPTIONAL } );
        _ability1Choices.insert( _ability1Choices.begin( ) + 3, ABILITY_SENTINEL_DIVIDER );
        _abilityOptionalChoices = { ABILITY_SENTINEL_NEVER_HIDDEN, ABILITY_SENTINEL_ONLY_HIDDEN,
                                    ABILITY_SENTINEL_HIDDEN_OPTIONAL, ABILITY_SENTINEL_DIVIDER,
                                    "-" };
        _abilityOptionalChoices.insert( _abilityOptionalChoices.end( ), _abilityAllChoices.begin( ),
                                        _abilityAllChoices.end( ) );

        std::unordered_set<std::string> regularAbilities;
        std::unordered_set<std::string> hiddenAbilities;
        for( const auto& row : _pkmnDescrRows ) {
            for( size_t i : { size_t( 3 ), size_t( 4 ) } ) {
                auto name = toLower( trim( safeGet( row, i ) ) );
                if( !name.empty( ) && name != "none" ) { regularAbilities.insert( name ); }
            }
            auto hidden = toLower( trim( safeGet( row, 5 ) ) );
            if( !hidden.empty( ) && hidden != "none" ) { hiddenAbilities.insert( hidden ); }
        }
        _abilityCategoryByLower.clear( );
        for( const auto& ability : _ability1Choices ) {
            auto key = toLower( ability );
            auto reg = regularAbilities.count( key ) > 0;
            auto hid = hiddenAbilities.count( key ) > 0;
            _abilityCategoryByLower[ key ] = reg && hid ? 3 : reg ? 1 : hid ? 2 : 0;
        }

        _ability1Model = Gtk::StringList::create( toUStrings( _ability1Choices ) );
        _abilityOptionalModel = Gtk::StringList::create( toUStrings( _abilityOptionalChoices ) );
        _statsAbility1Value.set_model( _ability1Model );
        _statsAbility2Value.set_model( _abilityOptionalModel );
        _statsHiddenAbilityValue.set_model( _abilityOptionalModel );

        _genderLabels = { "Equal (1:1)",
                          "Female-Biased (1:3)",
                          "Female-Dominant (1:7)",
                          "Female-Only",
                          "Male-Biased (3:1)",
                          "Male-Dominant (7:1)",
                          "Male-Only",
                          "Genderless/Unknown" };
        _genderValues = { "1m1f", "1m3f", "1m7f", "always female",
                          "3m1f", "7m1f", "always male", "genderless" };
        _genderModel = Gtk::StringList::create( toUStrings( _genderLabels ) );
        _statsGenderValue.set_model( _genderModel );
        captureAbilityPreviousSelections( );
        refreshAbilitySelectorTint( );

        _pokemonChoices.clear( );
        for( const auto& rec : _allRecords ) {
            _pokemonChoices.push_back( std::to_string( rec.m_id ) + " - " + rec.m_name );
        }
        _pokemonChoicesModel = Gtk::StringList::create( toUStrings( _pokemonChoices ) );

        _moveChoices.clear( );
        _moveIdByTokenLower.clear( );
        _moveTokenById.clear( );
        _moveNameKeyById.clear( );
        _moveChoiceIdByIndex.clear( );
        _moveChoiceIndexById.clear( );

        auto moveDataRows = readDelimitedCsv( _lastDataPath + "/movedata.csv", ',' );
        for( const auto& row : moveDataRows ) {
            auto idText = trim( safeGet( row, 0 ) );
            auto token  = trim( safeGet( row, 6 ) );
            if( idText.empty( ) || token.empty( ) ) { continue; }
            try {
                auto id = std::stoi( idText );
                _moveTokenById[ id ] = token;
                _moveIdByTokenLower[ toLower( token ) ] = id;
            } catch( ... ) { }
        }

        auto moveRows = readDelimitedCsv( _lastDataPath + "/movenames.csv", ',' );
        for( const auto& row : moveRows ) {
            auto id   = trim( safeGet( row, 0 ) );
            auto name = trim( safeGet( row, 1 ) );
            if( id.empty( ) || name.empty( ) ) { continue; }
            try {
                auto idNum = std::stoi( id );
                _moveChoices.push_back( id + " - " + name );
                _moveChoiceIdByIndex.push_back( idNum );
                _moveChoiceIndexById[ idNum ] = _moveChoices.size( ) - 1;
                _moveNameKeyById[ idNum ]     = canonicalMoveKey( name );
            } catch( ... ) { }
        }
        _moveChoicesModel = Gtk::StringList::create( toUStrings( _moveChoices ) );

        _tmhmInfoByMoveLower.clear( );
        auto tmhmRows = readDelimitedCsv( _lastDataPath + "/itemdata_tmhm.csv", ',' );
        for( const auto& row : tmhmRows ) {
            auto typeText = trim( safeGet( row, 1 ) );
            auto numText  = trim( safeGet( row, 2 ) );
            auto moveName = trim( safeGet( row, 3 ) );
            if( moveName.empty( ) ) { continue; }
            int type = 99;
            int num  = 9999;
            try { type = std::stoi( typeText ); } catch( ... ) { continue; }
            try { num = std::stoi( numText ); } catch( ... ) { continue; }

            TmhmInfo info;
            if( type == 0 ) {
                info.m_group = 1;
                info.m_label = std::string( "HM" ) + ( num < 10 ? "0" : "" ) + std::to_string( num );
            } else {
                info.m_group = 0;
                info.m_label = std::string( "TM" ) + ( num < 10 ? "0" : "" ) + std::to_string( num );
            }
            info.m_number = num;

            auto key = canonicalMoveKey( moveName );
            if( !_tmhmInfoByMoveLower.count( key )
                || std::tie( info.m_group, info.m_number )
                       < std::tie( _tmhmInfoByMoveLower[ key ].m_group,
                                   _tmhmInfoByMoveLower[ key ].m_number ) ) {
                _tmhmInfoByMoveLower[ key ] = info;
            }
        }

        _evoMethodChoices.clear( );
        _evoMethodRawByDisplay.clear( );
        _evoMethodChoices.push_back( "Level" );
        _evoMethodRawByDisplay[ "Level" ] = "level:1";

        std::set<std::string> rawMethods;
        for( const auto& row : _pkmnEvolvRows ) {
            auto raw = trim( safeGet( row, 4 ) );
            if( !raw.empty( ) ) { rawMethods.insert( raw ); }
        }
        for( const auto& raw : rawMethods ) {
            if( raw.rfind( "level:", 0 ) == 0 ) { continue; }
            if( raw.rfind( "item:", 0 ) == 0 ) {
                auto display = trim( raw.substr( 5 ) );
                if( display.empty( ) ) { continue; }
                if( !_evoMethodRawByDisplay.count( display ) ) {
                    _evoMethodChoices.push_back( display );
                    _evoMethodRawByDisplay[ display ] = raw;
                }
                continue;
            }
            if( !_evoMethodRawByDisplay.count( raw ) ) {
                _evoMethodChoices.push_back( raw );
                _evoMethodRawByDisplay[ raw ] = raw;
            }
        }
        _evoMethodChoices.push_back( "ItemOther" );
        _evoMethodChoicesModel = Gtk::StringList::create( toUStrings( _evoMethodChoices ) );
    }

    void pkmnDataEditor::syncSpecsFromEditorState( ) {
        if( _selectedRecordIndex >= _allRecords.size( ) ) { return; }
        auto& rec = _allRecords[ _selectedRecordIndex ];

        std::string evoSpec;
        for( const auto& evo : _editingEvolutions ) {
            auto toName = trim( evo.m_toName );
            if( toName.empty( ) ) { continue; }
            std::string method;
            if( evo.m_methodDisplay == "Level" ) {
                auto lv = trim( evo.m_param );
                if( lv.empty( ) ) { lv = "1"; }
                method = "level:" + lv;
            } else if( evo.m_methodDisplay == "ItemOther" ) {
                auto txt = trim( evo.m_param );
                if( txt.empty( ) ) { continue; }
                method = "item:" + txt;
            } else if( _evoMethodRawByDisplay.count( evo.m_methodDisplay ) ) {
                method = _evoMethodRawByDisplay[ evo.m_methodDisplay ];
            } else {
                method = evo.m_methodDisplay;
            }
            if( !evoSpec.empty( ) ) { evoSpec += ";"; }
            evoSpec += toName + "|" + method;
        }

        std::string learnsetSpec;
        for( const auto& lrn : _editingLearnset ) {
            auto move = trim( lrn.m_move );
            if( move.empty( ) ) { continue; }
            std::string code;
            switch( lrn.m_kind ) {
            case LearnsetKind::TMHM: code = "200"; break;
            case LearnsetKind::EGG: code = "202"; break;
            case LearnsetKind::TUTOR: code = "201"; break;
            case LearnsetKind::LEVEL:
            default: code = std::to_string( std::max( 1, lrn.m_level ) ); break;
            }
            if( !learnsetSpec.empty( ) ) { learnsetSpec += ","; }
            learnsetSpec += move + ";" + code;
        }

        rec.m_evolutionSpec = evoSpec;
        rec.m_learnsetSpec  = learnsetSpec;
    }

    void pkmnDataEditor::rebuildEvolutionUi( ) {
        auto wasUpdating = _updatingFields;
        _updatingFields  = true;
        while( auto* row = _evolutionList.get_row_at_index( 0 ) ) { _evolutionList.remove( *row ); }

        for( size_t idx = 0; idx < _editingEvolutions.size( ); ++idx ) {
            auto* rowBox = Gtk::make_managed<Gtk::Box>( Gtk::Orientation::HORIZONTAL );
            rowBox->set_spacing( MARGIN );
            rowBox->set_margin_start( MARGIN );
            rowBox->set_margin_end( MARGIN );
            rowBox->set_margin_top( MARGIN / 2 );
            rowBox->set_margin_bottom( MARGIN / 2 );

            auto* pkmnDD  = Gtk::make_managed<Gtk::DropDown>( );
            pkmnDD->set_model( _pokemonChoicesModel );
            pkmnDD->set_hexpand( true );
            size_t selectedPkmn = 0;
            auto targetLower    = toLower( _editingEvolutions[ idx ].m_toName );
            for( size_t i = 0; i < _allRecords.size( ); ++i ) {
                if( toLower( _allRecords[ i ].m_name ) == targetLower ) {
                    selectedPkmn = i;
                    break;
                }
            }
            pkmnDD->set_selected( selectedPkmn );
            pkmnDD->property_selected_item( ).signal_changed( ).connect( [ this, idx, pkmnDD ]( ) {
                if( _updatingFields ) { return; }
                auto sel = pkmnDD->get_selected( );
                if( idx >= _editingEvolutions.size( ) || sel == GTK_INVALID_LIST_POSITION
                    || sel >= _allRecords.size( ) ) {
                    return;
                }
                _editingEvolutions[ idx ].m_toName = _allRecords[ sel ].m_name;
                syncSpecsFromEditorState( );
                _dirty = true;
            } );
            rowBox->append( *pkmnDD );

            auto* methodDD  = Gtk::make_managed<Gtk::DropDown>( );
            methodDD->set_model( _evoMethodChoicesModel );
            size_t methodIdx = 0;
            for( size_t i = 0; i < _evoMethodChoices.size( ); ++i ) {
                if( _evoMethodChoices[ i ] == _editingEvolutions[ idx ].m_methodDisplay ) {
                    methodIdx = i;
                    break;
                }
            }
            methodDD->set_selected( methodIdx );
            rowBox->append( *methodDD );

            auto* paramEntry = Gtk::make_managed<Gtk::Entry>( );
            paramEntry->set_text( _editingEvolutions[ idx ].m_param );
            auto isLevelMethod = _editingEvolutions[ idx ].m_methodDisplay == "Level";
            auto isItemOther   = _editingEvolutions[ idx ].m_methodDisplay == "ItemOther";
            paramEntry->set_visible( isLevelMethod || isItemOther );
            paramEntry->set_placeholder_text( isLevelMethod ? "Level" : "Item name" );
            paramEntry->set_width_chars( 10 );
            paramEntry->signal_changed( ).connect( [ this, idx, paramEntry ]( ) {
                if( _updatingFields || idx >= _editingEvolutions.size( ) ) { return; }
                _editingEvolutions[ idx ].m_param = paramEntry->get_text( );
                syncSpecsFromEditorState( );
                _dirty = true;
            } );
            rowBox->append( *paramEntry );

            methodDD->property_selected_item( ).signal_changed( ).connect(
                [ this, idx, methodDD, paramEntry ]( ) {
                    if( _updatingFields || idx >= _editingEvolutions.size( ) ) { return; }
                    auto sel = methodDD->get_selected( );
                    if( sel == GTK_INVALID_LIST_POSITION || sel >= _evoMethodChoices.size( ) ) {
                        return;
                    }
                    _editingEvolutions[ idx ].m_methodDisplay = _evoMethodChoices[ sel ];
                    auto isLevel = _editingEvolutions[ idx ].m_methodDisplay == "Level";
                    auto isOther = _editingEvolutions[ idx ].m_methodDisplay == "ItemOther";
                    if( isLevel && _editingEvolutions[ idx ].m_param.empty( ) ) {
                        _editingEvolutions[ idx ].m_param = "1";
                    }
                    if( !isLevel && !isOther ) { _editingEvolutions[ idx ].m_param.clear( ); }
                    paramEntry->set_visible( isLevel || isOther );
                    paramEntry->set_placeholder_text( isLevel ? "Level" : "Item name" );
                    paramEntry->set_text( _editingEvolutions[ idx ].m_param );
                    syncSpecsFromEditorState( );
                    _dirty = true;
                } );

            auto* delButton = Gtk::make_managed<Gtk::Button>( "x" );
            delButton->signal_clicked( ).connect( [ this, idx ]( ) {
                if( _updatingFields || idx >= _editingEvolutions.size( ) ) { return; }
                _editingEvolutions.erase( _editingEvolutions.begin( ) + idx );
                rebuildEvolutionUi( );
                syncSpecsFromEditorState( );
                _dirty = true;
            } );
            rowBox->append( *delButton );

            auto* row = Gtk::make_managed<Gtk::ListBoxRow>( );
            row->set_child( *rowBox );
            _evolutionList.append( *row );
        }
        _updatingFields = wasUpdating;
    }

    void pkmnDataEditor::rebuildLearnsetUi( ) {
        auto wasUpdating = _updatingFields;
        _updatingFields  = true;

        std::sort( _editingLearnset.begin( ), _editingLearnset.end( ),
                   [ this ]( const LearnsetEntry& p_lhs, const LearnsetEntry& p_rhs ) {
                       auto kindOrder = []( LearnsetKind p_kind ) {
                           switch( p_kind ) {
                           case LearnsetKind::LEVEL: return 0;
                           case LearnsetKind::TMHM: return 1;
                           case LearnsetKind::EGG: return 2;
                           case LearnsetKind::TUTOR: return 3;
                           }
                           return 4;
                       };
                       if( kindOrder( p_lhs.m_kind ) != kindOrder( p_rhs.m_kind ) ) {
                           return kindOrder( p_lhs.m_kind ) < kindOrder( p_rhs.m_kind );
                       }
                       if( p_lhs.m_kind == LearnsetKind::LEVEL && p_lhs.m_level != p_rhs.m_level ) {
                           return p_lhs.m_level < p_rhs.m_level;
                       }
                       auto lhsToken = pkmnDataEditor::toLower( p_lhs.m_move );
                       auto rhsToken = pkmnDataEditor::toLower( p_rhs.m_move );
                       auto lhsMoveId = _moveIdByTokenLower.count( lhsToken )
                                          ? _moveIdByTokenLower.at( lhsToken )
                                          : 99999;
                       auto rhsMoveId = _moveIdByTokenLower.count( rhsToken )
                                          ? _moveIdByTokenLower.at( rhsToken )
                                          : 99999;
                       if( p_lhs.m_kind == LearnsetKind::TMHM ) {
                           auto lhsNameKey = _moveNameKeyById.count( lhsMoveId )
                                                ? _moveNameKeyById.at( lhsMoveId )
                                                : std::string{ };
                           auto rhsNameKey = _moveNameKeyById.count( rhsMoveId )
                                                ? _moveNameKeyById.at( rhsMoveId )
                                                : std::string{ };
                           auto lhsTm = _tmhmInfoByMoveLower.count( lhsNameKey )
                                            ? _tmhmInfoByMoveLower.at( lhsNameKey )
                                            : TmhmInfo{ };
                           auto rhsTm = _tmhmInfoByMoveLower.count( rhsNameKey )
                                            ? _tmhmInfoByMoveLower.at( rhsNameKey )
                                            : TmhmInfo{ };
                           if( lhsTm.m_group != rhsTm.m_group ) {
                               return lhsTm.m_group < rhsTm.m_group;
                           }
                           if( lhsTm.m_number != rhsTm.m_number ) {
                               return lhsTm.m_number < rhsTm.m_number;
                           }
                       }
                       if( lhsMoveId != rhsMoveId ) { return lhsMoveId < rhsMoveId; }
                       return lhsToken < rhsToken;
                   } );

        std::vector<size_t> levelIdx, tmhmIdx, eggIdx, tutorIdx;
        levelIdx.reserve( _editingLearnset.size( ) );
        tmhmIdx.reserve( _editingLearnset.size( ) );
        eggIdx.reserve( _editingLearnset.size( ) );
        tutorIdx.reserve( _editingLearnset.size( ) );
        for( size_t i = 0; i < _editingLearnset.size( ); ++i ) {
            switch( _editingLearnset[ i ].m_kind ) {
            case LearnsetKind::LEVEL: levelIdx.push_back( i ); break;
            case LearnsetKind::TMHM: tmhmIdx.push_back( i ); break;
            case LearnsetKind::EGG: eggIdx.push_back( i ); break;
            case LearnsetKind::TUTOR: tutorIdx.push_back( i ); break;
            }
        }

        auto getPool = [ this ]( int p_section ) -> std::vector<LearnsetRowWidgets>& {
            switch( p_section ) {
            case 0: return _learnsetLevelPool;
            case 1: return _learnsetTmhmPool;
            case 2: return _learnsetEggPool;
            default: return _learnsetTutorPool;
            }
        };
        auto getList = [ this ]( int p_section ) -> Gtk::ListBox& {
            switch( p_section ) {
            case 0: return _learnsetLevelList;
            case 1: return _learnsetTmhmList;
            case 2: return _learnsetEggList;
            default: return _learnsetTutorList;
            }
        };

        auto ensureRows = [ this, &getPool, &getList ]( int p_section, size_t p_count ) {
            auto& pool = getPool( p_section );
            auto& list = getList( p_section );
            while( pool.size( ) < p_count ) {
                auto slot = pool.size( );
                auto* rowBox = Gtk::make_managed<Gtk::Box>( Gtk::Orientation::HORIZONTAL );
                rowBox->set_spacing( 4 );
                rowBox->set_margin_start( 4 );
                rowBox->set_margin_end( 4 );
                rowBox->set_margin_top( 2 );
                rowBox->set_margin_bottom( 2 );

                auto* kindLabel = Gtk::make_managed<Gtk::Label>( );
                kindLabel->set_width_chars( 6 );
                kindLabel->set_xalign( 0.0f );
                rowBox->append( *kindLabel );

                auto* levelEntry = Gtk::make_managed<Gtk::Entry>( );
                levelEntry->set_width_chars( 4 );
                rowBox->append( *levelEntry );

                auto* moveDD = Gtk::make_managed<Gtk::DropDown>( );
                moveDD->set_model( _moveChoicesModel );
                moveDD->set_hexpand( true );
                rowBox->append( *moveDD );

                auto* delButton = Gtk::make_managed<Gtk::Button>( "x" );
                rowBox->append( *delButton );

                auto* row = Gtk::make_managed<Gtk::ListBoxRow>( );
                row->set_child( *rowBox );
                list.append( *row );
                levelEntry->signal_changed( ).connect(
                    [ this, p_section, slot, levelEntry ]( ) {
                        if( _updatingFields ) { return; }
                        auto* poolPtr = p_section == 0   ? &_learnsetLevelPool
                                      : p_section == 1 ? &_learnsetTmhmPool
                                      : p_section == 2 ? &_learnsetEggPool
                                                       : &_learnsetTutorPool;
                        auto& pool = *poolPtr;
                        if( slot >= pool.size( ) ) { return; }
                        auto dataIdx = pool[ slot ].m_dataIndex;
                        if( dataIdx >= _editingLearnset.size( ) ) { return; }
                        try {
                            _editingLearnset[ dataIdx ].m_level
                                = std::max( 1, std::stoi( levelEntry->get_text( ) ) );
                            syncSpecsFromEditorState( );
                            _dirty = true;
                        } catch( ... ) { }
                    } );
                moveDD->property_selected_item( ).signal_changed( ).connect(
                    [ this, p_section, slot, moveDD ]( ) {
                        if( _updatingFields ) { return; }
                        auto* poolPtr = p_section == 0   ? &_learnsetLevelPool
                                      : p_section == 1 ? &_learnsetTmhmPool
                                      : p_section == 2 ? &_learnsetEggPool
                                                       : &_learnsetTutorPool;
                        auto& pool = *poolPtr;
                        if( slot >= pool.size( ) ) { return; }
                        auto dataIdx = pool[ slot ].m_dataIndex;
                        if( dataIdx >= _editingLearnset.size( ) ) { return; }
                        auto sel = moveDD->get_selected( );
                        if( sel == GTK_INVALID_LIST_POSITION || sel >= _moveChoiceIdByIndex.size( ) ) {
                            return;
                        }
                        auto moveId = _moveChoiceIdByIndex[ sel ];
                        if( !_moveTokenById.count( moveId ) ) { return; }
                        _editingLearnset[ dataIdx ].m_move = _moveTokenById[ moveId ];
                        syncSpecsFromEditorState( );
                        _dirty = true;
                    } );
                delButton->signal_clicked( ).connect( [ this, p_section, slot ]( ) {
                    if( _updatingFields ) { return; }
                    auto* poolPtr = p_section == 0   ? &_learnsetLevelPool
                                  : p_section == 1 ? &_learnsetTmhmPool
                                  : p_section == 2 ? &_learnsetEggPool
                                                   : &_learnsetTutorPool;
                    auto& pool = *poolPtr;
                    if( slot >= pool.size( ) ) { return; }
                    auto dataIdx = pool[ slot ].m_dataIndex;
                    if( dataIdx >= _editingLearnset.size( ) ) { return; }
                    _editingLearnset.erase( _editingLearnset.begin( ) + dataIdx );
                    rebuildLearnsetUi( );
                    syncSpecsFromEditorState( );
                    _dirty = true;
                } );

                pool.push_back( { row, kindLabel, levelEntry, moveDD, delButton, size_t( -1 ) } );
            }
        };

        auto updateSection = [ this, &ensureRows, &getPool ]( int p_section,
                                                               const std::vector<size_t>& p_indices ) {
            ensureRows( p_section, p_indices.size( ) );
            auto& pool = getPool( p_section );
            for( size_t i = 0; i < p_indices.size( ); ++i ) {
                auto dataIdx        = p_indices[ i ];
                pool[ i ].m_dataIndex = dataIdx;
                auto& entry         = _editingLearnset[ dataIdx ];

                pool[ i ].m_levelEntry->set_visible( entry.m_kind == LearnsetKind::LEVEL );
                if( entry.m_kind == LearnsetKind::LEVEL ) {
                    pool[ i ].m_kindLabel->set_text( "Lv" );
                    pool[ i ].m_levelEntry->set_text( std::to_string( std::max( 1, entry.m_level ) ) );
                } else if( entry.m_kind == LearnsetKind::TMHM ) {
                    auto moveId = _moveIdByTokenLower.count( toLower( entry.m_move ) )
                                      ? _moveIdByTokenLower.at( toLower( entry.m_move ) )
                                      : -1;
                    auto key = _moveNameKeyById.count( moveId ) ? _moveNameKeyById.at( moveId )
                                                                 : std::string{ };
                    if( _tmhmInfoByMoveLower.count( key ) ) {
                        pool[ i ].m_kindLabel->set_text( _tmhmInfoByMoveLower.at( key ).m_label );
                    } else {
                        pool[ i ].m_kindLabel->set_text( "TM" );
                    }
                    pool[ i ].m_levelEntry->set_text( "" );
                } else if( entry.m_kind == LearnsetKind::EGG ) {
                    pool[ i ].m_kindLabel->set_text( "Egg" );
                    pool[ i ].m_levelEntry->set_text( "" );
                } else {
                    pool[ i ].m_kindLabel->set_text( "Tutor" );
                    pool[ i ].m_levelEntry->set_text( "" );
                }

                size_t selectedMove = 0;
                auto tokenLower = toLower( entry.m_move );
                if( _moveIdByTokenLower.count( tokenLower ) ) {
                    auto moveId = _moveIdByTokenLower.at( tokenLower );
                    if( _moveChoiceIndexById.count( moveId ) ) {
                        selectedMove = _moveChoiceIndexById.at( moveId );
                    }
                }
                pool[ i ].m_moveDD->set_selected( selectedMove );
                pool[ i ].m_row->set_visible( true );
            }
            for( size_t i = p_indices.size( ); i < pool.size( ); ++i ) {
                pool[ i ].m_dataIndex = size_t( -1 );
                pool[ i ].m_row->set_visible( false );
            }
        };

        updateSection( 0, levelIdx );
        updateSection( 1, tmhmIdx );
        updateSection( 2, eggIdx );
        updateSection( 3, tutorIdx );

        _learnsetLevelEmpty.set_visible( levelIdx.empty( ) );
        _learnsetTmhmEmpty.set_visible( tmhmIdx.empty( ) );
        _learnsetEggEmpty.set_visible( eggIdx.empty( ) );
        _learnsetTutorEmpty.set_visible( tutorIdx.empty( ) );
        _updatingFields = wasUpdating;
    }

    bool pkmnDataEditor::writeCurrentRowsToDisk( const std::string& p_dataPath ) {
        auto ok1 = writeDelimitedCsv( p_dataPath + "/pkmnnames.csv", _pkmnNamesRows, ',' );
        auto ok2 = writeDelimitedCsv( p_dataPath + "/pkmncategory.csv", _pkmnCategoryRows, ',' );
        auto ok3 = writeDelimitedCsv( p_dataPath + "/pkmnflavor.csv", _pkmnFlavorRows, ';' );
        auto ok4 = writeDelimitedCsv( p_dataPath + "/pkmndata.csv", _pkmnDataRows, ',' );
        auto ok5 = writeDelimitedCsv( p_dataPath + "/pkmndescr.csv", _pkmnDescrRows, ',' );
        auto ok6 = writeDelimitedCsv( p_dataPath + "/pkmnevolv.csv", _pkmnEvolvRows, ',' );
        auto ok7 = writeDelimitedCsv( p_dataPath + "/pkmnlearnsets.csv", _pkmnLearnsetRows, ',' );
        auto ok8 = writeDelimitedCsv( p_dataPath + "/pkmnformnames.csv", _pkmnFormNameRows, ',' );
        return ok1 && ok2 && ok3 && ok4 && ok5 && ok6 && ok7 && ok8;
    }

    bool pkmnDataEditor::saveToCsv( ) {
        reloadIfNeeded( );
        if( _lastDataPath.empty( ) ) { return false; }
        if( !_dirty ) { return true; }

        applyFieldEditsToSelectedRecord( );
        if( !writeCurrentRowsToDisk( _lastDataPath ) ) { return false; }

        loadRowsFromDisk( _lastDataPath );
        _dirty = false;
        return true;
    }

    bool pkmnDataEditor::isDirty( ) const {
        return _dirty;
    }

    void pkmnDataEditor::redraw( ) {
        reloadIfNeeded( );
        rebuildList( );
    }
} // namespace UI
