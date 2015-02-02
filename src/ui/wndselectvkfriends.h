#ifndef WNDSELECTVKFRIENDS_H
#define WNDSELECTVKFRIENDS_H

#include <QDialog>
#include <QTreeWidgetItem>
#include "core/mapivk.h"

namespace Ui {
class wndSelectVkFriends;
}

class wndSelectVkFriends : public QDialog
{
    Q_OBJECT
    
public:
    explicit wndSelectVkFriends(QWidget *parent = 0);
    ~wndSelectVkFriends();

signals:
    void slectedOneUser(vkUserProfile user);
    
private slots:
    void on_vkFriendsListWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);
	void on_edtVkFriendsFilter_textChanged(const QString &arg1);
	void on_btnVkFriendsFilter_clicked();

	void makeFriendsList();

	void on_buttonBox_accepted();

private:
    Ui::wndSelectVkFriends *ui;
	QString filterText;
};

#endif // WNDSELECTVKFRIENDS_H
