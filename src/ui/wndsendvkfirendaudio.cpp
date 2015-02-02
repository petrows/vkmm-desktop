#include "wndsendvkfirendaudio.h"
#include "ui_wndsendvkfirendaudio.h"

#include "ui/wndselectvkfriends.h"
#include "ui/wndmain.h"
#include "core/core.h"
#include "core/common.h"

#include <QMessageBox>

wndSendVkFirendAudio::wndSendVkFirendAudio(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::wndSendVkFirendAudio)
{
    ui->setupUi(this);
    ui->groupMessage->hide();
    ui->wdgSelectedFriend->hide();
	ui->lblAudioSizeWarning->setVisible(false);

	ui->lstAudio->menuAllowPlay = true;
	ui->lstAudio->menuAllowRemove = true;
	ui->lstAudio->menuCreate();
    playList = new mPlayList(this);
    ui->lstAudio->setPlayList(playList);
    connect(ui->btnChangeFriend,SIGNAL(clicked()),SLOT(on_btnSelectFriend_clicked()));

	connect(ui->lblFriend, SIGNAL(linkActivated(QString)), mCore::instance()->uiWndMain, SLOT(linkClicked(QString)));

	vkUserProfile lastSelectedUser = mCore::instance()->getLastSelectedVkUser();
	if (lastSelectedUser.isValid())
	{
		setFriend(lastSelectedUser);
	}
	langButtons::setButtons(ui->buttonBox);
}

wndSendVkFirendAudio::~wndSendVkFirendAudio()
{
    delete ui;
}

void wndSendVkFirendAudio::addItem(mPlayListItem *itm)
{
    playList->addItemCopy(itm);
}

void wndSendVkFirendAudio::setFriend(vkUserProfile user)
{
	if (!user.isValid()) return;

    ui->wdgSelectedFriend->show();
    ui->btnSelectFriend->hide();
	ui->lblFriend->setText(user.getLink());
	selectedUser = user;

	setWindowTitle(tr("Отправить на стену: %1").arg(user.getName()));

	mCore::instance()->setLastSelectedVkUser(selectedUser);
}

void wndSendVkFirendAudio::show()
{
	if (playList->getItemsCount() > 10)
	{
		ui->lstAudio->setColorCut(10);
		ui->lblAudioSizeWarning->setVisible(true);
	}
	QDialog::show();
}

void wndSendVkFirendAudio::on_btnShowMessage_clicked()
{
    ui->groupMessage->show();
    ui->btnShowMessage->hide();
}

void wndSendVkFirendAudio::on_btnSelectFriend_clicked()
{
    wndSelectVkFriends * wnd = new wndSelectVkFriends(this);
    connect(wnd,SIGNAL(slectedOneUser(vkUserProfile)),SLOT(friendSelected(vkUserProfile)));
    wnd->show();
}

void wndSendVkFirendAudio::friendSelected(vkUserProfile user)
{
	setFriend(user);
}

void wndSendVkFirendAudio::postSended(qint64 userId, quint64 postId)
{
	mCore::instance()->statusMessageAudioPosted(userId, postId);
	close();
}

void wndSendVkFirendAudio::postError(int code, QString text)
{
	QMessageBox::warning(this, tr("Ошибка API ВКонтакте"),tr("(%1) %2").arg(code).arg(text));
}

void wndSendVkFirendAudio::on_buttonBox_accepted()
{
	if (!selectedUser.isValid())
	{
		QMessageBox::warning(this, tr("Ошибка"),tr("Выберите получателя!"));
		return;
	}

	if (playList->getItemsCount() == 0)
	{
		QMessageBox::warning(this, tr("Ошибка"),tr("Список аудиозаписей пуст!"));
		return;
	}

	mApiVkRequest * req = new mApiVkRequest(this);
	vkAudioList lst;
	for (int x=0; x<playList->getItemsCount(); x++)
		lst.append(playList->getItem(x)->vkRecord);
	req->sendWallAudio(selectedUser.id, lst, ui->txtMessage->toPlainText());
	connect(req,SIGNAL(sendedWallAudio(qint64,quint64)),SLOT(postSended(qint64,quint64)));
	connect(req,SIGNAL(apiError(int,QString)),SLOT(postError(int,QString)));
	mApiVk::request(req);

	setDisabled(true);
}

void wndSendVkFirendAudio::on_buttonBox_rejected()
{
	close();
}
