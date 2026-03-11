#include <algorithm>
#include <cstring>

#include <gdkmm/texture.h>
#include <gtkmm/picture.h>

#include "mapSlice.h"

namespace UI {
    std::string toHexString( u8 p_value ) {
        char buffer[ 10 ];
        snprintf( buffer, 5, "%hhX", p_value );
        return std::string( buffer );
    }

    mapSlice::~mapSlice( ) {
        for( auto& im : _images ) { im->unparent( ); }
        if( _selectionRegion.get_parent( ) == this ) { _selectionRegion.unparent( ); }
        if( _hoverRegion.get_parent( ) == this ) { _hoverRegion.unparent( ); }
    }

    void mapSlice::updateBlockMovement( u8 p_oldvalue, u8 p_movement, u16 p_x, u16 p_y ) {
        auto pos = p_x + p_y * getWidth( );
        _overlayMovement[ pos ]->get_style_context( )->remove_class(
            block::classForMovement( p_oldvalue ) );
        _overlayMovement[ pos ]->set_text( toHexString( p_movement ) );
        _overlayMovement[ pos ]->get_style_context( )->add_class(
            block::classForMovement( p_movement ) );
    }

    std::shared_ptr<Gtk::Label> mapSlice::computeMarkLabel( u16 p_pos ) {
        auto res = std::make_shared<Gtk::Label>( );
        computeMarkLabel( p_pos, res );
        return res;
    }

    void mapSlice::computeMarkLabel( u16 p_pos, std::shared_ptr<Gtk::Label> p_label ) {
        if( !p_label || p_pos >= _marks.size( ) ) { return; }

        std::sort( _marks[ p_pos ].begin( ), _marks[ p_pos ].end( ) );

        std::string text  = "";
        std::string style = "";

        auto styles = std::vector<std::string>{ "mark-warp",    "mark-flypos",   "mark-script",
                                                "mark-message", "mark-sight",    "mark-movement",
                                                "mark-berry",   "mark-sight-red" };
        auto names  = std::vector<std::string>{ "W", "F", "S", "T", "", "", "B", "" };

        for( auto m : _marks[ p_pos ] ) {
            if( style == "" ) { style = styles[ u8( m ) ]; }
            text += names[ u8( m ) ];
        }

        for( auto s : styles ) {
            if( p_label->get_style_context( )->has_class( s ) ) {
                p_label->get_style_context( )->remove_class( s );
            }
        }
        p_label->set_text( text );
        if( style != "" ) { p_label->get_style_context( )->add_class( style ); }
    }

    void mapSlice::selectBlock( s16 p_blockIdx ) {
        if( p_blockIdx >= 0 && p_blockIdx < (int) _images.size( ) ) {
            const u16 x0 = p_blockIdx % getWidth( );
            const u16 y0 = p_blockIdx / getWidth( );
            if( x0 < getWidth( ) && y0 < getHeight( ) ) {
                _selectionX = x0;
                _selectionY = y0;
                _selectionW = std::min<u16>( _selectionWidth, getWidth( ) - x0 );
                _selectionH = std::min<u16>( _selectionHeight, getHeight( ) - y0 );
                _selectionVisible = _selectionW && _selectionH;
            } else {
                _selectionVisible = false;
            }
            _selectionAnchorIndex = p_blockIdx;
        } else {
            _selectionAnchorIndex = -1;
            _selectionVisible     = false;
        }

        if( !_selectionVisible ) {
            _selectionRegion.hide( );
            if( _selectionRegion.get_parent( ) == this ) { _selectionRegion.unparent( ); }
            return;
        }

        _selectionRegion.get_style_context( )->add_class( "mapblock-selected" );
        if( _selectionRegion.get_parent( ) == this ) { _selectionRegion.unparent( ); }
        _selectionRegion.set_parent( *this );
        _selectionRegion.show( );
        queue_allocate( );
    }

