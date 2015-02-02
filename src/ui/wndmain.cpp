#include "wndmain.h"
#include "ui_wndmain.h"

#include "core/core.h"
#include "core/mapivk.h"
#include "core/mplayer.h"

#include "core/common.h"

#include "ui/wndsettings.h"
#include "ui/wndabout.h"
#include "ui/wndupdate.h"

#include <QMessageBox>
#include <QDesktopServices>
#include <QTime>
#include <QCloseEvent>
#include <QDebug>

#include <math.h>

wndMain::wndMain(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::wndMain)
{
	ui->setupUi(this);

	connect(mCore::instance(),SIGNAL(statusMessageChanged(mCore::statusType,QString)),SLOT(statusMessage(mCore::statusType,QString)));
	connect(mCore::instance(),SIGNAL(loginChanged(bool)),SLOT(loginChanged(bool)));
	connect(mCore::instance(),SIGNAL(loginInitDone()),SLOT(loginInitDone()));

	connect(mCore::instance(),SIGNAL(playListAdded(mPlayList*)),SLOT(playListCreated(mPlayList*)));
	connect(mCore::instance(),SIGNAL(playListRemoved(uint)),SLOT(playListRemoved(uint)));

	connect(ui->vkFriendsListWidget,SIGNAL(itemClicked(QTreeWidgetItem*,int)),SLOT(vkFriendsListSelected(QTreeWidgetItem*,int)));
	connect(ui->vkGroupsListWidget,SIGNAL(itemClicked(QTreeWidgetItem*,int)),SLOT(vkGroupsListSelected(QTreeWidgetItem*,int)));

	connect(mCore::instance(),SIGNAL(randomChanged(mCore::randomType)),SLOT(randomChanged(mCore::randomType)));
	connect(mCore::instance(),SIGNAL(repeatChanged(mCore::repeatType)),SLOT(repeatChanged(mCore::repeatType)));
	connect(mCore::instance(),SIGNAL(volumeChanged(uint)),SLOT(volumeChanged(uint)));
	connect(mCore::instance(),SIGNAL(metaUpdated()),SLOT(metaUpdated()));
	connect(mCore::instance(),SIGNAL(lastFmSessionOk()),SLOT(lastFmSessionOk()));
	connect(mCore::instance(),SIGNAL(lastFmSessionReject()),SLOT(lastFmSessionReject()));

	connect(mPlayer::instance(),SIGNAL(downloadProgress(double,int,int,int)),SLOT(downloadProgress(double,int,int,int)));
	connect(mPlayer::instance(),SIGNAL(playbackProgress(double,int,int)),SLOT(playbackProgress(double,int,int)));
	connect(mPlayer::instance(),SIGNAL(playbackStarted(mPlayListItem*)),SLOT(playbackStarted(mPlayListItem*)));
	connect(mPlayer::instance(),SIGNAL(playbackStopped()),SLOT(playbackStopped()));
	connect(mPlayer::instance(),SIGNAL(playbackPaused(bool)),SLOT(playbackPaused(bool)));

	connect(ui->edtVKUserSearch,SIGNAL(returnPressed()),ui->btnVKUserSearch,SIGNAL(clicked()));
	connect(ui->edtVKGroupSearch,SIGNAL(returnPressed()),ui->btnVKGroupSearch,SIGNAL(clicked()));
	connect(ui->edtVkSearch,SIGNAL(returnPressed()),ui->btnVkSearch,SIGNAL(clicked()));

	ui->tabsMode->setCurrentIndex(0);
	ui->tabsModeVk->setCurrentIndex(0);
	ui->tabsModesLastfm->setCurrentIndex(0);
	// ui->dockPlayList->resize(500,0);

	//ui->lfmUserType->addItem(tr("Рекомендации"), LFM_USERLIST_RECOMMENDED);
	ui->lfmUserType->addItem(tr("Любимое"), LFM_USERLIST_LOVED);
	ui->lfmUserType->addItem(tr("Топ: всё время"), LFM_USERLIST_TOP_ALL);
	ui->lfmUserType->addItem(tr("Топ: год"), LFM_USERLIST_TOP_YEAR);
	ui->lfmUserType->addItem(tr("Топ: 6 месяцев"), LFM_USERLIST_TOP_M6);
	ui->lfmUserType->addItem(tr("Топ: 3 месяца"), LFM_USERLIST_TOP_M3);
	ui->lfmUserName->setText(mCore::instance()->settings->value("lastfm/login", "").toString());
	ui->lfmUserFriendsList->hide();
	ui->lfmUserFriendsList->setColumnWidth(0, 200);

	connect(ui->sldPlayProgress,SIGNAL(positionChange(uint)),SLOT(sldMoved(uint)));

	setWindowTitle("VKMM");

	QAction * vkBtnAction;
	vkBtnAction = new QAction(QIcon(":/icons/vk-plus.png"),tr("В мои Аудиозаписи"), this);
	connect(vkBtnAction,SIGNAL(triggered()),mCore::instance(),SLOT(addCurrentToVkAudio()));
	ui->btnAddVkAudio->addAction(vkBtnAction);
	//vkBtnAction = new QAction(QIcon(":/icons/vk-plus.png"),tr("Отправить в Новости"), this);
	//ui->btnAddVkAudio->addAction(vkBtnAction);
	vkBtnAction = new QAction(QIcon(":/icons/vk-tick.png"),tr("Отправить Другу"), this);
    connect(vkBtnAction,SIGNAL(triggered()),mCore::instance(),SLOT(addCurrentToVkFriend()));
	ui->btnAddVkAudio->addAction(vkBtnAction);
	connect(ui->btnAddVkAudio,SIGNAL(clicked()),mCore::instance(),SLOT(addCurrentToVkAudio()));

	ui->dockPlayer->setTitleBarWidget(new QWidget(this));
    ui->dockPlayer->setFixedHeight(88);
	ui->dockMode->setTitleBarWidget(new QWidget(this));

    ui->btnPlayNext->setProperty("button-play", 1);
    ui->btnPlayPause->setProperty("button-play", 1);
	ui->btnPlayPrev->setProperty("button-play", true);

	trayIcon = new QSystemTrayIcon(this);
	trayIcon->setIcon(QIcon(":/icons/vkmm-32.png"));

	QAction * trayAction;
	QMenu * trayActionMenu = new QMenu();
	trayIcon->setContextMenu(trayActionMenu);

	trayAction = new QAction(QIcon(":/icons/vkmm-32.png"),tr("Показать окно"),trayIcon);
	connect(trayAction,SIGNAL(triggered()),SLOT(showMe()));
	trayIcon->contextMenu()->addAction(trayAction);
	//trayAction = new QAction(QIcon(":/icons/cross.png"),tr("Выход"),trayIcon);
	//connect(trayAction,SIGNAL(triggered()),SLOT(on_actionExit_triggered()));
	trayAction = new QAction("",trayIcon);
	trayAction->setSeparator(true);
	trayIcon->contextMenu()->addAction(trayAction);

	trayAction = new QAction(QIcon(":/icons/control-double-180.png"),tr("Предыдущий трек"),trayIcon);
	trayActionPrev = trayAction;
	trayActionPrev->setDisabled(true);
	connect(trayActionPrev,SIGNAL(triggered()),SLOT(on_btnPlayPrev_clicked()));
	trayIcon->contextMenu()->addAction(trayAction);

	trayAction = new QAction(QIcon(":/icons/control.png"),tr("Пауза / Воспроизведение"),trayIcon);
	trayActionPlay = trayAction;
	trayActionPlay->setDisabled(true);
	connect(trayActionPlay,SIGNAL(triggered()),SLOT(on_btnPlayPause_clicked()));
	trayIcon->contextMenu()->addAction(trayAction);

	trayAction = new QAction(QIcon(":/icons/control-double.png"),tr("Следущий трек"),trayIcon);
	trayActionNext = trayAction;
	trayActionNext->setDisabled(true);
	connect(trayActionNext,SIGNAL(triggered()),SLOT(on_btnPlayNext_clicked()));
	trayIcon->contextMenu()->addAction(trayAction);

	trayAction = new QAction("",trayIcon);
	trayAction->setSeparator(true);
	trayIcon->contextMenu()->addAction(trayAction);
	trayIcon->contextMenu()->addAction(ui->actionExit);

	trayIcon->show();
	connect(trayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),SLOT(trayActivated(QSystemTrayIcon::ActivationReason)));

	ui->playListFilterFrame->hide();

	QWidget * statusLayoutWidget = new QWidget(this);
	statusLayoutWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	QHBoxLayout * statusLayout = new QHBoxLayout(statusLayoutWidget);
	statusLayout->setSpacing(0);
	statusLayout->setContentsMargins(0,0,0,0);

	statusLabelIcon = new QLabel("");
	statusLabelIcon->setPixmap(QPixmap(""));
	statusLabelIcon->setMinimumWidth(16);
	statusLabelIcon->setContentsMargins(3,1,0,1);
	statusLayout->addWidget(statusLabelIcon);
	statusLabel = new QLabel("");
	statusLabel->setContentsMargins(5,1,0,1);
	statusLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	statusLayout->addWidget(statusLabel);
	connect(statusLabel,SIGNAL(linkActivated(QString)),SLOT(linkClicked(QString)));

	statusLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

	statusBar()->addWidget(statusLayoutWidget, 100);

    statusLabelSpeed = new QLabel("");
    statusLabelSpeed->setContentsMargins(3,3,3,3);
	statusBar()->addWidget(statusLabelSpeed);

	// Fill vk load search pages
	for (int x=1, pn=0; x<=20; x++)
	{
		pn = x;
		QString cmbText = tr("%1 (%2)").arg(pn*vkSearchPageSize).arg(pn);
		ui->cmbVkSearchCount->addItem(cmbText, QVariant(pn));
	}
	ui->cmbVkSearchCount->setCurrentIndex(mCore::instance()->settings->value("vkSearchCount",4).toUInt());

	ui->cmbVkSearchSort->addItem(tr("Популярность"), (int)VK_AUDIO_SORT_POPULAR);
	ui->cmbVkSearchSort->addItem(tr("Длительность"), (int)VK_AUDIO_SORT_LENGTH);
	ui->cmbVkSearchSort->addItem(tr("Дата размещения"), (int)VK_AUDIO_SORT_DATE);

	int cfgSearchType = mCore::instance()->settings->value("vkSearchSort",(int)VK_AUDIO_SORT_POPULAR).toInt();
	for (int x=0; x<ui->cmbVkSearchSort->count(); x++)
	{
		int itemId = ui->cmbVkSearchSort->itemData(x).toInt();
		if (itemId == cfgSearchType)
		{
			ui->cmbVkSearchSort->setCurrentIndex(x);
			break;
		}
	}
	updateView();
	update();
	updateGeometry();
}

