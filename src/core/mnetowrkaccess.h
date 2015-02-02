#ifndef MNETOWRKACCESS_H
#define MNETOWRKACCESS_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class mNetowrkAccess : public QNetworkAccessManager
{
	Q_OBJECT
public:
	explicit mNetowrkAccess(QObject *parent = 0);
	static mNetowrkAccess * instance();

protected:
	QNetworkReply * createRequest (Operation op, const QNetworkRequest & req, QIODevice * outgoingData = 0);

signals:
	
public slots:
    void ignoreSSLErors(QNetworkReply * reply, const QList<QSslError> & errors);
	void ignoreSSLErorsReply(const QList<QSslError> & errors);
	void error(QNetworkReply::NetworkError code);
	void progress(qint64, qint64);
};

#endif // MNETOWRKACCESS_H
