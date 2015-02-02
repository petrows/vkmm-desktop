#include "mplayer.h"

#include <QDebug>
#include <QObject>
#include <QMessageBox>
#include <QUrl>
#include <QFileInfo>

#include <math.h>
#include <stdio.h>
#include "core/core.h"
#include "core/mnetowrkaccess.h"
#include "core/mapivk.h"

mPlayer::mPlayer(QObject *parent) :
	QObject(parent)
{
	currentItem = NULL;
	currentTask = NULL;
	volume = 100;

	qRegisterMetaType<HSTREAM>("HSTREAM");
	qRegisterMetaType<mStream*>("mStream*");

	if (HIWORD(BASS_GetVersion())!=BASSVERSION)
	{
		QMessageBox::warning(0,"BASS error","An incorrect version of BASS lib was loaded");
		return;
	}

	if (!BASS_Init(1,44100,0,NULL,NULL))
	{
		QMessageBox::warning(0,"BASS error",tr("Enable to init device! Error %1").arg(BASS_ErrorGetCode()));
		return;
	}

	//connect(&updTimer,SIGNAL(timeout()),SLOT(onUpdateTimer()));
	updTimer.setSingleShot(false);
	updTimer.setInterval(100);
	playbackId = 0;
}

mPlayer::~mPlayer()
{
	if (NULL != currentTask)
	{
		currentTask->stop();
		currentTask->wait();
		delete currentTask;
	}
}

mPlayer * mPLayer_glob_instance = NULL;
mPlayer *mPlayer::instance()
{
	if (NULL == mPLayer_glob_instance)
		mPLayer_glob_instance = new mPlayer();
	return mPLayer_glob_instance;
}

void mPlayer::playItem(mPlayListItem *item)
{
	static int idCounter = 0;

	item->reference();

	stop();
	checkDeletedTasks();

	idCounter++;
	playbackId = idCounter;
	currentItem = item;

	// Ok, need to be searched?
	if (!currentItem->vkRecord.isValid && !currentItem->lastfmTrack.artist.isEmpty())
	{
		// Search it!
		mApiVkRequest * req = new mApiVkRequest(this);
		req->searchAudio(currentItem->lastfmTrack.artist + " - " + currentItem->lastfmTrack.title, VK_AUDIO_SORT_POPULAR, false);
		req->setProperty("task-id", playbackId);
		connect(req, SIGNAL(searchAudioLoaded(vkAudioList)), SLOT(onVkAudioSearch(vkAudioList)));
		mApiVk::request(req);
		currentItem->isSearched = true;
		emit playbackStarted(currentItem);
		if (NULL != currentItem->playList) currentItem->playList->update();
		mCore::instance()->statusMessage(mCore::STATUS_VK, tr("Поиск аудиозаписи..."));
		return;
	}
	currentItem->isPlaying = true;

	currentTask = new mStream(playbackId, currentItem->vkRecord.url, currentItem->vkRecord.aid);
	currentTask->volume(volume);
	currentTask->start();

	connect(currentTask,SIGNAL(streamEnded(uint)),this,SLOT(onTaskStreamEnd(uint)), Qt::QueuedConnection);
	connect(currentTask,SIGNAL(streamPlaybackStatus(uint,int)),this,SLOT(onTaskStreamPlayProgress(uint,int)), Qt::QueuedConnection);
	connect(currentTask,SIGNAL(streamDownloadStatus(uint,qint64,qint64,int)),this,SLOT(onTaskStreamDownloadProgress(uint,qint64,qint64,int)), Qt::QueuedConnection);
	connect(currentTask,SIGNAL(streamPaused(uint,bool)),this,SLOT(onTaskStreamPaused(uint,bool)), Qt::QueuedConnection);
	connect(currentTask,SIGNAL(streamStopped(mStream*)),this,SLOT(onTaskStreamStopped(mStream*)), Qt::QueuedConnection);
	connect(currentTask,SIGNAL(streamDownloadError(uint)),this,SLOT(onTaskStreamDownloadError(uint)), Qt::QueuedConnection);

	qDebug() << "Started new task";
	emit playbackStarted(currentItem);
	emit playbackProgress(0,0,currentItem->vkRecord.duration);
	emit downloadProgress(0,0,1,0);
	emit playbackStartedStream(currentItem);

	if (NULL != currentItem->playList) currentItem->playList->update();
}

