import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.hauk 1.0
import "pages"

ApplicationWindow {
    id: app

    property HaukClient client: HaukClient {}

    initialPage: Component {
        MainPage {}
    }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations
}
