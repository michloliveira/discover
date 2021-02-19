/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.1
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1
import "navigation.js" as Navigation
import org.kde.kirigami 2.6 as Kirigami

Kirigami.AbstractCard
{
    id: delegateArea
    property alias application: installButton.application
    showClickFeedback: true

    function trigger() {
        Navigation.openApplication(application)
    }
    highlighted: delegateRecycler && delegateRecycler.ListView.isCurrentItem
    Keys.onReturnPressed: trigger()
    onClicked: trigger()

    contentItem: Item {
        implicitHeight: Kirigami.Units.gridUnit * 2

        InstallApplicationButton {
            id: installButton
            visible: false
        }
        Kirigami.Icon {
            id: resourceIcon
            source: application.icon
            height: Kirigami.Units.gridUnit * 3
            width: height
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
            }
        }

        ColumnLayout {
            spacing: 0
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
                left: resourceIcon.right
                leftMargin: Kirigami.Units.largeSpacing
            }

            Kirigami.Heading {
                id: head
                level: delegateArea.compact ? 3 : 2
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: delegateArea.application.name
                maximumLineCount: 1
            }

            Kirigami.Heading {
                id: category
                level: 5
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: delegateArea.application.categoryDisplay
                maximumLineCount: 1
                opacity: 0.6
                visible: delegateArea.application.categoryDisplay && delegateArea.application.categoryDisplay !== page.title && !parent.bigTitle
            }

            RowLayout {
                visible: showRating
                spacing: Kirigami.Units.largeSpacing
                Layout.topMargin: Kirigami.Units.smallSpacing
                Layout.fillWidth: true
                Rating {
                    rating: delegateArea.application.rating ? delegateArea.application.rating.sortableRating : 0
                    starSize: delegateArea.compact ? summary.font.pointSize : head.font.pointSize
                }
                Label {
                    Layout.fillWidth: true
                    text: delegateArea.application.rating ? i18np("%1 rating", "%1 ratings", delegateArea.application.rating.ratingCount) : i18n("No ratings yet")
                    visible: delegateArea.application.rating || delegateArea.application.backend.reviewsBackend.isResourceSupported(delegateArea.application)
                    opacity: 0.5
                    elide: Text.ElideRight
                }
            }
        }
    }
}
