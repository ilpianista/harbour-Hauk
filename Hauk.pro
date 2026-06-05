TARGET = harbour-hauk

CONFIG += sailfishapp

QT += positioning

PKGCONFIG += sailfishsecrets

INCLUDEPATH += /usr/include/Sailfish

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

SOURCES += \
    src/main.cpp \
    src/haukclient.cpp \
    src/securestorage.cpp

HEADERS += \
    src/haukclient.h \
    src/securestorage.h

OTHER_FILES += \
    qml/cover/CoverPage.qml \
    qml/Hauk.qml \
    qml/pages/MainPage.qml \
    qml/pages/SettingsPage.qml \
    harbour-hauk.desktop \
    rpm/harbour-hauk.changes \
    rpm/harbour-hauk.spec \
    translations/*.ts

CONFIG += sailfishapp_i18n

PKGCONFIG += sailfishsecrets

TRANSLATIONS += \
    translations/harbour-hauk-et.ts \
    translations/harbour-hauk-it.ts \
    translations/harbour-hauk-nb_NO.ts
