#include "haukclient.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QGeoPositionInfoSource>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrlQuery>

#include "securestorage.h"

HaukClient::HaukClient(QObject *parent)
    : QObject(parent)
    , m_duration(3600)
    , m_interval(30)
    , m_tracking(false)
    , m_remainingSeconds(0)
    , m_latitude(0)
    , m_longitude(0)
    , m_accuracy(0)
    , m_nam(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
    , m_countdownTimer(new QTimer(this))
    , m_secureStorage(new SecureStorage(this))
{
    const QString settingsPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                                 + QDir::separator() + QCoreApplication::applicationName()
                                 + ".conf";
    m_settings = new QSettings(settingsPath, QSettings::NativeFormat, this);

    m_serverUrl = m_settings->value("serverUrl", "https://example.com/hauk").toString();
    m_username = m_settings->value("username", "").toString();
    m_duration = m_settings->value("duration", 3600).toInt();
    m_interval = m_settings->value("interval", 30).toInt();

    m_secureStorage->initialize();
    m_password = m_secureStorage->loadPassword();

    m_positionSource = QGeoPositionInfoSource::createDefaultSource(this);

    connect(m_countdownTimer, &QTimer::timeout, this, [this]() {
        int remaining = m_remainingSeconds - 1;
        if (remaining <= 0) {
            stopTracking();
        } else {
            setRemainingSeconds(remaining);
        }
    });
    connect(m_positionSource,
            &QGeoPositionInfoSource::positionUpdated,
            this,
            &HaukClient::onPositionUpdated);
}

void HaukClient::setServerUrl(const QString &url)
{
    if (m_serverUrl != url) {
        m_serverUrl = url;
        m_settings->setValue("serverUrl", url);
        emit serverUrlChanged();
    }
}

void HaukClient::setUsername(const QString &user)
{
    if (m_username != user) {
        m_username = user;
        m_settings->setValue("username", user);
        emit usernameChanged();
    }
}

void HaukClient::setPassword(const QString &pwd)
{
    if (m_password != pwd) {
        m_password = pwd;
        if (m_secureStorage) {
            m_secureStorage->savePassword(pwd);
        }
        emit passwordChanged();
    }
}

void HaukClient::setDuration(int d)
{
    if (m_duration != d) {
        m_duration = d;
        m_settings->setValue("duration", d);
        emit durationChanged();
    }
}

void HaukClient::setInterval(int i)
{
    if (m_interval != i) {
        m_interval = i;
        m_settings->setValue("interval", i);
        emit intervalChanged();
    }
}

void HaukClient::setStatus(const QString &s)
{
    if (m_status != s) {
        m_status = s;
        emit statusChanged();
    }
}

void HaukClient::setTracking(bool t)
{
    if (m_tracking != t) {
        m_tracking = t;
        emit trackingChanged();
    }
}

void HaukClient::setSessionId(const QString &sid)
{
    if (m_sessionId != sid) {
        m_sessionId = sid;
        emit sessionIdChanged();
    }
}

void HaukClient::setShareLink(const QString &link)
{
    if (m_shareLink != link) {
        m_shareLink = link;
        emit shareLinkChanged();
    }
}

void HaukClient::setRemainingSeconds(int s)
{
    if (m_remainingSeconds != s) {
        m_remainingSeconds = s;
        emit remainingSecondsChanged();
    }
}

void HaukClient::startTracking()
{
    if (m_serverUrl.isEmpty()) {
        emit error("Server URL not configured");
        return;
    }

    setStatus("Creating session...");
    createSession();
}

void HaukClient::stopTracking()
{
    if (m_sessionId.isEmpty())
        return;

    setStatus("Stopping session...");

    QUrl url(m_serverUrl + "/api/stop.php");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // QUrlQuery doesn't convert "+" to "&2B" so we must perform this manually
    // See https://doc.qt.io/qt-5/qurlquery.html#handling-of-spaces-and-plus
    QUrlQuery data;
    data.addQueryItem(QLatin1String("sid"), m_sessionId);

    if (!m_username.isEmpty()) {
        data.addQueryItem(QLatin1String("usr"), QString(m_username).replace("+", "%2B"));
    }

    if (!m_password.isEmpty()) {
        data.addQueryItem(QLatin1String("pwd"), QString(m_password).replace("+", "%2B"));
    }

    m_currentReply = m_nam->post(request, data.toString(QUrl::FullyEncoded).toUtf8());
    connect(m_currentReply, &QNetworkReply::finished, this, &HaukClient::onStopSessionFinished);

    m_countdownTimer->stop();
    m_positionSource->stopUpdates();
    setTracking(false);
    setSessionId("");
    setShareLink("");
    setRemainingSeconds(0);
    setStatus("Stopped");
}

void HaukClient::createSession()
{
    QUrl url(m_serverUrl + "/api/create.php");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // QUrlQuery doesn't convert "+" to "&2B" so we must perform this manually
    // See https://doc.qt.io/qt-5/qurlquery.html#handling-of-spaces-and-plus
    QUrlQuery data;
    data.addQueryItem(QLatin1String("int"), QString::number(m_interval));
    data.addQueryItem(QLatin1String("dur"), QString::number(m_duration));
    data.addQueryItem(QLatin1String("sid"), m_sessionId);

    if (!m_username.isEmpty()) {
        data.addQueryItem(QLatin1String("usr"), QString(m_username).replace("+", "%2B"));
    }

    if (!m_password.isEmpty()) {
        data.addQueryItem(QLatin1String("pwd"), QString(m_password).replace("+", "%2B"));
    }

    m_currentReply = m_nam->post(request, data.toString(QUrl::FullyEncoded).toUtf8());
    connect(m_currentReply, &QNetworkReply::finished, this, &HaukClient::onCreateSessionFinished);
}

void HaukClient::onCreateSessionFinished()
{
    if (!m_currentReply)
        return;

    QByteArray response = m_currentReply->readAll();
    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    QString responseStr = QString::fromUtf8(response);
    //qDebug() << "Create session response:" << responseStr;

    // Parse response: OK\n<share_link>\n<session_id>
    QStringList lines = responseStr.split('\n');
    if (lines.isEmpty() || !lines[0].startsWith("OK")) {
        setStatus("Failed to create session");
        emit error(responseStr);
        return;
    }

    if (lines.size() >= 3) {
        QString sessionId = lines[1];
        QString shareLink = lines[2];

        setSessionId(sessionId);
        setShareLink(shareLink);
        setTracking(true);
        setRemainingSeconds(m_duration);
        setStatus("Sharing location");

        m_sessionExpiry = QDateTime::currentMSecsSinceEpoch() / 1000 + m_duration;

        m_countdownTimer->start(1000);
        m_positionSource->setUpdateInterval(m_interval * 1000);
        m_positionSource->startUpdates();
    } else {
        setStatus("Invalid server response");
        emit error("Invalid response format");
    }
}

void HaukClient::onStopSessionFinished()
{
    if (!m_currentReply)
        return;

    QByteArray response = m_currentReply->readAll();
    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    qDebug() << "Stop session response:" << QString::fromUtf8(response);
}

void HaukClient::updateLocation()
{
    // Position will come via onPositionUpdated signal
}

void HaukClient::onPositionUpdated(const QGeoPositionInfo &info)
{
    if (!info.isValid())
        return;

    QGeoCoordinate coord = info.coordinate();
    m_latitude = coord.latitude();
    m_longitude = coord.longitude();
    m_accuracy = info.attribute(QGeoPositionInfo::HorizontalAccuracy);
    double speed = info.attribute(QGeoPositionInfo::GroundSpeed);

    emit positionChanged();

    postLocation(m_latitude, m_longitude, m_accuracy, speed);
}

void HaukClient::postLocation(double lat, double lon, double accuracy, double speed)
{
    if (m_sessionId.isEmpty())
        return;

    QUrl url(m_serverUrl + "/api/post.php");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    qint64 timestamp = QDateTime::currentMSecsSinceEpoch() / 1000;

    // QUrlQuery doesn't convert "+" to "&2B" so we must perform this manually
    // See https://doc.qt.io/qt-5/qurlquery.html#handling-of-spaces-and-plus
    QUrlQuery data;
    data.addQueryItem(QLatin1String("lat"), QString::number(lat, 'f', 6));
    data.addQueryItem(QLatin1String("lon"), QString::number(lon, 'f', 6));
    data.addQueryItem(QLatin1String("time"), QString::number(timestamp));
    data.addQueryItem(QLatin1String("acc"), QString::number(accuracy, 'f', 1));
    data.addQueryItem(QLatin1String("sid"), m_sessionId);

    if (speed >= 0) {
        data.addQueryItem(QLatin1String("spd"), QString::number(speed, 'f', 1));
    }

    if (!m_username.isEmpty()) {
        data.addQueryItem(QLatin1String("usr"), QString(m_username).replace("+", "%2B"));
    }

    if (!m_password.isEmpty()) {
        data.addQueryItem(QLatin1String("pwd"), QString(m_password).replace("+", "%2B"));
    }

    m_currentReply = m_nam->post(request, data.toString(QUrl::FullyEncoded).toUtf8());
    connect(m_currentReply, &QNetworkReply::finished, this, &HaukClient::onPostLocationFinished);
}

void HaukClient::onPostLocationFinished()
{
    if (!m_currentReply)
        return;

    QByteArray response = m_currentReply->readAll();
    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    QString responseStr = QString::fromUtf8(response).trimmed();
    qDebug() << "Post location response:" << responseStr;

    // Check if session expired
    if (responseStr.startsWith("Session expired") || responseStr.contains("expired")) {
        stopTracking();
        emit error("Session expired");
    }
}
