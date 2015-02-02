#ifndef MUPDATER_H
#define MUPDATER_H

#include <QObject>

class mUpdater : public QObject
{
	Q_OBJECT
public:
	explicit mUpdater(QObject *parent = 0);


signals:
    void needUpdate(bool need, QString version);
public slots:
	void startUpdate();
	void checkForUpdates();
	static void setKeyInfo(QString key);

private slots:
	void checkForUpdatesFinished();

private:
    QString dirProgram;
    QString dirUpdateTemp;

};

#endif // MUPDATER_H
