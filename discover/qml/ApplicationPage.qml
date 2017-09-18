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
import QtQuick.Window 2.1
import QtQuick.Layouts 1.1
import org.kde.kquickcontrolsaddons 2.0
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import org.kde.kirigami 2.0 as Kirigami
import "navigation.js" as Navigation

DiscoverPage {
    id: appInfo
    property QtObject application: null
    clip: true

    title: application.name

    background: Rectangle { color: Kirigami.Theme.viewBackgroundColor }

    ReviewsPage {
        id: reviewsSheet
        model: reviewsModel
    }

    pageOverlay: Item {
        Rectangle {
            color: Kirigami.Theme.viewBackgroundColor
            anchors.fill: layo
        }
        RowLayout {
            Binding {
                target: appInfo
                property: "bottomPadding"
                value: layo.height + Kirigami.Units.largeSpacing
            }
            id: layo
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            Kirigami.BasicListItem {
                implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
                separatorVisible: false
                Layout.fillWidth: false
                icon: "draw-arrow-forward"
                label: i18n("Close Description")
                enabled: appInfo.sClose.enabled
                onClicked: appInfo.sClose.activated()
            }

            Item {
                Layout.fillWidth: true
            }

            spacing: 0

            ToolButton {
                Layout.minimumWidth: Kirigami.Units.gridUnit * 10
                visible: application.isInstalled && application.canExecute
                text: i18n("Launch")
                onClicked: application.invokeApplication()
            }

            InstallApplicationButton {
                application: appInfo.application
                flat: true
                Layout.minimumWidth: Kirigami.Units.gridUnit * 10
            }
        }
        Kirigami.Separator {
            anchors {
                left: layo.left
                right: layo.right
                top: layo.top
            }
            z: 4000
        }
    }

    Kirigami.OverlaySheet {
        id: originsOverlay
        bottomPadding: Kirigami.Units.largeSpacing
        topPadding: Kirigami.Units.largeSpacing
        readonly property alias model: alternativeResourcesView.model
        function listBackends() {
            var first = true;
            var ret = "";
            var m = alternativeResourcesView.model;
            for(var i=0, count=m.rowCount(); i<count; ++i) {
                var res = m.resourceAt(i)
                if (res != appInfo.application) {
                    if (!first) {
                        ret += ", "
                    }
                    ret += "<a href='" + i + "'>" + res.displayOrigin + "</a>"
                    first = false
                }
            }
            return ret
        }
        readonly property string sentence: alternativeResourcesView.count <= 1 ? "" : i18n("\nAlso available in %1", listBackends())
        ListView {
            id: alternativeResourcesView
            model: ResourcesProxyModel {
                allBackends: true
                resourcesUrl: appInfo.application.url
            }
            delegate: Kirigami.BasicListItem {
                label: displayOrigin
                checked: appInfo.application == model.application
                onClicked: if(index>=0) {
                    var res = model.application
                    console.assert(res)
                    window.stack.pop()
                    Navigation.openApplication(res)
                }
            }
        }
    }

    ColumnLayout {
        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.gridUnit
            Kirigami.Icon {
                Layout.preferredHeight: 128
                Layout.preferredWidth: 128

                source: appInfo.application.icon
                Layout.alignment: Qt.AlignVCenter
            }
            ColumnLayout {
                id: conts

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.topMargin: Kirigami.Units.largeSpacing
                Layout.bottomMargin: Kirigami.Units.largeSpacing
                spacing: 0

                Heading {
                    id: title
                    text: appInfo.application.name
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                Rectangle {
                    color: Kirigami.Theme.linkColor
                    Layout.fillWidth: true
                    height: 1
                }
                Label {
                    Layout.fillWidth: true
                    text: appInfo.application.comment
                    wrapMode: Text.WordWrap
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: appInfo.application.categoryDisplay
                    color: Kirigami.Theme.linkColor
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
                Label {
                    readonly property string version: appInfo.application.isInstalled ? appInfo.application.installedVersion : appInfo.application.availableVersion
                    visible: version.length > 0
                    text: version ? i18n("Version: %1", version) : ""
                }
                Label {
                    text: i18n("Size: %1", appInfo.application.sizeDescription)
                }
                RowLayout {
                    Label {
                        text: i18n("Source:")
                    }
                    LinkButton {
                        enabled: alternativeResourcesView.count > 1
                        text: appInfo.application.displayOrigin
                        onClicked: originsOverlay.open()
                    }
                }
                RowLayout {
                    Label {
                        text: i18n("License:")
                    }
                    LinkButton {
                        text: appInfo.application.license
//                         tooltip: i18n("See full license terms")
                        onClicked: Qt.openUrlExternally("https://spdx.org/licenses/" + appInfo.application.license + ".html#licenseText")
                    }
                }
            }
        }

        ApplicationScreenshots {
            Layout.fillWidth: true
            resource: appInfo.application
            page: appInfo
        }

        Heading {
            text: i18n("Description")
            Layout.fillWidth: true
            visible: appInfo.application.longDescription.length > 0
        }
        Kirigami.Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignJustify
            wrapMode: Text.WordWrap
            text: appInfo.application.longDescription + originsOverlay.sentence
            onLinkActivated: {
                var idx = parseInt(link, 10)
                var res = originsOverlay.model.resourceAt(idx)
                window.stack.pop()
                Navigation.openApplication(res)
            }
        }

        RowLayout {
            visible: button.text.length > 0
            Label {
                text: i18n("Homepage: ")
            }
            LinkButton {
                id: button
                text: application.homepage
                onClicked: Qt.openUrlExternally(application.homepage)
            }
        }

        LinkButton {
            id: addonsButton
            text: i18n("Addons")
            visible: addonsView.containsAddons
            onClicked: addonsView.sheetOpen = true
        }

        LinkButton {
            text: i18n("Review")
            onClicked: reviewsSheet.openReviewDialog()
            visible: !commentsButton.visible && reviewsModel.backend.isResourceSupported(appInfo.application)
        }
        LinkButton {
            id: commentsButton
            readonly property QtObject rating: appInfo.application.rating
            visible: rating && rating.ratingCount>0 && reviewsModel.count
            text: i18n("Show reviews (%1)...", rating ? reviewsModel.count : 0)

            ReviewsModel {
                id: reviewsModel
                resource: appInfo.application
            }

            onClicked: {
                reviewsSheet.open()
            }
        }

        Item {
            height: addonsButton.height
            width: 5
        }
    }

    readonly property var addons: AddonsView {
        id: addonsView
        application: appInfo.application
        parent: overlay
    }
}
