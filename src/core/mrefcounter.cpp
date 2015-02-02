#include "mrefcounter.h"

mRefCounter::mRefCounter(QObject *parent) :
	QObject(parent)
{
	refCount = 1;
}

void mRefCounter::reference()
{
	refLock.lock();
	refCount++;
	refLock.unlock();
}

void mRefCounter::release()
{
	int newRefCount;
	refLock.lock();
	refCount--;
	newRefCount = refCount;
	refLock.unlock();
	if (newRefCount <= 0)
		deleteLater();
}

int mRefCounter::getReferences()
{
	int currRefCount;
	refLock.lock();
	currRefCount = refCount;
	refLock.unlock();
	return currRefCount;
}

