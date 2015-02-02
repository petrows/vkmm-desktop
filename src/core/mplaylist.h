#ifndef MPLAYLIST_H
#define MPLAYLIST_H

#include <QObject>
#include <QList>
#include <QDomDocument>
#include "core/mrefcounter.h"
#include "core/mapivk.h"
#include "core/mapilastfm.h"

class mPlayListItem;
class mPlayList;

const quint32 vkSearchPageSize = 200;

class mPlayListItem : public mRefCounter
{
	Q_OBJECT
public:
	explicit mPlayListItem(QObject *parent = 0);
	~mPlayListItem();

	void setVkRecord(vkAudio rec);

	QString & getAudioArtist();
	QString & getAudioTitle();
	int       getAudioLength();
	QString getTimeFormatted();

    mPlayListItem * copy();
	void toDomElement(QDomElement & in, QDomDocument * doc);
	void fromDomElement(QDomElement & in);

	bool isCached();

	void updateMetaData();
	bool selectBestVkSearched(vkAudioList & list);

	bool isPlaying;
	bool isSearched;
	bool isError;
	int  indexOriginal;

	bool isLovedLastFm;
	bool isInUserVk;

	vkAudio vkRecord;
	lastFmTrack lastfmTrack;
	mPlayList * playList;
};

class mPlayList : public QObject
{
	Q_OBJECT
public:
	explicit mPlayList(QObject *parent = 0);
	~mPlayList();

	enum playListLoadState {
		LS_IDLE = 0,
		LS_LOADING,
		LS_DONE,
		LS_ERROR
	} loadState;

	enum playListOrigin {
		ORIGIN_UNKNOWN = 0,
		FROM_VK_USER,
		FROM_VK_GROUP,
		FROM_VK_SEARCH,
		FROM_LASTFM_SIM,
		FROM_LASTFM_USER,
		FROM_LASTFM_ARTIST
	} origin;

	enum playListSorting {
		SORT_ORIGINAL = 0,
		SORT_NAME,
		SORT_LENGTH
	//	SORT_POPULAR
	} sort;
	enum playListSortingOrder {
		ORDER_ASC = 0,
		ORDER_DESC
	} sortOrder;

	void clearList();

	void setSorting(playListSorting s, playListSortingOrder o);

	QString getTitle();
	QIcon   getIcon();
	void getPagesProgress(int & loaded, int & total);
	int getLoadBytes() { return loadBytes; }

	mPlayListItem * getItem(uint index);
	int getItemIndex(mPlayListItem * item);
	uint getItemsCount();

	mPlayListItem * getNextItem();
	void setCurrentIndexItem(mPlayListItem * item);

	bool isFiltered() { return filtered; }
	void setFilter(QString text);
	QString getFilter() { return filterText; }
	
	void addItem(mPlayListItem * item);
    void addItemCopy(mPlayListItem * item);
	void removeItems(QList<mPlayListItem*> list);
	void createFromVkUser(quint64 userId, vkUserProfile info = vkUserProfile());
	void createFromVkGroup(quint64 groupId, vkGroup info = vkGroup());
	void createFromVkSearch(QString text, vkAudioSort sort, bool fixMisspell, bool exactSearch, quint32 pages);
	void createFromLastfmSimTrack(QString artist, QString title, QString mbid = "");
	void createFromLastfmUserLoved(QString user, uint limit);
	void createFromLastfmUserTop(QString user, mApiLastfmRequest::apiTopPeriod period, uint limit);
	void createFromLastfmArtistTop(QString artist, QString mbid, uint limit);

	void saveToXml(QDomElement &root, QDomDocument * doc);
	void loadFromXml(QDomElement &root);
signals:
	void stateChanged(mPlayList::playListLoadState s);
	void dataChanged();
public slots:
	void updateMetaData();
	void update();
private slots:
	void createFromVkUserLoaded(vkAudioList list);
	void createFromLastfmListLoaded(lastFmTrackList list);
	void createApiError(int code, QString msg);

	void createDownload(int bytes);

	void originUpdatedVkUser(vkUserProfile user);
	void originUpdatedVkGroup(vkGroup group);

private:
	QList<mPlayListItem*> items;
	QList<mPlayListItem*> itemsVisible;
	QList<mPlayListItem*> playHistory;
	QString		originTitle;
	QString		originText;
	qint64		originId;
	vkAudioSort originSort;
	bool		originSearchExact;
	bool		originSearchMisspel;
	int lastPlayIndex;
	bool filtered;
	QString filterText;
	int loadBytes;

	int pagedLoadingCurrent;
	int pagedLoadingCount;
};

#endif // MPLAYLIST_H
