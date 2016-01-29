#include "mstream.h"
#include <core/core.h>
#include "core/mcacheaudio.h"

#include <QTime>
#include <QTimer>
#include <QFile>

// Callback functions
void CALLBACK mStreamCbEnded(HSYNC, DWORD, DWORD, void *user) { ((mStream*)user)->cbStreamEnded(); }
void CALLBACK mStreamCbClose(void *) {}
BOOL CALLBACK mStreamCbSeek(QWORD offset, void *user) { return ((mStream*)user)->cbStreamSeek(offset); }
QWORD CALLBACK mStreamCbLength(void * user) { return ((mStream*)user)->cbStreamLength(); }
DWORD CALLBACK mStreamCbReader(void * buf, DWORD len, void * user) { return ((mStream*)user)->cbStreamRead(buf, len); }

mStreamStarter::mStreamStarter(mStream *str, QObject *parent) :
	QThread(parent)
{
	stream = str;
}

mStreamStarter::~mStreamStarter()
{
	MSTREAM_DEBUG(0);
}

void mStreamStarter::run()
{
	// Run start stream & exit
	MSTREAM_DEBUG(0);

	BASS_Free();

	if (!BASS_Init(-1, 44100, BASS_DEVICE_FREQ | BASS_DEVICE_DMIX, NULL, NULL))
	{
		MSTREAM_DEBUG("Error in init!!");
	}

	// Start stream creation proc
	BASS_FILEPROCS fileprocs = {&mStreamCbClose, &mStreamCbLength, &mStreamCbReader, &mStreamCbSeek}; // callback table
	HSTREAM ss = BASS_StreamCreateFileUser(STREAMFILE_BUFFER, BASS_STREAM_PRESCAN, &fileprocs, stream);
	if (ss)
	{
		BASS_ChannelSetSync(ss, BASS_SYNC_END|BASS_SYNC_ONETIME, 0, &mStreamCbEnded, stream);
	}
	MSTREAM_DEBUG(0);
	QMetaObject::invokeMethod(stream,"onStreamCreated",Qt::QueuedConnection,Q_ARG(HSTREAM,ss));
	exec();
}

mStream::mStream(uint streamId, QUrl streamUrl, qint64 streamUrlId, QObject *parent) :
	QThread(parent)
{
	QObject::moveToThread(this);
	starter		= NULL;
	net			= NULL;
	netReply	= NULL;
	id			= streamId;
	url			= streamUrl;
	urlId		= streamUrlId;
	playback	= 0;

	needStop		= false;
	streamCreated	= false;
	starterCreated	= false;

	streamIsPaused	= false;
	streamVolume	= 100;
	streamPrevPos	= -1;
	streamPosTimer	= NULL;

	// Streaming media buffer
	streamBuf		= NULL;
	streamBufRdy	= 0;
	streamBufSize	= 0;
	streamBufPos	= 0;
	bufferStatusNeed  = 0;
	bufferStatusReady = 0;
	bufferStatusSigTime = 0;

	mCore::instance()->statusMessage(mCore::STATUS_VK, tr("Подключение..."));
}

mStream::~mStream()
{
	MSTREAM_DEBUG("Deleted");
}

void mStream::run()
{
	// Cache playing?
	QString cachedFile = mCacheAudio::instance()->checkCachedFile(urlId);
	if (cachedFile.isEmpty())
	{
		// Start network direct load...
		net = new mNetowrkAccess(this);
		net->moveToThread(this);
		setProxy(net);
		netReply = net->get(QNetworkRequest(url));
		netReply->moveToThread(this);
		downloadSpeedTimer.restart();
		downloadSpeedSize = 0;
		downloadSpeedLast = -1;
		MSTREAM_DEBUG(1);
		// Data signal
		connect(netReply,SIGNAL(readyRead()),SLOT(onNetReadyRead()));
		// Progress signal
		connect(netReply,SIGNAL(downloadProgress(qint64,qint64)),SLOT(onNetProgress(qint64,qint64)));
		// Error signal
		connect(netReply,SIGNAL(error(QNetworkReply::NetworkError)),SLOT(onNetError(QNetworkReply::NetworkError)));
		// End request (check an error)
		connect(netReply, &QNetworkReply::finished, this, &mStream::onNetResult);
	} else {
		// Start cached (local file) playback
		QFile cachedData(cachedFile);
		if (cachedData.open(QIODevice::ReadOnly))
		{
			starterCreated = true;

			streamBufSize	= cachedData.size();
			streamBuf		= new quint8 [streamBufSize];
			streamBufRdy	= streamBufSize;
			streamBufPos	= 0;

			cachedData.read((char*)streamBuf, streamBufSize);

			// Start 'Starter' thread - for async media call
			starter = new mStreamStarter(this, this);
			starter->moveToThread(this);
			starter->start();

			cachedData.close();
			emit streamDownloadStatus(id, streamBufSize, streamBufSize, -1);
		}
	}

	exec();

	SAFE_DELETE(starter);
	SAFE_DELETE(net);
	SAFE_DELETE(netReply);
	SAFE_DELETE(streamPosTimer);
	SAFE_DELETE_ARRAY(streamBuf);

	MSTREAM_DEBUG("Loop exit");
}