wndMain::~wndMain()
{
	delete ui;
	trayIcon->hide();
	delete trayIcon;
}

void wndMain::setColumsWidth()
{
	//int w = width();
	//ui->dockWidgetContents->setBaseSize(w/2,10);
	//ui->dockPlayList->setBaseSize(w/2,0);
}

void wndMain::closeEvent(QCloseEvent *event)
{
	hideMe();
	event->ignore();
}

void wndMain::resizeEvent(QResizeEvent * ev)
{
	QMainWindow::resizeEvent(ev);
	/*
	static QSize oldWnd = size();
	static QSize oldPls = ui->dockPlayList->size();



	// Resize play-list to proposional-move
	double plsProp = (double)oldPls.width() / (double)oldWnd.width();
	// ui->dockPlayList->resize(plsProp * (double)size().width(), ui->dockPlayList->size().height());

	// layout()->;
	QMainWindowLayout* mwl = (QMainWindowLayout*)layout();
	if (mwl)
	{
		QDockAreaLayout& dal = mwl->layoutState.dockAreaLayout;
		dal.docks[QInternal::BottomDock].rect = QRect(0, 0, 0, 100);
	}

	oldWnd = size();
	oldPls = ui->dockPlayList->size();*/
}

void wndMain::on_actionLogOut_triggered()
{
	mCore::instance()->logOut();
}

