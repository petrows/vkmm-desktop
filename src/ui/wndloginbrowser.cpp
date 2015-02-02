#include "wndloginbrowser.h"
#include "ui_wndloginbrowser.h"

#include <QCloseEvent>
#include <QWebFrame>
#include <QDebug>
#include <QUrlQuery>

#include "core/core.h"
#include "core/mnetowrkaccess.h"
#include "config.h"

wndLoginBrowser::wndLoginBrowser(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::wndLoginBrowser)
{
	ui->setupUi(this);
	authOk = false;

	ui->webView->page()->setNetworkAccessManager(mCore::instance()->net);
	ui->webView->page()->currentFrame()->setHtml("<center><br/><br/>Пожалуйста, подождите...<br/><br/>Загрузка авторизации ВКонтакте</center>");

	connect(ui->webView,SIGNAL(urlChanged(QUrl)),SLOT(urlChanged(QUrl)));
	connect(ui->webView,SIGNAL(loadProgress(int)),SLOT(urlProgress(int)));
	connect(ui->webView,SIGNAL(loadFinished(bool)),SLOT(loadFinished(bool)));
	connect(ui->webView,SIGNAL(loadStarted()),SLOT(loadStarted()));
	connect(ui->webView->page(), SIGNAL(unsupportedContent(QNetworkReply*)), SLOT(unsupportedContent(QNetworkReply*)));

	connect(&urlCheckTimer, SIGNAL(timeout()), SLOT(onCheckTimer()));
}

wndLoginBrowser::~wndLoginBrowser()
{
	delete ui;
}

void wndLoginBrowser::closeEvent(QCloseEvent * ev)
{
	if (!authOk)
	{
		emit authRejected();
	}
}

void wndLoginBrowser::hideEvent(QHideEvent *)
{

}

void wndLoginBrowser::runAuth()
{
	QUrl loginUrl("https://oauth.vk.com/authorize");
	QUrlQuery q(loginUrl.query());
	q.addQueryItem("client_id",QString::number(APP_ID));
	q.addQueryItem("display","popup");
	q.addQueryItem("redirect_uri","close.html");
	q.addQueryItem("response_type","token");

	int scope = 0;
	scope += 2; // Firends
	scope += 8; // Audio
	scope += 1024 ; // Status
	scope += 2048 ; // Wall
    scope += 8192 ; // Wall post
    scope += 262144 ; // Groups

	q.addQueryItem("scope",QString::number(scope));
	
	loginUrl.setQuery(q);

	ui->webView->setHtml(tr("<html><head><meta http-equiv=\"refresh\" content=\"0;URL='%1'\"></head><body><center><br/><br/>Пожалуйста, подождите...<br/><br/>Загрузка авторизации ВКонтакте</center></body></html>").arg(loginUrl.toString()));

	urlCheckTimer.start(10000);
	urlCheckTimer.setSingleShot(true);
}

void wndLoginBrowser::loadFinished(bool isOk)
{
	qDebug() << "Load result: " << isOk;
}

void wndLoginBrowser::loadStarted()
{
	qDebug() << "Load started";
}

void wndLoginBrowser::unsupportedContent(QNetworkReply *reply)
{
	qDebug() << "Unsopported content!";
}

void wndLoginBrowser::onCheckTimer()
{
	qDebug() << "Reset auth browser!!!!";
	runAuth();
}

void wndLoginBrowser::urlChanged(QUrl url)
{
	qDebug() << "New URL: " << url;
	urlCheckTimer.stop();

	if (!authOk && url.host() == "oauth.vk.com" && url.path() == "/close.html")
	{
		// Auth is OK!
		QUrlQuery hashData(url.fragment());
		QString token = hashData.queryItemValue("access_token");
		qint32  exp   = hashData.queryItemValue("expires_in").toInt();
		qint32  uid   = hashData.queryItemValue("user_id").toInt();

		authOk = true;

		hide();
		close();
		emit authDone(token, exp, uid);
		deleteLater();
	}
}

void wndLoginBrowser::urlProgress(int p)
{
	//qDebug() << "Progress: " << p;
}
