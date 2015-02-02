#include "mplaylist.h"
#include "core/core.h"
#include "core/common.h"
#include "core/mcacheaudio.h"

#include <QDebug>
#include <QTime>

mPlayListItem::mPlayListItem(QObject *parent) :
	mRefCounter(parent)
{
	playList		= NULL;
	isPlaying		= false;
	isSearched		= false;
	isError  		= false;
	isInUserVk		= false;
	isLovedLastFm	= false;
}

mPlayListItem::~mPlayListItem()
{
}

void mPlayListItem::setVkRecord(vkAudio rec)
{
	vkRecord = rec;
}

QString & mPlayListItem::getAudioArtist()
{
	if (!lastfmTrack.artist.isEmpty())
	{
		return lastfmTrack.artist;
	}
	if (vkRecord.isValid)
	{
		return vkRecord.artist;
	}
	return vkRecord.artist;
}

QString & mPlayListItem::getAudioTitle()
{
	if (!lastfmTrack.title.isEmpty())
	{
		return lastfmTrack.title;
	}
	if (vkRecord.isValid)
	{
		return vkRecord.title;
	}
	return vkRecord.title;
}

int mPlayListItem::getAudioLength()
{
	if (vkRecord.isValid)
	{
		return vkRecord.duration;
	}
	return 0;
}

QString mPlayListItem::getTimeFormatted()
{
	if (vkRecord.isValid)
	{
		QTime t(0,0,0);
		t = t.addSecs(vkRecord.duration);
		return t.toString("mm:ss");
	}
	return "-";
}

mPlayListItem *mPlayListItem::copy()
{
    mPlayListItem * out = new mPlayListItem();
    out->vkRecord = vkRecord;
	return out;
}

void mPlayListItem::toDomElement(QDomElement &in, QDomDocument *doc)
{
	if (vkRecord.isValid)
	{
		QDomElement vk = doc->createElement("vk");
		vkRecord.toVkRecord(vk);
		in.appendChild(vk);
	}
	if (!lastfmTrack.artist.isEmpty())
	{
		QDomElement lastfm = doc->createElement("lastfm");
		lastfmTrack.toXml(lastfm);
		in.appendChild(lastfm);
	}
}

void mPlayListItem::fromDomElement(QDomElement &in)
{
	QDomElement vkRecordDom = in.firstChildElement("vk");
	if (!vkRecordDom.isNull())
	{
		vkRecord.fromVkRecord(vkRecordDom);
	}
	QDomElement vkRecordLastFm = in.firstChildElement("lastfm");
	if (!vkRecordLastFm.isNull())
	{
		lastfmTrack.fromXml(vkRecordLastFm);
	}
}

bool mPlayListItem::isCached()
{
	if (!vkRecord.isValid) return false;
	// Check cache
	return (NULL != mCacheAudio::instance()->searchFile(vkRecord.aid));
}

void mPlayListItem::updateMetaData()
{
	// Okay, lets see - what we are...
	if (!lastfmTrack.mbid.isEmpty())
	{
		isLovedLastFm = mCore::instance()->isItemLastfmLoved(lastfmTrack.mbid);
	} else if (!lastfmTrack.artist.isEmpty() && !lastfmTrack.title.isEmpty()) {
		isLovedLastFm = mCore::instance()->isItemLastfmLoved(lastfmTrack.artist, lastfmTrack.title);
	} else if (vkRecord.isValid) {
		isLovedLastFm = mCore::instance()->isItemLastfmLoved(vkRecord.artist, vkRecord.title);
	}
}

bool mPlayListItem::selectBestVkSearched(vkAudioList &list)
{
	QString checkArtist = lastfmTrack.artist.trimmed().toLower();
	QString checkTitle  = lastfmTrack.title.trimmed().toLower();
	int matchMinimal = -1;
	int matchMinimalIndex = -1;
	for (int x=0; x<list.size(); x++)
	{
		int lvArtist = levenshteinDistance(checkArtist, list.at(x).artist.trimmed().toLower());
		int lvTitle  = levenshteinDistance(checkTitle, list.at(x).title.trimmed().toLower());
		if (0 == lvArtist && 0 == lvTitle)
		{
			// 100% match
			matchMinimalIndex = x; break;
		}
		//continue;
		int lvSumm = lvArtist + lvTitle;
		if (-1 == matchMinimal || (lvSumm < matchMinimal))
		{
			matchMinimalIndex = x;
		}
	}
	if (-1 == matchMinimalIndex) return false;
	vkRecord = list.at(matchMinimalIndex);
	return true;
}