void mPlayer::stop()
{
	if (NULL != currentItem)
	{
		currentItem->isPlaying	= false;
		currentItem->isSearched = false;
	}
	if (NULL != currentTask)
	{
		QMetaObject::invokeMethod(currentTask,"stop",Qt::QueuedConnection);
		currentTask = NULL; // Will be self-destroy
	}
	SAFE_RELEASE(currentItem);
}

void mPlayer::pause()
{
	if (NULL != currentTask)
	{
		QMetaObject::invokeMethod(currentTask,"pause",Qt::QueuedConnection);
	}
}

void mPlayer::onTaskStreamEnd(uint id)
{
	qDebug() << "Ended stream # " << id;
	if (id != playbackId) return; // Ignore not our-s signal
	emit sigStreamEnded();
}

void mPlayer::onTaskStreamCreated(uint id)
{
	if (id != playbackId) return; // Ignore not our-s signal
	emit playbackStarted(currentItem);
}

void mPlayer::onTaskStreamPlayProgress(uint id, int time)
{
	if (id != playbackId) return; // Ignore not our-s signal
	emit playbackProgress(((double)currentItem->vkRecord.duration/(double)time)*100.0, time, currentItem->vkRecord.duration);
}

void mPlayer::onTaskStreamDownloadProgress(uint id, qint64 bytesReceived, qint64 bytesTotal, int speed)
{
	if (id != playbackId) return; // Ignore not our-s signal
	emit downloadProgress(((double)bytesReceived/(double)bytesTotal)*100.0, bytesReceived, bytesTotal, speed);
}

void mPlayer::onTaskStreamPaused(uint id, bool status)
{
	if (id != playbackId) return; // Ignore not our-s signal
	emit playbackPaused(status);
}

void mPlayer::onTaskStreamDownloadError(uint id)
{
	if (id != playbackId) return; // Ignore not our-s signal
	// Fuck! Download error - we need to make re-request...
	mCore::instance()->statusMessage(mCore::STATUS_VK, tr("Получение ссылки на аудиозапись..."));
	mApiVkRequest * req = new mApiVkRequest();
	req->setProperty("task-id",playbackId);
	connect(req,SIGNAL(getAudioLoaded(vkAudio)),SLOT(onVkAudioGet(vkAudio)));
	req->getAudio(currentItem->vkRecord.ownerId, currentItem->vkRecord.aid);
	mApiVk::request(req);
}

void mPlayer::onTaskStreamStopped(mStream *obj)
{
	obj->exit();
	obj->wait();
	qDebug() << obj << "Stop slot";
	QMetaObject::invokeMethod(this, "deleteStreamObj", Qt::QueuedConnection, Q_ARG(mStream*,obj));
}

void mPlayer::deleteStreamObj(mStream *obj)
{
	qDebug() << obj << "Delete slot";
	obj->wait();
	delete obj;
}

void mPlayer::onVkAudioGet(vkAudio a)
{
	if (NULL == currentItem) return; // GTFO
	if (sender()->property("task-id").toUInt() != playbackId) return; // Ignore not our-s signal
	currentItem->vkRecord = a;
	playItem(currentItem);
}

void mPlayer::onVkAudioSearch(vkAudioList list)
{
	if (NULL == currentItem) return; // GTFO
	if (!currentItem->selectBestVkSearched(list))
	{
		currentItem->isError = true;
		if (NULL != currentItem->playList)
			currentItem->playList->update();
		emit sigStreamEnded(); // Error!
		return;
	}
	playItem(currentItem);
}

void mPlayer::checkDeletedTasks() {}

void mPlayer::setPos(int pos)
{
	if (NULL != currentTask)
	{
		QMetaObject::invokeMethod(currentTask,"seek",Qt::QueuedConnection,Q_ARG(int,pos));
	}
}

void mPlayer::setVolume(double val)
{
	volume = val;
	if (NULL != currentTask)
	{
		QMetaObject::invokeMethod(currentTask,"volume",Qt::QueuedConnection,Q_ARG(uint,val));
	}
}

bool mPlayer::isPaused()
{
	if (NULL != currentTask)
	{
		return currentTask->getStreamPaused();
	}
	return false;
}
