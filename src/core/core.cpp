#include "core/core.h"
#include "core/mcacheaudio.h"
#include "core/mnetworkcookiejar.h"
#include "core/mnetowrkaccess.h"
#include "core/mapivk.h"
#include "core/mplayer.h"
#include "core/mupdater.h"
#include "core/common.h"
#include "ui/wndmain.h"
#include "ui/wndloginbrowser.h"
#include "ui/wndsessionload.h"
#include "ui/wndsendvkfirendaudio.h"
#include "ui/wndmessage.h"

#include <QNetworkProxy>
#include <QMessageBox>
#include <QApplication>
#include <QDir>
#include <QDebug>

mCore * glob_instance = NULL;

mCore::mCore(QObject *parent) :
    QObject(parent)
{
	logged			= false;
	uiSessionLoad	= NULL;
	uiWndMain		= NULL;
	repeat			= REPEAT_LIST;
	random			= RANDOM_OFF;
	itemCurrent		= NULL;

	itemHistoryPos		= -1;
	isLastfmLovedLoaded = false;

	qRegisterMetaType<mCore::statusType>("mCore::statusType");
}

mCore::~mCore()
{
    SAFE_DELETE(uiWndMain);
	cookies->cookiesSave();
	settings->sync();

	QDomDocument * dom = new QDomDocument();
	QDomElement    playListsSaveRoot = dom->createElement("playlists");
	for (int x=0; x<lists.size(); x++)
	{
		QDomElement playlist = dom->createElement("playlist");
		lists.at(x)->saveToXml(playlist, dom);
		playListsSaveRoot.appendChild(playlist);
	}
	dom->appendChild(playListsSaveRoot);
	QFile playListsFile(pathStore + "/playlists.xml");
	if (playListsFile.open(QIODevice::WriteOnly))
	{
		playListsFile.write(dom->toString().toStdString().c_str());
		playListsFile.close();
	}

	mCacheAudio::instance()->saveCache();
	mPlayer::instance()->stop();
}

mCore * mCore::instance()
{
	if (NULL == glob_instance)
		glob_instance = new mCore();
	return glob_instance;
}

void mCore::init()
{
	qDebug() << "Init starts!";

	// Init work paths
#ifdef Q_OS_WIN32
	pathStore = QDir::homePath() + "/vkmm";
#else
	pathStore = QDir::homePath() + "/.vkmm";
#endif
	QDir().mkpath(pathStore);
	pathCache = pathStore + "/cache";
	QDir().mkpath(pathCache);

	settings = new QSettings(pathStore + "/settings.ini", QSettings::IniFormat, this);

    qApp->setStyleSheet(getCss("global"));

	// Load settings
	random = (randomType)settings->value("random").toInt();
	repeat = (repeatType)settings->value("repeat").toInt();

	net = new mNetowrkAccess();
	cookies = new mNetworkCookieJar();
	cookies->cookiesLoad();
	net->setCookieJar(cookies);
	setProxy(net);

	mCacheAudio::instance()->loadCache();

	connect(mPlayer::instance(),SIGNAL(sigStreamEnded()),SLOT(streamEnded()));
	connect(mPlayer::instance(),SIGNAL(playbackStartedStream(mPlayListItem*)),SLOT(streamStarted(mPlayListItem*)));
	connect(mPlayer::instance(),SIGNAL(playbackStarted(mPlayListItem*)), SLOT(ptsStart()));
	connect(mPlayer::instance(),SIGNAL(playbackPaused(bool)), SLOT(ptsPaused()));
	connect(mPlayer::instance(),SIGNAL(playbackProgress(double,int,int)), SLOT(ptsProgress(double,int,int)));

	uiWndMain = new wndMain();
	uiWndMain->show();
	uiWndMain->setColumsWidth();

	// if settings allow this
	if (settings->value("updateAllow",true).toBool())
	{
		mUpdater * updater = new mUpdater(this);
        connect(updater,SIGNAL(needUpdate(bool,QString)),SLOT(updateNeed(bool,QString)));
		QTimer::singleShot(60000, updater, SLOT(checkForUpdates())); // Start update check in background
		// updater->checkForUpdates(); // Start update check in background
	}

	init_login_window();

	// Send signal, what we are just init...
	emit loginChanged(false);
	setRepeat(repeat);
	setRandom(random);
	setVolume(settings->value("volume", 100).toUInt());
	// qDebug() << "a3";

	// Run api record thread
	mApiLastfm::instance()->start();
	init_lastfm();
	mApiVk::instance()->start();
}

