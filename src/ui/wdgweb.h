#ifndef WDGWEB_H
#define WDGWEB_H

#include <QWebView>
#include <QList>

class wdgWeb : public QWebView
{
	Q_OBJECT
public:
	explicit wdgWeb(QWidget *parent = 0);
	virtual ~wdgWeb();
	
signals:
	
public slots:

protected:
	void keyPressEvent(QKeyEvent * ev);
};

#endif // WDGWEB_H
