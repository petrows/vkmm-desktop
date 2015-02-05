#include <QtSingleApplication>
#include <QObject>
#include <QTextCodec>
#include <QDateTime>
#include <QLibrary>
#include <QDebug>
#include <QMessageBox>
#include "core/core.h"

int main(int argc, char *argv[])
{
	QtSingleApplication a(argc, argv);

	a.setQuitOnLastWindowClosed(false);
	
	a.setOrganizationName("petro.ws");
	a.setApplicationName("VkMusicMania");
	a.setWindowIcon(QIcon(":/icons/vkmm-32.png"));

	qsrand(QDateTime::currentDateTime().toTime_t() - QApplication::applicationPid() - QDateTime::currentDateTime().time().msec());

	//QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
	QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
	//QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
	
	if (a.sendMessage("show"))
		return 2;

	// Run application!
	mCore::instance()->init();
	QObject::connect(&a,SIGNAL(messageReceived(QString)),mCore::instance(),SLOT(qsaMessage(QString)));

	int ret = a.exec();
	delete mCore::instance();
	return ret;
}
