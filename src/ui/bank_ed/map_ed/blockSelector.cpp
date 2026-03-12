#include "blockSelector.h"
#include <algorithm>
#include <gdk/gdkkeysyms.h>
#include <glibmm/main.h>
#include "../../root.h"

namespace UI::MED {
    blockSelector::blockSelector( model& p_model, root& p_root )
        : _model{ p_model }, _rootWindow{ p_root } {

        auto bsselbox = Gtk::Box( Gtk::Orientation::HORIZONTAL );
        bsselbox.set_margin( MARGIN );
        bsselbox.set_halign( Gtk::Align::CENTER );

        _blockSetFrame = Gtk::Frame( "Tile Sets" );
        _blockSetFrame.set_margin_start( MARGIN );
        _blockSetFrame.set_label_align( Gtk::Align::CENTER );
        _blockSetFrame.set_child( _mapEditorBlockSetBox );

        _mapBankStrList = Gtk::StringList::create( _model.m_fsdata.m_mapBankStrList );

        _mapEditorBS1CB.set_model( _mapBankStrList );
        _mapEditorBS1CB.property_selected_item( ).signal_changed( ).connect( [ &, this ]( ) {
            auto sc = _model.slice( );
            if( _disableRedraw || _mapEditorBS1CB.get_selected( ) == GTK_INVALID_LIST_POSITION
                || _model.m_fsdata.m_blockSets[ sc.m_data.m_tIdx1 ].m_stringListItem
                       == _mapEditorBS1CB.get_selected( ) ) {
                return;
            }
            try {
                u16 newTS1
                    = std::stoi( _mapBankStrList->get_string( _mapEditorBS1CB.get_selected( ) ) );
                _model.setTileSet( 0, newTS1 );
                _rootWindow.redraw( );
            } catch( ... ) { return; }
        } );
        _mapEditorBS2CB.set_model( _mapBankStrList );
        _mapEditorBS2CB.property_selected_item( ).signal_changed( ).connect( [ this ]( ) {
            auto sc = _model.slice( );
            if( _disableRedraw || _mapEditorBS2CB.get_selected( ) == GTK_INVALID_LIST_POSITION
                || _model.m_fsdata.m_blockSets[ sc.m_data.m_tIdx2 ].m_stringListItem
                       == _mapEditorBS2CB.get_selected( ) ) {
                return;
            }
            try {
                u16 newTS2
                    = std::stoi( _mapBankStrList->get_string( _mapEditorBS2CB.get_selected( ) ) );
                _model.setTileSet( 1, newTS2 );
                _rootWindow.redraw( );
            } catch( ... ) { return; }
        } );

        auto attachNudgeControls
            = [ this ]( Gtk::DropDown& p_dd, bool& p_hover, bool& p_upHeld, bool& p_downHeld,
                        int& p_pendingSteps, bool& p_draining ) {
            auto motion = Gtk::EventControllerMotion::create( );
            motion->signal_enter( ).connect( [ &p_hover ]( double, double ) { p_hover = true; } );
            motion->signal_leave( ).connect( [ &p_hover ]( ) { p_hover = false; } );
            p_dd.add_controller( motion );

            auto applyStep = [ this, &p_dd ]( int p_step ) {
                if( !_mapBankStrList || !_mapBankStrList->get_n_items( ) ) { return; }
                auto current = p_dd.get_selected( );
                if( current == GTK_INVALID_LIST_POSITION ) { current = 0; }
                auto maxIdx = int( _mapBankStrList->get_n_items( ) ) - 1;
                auto next   = std::clamp<int>( int( current ) + p_step, 0, maxIdx );
                if( next != int( current ) ) { p_dd.set_selected( next ); }
            };
            auto scheduleDrain = [ &p_pendingSteps, &p_draining, applyStep ]( ) {
                if( p_draining ) { return; }
                p_draining = true;
                Glib::signal_timeout( ).connect(
                    [ &p_pendingSteps, &p_draining, applyStep ]( ) -> bool {
                        if( p_pendingSteps == 0 ) {
                            p_draining = false;
                            return false;
                        }
                        if( p_pendingSteps > 0 ) {
                            --p_pendingSteps;
                            applyStep( 1 );
                        } else {
                            ++p_pendingSteps;
                            applyStep( -1 );
                        }
                        return true;
                    },
                    12 );
            };

            auto key = Gtk::EventControllerKey::create( );
            key->signal_key_pressed( ).connect(
                [ this, &p_dd, &p_hover, &p_upHeld,
                  &p_downHeld, &p_pendingSteps, scheduleDrain ]( guint p_keyval, guint,
                                                                  Gdk::ModifierType ) -> bool {
                    if( !p_dd.has_focus( ) && !p_hover ) { return false; }
                    if( !_mapBankStrList || !_mapBankStrList->get_n_items( ) ) { return false; }

                    int step = 0;
                    if( p_keyval == GDK_KEY_Down ) {
                        if( p_downHeld ) { return true; }
                        p_downHeld = true;
                        step = 1;
                    } else if( p_keyval == GDK_KEY_Up ) {
                        if( p_upHeld ) { return true; }
                        p_upHeld = true;
                        step = -1;
                    } else {
                        return false;
                    }

                    constexpr int MAX_PENDING_STEPS = 10;
                    p_pendingSteps = std::clamp( p_pendingSteps + step, -MAX_PENDING_STEPS,
                                                 MAX_PENDING_STEPS );
                    scheduleDrain( );
                    return true;
                },
                false );
            key->signal_key_released( ).connect(
                [ &p_upHeld, &p_downHeld ]( guint p_keyval, guint, Gdk::ModifierType ) {
                    if( p_keyval == GDK_KEY_Up ) { p_upHeld = false; }
                    if( p_keyval == GDK_KEY_Down ) { p_downHeld = false; }
                } );
            p_dd.add_controller( key );
        };
        attachNudgeControls( _mapEditorBS1CB, _hoverBS1, _bs1UpHeld, _bs1DownHeld,
                             _bs1PendingSteps, _bs1Draining );
        attachNudgeControls( _mapEditorBS2CB, _hoverBS2, _bs2UpHeld, _bs2DownHeld,
                             _bs2PendingSteps, _bs2Draining );

        bsselbox.append( _mapEditorBS1CB );
        bsselbox.append( _mapEditorBS2CB );
        bsselbox.get_style_context( )->add_class( "linked" );
        _mapEditorBlockSetBox.append( bsselbox );

        auto meScrolledWindow1 = Gtk::ScrolledWindow( );
        meScrolledWindow1.set_child( _ts1widget );
        meScrolledWindow1.set_margin( MARGIN );
        meScrolledWindow1.set_vexpand( );
        meScrolledWindow1.set_halign( Gtk::Align::CENTER );
        meScrolledWindow1.set_policy( Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC );
        _mapEditorBlockSetBox.append( meScrolledWindow1 );

        _ts1widget.connectClick(
            [ this ]( mapSlice::clickType p_button, u16 p_blockX, u16 p_blockY ) {
                onTSClicked( p_button, p_blockX, p_blockY, 0 );
            } );

        auto meScrolledWindow2 = Gtk::ScrolledWindow( );
        meScrolledWindow2.set_child( _ts2widget );
        meScrolledWindow2.set_margin( MARGIN );
        meScrolledWindow2.set_vexpand( );
        meScrolledWindow2.set_halign( Gtk::Align::CENTER );
        meScrolledWindow2.set_policy( Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC );
        _mapEditorBlockSetBox.append( meScrolledWindow2 );

        _ts2widget.connectClick(
            [ this ]( mapSlice::clickType p_button, u16 p_blockX, u16 p_blockY ) {
                onTSClicked( p_button, p_blockX, p_blockY, 1 );
            } );
    }

