#pragma once
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/eventcontrollermotion.h>
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
        std::shared_ptr<Gtk::EventControllerMotion> _movementHoverMotion;
        computedMapSlice _movementWidget;

        static std::string movementTooltipText( u8 p_movement );

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
