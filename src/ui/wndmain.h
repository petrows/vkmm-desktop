#ifndef WNDMAIN_H
#define WNDMAIN_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTreeWidgetItem>
#include <QLabel>
#include <QList>
#include "ui/wdgplaylist.h"
#include "core/mplaylist.h"
#include "core/core.h"

#define LFM_USERLIST_RECOMMENDED 1
#define LFM_USERLIST_LOVED		 2
#define LFM_USERLIST_TOP_ALL	 3
#define LFM_USERLIST_TOP_WEEK	 4
#define LFM_USERLIST_TOP_YEAR	 5
#define LFM_USERLIST_TOP_M6		 6
#define LFM_USERLIST_TOP_M3		 7

namespace Ui {
class wndMain;
}

class wndMain : public QMainWindow
{
	Q_OBJECT
	
public:
	explicit wndMain(QWidget *parent = 0);
	~wndMain();

	void setColumsWidth();
	void closeEvent(QCloseEvent *event);
	void resizeEvent(QResizeEvent *ev);

	QLabel * statusLabel;
	QLabel * statusLabelIcon;
    QLabel * statusLabelSpeed;

	QSystemTrayIcon * trayIcon;
	QAction			* trayActionPrev;
	QAction			* trayActionNext;
	QAction			* trayActionPlay;

public slots:
	void showMe();
	void hideMe();

	
private slots:
	void on_actionLogOut_triggered();
	void loginChanged(bool status);
	void loginInitDone();

	void statusMessage(mCore::statusType, QString text);
	void linkClicked(QString url);

	void trayActivated(QSystemTrayIcon::ActivationReason res);

	void playbackStarted(mPlayListItem * item);
	void playbackStopped();
	void playbackPaused(bool pause);
	void playbackProgress(double progress, int playTime, int totalTime);
	void downloadProgress(double progress, int done, int all, int speed);

	void randomChanged(mCore::randomType t);
	void repeatChanged(mCore::repeatType t);

	void volumeChanged(uint vol);

	void metaUpdated();

	void lastFmSessionReject();
	void lastFmSessionOk();
	void lastFmUserFriendsLoaded(lastFmUserList list);

	void updateView();
	void playListCreated(mPlayList * newList);
	void playListRemoved(uint index);
	void sldMoved(uint pos);

	void filterVkFriends(QString text);
	void filterVkGroups(QString text);

	void playListWidgetFilterEntered(QString text);

	void vkFriendsListSelected(QTreeWidgetItem* itm,int);
	void vkGroupsListSelected(QTreeWidgetItem* itm,int);

	void on_actionProfile_triggered();
	void on_btnVKUserSearch_clicked();
	void on_vkFriendsListWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

	void on_btnVkUserMineList_clicked();
	void on_btnPlayPause_clicked();
	void on_chRandom_toggled(bool checked);
	void on_chRepeatOne_toggled(bool checked);
	void on_btnPlayNext_clicked();
	void on_btnPlayPrev_clicked();

	void on_sldVolume_valueChanged(int value);

	void on_vkGroupsListWidget_itemDoubleClicked(QTreeWidgetItem * item, int );

	void on_btnVKGroupSearch_clicked();

	void on_actionSettings_triggered();

	void on_btnVkSearch_clicked();

	void on_actionExit_triggered();

	void on_edtPlayListFilter_textChanged(const QString &arg1);

	void on_btnFilterClose_clicked();

	void on_edtPlayListFilter_lostFocus();

	void on_tabsPlayLists_currentChanged(int index);

	void on_edtVkFriendsFilter_textChanged(const QString &arg1);

	void on_btnVkFriendsFilter_clicked();

	void on_edtVkGroupsFilter_textChanged(const QString &arg1);

	void on_btnVkGroupsFilter_clicked();

	void on_tabsPlayLists_tabCloseRequested(int index);

	void on_actionHide_triggered();

	void on_actionAbout_triggered();

	void on_btnLoved_clicked();

	void on_lfmUserLoad_clicked();

	void on_lfmUserFriendsList_itemClicked(QTreeWidgetItem *item, int column);

	void on_lfmUserFriendsList_itemDoubleClicked(QTreeWidgetItem *item, int column);

	void on_toolButton_clicked();

private:
	Ui::wndMain *ui;
	QList<wdgPlayList*> playlists;
};

#endif // WNDMAIN_H
