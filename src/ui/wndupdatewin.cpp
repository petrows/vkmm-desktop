#include "wndupdatewin.h"
#include "ui_wndupdatewin.h"

#include "config.h"
#include "core/core.h"
#include "core/common.h"


#include <QDebug>

#include <QCloseEvent>
#include <QMessageBox>
#include <QDir>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDomDocument>
#include <QDesktopServices>
#include <QTextCodec>

wndUpdateWin::wndUpdateWin(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::wndUpdateWin)
{
    ui->setupUi(this);

	fileCurrentIndex = 0;
	sizeTotal = 0;
	sizeReady = 0;

	appDir = QApplication::applicationDirPath();

    QUrl reqUrl;
    reqUrl.setUrl(QString(UPDATE_BASE_URL));
    reqUrl.addQueryItem("a", "d");
    reqUrl.addQueryItem("v", getVersion());
    reqUrl.addQueryItem("os", OS_NAME);
    reqUrl.addQueryItem("arch", getArch());
    reqUrl.addQueryItem("vkuid", mCore::instance()->settings->value("vkuid",0).toString());

	// Scan program files

    QNetworkRequest req(reqUrl);
    QNetworkReply * reply = mCore::instance()->net->get(req);
    connect(reply,SIGNAL(finished()),SLOT(reqInfoFinished()));
}

wndUpdateWin::~wndUpdateWin()
{
    delete ui;
}

void wndUpdateWin::closeEvent(QCloseEvent *e)
{
	showMinimized();
	e->ignore();
}

void wndUpdateWin::loadNextFile()
{
	fileStartTimer.restart();
	updateFile f = updateFiles.at(fileCurrentIndex);
	QNetworkRequest req(QUrl(f.remote));
	rep = mCore::instance()->net->get(req);
	connect(rep, SIGNAL(finished()),SLOT(fileFinished()));
	connect(rep, SIGNAL(downloadProgress(qint64,qint64)),SLOT(filedownloadProgress(qint64,qint64)));

	ui->lblFile->setText(tr("%1 / %2 файлов").arg(fileCurrentIndex+1).arg(updateFiles.size()));
}

void wndUpdateWin::updateError(QString info)
{
    if (info.isEmpty()) info = tr("Произошла ошибка обновления! Повторите попытку позже.");
    QMessageBox::warning(this, tr("Ошибка обновления"),info);
    deleteLater();
	if (!tempDir.isEmpty()) removeDir(tempDir);
}

void wndUpdateWin::reqInfoFinished()
{
    QNetworkReply * reply = qobject_cast<QNetworkReply*>(sender());
    if (NULL == reply) return; // Error
    QByteArray xmlData = reply->readAll();
    reply->deleteLater();

    qDebug() << xmlData;

    QDomDocument xmlDoc;
    xmlDoc.setContent(xmlData);

    QDomElement updateTag = xmlDoc.firstChildElement("update");
    if (updateTag.isNull())
    {
        updateError(); return;
    }

    // Get notes...
    ui->textBrowser->setText(updateTag.firstChildElement("notes").firstChild().toCharacterData().data());

	// Get files list
	sizeTotal = 0;
	QDomNodeList fList = updateTag.firstChildElement("files").elementsByTagName("f");
	for (int x=0; x<fList.size(); x++)
	{
		QDomElement fEl = fList.at(x).toElement();
		updateFile updateRec;
		updateRec.local		= fEl.attribute("l");
		updateRec.remote	= fEl.attribute("r");
		updateRec.size   	= fEl.attribute("s").toUInt();
		updateFiles.push_back(updateRec);

		sizeTotal += updateRec.size;
	}

	ui->lblProgress->setText(tr("? / %1").arg(formatSize(sizeTotal)));

	// Create temp dir
	tempDir = QDir::tempPath() + QDir::separator() + "vkmm-update";
	removeDir(tempDir);
	QDir tmp = QDir(tempDir);
	if (!tmp.mkpath(tempDir))
	{
		updateError(tr("Не удалось создать временную папку: %1").arg(tempDir)); return;
	}
	qDebug() << "temp dir: " << tempDir;

	// Write MS-DOS config file...
	QFile msdosConfig(tempDir + QDir::separator() + "config.msdos.vkmmpath");
	if (!msdosConfig.open(QIODevice::WriteOnly))
	{
		updateError(tr("Ошибка записи в файл: config.msdos.vkmmpath")); return;
	}
	QTextCodec* localCodec = QTextCodec::codecForName("UTF-16");
	msdosConfig.write(localCodec->fromUnicode(QApplication::applicationDirPath()).mid(2));
	msdosConfig.close();

	// Copy skel
	if (!QFile::copy(QApplication::applicationDirPath() + QDir::separator() + "vkmm-updater.exe", tempDir + QDir::separator() + "/vkmm-updater.exe"))
	{
		updateError(tr("Ошибка записи во временную папку: %1").arg(tempDir)); return;
	}

	loadNextFile();

	show();
}

