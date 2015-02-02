#include "mupdater.h"
#include "ui/wndupdate.h"

#include "core/core.h"
#include "core/common.h"
#include "core/mnetowrkaccess.h"
#include "config.h"

#include <QProcess>
#include <QFile>
#include <QDebug>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDomDocument>

#ifdef Q_OS_WIN
#   include "ui/wndupdatewin.h"
#endif

mUpdater::mUpdater(QObject *parent) :
	QObject(parent)
{
}

void mUpdater::checkForUpdates()
{
	QUrl reqUrl;
	reqUrl.setUrl(QString(UPDATE_BASE_URL));
    reqUrl.addQueryItem("a", "i");
    reqUrl.addQueryItem("v", getVersion());
    reqUrl.addQueryItem("os", OS_NAME);
    reqUrl.addQueryItem("arch", getArch());
	reqUrl.addQueryItem("vkuid", mCore::instance()->settings->value("vkuid",0).toString());
	// qDebug() << reqUrl;
	QNetworkRequest req(reqUrl);
	if (mCore::instance()->updateKey.length())
	{
		req.setRawHeader("Update-Key", mCore::instance()->updateKey.toStdString().c_str());
	}
	QNetworkReply * reply = mCore::instance()->net->get(req);
	connect(reply,SIGNAL(finished()),SLOT(checkForUpdatesFinished()));
}

void mUpdater::setKeyInfo(QString key)
{
	QUrl reqUrl;
	reqUrl.setUrl(QString(UPDATE_BASE_URL));
	reqUrl.addQueryItem("a", "key");
	QNetworkRequest req(reqUrl);
	req.setRawHeader("Update-Key", key.toStdString().c_str());
	// qDebug() << "Update UID: " << key;
	mCore::instance()->net->get(req);
}

void mUpdater::startUpdate()
{
    // Starting update!

#ifdef Q_OS_WIN
    wndUpdateWin * wnd = new wndUpdateWin();
#endif
}

void mUpdater::checkForUpdatesFinished()
{
	// Check XML result
	QNetworkReply * reply = qobject_cast<QNetworkReply*>(sender());
	QByteArray xmlData = reply->readAll();
	reply->deleteLater();

	QDomDocument xmlDoc;
	xmlDoc.setContent(xmlData);

	QDomElement updateTag = xmlDoc.firstChildElement("update");
	if (!updateTag.isNull())
	{
		if (updateTag.attribute("v","").length()>1)
		{
			// Okay, got tag? Send signal!
            emit needUpdate(true, updateTag.attribute("v",""));
			return;
		}
	}

	// No update
	// Kill myself
	deleteLater();
}
