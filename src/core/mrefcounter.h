#ifndef MREFCOUNTER_H
#define MREFCOUNTER_H

#include <QObject>
#include <QMutex>

#define SAFE_RELEASE(p) {if (p) ((p)->release()); p = NULL;}

class mRefCounter: public QObject
{
	Q_OBJECT
public:
	mRefCounter(QObject * parent = 0);

	void reference();
	void release();
	int  getReferences();

private:
	int refCount;
	QMutex refLock;
};

#endif // MREFCOUNTER_H