void wndMain::loginChanged(bool status)
{
	ui->vkFriendsListWidget->clear();
    ui->centralwidget->setEnabled(status);

	if (status)
	{

	} else {
		ui->menuUser->setTitle("-");
		ui->menuUser->setDisabled(true);
	}
}

void wndMain::loginInitDone()
{
	// Set user
	ui->menuUser->setTitle(mCore::instance()->user.name_f + " " + mCore::instance()->user.name_l);
	ui->menuUser->setDisabled(false);

	ui->edtVkFriendsFilter->setText(""); // Emit filter/list fill
	filterVkFriends("");
	ui->edtVkGroupsFilter->setText(""); // Emit filter/list fill
	filterVkGroups("");
}

void wndMain::statusMessage(mCore::statusType status, QString text)
{
	statusLabel->setText(text);
	statusLabelIcon->setPixmap(QPixmap(""));
	if (mCore::STATUS_OK == status)
		statusLabelIcon->setPixmap(QPixmap(":/icons/tick.png"));
	if (mCore::STATUS_FAIL == status)
		statusLabelIcon->setPixmap(QPixmap(":/icons/cross.png"));
	if (mCore::STATUS_VK == status)
        statusLabelIcon->setPixmap(QPixmap(":/icons/vk.png"));
	if (mCore::STATUS_LASTFM == status)
		statusLabelIcon->setPixmap(QPixmap(":/icons/lastfm.png"));
}

