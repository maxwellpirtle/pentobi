//-----------------------------------------------------------------------------
/** @file pentobi/qml/themes/light/Theme.qml
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

import QtQuick 2.11

/** Theme using light colors. */
Item {
    /** @name Colors for board and piece elements.
        Each color has several versions to paint raised or sunken borders. The
        first color is the base color, the second a darker version, the third
        a lighter version. The board has a second set of three colors for
        painting the center section in Callisto, the pieces have a fourth color
        for painting markup. */
    /// @{
    property var colorBlue: [ "#0073cf", "#004881", "#1499ff", "#ffffff" ]
    property var colorBoard: [ "#aea7ac", "#868084", "#c7bfc5",
                               "#918b8f", "#7c777b", "#a09a9f"]
    property var colorGreen: [ "#00c000", "#007800", "#00fa00", "#333333" ]
    property var colorOrange: [ "#f09217", "#9d5e0b", "#ffbb67", "#333333" ]
    property var colorPurple: [ "#a12ccf", "#6d2787", "#be70dc", "#ffffff" ]
    property var colorRed: [ "#e63e2c", "#90261b", "#ff655a", "#ffffff" ]
    property var colorYellow: [ "#f5c320", "#aa8516", "#ffdb58", "#333333" ]
    /// @}

    property color colorBackground: "#eff0f1"
    property color colorButtonPressed: Qt.darker(colorBackground, 1.1)
    property color colorButtonHovered: Qt.lighter(colorBackground, 3)
    property color colorButtonBorder: Qt.darker(colorBackground, 2)
    property color colorCommentBase: "#ffffff"
    property color colorCommentBorder: "#b4b3b3"
    property color colorCommentFocus: "#4799cc"
    property color colorCommentText: colorText
    property color colorMessageText: "black"
    property color colorMessageBase: "#cac9c9"
    property color colorSelectedText: colorBackground
    property color colorSelection: "#4799cc"
    property color colorStartingPoint: "#767074"
    property color colorBoardMarkup: "#111111"
    property color colorText: "#111111"

    function getImage(name) { return "themes/light/" + name + ".png" }
}