mPlayList::mPlayList(QObject *parent) :
	QObject(parent)
{
	loadState		= LS_IDLE;
	origin			= ORIGIN_UNKNOWN;
	originSearchExact	= false;
	originSearchMisspel = false;
	lastPlayIndex	= -1;
	filtered		= false;
	sort			= SORT_ORIGINAL;
	sortOrder		= ORDER_ASC;
	loadBytes		= -1;
}

mPlayList::~mPlayList()
{
	clearList();
}

void mPlayList::clearList()
{
	for (int x=0; x<items.size(); x++)
	{
		items.at(x)->playList = NULL;
		items.at(x)->release();
	}
	items.clear();
	itemsVisible.clear();
}

void mPlayList::setSorting(mPlayList::playListSorting s, mPlayList::playListSortingOrder o)
{
	sort		= s;
	sortOrder	= o;

	while (true)
	{
		bool found = false;

		for (int x=1; x<items.size(); x++)
		{
			bool needSort = false;

			if (SORT_ORIGINAL == s)
			{
				int s1 = items.at(x-1)->indexOriginal;
				int s2 = items.at(x  )->indexOriginal;
				needSort = ( (ORDER_ASC == o && s1 > s2) || (ORDER_DESC == o && s1 < s2) );
			}
			if (SORT_NAME == s)
			{
				QString s1a = items.at(x-1)->getAudioArtist();
				QString s1b = items.at(x-1)->getAudioTitle();
				QString s2a = items.at(x  )->getAudioArtist();
				QString s2b = items.at(x  )->getAudioTitle();

				if (s1a == s2a)
				{
					// Check title
					if (s1b != s2b)
					{
						needSort = ( (ORDER_ASC == o && s1b > s2b) || (ORDER_DESC == o && s1b < s2b) );
					}
				} else {
					// Check artist
					needSort = ( (ORDER_ASC == o && s1a > s2a) || (ORDER_DESC == o && s1a < s2a) );
				}
			}
			if (SORT_LENGTH == s)
			{
				int s1 = items.at(x-1)->getAudioLength();
				int s2 = items.at(x  )->getAudioLength();
				needSort = ( (ORDER_ASC == o && s1 > s2) || (ORDER_DESC == o && s1 < s2) );
			}

			if ( needSort )
			{
				items.swap(x-1,x);
				found = true;
			}
		}

		if (!found) break;
	}

	itemsVisible.clear();
	if (!filterText.isEmpty())
	{
		setFilter(filterText);
	}
	emit stateChanged(loadState);
}

QString mPlayList::getTitle()
{
	if (!originTitle.isEmpty())
		return originTitle;
	return tr("Список");
}

QIcon mPlayList::getIcon()
{
	switch (origin)
	{
		case FROM_VK_GROUP:
		case FROM_VK_SEARCH:
		case FROM_VK_USER:
			return QIcon(":/icons/vk.png");
			break;
		case FROM_LASTFM_SIM:
		case FROM_LASTFM_USER:
		case FROM_LASTFM_ARTIST:
			return QIcon(":/icons/lastfm.png");
			break;
		default:
			break;
	}
	return QIcon(":/icons/music.png");
}

void mPlayList::getPagesProgress(int &loaded, int &total)
{
	loaded = pagedLoadingCurrent;
	total  = pagedLoadingCount;
}

mPlayListItem *mPlayList::getItem(uint index)
{
	if (!isFiltered())
	{
        if (items.count() <= (int)index) return NULL;
		return items.at(index);
	} else {
        if (itemsVisible.count() <= (int)index) return NULL;
		return itemsVisible.at(index);
	}
}

int mPlayList::getItemIndex(mPlayListItem *item)
{
	QList<mPlayListItem*> * currentList;
	if (!isFiltered())
		currentList = &items;
	else
		currentList = &itemsVisible;

	if (0 == currentList->size()) return -1; // Empty
	return currentList->indexOf(item);
}

uint mPlayList::getItemsCount()
{
	if (!isFiltered()) return items.size(); else return itemsVisible.size();
}