void wndUpdateWin::fileFinished()
{
	if (rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200)
	{
		updateError(tr("Не удалось загрузить файл %1").arg(updateFiles.at(fileCurrentIndex).local)); return;
	}

	// Save file...
	updateFile finfo = updateFiles.at(fileCurrentIndex);
	QString locFilePath = tempDir + QDir::separator() + "files" + QDir::separator() + finfo.local;
	// Create dir for file (if needed)
	QDir(tempDir).mkpath(QFileInfo(locFilePath).path());
	QFile updatedFile(locFilePath);
	if (!updatedFile.open(QIODevice::WriteOnly))
	{
		updateError(tr("Не удалось записать файл <TEMP>\\%1").arg(finfo.local)); return;
	}
	updatedFile.write(rep->readAll());
	updatedFile.close();

	sizeReady += finfo.size;
	fileCurrentIndex++;
	if (fileCurrentIndex >= updateFiles.size())
	{
		// All done!!!!
		startUpdater();
		return;
	}
	loadNextFile();
}

void wndUpdateWin::filedownloadProgress(qint64 bytesReceived, qint64 )
{
	// Calc persent
	double timeLoad  = fileStartTimer.elapsed();
	double speed     = (double)bytesReceived / (timeLoad/1000.0);
	int    totalProg = (int)((((double)sizeReady+(double)bytesReceived)/(double)sizeTotal)*100.0);
	totalProgress(totalProg);

	ui->lblProgress->setText(tr("%1 / %2 (%3/с)").arg(formatSize(sizeReady + bytesReceived)).arg(formatSize(sizeTotal)).arg(formatSize((int)speed)));
}

void wndUpdateWin::totalProgress(int pers)
{
	setWindowTitle(tr("%1% обновление VKMM").arg(pers));
	ui->progressBar->setValue(pers);
}

void wndUpdateWin::startUpdater()
{
	// Final stage

	// Save files list...
	QStringList msdosFiles;
	for (int x=0; x<updateFiles.size(); x++)
	{
		msdosFiles.push_back(updateFiles.at(x).local);
	}
	QFile msdosConfigFiles(tempDir + QDir::separator() + "config.msdos.files");
	if (!msdosConfigFiles.open(QIODevice::WriteOnly))
	{
		updateError(tr("Ошибка записи в файл: config.msdos.files")); return;
	}
	QTextCodec* localCodec = QTextCodec::codecForName("UTF-16");
	msdosConfigFiles.write(localCodec->fromUnicode(msdosFiles.join("\n").replace("/","\\").toStdString().c_str()).mid(2));
	msdosConfigFiles.close();

	// Run updater:
	QProcess::startDetached(tempDir + QDir::separator() + "vkmm-updater.exe");

	// QApplication::exit(0);
}

void wndUpdateWin::scanDir(QString fname)
{
	QFileInfo finf(this->appDir + fname);
	if (finf.isDir())
	{
		// It is a dir...
		QDir dr(this->appDir + fname);
		QStringList drData = dr.entryList(QDir::AllEntries|QDir::NoDotAndDotDot);
		if (!fname.isEmpty()) fname += "/";
		for (int x=0; x<drData.count(); x++)
		{
			this->scanDir(fname + drData.at(x));
		}
	} else {
		// File?
		this->appFiles << fname;
	}
}