void wndMain::linkClicked(QString url)
{
	QDesktopServices::openUrl(QUrl(url));
}

void wndMain::showMe()
{
	showNormal();
	setFocus();
	activateWindow();
}

void wndMain::hideMe()
{
	if (!mCore::instance()->settings->value("no-ask-window/trayFirstMessage", false).toBool())
	{
		trayIcon->showMessage(tr("Vkontakte Music Mania"),tr("Программа свёрнута и продолжает работу в фоне.\nДля открытия главного окна щелкните по этой иконке."));
	}
	mCore::instance()->settings->setValue("no-ask-window/trayFirstMessage", true);
	hide();
}

void wndMain::trayActivated(QSystemTrayIcon::ActivationReason res)
{
	if (QSystemTrayIcon::Trigger == res)
	{
        if (isVisible())
			hideMe();
		else
			showMe();
	}
}

void wndMain::playbackStarted(mPlayListItem *item)
{
	playbackPaused(false);
	ui->sldPlayProgress->setEnabled(true);
	ui->btnPlayPause->setEnabled(true);
	trayActionPlay->setEnabled(true);
	if (NULL != item->playList)
	{
		ui->btnPlayNext->setEnabled(true);
		trayActionNext->setEnabled(true);
	} else {
		ui->btnPlayNext->setDisabled(true);
		trayActionNext->setDisabled(true);
	}
	if (mCore::instance()->isPrevAvaliable())
	{
		ui->btnPlayPrev->setEnabled(true);
		trayActionPrev->setEnabled(true);
	} else {
		ui->btnPlayPrev->setDisabled(true);
		trayActionPrev->setDisabled(true);
	}
	ui->lblPlayTitle->setText("<b>" + item->getAudioArtist() + "</b> - " + item->getAudioTitle());
	setWindowTitle(item->getAudioArtist() + " - " + item->getAudioTitle() + " / VKMM");
	ui->sldPlayProgress->resetTrack();
	ui->btnAddVkAudio->setDisabled(false);

	metaUpdated();
}

void wndMain::playbackStopped()
{
	// Disable play/pause button
	ui->btnAddVkAudio->setDisabled(true);
	ui->btnLoved->setChecked(false);
	ui->btnLoved->setDisabled(true);
	ui->btnPlayPause->setDisabled(true);
	trayActionPlay->setDisabled(true);
	ui->btnPlayNext->setDisabled(true);
	trayActionNext->setDisabled(true);
	ui->btnPlayPrev->setDisabled(true);
	trayActionPrev->setDisabled(true);
	ui->sldPlayProgress->resetTrack();
	ui->sldPlayProgress->setDisabled(true);
	ui->lblPlayTitle->setText("");
	ui->lblPlayTime->setText("");
	setWindowTitle("VKMM");
	update();

	metaUpdated();
}

void wndMain::playbackPaused(bool pause)
{
	if (pause)
	{
		ui->btnPlayPause->setIcon(QIcon(":/icons/control.png"));
		trayActionPlay->setIcon(QIcon(":/icons/control.png"));
	} else {
		ui->btnPlayPause->setIcon(QIcon(":/icons/control-pause.png"));
		trayActionPlay->setIcon(QIcon(":/icons/control-pause.png"));
	}
}

