import QtQuick 2.2
import Sailfish.Share 1.0
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    // Allow going back to the start dialog if sharing never started (for
    // example when session creation failed), but not while sharing.
    backNavigation: !client.tracking

    property bool creating: client.status === "Creating session..."

    function goBack() {
        // Remove any pages pushed on top of the tracking page, such as the
        // adopt dialog. Use an immediate pop so the navigation below can
        // proceed without waiting for a transition.
        if (pageStack.currentPage !== page) {
            pageStack.pop(page, PageStackAction.Immediate)
        }

        if (pageStack.depth > 1) {
            pageStack.pop()
        } else {
            pageStack.replace(Qt.resolvedUrl("MainPage.qml"))
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        PullDownMenu {
            MenuItem {
                text: qsTr("Stop sharing")
                visible: client.tracking
                onClicked: {
                    Remorse.popupAction(page, qsTr("Stopping sharing..."),
                                        function () {
                                            client.stopTracking()
                                        })
                }
            }

            MenuItem {
                text: qsTr("Share group PIN")
                visible: client.groupPin != ""
                onClicked: sharePin.trigger()
            }

            MenuItem {
                text: qsTr("Adopt a share")
                visible: client.groupHost && client.groupPin != ""
                onClicked: pageStack.push(Qt.resolvedUrl(
                                              "AdoptShareDialog.qml"))
            }

            MenuItem {
                text: qsTr("Share link")
                visible: client.shareLink != ""
                onClicked: share.trigger()
            }
        }

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge
            anchors.left: parent.left
            anchors.right: parent.right

            PageHeader {
                title: "Hauk"
            }

            Label {
                id: statusLabel
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: client.status
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeLarge
                horizontalAlignment: Text.AlignHCenter
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: formatTime(client.remainingSeconds)
                color: Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeExtraLarge
                horizontalAlignment: Text.AlignHCenter
                visible: client.tracking
            }

            Column {
                width: parent.width
                visible: client.tracking
                spacing: Theme.paddingMedium

                DetailItem {
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    label: qsTr("Share type")
                    value: client.groupHost ? qsTr("Group share (host)") : client.groupMember ? qsTr("Group share (member)") : qsTr("Your location only")
                }

                DetailItem {
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    label: qsTr("Adopted into group")
                    value: client.adoptedGroupShareId
                    visible: client.adoptedGroupShareId.length > 0
                }

                DetailItem {
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    label: qsTr("Group PIN")
                    value: client.groupPin
                    visible: client.groupPin !== ""
                }

                DetailItem {
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    label: qsTr("Latitude")
                    value: client.latitude.toFixed(6)
                }

                DetailItem {
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    label: qsTr("Longitude")
                    value: client.longitude.toFixed(6)
                }

                DetailItem {
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    label: qsTr("Accuracy")
                    value: client.accuracy.toFixed(1) + " m"
                }

                DetailItem {
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    label: qsTr("Share ID")
                    value: client.shareId.length > 0 ? client.shareId : client.shareLink.substring(
                                                           client.shareLink.indexOf(
                                                               "?") + 1)
                }
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }

            Label {
                id: errorMsg
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                topPadding: Theme.paddingMedium
                wrapMode: Text.Wrap
                color: Theme.errorColor
                visible: text.length > 0
                font.pixelSize: Theme.fontSizeSmall
            }
        }

        VerticalScrollDecorator {}
    }

    ShareAction {
        id: share

        title: qsTr("Share link")
        mimeType: "text/x-url"
        resources: [{
                "type": "text/x-url",
                "linkTitle": "Hauk",
                "status": client.shareLink
            }]
    }

    ShareAction {
        id: sharePin

        title: qsTr("Share group PIN")
        mimeType: "text/plain"
        resources: [{
                "type": "text/plain",
                "status": qsTr("Join my Hauk group share with PIN %1").arg(
                              client.groupPin)
            }]
    }

    function formatTime(seconds) {
        if (seconds <= 0)
            return ""
        var hours = Math.floor(seconds / 3600)
        var mins = Math.floor((seconds % 3600) / 60)
        var secs = seconds % 60
        if (hours > 0) {
            return hours + ":" + (mins < 10 ? "0" : "") + mins + ":" + (secs < 10 ? "0" : "") + secs
        }
        return mins + ":" + (secs < 10 ? "0" : "") + secs
    }

    Connections {
        target: client
        onStatusChanged: {
            // Keep the primary status line in sync even if the binding is
            // interrupted by the page being created during a transition.
            statusLabel.text = client.status
            if (client.status === "Creating session...") {
                errorMsg.text = ""
            }
        }
        onError: errorMsg.text = message
        onTrackingChanged: {
            // Stopping from the menu, the cover, or session expiry returns to
            // the start dialog. Inactive means the tracking page is covered by
            // a dialog, for example the adopt dialog.
            if (!client.tracking && (page.status === PageStatus.Active
                                     || page.status === PageStatus.Inactive)) {
                page.goBack()
            }
        }
    }
}
