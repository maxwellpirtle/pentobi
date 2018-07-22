//-----------------------------------------------------------------------------
/** @file pentobi/qml/themes/light/Theme.qml
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

import QtQuick 2.11

Item {
    property color colorBackground: "#f3f3f3"
    property color colorBlue: "#0077D2"
    property color colorButtonPressed: Qt.darker(colorBackground, 1.1)
    property color colorCommentBorder: "#b4b3b3"
    property color colorCommentFocus: colorText
    property color colorGreen: "#00C000"
    property color colorMessageText: "black"
    property color colorMessageBase: "#cac9c9"
    property color colorOrange: "#F09217"
    property color colorPurple: "#a12CCF"
    property color colorRed: "#E63E2C"
    property color colorSelectedText: colorBackground
    property color colorSelection: "#645f5c"
    property color colorStartingPoint: "#767074"
    property color colorText: "#282625"
    property color colorYellow: "#e8c616"

    property real lighterToPlay: 0.5
    property real opacityCoordinates: 0.5
    property real opacityScore: 0.7
    property real opacitySubduedText: 0.5
    property real opacityPieceList: 1

    function getImage(name) { return "themes/light/" + name + ".svg" }
}