void wndMain::updateView()
{
	// Playlists count...
	if (playlists.count() == 0)
	{
		// No playlists, show info unstead
		ui->lblNoPlayLists->show();
		ui->tabsPlayLists->hide();
	} else {
		ui->lblNoPlayLists->hide();
		ui->tabsPlayLists->show();

		for (int x=0; x<mCore::instance()->lists.size(); x++)
		{
			if (x >= ui->tabsPlayLists->count()) break;
			QString tt = mCore::instance()->lists.at(x)->getTitle();
			if (tt.length() > 25) tt = tt.left(20) + "...";
			ui->tabsPlayLists->setTabText(x, tt);
			ui->tabsPlayLists->setTabIcon(x, mCore::instance()->lists.at(x)->getIcon());
		}
	}
}

void wndMain::playListCreated(mPlayList *newList)
{
	// New playlist!
	connect(newList,SIGNAL(stateChanged(mPlayList::playListLoadState)),SLOT(updateView()));
	wdgPlayList * list = new wdgPlayList(centralWidget());
	list->show();
	playlists.push_back(list);
	list->setPlayList(newList);
	list->menuAllowPlay = true;
	list->menuAllowSave = true; // TODO: do not allow 'SAVE' if this is a user own playlist
	list->menuAllowSend = true;
	list->menuAllowRemove = true;
	list->menuCreate();
	connect(list,SIGNAL(textEntered(QString)),SLOT(playListWidgetFilterEntered(QString)));
	QIcon tabIcon;
	if (mPlayList::FROM_VK_USER == newList->origin || mPlayList::FROM_VK_GROUP == newList->origin)
	{
		tabIcon = QIcon(":/icons/vk.png");
	}
	QString tabTitle = newList->getTitle();
	if (tabTitle.length() > 12)
		tabTitle = tabTitle.left(10).trimmed() + "..";
	int tab_index = ui->tabsPlayLists->addTab(list, tabIcon, tabTitle);
	ui->tabsPlayLists->setCurrentIndex(tab_index);
	updateView();
}

void wndMain::playListRemoved(uint index)
{
	ui->tabsPlayLists->removeTab(index);
	playlists.at(index)->deleteLater();
	playlists.removeAt(index);
	updateView();
}

void wndMain::sldMoved(uint pos)
{
	mPlayer::instance()->setPos(pos);
}

void wndMain::filterVkFriends(QString text)
{
	// Fill friends list...
	ui->vkFriendsListWidget->clear();
	ui->vkFriendsListWidget->setColumnWidth(0,20);
	ui->vkFriendsListWidget->setAlternatingRowColors(true);

	QString origTitle;
	QStringList searchWords = text.split(QRegExp("\\s+"),QString::SkipEmptyParts);

	for (int x=0; x<mCore::instance()->userFriends.size(); x++)
	{
		vkUserProfile user = mCore::instance()->userFriends.at(x);
		if (!user.screen_name.length()) continue;

		origTitle = user.getName().trimmed();
		bool match = true;

		if (searchWords.size() > 0)
		{
			for (int z=0; z<searchWords.size(); z++)
			{
				if (!origTitle.contains(searchWords.at(z),Qt::CaseInsensitive))
				{
					match = false;
					break;
				}
			}
		}
		if (!match)
		{
			continue;
		}

		QTreeWidgetItem * item = new QTreeWidgetItem(ui->vkFriendsListWidget);
		item->setIcon(0,user.getIcon());
		item->setText(1,origTitle);
		item->setData(0,Qt::UserRole,user.id);
		ui->vkFriendsListWidget->addTopLevelItem(item);
	}
	ui->vkFriendsListWidget->setSortingEnabled(true);
	ui->vkFriendsListWidget->sortByColumn(0,Qt::AscendingOrder);
}

