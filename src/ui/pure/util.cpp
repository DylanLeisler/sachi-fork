#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/eventcontrollerscroll.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>

#include "../../data/bitmap.h"
#include "util.h"

namespace UI {
    namespace {
        constexpr const char* SPIN_SCROLL_DISABLED_KEY = "sachi-spin-scroll-disabled";
    }

    void disableSpinButtonScroll( Gtk::SpinButton& p_spinButton ) {
        if( g_object_get_data( G_OBJECT( p_spinButton.gobj( ) ), SPIN_SCROLL_DISABLED_KEY ) ) {
            return;
        }

        auto scrollController = Gtk::EventControllerScroll::create( );
        scrollController->set_flags( Gtk::EventControllerScroll::Flags::VERTICAL
                                     | Gtk::EventControllerScroll::Flags::HORIZONTAL
                                     | Gtk::EventControllerScroll::Flags::DISCRETE );
        scrollController->signal_scroll( ).connect( []( double, double ) { return true; }, false );
        p_spinButton.add_controller( scrollController );

        g_object_set_data( G_OBJECT( p_spinButton.gobj( ) ), SPIN_SCROLL_DISABLED_KEY,
                           reinterpret_cast<void*>( 1 ) );
    }

    void disableSpinButtonScrollRecursive( Gtk::Widget& p_widget ) {
        if( auto spinButton = dynamic_cast<Gtk::SpinButton*>( &p_widget ) ) {
            disableSpinButtonScroll( *spinButton );
        }

        for( auto child = p_widget.get_first_child( ); child;
             child      = child->get_next_sibling( ) ) {
            disableSpinButtonScrollRecursive( *child );
        }
    }

    std::shared_ptr<Gtk::Button> createButton( const std::string&     p_iconName,
                                               const std::string&     p_labelText,
                                               std::function<void( )> p_callback ) {
        auto hbox = Gtk::Box( Gtk::Orientation::HORIZONTAL, 5 );
        if( p_iconName != "" ) {
            auto icon = Gtk::Image( );
            hbox.append( icon );
            icon.set_from_icon_name( p_iconName );
        }
        if( p_labelText != "" ) {
            auto label = Gtk::Label( p_labelText );
            label.set_expand( true );
            label.set_use_underline( );
            hbox.append( label );
        }

        auto resultButton = std::make_shared<Gtk::Button>( );
        resultButton->set_child( hbox );
        resultButton->signal_clicked( ).connect(
            [ = ]( ) {
                p_callback( );
                resultButton->grab_focus( );
            },
            false );
        return resultButton;
    }

    std::shared_ptr<Gdk::Pixbuf> block::createImage( const DATA::computedBlock& p_block,
                                                     const DATA::palette*       p_palette,
                                                     u8                         p_daytime ) {
        auto btm = new DATA::bitmap( DATA::BLOCK_SIZE, DATA::BLOCK_SIZE );
        DATA::renderBlock( &p_block, p_palette, btm, 0, 0, 1, p_daytime );
        auto pixbuf = btm->pixbuf( );
        delete btm;
        return pixbuf;
    }

    std::shared_ptr<Gdk::Pixbuf> tile::createImage( const DATA::tile&    p_tile,
                                                    const DATA::palette& p_palette, bool p_flipX,
                                                    bool p_flipY ) {
        auto btm = new DATA::bitmap( DATA::TILE_SIZE, DATA::TILE_SIZE );
        DATA::renderTile( &p_tile, &p_palette, p_flipX, p_flipY, btm, 0, 0, 1 );
        auto pixbuf = btm->pixbuf( );
        delete btm;
        return pixbuf;
    }

    std::shared_ptr<Gtk::Image> tile::createImage( const DATA::computedBlockAtom& p_tile,
                                                   const DATA::palette            p_pals[ 5 * 16 ],
                                                   u8                             p_daytime ) {
        auto btm = new DATA::bitmap( DATA::TILE_SIZE, DATA::TILE_SIZE );

        DATA::renderTile( &p_tile.m_tile, &p_pals[ 16 * p_daytime + p_tile.m_palno ],
                          p_tile.m_vflip, p_tile.m_hflip, btm );

        //    btm->writeToFile( ( "/tmp/" + std::to_string( cnt++ ) + ".png" ).c_str( ) );

        auto pixbuf = btm->pixbuf( );
        auto res    = std::make_shared<Gtk::Image>( );
        res->set( pixbuf );

        delete btm;
        return res;
    }

    std::shared_ptr<Gdk::Pixbuf> tile::createImage( u16 p_color ) {
        auto btm         = new DATA::bitmap( 1, 1 );
        ( *btm )( 0, 0 ) = DATA::pixel( red( p_color ), green( p_color ), blue( p_color ), 255 );
        //    btm->writeToFile( ( "/tmp/" + std::to_string( cnt++ ) + ".png" ).c_str( ) );

        auto pixbuf = btm->pixbuf( );
        delete btm;
        return pixbuf;
    }

} // namespace UI
