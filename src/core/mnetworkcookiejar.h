#ifndef MNETWORKCOOKIEJAR_H
#define MNETWORKCOOKIEJAR_H

#include <QNetworkCookieJar>

class mNetworkCookieJar : public QNetworkCookieJar
{
	Q_OBJECT
public:
	explicit mNetworkCookieJar(QObject *parent = 0);

	void cookiesLoad();
	void cookiesSave();
	void cookiesClean();

	QList<QNetworkCookie> cookiesForUrl ( const QUrl & url );
	
signals:
	
public slots:
	
};

#endif // MNETWORKCOOKIEJAR_H
