#ifndef WDGPLAYLIST_H
#define WDGPLAYLIST_H

#include <QTreeView>
#include <QAbstractItemModel>
#include <QMenu>

class mPlayListItem;
class mPlayList;
class wdgPlayList;

#include "core/mplaylist.h"

class wdgPlayListModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit wdgPlayListModel(mPlayList * lst, QObject *parent = 0);

	wdgPlayList * view;

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role) const;
	int columnCount(const QModelIndex &parent) const;
	QModelIndex parent(const QModelIndex &child) const;
	QModelIndex index(int row, int column, const QModelIndex &parent) const;

	Qt::ItemFlags flags(const QModelIndex &index) const;

	void setColorCut(int size);

signals:
	void itemStarted(mPlayListItem * item);

private:
	mPlayList * playList;
	int colorCut;

private slots:
	void playListStatusChanged(mPlayList::playListLoadState s);
	void playListDataChanged();
	void playStarted(mPlayListItem * item);
};

class wdgPlayList : public QTreeView
{
	Q_OBJECT
public:
	explicit wdgPlayList(QWidget *parent = 0);
	void setPlayList(mPlayList * list);
	void setColorCut(int size);
	void resizeEvent(QResizeEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void keyPressEvent(QKeyEvent * event);

	QList<mPlayListItem*> selectedSongs();

	// Standart Menus:
	bool menuAllowPlay;
	bool menuAllowSave;
	bool menuAllowSend;
	bool menuAllowRemove;
	void menuCreate();
	
signals:
	void textEntered(QString text);
public slots:
	void menuPlay();
	void menuSaveAudio();
	void menuSendFriend();
	void menuRemove();
	void menuLastfmSimTrack();
	void menuLastfmSimArtist();
	void menuLastfmTopArtist();
	void menuSort();
	void menuSortOrder(bool desc);
	void menuRequest(const QPoint&);

private slots:

private:
	mPlayList * playList;
	wdgPlayListModel * mdl;
	QMenu * contextMenu;
	mPlayListItem * itemAtMenu;
};

#endif // WDGPLAYLIST_H
