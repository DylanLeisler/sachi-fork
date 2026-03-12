#pragma once
#include <map>
#include <memory>
#include <set>
#include <vector>

#include <gtkmm/box.h>
#include <gtkmm/dropdown.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/frame.h>

#include "../../../data/maprender.h"
#include "../../../model.h"
#include "../../pure/editableBlock.h"
#include "../../pure/mapSlice.h"

namespace UI {
    class root;
}

namespace UI::MED {
    /*
     * @brief: Widget to select a block from a blockset
     */
    class blockSelector {
        model& _model;
        root&  _rootWindow;

        bool _disableRedraw = false;

        Gtk::Frame       _blockSetFrame;
        Gtk::Box         _mapEditorBlockSetBox{ Gtk::Orientation::VERTICAL };
        Gtk::DropDown    _mapEditorBS1CB, _mapEditorBS2CB; // select BS1/BS2
        computedMapSlice _ts1widget, _ts2widget;
        bool             _hoverBS1 = false, _hoverBS2 = false;
        bool             _bs1UpHeld = false, _bs1DownHeld = false;
        bool             _bs2UpHeld = false, _bs2DownHeld = false;
        int              _bs1PendingSteps = 0, _bs2PendingSteps = 0;
        bool             _bs1Draining = false, _bs2Draining = false;

        std::shared_ptr<Gtk::StringList> _mapBankStrList; // block set names

      public:
        blockSelector( model& p_model, root& p_root );

        inline operator Gtk::Widget&( ) {
            return _blockSetFrame;
        }

        void redraw( );

        void updateSelection( );

        inline void show( ) {
            _blockSetFrame.show( );
        }

        inline void hide( ) {
            _blockSetFrame.hide( );
        }

        inline bool isVisible( ) {
            return _blockSetFrame.is_visible( );
        }

        /*
         * @brief: Looks up the computed image data of the block sets.
         */
        inline std::shared_ptr<Gdk::Pixbuf> blockSetLookup( u16 p_blockIdx ) {
            if( p_blockIdx >= DATA::MAX_BLOCKS_PER_TILE_SET ) {
                p_blockIdx -= DATA::MAX_BLOCKS_PER_TILE_SET;
                return _ts2widget.getImageData( p_blockIdx );
            } else {
                return _ts1widget.getImageData( p_blockIdx );
            }
        }

        void onTSClicked( mapSlice::clickType, u16 p_blockX, u16 p_blockY, u8 p_ts );
    };
} // namespace UI::MED
