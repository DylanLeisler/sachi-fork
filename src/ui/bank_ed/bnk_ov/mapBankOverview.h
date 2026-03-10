#pragma once
#include <functional>
#include <memory>

#include <gtkmm/box.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/label.h>
#include <gtkmm/overlay.h>
#include <gtkmm/picture.h>
#include <gtkmm/widget.h>

#include "../../../data/maprender.h"
#include "../../../defines.h"

namespace UI::BOV {
    class mapBankOverview : public Gtk::Widget {
      public:
        static constexpr u16 PREVIEW_DOWN_SCALE = 8;
        static constexpr u16 PREVIEW_RENDER_SIZE
            = DATA::BLOCK_SIZE * DATA::SIZE / PREVIEW_DOWN_SCALE;
        static constexpr u16 DISPLAY_SLICE_SIZE = DATA::BLOCK_SIZE;

        enum clickType {
            LEFT_DOUBLE   = GDK_BUTTON_PRIMARY,
            RIGHT_DOUBLE  = GDK_BUTTON_SECONDARY,
            MIDDLE_DOUBLE = GDK_BUTTON_MIDDLE,
        };

      private:
        u16 _mapScale   = 3;
        u16 _mapSpacing = 2;

        u8 _currentDaytime = 0;

        s16 _currentSelectionIndex = -1;

        std::shared_ptr<Gtk::GestureClick> _clickEvent;

        std::vector<std::vector<DATA::computedMapSlice>> _mapBank;
        std::vector<std::shared_ptr<Gtk::Overlay>>       _images;

        Gtk::Box _selectionBox;

      public:
        inline mapBankOverview( ) {
            _clickEvent = Gtk::GestureClick::create( );
            _clickEvent->set_propagation_phase( Gtk::PropagationPhase::CAPTURE );
            _clickEvent->set_button( 0 );
            add_controller( _clickEvent );

            _selectionBox = Gtk::Box( );
            _selectionBox.get_style_context( )->add_class( "mapblock-selected" );
            _selectionBox.set_size_request( _mapScale * DISPLAY_SLICE_SIZE - 4,
                                            _mapScale * DISPLAY_SLICE_SIZE - 4 );
            _currentSelectionIndex = -1;
        }
        virtual ~mapBankOverview( );

        virtual inline void
        connectClick( const std::function<void( clickType, u16, u16 )>& p_callback ) {
            _clickEvent->signal_pressed( ).connect(
                [ this, p_callback ]( int p_numPresses, double p_x, double p_y ) {
                    if( !_clickEvent->get_current_button( ) || p_numPresses <= 1 ) { return; }
                    auto blockwd = _mapScale * DATA::BLOCK_SIZE + _mapSpacing;
                    p_callback( (clickType) _clickEvent->get_current_button( ),
                                int( p_x / blockwd ), int( p_y / blockwd ) );
                } );
        }

        void setScale( u16 p_scale = 1 );
        void setSpacing( u16 p_blockSpacing = 0 );

        /*
         * @brief: Sets the map bank to display, doesn't draw anything.
         */
        void set( const std::vector<std::vector<DATA::computedMapSlice>>& p_bank );

        /*
         * @brief: Replaces and redraws the specified map slice.
         */
        void replaceMap( const DATA::computedMapSlice& p_map, u8 p_mapY, u8 p_mapX );

        /*
         * @brief: Redraws the map for the specified daytime.
         */
        void redraw( u8 p_daytime );

        virtual void        selectMap( s16 p_mapIdx );
        virtual inline void selectMap( u16 p_mapX, u16 p_mapY ) {
            if( _mapBank.empty( ) ) {
                selectMap( -1 );
                return;
            }

            selectMap( p_mapY * _mapBank[ 0 ].size( ) + p_mapX );
        }

        static std::shared_ptr<Gtk::Picture> createImage( const DATA::computedMapSlice& p_slice,
                                                          u8                            p_daytime,
                                                          u16 p_scale = 1 );

      protected:
        Gtk::SizeRequestMode get_request_mode_vfunc( ) const override;
        void measure_vfunc( Gtk::Orientation p_orientation, int, int& p_minimum, int& p_natural,
                            int& p_minimumBaseline, int& p_naturalBaseline ) const override;
        void size_allocate_vfunc( int p_width, int p_height, int p_baseline ) override;
    };
} // namespace UI::BOV
