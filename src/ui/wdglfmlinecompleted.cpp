#include "wdglfmlinecompleted.h"
#include "core/core.h"

#include <QCompleter>
#include <QDebug>
#include <QUrlQuery>

wdgLfmLineCompleted::wdgLfmLineCompleted(QWidget *parent) :
    QLineEdit(parent)
{
	rep = NULL;
	connect(&completeTimer, SIGNAL(timeout()), SLOT(completeTimerShot()));
	//connect(this, SIGNAL(textChanged(QString)), SLOT(textChangedStart(QString)));
	connect(this, SIGNAL(textEdited(QString)), SLOT(textChangedStart(QString)));
}

void wdgLfmLineCompleted::setCompleteMode(wdgLfmLineCompleted::lfmCompleteType m)
{
}

void wdgLfmLineCompleted::requestFinished()
{
	if (!parseReply(rep->readAll()))
	{
		// Ivalid/empty reply
		setCompleter(new QCompleter());
	} else {
		QCompleter * cc = new QCompleter(repStrings, this);
		setCompleter(cc);
		cc->complete();
	}

	rep->deleteLater();
	rep = NULL;
}

bool wdgLfmLineCompleted::parseReply(QString data)
{
	repStrings.clear();

	// qDebug() << data;

	QJson::Parser parser;
	bool ok;

	QVariantMap result = parser.parse(data.toStdString().c_str(), &ok).toMap();
	if (!ok) return false;

	QVariantMap resultHeader = result["response"].toMap();
	QVariantList res = resultHeader["docs"].toList();

	if (res.size() == 0) return false; // Empty reply

	for (int x=0; x<res.size(); x++)
	{
		QVariantMap doc = res.at(x).toMap();
		if (doc["restype"].toInt() != 6) continue;
		repStrings.push_back(doc["artist"].toString());
	}

	return repStrings.size()>0;
}

void wdgLfmLineCompleted::textChangedStart(QString)
{
	completeTimer.stop();
	completeTimer.setSingleShot(true);
	completeTimer.start(300);
}

void wdgLfmLineCompleted::completeTimerShot()
{
	QString q = text().toLower().trimmed();
	if (q.length() < 2) return; // Fuck off

	requestComplete(q);
}

void wdgLfmLineCompleted::requestComplete(QString text)
{
	if (NULL != rep)
	{
		rep->abort();
		rep->deleteLater();
		rep = NULL;
	}


	// http://www.lastfm.ru/search/autocomplete?q=as&force=1&username=SA_Mann
	QUrl reqUrl("http://www.lastfm.ru/search/autocomplete");
	QUrlQuery q;
	q.addQueryItem("q", text.toStdString().c_str());
	q.addQueryItem("force", "1");
	reqUrl.setQuery(q);

	rep = mCore::instance()->net->get(QNetworkRequest(reqUrl));
	connect(rep,SIGNAL(finished()),SLOT(requestFinished()));
}