void wndMain::filterVkGroups(QString text)
{
	ui->vkGroupsListWidget->clear();
	ui->vkGroupsListWidget->setAlternatingRowColors(true);
	ui->vkGroupsListWidget->setColumnCount(2);
	ui->vkGroupsListWidget->setColumnWidth(0,20);

	QStringList searchWords = text.split(QRegExp("\\s+"),QString::SkipEmptyParts);

	for (int x=0; x<mCore::instance()->userGroups.size(); x++)
	{
		vkGroup group = mCore::instance()->userGroups.at(x);

		bool match = true;
		if (searchWords.size() > 0)
		{
			for (int z=0; z<searchWords.size(); z++)
			{
				if (!group.title.contains(searchWords.at(z),Qt::CaseInsensitive))
				{
					match = false;
					break;
				}
			}
		}
		if (!match)
		{
			continue;
		}

		QTreeWidgetItem * item = new QTreeWidgetItem(ui->vkGroupsListWidget);
		item->setText(1, group.title);
		item->setIcon(0,QIcon(":/icons/users.png"));
		item->setData(0,Qt::UserRole,group.gid);
		ui->vkGroupsListWidget->addTopLevelItem(item);
	}

}

void wndMain::playListWidgetFilterEntered(QString text)
{
	ui->playListFilterFrame->show();
	ui->edtPlayListFilter->setText(text);
	ui->edtPlayListFilter->setFocus();
}

void wndMain::vkFriendsListSelected(QTreeWidgetItem * itm, int)
{
	ui->edtVKUserSearch->setText(itm->data(0,Qt::UserRole).toString());
}

void wndMain::vkGroupsListSelected(QTreeWidgetItem *itm, int)
{
	ui->edtVKGroupSearch->setText(itm->data(0,Qt::UserRole).toString());
}

void wndMain::on_actionProfile_triggered()
{
	QDesktopServices::openUrl(QUrl("http://vk.com/" + mCore::instance()->user.screen_name));
}

void wndMain::on_actionSettings_triggered()
{
	wndSettings * wnd = new wndSettings(this);
	wnd->show();
}

void wndMain::on_btnVKUserSearch_clicked()
{
	// Start download user audio!
	mCore::instance()->createPlaylistFromVkUser(ui->edtVKUserSearch->text().toULongLong());
}

void wndMain::on_vkFriendsListWidget_itemDoubleClicked(QTreeWidgetItem *item, int)
{
	// Get frined info
	ui->edtVKUserSearch->setText(item->data(0,Qt::UserRole).toString());
	on_btnVKUserSearch_clicked();
}

void wndMain::on_btnVkUserMineList_clicked()
{
	ui->edtVKUserSearch->setText(QString::number(mCore::instance()->user.id));
	on_btnVKUserSearch_clicked();
}

void wndMain::playbackProgress(double progress, int playTime, int totalTime)
{
	Q_UNUSED(progress);
	ui->sldPlayProgress->setLength(totalTime);
	ui->sldPlayProgress->setPlayPos(playTime);
	ui->lblPlayTime->setText(QTime(0,0,0).addSecs(playTime).toString("mm:ss") + " / " + QTime(0,0,0).addSecs(totalTime).toString("mm:ss"));
}

void wndMain::downloadProgress(double /*progress*/, int done, int all, int speed)
{
	ui->sldPlayProgress->setSize(all);
	ui->sldPlayProgress->setDownloadPos(0,done);
    if (speed < 0)
    {
        statusLabelSpeed->setText("");
    } else {
        statusLabelSpeed->setText(formatSize(speed,"%.0f {S}")+"/с");
    }
}

void wndMain::randomChanged(mCore::randomType t)
{
	ui->chRandom->blockSignals(true);
	ui->chRandom->setChecked(mCore::RANDOM_ON == t);
	ui->chRandom->blockSignals(false);
}

void wndMain::repeatChanged(mCore::repeatType t)
{
	ui->chRepeatOne->blockSignals(true);
	ui->chRepeatOne->setChecked(mCore::REPEAT_TRACK == t);
	ui->chRepeatOne->blockSignals(false);
}

void wndMain::volumeChanged(uint vol)
{
	ui->sldVolume->blockSignals(true);
	ui->sldVolume->setValue(vol);
	ui->sldVolume->blockSignals(false);
}

void wndMain::metaUpdated()
{
	if (mApiLastfm::instance()->isActive() && mCore::instance()->isLastfmLovedLoaded)
	{
		mPlayListItem * itm = mPlayer::instance()->getCurrentItem();
		if (NULL != itm)
		{
			ui->btnLoved->setEnabled(true);
			ui->btnLoved->setChecked(itm->isLovedLastFm);
		} else {
			ui->btnLoved->setChecked(false);
			ui->btnLoved->setEnabled(false);
		}
	} else {
		ui->btnLoved->setEnabled(false);
	}
}