void mStream::cbStreamEnded()
{
	emit streamEnded(id);
}

BOOL mStream::cbStreamSeek(QWORD offset)
{
	streamBufMtx.lock();
	int readySize = streamBufRdy;
	streamBufMtx.unlock();
	if (readySize < (int)offset) return false;
	streamBufPos = offset;
	return true;
}

QWORD mStream::cbStreamLength()
{
	return streamBufSize;
}

DWORD mStream::cbStreamRead(void *buf, DWORD len)
{
	int readySize = 0;
	while (true)
	{
		streamBufMtx.lock();
		readySize = streamBufRdy;
		streamBufMtx.unlock();

		if (len <= (readySize - streamBufPos))
		{
			// All ok, not buffering
			streamBufMtx.lock();
			bufferStatusNeed = 0;
			streamBufMtx.unlock();
			break;
		} else {
			streamBufMtx.lock();
			bufferStatusNeed	= len - (readySize - streamBufPos);
			bufferStatusReady	= len;
			streamBufMtx.unlock();
		}
		msleep(100);
		// Need to exit??
		if (needStop)
		{
			return 0;
		}
	}
	if (needStop)
	{
		return 0;
	}
	// Copy data to buffer
	memcpy(buf, streamBuf+streamBufPos, len);
	streamBufPos += len;
	return len;
}

int mStream::getStreamPos()
{
	if (!playback) return 0;
	return BASS_ChannelBytes2Seconds(playback,BASS_ChannelGetPosition(playback,BASS_POS_BYTE));
}

void mStream::stop()
{
	if (NULL != netReply)
	{
		netReply->abort();
		SAFE_DELETE(netReply);
	}
	if (!streamCreated)
	{
		needStop = true;
		return; // Wait
	}
	needStop = true;
	// We got here from onStreamCreated or from onStreamError
	if (playback)
	{
		BASS_ChannelSetAttribute(playback,BASS_ATTRIB_VOL,0.0);
		msleep(200); // Windows fix to prevent sound noise at channel stop
		BASS_ChannelStop(playback);
		BASS_StreamFree(playback);
		playback = 0;
	}
	MSTREAM_DEBUG("Stream stopped...");
	emit streamStopped(this);
}

void mStream::volume(uint volume)
{
	streamVolume = volume;
	if (playback)
	{
		BASS_ChannelSetAttribute(playback,BASS_ATTRIB_VOL,streamVolume/100.0f);
	}
}

void mStream::pause()
{
	if (playback)
	{
		BASS_ChannelLock(playback, true);
		if (!streamIsPaused)
		{
			BASS_ChannelSetAttribute(playback,BASS_ATTRIB_VOL,0.0);
			QThread::msleep(100);
			BASS_ChannelPause(playback);
		} else {
			BASS_ChannelPlay(playback, false);
			QThread::msleep(100);
			volume(streamVolume);
		}
		BASS_ChannelLock(playback, false);
		streamIsPaused = !streamIsPaused;
		emit streamPaused(id,streamIsPaused);
	}
}

void mStream::seek(int time)
{
	if (playback)
	{
		int newTime = time;
		int posBytes = BASS_ChannelSeconds2Bytes(playback,newTime);

		BASS_ChannelPause(playback);
		BASS_ChannelSetPosition(playback,posBytes,BASS_POS_BYTE);
        if (!streamIsPaused)
            BASS_ChannelPlay(playback, false); // Not restart on pause
	}
}