void mCore::init_login_window()
{
	wndLoginBrowser * logn_window = new wndLoginBrowser(uiWndMain);
	logn_window->show();

	// qDebug() << "a1";
	connect(logn_window,SIGNAL(authDone(QString,qint32,qint32)),SLOT(authDone(QString,qint32,qint32)));
	connect(logn_window,SIGNAL(authRejected()),SLOT(authReject()));
	// qDebug() << "a2";

	QMetaObject::invokeMethod(logn_window,"runAuth",Qt::QueuedConnection);
}

void mCore::init_lastfm()
{
	// Okay, we got settings?
	QString session   = settings->value("lastfm/sid", "").toString();
	QString userName  = settings->value("lastfm/login", "").toString();
	QString userToken = settings->value("lastfm/token", "").toString();
	if (session.isEmpty() && userToken.isEmpty())
	{
		emit lastFmSessionReject();
		return; // Nothing to init...
	}
	if (session.isEmpty())
	{
		// Try to init new session
		mApiLastfmRequest * req = new mApiLastfmRequest();
		req->sessionOpen(userName, userToken);
		connect(req,SIGNAL(sessionOpenResult(bool,QString,QString)),SLOT(lastfmSessionDone(bool,QString,QString)));
		mApiLastfm::request(req);
	} else {
		mApiLastfm::instance()->setSid(session);
		init_lastfm_data();
	}
}

void mCore::init_lastfm_session()
{
}

void mCore::init_lastfm_data()
{
	if (!mApiLastfm::instance()->isActive()) return; // Idle mode

	emit lastFmSessionOk();

	isLastfmLovedLoaded = false;

	// Load 'fav list'
	mApiLastfmRequest * req = new mApiLastfmRequest();
	req->userLoved(settings->value("lastfm/login", "").toString(), 80000);
	connect(req,SIGNAL(userLovedLoaded(lastFmTrackList)),SLOT(metaReloadLastFmLoved(lastFmTrackList)));
	mApiLastfm::request(req);
}

void mCore::authDone(QString token, qint32 expired, qint32 userId)
{
	logged = true;
	emit loginChanged(true);
	//qDebug() << token << expired << userId;
	// QMessageBox::information(uiWndMain,tr("Авторизация прошла успешно"),tr("Параметры входа:\nТокен: %1\nUser ID: %2").arg(token.left(32)+"...").arg(userId));
	if (NULL != uiSessionLoad)
		uiSessionLoad->deleteLater();

	apiVkToken = token;
	apiVkUid   = userId;

	settings->setValue("vkuid",apiVkUid);

	uiSessionLoad = new wndSessionLoad(uiWndMain);
	uiSessionLoad->show();
	connect(this,SIGNAL(loginInitDone()),uiSessionLoad,SLOT(close()));
	connect(this,SIGNAL(loginChanged(bool)),uiSessionLoad,SLOT(close()));
	connect(this,SIGNAL(statusMessageChanged(mCore::statusType,QString)),uiSessionLoad,SLOT(setText(mCore::statusType,QString)));
	connect(this,SIGNAL(sessionProgress(int)),uiSessionLoad,SLOT(setProgress(int)));

	// Start user session...
	statusMessage(STATUS_OK, tr("Загрузка профиля..."));
	mApiVkRequest * req = new mApiVkRequest(this);
	connect(req,SIGNAL(profileLoaded(vkUserProfile)),SLOT(init_profile_loaded(vkUserProfile)));
	connect(req,SIGNAL(apiError(int,QString)),SLOT(initError(int,QString)));
	req->profileLoad(apiVkUid);
	mApiVk::request(req);

	// Load playlists from XML cache
	QFile playListsXml(pathStore + "/playlists.xml");
	if (playListsXml.open(QIODevice::ReadOnly))
	{
		QDomDocument playListsXmlDom;
		if (playListsXmlDom.setContent(&playListsXml))
		{
			// Okay, lets read it!
			QDomElement playListsXmlRoot = playListsXmlDom.firstChildElement("playlists");
			QDomNodeList playListsXmlList = playListsXmlRoot.elementsByTagName("playlist");
			for (int x=0; x<playListsXmlList.size(); x++)
			{
				QDomElement playListEl = playListsXmlList.at(x).toElement();

				mPlayList * list = new mPlayList(this);
				list->loadFromXml(playListEl);
				lists.push_back(list);
				emit playListAdded(list);
			}
		}
		playListsXml.close();
	}
}

