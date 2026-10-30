import QtQuick 2.2
import Sailfish.Silica 1.0

Dialog {
    id: page
    allowedOrientations: Orientation.All

    property int shareMode: 0
    property bool joining: shareMode === 2
    property bool group: shareMode !== 0

    backNavigation: false
    // The dialog needs a pending destination, otherwise the page stack reports
    // that forward navigation is unavailable and disables the accept action.
    acceptDestination: trackingPage
    acceptDestinationAction: PageStackAction.Push

    canAccept: !group || (nicknameField.text.trim().length > 0
                          && (!joining || /^\d{6}$/.test(pinField.text)))

    Component {
        id: trackingPage

        TrackingPage {}
    }

    onOpened: {
        errorMsg.text = ""

        if (group) {
            nicknameField.text = client.nickname
            pinField.text = ""
            nicknameField.focus = true
        }
    }

    onStatusChanged: {
        if (status === PageStatus.Active && client.tracking && !acceptPending) {
            pageStack.replace(Qt.resolvedUrl("TrackingPage.qml"))
        }
    }

    onGroupChanged: {
        if (pinField) {
            pinField.text = ""
        }

        if (group && nicknameField) {
            nicknameField.text = client.nickname
            nicknameField.focus = true
        }
    }

    onAccepted: {
        errorMsg.text = ""

        if (!group) {
            client.startTracking()
        } else if (joining) {
            client.joinGroupShare(nicknameField.text, pinField.text)
        } else {
            client.createGroupShare(nicknameField.text)
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        PullDownMenu {
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
        }

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            DialogHeader {
                title: "Hauk"
                cancelText: ""
                acceptText: !page.group ? qsTr(
                                              "Start") : page.joining ? qsTr("Join") : qsTr(
                                                                            "Create")
            }

            // Sharing mode
            ComboBox {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                label: qsTr("Sharing mode")
                currentIndex: page.shareMode
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("Share my own location only")
                        onClicked: page.shareMode = 0
                    }
                    MenuItem {
                        text: qsTr("Create a group share")
                        onClicked: page.shareMode = 1
                    }
                    MenuItem {
                        text: qsTr("Join a group share")
                        onClicked: {
                            pinField.text = ""
                            page.shareMode = 2
                        }
                    }
                }
            }

            // Duration selector
            ComboBox {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                label: qsTr("Duration")
                currentIndex: {
                    switch (client.duration) {
                    case 1800:
                        return 0
                    case 3600:
                        return 1
                    case 7200:
                        return 2
                    case 14400:
                        return 3
                    case 43200:
                        return 4
                    case 86400:
                        return 5
                    default:
                        return 1
                    }
                }
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("30 minutes")
                        onClicked: client.duration = 1800
                    }
                    MenuItem {
                        text: qsTr("1 hour")
                        onClicked: client.duration = 3600
                    }
                    MenuItem {
                        text: qsTr("2 hours")
                        onClicked: client.duration = 7200
                    }
                    MenuItem {
                        text: qsTr("4 hours")
                        onClicked: client.duration = 14400
                    }
                    MenuItem {
                        text: qsTr("12 hours")
                        onClicked: client.duration = 43200
                    }
                    MenuItem {
                        text: qsTr("24 hours")
                        onClicked: client.duration = 86400
                    }
                }
            }

            // Interval selector
            ComboBox {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                label: qsTr("Update interval")
                currentIndex: {
                    switch (client.interval) {
                    case 10:
                        return 0
                    case 30:
                        return 1
                    case 60:
                        return 2
                    case 120:
                        return 3
                    case 300:
                        return 4
                    default:
                        return 1
                    }
                }
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("10 seconds")
                        onClicked: client.interval = 10
                    }
                    MenuItem {
                        text: qsTr("30 seconds")
                        onClicked: client.interval = 30
                    }
                    MenuItem {
                        text: qsTr("1 minute")
                        onClicked: client.interval = 60
                    }
                    MenuItem {
                        text: qsTr("2 minutes")
                        onClicked: client.interval = 120
                    }
                    MenuItem {
                        text: qsTr("5 minutes")
                        onClicked: client.interval = 300
                    }
                }
            }

            TextField {
                id: nicknameField
                width: parent.width
                visible: page.group
                label: qsTr("Nickname")
                placeholderText: qsTr("Your name on the map")
                EnterKey.iconSource: page.joining ? "image://theme/icon-m-enter-next" : "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: {
                    if (page.joining) {
                        pinField.focus = true
                    } else {
                        page.accept()
                    }
                }
            }

            TextField {
                id: pinField
                width: parent.width
                visible: page.joining
                label: qsTr("Group PIN")
                placeholderText: qsTr("6-digit group PIN")
                inputMethodHints: Qt.ImhDigitsOnly
                maximumLength: 6
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: page.accept()
            }

            // Server info
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr(
                          "Server:") + " " + (client.serverUrl ? client.serverUrl : qsTr(
                                                                     "Not configured"))
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
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

    Connections {
        target: client
        onError: errorMsg.text = message
        onTrackingChanged: {
            // If sharing was started outside this dialog (for example from the
            // cover), move to the tracking page. When the dialog itself starts
            // sharing, the acceptDestination push is already in progress.
            if (client.tracking && page.status === PageStatus.Active
                    && !page.acceptPending) {
                pageStack.replace(Qt.resolvedUrl("TrackingPage.qml"))
            }
        }
    }
}