void wndMain::lastFmSessionReject()
{
	ui->lfmUserFriendsList->hide();
	ui->lfmUserFriendsStatus->show();
	ui->lfmUserFriendsStatus->setText(tr("Пользователь Last.FM не настроен"));
}

void wndMain::lastFmSessionOk()
{
	// Load friends list
	ui->lfmUserFriendsList->clear();

	mApiLastfmRequest * req = new mApiLastfmRequest();
	req->userFriends(mCore::instance()->settings->value("lastfm/login", "").toString());
	connect(req, SIGNAL(userFriendsLoaded(lastFmUserList)),SLOT(lastFmUserFriendsLoaded(lastFmUserList)));
	mApiLastfm::request(req);

	if(!ui->lfmUserName->text().isEmpty()) ui->lfmUserName->setText(mCore::instance()->settings->value("lastfm/login", "").toString());
}

void wndMain::lastFmUserFriendsLoaded(lastFmUserList list)
{
	if (list.size() == 0)
	{
		ui->lfmUserFriendsStatus->show();
		ui->lfmUserFriendsStatus->setText(tr("У вас нет друзей"));
		ui->lfmUserFriendsList->hide();
		return;
	}

	ui->lfmUserFriendsStatus->hide();
	ui->lfmUserFriendsList->show();
	ui->lfmUserFriendsList->clear();
	ui->lfmUserFriendsList->setColumnWidth(0, 20);
	for (int x=0; x<list.size(); x++)
	{
		QTreeWidgetItem * itm = new QTreeWidgetItem();
		itm->setIcon(0, QIcon(":/icons/user-red.png"));
		itm->setData(0, Qt::UserRole, list.at(x).name);
		itm->setText(1, list.at(x).name);
		ui->lfmUserFriendsList->addTopLevelItem(itm);
	}
}

void wndMain::on_btnPlayPause_clicked()
{
	mPlayer::instance()->pause();
}

void wndMain::on_chRandom_toggled(bool checked)
{
	mCore::instance()->setRandom(checked?mCore::RANDOM_ON:mCore::RANDOM_OFF);
}

void wndMain::on_chRepeatOne_toggled(bool checked)
{
	mCore::instance()->setRepeat(checked?mCore::REPEAT_TRACK:mCore::REPEAT_LIST);
}

void wndMain::on_btnPlayNext_clicked()
{
	mCore::instance()->playNext();
}

void wndMain::on_btnPlayPrev_clicked()
{
	mCore::instance()->playPrev();
}

void wndMain::on_sldVolume_valueChanged(int value)
{
	mCore::instance()->setVolume(value);
}

void wndMain::on_vkGroupsListWidget_itemDoubleClicked(QTreeWidgetItem * item, int)
{
	// Start download group audio!
	ui->edtVKGroupSearch->setText(item->data(0,Qt::UserRole).toString());
	on_btnVKGroupSearch_clicked();
}

void wndMain::on_btnVKGroupSearch_clicked()
{
	mCore::instance()->createPlaylistFromVkGroup(ui->edtVKGroupSearch->text().toULongLong());
}

void wndMain::on_btnVkSearch_clicked()
{
	mCore::instance()->settings->setValue("vkSearchCount", ui->cmbVkSearchCount->currentIndex());
	mCore::instance()->settings->setValue("vkSearchSort", ui->cmbVkSearchSort->itemData(ui->cmbVkSearchSort->currentIndex()).toUInt());

	int searchCount = ui->cmbVkSearchCount->itemData(ui->cmbVkSearchCount->currentIndex()).toInt();
	int searchSort = ui->cmbVkSearchSort->itemData(ui->cmbVkSearchSort->currentIndex()).toInt();

	QString text = ui->edtVkSearch->text().trimmed();
	if (text.length() < 3)
	{
		QMessageBox::warning(this, tr("Короткий запрос"), tr("Слишком короткий поисковый запрос, введите хотя бы 3 символа для поиска!"));
		return;
	}

	mCore::instance()->createPlaylistFromVkSearch(text, (vkAudioSort)searchSort, ui->chVkSearchMisspell->isChecked(), ui->chVkSearchExact->isChecked(), searchCount);
}

void wndMain::on_actionExit_triggered()
{
	QApplication::exit(0);
}