void mCore::init_profile_loaded(vkUserProfile pp)
{
	qDebug() << "User loaded!";
	user = pp;
	emit sessionProgress(10);

	// Load friends...
	statusMessage(STATUS_OK, tr("Загрузка списка друзей..."));
	mApiVkRequest * req = new mApiVkRequest(this);
	connect(req,SIGNAL(friendsLoaded(vkUserProfileList)),SLOT(init_friends_loaded(vkUserProfileList)));
	connect(req,SIGNAL(apiError(int,QString)),SLOT(initError(int,QString)));
	req->friendsLoad(apiVkUid);
	mApiVk::request(req);
}

void mCore::init_friends_loaded(vkUserProfileList lst)
{
	qDebug() << "Friends loaded!";
	userFriends = lst;
	emit sessionProgress(20);

	statusMessage(STATUS_OK, tr("Загрузка списка групп..."));
	mApiVkRequest * req = new mApiVkRequest(this);
	connect(req,SIGNAL(userGroupsLoaded(vkGroupList)),SLOT(init_groups_loaded(vkGroupList)));
	connect(req,SIGNAL(apiError(int,QString)),SLOT(initError(int,QString)));
	req->userGroupsLoad(apiVkUid);
	mApiVk::request(req);
}

void mCore::init_groups_loaded(vkGroupList lst)
{
	qDebug() << "Groups loaded!";
	userGroups = lst;
	emit sessionProgress(30);

	statusMessage(STATUS_OK, tr("Готов к работе!"));
	emit loginInitDone();
}

void mCore::initError(int code, QString msg)
{
	QMessageBox::warning(uiWndMain,tr("Ошибка API ВКонтакте"),tr("(%1) %2").arg(code).arg(msg));
	authReject();
}

void mCore::authReject()
{
	logged = false;
	apiVkToken = "";
	emit loginChanged(false);
	QMessageBox::warning(uiWndMain,tr("Ошибка авторизации"),tr("Приложение не может работать без авторизации ВКонтакте!"));
}

void mCore::lastfmSessionDone(bool res, QString user, QString sid)
{
	if (!res)
	{
		// Show error!
	} else {
		mApiLastfm::instance()->setSid(sid); // Saving new sid!
		settings->setValue("lastfm/sid", sid);

		// TODO: reload data - lists, etc
		init_lastfm_data();
	}
}

void mCore::updateNeed(bool need, QString version)
{
    if (!need)
    {
        sender()->deleteLater();
        return;
    }

    // Got update!!!
	// if (QMessageBox::No == QMessageBox::question(uiWndMain,tr("Доступно обновление!"),tr("Доступна новая версия программы: <b>%1</b>!\n Обновить сейчас?").arg(version),QMessageBox::Yes,QMessageBox::No))
	if (!wndMessage::confirm(uiWndMain,"",tr("Доступна новая версия программы: <b>%1</b>!\n Обновить сейчас?").arg(version)))
    {
        sender()->deleteLater();
        return;
    }

    // Start update!
    sender()->deleteLater();
    mUpdater * upd = new mUpdater(this);
    upd->startUpdate();
    qDebug() << "Start update window...";
}