    void mapSlice::setScale( u16 p_scale ) {
        if( p_scale ) {
            _currentScale = p_scale;
            if( _selectionAnchorIndex >= 0 ) { selectBlock( _selectionAnchorIndex ); }
        }
    }

    void mapSlice::setSpacing( u16 p_blockSpacing ) {
        _blockSpacing = p_blockSpacing;
        if( _selectionAnchorIndex >= 0 ) { selectBlock( _selectionAnchorIndex ); }
    }

    void mapSlice::setOverlayOpacity( double p_newValue ) {
        _overlayOpacity = p_newValue;
        for( auto mnt : _overlayMovement ) { mnt->set_opacity( _overlayOpacity ); }
    }

    void mapSlice::setMarksOpacity( double p_newValue ) {
        _marksOpacity = p_newValue;
        for( auto mnt : _overlayMarks ) { mnt->set_opacity( _marksOpacity ); }
    }

    void mapSlice::setOverlayHidden( bool p_hidden ) {
        _showOverlay = !p_hidden;
        for( auto i : _overlayMovement ) {
            if( _showOverlay ) {
                i->show( );
            } else {
                i->hide( );
            }
        }
    }

    void mapSlice::setMarksHidden( bool p_hidden ) {
        _showMarks = !p_hidden;
        for( auto i : _overlayMarks ) {
            if( _showMarks ) {
                i->show( );
            } else {
                i->hide( );
            }
        }
    }

    void mapSlice::addMark( s16 p_blockIdx, mark p_mark ) {
        _marks[ p_blockIdx ].push_back( p_mark );
        computeMarkLabel( p_blockIdx, _overlayMarks[ p_blockIdx ] );
    }

    void mapSlice::removeMark( s16 p_blockIdx, mark p_mark ) {
        for( auto it = _marks[ p_blockIdx ].begin( ); it != _marks[ p_blockIdx ].end( ); ++it ) {
            if( *it == p_mark ) {
                _marks[ p_blockIdx ].erase( it );
                break;
            }
        }
        computeMarkLabel( p_blockIdx, _overlayMarks[ p_blockIdx ] );
    }

    void mapSlice::clearMarks( ) {
        for( auto& m : _marks ) { m.clear( ); }
        for( auto l : _overlayMarks ) { computeMarkLabel( 0, l ); }
    }

    void mapSlice::redrawBlock( u16 p_blockIdx ) {
        _images[ p_blockIdx ]->unparent( );
        _images[ p_blockIdx ] = std::make_shared<Gtk::Overlay>( );

        _imageData[ p_blockIdx ] = computeImageData( p_blockIdx );
        auto im                  = Gtk::Picture( );
        im.set_content_fit( Gtk::ContentFit::FILL );
        im.set_can_shrink( true );
        if( _imageData[ p_blockIdx ] ) {
            auto tx = Gdk::Texture::create_for_pixbuf( _imageData[ p_blockIdx ] );
            im.set_paintable( tx );
        }
        _images[ p_blockIdx ]->set_child( im );
        _images[ p_blockIdx ]->set_parent( *this );

        auto movement                  = computeMovementData( p_blockIdx );
        _overlayMovement[ p_blockIdx ] = std::make_shared<Gtk::Label>( toHexString( movement ) );
        _overlayMarks[ p_blockIdx ]    = computeMarkLabel( p_blockIdx );
        if( colorMovement( ) ) {
            _overlayMovement[ p_blockIdx ]->get_style_context( )->add_class(
                block::classForMovement( movement ) );
        }
        _overlayMovement[ p_blockIdx ]->set_opacity( _overlayOpacity );
        _overlayMarks[ p_blockIdx ]->set_opacity( _marksOpacity );
        if( _showOverlay ) {
            _overlayMovement[ p_blockIdx ]->show( );
        } else {
            _overlayMovement[ p_blockIdx ]->hide( );
        }

        if( _showMarks ) {
            _overlayMarks[ p_blockIdx ]->show( );
        } else {
            _overlayMarks[ p_blockIdx ]->hide( );
        }

        _images[ p_blockIdx ]->add_overlay( *_overlayMovement[ p_blockIdx ] );
        _images[ p_blockIdx ]->add_overlay( *_overlayMarks[ p_blockIdx ] );

        // Keep selection/hover overlays on top after replacing one tile overlay.
        if( _selectionVisible && _selectionRegion.get_parent( ) == this ) {
            _selectionRegion.unparent( );
            _selectionRegion.set_parent( *this );
            _selectionRegion.show( );
        }
        if( _hoverEnabled && _hoverVisible && _hoverRegion.get_parent( ) == this ) {
            _hoverRegion.unparent( );
            _hoverRegion.set_parent( *this );
            _hoverRegion.show( );
        }
    }