void mStream::onStreamCreated(HSTREAM playBackHandle)
{
	playback = playBackHandle;
	starter->exit();
	starter->wait();
	SAFE_DELETE(starter);
	MSTREAM_DEBUG("Stream created!");
	streamCreated = true;
	if (needStop)
	{
		// Killing :(
		stop();
		return;
	}

	// Start the New PlayBack!
	mCore::instance()->statusMessage(mCore::STATUS_OK, tr("Начинаю проигрывание"));
	if (playback)
	{
		BASS_ChannelSetAttribute(playback,BASS_ATTRIB_VOL,streamVolume/100.0f);
		if (!streamIsPaused)
		{
			BASS_ChannelPlay(playback, false);
		}
		streamPosTimer = new QTimer(this);
		streamPosTimer->start(300);
		connect(streamPosTimer,SIGNAL(timeout()),SLOT(onStreamUpdPos()));
	}
	if (NULL != netReply)
	{
		disconnect(netReply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(onNetError(QNetworkReply::NetworkError)));
	}
}

void mStream::onStreamUpdPos()
{
	if (!playback) return;

	BASS_ChannelLock(playback, true);
	int pos = getStreamPos();
	BASS_ChannelLock(playback, false);
	if (pos != streamPrevPos)
		emit streamPlaybackStatus(id, pos);
	streamPrevPos = pos;
}

void mStream::onNetProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	if (200 != netReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()) return; // Code if fuck off

	if (0 == streamBufSize)
		streamBufSize = bytesTotal;
    
    if (downloadSpeedTimer.elapsed() > 1000)
    {    
        int speedms     = downloadSpeedTimer.elapsed();
        int speedsize   = bytesReceived - downloadSpeedSize;
        int speed       = (int)((double)speedsize/(double)((double)speedms/1000.0));
        
        downloadSpeedLast = speed;
        
        downloadSpeedTimer.restart();
        downloadSpeedSize = bytesReceived;    
    }
	if (bytesReceived == bytesTotal)
		downloadSpeedLast = -1;
    
	emit streamDownloadStatus(id, bytesReceived, bytesTotal, downloadSpeedLast);
}

void mStream::onNetReadyRead()
{
	if (!starterCreated)
	{
		// Set up reading vars - size
		if (streamBufSize <= 0) return; // Wait a little

		// Code is 200?
		if (200 != netReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()) return; // Code if fuck off

		starterCreated = true;
		MSTREAM_DEBUG("Ready read - first call, " << streamBufSize);

		streamBuf		= new quint8 [streamBufSize];
		emit streamDownloadStatus(id, 0, streamBufSize, -1);
		streamBufRdy	= 0;
		streamBufPos	= 0;

		// Start 'Starter' thread - for async media call
		starter = new mStreamStarter(this, this);
		starter->start();
	}

	QMutexLocker lock(&streamNetMtx);
	Q_UNUSED(lock);

	int readySize = netReply->bytesAvailable();
	if (readySize <= 0) return;
	if (readySize > (streamBufSize - streamBufRdy))
		readySize = (streamBufSize - streamBufRdy);
	if (readySize <= 0) return;
	streamBufMtx.lock();
	int realRead = netReply->read((char*)(streamBuf+streamBufRdy), readySize);
	streamBufRdy += realRead;
	streamBufMtx.unlock();

	quint32 currTms = QTime::currentTime().second();
	if (currTms != bufferStatusSigTime)
	{
		bufferStatusSigTime = currTms;
	}
	if (streamBufSize == streamBufRdy)
	{
		// All done!
		MSTREAM_DEBUG("Download done");
		mCore::instance()->statusMessage(mCore::STATUS_OK, tr("Файл загружен"));
		mCacheAudio::instance()->cacheFile(urlId, streamBuf, streamBufSize);
		emit streamDownloadStatus(id, streamBufRdy, streamBufSize, -1);
	}
}

void mStream::onNetError(QNetworkReply::NetworkError code)
{
	if (QNetworkReply::OperationCanceledError == code) return; // This is OK
	if (QNetworkReply::NoError == code) return; // This is OK
	MSTREAM_DEBUG("Network error!");
	netReply->blockSignals(true);
	streamCreated = true;
	needStop = true;

	// Error!
	mCore::instance()->statusMessage(mCore::STATUS_FAIL, tr("Ошибка загрузки файла"));
	emit streamDownloadError(id);
}

void mStream::onNetResult()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	Q_CHECK_PTR(reply);

	if (!reply) return;

	switch (reply->error())
	{
	case QNetworkReply::NoError:
	{
		int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		switch (statusCode) {
		case 301:
		case 302:
		case 307:
			qDebug() << "redirected: " << reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();

			onNetError(QNetworkReply::ContentNotFoundError);
			break;
		default:
			break;
		}
	} break;
	case QNetworkReply::ContentNotFoundError:
		// 404 Not found
		MSTREAM_DEBUG("Error 404!")
		break;
	}
}