void mCore::playItem(mPlayListItem *item, bool fromHistory)
{
	if (NULL == item) return;
	item->reference();
	SAFE_RELEASE(itemCurrent);
	itemCurrent = item;

	if (NULL != itemCurrent->playList)
		itemCurrent->playList->setCurrentIndexItem(itemCurrent);

	// Update history?
	if (!fromHistory)
	{
		// Remove all items below current index (reset forward history)
		if (-1 != itemHistoryPos)
		{
			// Delete forward...
			for (int x=0; x<itemHistoryPos; x++)
			{
				if (itemHistory.isEmpty()) break;

				itemHistory.at(0)->release();
				itemHistory.removeFirst();
			}
		}
		if (itemHistory.isEmpty() || (!itemHistory.isEmpty() && itemHistory.at(0) != itemCurrent))
		{
			// Add new item to history
			itemCurrent->reference();
			itemHistory.push_front(itemCurrent);
		}

		itemHistoryPos = 0; // We are play history-leader item
	} else {
		// Do nothing, we are play from history!
	}

	emit playbackStarted(item);
	mPlayer::instance()->playItem(item);
}

void mCore::playNext()
{
	// Check history mode
	if (!itemHistory.isEmpty() && itemHistoryPos>0)
	{
		// Play from forward-history
		itemHistoryPos--;
		playItem(itemHistory.at(itemHistoryPos), true);
		return;
	}

	// Check - current item has playlist?
	if (NULL == itemCurrent) return;
	if (NULL == itemCurrent->playList)
	{
		// No current playlist...
		// So, do nothing!
		SAFE_RELEASE(itemCurrent);
		return;
	}

	// Try to get next item
	mPlayListItem * next = itemCurrent->playList->getNextItem();
	if (NULL == next)
	{
		// Error?
		// So, do nothing!
		SAFE_RELEASE(itemCurrent);
		return;
	}
	// Play next!
	playItem(next);
}

void mCore::playPrev()
{
	// Check history mode
	if (!itemHistory.isEmpty() && -1 != itemHistoryPos)
	{
		// Play from backward-history?
		if (itemHistory.size() > itemHistoryPos+1)
		{
			itemHistoryPos++;
			playItem(itemHistory.at(itemHistoryPos), true);
			return;
		}
	}
	// History is empty...
	qDebug() << "History is not enougth!" << itemHistory.size();
	// Get 'prev' due to current random play settings
}

void mCore::setVolume(uint vol)
{
	mPlayer::instance()->setVolume((double)vol);
	emit volumeChanged(vol);
	settings->setValue("volume",vol);
}

void mCore::addCurrentToVkAudio()
{
	if (NULL == itemCurrent) return; // Do nothing
	if (!itemCurrent->vkRecord.isValid)
	{
		// Show warning
		QMessageBox::warning(uiWndMain,tr("Ошибка"),tr("Данная аудиозапись не найдена В Контакте!"));
		return;
	}

	if (!wndMessage::confirm(uiWndMain,"vk-add-self",tr("Добавить аудиозапись к себе на страницу?")))
		return;

	// Add to self...
	mApiVkRequest * req = new mApiVkRequest(this);
	req->saveAudio(itemCurrent->vkRecord.ownerId,itemCurrent->vkRecord.aid);
	connect(req,SIGNAL(audioAdded()),SLOT(vkAudioAdded()));
	mApiVk::request(req);
}

void mCore::addCurrentToVkUpdates()
{
}

