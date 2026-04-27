import QtQuick 2.2
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height

        Column {
            id: content
            width: parent.width

            PageHeader {
                title: qsTr("Settings")
            }

            SectionHeader {
                text: qsTr("Server")
            }

            TextField {
                id: serverUrlField
                width: parent.width
                placeholderText: "https://your-server.com/hauk"
                text: client.serverUrl
                label: qsTr("Server URL")
                inputMethodHints: Qt.ImhUrlCharactersOnly
                onTextChanged: client.serverUrl = text
            }

            SectionHeader {
                text: qsTr("Authentication")
            }

            TextField {
                id: usernameField
                width: parent.width
                placeholderText: qsTr("Username (optional)")
                text: client.username
                label: qsTr("Username")
                onTextChanged: client.username = text
            }

            TextField {
                id: passwordField
                width: parent.width
                placeholderText: qsTr("Password")
                text: client.password
                label: qsTr("Password")
                echoMode: TextInput.Password
                onTextChanged: client.password = text
            }

            SectionHeader {
                text: qsTr("Defaults")
            }

            ComboBox {
                width: parent.width
                label: qsTr("Default duration")
                currentIndex: {
                    switch (client.duration) {
                    case 1800:
                        return 0;
                    case 3600:
                        return 1;
                    case 7200:
                        return 2;
                    case 14400:
                        return 3;
                    case 43200:
                        return 4;
                    default:
                        return 1;
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
                }
            }

            ComboBox {
                width: parent.width
                label: qsTr("Default update interval")
                currentIndex: {
                    switch (client.interval) {
                    case 10:
                        return 0;
                    case 30:
                        return 1;
                    case 60:
                        return 2;
                    case 120:
                        return 3;
                    case 300:
                        return 4;
                    default:
                        return 1;
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
        }

        VerticalScrollDecorator {}
    }
}