    void mapSlice::draw( ) {
        auto oldsel = _selectionAnchorIndex;
        selectBlock( -1 );
        for( auto& im : _images ) { im->unparent( ); }
        _images.clear( );
        _imageData.clear( );
        _overlayMovement.clear( );
        _overlayMarks.clear( );

        auto numblocks = getWidth( ) * getHeight( );

        if( _marks.empty( ) ) { _marks.assign( numblocks, std::vector<mapSlice::mark>{ } ); }

        for( u16 pos{ 0 }; pos < numblocks; ++pos ) {
            auto pb = computeImageData( pos );
            _imageData.push_back( pb );
            auto im = Gtk::Picture( );
            im.set_content_fit( Gtk::ContentFit::FILL );
            im.set_can_shrink( true );
            if( pb ) {
                auto tx = Gdk::Texture::create_for_pixbuf( pb );
                im.set_paintable( tx );
            }
            auto overlay = std::make_shared<Gtk::Overlay>( );
            overlay->set_child( im );
            overlay->set_parent( *this );
            _images.push_back( overlay );

            {
                auto movement = computeMovementData( pos );
                auto mnt      = std::make_shared<Gtk::Label>( toHexString( movement ) );
                if( colorMovement( ) ) {
                    mnt->get_style_context( )->add_class( block::classForMovement( movement ) );
                }
                mnt->set_opacity( _overlayOpacity );
                overlay->add_overlay( *mnt );
                if( !_showOverlay ) {
                    mnt->hide( );
                } else {
                    mnt->show( );
                }

                _overlayMovement.push_back( mnt );
            }
            {
                auto mnt = computeMarkLabel( pos );
                mnt->set_opacity( _marksOpacity );
                overlay->add_overlay( *mnt );
                if( !_showMarks ) {
                    mnt->hide( );
                } else {
                    mnt->show( );
                }

                _overlayMarks.push_back( mnt );
            }
        }
        selectBlock( oldsel );
    }

    Gtk::SizeRequestMode mapSlice::get_request_mode_vfunc( ) const {
        return Gtk::SizeRequestMode::CONSTANT_SIZE;
    }

    void mapSlice::measure_vfunc( Gtk::Orientation p_orientation, int, int& p_minimum,
                                  int& p_natural, int& p_minimumBaseline,
                                  int& p_naturalBaseline ) const {
        p_minimumBaseline = -1;
        p_naturalBaseline = -1;

        if( _images.empty( ) ) {
            p_minimum = 0;
            p_natural = 0;
            return;
        }

        auto width  = getWidth( );
        auto height = getHeight( );
        if( !width || !height ) {
            p_minimum = 0;
            p_natural = 0;
            return;
        }

        // Keep sizing bounded even if corrupted state slips through.
        constexpr int MAX_EXTENT = 8192;

        if( p_orientation == Gtk::Orientation::HORIZONTAL ) {
            auto req = int( width ) * int( _currentScale ) * int( DATA::BLOCK_SIZE )
                       + ( int( width ) - 1 ) * int( _blockSpacing );
            req       = std::clamp( req, 0, MAX_EXTENT );
            p_minimum = req;
            p_natural = req;
        } else {
            auto req = int( height ) * int( _currentScale ) * int( DATA::BLOCK_SIZE )
                       + ( int( height ) - 1 ) * int( _blockSpacing );
            req       = std::clamp( req, 0, MAX_EXTENT );
            p_minimum = req;
            p_natural = req;
        }
    }