void mCore::addCurrentToVkFriend()
{
    if (NULL == itemCurrent) return; // Do nothing
    wndSendVkFirendAudio * wnd = new wndSendVkFirendAudio(uiWndMain);
    wnd->addItem(itemCurrent);
	wnd->show();
}

void mCore::updateMetaData()
{
	for (int x=0; x<lists.size(); x++)
		lists.at(x)->updateMetaData();

	emit metaUpdated();
}

void mCore::metaReloadLastFmLoved(lastFmTrackList list)
{
	userLovedLastFmMbid.clear();
	userLovedLastFmTitles.clear();

	isLastfmLovedLoaded = true;

	for (int x=0; x<list.size(); x++)
	{
		if (!list.at(x).mbid.isEmpty()) userLovedLastFmMbid.push_back(list.at(x).mbid);
		userLovedLastFmTitles.push_back(qMakePair<QString,QString>(list.at(x).artist,list.at(x).title));
	}
	updateMetaData();

	statusMessage(STATUS_LASTFM, tr("Получен список любимых композиций (%1)").arg(list.size()));
}

void mCore::lovedToggle(mPlayListItem *itm, bool isLoved)
{
	if (NULL == itm) return;

	mApiLastfmRequest * req = new mApiLastfmRequest();
	req->trackLove(itm->getAudioArtist(), itm->getAudioTitle(), isLoved);
	mApiLastfm::request(req);

	if (!isLoved)
	{
		// Remove from loved
		itm->isLovedLastFm = false;
		userLovedLastFmTitles.removeOne(qMakePair<QString,QString>(itm->getAudioArtist(),itm->getAudioTitle()));
		if (!itm->lastfmTrack.mbid.isEmpty())
			userLovedLastFmMbid.removeOne(itm->lastfmTrack.mbid);
	} else {
		itm->isLovedLastFm = true;
		if (!userLovedLastFmTitles.contains(qMakePair<QString,QString>(itm->getAudioArtist(),itm->getAudioTitle())))
		{
			userLovedLastFmTitles.push_back(qMakePair<QString,QString>(itm->getAudioArtist(),itm->getAudioTitle()));
		}
		if (!itm->lastfmTrack.mbid.isEmpty())
		{
			if (!userLovedLastFmMbid.contains(itm->lastfmTrack.mbid))
				userLovedLastFmMbid.push_back(itm->lastfmTrack.mbid);
		}
	}
	updateMetaData();
}

void mCore::streamEnded()
{
	// Repeat item?
	if (REPEAT_TRACK == repeat)
	{
		if (NULL != itemCurrent)
		{
			playItem(itemCurrent,true);
			return;
		}
	}
	// Play next file...
	playNext();
}

void mCore::streamStarted(mPlayListItem *item)
{
	// Send to Vk?
	if (item->vkRecord.isValid)
	{
		if (settings->value("vk/sendStatus",true).toBool())
		{
			mApiVkRequest * req = new mApiVkRequest(this);
			req->sendStatusAudio(item->vkRecord.ownerId, item->vkRecord.aid);
			mApiVk::request(req);
		}
	}
	if (settings->value("vk/sendOnline",true).toBool())
	{
		// Send online request
		mApiVkRequest * req = new mApiVkRequest(this);
		req->sendStatusOnline();
		mApiVk::request(req);
	}
	if (mApiLastfm::instance()->isActive())
	{
		// Send 'Now playing'
		mApiLastfmRequest * req = new mApiLastfmRequest();
		req->trackNowPlaying(item->getAudioArtist(), item->getAudioTitle());
		mApiLastfm::request(req);
	}
}

void mCore::vkAudioAdded()
{
	statusMessageAudioAdded();
}

void mCore::vkAudioPosted()
{
}

void mCore::lastfmResultScrobbed(QString artist, QString title)
{
	statusMessage(STATUS_LASTFM, tr("Заскроблено: <b>%1</b>").arg(title));
	emit lastfmTrackScrobbed(artist, title);
}