mPlayListItem *mPlayList::getNextItem()
{
	QList<mPlayListItem*> * currentList;
	if (!isFiltered())
		currentList = &items;
	else
		currentList = &itemsVisible;

	if (0 == currentList->size()) return NULL; // Empty
	if (mCore::RANDOM_ON == mCore::instance()->random)
	{
		uint nextIndex = qrand()%currentList->size();
		return getItem(nextIndex);
	}
	if (-1 == lastPlayIndex)
	{
		return getItem(0);
	}
	if (mCore::REPEAT_LIST == mCore::instance()->repeat)
	{
		int nextIndex = lastPlayIndex+1;
		if (nextIndex >= currentList->size())
			nextIndex = 0;
		return getItem(nextIndex);
	}
	return NULL;
}

void mPlayList::setCurrentIndexItem(mPlayListItem *item)
{
	QList<mPlayListItem*> * currentList;
	if (!isFiltered())
		currentList = &items;
	else
		currentList = &itemsVisible;

	int newIndex = currentList->indexOf(item);
	if (-1 == newIndex) return;
	lastPlayIndex = newIndex;
}

void mPlayList::setFilter(QString text)
{
	text = text.trimmed();
	if (!text.length())
	{
		itemsVisible.clear();
		filtered = false;
		filterText = "";
	} else {
		// Filter items!
		itemsVisible.clear();
		filtered = true;
		filterText = text;
		QString origTitle;
		QStringList searchWords = text.split(QRegExp("\\s+"),QString::SkipEmptyParts);
		for (int x=0; x<items.count(); x++)
		{
			origTitle = items.at(x)->getAudioArtist() + " " + items.at(x)->getAudioTitle();
			bool match = true;
			for (int z=0; z<searchWords.size(); z++)
			{
				if (!origTitle.contains(searchWords.at(z),Qt::CaseInsensitive))
				{
					match = false;
					break;
				}
			}
			if (match)
			{
				itemsVisible.push_back(items.at(x));
			}
			if (items.at(x)->isPlaying)
			{
				if (match)
					lastPlayIndex = itemsVisible.size() - 1;
				else
					lastPlayIndex = -1;
			}
		}
	}
	emit stateChanged(loadState);
}

void mPlayList::addItem(mPlayListItem *item)
{
	if (NULL == item) return;
	item->reference();
	item->playList = this;
	item->indexOriginal = items.size();
	item->updateMetaData();
    items.append(item);
}

void mPlayList::addItemCopy(mPlayListItem *item)
{
    if (NULL == item) return;
    mPlayListItem * added = item->copy();
	addItem(added);
}

void mPlayList::removeItems(QList<mPlayListItem *> list)
{
	for (int x=0; x<list.size(); x++)
	{
		int itemIndex = items.indexOf(list.at(x));
		if (-1 == itemIndex) continue; // Not found
		items.at(itemIndex)->release();
		items.removeAt(itemIndex);
	}
	itemsVisible.clear();
	if (!filterText.isEmpty())
	{
		setFilter(filterText);
	}
	emit stateChanged(loadState);
}

void mPlayList::createFromVkUser(quint64 userId, vkUserProfile info)
{
	loadState = LS_LOADING;
	origin = FROM_VK_USER;

	if (!userId || userId == mCore::instance()->user.id)
	{
		originTitle = "Моё Аудио";
	} else {
		originTitle = QString("VK:") + QString::number(userId);
		mApiVkRequest * reqInfo = new mApiVkRequest(this);
		reqInfo->profileLoad(userId);
		connect(reqInfo,SIGNAL(profileLoaded(vkUserProfile)),SLOT(originUpdatedVkUser(vkUserProfile)));
		mApiVk::request(reqInfo);
	}
	emit stateChanged(loadState);

	clearList();

	mApiVkRequest * req = new mApiVkRequest(this);
	req->userAudioLoad(userId);
	connect(req,SIGNAL(userAudioLoaded(vkAudioList)),SLOT(createFromVkUserLoaded(vkAudioList)));
	connect(req,SIGNAL(apiError(int,QString)),SLOT(createApiError(int,QString)));
	mApiVk::request(req);
}

void mPlayList::createFromVkGroup(quint64 groupId, vkGroup)
{
	loadState = LS_LOADING;
	origin = FROM_VK_GROUP;
	originTitle = QString("GROUP:") + QString::number(groupId);
	emit stateChanged(loadState);

	clearList();

	mApiVkRequest * reqInfo = new mApiVkRequest(this);
	reqInfo->groupInfoLoad(groupId);
	connect(reqInfo,SIGNAL(groupinfoLoaded(vkGroup)),SLOT(originUpdatedVkGroup(vkGroup)));
	mApiVk::request(reqInfo);

	mApiVkRequest * req = new mApiVkRequest(this);
	req->groupAudioLoad(groupId);
	connect(req,SIGNAL(userAudioLoaded(vkAudioList)),SLOT(createFromVkUserLoaded(vkAudioList)));
	connect(req,SIGNAL(apiError(int,QString)),SLOT(createApiError(int,QString)));
	mApiVk::request(req);
}

