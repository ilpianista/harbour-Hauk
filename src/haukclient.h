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
    Q_PROPERTY(QString nickname READ nickname WRITE setNickname NOTIFY nicknameChanged)
    Q_PROPERTY(bool allowAdoption READ allowAdoption WRITE setAllowAdoption NOTIFY allowAdoptionChanged)
    Q_PROPERTY(QString groupPin READ groupPin NOTIFY groupPinChanged)
    Q_PROPERTY(QString shareId READ shareId NOTIFY shareIdChanged)
    Q_PROPERTY(int activeShareMode READ activeShareMode NOTIFY activeShareModeChanged)
    Q_PROPERTY(bool groupHost READ isGroupHost NOTIFY activeShareModeChanged)
    Q_PROPERTY(bool groupMember READ isGroupMember NOTIFY activeShareModeChanged)
    Q_PROPERTY(QString adoptedGroupShareId READ adoptedGroupShareId NOTIFY adoptedGroupShareChanged)

public:
    enum ShareMode {
        ShareAlone = 0,
        ShareGroupHost = 1,
        ShareGroupMember = 2
    };
    Q_ENUM(ShareMode)

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

    QString nickname() const { return m_nickname; }
    void setNickname(const QString &nickname);

    bool allowAdoption() const { return m_allowAdoption; }
    void setAllowAdoption(bool allowAdoption);

    QString groupPin() const { return m_groupPin; }
    QString shareId() const { return m_shareId; }
    int activeShareMode() const { return m_activeShareMode; }
    bool isGroupHost() const { return m_activeShareMode == ShareGroupHost; }
    bool isGroupMember() const { return m_activeShareMode == ShareGroupMember; }
    QString adoptedGroupShareId() const { return m_adoptedGroupShareId; }

    Q_INVOKABLE void startTracking();
    Q_INVOKABLE void stopTracking();
    Q_INVOKABLE void createGroupShare(const QString &nickname);
    Q_INVOKABLE void joinGroupShare(const QString &nickname, const QString &pin);
    Q_INVOKABLE void adoptShare(const QString &shareId, const QString &nickname);

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
    void nicknameChanged();
    void allowAdoptionChanged();
    void groupPinChanged();
    void shareIdChanged();
    void activeShareModeChanged();
    void adoptedGroupShareChanged();
    void error(const QString &message);
    void adoptionFinished(bool success, const QString &message);

private slots:
    void onCreateSessionFinished();
    void onStopSessionFinished();
    void onPostLocationFinished();
    void onAdoptFinished();
    void updateLocation();
    void onPositionUpdated(const QGeoPositionInfo &info);

private:
    void startSession(int mode, const QString &nickname, const QString &pin);
    void createSession();
    void postLocation(double lat, double lon, double accuracy, double speed);
    void setStatus(const QString &s);
    void setTracking(bool t);
    void setSessionId(const QString &sid);
    void setShareLink(const QString &link);
    void setRemainingSeconds(int s);
    void setGroupPin(const QString &pin);
    void setShareId(const QString &id);
    void setActiveShareMode(int mode);
    void setAdoptedGroupShare(const QString &id);

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

    QString m_nickname;
    bool m_allowAdoption;
    QString m_joinPin;
    QString m_groupPin;
    QString m_shareId;
    QString m_adoptedGroupShareId;
    int m_shareMode;
    int m_activeShareMode;

    QNetworkAccessManager *m_nam;
    QNetworkReply *m_currentReply;
    QNetworkReply *m_adoptReply;
    QTimer *m_countdownTimer;
    QGeoPositionInfoSource *m_positionSource;
    int m_sessionExpiry;
    SecureStorage *m_secureStorage;
    QSettings *m_settings;
};
