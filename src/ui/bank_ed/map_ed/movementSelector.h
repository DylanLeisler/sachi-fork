#pragma once
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/frame.h>
#include <gtkmm/togglebutton.h>

#include "../../../model.h"
#include "../../pure/mapSlice.h"

namespace UI {
    class root;
}

namespace UI::MED {
    /*
     * @brief: Widget to select a movement
     */
    class movementSelector {
        model& _model;
        root&  _rootWindow;

        Gtk::Frame       _movementFrame;
        Gtk::Box         _movementBox{ Gtk::Orientation::VERTICAL };
        Gtk::Box         _bucketToggleRow{ Gtk::Orientation::HORIZONTAL };
        Gtk::ToggleButton _bucketToggle;
        computedMapSlice _movementWidget;

      public:
        movementSelector( model& p_model, root& p_root );

        inline operator Gtk::Widget&( ) {
            return _movementFrame;
        }

        void redraw( );

        inline void hide( ) {
            _movementFrame.hide( );
        }

        inline void show( ) {
            _movementFrame.show( );
        }

        void updateSelection( );

        inline bool isVisible( ) {
            return _movementFrame.is_visible( );
        }
    };
} // namespace UI::MED
