import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.hauk 1.0
import "pages"

ApplicationWindow {
    id: app

    property HaukClient client: HaukClient {}

    Component {
        id: startPage

        MainPage {}
    }

    Component {
        id: trackingPage

        TrackingPage {}
    }

    initialPage: client.tracking ? trackingPage : startPage
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations
}
