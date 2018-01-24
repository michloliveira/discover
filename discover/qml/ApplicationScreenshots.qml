/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls 2.1 as QQC2
import QtGraphicalEffects 1.0
import org.kde.discover 2.0
import org.kde.kirigami 2.0 as Kirigami

ListView {
    id: root
    property alias resource: screenshotsModel.application
    property QtObject page

    spacing: Kirigami.Units.largeSpacing
    focus: overlay.visible

    Keys.onLeftPressed:  if (leftAction.visible)  leftAction.trigger()
    Keys.onRightPressed: if (rightAction.visible) rightAction.trigger()
    orientation: ListView.Horizontal

    QQC2.Popup {
        id: overlay
        parent: applicationWindow().overlay
        modal: true

        x: (parent.width - width)/2
        y: (parent.height - height)/2
        readonly property real proportion: overlayImage.sourceSize.height/overlayImage.sourceSize.width
        height: Math.min(parent.height * 0.9, (parent.width * 0.9) * proportion, overlayImage.sourceSize.height)
        width: height/proportion

        Image {
            id: overlayImage
            anchors.fill: parent
            source: root.currentItem ? root.currentItem.imageSource : ""
            fillMode: Image.PreserveAspectFit
            smooth: true
        }

        Button {
            anchors {
                right: parent.left
                verticalCenter: parent.verticalCenter
            }
            visible: leftAction.visible
            iconName: leftAction.iconName
            onClicked: leftAction.triggered(null)
        }

        Button {
            anchors {
                left: parent.right
                verticalCenter: parent.verticalCenter
            }
            visible: rightAction.visible
            iconName: rightAction.iconName
            onClicked: rightAction.triggered(null)
        }

        Kirigami.Action {
            id: leftAction
            iconName: "arrow-left"
            enabled: overlay.visible && visible
            visible: root.currentIndex >= 1
            onTriggered: root.decrementCurrentIndex()
        }

        Kirigami.Action {
            id: rightAction
            iconName: "arrow-right"
            enabled: overlay.visible && visible
            visible: root.currentIndex < (root.count - 1)
            onTriggered: root.incrementCurrentIndex()
        }
    }

    model: ScreenshotsModel {
        id: screenshotsModel
    }

    delegate: Item {
        readonly property url imageSource: large_image_url
        readonly property real proportion: thumbnail.sourceSize.height/thumbnail.sourceSize.width
        y: parent.height * 0.05
        height: parent.height * 0.9
        width: Math.max(50, height/proportion)
        DropShadow {
            source: thumbnail
            anchors.fill: parent
            verticalOffset: 3
            horizontalOffset: 0
            radius: 12.0
            samples: 25
            color: Kirigami.Theme.disabledTextColor
            cached: true
        }

        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor

            onClicked: {
                root.currentIndex = index
                overlay.open()
            }
        }
        BusyIndicator {
            visible: running
            running: parent.status == Image.Loading
            anchors.centerIn: thumbnail
        }

        Image {
            id: thumbnail
            source: small_image_url
            anchors {
                top: parent.top
                bottom: parent.bottom
            }
            fillMode: Image.PreserveAspectFit
            smooth: true
        }

    }

    layer.enabled: true
    // This item should be used as the 'mask'
    layer.effect: ShaderEffect {
        property var colorSource: root;
        property bool atLeft: root.atXBeginning;
        property bool atRight: root.atXEnd;
        fragmentShader: "
            uniform lowp bool atLeft;
            uniform lowp bool atRight;
            uniform lowp sampler2D colorSource;
            uniform lowp float qt_Opacity;
            varying highp vec2 qt_TexCoord0;
            void main() {
                gl_FragColor =
                    texture2D(colorSource, qt_TexCoord0)
                    * (atRight ? 1. : min(1.0, qt_TexCoord0.x * -20.0 + 20.))
                    * (atLeft  ? 1. : min(1.0, qt_TexCoord0.x *  20.0))
                    * qt_Opacity;
            }
        "
    }
}
