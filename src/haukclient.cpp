#include "haukclient.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QGeoPositionInfoSource>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QRegularExpression>
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
    , m_allowAdoption(false)
    , m_shareMode(ShareAlone)
    , m_activeShareMode(ShareAlone)
    , m_nam(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
    , m_adoptReply(nullptr)
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
    m_nickname = m_settings->value("nickname", "").toString();
    m_allowAdoption = m_settings->value("allowAdoption", false).toBool();

    connect(m_secureStorage, &SecureStorage::initialized, this, [this]() {
        m_password = m_secureStorage->loadPassword();
        emit passwordChanged();
    });
    m_secureStorage->initialize();

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

void HaukClient::setNickname(const QString &nickname)
{
    if (m_nickname != nickname) {
        m_nickname = nickname;
        m_settings->setValue("nickname", nickname);
        emit nicknameChanged();
    }
}

void HaukClient::setAllowAdoption(bool allowAdoption)
{
    if (m_allowAdoption != allowAdoption) {
        m_allowAdoption = allowAdoption;
        m_settings->setValue("allowAdoption", allowAdoption);
        emit allowAdoptionChanged();
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

void HaukClient::setGroupPin(const QString &pin)
{
    if (m_groupPin != pin) {
        m_groupPin = pin;
        emit groupPinChanged();
    }
}

void HaukClient::setShareId(const QString &id)
{
    if (m_shareId != id) {
        m_shareId = id;
        emit shareIdChanged();
    }
}

void HaukClient::setActiveShareMode(int mode)
{
    if (m_activeShareMode != mode) {
        m_activeShareMode = mode;
        emit activeShareModeChanged();
    }
}

void HaukClient::setAdoptedGroupShare(const QString &id)
{
    if (m_adoptedGroupShareId != id) {
        m_adoptedGroupShareId = id;
        emit adoptedGroupShareChanged();
    }
}

void HaukClient::startTracking()
{
    startSession(ShareAlone, QString(), QString());
}

void HaukClient::createGroupShare(const QString &nickname)
{
    startSession(ShareGroupHost, nickname, QString());
}

void HaukClient::joinGroupShare(const QString &nickname, const QString &pin)
{
    startSession(ShareGroupMember, nickname, pin);
}

void HaukClient::adoptShare(const QString &shareId, const QString &nickname)
{
    if (!m_tracking || m_sessionId.isEmpty()) {
        emit adoptionFinished(false, QStringLiteral("No active sharing session"));
        return;
    }

    if (m_activeShareMode != ShareGroupHost || m_groupPin.isEmpty()) {
        emit adoptionFinished(false, QStringLiteral("Only group share hosts can adopt shares"));
        return;
    }

    const QString trimmedNickname = nickname.trimmed();
    if (trimmedNickname.isEmpty()) {
        emit adoptionFinished(false, QStringLiteral("A nickname is required"));
        return;
    }

    QString id = shareId.trimmed();
    static const QRegularExpression linkPattern(QStringLiteral("\\?([A-Za-z0-9-]+)"));
    const QRegularExpressionMatch match = linkPattern.match(id);
    if (match.hasMatch()) {
        id = match.captured(1);
    }

    if (id.isEmpty()) {
        emit adoptionFinished(false, QStringLiteral("A share ID is required"));
        return;
    }

    QUrl url(m_serverUrl + "/api/adopt.php");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery data;
    data.addQueryItem(QLatin1String("sid"), m_sessionId);
    data.addQueryItem(QLatin1String("nic"), QString(trimmedNickname).replace("+", "%2B"));
    data.addQueryItem(QLatin1String("aid"), id);
    data.addQueryItem(QLatin1String("pin"), m_groupPin);

    if (!m_username.isEmpty()) {
        data.addQueryItem(QLatin1String("usr"), QString(m_username).replace("+", "%2B"));
    }

    data.addQueryItem(QLatin1String("pwd"), QString(m_password).replace("+", "%2B"));

    if (m_adoptReply) {
        m_adoptReply->disconnect(this);
        m_adoptReply->abort();
        m_adoptReply->deleteLater();
        m_adoptReply = nullptr;
    }

    qDebug() << "Adopt share request:" << url << "share ID:" << id;

    QNetworkReply *reply = m_nam->post(request, data.toString(QUrl::FullyEncoded).toUtf8());
    m_adoptReply = reply;

    QTimer *timeout = new QTimer(reply);
    timeout->setSingleShot(true);
    connect(timeout, &QTimer::timeout, this, [this, reply]() {
        if (reply == m_adoptReply) {
            qDebug() << "Adopt share request timed out:" << reply->url();
            reply->setProperty("timedOut", true);
            reply->abort();
        }
    });
    timeout->start(15000);

    connect(reply, &QNetworkReply::finished, this, &HaukClient::onAdoptFinished);
}

void HaukClient::startSession(int mode, const QString &nickname, const QString &pin)
{
    if (m_serverUrl.isEmpty()) {
        emit error("Server URL not configured");
        return;
    }

    const QString trimmedNickname = nickname.trimmed();
    const QString trimmedPin = pin.trimmed();

    if (mode != ShareAlone && trimmedNickname.isEmpty()) {
        emit error("A nickname is required for group shares");
        return;
    }

    if (mode == ShareGroupMember) {
        static const QRegularExpression pinPattern(QStringLiteral("^\\d{6}$"));
        if (!pinPattern.match(trimmedPin).hasMatch()) {
            emit error("The group PIN must be exactly 6 digits");
            return;
        }
    }

    m_shareMode = mode;
    m_joinPin = trimmedPin;
    setAdoptedGroupShare(QString());

    if (!trimmedNickname.isEmpty()) {
        setNickname(trimmedNickname);
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
    setGroupPin("");
    setShareId("");
    setActiveShareMode(ShareAlone);
    setAdoptedGroupShare(QString());
    setRemainingSeconds(0);
    m_joinPin.clear();
    m_shareMode = ShareAlone;
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
    data.addQueryItem(QLatin1String("mod"), QString::number(m_shareMode));

    if (m_shareMode == ShareAlone) {
        data.addQueryItem(QLatin1String("ado"), m_allowAdoption ? QLatin1String("1") : QLatin1String("0"));
    } else {
        data.addQueryItem(QLatin1String("nic"), QString(m_nickname).replace("+", "%2B"));

        if (m_shareMode == ShareGroupMember) {
            data.addQueryItem(QLatin1String("pin"), m_joinPin);
        }
    }

    if (!m_username.isEmpty()) {
        data.addQueryItem(QLatin1String("usr"), QString(m_username).replace("+", "%2B"));
    }

    // Always send pwd parameter as the server requires it for authentication
    // (even if empty, the server will verify against its configured hash)
    data.addQueryItem(QLatin1String("pwd"), QString(m_password).replace("+", "%2B"));

    if (m_currentReply) {
        m_currentReply->disconnect(this);
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    qDebug() << "Create session request:" << url;

    QNetworkReply *reply = m_nam->post(request, data.toString(QUrl::FullyEncoded).toUtf8());
    m_currentReply = reply;

    QTimer *timeout = new QTimer(reply);
    timeout->setSingleShot(true);
    connect(timeout, &QTimer::timeout, this, [this, reply]() {
        if (reply == m_currentReply) {
            qDebug() << "Create session request timed out:" << reply->url();
            reply->setProperty("timedOut", true);
            reply->abort();
        }
    });
    timeout->start(15000);

    connect(reply, &QNetworkReply::finished, this, &HaukClient::onCreateSessionFinished);
}

void HaukClient::onCreateSessionFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    if (reply != m_currentReply) {
        reply->deleteLater();
        return;
    }

    const bool timedOut = reply->property("timedOut").toBool();
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString networkErrorString = reply->errorString();
    const QByteArray response = reply->readAll();

    reply->deleteLater();
    m_currentReply = nullptr;

    QString responseStr = QString::fromUtf8(response).trimmed();
    qDebug() << "Create session response:" << responseStr
             << "error:" << networkError << networkErrorString;

    if (timedOut) {
        setStatus("Session creation timed out");
        emit error("The server did not respond in time");
        return;
    }

    if (networkError != QNetworkReply::NoError) {
        setStatus("Failed to create session");
        emit error(responseStr.isEmpty() ? networkErrorString : responseStr);
        return;
    }

    // Parse response: OK\n<session_id>\n<share_link>[\n<group_pin>]\n<share_id>
    QStringList lines = responseStr.split('\n');
    if (lines.isEmpty() || !lines[0].startsWith("OK")) {
        setStatus("Failed to create session");
        emit error(responseStr.isEmpty() ? networkErrorString : responseStr);
        return;
    }

    if (lines.size() < 3) {
        setStatus("Invalid server response");
        emit error("Invalid response format");
        return;
    }

    const QString sessionId = lines[1].trimmed();
    const QString shareLink = lines[2].trimmed();

    if (m_shareMode == ShareGroupHost) {
        if (lines.size() < 5) {
            setStatus("Invalid server response");
            emit error("Invalid response format");
            return;
        }

        setGroupPin(lines[3].trimmed());
        setShareId(lines[4].trimmed());
    } else {
        setGroupPin(QString());
        setShareId(lines.size() > 3 ? lines[3].trimmed()
                                    : shareLink.mid(shareLink.indexOf('?') + 1));
    }

    setSessionId(sessionId);
    setShareLink(shareLink);
    setActiveShareMode(m_shareMode);
    setTracking(true);
    setRemainingSeconds(m_duration);
    setStatus("Sharing location");

    m_sessionExpiry = QDateTime::currentMSecsSinceEpoch() / 1000 + m_duration;

    m_countdownTimer->start(1000);
    m_positionSource->setUpdateInterval(m_interval * 1000);
    m_positionSource->startUpdates();
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

void HaukClient::onAdoptFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    if (reply != m_adoptReply) {
        reply->deleteLater();
        return;
    }

    const bool timedOut = reply->property("timedOut").toBool();
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString networkErrorString = reply->errorString();
    const QByteArray response = reply->readAll();

    reply->deleteLater();
    m_adoptReply = nullptr;

    const QString responseStr = QString::fromUtf8(response).trimmed();
    qDebug() << "Adopt share response:" << responseStr
             << "error:" << networkError << networkErrorString;

    if (timedOut) {
        emit adoptionFinished(false, QStringLiteral("The server did not respond in time"));
        return;
    }

    if (networkError != QNetworkReply::NoError) {
        emit adoptionFinished(false, responseStr.isEmpty() ? networkErrorString : responseStr);
        return;
    }

    if (responseStr.startsWith(QStringLiteral("OK"))) {
        emit adoptionFinished(true, QString());
        return;
    }

    emit adoptionFinished(false, responseStr.isEmpty() ? networkErrorString : responseStr);
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

    // The server returns all shares this session contributes to. When a solo
    // share is adopted, the group share appears in this list in addition to
    // the user's own share.
    if (responseStr.startsWith("OK")) {
        const QStringList lines = responseStr.split('\n');
        if (lines.size() >= 2) {
            const QString targetList = lines.size() > 2 ? lines[2] : QString();
            const QStringList targetIds = targetList.split(',');

            QString adoptedId;
            for (const QString &targetId : targetIds) {
                const QString id = targetId.trimmed();
                if (!id.isEmpty() && id != m_shareId) {
                    adoptedId = id;
                    break;
                }
            }

            setAdoptedGroupShare(adoptedId);
        }
    } else if (responseStr.contains("expired")) {
        stopTracking();
        emit error("Session expired");
    }
}
