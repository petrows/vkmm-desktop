#ifndef WNDSENDVKFIRENDAUDIO_H
#define WNDSENDVKFIRENDAUDIO_H

#include <QDialog>
#include "core/mplaylist.h"
#include "core/mapivk.h"

namespace Ui {
class wndSendVkFirendAudio;
}

class wndSendVkFirendAudio : public QDialog
{
    Q_OBJECT
    
public:
    explicit wndSendVkFirendAudio(QWidget *parent = 0);
    ~wndSendVkFirendAudio();

    void addItem(mPlayListItem * itm);
    void setFriend(vkUserProfile user);

public slots:
	void show();
    
private slots:
    void on_btnShowMessage_clicked();
    void on_btnSelectFriend_clicked();
    void friendSelected(vkUserProfile user);

	void postSended(qint64 userId, quint64 postId);
	void postError(int code, QString text);

	void on_buttonBox_accepted();

	void on_buttonBox_rejected();

private:
    Ui::wndSendVkFirendAudio *ui;
    mPlayList * playList;
	vkUserProfile selectedUser;
};

#endif // WNDSENDVKFIRENDAUDIO_H