void mCore::ptsStart()
{
	ptsTime = 0;
	ptsTimer.restart();
	ptsTimerStatus.restart();
}

void mCore::ptsProgress(double , int , int totalTime)
{
	if (mPlayer::instance()->isPaused())
	{
		return; // Do not count pauses
	}

	// Need update 'Playing' status to lastfm?
	if (ptsTimerStatus.elapsed() > 30000)
	{
		if (mApiLastfm::instance()->isActive() && settings->value("lastfm/scrobbingEnable",true).toBool())
		{
			// Send track to be listened!
			mPlayListItem * item = mPlayer::instance()->getCurrentItem();
			if (NULL != item)
			{
				mApiLastfmRequest * req = new mApiLastfmRequest();
				req->trackNowPlaying(item->getAudioArtist(), item->getAudioTitle());
				mApiLastfm::request(req);
			}
		}
		ptsTimerStatus.restart();
	}

	if (-1 == ptsTime) return; // Already sended signal for this playback

	int elapsed = ptsTimer.elapsed();
	ptsTimer.restart(); // Start new period-count

	ptsTime += elapsed;

	// Calc persent
	int pers = (int)((((double)ptsTime/1000.0)/(double)totalTime) * 100.0);
	if (pers > settings->value("lastfm/scrobbingSize").toInt())
	{
		// Send signal!
		emit playedPers(pers);

		if (mApiLastfm::instance()->isActive() && settings->value("lastfm/scrobbingEnable",true).toBool())
		{
			// Send track to be scrobbed!
			mPlayListItem * item = mPlayer::instance()->getCurrentItem();
			if (NULL != item)
			{
				mApiLastfmRequest * req = new mApiLastfmRequest();
				req->trackScrobble(item->getAudioArtist(), item->getAudioTitle(), item->vkRecord.duration);
				connect(req, SIGNAL(trackScrobbed(QString,QString)),SLOT(lastfmResultScrobbed(QString,QString)));
				mApiLastfm::request(req);
			}
		}

		ptsTime = -1; // Send only once
	}
}

void mCore::ptsPaused()
{
	ptsTimer.restart(); // Start new period-count
}

void mCore::logOut()
{
	logged = false;
	apiVkToken = "";
	cookies->cookiesClean();
	init_login_window();
}

void mCore::reloadSettings()
{
	setProxy(net);
	mCacheAudio::instance()->syncFiles();
	mCacheAudio::instance()->syncCache();
}

void mCore::setRepeat(mCore::repeatType t)
{
	repeat = t;
	emit repeatChanged(t);
	settings->setValue("repeat",(int)t);
}

void mCore::setRandom(mCore::randomType t)
{
	random = t;
	emit randomChanged(t);
	settings->setValue("random",(int)t);
}

vkUserProfile mCore::userFriendById(quint64 id)
{
	for (int x=0; x<userFriends.size(); x++)
	{
		if (userFriends.at(x).id == id)
			return userFriends.at(x);
	}
	return vkUserProfile();
}

void mCore::createPlaylistFromVkUser(quint64 userId, vkUserProfile info)
{
	mPlayList * list = new mPlayList(this);
	if (0 == info.id)
	{
		// Try to find in friends...
		vkUserProfile friendInfo = userFriendById(userId);
		if (0 != friendInfo.id)
			info = friendInfo;
	}
	list->createFromVkUser(userId, info);
	lists.push_back(list);
	emit playListAdded(list);
}

void mCore::createPlaylistFromVkGroup(quint64 groupId, vkGroup info)
{
	mPlayList * list = new mPlayList(this);
	list->createFromVkGroup(groupId, info);
	lists.push_back(list);
	emit playListAdded(list);
}

void mCore::createPlaylistFromVkSearch(QString text, vkAudioSort sort, bool fixMisspell, bool exactSearch, quint32 pages)
{
	mPlayList * list = new mPlayList(this);
	list->createFromVkSearch(text, sort, fixMisspell, exactSearch, pages);
	lists.push_back(list);
	emit playListAdded(list);
}

