import QtQuick 2.2
import Sailfish.Silica 1.0

Dialog {
    id: dialog
    allowedOrientations: Orientation.All

    property bool submitting: false
    property string errorText: ""

    // The request is asynchronous, so accepting must not navigate forward
    // immediately. Keep the dialog open and pop it once adoption succeeds.
    forwardNavigation: false
    backNavigation: !submitting

    canAccept: !submitting && shareIdField.text.trim().length > 0
               && nicknameField.text.trim().length > 0

    function accept() {
        if (!canAccept || submitting)
            return

        errorText = ""
        submitting = true
        client.adoptShare(shareIdField.text, nicknameField.text)
    }

    onOpened: {
        shareIdField.text = ""
        nicknameField.text = ""
        errorText = ""
        submitting = false
        shareIdField.focus = true
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            DialogHeader {
                title: qsTr("Adopt a share")
                acceptText: qsTr("Adopt")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Enter the ID of a share that allows adoption. The share will be added to your group.")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            TextField {
                id: shareIdField
                width: parent.width
                enabled: !dialog.submitting
                label: qsTr("Share ID")
                placeholderText: qsTr("e.g. KYL6-WWNJ")
                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: nicknameField.focus = true
                onTextChanged: if (dialog.errorText.length > 0)
                                   dialog.errorText = ""
            }

            TextField {
                id: nicknameField
                width: parent.width
                enabled: !dialog.submitting
                label: qsTr("Nickname")
                placeholderText: qsTr("Share name on the map")
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: dialog.accept()
                onTextChanged: if (dialog.errorText.length > 0)
                                   dialog.errorText = ""
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: dialog.errorText
                color: Theme.errorColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
                visible: text.length > 0
            }

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                size: BusyIndicatorSize.Medium
                running: dialog.submitting
                visible: dialog.submitting
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }
        }

        VerticalScrollDecorator {}
    }

    Connections {
        target: client
        onAdoptionFinished: {
            dialog.submitting = false

            if (success) {
                if (dialog.status === PageStatus.Active)
                    pageStack.pop()
            } else {
                dialog.errorText = message
            }
        }
    }
}
