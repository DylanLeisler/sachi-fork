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
#include "moveDataEditor.h"

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

    std::vector<MoveRecord> moveCsvAdapter::load( const std::string& p_fsrootPath ) {
        auto fsrootPath = fs::path( p_fsrootPath );
        auto pneoRoot
            = fsrootPath.filename( ) == "FSROOT" ? fsrootPath.parent_path( ) : fsrootPath;
        auto dataPath = pneoRoot / "tools" / "fsdata" / "data";

        auto moveNamesPath = dataPath / "movenames.csv";
        auto moveDataPath  = dataPath / "movedata.csv";
        auto moveDescrPath = dataPath / "movedescr.csv";

        std::map<u16, MoveRecord> records;

        {
            std::ifstream f( moveNamesPath );
            std::string   line;
            while( std::getline( f, line ) ) {
                auto cols = splitLine( normalizeLine( line ), ',' );
                if( cols.empty( ) ) { continue; }

                auto id           = parseId( safeGet( cols, 0 ) );
                auto& rec         = records[ id ];
                rec.m_id          = id;
                rec.m_displayName = safeGet( cols, 1 );
            }
        }

        {
            std::ifstream f( moveDescrPath );
            std::string   line;
            while( std::getline( f, line ) ) {
                auto cols = splitLine( normalizeLine( line ), ';' );
                if( cols.empty( ) ) { continue; }

                auto id           = parseId( safeGet( cols, 0 ) );
                auto& rec         = records[ id ];
                rec.m_id          = id;
                rec.m_description = safeGet( cols, 1 );
            }
        }

        {
            std::ifstream f( moveDataPath );
            std::string   line;
            while( std::getline( f, line ) ) {
                auto cols = splitLine( normalizeLine( line ), ',' );
                if( cols.empty( ) ) { continue; }

                auto id   = parseId( safeGet( cols, 0 ) );
                auto& rec = records[ id ];
                rec.m_id  = id;

                rec.m_accuracy   = safeGet( cols, 1 );
                rec.m_power      = safeGet( cols, 2 );
                rec.m_effectCode = safeGet( cols, 3 );
                rec.m_category   = safeGet( cols, 4 );
                rec.m_pp         = safeGet( cols, 7 );
                rec.m_priority   = safeGet( cols, 8 );
                rec.m_flags      = safeGet( cols, 10 );
                rec.m_target     = safeGet( cols, 30 );
                rec.m_type       = safeGet( cols, 31 );

                rec.m_effectChance    = safeGet( cols, 22 );
                rec.m_secondaryBoosts = safeGet( cols, 24 );
                rec.m_secondaryStatus = safeGet( cols, 26 );
            }
        }

        std::vector<MoveRecord> out;
        out.reserve( records.size( ) );
        for( auto& [ _, rec ] : records ) { out.push_back( std::move( rec ) ); }
        std::sort( out.begin( ), out.end( ),
                   []( const MoveRecord& p_lhs, const MoveRecord& p_rhs ) {
                       return p_lhs.m_id < p_rhs.m_id;
                   } );
        return out;
    }

    std::string moveDataEditor::toLower( const std::string& p_text ) {
        std::string out = p_text;
        std::transform( out.begin( ), out.end( ), out.begin( ),
                        []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
        return out;
    }

    std::vector<std::vector<std::string>>
    moveDataEditor::readDelimitedCsv( const std::string& p_path, char p_delim ) {
        std::vector<std::vector<std::string>> rows;
        std::ifstream                         f( p_path );
        std::string                           line;
        while( std::getline( f, line ) ) { rows.push_back( splitLine( normalizeLine( line ), p_delim ) ); }
        return rows;
    }

    bool moveDataEditor::writeDelimitedCsv( const std::string& p_path,
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

    void moveDataEditor::setupGrid( Gtk::Grid& p_grid ) {
        p_grid.set_row_spacing( MARGIN );
        p_grid.set_column_spacing( MARGIN * 2 );
        p_grid.set_margin( MARGIN );
    }

    void moveDataEditor::attachRow( Gtk::Grid& p_grid, int p_row, const std::string& p_label,
                                    Gtk::Entry& p_value ) {
        auto* key = Gtk::make_managed<Gtk::Label>( p_label );
        key->set_halign( Gtk::Align::START );
        key->set_xalign( 0.0f );
        p_grid.attach( *key, 0, p_row, 1, 1 );

        p_value.set_hexpand( true );
        p_grid.attach( p_value, 1, p_row, 1, 1 );
    }

    moveDataEditor::moveDataEditor( model& p_model, root& p_root )
        : _model( p_model ), _rootWindow( p_root ) {
        _mainBox.set_margin( MARGIN );
        _mainBox.set_spacing( MARGIN );
        _mainBox.set_expand( );

        _title.set_markup( "<span size=\"x-large\">Move Editor</span>" );
        _title.set_halign( Gtk::Align::START );
        _mainBox.append( _title );

        _splitPane.set_wide_handle( true );
        _splitPane.set_position( 300 );
        _splitPane.set_expand( true );
        _mainBox.append( _splitPane );

        _browserBox.set_spacing( MARGIN );
        _browserBox.set_margin( MARGIN );
        _search.set_placeholder_text( "Search moves..." );
        _browserBox.append( _search );

        _listScroll.set_policy( Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC );
        _listScroll.set_expand( true );
        _listScroll.set_child( _recordList );
        _browserBox.append( _listScroll );
        _splitPane.set_start_child( _browserBox );

        _tabs.set_expand( true );
        setupGrid( _mechanicsGrid );
        setupGrid( _effectsGrid );
        setupGrid( _textGrid );

        attachRow( _mechanicsGrid, 0, "Name", _mechanicsNameValue );
        attachRow( _mechanicsGrid, 1, "Type", _mechanicsTypeValue );
        attachRow( _mechanicsGrid, 2, "Category", _mechanicsCategoryValue );
        attachRow( _mechanicsGrid, 3, "Power", _mechanicsPowerValue );
        attachRow( _mechanicsGrid, 4, "Accuracy", _mechanicsAccuracyValue );
        attachRow( _mechanicsGrid, 5, "PP", _mechanicsPpValue );
        attachRow( _mechanicsGrid, 6, "Priority", _mechanicsPriorityValue );
        attachRow( _mechanicsGrid, 7, "Target", _mechanicsTargetValue );
        attachRow( _mechanicsGrid, 8, "Flags", _mechanicsFlagsValue );

        attachRow( _effectsGrid, 0, "Effect Code", _effectsCodeValue );
        attachRow( _effectsGrid, 1, "Effect Chance", _effectsChanceValue );
        attachRow( _effectsGrid, 2, "Secondary Status", _effectsStatusValue );
        attachRow( _effectsGrid, 3, "Secondary Boosts", _effectsBoostValue );

        attachRow( _textGrid, 0, "Display Name", _textNameValue );
        attachRow( _textGrid, 1, "Description", _textDescriptionValue );

        _tabs.append_page( _mechanicsGrid, "Mechanics" );
        _tabs.append_page( _effectsGrid, "Effects" );
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
        connectField( _mechanicsNameValue );
        connectField( _mechanicsTypeValue );
        connectField( _mechanicsCategoryValue );
        connectField( _mechanicsPowerValue );
        connectField( _mechanicsAccuracyValue );
        connectField( _mechanicsPpValue );
        connectField( _mechanicsPriorityValue );
        connectField( _mechanicsTargetValue );
        connectField( _mechanicsFlagsValue );
        connectField( _effectsCodeValue );
        connectField( _effectsChanceValue );
        connectField( _effectsStatusValue );
        connectField( _effectsBoostValue );
        connectField( _textNameValue );
        connectField( _textDescriptionValue );
    }

    void moveDataEditor::loadRowsFromDisk( const std::string& p_dataPath ) {
        _moveDataRows = readDelimitedCsv( p_dataPath + "/movedata.csv", ',' );
        _moveNameRows = readDelimitedCsv( p_dataPath + "/movenames.csv", ',' );
        _moveDescrRows = readDelimitedCsv( p_dataPath + "/movedescr.csv", ';' );

        _moveDataRowById.clear( );
        _moveNameRowById.clear( );
        _moveDescrRowById.clear( );
        for( size_t i = 0; i < _moveDataRows.size( ); ++i ) {
            _moveDataRowById[ parseId( safeGet( _moveDataRows[ i ], 0 ) ) ] = i;
        }
        for( size_t i = 0; i < _moveNameRows.size( ); ++i ) {
            _moveNameRowById[ parseId( safeGet( _moveNameRows[ i ], 0 ) ) ] = i;
        }
        for( size_t i = 0; i < _moveDescrRows.size( ); ++i ) {
            _moveDescrRowById[ parseId( safeGet( _moveDescrRows[ i ], 0 ) ) ] = i;
        }
    }

    void moveDataEditor::reloadIfNeeded( ) {
        if( _model.m_fsdata.m_fsrootPath.empty( ) ) { return; }

        auto fsrootPath = fs::path( _model.m_fsdata.m_fsrootPath );
        auto pneoRoot
            = fsrootPath.filename( ) == "FSROOT" ? fsrootPath.parent_path( ) : fsrootPath;
        auto dataPath = ( pneoRoot / "tools" / "fsdata" / "data" ).string( );
        if( dataPath == _lastDataPath && !_allRecords.empty( ) ) { return; }

        _allRecords = moveCsvAdapter::load( _model.m_fsdata.m_fsrootPath );
        loadRowsFromDisk( dataPath );
        _lastDataPath = dataPath;
        _dirty        = false;
    }

    void moveDataEditor::rebuildList( ) {
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
            auto labelText  = std::to_string( rec.m_id ) + " - " + rec.m_displayName;
            auto haystack   = toLower( labelText + " " + rec.m_description );
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
            _updatingFields = true;
            _mechanicsNameValue.set_text( "" );
            _mechanicsTypeValue.set_text( "" );
            _mechanicsCategoryValue.set_text( "" );
            _mechanicsPowerValue.set_text( "" );
            _mechanicsAccuracyValue.set_text( "" );
            _mechanicsPpValue.set_text( "" );
            _mechanicsPriorityValue.set_text( "" );
            _mechanicsTargetValue.set_text( "" );
            _mechanicsFlagsValue.set_text( "" );
            _effectsCodeValue.set_text( "" );
            _effectsChanceValue.set_text( "" );
            _effectsStatusValue.set_text( "" );
            _effectsBoostValue.set_text( "" );
            _textNameValue.set_text( "" );
            _textDescriptionValue.set_text( "" );
            _updatingFields = false;
            _selectedRecordIndex = size_t( -1 );
        }
    }

    void moveDataEditor::selectByFilteredPosition( size_t p_pos ) {
        if( p_pos >= _filteredIndices.size( ) ) { return; }
        auto idx = _filteredIndices[ p_pos ];
        if( idx >= _allRecords.size( ) ) { return; }
        _selectedRecordIndex = idx;
        showRecord( _allRecords[ idx ] );
    }

    void moveDataEditor::showRecord( const MoveRecord& p_record ) {
        _updatingFields = true;
        _mechanicsNameValue.set_text( p_record.m_displayName );
        _mechanicsTypeValue.set_text( p_record.m_type );
        _mechanicsCategoryValue.set_text( p_record.m_category );
        _mechanicsPowerValue.set_text( p_record.m_power );
        _mechanicsAccuracyValue.set_text( p_record.m_accuracy );
        _mechanicsPpValue.set_text( p_record.m_pp );
        _mechanicsPriorityValue.set_text( p_record.m_priority );
        _mechanicsTargetValue.set_text( p_record.m_target );
        _mechanicsFlagsValue.set_text( p_record.m_flags );

        _effectsCodeValue.set_text( p_record.m_effectCode );
        _effectsChanceValue.set_text( p_record.m_effectChance );
        _effectsStatusValue.set_text( p_record.m_secondaryStatus );
        _effectsBoostValue.set_text( p_record.m_secondaryBoosts );

        _textNameValue.set_text( p_record.m_displayName );
        _textDescriptionValue.set_text( p_record.m_description );
        _updatingFields = false;
    }

    void moveDataEditor::handleFieldEdited( ) {
        if( _updatingFields || _selectedRecordIndex >= _allRecords.size( ) ) { return; }

        auto& rec            = _allRecords[ _selectedRecordIndex ];
        rec.m_displayName    = _textNameValue.get_text( );
        rec.m_description    = _textDescriptionValue.get_text( );
        rec.m_type           = _mechanicsTypeValue.get_text( );
        rec.m_category       = _mechanicsCategoryValue.get_text( );
        rec.m_power          = _mechanicsPowerValue.get_text( );
        rec.m_accuracy       = _mechanicsAccuracyValue.get_text( );
        rec.m_pp             = _mechanicsPpValue.get_text( );
        rec.m_priority       = _mechanicsPriorityValue.get_text( );
        rec.m_target         = _mechanicsTargetValue.get_text( );
        rec.m_flags          = _mechanicsFlagsValue.get_text( );
        rec.m_effectCode     = _effectsCodeValue.get_text( );
        rec.m_effectChance   = _effectsChanceValue.get_text( );
        rec.m_secondaryStatus = _effectsStatusValue.get_text( );
        rec.m_secondaryBoosts = _effectsBoostValue.get_text( );

        _dirty = true;
    }

    void moveDataEditor::applyFieldEditsToSelectedRecord( ) {
        if( _selectedRecordIndex >= _allRecords.size( ) ) { return; }
        auto& rec = _allRecords[ _selectedRecordIndex ];

        auto writeAt = []( std::vector<std::string>& p_row, size_t p_idx, const std::string& p_val ) {
            if( p_row.size( ) <= p_idx ) { p_row.resize( p_idx + 1 ); }
            p_row[ p_idx ] = p_val;
        };

        if( _moveNameRowById.count( rec.m_id ) ) {
            auto& row = _moveNameRows[ _moveNameRowById[ rec.m_id ] ];
            writeAt( row, 1, rec.m_displayName );
        }

        if( _moveDescrRowById.count( rec.m_id ) ) {
            auto& row = _moveDescrRows[ _moveDescrRowById[ rec.m_id ] ];
            writeAt( row, 1, rec.m_description );
        }

        if( _moveDataRowById.count( rec.m_id ) ) {
            auto& row = _moveDataRows[ _moveDataRowById[ rec.m_id ] ];
            writeAt( row, 1, rec.m_accuracy );
            writeAt( row, 2, rec.m_power );
            writeAt( row, 3, rec.m_effectCode );
            writeAt( row, 4, rec.m_category );
            writeAt( row, 7, rec.m_pp );
            writeAt( row, 8, rec.m_priority );
            writeAt( row, 10, rec.m_flags );
            writeAt( row, 22, rec.m_effectChance );
            writeAt( row, 24, rec.m_secondaryBoosts );
            writeAt( row, 26, rec.m_secondaryStatus );
            writeAt( row, 30, rec.m_target );
            writeAt( row, 31, rec.m_type );
        }
    }

    bool moveDataEditor::writeCurrentRowsToDisk( const std::string& p_dataPath ) {
        auto ok1 = writeDelimitedCsv( p_dataPath + "/movedata.csv", _moveDataRows, ',' );
        auto ok2 = writeDelimitedCsv( p_dataPath + "/movenames.csv", _moveNameRows, ',' );
        auto ok3 = writeDelimitedCsv( p_dataPath + "/movedescr.csv", _moveDescrRows, ';' );
        return ok1 && ok2 && ok3;
    }

    bool moveDataEditor::saveToCsv( ) {
        reloadIfNeeded( );
        if( _lastDataPath.empty( ) ) { return false; }
        if( !_dirty ) { return true; }

        applyFieldEditsToSelectedRecord( );
        if( !writeCurrentRowsToDisk( _lastDataPath ) ) { return false; }

        _dirty = false;
        return true;
    }

    bool moveDataEditor::isDirty( ) const {
        return _dirty;
    }

    void moveDataEditor::redraw( ) {
        reloadIfNeeded( );
        rebuildList( );
    }
} // namespace UI
