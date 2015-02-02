#ifndef MCACHEAUDIO_H
#define MCACHEAUDIO_H

#include <QObject>
#include <QMutex>

class mCacheAudio : public QObject
{
	Q_OBJECT
public:

	struct cacheRecord
	{
		quint64 aid;
		quint64 size;
	};

	explicit mCacheAudio(QObject *parent = 0);
	static mCacheAudio * instance();
	void syncCache();
	void syncFiles();
	void loadCache();
	void saveCache();
	void cacheFile(quint64 id, void * fdata, quint32 fsize);
	QString checkCachedFile(quint64 id);
	cacheRecord * searchFile(quint64 id);

	static QString cachedFileName(quint64 id);

	void cleanAll();

	quint64 getTotalSize();
	quint64 getTotalCount();
	
signals:
	
public slots:
private:
	QMutex dataMtx;
	QList<cacheRecord*> data;
	quint64 totalSize;

	quint64 limitSize;
	quint64 limitCount;
};

#endif // MCACHEAUDIO_H