void mPlayList::createFromVkSearch(QString text, vkAudioSort sort, bool fixMisspell, bool exactSearch, quint32 pages)
{
	loadState = LS_LOADING;
	origin = FROM_VK_SEARCH;
	originTitle = text;

	clearList();

	// Calc how many requests we need...
	pagedLoadingCurrent = 0;
	pagedLoadingCount	= pages;
	originSearchExact	= exactSearch;
	originSearchMisspel	= fixMisspell;
	originText = text;
	originSort = sort;

	mApiVkRequest * req = new mApiVkRequest(this);
	req->searchAudio(text, sort, fixMisspell, 0, vkSearchPageSize);
	connect(req,SIGNAL(searchAudioLoaded(vkAudioList)),SLOT(createFromVkUserLoaded(vkAudioList)));
	connect(req,SIGNAL(apiError(int,QString)),SLOT(createApiError(int,QString)));
	mApiVk::request(req);

	emit stateChanged(loadState);
}

void mPlayList::createFromLastfmSimTrack(QString artist, QString title, QString mbid)
{
	loadState	= LS_LOADING;
	origin		= FROM_LASTFM_SIM;
	originTitle = tr("Как: %1").arg(title);

	clearList();

	mApiLastfmRequest * req = new mApiLastfmRequest(this);
	req->similarTrack(artist, title, mbid);
	connect(req, SIGNAL(tracksListLoaded(lastFmTrackList)), SLOT(createFromLastfmListLoaded(lastFmTrackList)));
	connect(req, SIGNAL(apiError(int,QString)), SLOT(createApiError(int,QString)));
	connect(req, SIGNAL(loadProgress(int)), SLOT(createDownload(int)));
	mApiLastfm::request(req);

	emit stateChanged(loadState);
}

void mPlayList::createFromLastfmUserLoved(QString user, uint limit)
{
	loadState	= LS_LOADING;
	origin		= FROM_LASTFM_USER;
	originTitle = tr("Любимое %1").arg(user);

	clearList();

	mApiLastfmRequest * req = new mApiLastfmRequest(this);
	req->userLoved(user, limit);
	connect(req, SIGNAL(tracksListLoaded(lastFmTrackList)), SLOT(createFromLastfmListLoaded(lastFmTrackList)));
	connect(req, SIGNAL(apiError(int,QString)), SLOT(createApiError(int,QString)));
	connect(req, SIGNAL(loadProgress(int)), SLOT(createDownload(int)));
	mApiLastfm::request(req);

	emit stateChanged(loadState);
}

void mPlayList::createFromLastfmUserTop(QString user, mApiLastfmRequest::apiTopPeriod period, uint limit)
{
	loadState	= LS_LOADING;
	origin		= FROM_LASTFM_USER;
	originTitle = tr("Топ %1").arg(user);

	clearList();

	mApiLastfmRequest * req = new mApiLastfmRequest(this);
	req->userTracks(user, period, limit);
	connect(req, SIGNAL(tracksListLoaded(lastFmTrackList)), SLOT(createFromLastfmListLoaded(lastFmTrackList)));
	connect(req, SIGNAL(apiError(int,QString)), SLOT(createApiError(int,QString)));
	connect(req, SIGNAL(loadProgress(int)), SLOT(createDownload(int)));
	mApiLastfm::request(req);

	emit stateChanged(loadState);
}

void mPlayList::createFromLastfmArtistTop(QString artist, QString mbid, uint limit)
{
	loadState	= LS_LOADING;
	origin		= FROM_LASTFM_ARTIST;
	originTitle = artist;

	clearList();

	mApiLastfmRequest * req = new mApiLastfmRequest(this);
	req->artistTracks(artist, mbid, limit);
	connect(req, SIGNAL(tracksListLoaded(lastFmTrackList)), SLOT(createFromLastfmListLoaded(lastFmTrackList)));
	connect(req, SIGNAL(apiError(int,QString)), SLOT(createApiError(int,QString)));
	connect(req, SIGNAL(loadProgress(int)), SLOT(createDownload(int)));
	mApiLastfm::request(req);

	emit stateChanged(loadState);
}

