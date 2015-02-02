#ifndef WDGLFMLINECOMPLETED_H
#define WDGLFMLINECOMPLETED_H

#include <QLineEdit>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <QTimer>

#include "core/mnetowrkaccess.h"

class wdgLfmLineCompleted : public QLineEdit
{
	Q_OBJECT
public:
	explicit wdgLfmLineCompleted(QWidget *parent = 0);

	enum lfmCompleteType
	{
		COMPLETE_ARTIST
	};

	void setCompleteMode(lfmCompleteType m);

signals:
	
public slots:

private slots:
	void requestFinished();
	void textChangedStart(QString text);
	void completeTimerShot();

private:
	lfmCompleteType completeMode;
	QTimer completeTimer;
	QNetworkReply * rep;
	QStringList repStrings;
	void requestComplete(QString text);
	bool parseReply(QString data);
};

#endif // WDGLFMLINECOMPLETED_H
