#ifndef MAPIVK_H
#define MAPIVK_H

#include <QObject>
#include <QThread>
#include <QTime>
#include <QUrl>
#include <QIcon>
#include <QDomElement>

#include "core/mnetowrkaccess.h"

class QNetworkReply;

struct vkUserProfile
{
	QString name_f;
	QString name_l;
	QString name_n;
	quint64	id;
	QString screen_name;
	QUrl    photo;
	bool    online;

	enum userSex {
		FEMALE	= 1,
		MALE	= 2,
		UNKNOWN = 0
	} sex;

	vkUserProfile() : id(0) {}
	bool isValid() { return id > 0; }
	QString getName() { return name_f + " " + name_l; }
	QString getLink() { return "<a href='http://vk.com/" + screen_name + "'>" + getName() + "</a>"; }
	QIcon getIcon() { if (FEMALE == sex) return QIcon(":/icons/user-female.png"); if (MALE == sex) return QIcon(":/icons/user-male.png"); return QIcon(":/icons/user-silhouette-question.png"); }
};

typedef QList<vkUserProfile> vkUserProfileList;

struct vkAudio
{
	quint64 aid;
	qint64	ownerId;
	quint64 lyricsId;
	QString artist;
	QString title;
	uint	duration;
	QUrl	url;
	QString fileId;
	bool	isValid;

	vkAudio() : isValid(false) {}
	void toVkRecord(QDomElement &in);
	void fromVkRecord(QDomElement &in);
	void setUrl(QUrl newUrl);
};

typedef QList<vkAudio> vkAudioList;

enum vkAudioSort
{
	VK_AUDIO_SORT_POPULAR = 2,
	VK_AUDIO_SORT_LENGTH  = 1,
	VK_AUDIO_SORT_DATE    = 0
};

struct vkGroup
{
	QString title;
	QString screen_name;
	quint64 gid;
};

typedef QList<vkGroup> vkGroupList;

class mApiVkRequest : public QObject
{
	Q_OBJECT
public:
	explicit mApiVkRequest(QObject *parent = 0);
	~mApiVkRequest();
	QUrl getUrl();
	void process(QNetworkReply * rep);

	void profileLoad(quint64 userId);
	void groupInfoLoad(quint64 groupId);
	void friendsLoad(quint64 userId);
	void userAudioLoad(quint64 userId);
	void groupAudioLoad(quint64 groupId);
	void userGroupsLoad(quint64 userId);

	void sendWallAudio(quint64 userId, vkAudioList audio, QString message);
	void sendStatusAudio(qint64 oid, quint64 aid);
	void sendStatusOnline();

	void searchAudio(QString text, vkAudioSort sort, bool fixMisspell, uint offset = 0, uint size = 200);

	void getAudio(qint64 oid, quint64 aid);

	void setReply(QNetworkReply * r);
	void saveAudio(qint64 oid, quint64 aid);

private:
	enum apiMethod {
		API_GETPROFILE,
		API_GETGROUP,
		API_FRIENDS,
		API_USERADIO,
		API_USERGROUPS,
		API_WALLPOST,
		API_STATUSAUDIO,
		API_STATUSONLINE,
		API_SAVEAUDIO,
		API_SEARCHAUDIO,
		API_GETAUDIO
	} method;
	QUrl url;
	QUrl baseUrl(QString method);
	QString normalizeString(QString s);
	QNetworkReply * rep;

	qint64 posterUid;

signals:
	void apiError(int code, QString text);
	void profileLoaded(vkUserProfile profile);
	void groupinfoLoaded(vkGroup group);
	void friendsLoaded(vkUserProfileList list);
	void userAudioLoaded(vkAudioList list);
	void userGroupsLoaded(vkGroupList list);

	void searchAudioLoaded(vkAudioList list);

	void getAudioLoaded(vkAudio audio);

	void sendedWallAudio(qint64 userId, quint64 postId);
	void audioAdded();
	void statusOnline();

public slots:
private slots:
	void requestFinished();
};

class mApiVk : public QThread
{
	Q_OBJECT
public:
	explicit mApiVk(QObject *parent = 0);
	~mApiVk();
	static void request(mApiVkRequest * r);

	static mApiVk * instance();
	void run();

public slots:
	void addRequest(mApiVkRequest * r);
private:
	QList<int> requestHistory;
	mNetowrkAccess * net;
	QTime reqTimer;
	int calcWait();
};

#endif // MAPIVK_H