void mPlayList::saveToXml(QDomElement & root, QDomDocument *doc)
{
	root.setAttribute("vkmm_version",QString(getVersion()));
	root.setAttribute("time",QDateTime::currentDateTime().toTime_t());
	root.setAttribute("origin_type",(int)origin);
	root.setAttribute("origin_id",originId);
	root.setAttribute("origin_title",originTitle);

	QDomElement list = doc->createElement("list");

	for (int x=0; x<items.size(); x++)
	{
		QDomElement itemDom = doc->createElement("item");
		items.at(x)->toDomElement(itemDom, doc);
		list.appendChild(itemDom);
	}

	root.appendChild(list);
}

void mPlayList::loadFromXml(QDomElement &root)
{
	loadState	= LS_DONE;
	origin		= (mPlayList::playListOrigin)root.attribute("origin_type","0").toInt();
	originId	= root.attribute("origin_id","0").toLongLong();
	originTitle	= root.attribute("origin_title","");

	// Load contents data...
	QDomElement lst = root.firstChildElement("list");
	QDomNodeList itemsListXml = lst.elementsByTagName("item");
	for (int x=0; x<itemsListXml.size(); x++)
	{
		QDomElement item = itemsListXml.at(x).toElement();
		mPlayListItem * inItem = new mPlayListItem();
		inItem->fromDomElement(item);
		addItem(inItem);
		inItem->release();
	}
	emit stateChanged(loadState);
}

void mPlayList::createFromVkUserLoaded(vkAudioList list)
{
	QStringList searchWords;

	if (FROM_VK_SEARCH == origin && originSearchExact)
	{
		searchWords = originText.split(QRegExp("\\s+"),QString::SkipEmptyParts);
	}

	for (int x=0; x<list.size(); x++)
	{
		if (FROM_VK_SEARCH == origin && originSearchExact)
		{
			QStringList origTitle = QString(list.at(x).artist + " - " + list.at(x).title).split(QRegExp("\\s+"),QString::SkipEmptyParts);
			bool match = true;
			for (int z=0; z<searchWords.size(); z++)
			{
				if (!origTitle.contains(searchWords.at(z),Qt::CaseInsensitive))
				{
					match = false;
					break;
				}
			}
			if (!match)
			{
				continue;
			}
		}
		mPlayListItem * item = new mPlayListItem();
		item->setVkRecord(list.at(x));
		addItem(item);
		item->release();
	}

	// Group loading? Calc next page
	if (FROM_VK_SEARCH == origin)
	{
		pagedLoadingCurrent++;
		if (pagedLoadingCurrent < pagedLoadingCount)
		{
			// Load next page...
			mApiVkRequest * req = new mApiVkRequest(this);
			req->searchAudio(originText, originSort, originSearchMisspel, vkSearchPageSize*pagedLoadingCurrent, vkSearchPageSize);
			connect(req,SIGNAL(searchAudioLoaded(vkAudioList)),SLOT(createFromVkUserLoaded(vkAudioList)));
			connect(req,SIGNAL(apiError(int,QString)),SLOT(createApiError(int,QString)));
			mApiVk::request(req);
			loadState = LS_LOADING;
			emit stateChanged(loadState);
			return;
		}
	}
	loadState = LS_DONE;
	emit stateChanged(loadState);
}

void mPlayList::createFromLastfmListLoaded(lastFmTrackList list)
{
	for (int x=0; x<list.size(); x++)
	{
		mPlayListItem * item = new mPlayListItem();
		item->lastfmTrack = list.at(x);
		addItem(item);
		item->release();
	}

	loadState = LS_DONE;
	emit stateChanged(loadState);
}

void mPlayList::createApiError(int code, QString msg)
{
	Q_UNUSED(code);
	Q_UNUSED(msg);
	loadState = LS_ERROR;
	emit stateChanged(loadState);
}

void mPlayList::createDownload(int bytes)
{
	loadBytes = bytes;
	update();
}

void mPlayList::originUpdatedVkUser(vkUserProfile user)
{
	originId	= user.id;
	originTitle = user.getName();
	emit stateChanged(loadState);
}

void mPlayList::originUpdatedVkGroup(vkGroup group)
{
	originId	= group.gid;
	originTitle = clearGroupTitle(group.title);
	emit stateChanged(loadState);
}

void mPlayList::updateMetaData()
{
	for (int x=0; x<items.size(); x++)
	{
		items.at(x)->updateMetaData();
	}
	update();
}

void mPlayList::update()
{
	emit dataChanged();
}
