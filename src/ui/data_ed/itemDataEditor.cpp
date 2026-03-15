#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <sstream>
#include <utility>
#include <filesystem>

#include <gtkmm/box.h>

#include "../../defines.h"
#include "../root.h"
#include "itemDataEditor.h"

namespace UI {
    namespace fs = std::filesystem;

    namespace {
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
    } // namespace

    std::vector<ItemRecord> itemCsvAdapter::load( const std::string& p_fsrootPath ) {
        auto fsrootPath = fs::path( p_fsrootPath );
        auto pneoRoot
            = fsrootPath.filename( ) == "FSROOT" ? fsrootPath.parent_path( ) : fsrootPath;
        auto dataPath = pneoRoot / "tools" / "fsdata" / "data";

        auto itemNamesPath    = dataPath / "itemnames.csv";
        auto itemDataPath     = dataPath / "itemdata.csv";
        auto itemFlavorPath   = dataPath / "itemflavor.csv";
        auto itemTmhmPath     = dataPath / "itemdata_tmhm.csv";
        auto itemMedicinePath = dataPath / "itemdata_medicine.csv";
        auto itemFormePath    = dataPath / "itemdata_formechange.csv";

        std::map<u16, ItemRecord> records;

        {
            std::ifstream f( itemNamesPath );
            std::string   line;
            while( std::getline( f, line ) ) {
                auto cols = splitLine( normalizeLine( line ), ',' );
                if( cols.empty( ) ) { continue; }
                auto id   = parseId( safeGet( cols, 0 ) );
                auto& rec = records[ id ];
                rec.m_id  = id;
                rec.m_itemType = safeGet( cols, 1 );
                rec.m_name = safeGet( cols, 2 );
                if( rec.m_name.empty( ) ) { rec.m_name = safeGet( cols, 1 ); }
            }
        }

        {
            std::ifstream f( itemFlavorPath );
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
            std::ifstream f( itemDataPath );
            std::string   line;
            while( std::getline( f, line ) ) {
                auto cols = splitLine( normalizeLine( line ), ',' );
                if( cols.empty( ) ) { continue; }
                auto id   = parseId( safeGet( cols, 0 ) );
                auto& rec = records[ id ];
                rec.m_id  = id;
                rec.m_sellPrice = safeGet( cols, 2 );
                rec.m_effect    = safeGet( cols, 3 );
                rec.m_param1    = safeGet( cols, 4 );
                rec.m_param2    = safeGet( cols, 5 );
                rec.m_param3    = safeGet( cols, 6 );
            }
        }

        {
            std::ifstream f( itemTmhmPath );
            std::string   line;
            while( std::getline( f, line ) ) {
                auto cols = splitLine( normalizeLine( line ), ',' );
                if( cols.empty( ) ) { continue; }
                auto id   = parseId( safeGet( cols, 0 ) );
                auto& rec = records[ id ];
                rec.m_id  = id;
                rec.m_tmhmType   = safeGet( cols, 1 );
                rec.m_tmhmNumber = safeGet( cols, 2 );
                rec.m_tmhmMove   = safeGet( cols, 3 );
            }
        }

        {
            std::ifstream f( itemMedicinePath );
            std::string   line;
            while( std::getline( f, line ) ) {
                auto cols = splitLine( normalizeLine( line ), ',' );
                if( cols.empty( ) ) { continue; }
                auto id   = parseId( safeGet( cols, 0 ) );
                auto& rec = records[ id ];
                rec.m_id  = id;
                rec.m_medicineEffect = safeGet( cols, 1 );
                rec.m_medicineParam1 = safeGet( cols, 2 );
                rec.m_medicineParam2 = safeGet( cols, 3 );
                rec.m_medicineParam3 = safeGet( cols, 4 );
            }
        }

        {
            std::ifstream f( itemFormePath );
            std::string   line;
            while( std::getline( f, line ) ) {
                auto cols = splitLine( normalizeLine( line ), ',' );
                if( cols.empty( ) ) { continue; }
                auto id   = parseId( safeGet( cols, 0 ) );
                auto& rec = records[ id ];
                rec.m_id  = id;
                rec.m_formeTargetSpecies = safeGet( cols, 2 );
                rec.m_formeTargetForme   = safeGet( cols, 3 );
            }
        }

        std::vector<ItemRecord> out;
        out.reserve( records.size( ) );
        for( auto& [ _, rec ] : records ) { out.push_back( std::move( rec ) ); }
        std::sort( out.begin( ), out.end( ),
                   []( const ItemRecord& p_lhs, const ItemRecord& p_rhs ) {
                       return p_lhs.m_id < p_rhs.m_id;
                   } );
        return out;
    }