    void mapSlice::size_allocate_vfunc( int, int, int p_baseline ) {
        // make sure bordering blocks stay glued together, even if we get surplus space
        for( size_t i = 0; i < _images.size( ); ++i ) {
            auto& im = _images[ i ];

            // make dummy calls to measure to suppress warnings (yes we do know how big
            // every block should be.)
            int ignore;
            im->measure( Gtk::Orientation::HORIZONTAL, -1, ignore, ignore, ignore, ignore );

            Gtk::Allocation allo;

            u16 x = i % getWidth( ), y = i / getWidth( );
            u16 sx = x * _currentScale * DATA::BLOCK_SIZE;
            u16 sy = y * _currentScale * DATA::BLOCK_SIZE;
            sx += x * _blockSpacing;
            sy += y * _blockSpacing;

            allo.set_x( sx );
            allo.set_y( sy );
            auto width  = _currentScale * DATA::BLOCK_SIZE;
            auto height = _currentScale * DATA::BLOCK_SIZE;
            allo.set_width( width );
            allo.set_height( height );
            im->size_allocate( allo, p_baseline );
        }

        if( _selectionVisible && _selectionRegion.get_parent( ) == this ) {
            Gtk::Allocation selAllo;
            const auto sx = _selectionX * _currentScale * DATA::BLOCK_SIZE + _selectionX * _blockSpacing;
            const auto sy = _selectionY * _currentScale * DATA::BLOCK_SIZE + _selectionY * _blockSpacing;
            auto sw       = _selectionW * _currentScale * DATA::BLOCK_SIZE;
            auto sh       = _selectionH * _currentScale * DATA::BLOCK_SIZE;
            if( _selectionW > 0 ) { sw += ( _selectionW - 1 ) * _blockSpacing; }
            if( _selectionH > 0 ) { sh += ( _selectionH - 1 ) * _blockSpacing; }

            selAllo.set_x( sx );
            selAllo.set_y( sy );
            selAllo.set_width( sw );
            selAllo.set_height( sh );
            _selectionRegion.size_allocate( selAllo, p_baseline );
        }

        if( _hoverEnabled && _hoverVisible ) {
            _hoverRegion.get_style_context( )->add_class( "mapblock-selected" );
            if( _hoverRegion.get_parent( ) != this ) { _hoverRegion.set_parent( *this ); }
            _hoverRegion.show( );

            Gtk::Allocation hoverAllo;
            const auto hx = _hoverX * _currentScale * DATA::BLOCK_SIZE + _hoverX * _blockSpacing;
            const auto hy = _hoverY * _currentScale * DATA::BLOCK_SIZE + _hoverY * _blockSpacing;
            auto hw       = _hoverW * _currentScale * DATA::BLOCK_SIZE;
            auto hh       = _hoverH * _currentScale * DATA::BLOCK_SIZE;
            if( _hoverW > 0 ) { hw += ( _hoverW - 1 ) * _blockSpacing; }
            if( _hoverH > 0 ) { hh += ( _hoverH - 1 ) * _blockSpacing; }

            hoverAllo.set_x( hx );
            hoverAllo.set_y( hy );
            hoverAllo.set_width( hw );
            hoverAllo.set_height( hh );
            _hoverRegion.size_allocate( hoverAllo, p_baseline );
        } else {
            if( _hoverRegion.get_parent( ) == this ) { _hoverRegion.unparent( ); }
        }
    }

    void lookupMapSlice::updateBlock( const DATA::mapBlockAtom& p_block, u16 p_x, u16 p_y ) {
        auto pos{ p_x + p_y * _blocksPerRow };
        if( pos >= _blocks.size( ) ) { return; }
        _blocks[ pos ] = p_block;
        redrawBlock( pos );
    }

