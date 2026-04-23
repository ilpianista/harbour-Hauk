#pragma once
#include <QGeoPositionInfoSource>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QTimer>

class SecureStorage;

class HaukClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(int interval READ interval WRITE setInterval NOTIFY intervalChanged)
    Q_PROPERTY(QString sessionId READ sessionId NOTIFY sessionIdChanged)
    Q_PROPERTY(QString shareLink READ shareLink NOTIFY shareLinkChanged)
    Q_PROPERTY(bool tracking READ tracking NOTIFY trackingChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(int remainingSeconds READ remainingSeconds NOTIFY remainingSecondsChanged)
    Q_PROPERTY(double latitude READ latitude NOTIFY positionChanged)
    Q_PROPERTY(double longitude READ longitude NOTIFY positionChanged)
    Q_PROPERTY(double accuracy READ accuracy NOTIFY positionChanged)

public:
    explicit HaukClient(QObject *parent = nullptr);

    QString serverUrl() const { return m_serverUrl; }
    void setServerUrl(const QString &url);

    QString username() const { return m_username; }
    void setUsername(const QString &user);

    QString password() const { return m_password; }
    void setPassword(const QString &pwd);

    int duration() const { return m_duration; }
    void setDuration(int d);

    int interval() const { return m_interval; }
    void setInterval(int i);

    QString sessionId() const { return m_sessionId; }
    QString shareLink() const { return m_shareLink; }
    bool tracking() const { return m_tracking; }
    QString status() const { return m_status; }
    int remainingSeconds() const { return m_remainingSeconds; }
    double latitude() const { return m_latitude; }
    double longitude() const { return m_longitude; }
    double accuracy() const { return m_accuracy; }

    Q_INVOKABLE void startTracking();
    Q_INVOKABLE void stopTracking();

signals:
    void serverUrlChanged();
    void usernameChanged();
    void passwordChanged();
    void durationChanged();
    void intervalChanged();
    void sessionIdChanged();
    void shareLinkChanged();
    void trackingChanged();
    void statusChanged();
    void remainingSecondsChanged();
    void positionChanged();
    void error(const QString &message);

private slots:
    void onCreateSessionFinished();
    void onStopSessionFinished();
    void onPostLocationFinished();
    void updateLocation();
    void onPositionUpdated(const QGeoPositionInfo &info);

private:
    void createSession();
    void postLocation(double lat, double lon, double accuracy, double speed);
    void setStatus(const QString &s);
    void setTracking(bool t);
    void setSessionId(const QString &sid);
    void setShareLink(const QString &link);
    void setRemainingSeconds(int s);

    QString m_serverUrl;
    QString m_username;
    QString m_password;
    int m_duration;
    int m_interval;

    QString m_sessionId;
    QString m_shareLink;
    bool m_tracking;
    QString m_status;
    int m_remainingSeconds;
    double m_latitude;
    double m_longitude;
    double m_accuracy;

    QNetworkAccessManager *m_nam;
    QNetworkReply *m_currentReply;
    QTimer *m_countdownTimer;
    QGeoPositionInfoSource *m_positionSource;
    int m_sessionExpiry;
    SecureStorage *m_secureStorage;
    QSettings *m_settings;
};
