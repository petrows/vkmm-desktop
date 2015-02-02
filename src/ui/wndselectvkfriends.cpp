#include "wndselectvkfriends.h"
#include "ui_wndselectvkfriends.h"

#include "core/core.h"
#include "core/common.h"

wndSelectVkFriends::wndSelectVkFriends(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::wndSelectVkFriends)
{
    ui->setupUi(this);
	makeFriendsList();
	langButtons::setButtons(ui->buttonBox);
}

wndSelectVkFriends::~wndSelectVkFriends()
{
    delete ui;
}

void wndSelectVkFriends::on_vkFriendsListWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    uint uid = item->data(0,Qt::UserRole).toUInt();
	vkUserProfile user = mCore::instance()->userFriendById(uid);
	if (user.isValid())
	{
		emit slectedOneUser(user);
		close();
		return;
	}
}

void wndSelectVkFriends::on_edtVkFriendsFilter_textChanged(const QString &arg1)
{
	filterText = arg1;
	makeFriendsList();
}

void wndSelectVkFriends::on_btnVkFriendsFilter_clicked()
{
	ui->edtVkFriendsFilter->setText("");
}

void wndSelectVkFriends::makeFriendsList()
{
	// Fill friends list...
	ui->vkFriendsListWidget->clear();
	ui->vkFriendsListWidget->setColumnCount(2);
	ui->vkFriendsListWidget->setColumnWidth(0,20);
	ui->vkFriendsListWidget->setAlternatingRowColors(true);

	QString origTitle;
	QStringList searchWords = filterText.split(QRegExp("\\s+"),QString::SkipEmptyParts);

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
		item->setText(1,user.name_f + " " + QString(user.name_n + " " + user.name_l).trimmed());
		item->setData(0,Qt::UserRole,user.id);
		ui->vkFriendsListWidget->addTopLevelItem(item);
	}

}

void wndSelectVkFriends::on_buttonBox_accepted()
{
	QList<QTreeWidgetItem*> items = ui->vkFriendsListWidget->selectedItems();
	if (!items.size()) return;
	uint uid = items.at(0)->data(0,Qt::UserRole).toUInt();
	vkUserProfile user = mCore::instance()->userFriendById(uid);
	if (user.isValid())
	{
		emit slectedOneUser(user);
	}
}
