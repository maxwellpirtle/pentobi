import QtQuick 2.0

// Piece element (square) with pseudo-3D effect.
// Simulates lighting by using different images for the lighting at different
// rotations and interpolating between them with an opacity animation.
Item {
    id: root

    Repeater {
        // Image rotation
        model: [ 0, 90, 180, 270 ]

        Loader {
            property real _imageOpacity: {
                var angle = (((pieceAngle - modelData) % 360) + 360) % 360 // JS modulo bug
                return (angle >= 90 && angle <= 270 ? 0 : Math.cos(angle * Math.PI / 180))
            }

            on_ImageOpacityChanged:
                if (_imageOpacity > 0 && status == Loader.Null)
                    sourceComponent = component

            Component {
                id: component

                Image {
                    source: imageName
                    width: root.width
                    height: root.height
                    sourceSize {
                        width: imageSourceWidth
                        height: imageSourceHeight
                    }
                    asynchronous: true
                    opacity: _imageOpacity
                    rotation: -modelData
                }
            }
        }
    }
}