    std::string itemDataEditor::toLower( const std::string& p_text ) {
        std::string out = p_text;
        std::transform( out.begin( ), out.end( ), out.begin( ),
                        []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
        return out;
    }

    std::vector<std::vector<std::string>>
    itemDataEditor::readDelimitedCsv( const std::string& p_path, char p_delim ) {
        std::vector<std::vector<std::string>> rows;
        std::ifstream                         f( p_path );
        std::string                           line;
        while( std::getline( f, line ) ) { rows.push_back( splitLine( normalizeLine( line ), p_delim ) ); }
        return rows;
    }

    bool itemDataEditor::writeDelimitedCsv( const std::string& p_path,
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

    void itemDataEditor::setupGrid( Gtk::Grid& p_grid ) {
        p_grid.set_row_spacing( MARGIN );
        p_grid.set_column_spacing( MARGIN * 2 );
        p_grid.set_margin( MARGIN );
    }

    void itemDataEditor::attachRow( Gtk::Grid& p_grid, int p_row, const std::string& p_label,
                                    Gtk::Entry& p_value ) {
        auto* key = Gtk::make_managed<Gtk::Label>( p_label );
        key->set_halign( Gtk::Align::START );
        key->set_xalign( 0.0f );
        p_grid.attach( *key, 0, p_row, 1, 1 );

        p_value.set_hexpand( true );
        p_grid.attach( p_value, 1, p_row, 1, 1 );
    }

    itemDataEditor::itemDataEditor( model& p_model, root& p_root )
        : _model( p_model ), _rootWindow( p_root ) {
        _mainBox.set_margin( MARGIN );
        _mainBox.set_spacing( MARGIN );
        _mainBox.set_expand( );

        _title.set_markup( "<span size=\"x-large\">Item Editor</span>" );
        _title.set_halign( Gtk::Align::START );
        _mainBox.append( _title );

        _splitPane.set_wide_handle( true );
        _splitPane.set_position( 300 );
        _splitPane.set_expand( true );
        _mainBox.append( _splitPane );

        _browserBox.set_spacing( MARGIN );
        _browserBox.set_margin( MARGIN );
        _search.set_placeholder_text( "Search items..." );
        _browserBox.append( _search );

        _listScroll.set_policy( Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC );
        _listScroll.set_expand( true );
        _listScroll.set_child( _recordList );
        _browserBox.append( _listScroll );
        _splitPane.set_start_child( _browserBox );

        _tabs.set_expand( true );
        setupGrid( _baseGrid );
        setupGrid( _behaviorGrid );
        setupGrid( _textGrid );

        attachRow( _baseGrid, 0, "Name", _baseNameValue );
        attachRow( _baseGrid, 1, "Item Type", _baseTypeValue );
        attachRow( _baseGrid, 2, "Sell Price", _baseSellPriceValue );
        attachRow( _baseGrid, 3, "Effect", _baseEffectValue );
        attachRow( _baseGrid, 4, "Param 1", _baseParam1Value );
        attachRow( _baseGrid, 5, "Param 2", _baseParam2Value );
        attachRow( _baseGrid, 6, "Param 3", _baseParam3Value );

        attachRow( _behaviorGrid, 0, "TM/HM Type", _behaviorTmTypeValue );
        attachRow( _behaviorGrid, 1, "TM/HM Number", _behaviorTmNumberValue );
        attachRow( _behaviorGrid, 2, "TM/HM Move", _behaviorTmMoveValue );
        attachRow( _behaviorGrid, 3, "Medicine Effect", _behaviorMedicineEffectValue );
        attachRow( _behaviorGrid, 4, "Medicine P1", _behaviorMedicineP1Value );
        attachRow( _behaviorGrid, 5, "Medicine P2", _behaviorMedicineP2Value );
        attachRow( _behaviorGrid, 6, "Medicine P3", _behaviorMedicineP3Value );
        attachRow( _behaviorGrid, 7, "Forme Species", _behaviorFormeSpeciesValue );
        attachRow( _behaviorGrid, 8, "Forme Forme", _behaviorFormeFormeValue );

        attachRow( _textGrid, 0, "Display Name", _textNameValue );
        attachRow( _textGrid, 1, "Flavor", _textFlavorValue );

        _tabs.append_page( _baseGrid, "Base" );
        _tabs.append_page( _behaviorGrid, "Behavior" );
        _tabs.append_page( _textGrid, "Text" );
        _splitPane.set_end_child( _tabs );

        _search.signal_changed( ).connect( [ this ]( ) { rebuildList( ); } );
        _recordList.signal_row_selected( ).connect( [ this ]( Gtk::ListBoxRow* p_row ) {
            if( p_row == nullptr ) { return; }
            const auto idx = static_cast<size_t>( p_row->get_index( ) );
            selectByFilteredPosition( idx );
        } );

        auto connectField = [ this ]( Gtk::Entry& p_entry ) {
            p_entry.signal_changed( ).connect( [ this ]( ) { handleFieldEdited( ); } );
        };
        connectField( _baseNameValue );
        connectField( _baseTypeValue );
        connectField( _baseSellPriceValue );
        connectField( _baseEffectValue );
        connectField( _baseParam1Value );
        connectField( _baseParam2Value );
        connectField( _baseParam3Value );
        connectField( _behaviorTmTypeValue );
        connectField( _behaviorTmNumberValue );
        connectField( _behaviorTmMoveValue );
        connectField( _behaviorMedicineEffectValue );
        connectField( _behaviorMedicineP1Value );
        connectField( _behaviorMedicineP2Value );
        connectField( _behaviorMedicineP3Value );
        connectField( _behaviorFormeSpeciesValue );
        connectField( _behaviorFormeFormeValue );
        connectField( _textNameValue );
        connectField( _textFlavorValue );
    }

    void itemDataEditor::loadRowsFromDisk( const std::string& p_dataPath ) {
        _itemDataRows     = readDelimitedCsv( p_dataPath + "/itemdata.csv", ',' );
        _itemNameRows     = readDelimitedCsv( p_dataPath + "/itemnames.csv", ',' );
        _itemFlavorRows   = readDelimitedCsv( p_dataPath + "/itemflavor.csv", ';' );
        _itemTmhmRows     = readDelimitedCsv( p_dataPath + "/itemdata_tmhm.csv", ',' );
        _itemMedicineRows = readDelimitedCsv( p_dataPath + "/itemdata_medicine.csv", ',' );
        _itemFormeRows    = readDelimitedCsv( p_dataPath + "/itemdata_formechange.csv", ',' );

        _itemDataRowById.clear( );
        _itemNameRowById.clear( );
        _itemFlavorRowById.clear( );
        _itemTmhmRowById.clear( );
        _itemMedicineRowById.clear( );
        _itemFormeRowById.clear( );
        for( size_t i = 0; i < _itemDataRows.size( ); ++i ) {
            _itemDataRowById[ parseId( safeGet( _itemDataRows[ i ], 0 ) ) ] = i;
        }
        for( size_t i = 0; i < _itemNameRows.size( ); ++i ) {
            _itemNameRowById[ parseId( safeGet( _itemNameRows[ i ], 0 ) ) ] = i;
        }
        for( size_t i = 0; i < _itemFlavorRows.size( ); ++i ) {
            _itemFlavorRowById[ parseId( safeGet( _itemFlavorRows[ i ], 0 ) ) ] = i;
        }
        for( size_t i = 0; i < _itemTmhmRows.size( ); ++i ) {
            _itemTmhmRowById[ parseId( safeGet( _itemTmhmRows[ i ], 0 ) ) ] = i;
        }
        for( size_t i = 0; i < _itemMedicineRows.size( ); ++i ) {
            _itemMedicineRowById[ parseId( safeGet( _itemMedicineRows[ i ], 0 ) ) ] = i;
        }
        for( size_t i = 0; i < _itemFormeRows.size( ); ++i ) {
            _itemFormeRowById[ parseId( safeGet( _itemFormeRows[ i ], 0 ) ) ] = i;
        }
    }

    void itemDataEditor::reloadIfNeeded( ) {
        if( _model.m_fsdata.m_fsrootPath.empty( ) ) { return; }

        auto fsrootPath = fs::path( _model.m_fsdata.m_fsrootPath );
        auto pneoRoot
            = fsrootPath.filename( ) == "FSROOT" ? fsrootPath.parent_path( ) : fsrootPath;
        auto dataPath = ( pneoRoot / "tools" / "fsdata" / "data" ).string( );
        if( dataPath == _lastDataPath && !_allRecords.empty( ) ) { return; }

        _allRecords = itemCsvAdapter::load( _model.m_fsdata.m_fsrootPath );
        loadRowsFromDisk( dataPath );
        _lastDataPath = dataPath;
        _dirty        = false;
    }

    void itemDataEditor::rebuildList( ) {
        u16 selectedId = 0;
        if( _selectedRecordIndex < _allRecords.size( ) ) {
            selectedId = _allRecords[ _selectedRecordIndex ].m_id;
        }

        while( auto* row = _recordList.get_row_at_index( 0 ) ) { _recordList.remove( *row ); }

        _filteredIndices.clear( );
        auto   query = toLower( _search.get_text( ) );
        size_t selectedPos = size_t( -1 );
        for( size_t i = 0; i < _allRecords.size( ); ++i ) {
            const auto& rec = _allRecords[ i ];
            auto        labelText = std::to_string( rec.m_id ) + " - " + rec.m_name;
            auto        haystack  = toLower( labelText + " " + rec.m_flavor );
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
            if( rec.m_id == selectedId ) { selectedPos = _filteredIndices.size( ) - 1; }
        }

        if( !_filteredIndices.empty( ) ) {
            size_t pos = selectedPos == size_t( -1 ) ? 0 : selectedPos;
            if( auto* row = _recordList.get_row_at_index( static_cast<int>( pos ) ) ) {
                _recordList.select_row( *row );
            }
        } else {
            _updatingFields = true;
            _baseNameValue.set_text( "" );
            _baseTypeValue.set_text( "" );
            _baseSellPriceValue.set_text( "" );
            _baseEffectValue.set_text( "" );
            _baseParam1Value.set_text( "" );
            _baseParam2Value.set_text( "" );
            _baseParam3Value.set_text( "" );
            _behaviorTmTypeValue.set_text( "" );
            _behaviorTmNumberValue.set_text( "" );
            _behaviorTmMoveValue.set_text( "" );
            _behaviorMedicineEffectValue.set_text( "" );
            _behaviorMedicineP1Value.set_text( "" );
            _behaviorMedicineP2Value.set_text( "" );
            _behaviorMedicineP3Value.set_text( "" );
            _behaviorFormeSpeciesValue.set_text( "" );
            _behaviorFormeFormeValue.set_text( "" );
            _textNameValue.set_text( "" );
            _textFlavorValue.set_text( "" );
            _updatingFields = false;
            _selectedRecordIndex = size_t( -1 );
        }
    }

    void itemDataEditor::selectByFilteredPosition( size_t p_pos ) {
        if( p_pos >= _filteredIndices.size( ) ) { return; }
        auto idx = _filteredIndices[ p_pos ];
        if( idx >= _allRecords.size( ) ) { return; }
        _selectedRecordIndex = idx;
        showRecord( _allRecords[ idx ] );
    }

    void itemDataEditor::showRecord( const ItemRecord& p_record ) {
        _updatingFields = true;
        _baseNameValue.set_text( p_record.m_name );
        _baseTypeValue.set_text( p_record.m_itemType );
        _baseSellPriceValue.set_text( p_record.m_sellPrice );
        _baseEffectValue.set_text( p_record.m_effect );
        _baseParam1Value.set_text( p_record.m_param1 );
        _baseParam2Value.set_text( p_record.m_param2 );
        _baseParam3Value.set_text( p_record.m_param3 );

        _behaviorTmTypeValue.set_text( p_record.m_tmhmType );
        _behaviorTmNumberValue.set_text( p_record.m_tmhmNumber );
        _behaviorTmMoveValue.set_text( p_record.m_tmhmMove );
        _behaviorMedicineEffectValue.set_text( p_record.m_medicineEffect );
        _behaviorMedicineP1Value.set_text( p_record.m_medicineParam1 );
        _behaviorMedicineP2Value.set_text( p_record.m_medicineParam2 );
        _behaviorMedicineP3Value.set_text( p_record.m_medicineParam3 );
        _behaviorFormeSpeciesValue.set_text( p_record.m_formeTargetSpecies );
        _behaviorFormeFormeValue.set_text( p_record.m_formeTargetForme );

        _textNameValue.set_text( p_record.m_name );
        _textFlavorValue.set_text( p_record.m_flavor );
        _updatingFields = false;
    }

    void itemDataEditor::handleFieldEdited( ) {
        if( _updatingFields || _selectedRecordIndex >= _allRecords.size( ) ) { return; }

        auto& rec        = _allRecords[ _selectedRecordIndex ];
        rec.m_name       = _textNameValue.get_text( );
        rec.m_flavor     = _textFlavorValue.get_text( );
        rec.m_itemType   = _baseTypeValue.get_text( );
        rec.m_sellPrice  = _baseSellPriceValue.get_text( );
        rec.m_effect     = _baseEffectValue.get_text( );
        rec.m_param1     = _baseParam1Value.get_text( );
        rec.m_param2     = _baseParam2Value.get_text( );
        rec.m_param3     = _baseParam3Value.get_text( );
        rec.m_tmhmType   = _behaviorTmTypeValue.get_text( );
        rec.m_tmhmNumber = _behaviorTmNumberValue.get_text( );
        rec.m_tmhmMove   = _behaviorTmMoveValue.get_text( );
        rec.m_medicineEffect = _behaviorMedicineEffectValue.get_text( );
        rec.m_medicineParam1 = _behaviorMedicineP1Value.get_text( );
        rec.m_medicineParam2 = _behaviorMedicineP2Value.get_text( );
        rec.m_medicineParam3 = _behaviorMedicineP3Value.get_text( );
        rec.m_formeTargetSpecies = _behaviorFormeSpeciesValue.get_text( );
        rec.m_formeTargetForme   = _behaviorFormeFormeValue.get_text( );

        _dirty = true;
    }

    void itemDataEditor::applyFieldEditsToSelectedRecord( ) {
        if( _selectedRecordIndex >= _allRecords.size( ) ) { return; }
        auto& rec = _allRecords[ _selectedRecordIndex ];

        auto writeAt = []( std::vector<std::string>& p_row, size_t p_idx, const std::string& p_val ) {
            if( p_row.size( ) <= p_idx ) { p_row.resize( p_idx + 1 ); }
            p_row[ p_idx ] = p_val;
        };

        if( _itemNameRowById.count( rec.m_id ) ) {
            auto& row = _itemNameRows[ _itemNameRowById[ rec.m_id ] ];
            writeAt( row, 1, rec.m_itemType );
            writeAt( row, 2, rec.m_name );
        }

        if( _itemFlavorRowById.count( rec.m_id ) ) {
            auto& row = _itemFlavorRows[ _itemFlavorRowById[ rec.m_id ] ];
            writeAt( row, 1, rec.m_flavor );
        }

        if( _itemDataRowById.count( rec.m_id ) ) {
            auto& row = _itemDataRows[ _itemDataRowById[ rec.m_id ] ];
            writeAt( row, 2, rec.m_sellPrice );
            writeAt( row, 3, rec.m_effect );
            writeAt( row, 4, rec.m_param1 );
            writeAt( row, 5, rec.m_param2 );
            writeAt( row, 6, rec.m_param3 );
        }

        if( _itemTmhmRowById.count( rec.m_id ) ) {
            auto& row = _itemTmhmRows[ _itemTmhmRowById[ rec.m_id ] ];
            writeAt( row, 1, rec.m_tmhmType );
            writeAt( row, 2, rec.m_tmhmNumber );
            writeAt( row, 3, rec.m_tmhmMove );
        }

        if( _itemMedicineRowById.count( rec.m_id ) ) {
            auto& row = _itemMedicineRows[ _itemMedicineRowById[ rec.m_id ] ];
            writeAt( row, 1, rec.m_medicineEffect );
            writeAt( row, 2, rec.m_medicineParam1 );
            writeAt( row, 3, rec.m_medicineParam2 );
            writeAt( row, 4, rec.m_medicineParam3 );
        }

        if( _itemFormeRowById.count( rec.m_id ) ) {
            auto& row = _itemFormeRows[ _itemFormeRowById[ rec.m_id ] ];
            writeAt( row, 2, rec.m_formeTargetSpecies );
            writeAt( row, 3, rec.m_formeTargetForme );
        }
    }

    bool itemDataEditor::writeCurrentRowsToDisk( const std::string& p_dataPath ) {
        auto ok1 = writeDelimitedCsv( p_dataPath + "/itemdata.csv", _itemDataRows, ',' );
        auto ok2 = writeDelimitedCsv( p_dataPath + "/itemnames.csv", _itemNameRows, ',' );
        auto ok3 = writeDelimitedCsv( p_dataPath + "/itemflavor.csv", _itemFlavorRows, ';' );
        auto ok4 = writeDelimitedCsv( p_dataPath + "/itemdata_tmhm.csv", _itemTmhmRows, ',' );
        auto ok5 = writeDelimitedCsv( p_dataPath + "/itemdata_medicine.csv", _itemMedicineRows, ',' );
        auto ok6 = writeDelimitedCsv( p_dataPath + "/itemdata_formechange.csv", _itemFormeRows, ',' );
        return ok1 && ok2 && ok3 && ok4 && ok5 && ok6;
    }

    bool itemDataEditor::saveToCsv( ) {
        reloadIfNeeded( );
        if( _lastDataPath.empty( ) ) { return false; }
        if( !_dirty ) { return true; }

        applyFieldEditsToSelectedRecord( );
        if( !writeCurrentRowsToDisk( _lastDataPath ) ) { return false; }
        _dirty = false;
        return true;
    }

    bool itemDataEditor::isDirty( ) const {
        return _dirty;
    }

    void itemDataEditor::redraw( ) {
        reloadIfNeeded( );
        rebuildList( );
    }
} // namespace UI
