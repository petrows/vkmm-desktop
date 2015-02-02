#ifndef WNDLOGINBROWSER_H
#define WNDLOGINBROWSER_H

#include <QDialog>
#include <QUrl>
#include <QNetworkReply>
#include <QTimer>

namespace Ui {
class wndLoginBrowser;
}

class wndLoginBrowser : public QDialog
{
	Q_OBJECT
	
public:
	explicit wndLoginBrowser(QWidget *parent = 0);
	~wndLoginBrowser();

	void closeEvent(QCloseEvent *);
	void hideEvent(QHideEvent *);
	
private:
	Ui::wndLoginBrowser *ui;
	bool authOk;
	QTimer urlCheckTimer;

public slots:
	void urlChanged(QUrl url);
	void urlProgress(int p);
	void runAuth();
	void loadFinished(bool isOk);
	void loadStarted();
	void unsupportedContent (QNetworkReply * reply);
	void onCheckTimer();

signals:
	void authRejected();
	void authDone(QString token, qint32 expired, qint32 userId);
};

#endif // WNDLOGINBROWSER_H