    void blockSelector::updateSelection( ) {
        if( _model.m_settings.m_currentlySelectedBlock.m_blockidx
            < DATA::MAX_BLOCKS_PER_TILE_SET ) {
            _ts1widget.selectBlock( _model.m_settings.m_currentlySelectedBlock.m_blockidx );
            _ts2widget.selectBlock( -1 );
        } else {
            _ts1widget.selectBlock( -1 );
            _ts2widget.selectBlock( _model.m_settings.m_currentlySelectedBlock.m_blockidx
                                    - DATA::MAX_BLOCKS_PER_TILE_SET );
        }
    }

    void blockSelector::redraw( ) {
        auto sc = _model.slice( );

        _disableRedraw = true;
        if( _mapBankStrList ) {
            _mapBankStrList->splice( 0, _mapBankStrList->get_n_items( ),
                                     _model.m_fsdata.m_mapBankStrList );
        }

        _mapEditorBS1CB.set_selected(
            _model.m_fsdata.m_blockSets[ sc.m_data.m_tIdx1 ].m_stringListItem );
        _mapEditorBS2CB.set_selected(
            _model.m_fsdata.m_blockSets[ sc.m_data.m_tIdx2 ].m_stringListItem );
        _disableRedraw = false;

        auto ts = DATA::tileSet<2>( );
        _model.buildTileSet( &ts );
        DATA::palette pals[ 16 * 5 ] = { 0 };
        _model.buildPalette( pals );

        _ts1widget.setScale( _model.m_settings.m_blockScale );
        _ts1widget.setSpacing( _model.m_settings.m_blockSpacing );
        if( _model.m_settings.m_tileBrushPatternMode ) {
            _ts1widget.setSelectionSize( _model.m_settings.m_tileBrushWidth,
                                         _model.m_settings.m_tileBrushHeight );
        } else {
            _ts1widget.setSelectionSize( 1, 1 );
        }
        _ts1widget.queue_resize( );
        _ts2widget.setScale( _model.m_settings.m_blockScale );
        _ts2widget.setSpacing( _model.m_settings.m_blockSpacing );
        if( _model.m_settings.m_tileBrushPatternMode ) {
            _ts2widget.setSelectionSize( _model.m_settings.m_tileBrushWidth,
                                         _model.m_settings.m_tileBrushHeight );
        } else {
            _ts2widget.setSelectionSize( 1, 1 );
        }
        _ts2widget.queue_resize( );

        _ts1widget.set( DATA::mapBlockAtom::computeBlockSet(
                            &_model.m_fsdata.m_blockSets[ sc.m_data.m_tIdx1 ].m_blockSet, &ts ),
                        pals, _model.m_settings.m_blockSetWidth );

        _ts2widget.set( DATA::mapBlockAtom::computeBlockSet(
                            &_model.m_fsdata.m_blockSets[ sc.m_data.m_tIdx2 ].m_blockSet, &ts ),
                        pals, _model.m_settings.m_blockSetWidth );

        _ts1widget.setDaytime( _model.m_settings.m_currentDayTime );
        _ts2widget.setDaytime( _model.m_settings.m_currentDayTime );

        _ts1widget.draw( );
        _ts2widget.draw( );
        updateSelection( );
    }

    void blockSelector::onTSClicked( mapSlice::clickType, u16 p_blockX, u16 p_blockY, u8 p_ts ) {
        u16 block = p_blockY * _model.m_settings.m_blockSetWidth + p_blockX;
        _model.updateSelectedBlock( { u16( block + p_ts * DATA::MAX_BLOCKS_PER_TILE_SET ), 0 } );
        if( _model.m_settings.m_currentlySelectedBlock.m_blockidx
            < DATA::MAX_BLOCKS_PER_TILE_SET ) {
            _ts1widget.selectBlock( _model.m_settings.m_currentlySelectedBlock.m_blockidx );
            _ts2widget.selectBlock( -1 );
        } else {
            _ts1widget.selectBlock( -1 );
            _ts2widget.selectBlock( _model.m_settings.m_currentlySelectedBlock.m_blockidx
                                    - DATA::MAX_BLOCKS_PER_TILE_SET );
        }
    }
} // namespace UI::MED