void mCore::createPlayListFromLastfmSimTrack(QString artist, QString title)
{
	if (!mApiLastfm::instance()->isActive()) return;
	mPlayList * list = new mPlayList(this);
	list->createFromLastfmSimTrack(artist, title);
	lists.push_back(list);
	emit playListAdded(list);
}

void mCore::createPlayListFromLastfmUserRecomendation(QString user, uint limit)
{

}

void mCore::createPlayListFromLastfmUserLoved(QString user, uint limit)
{
	mPlayList * list = new mPlayList(this);
	list->createFromLastfmUserLoved(user, limit);
	lists.push_back(list);
	emit playListAdded(list);
}

void mCore::createPlayListFromLastfmUserTop(QString user, mApiLastfmRequest::apiTopPeriod period, uint limit)
{
	mPlayList * list = new mPlayList(this);
	list->createFromLastfmUserTop(user, period, limit);
	lists.push_back(list);
	emit playListAdded(list);
}

void mCore::createPlayListFromLastfmArtistTop(QString artist, QString artistMbid, uint limit)
{
	mPlayList * list = new mPlayList(this);
	list->createFromLastfmArtistTop(artist, artistMbid, limit);
	lists.push_back(list);
	emit playListAdded(list);
}

void mCore::removePlayList(uint index)
{
    if ((int)index >= lists.size()) return;
	mPlayList * list = lists.at(index);
	list->clearList();
	list->deleteLater();
	lists.removeAt(index);
	emit playListRemoved(index);
}

bool mCore::isPrevAvaliable()
{
	if (!itemHistory.isEmpty() && -1 != itemHistoryPos)
	{
		// Play from backward-history?
		if (itemHistory.size() > itemHistoryPos+1)
		{
			return true;
		}
	}
	return false;
}

void mCore::sendAudioStatus(vkAudio track)
{
	mApiVkRequest * req = new mApiVkRequest(this);
	req->sendStatusAudio(track.ownerId, track.aid);
	mApiVk::request(req);
}

void mCore::setPlayListFilter(uint index, QString text)
{
    if ((int)index >= lists.size()) return;
	lists.at(index)->setFilter(text);
}

QString mCore::getPlayListFilter(uint index)
{
	if (index >= lists.size()) return "";
	return lists.at(index)->getFilter();
}

vkUserProfile mCore::getLastSelectedVkUser()
{
	quint64 lastId = settings->value("vk/lastSelectedUser", 0).toULongLong();
	if (0 == lastId) return vkUserProfile();
	// Current user?
	if (lastId == user.id) return user;
	// Search in friends by-id
	return userFriendById(lastId);
}

void mCore::setLastSelectedVkUser(vkUserProfile user)
{
	settings->setValue("vk/lastSelectedUser", user.id);
}

bool mCore::isItemLastfmLoved(QString artist, QString title)
{
	return userLovedLastFmTitles.contains(qMakePair<QString,QString>(artist,title));
}

bool mCore::isItemLastfmLoved(QString mbId)
{
	if (mbId.isEmpty()) return false;
	return userLovedLastFmMbid.contains(mbId);
}

void mCore::statusMessage(statusType type, QString msg)
{
	emit statusMessageChanged(type, msg);
}

void mCore::statusMessageAudioAdded()
{
	QString text = tr("Аудиозапись добавлена!");
	statusMessage(STATUS_VK, text);
}

void mCore::statusMessageAudioPosted(qint64 uid, quint64 postId)
{
	QString text = tr("Запись добавлена!");
	if (uid && postId)
		text += tr(" <a href='http://vk.com/wall%1_%2'>Просмотреть</a>").arg(uid).arg(postId);
	statusMessage(STATUS_VK, text);
}

void mCore::qsaMessage(QString msg)
{
	if ("show" == msg)
	{
		if (NULL != uiWndMain)
			uiWndMain->showMe();
	}
}
