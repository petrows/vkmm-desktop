#ifndef MSTREAM_H
#define MSTREAM_H

#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QUrl>

#include <QNetworkReply>
#include <QNetworkRequest>

#include "bass.h"

#include "core/common.h"
#include "core/mnetowrkaccess.h"

#define MSTREAM_DEBUG(a) { qDebug() << this << __FUNCTION__ << __LINE__ << a; }

class mStreamStarter;
class mStream;

class mStreamStarter : public QThread
{
	Q_OBJECT
public:
	explicit mStreamStarter(mStream * str, QObject *parent = 0);
	~mStreamStarter();
	void run();
signals:

public slots:

private:
	mStream * stream;
};

class mStream : public QThread
{
	Q_OBJECT
public:
	explicit mStream(uint streamId, QUrl streamUrl, qint64 streamUrlId, QObject *parent = 0);
	~mStream();
	void run();

	// Stream callbacks:
	void	cbStreamEnded();
	BOOL	cbStreamSeek(QWORD offset);
	QWORD	cbStreamLength();
	DWORD	cbStreamRead(void * buf, DWORD len);

	int		getStreamPos();
	bool	getStreamPaused() { return streamIsPaused; }

signals:
	void streamEnded(uint id);
	void streamDownloadStatus(uint id, qint64 bytesReceived, qint64 bytesTotal, int speed);
	void streamPlaybackStatus(uint id, int time);
	void streamPaused(uint id, bool isPaused);

	void streamStopped(mStream * obj);

	void streamDownloadError(uint id);

public slots:
	void stop();
	void volume(uint volume);
	void pause();
	void seek(int time);

private slots:
	void onStreamCreated(HSTREAM playBackHandle);
	void onStreamUpdPos();
	void onNetProgress(qint64 bytesReceived , qint64 bytesTotal);
	void onNetReadyRead();
	void onNetError(QNetworkReply::NetworkError code);

private:
	mStreamStarter			* starter;
	uint					  id;
	QUrl					  url;
	qint64					  urlId;
	HSTREAM					  playback;
	mNetowrkAccess			* net;
	QNetworkReply			* netReply;
	bool					  needStop;
	bool					  streamCreated;
	bool					  starterCreated;
	QMutex					  streamBufMtx;
	QMutex					  streamNetMtx;

	quint8					* streamBuf;
	volatile qint32			  streamBufRdy;
	volatile qint32			  streamBufPos;
	volatile qint32			  streamBufSize;
    
    QTime                     downloadSpeedTimer;
    quint32                   downloadSpeedSize;
    int                       downloadSpeedLast;

	bool					  streamIsPaused;
	uint					  streamVolume;
	int						  streamPrevPos;
	QTimer					* streamPosTimer;

	// Stream buffering progress
	quint32					  bufferStatusNeed;
	quint32					  bufferStatusReady;
	quint32					  bufferStatusSigTime;
};

#endif // MSTREAM_H
