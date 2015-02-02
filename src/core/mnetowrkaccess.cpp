#include "mnetowrkaccess.h"
#include "core/common.h"
#include <QDebug>

mNetowrkAccess::mNetowrkAccess(QObject *parent) :
	QNetworkAccessManager(parent)
{
	connect(this,SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),SLOT(ignoreSSLErors(QNetworkReply*,QList<QSslError>)));
}

mNetowrkAccess * mNetowrkAccess::instance()
{
	static mNetowrkAccess * inst = NULL;
	if (NULL == inst)
		inst = new mNetowrkAccess();
	return inst;
}

QNetworkReply * mNetowrkAccess::createRequest (Operation op, const QNetworkRequest & req, QIODevice * outgoingData)
{
	//qDebug() << "Request: " << req.url();
	QNetworkRequest req_out = req;
	req_out.setRawHeader(QByteArray("User-Agent"),QByteArray(QString("VKMM/%1").arg(getVersion()).toStdString().c_str()));
	QNetworkReply * ret = QNetworkAccessManager::createRequest(op, req_out, outgoingData);
	ret->ignoreSslErrors();
	connect(ret,SIGNAL(error(QNetworkReply::NetworkError)),SLOT(error(QNetworkReply::NetworkError)));
	connect(ret,SIGNAL(downloadProgress(qint64,qint64)),SLOT(progress(qint64, qint64)));
	connect(ret,SIGNAL(sslErrors(QList<QSslError>)),SLOT(ignoreSSLErorsReply(QList<QSslError>)));
	return ret;
}

void mNetowrkAccess::ignoreSSLErors(QNetworkReply *reply, const QList<QSslError> &errors)
{
	//qDebug() << "SSL error!";
	reply->ignoreSslErrors();
}

void mNetowrkAccess::ignoreSSLErorsReply(const QList<QSslError> &errors)
{
	//qDebug() << "SSL error!";
	QNetworkReply * ret = qobject_cast<QNetworkReply*>(sender());
	if (NULL != ret)
	{
		ret->ignoreSslErrors();
	}
}

void mNetowrkAccess::error(QNetworkReply::NetworkError code)
{
	//qDebug() << "Net error: " << (int)code;
}

void mNetowrkAccess::progress(qint64 a, qint64 b)
{
	//qDebug() << "Progress: " << a << b;
}