    void lookupMapSlice::updateBlockMovement( u8 p_movement, u16 p_x, u16 p_y ) {
        auto pos{ p_x + p_y * _blocksPerRow };
        if( pos >= _blocks.size( ) ) { return; }
        mapSlice::updateBlockMovement( _blocks[ pos ].m_movedata, p_movement, p_x, p_y );
        _blocks[ pos ].m_movedata = p_movement;
    }

    void lookupMapSlice::set(
        const std::vector<DATA::mapBlockAtom>&                                   p_blocks,
        const std::function<std::shared_ptr<Gdk::Pixbuf>( DATA::mapBlockAtom )>& p_lookupFunction,
        u16                                                                      p_blocksPerRow ) {
        _blocks         = p_blocks;
        _blocksPerRow   = p_blocksPerRow;
        _lookupFunction = p_lookupFunction;
        if( !_blocksPerRow ) { _blocksPerRow = DATA::SIZE; }
        _height = _blocks.size( ) / _blocksPerRow;
        if( _height * _blocksPerRow < _blocks.size( ) ) { ++_height; }
    }

    void computedMapSlice::updateBlock( const DATA::computedBlock& p_block, u16 p_x, u16 p_y ) {
        auto pos{ p_x + p_y * _blocksPerRow };
        if( pos >= _blocks.size( ) ) { return; }
        _blocks[ pos ] = { p_block, _blocks[ pos ].second };
        redrawBlock( pos );
    }

    void computedMapSlice::updateBlockMovement( u8 p_movement, u16 p_x, u16 p_y ) {
        auto pos{ p_x + p_y * _blocksPerRow };
        if( pos >= _blocks.size( ) ) { return; }
        mapSlice::updateBlockMovement( _blocks[ pos ].second, p_movement, p_x, p_y );
        _blocks[ pos ] = { _blocks[ pos ].first, p_movement };
    }

    void computedMapSlice::set( const std::vector<std::pair<DATA::computedBlock, u8>>& p_blocks,
                                DATA::palette p_pals[ 5 * 16 ], u16 p_blocksPerRow ) {
        _blocks       = p_blocks;
        _blocksPerRow = p_blocksPerRow;
        std::memcpy( _pals, p_pals, sizeof( _pals ) );
        if( !_blocksPerRow ) { _blocksPerRow = DATA::SIZE; }
        _height = _blocks.size( ) / _blocksPerRow;
        if( _height * _blocksPerRow < _blocks.size( ) ) { ++_height; }
    }

    void tileSetMapSlice::set( const DATA::tileSet<1>& p_tiles, DATA::palette p_pals[ 5 * 16 ],
                               u16 p_tilesPerRow ) {
        _tiles       = p_tiles;
        _tilesPerRow = p_tilesPerRow;
        std::memcpy( _pals, p_pals, sizeof( _pals ) );
        if( !_tilesPerRow ) { _tilesPerRow = DATA::SIZE; }
        _height = DATA::MAX_TILES_PER_TILE_SET / _tilesPerRow;
        if( _height * _tilesPerRow < DATA::MAX_TILES_PER_TILE_SET ) { ++_height; }
    }

    void tileSlice::set( const DATA::tile& p_tile, DATA::palette p_pals[ 5 * 16 ] ) {
        _tile = p_tile;
        std::memcpy( _pals, p_pals, sizeof( _pals ) );
    }

    void colorSlice::set( const std::vector<u16>& p_colors, u16 p_colorsPerRow ) {
        _data         = p_colors;
        _colorsPerRow = p_colorsPerRow;
        if( !_colorsPerRow ) { _colorsPerRow = _data.size( ); }
        _height = _data.size( ) / _colorsPerRow;
        if( _height * _colorsPerRow < _data.size( ) ) { ++_height; }
    }

} // namespace UI
