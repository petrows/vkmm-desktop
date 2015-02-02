#ifndef MPLAYER_H
#define MPLAYER_H

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRunnable>
#include <QThreadPool>


#include "bass.h"

#include "core/mstream.h"
#include "core/mplaylist.h"

class mPlayer : public QObject
{
	Q_OBJECT
public:
	explicit mPlayer(QObject *parent = 0);
	~mPlayer();
	static mPlayer * instance();

	void setPos(int pos);
	void setVolume(double val);
	bool isPaused();
	mPlayListItem * getCurrentItem() { return currentItem; }
	double volume;
	
signals:
	void sigStreamEnded();
	void downloadProgress(double progress, int loaded, int total, int speed);
	void playbackProgress(double progress, int playTime, int totalTime);
	void playbackStarted(mPlayListItem * item);
	void playbackStartedStream(mPlayListItem * item);
	void playbackStopped();
	void playbackPaused(bool pause);

	// Internal task events...
	void taskStart();
	void taskKill(int id);
	void taskVolume(int id, double vol);

public slots:
	void playItem(mPlayListItem * item);
	void stop();
	void pause();
private slots:
	void onTaskStreamEnd(uint id);
	void onTaskStreamCreated(uint id);
	void onTaskStreamPlayProgress(uint id, int time);
	void onTaskStreamDownloadProgress(uint id, qint64 bytesReceived, qint64 bytesTotal, int speed);
	void onTaskStreamPaused(uint id, bool status);
	void onTaskStreamDownloadError(uint id);
	void onTaskStreamStopped(mStream * obj);
	void deleteStreamObj(mStream * obj);

	void onVkAudioGet(vkAudio a);
	void onVkAudioSearch(vkAudioList list);
private:
	void checkDeletedTasks();
    uint	  playbackId;
	mStream * currentTask;
	QList<mStream*> taskToDelete;
	mPlayListItem * currentItem;
	QTimer updTimer;
};

#endif // MPLAYER_H
