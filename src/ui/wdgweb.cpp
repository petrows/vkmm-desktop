#include "wdgweb.h"
#include <QKeyEvent>
#include <QDebug>

#include "core/core.h"
#include "core/mupdater.h"
#include "version.h"

wdgWeb::wdgWeb(QWidget *parent) :
    QWebView(parent)
{
}

wdgWeb::~wdgWeb()
{
	qDebug() << "Web deleted";
}

void wdgWeb::keyPressEvent(QKeyEvent *ev)
{
	QWebView::keyPressEvent(ev);
}
