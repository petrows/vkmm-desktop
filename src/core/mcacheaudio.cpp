#include "mcacheaudio.h"
#include "core/core.h"

#include <QFile>
#include <QDir>
#include <QDebug>

mCacheAudio::mCacheAudio(QObject *parent) :
	QObject(parent)
{
	totalSize = 0;
}

mCacheAudio *mCacheAudio::instance()
{
	static mCacheAudio * mCacheAudioGlob = NULL;
	if (NULL == mCacheAudioGlob)
		mCacheAudioGlob = new mCacheAudio();
	return mCacheAudioGlob;
}

void mCacheAudio::syncCache()
{
	QMutexLocker lock(&dataMtx);

	limitCount = 0;
	if (mCore::instance()->settings->value("cache/limitCount", true).toBool())
	{
		limitCount = mCore::instance()->settings->value("cache/limitCountSize",100).toULongLong();
	}
	limitSize = 0;
	if (mCore::instance()->settings->value("cache/limitSize", true).toBool())
	{
		limitSize = mCore::instance()->settings->value("cache/limitSizeSize",100).toULongLong() * 1024*1024;
	}

	if (!limitCount && !limitSize) return; // No limits
	if ((0 == limitCount || limitCount >= data.size()) && (0 == limitSize || limitSize >= totalSize)) return; // Limits are in correct values

	quint64 sizeCalc = 0;
	for (int x=0; x<data.size(); x++)
	{
		sizeCalc += data.at(x)->size;
		if ((0 != limitCount && x >= limitCount) || (0 != limitSize && sizeCalc > limitSize))
		{
			// Remove file
			QFile::remove(cachedFileName(data.at(x)->aid));
			totalSize -= data.at(x)->size;
			delete data.at(x);
			data.removeAt(x);
			x--;
			continue;
		}
	}
	qDebug() << "Cache rebuild, new values: " << totalSize << "," << data.size();
}

void mCacheAudio::syncFiles()
{
	// get list of files in cache
	QFileInfoList dirList = QDir(mCore::instance()->pathCache).entryInfoList(QDir::Files);
	for (int x=0; x<dirList.size(); x++)
	{
		quint64 fileId = dirList.at(x).baseName().toULongLong();
		if (NULL == searchFile(fileId))
		{
			qDebug() << "Removing bad cache file " << fileId;
			QFile::remove(cachedFileName(fileId));
		}
	}
}

void mCacheAudio::loadCache()
{
	dataMtx.lock();
	// Load cache from stored file...
	QString cacheFile = mCore::instance()->pathStore + "/cachelist.dat";
	QFile f(cacheFile);
	if (!f.open(QIODevice::ReadOnly))
	{
		// Error!
		dataMtx.unlock();
		qDebug() << "Unable to load cache file " << cacheFile;
		return;
	}

	while (!f.atEnd())
	{
		quint64 aid = f.readLine().trimmed().toULongLong();
		quint64 size = QFile(cachedFileName(aid)).size();
		if (0 != aid && 0 != size)
		{
			cacheRecord * rec = new cacheRecord();
			rec->aid  = aid;
			rec->size = size;
			data.push_back(rec);
			totalSize += rec->size;
		}
	}
	f.close();
	qDebug() << "Cache: loaded " << data.size() << "records";
	dataMtx.unlock();

	syncCache();
	syncFiles();
}

void mCacheAudio::saveCache()
{
	dataMtx.lock();
	QString cacheFile = mCore::instance()->pathStore + "/cachelist.dat";
	QFile f(cacheFile);
	if (!f.open(QIODevice::WriteOnly))
	{
		// Error!
		dataMtx.unlock();
		qDebug() << "Unable to save cache file " << cacheFile;
		return;
	}
	totalSize = 0;
	for (int x=0; x<data.size(); x++)
	{
		QString writeData(QString::number(data.at(x)->aid) + "\n");
		f.write(writeData.toStdString().c_str());
		totalSize += data.at(x)->size;
	}
	f.close();
	qDebug() << "Cache: saved " << data.size() << "records";
	dataMtx.unlock();
	syncCache();
}

void mCacheAudio::cacheFile(quint64 id, void *fdata, quint32 fsize)
{
	dataMtx.lock();

	if (searchFile(id) != NULL)
	{
		// Already cached
		dataMtx.unlock();
		return;
	}

	QString cacheFile = cachedFileName(id);
	QFile f(cacheFile);
	if (!f.open(QIODevice::WriteOnly))
	{
		// Error!
		qDebug() << "Unable to save cache file " << cacheFile;
		dataMtx.unlock();
		return;
	}
	f.write((const char *)fdata, fsize);
	f.close();
	cacheRecord * rec = new cacheRecord();
	rec->aid  = id;
	rec->size = fsize;
	qDebug() << "Cached file " << cacheFile;
	data.push_front(rec);
	totalSize += rec->size;
	dataMtx.unlock();

	syncCache();

	static int saveCounter = 0;
	saveCounter++;
	if (saveCounter >= 3)
	{
		saveCache();
		saveCounter = 0;
	}
}

QString mCacheAudio::checkCachedFile(quint64 id)
{
	QMutexLocker lock(&dataMtx);

	cacheRecord * rec = searchFile(id);
	if (NULL == rec) return QString();

	int dataIndex = data.indexOf(rec);
	if (dataIndex != -1)
	{
		// Exists!!!
		QString cacheFile =cachedFileName(id);
		if (!QFile().exists(cacheFile))
		{
			// Error!!!
			data.removeAt(dataIndex);
			totalSize -= rec->size;
			return QString();
		}
		// Move to the top of cache
		data.removeAt(dataIndex);
		data.push_front(rec);
		return cacheFile;
	}
	return QString();
}

mCacheAudio::cacheRecord *mCacheAudio::searchFile(quint64 id)
{
	for (int x=0; x<data.size(); x++)
	{
		if (data.at(x)->aid == id) return data.at(x);
	}
	return NULL;
}

QString mCacheAudio::cachedFileName(quint64 id)
{
	return mCore::instance()->pathCache + "/" + QString::number(id) + ".mp3";
}

void mCacheAudio::cleanAll()
{
	QMutexLocker lock(&dataMtx);
	for (int x=0; x<data.size(); x++)
	{
		QFile::remove(cachedFileName(data.at(x)->aid));
	}
	totalSize = 0;
	data.clear();
}

quint64 mCacheAudio::getTotalSize()
{
	return totalSize;
}

quint64 mCacheAudio::getTotalCount()
{
	return data.size();
}

