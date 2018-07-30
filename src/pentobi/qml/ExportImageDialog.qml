//-----------------------------------------------------------------------------
/** @file pentobi/qml/ExportImageDialog.qml
    @author Markus Enzenberger
    @copyright GNU General Public License version 3 or later */
//-----------------------------------------------------------------------------

import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.2
import "." as Pentobi

Pentobi.Dialog {
    footer: DialogButtonBoxOkCancel { }
    onAccepted: {
        exportImageWidth = parseInt(textField.text)
        var dialog = imageSaveDialog.get()
        dialog.name = gameModel.suggestFileName(folder, "png")
        dialog.selectNameFilter(0)
        dialog.open()
    }

    Item {
        implicitWidth: {
            // Qt 5.11 doesn't correctly handle dialog sizing if dialog (incl.
            // frame) is wider than window and Default style never makes footer
            // wider than content (potentially eliding button texts).
            var w = rowLayout.implicitWidth
            w = Math.max(w, font.pixelSize * 18)
            w = Math.min(w, maxContentWidth)
            return w
        }
        implicitHeight: rowLayout.implicitHeight

        RowLayout {
            id: rowLayout

            anchors.fill: parent

            Label { text: qsTr("Image width:") }
            TextField {
                id: textField

                text: exportImageWidth
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator{ bottom: 0; top: 32767 }
                selectByMouse: true
                Layout.preferredWidth: font.pixelSize * 7
            }
            Item { Layout.fillWidth: true }
        }
    }
}
