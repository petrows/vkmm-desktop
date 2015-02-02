#ifndef WNDUPDATEWIN_H
#define WNDUPDATEWIN_H

#include <QDialog>
#include <QList>
#include <QTime>
#include <QProcess>

#include "core/mnetowrkaccess.h"

namespace Ui {
class wndUpdateWin;
}

struct updateFile
{
    QString remote;
    QString local;
    quint64 size;
};

class wndUpdateWin : public QDialog
{
    Q_OBJECT
    
public:
    explicit wndUpdateWin(QWidget *parent = 0);
    ~wndUpdateWin();

    void closeEvent(QCloseEvent * e);
	void loadNextFile();
	void scanDir(QString fname);
    
private:
    Ui::wndUpdateWin *ui;
    quint64             sizeTotal;
    quint64             sizeReady;
	QTime				fileStartTimer;
    int                 fileCurrentIndex;
    QList<updateFile>   updateFiles;
	QNetworkReply		* rep;
	QString				tempDir;
	QString				appDir;
	QStringList			appFiles;

private slots:
    void updateError(QString info = "");
    void reqInfoFinished();
	void fileFinished();
	void filedownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void totalProgress(int pers);
	void startUpdater();
};

#endif // WNDUPDATEWIN_H