void wndMain::on_edtPlayListFilter_textChanged(const QString &arg1)
{
	// Send filter command
	mCore::instance()->setPlayListFilter(ui->tabsPlayLists->currentIndex(), arg1);
}

void wndMain::on_btnFilterClose_clicked()
{
	ui->edtPlayListFilter->setText("");
	ui->edtPlayListFilter->clearFocus();
	ui->playListFilterFrame->hide();
}

void wndMain::on_edtPlayListFilter_lostFocus()
{
	if (ui->edtPlayListFilter->text().trimmed().length() == 0)
	{
		ui->playListFilterFrame->hide();
	}
}

void wndMain::on_tabsPlayLists_currentChanged(int index)
{
	QString filterText = mCore::instance()->getPlayListFilter(index);
	if (filterText.isEmpty())
	{
		ui->playListFilterFrame->hide();
	} else {
		ui->edtPlayListFilter->blockSignals(true);
		ui->edtPlayListFilter->setText(filterText);
		ui->edtPlayListFilter->blockSignals(false);
		ui->playListFilterFrame->show();
	}
}

void wndMain::on_edtVkFriendsFilter_textChanged(const QString &arg1)
{
	filterVkFriends(arg1);
}

void wndMain::on_btnVkFriendsFilter_clicked()
{
	ui->edtVkFriendsFilter->setText("");
}

void wndMain::on_edtVkGroupsFilter_textChanged(const QString &arg1)
{
	filterVkGroups(arg1);
}

void wndMain::on_btnVkGroupsFilter_clicked()
{
	ui->edtVkGroupsFilter->setText("");
}

void wndMain::on_tabsPlayLists_tabCloseRequested(int index)
{
	mCore::instance()->removePlayList(index);
}

void wndMain::on_actionHide_triggered()
{
	hideMe();
}

void wndMain::on_actionAbout_triggered()
{
	wndAbout * wnd = new wndAbout(this);
	wnd->show();
}

void wndMain::on_btnLoved_clicked()
{
	mPlayListItem * itm = mPlayer::instance()->getCurrentItem();
	if (NULL != itm)
	{
		mCore::instance()->lovedToggle(itm, !itm->isLovedLastFm);
	}
}

void wndMain::on_lfmUserLoad_clicked()
{
	// Select method...
	QString user     = ui->lfmUserName->text().trimmed();
	int     limit    = ui->lfmUserSize->value();
	int     userType = ui->lfmUserType->itemData(ui->lfmUserType->currentIndex()).toInt();
	switch (userType)
	{
		case LFM_USERLIST_LOVED:
			mCore::instance()->createPlayListFromLastfmUserLoved(user, limit);
			break;
		case LFM_USERLIST_TOP_ALL:
			mCore::instance()->createPlayListFromLastfmUserTop(user, mApiLastfmRequest::PERIOD_ALL, limit);
			break;
		case LFM_USERLIST_TOP_YEAR:
			mCore::instance()->createPlayListFromLastfmUserTop(user, mApiLastfmRequest::PERIOD_MONTH_12, limit);
			break;
		case LFM_USERLIST_TOP_M6:
			mCore::instance()->createPlayListFromLastfmUserTop(user, mApiLastfmRequest::PERIOD_MONTH_6, limit);
			break;
		case LFM_USERLIST_TOP_M3:
			mCore::instance()->createPlayListFromLastfmUserTop(user, mApiLastfmRequest::PERIOD_MONTH_3, limit);
			break;
		case LFM_USERLIST_TOP_WEEK:
			mCore::instance()->createPlayListFromLastfmUserTop(user, mApiLastfmRequest::PERIOD_WEEK, limit);
			break;
		default:
			break;
	}
}

void wndMain::on_lfmUserFriendsList_itemClicked(QTreeWidgetItem *item, int column)
{
	ui->lfmUserName->setText(item->data(0,Qt::UserRole).toString());
}

void wndMain::on_lfmUserFriendsList_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
	ui->lfmUserName->setText(item->data(0,Qt::UserRole).toString());
	on_lfmUserLoad_clicked();
}

void wndMain::on_toolButton_clicked()
{
	mCore::instance()->createPlayListFromLastfmArtistTop(ui->lfmArtistName->text(), "", ui->lfmArtistSize->value());
}
