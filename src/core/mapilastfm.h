#ifndef MAPILASTFM_H
#define MAPILASTFM_H

#include <QObject>
#include <QUrl>
#include <QThread>
#include <QList>
#include <QPair>

#include <QDomElement>

#include "core/mnetowrkaccess.h"
class QNetworkReply;

class lastFmTrack
{
public:
	QString artist;
	QString title;
	QString mbid;
	QString mbidArtist;

	void toXml(QDomElement &in);
	void fromXml(QDomElement &in);
};
typedef QList<lastFmTrack> lastFmTrackList;

class lastFmUser
{
public:
	QString name;
	QString realname;
};
typedef QList<lastFmUser> lastFmUserList;

class mApiLastfm;

class mApiLastfmRequest : public QObject
{
	Q_OBJECT
public:

	enum apiTopPeriod
	{
		PERIOD_ALL,
		PERIOD_WEEK,
		PERIOD_MONTH_3,
		PERIOD_MONTH_6,
		PERIOD_MONTH_12
	};

	explicit mApiLastfmRequest(QObject *parent = 0);
	~mApiLastfmRequest();
	QUrl baseUrl(QString method);
	void process(QNetworkReply * rep);

	void sessionOpen(QString user, QString token);
	void trackNowPlaying(QString artist, QString title, QString mbId = "");
	void trackScrobble(QString artist, QString title, quint32 duration = 0, QString mbId = "", quint64 streamId = 0, uint tms = 0);
	void trackLove(QString artist, QString title, bool isLove);

	void userLoved(QString user, uint limit = 2000);
	void userRecomended(QString user, uint limit = 2000);
	void userTracks(QString user, mApiLastfmRequest::apiTopPeriod period, uint limit = 2000);
	void userFriends(QString user);

	void artistTracks(QString artist, QString mbid, uint limit = 2000);

	void similarTrack(QString artist, QString title, QString mbid);

	void setSign();
	void setReply(QNetworkReply * r);
	QUrl getUrl();
	bool postMode() { return isPost; }

signals:
	void sessionOpenResult(bool isOk, QString userName, QString sid);
	void trackScrobbed(QString artist, QString title);
	void userLovedLoaded(lastFmTrackList list);
	void userTracksLoaded(lastFmTrackList list);
	void userFriendsLoaded(lastFmUserList list);
	void similarLoaded(lastFmTrackList list);
	void tracksListLoaded(lastFmTrackList list);
    void apiError(int code, QString text);
	void loadProgress(int bytes);
private:
	enum apiMethod {
		API_SESSION_OPEN,
		API_TRACK_NOWPLAYING,
		API_TRACK_SCROBBLE,
		API_TRACK_LOVE,
		API_USER_LOVED,
		API_USER_RECOMEND,
		API_USER_TOP,
		API_USER_FRIENDS,
		API_ARTIST_TOP,
		API_SIM_TRACK,
		API_SIM_ARTIST
	} method;
	QUrl url;
	bool isPost;
	QNetworkReply * rep;

	QString requestData1;
	QString requestData2;

private slots:
	void requestFinished();
	void requestProgress(qint64 bytesReceived , qint64 bytesTotal);
};

class mApiLastfm : public QThread
{
	Q_OBJECT
public:
	explicit mApiLastfm(QObject *parent = 0);
	~mApiLastfm();

	static mApiLastfm * instance();
	void run();

	static void request(mApiLastfmRequest * r);

	static QString genToken(QString login, QString password);
	static QString genSign(QUrl requestUrl);
	static void setSign(QUrl &requestUrl);

	void setSid(QString newSid);
	bool isActive();

signals:
	
public slots:
	void addRequest(mApiLastfmRequest * r);
private:
	mNetowrkAccess * net;
	QString sid;
};

#endif // MAPILASTFM_H
