#ifndef COMMON_H
#define COMMON_H

#include <QObject>
#include <QDialogButtonBox>
#include <QString>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkProxy>

#define SAFE_DELETE(p) {if(NULL!=p) {delete p; p = NULL;} }
#define SAFE_DELETE_ARRAY(p) {if(NULL!=p) {delete[] p; p = NULL;} }

#define LISTS_ITEMS_HEIGHT 20

#ifdef Q_OS_LINUX
    #define OS_NAME "linux"
#endif
#ifdef Q_OS_WIN32
    #define OS_NAME "win"
#endif
#ifdef Q_OS_MAC
    #define OS_NAME "macx"
#endif

QString getCss(QString name);
QString getFile(QString name);
void getVersion(int & major, int & minor, int & build);
QString getVersion();
QString getArch();
QDateTime getBuildTime();
int getMonthNum(const QString &m);

void removeDir(QString dir);

QString formatSize(quint64 size, QString format = "%.2f {S}");
QString md5(QString str);
int levenshteinDistance(const QString& s1, const QString& s2);
QString clearGroupTitle(QString title);
void setProxy(QNetworkAccessManager * mng);

class langButtons
{
public:
	langButtons();
	static void setButtons(QDialogButtonBox * box);

	QString btnOk;
	QString btnCancel;
	QString btnApply;
	QString btnSave;
};



#endif // COMMON_H
