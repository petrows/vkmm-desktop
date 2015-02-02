#ifndef CORE_H
#define CORE_H

#include <QObject>
#include <QSettings>
#include <QtNetwork/QNetworkAccessManager>
#include <QTime>
#include <QTimer>
#include "core/mapivk.h"
#include "core/mapilastfm.h"
#include "core/mplaylist.h"

class wndMain;
class wndSessionLoad;
class mNetworkCookieJar;
class mNetowrkAccess;

class mCore : public QObject
{
    Q_OBJECT
public:
	explicit mCore(QObject *parent = 0);
	~mCore();

	// Singleton interface...
	static mCore * instance();

	void init();
	void init_login_window();
	void init_lastfm();
	void init_lastfm_session();
	void init_lastfm_data();
	void logOut();

	void reloadSettings();

	QSettings * settings;

	enum repeatType {
		REPEAT_LIST,
		REPEAT_TRACK
	} repeat;
	enum randomType {
		RANDOM_OFF = 0,
		RANDOM_ON
	} random;

	enum statusType {
		STATUS_OK,
		STATUS_VK,
		STATUS_LASTFM,
		STATUS_WARN,
		STATUS_FAIL
	};

	void setRepeat(mCore::repeatType t);
	void setRandom(mCore::randomType t);

	wndMain * uiWndMain;
	wndSessionLoad * uiSessionLoad;
	mNetowrkAccess * net;
	mNetworkCookieJar * cookies;

	bool isLastfmLovedLoaded;

	QString pathStore;
	QString pathCache;

	QString updateKey;

	bool isLogged() { return logged; }
	QString getApiTokenVk() { return apiVkToken; }
	vkUserProfile user;
	vkUserProfileList userFriends;
	vkGroupList userGroups;
	vkUserProfile userFriendById(quint64 id);

	void createPlaylistFromVkUser(quint64 userId, vkUserProfile info = vkUserProfile());
	void createPlaylistFromVkGroup(quint64 groupId, vkGroup info = vkGroup());
	void createPlaylistFromVkSearch(QString text, vkAudioSort sort, bool fixMisspell, bool exactSearch, quint32 pages);
	void createPlayListFromLastfmSimTrack(QString artist, QString title);
	void createPlayListFromLastfmUserRecomendation(QString user, uint limit);
	void createPlayListFromLastfmUserLoved(QString user, uint limit);
	void createPlayListFromLastfmUserTop(QString user, mApiLastfmRequest::apiTopPeriod period, uint limit);
	void createPlayListFromLastfmArtistTop(QString artist, QString artistMbid, uint limit);
	void removePlayList(uint index);

	bool isPrevAvaliable();

	void sendAudioStatus(vkAudio track);

	void setPlayListFilter(uint index, QString text);
	QString getPlayListFilter(uint index);

	vkUserProfile getLastSelectedVkUser();
	void setLastSelectedVkUser(vkUserProfile user);

	bool isItemLastfmLoved(QString artist, QString title);
	bool isItemLastfmLoved(QString mbId);

	QList<mPlayList*> lists;
protected:
	bool logged;
	QString apiVkToken;
	quint64 apiVkUid;
	mPlayListItem * itemCurrent;
	QList<mPlayListItem*> itemHistory;
	int itemHistoryPos;

	// User-data caches
	QList< QPair<QString,QString> > userLovedLastFmTitles;
	QList< QString > userLovedLastFmMbid;

	// Play-time timers
	QTime	ptsTimer;
	QTime	ptsTimerStatus;
	int		ptsTime;

signals:
	void loginChanged(bool status);
	void loginInitDone();
	void statusMessageChanged(mCore::statusType type, QString txt);
	void sessionProgress(int pers);
	void playListAdded(mPlayList * list);
	void playListRemoved(uint index);
	void repeatChanged(mCore::repeatType t);
	void randomChanged(mCore::randomType t);
	void playbackStarted(mPlayListItem * item);
	void playbackStopped();
	void volumeChanged(uint vol);
	void playedPers(int pers);

	void lastfmTrackScrobbed(QString artist, QString title);

	void lastFmSessionReject();
	void lastFmSessionOk();

	void metaUpdated();


public slots:
	void qsaMessage(QString msg);
	void statusMessage(mCore::statusType type, QString msg);
	void statusMessageAudioAdded();
	void statusMessageAudioPosted(qint64 uid, quint64 postId);
	void playItem(mPlayListItem * item, bool fromHistory = false);
	void playNext();
	void playPrev();
	void setVolume(uint vol);
	void addCurrentToVkAudio();
	void addCurrentToVkUpdates();
    void addCurrentToVkFriend();

	void updateMetaData();

	void metaReloadLastFmLoved(lastFmTrackList list);
	void lovedToggle(mPlayListItem * itm, bool isLoved);

private slots:
	void init_profile_loaded(vkUserProfile pp);
	void init_friends_loaded(vkUserProfileList lst);
	void init_groups_loaded(vkGroupList lst);
	void initError(int code, QString msg);
	void authDone(QString token, qint32 expired, qint32 userId);
	void authReject();

	void lastfmSessionDone(bool res, QString user, QString sid);

    void updateNeed(bool need, QString version);

	void streamEnded();
	void streamStarted(mPlayListItem * item);

	void vkAudioAdded();
	void vkAudioPosted();

	void lastfmResultScrobbed(QString artist, QString title);

	void ptsStart();
	void ptsProgress(double progress, int playTime, int totalTime);
	void ptsPaused();
};

#endif // CORE_H
