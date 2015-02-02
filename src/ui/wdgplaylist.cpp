#include "wdgplaylist.h"
#include "core/core.h"
#include "core/mplaylist.h"
#include "core/mplayer.h"

#include "core/common.h"

#include "ui/wndsendvkfirendaudio.h"

#include <QDebug>
#include <QResizeEvent>
#include <QScrollBar>
#include <QMenu>
#include <QAction>

wdgPlayList::wdgPlayList(QWidget *parent) :
	QTreeView(parent)
{
	playList = NULL;
	mdl = NULL;
	setRootIsDecorated(false);
	setHeaderHidden(true);
	setAllColumnsShowFocus(true);
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	menuAllowPlay	= false;
	menuAllowSave	= false;
	menuAllowSend	= false;
	menuAllowRemove = false;
	contextMenu		= new QMenu(this);
	connect(this,SIGNAL(customContextMenuRequested(QPoint)),SLOT(menuRequest(QPoint)));
	itemAtMenu = NULL;
}

void wdgPlayList::setPlayList(mPlayList *list)
{
	playList = list;
	mdl = new wdgPlayListModel(list,this);
	mdl->view = this;
	setModel(mdl);
}

void wdgPlayList::setColorCut(int size)
{
	if (NULL != mdl)
		mdl->setColorCut(size);
}

void wdgPlayList::resizeEvent(QResizeEvent *event)
{
	int v_width = width() - 20;
	if (NULL != verticalScrollBar() && verticalScrollBar()->isVisible())
	{
		v_width = v_width - verticalScrollBar()->width();
	}

	int row_1_w = 20;
	int row_3_w = 20;
	int row_4_w = 20;
	int row_5_w = 50;

	setColumnWidth(0, row_1_w);
	setColumnWidth(1, v_width - (row_1_w + row_3_w + row_4_w + row_5_w));
	setColumnWidth(2, row_3_w);
	setColumnWidth(3, row_4_w);
	setColumnWidth(4, row_5_w);
	if (NULL != event) event->accept();
}

void wdgPlayList::mouseDoubleClickEvent(QMouseEvent *event)
{
	// Start play of item...
	QModelIndex clickIndex = indexAt(event->pos());
	if (!clickIndex.isValid())
		return;

	mPlayListItem *item = static_cast<mPlayListItem*>(clickIndex.internalPointer());

	// Play song!
	mCore::instance()->playItem(item);
	qDebug() << "Play queed";
}

void wdgPlayList::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Delete)
	{
		menuRemove();
		return;
	}
	if (event->text().length())
	{
		if (!event->modifiers())
		{
			// Emit signal
			emit textEntered(event->text());
		}
	}
	QTreeView::keyPressEvent(event);
}

QList<mPlayListItem *> wdgPlayList::selectedSongs()
{
	QList<mPlayListItem *> out;
	// Get selected indexes...
	QModelIndexList indexes = selectedIndexes();
	for (int x=0; x<indexes.size(); x++)
	{
		if (indexes.at(x).column() != 0) continue;
		mPlayListItem *item = static_cast<mPlayListItem*>(indexes.at(x).internalPointer());
		if (NULL == item) continue;
		out.push_back(item);
	}
	return out;
}

void wdgPlayList::menuCreate()
{
	contextMenu->clear();
	setContextMenuPolicy(Qt::CustomContextMenu);
}

void wdgPlayList::menuPlay()
{
	QList<mPlayListItem*> selected = selectedSongs();
	if (selected.size() == 0) return; // WTF???
	mCore::instance()->playItem(selected.at(0));
}

void wdgPlayList::menuSaveAudio()
{
}

void wdgPlayList::menuSendFriend()
{
	QList<mPlayListItem*> selected = selectedSongs();
	if (selected.size() == 0) return; // WTF???
	wndSendVkFirendAudio * wnd = new wndSendVkFirendAudio();
	for (int x=0; x<selected.size(); x++)
		if (selected.at(x)->vkRecord.isValid) wnd->addItem(selected.at(x));
	wnd->show();
}

void wdgPlayList::menuRemove()
{
	QList<mPlayListItem*> selected = selectedSongs();
	if (selected.size() == 0) return; // WTF???
	playList->removeItems(selected);
}

void wdgPlayList::menuLastfmSimTrack()
{
	if (NULL == itemAtMenu) return;
	mCore::instance()->createPlayListFromLastfmSimTrack(itemAtMenu->getAudioArtist(), itemAtMenu->getAudioTitle());
}

void wdgPlayList::menuLastfmSimArtist()
{
	if (NULL == itemAtMenu) return;
}

void wdgPlayList::menuLastfmTopArtist()
{
	if (NULL == itemAtMenu) return;
	mCore::instance()->createPlayListFromLastfmArtistTop(itemAtMenu->getAudioArtist(), itemAtMenu->lastfmTrack.mbidArtist, 1000);
}

void wdgPlayList::menuRequest(const QPoint &p)
{
	contextMenu->clear();
	// Check - one or not selected?
	if (selectedSongs().size() == 0)
	{
		// WTF??
		return;
	}

	QAction * act;

	if (menuAllowPlay)
	{
		act = new QAction(QIcon(":/icons/control.png"), tr("Играть"), contextMenu);
		connect(act,SIGNAL(triggered()),SLOT(menuPlay()));
		contextMenu->addAction(act);
	}

	act = new QAction(QIcon(":/icons/cross.png"), tr("Убрать"), contextMenu);
	connect(act,SIGNAL(triggered()),SLOT(menuRemove()));
	contextMenu->addAction(act);

	act = new QAction(this);
	act->setSeparator(true);
	contextMenu->addAction(act);

	// Sorting
	QMenu * sortmenu = contextMenu->addMenu(QIcon(":/icons/sort.png"),tr("Сортировка"));
	QActionGroup * sortGroup = new QActionGroup(sortmenu);
	act = new QAction(tr("Оригинальная"),contextMenu);
	act->setCheckable(true);
	act->setChecked(mPlayList::SORT_ORIGINAL == playList->sort);
	act->setProperty("st",(int)mPlayList::SORT_ORIGINAL);
	act->setActionGroup(sortGroup);
	connect(act,SIGNAL(triggered()),SLOT(menuSort()));
	sortmenu->addAction(act);
	act = new QAction(tr("Название"),contextMenu);
	act->setCheckable(true);
	act->setChecked(mPlayList::SORT_NAME == playList->sort);
	act->setProperty("st",(int)mPlayList::SORT_NAME);
	act->setActionGroup(sortGroup);
	connect(act,SIGNAL(triggered()),SLOT(menuSort()));
	sortmenu->addAction(act);
	act = new QAction(tr("Длительность"),contextMenu);
	act->setCheckable(true);
	act->setChecked(mPlayList::SORT_LENGTH == playList->sort);
	act->setProperty("st",(int)mPlayList::SORT_LENGTH);
	act->setActionGroup(sortGroup);
	connect(act,SIGNAL(triggered()),SLOT(menuSort()));
	sortmenu->addAction(act);
	act = new QAction(this);
	act->setSeparator(true);
	sortmenu->addAction(act);
	act = new QAction(tr("В обратном порядке"),contextMenu);
	act->setCheckable(true);
	act->setChecked(playList->sortOrder == mPlayList::ORDER_DESC);
	connect(act,SIGNAL(toggled(bool)),SLOT(menuSortOrder(bool)));
	sortmenu->addAction(act);

	act = new QAction(contextMenu);
	act->setSeparator(true);
	contextMenu->addAction(act);

	QMenu * actMenuVk = NULL;
	QMenu * actSelectedTrack = NULL;
	QList<mPlayListItem*> items = selectedSongs();

	if (indexAt(p).isValid())
	{
		itemAtMenu = static_cast<mPlayListItem*>(indexAt(p).internalPointer());
	} else {
		itemAtMenu = NULL;
	}

	if (items.size() == 1)
	{
		// One-track
		mPlayListItem * selectedItem = items.at(0);
		QString songTitle = selectedItem->getAudioTitle();
		if (songTitle.length() > 33) songTitle = songTitle.left(30) + "...";

		// Vk actions?
		if (selectedItem->vkRecord.isValid && (menuAllowSave && menuAllowSend))
		{
			actMenuVk = contextMenu->addMenu(QIcon(":/icons/vk.png"),songTitle);
		}
	} else {
		// Count valid vk records
		int vkValid = 0;
		for (int x=0; x<items.size(); x++) if (items.at(x)->vkRecord.isValid) vkValid++;
		if (vkValid > 0 && (menuAllowSave && menuAllowSend))
		{
			actMenuVk = contextMenu->addMenu(QIcon(":/icons/vk.png"),tr("Выбрано аудиозаписей: %1").arg(vkValid));
		}
	}

	if (NULL != actMenuVk)
	{
		if (menuAllowSave)
		{
			act = new QAction(QIcon(":/icons/vk-plus.png"), tr("Добавить в 'Мои Аудиозаписи'"), contextMenu);
			connect(act,SIGNAL(triggered()),SLOT(menuSaveAudio()));
			actMenuVk->addAction(act);
		}
		if (menuAllowSend)
		{
			act = new QAction(QIcon(":/icons/vk-plus.png"), tr("Отправить другу"), contextMenu);
			connect(act,SIGNAL(triggered()),SLOT(menuSendFriend()));
			actMenuVk->addAction(act);
		}
	}

	if (NULL != itemAtMenu)
	{
		QString songTitle = itemAtMenu->getAudioTitle();
		if (songTitle.length() > 33) songTitle = songTitle.left(30) + "...";
		QString songArtist = itemAtMenu->getAudioArtist();
		if (songArtist.length() > 33) songArtist = songArtist.left(30) + "...";
		actSelectedTrack = contextMenu->addMenu(QIcon(":/icons/music.png"), songTitle);

		act = new QAction(QIcon(":/icons/lastfm.png"), tr("Похожие на \"%1\"").arg(songTitle), contextMenu);
		connect(act, SIGNAL(triggered()), SLOT(menuLastfmSimTrack()));
		actSelectedTrack->addAction(act);

		act = new QAction(QIcon(":/icons/lastfm.png"), tr("Топ песен \"%1\"").arg(songArtist), contextMenu);
		connect(act, SIGNAL(triggered()), SLOT(menuLastfmTopArtist()));
		actSelectedTrack->addAction(act);
	}

	contextMenu->popup(mapToGlobal(p));
}

void wdgPlayList::menuSort()
{
	mPlayList::playListSorting sort = (mPlayList::playListSorting)sender()->property("st").toInt();
	playList->setSorting(sort, playList->sortOrder);
}

void wdgPlayList::menuSortOrder(bool desc)
{
	playList->setSorting(playList->sort, desc?mPlayList::ORDER_DESC:mPlayList::ORDER_ASC);
}

wdgPlayListModel::wdgPlayListModel(mPlayList *lst, QObject *parent)
	: QAbstractItemModel(parent)
{
	playList = lst;
	connect(playList,SIGNAL(stateChanged(mPlayList::playListLoadState)),SLOT(playListStatusChanged(mPlayList::playListLoadState)));
	connect(playList,SIGNAL(dataChanged()),SLOT(playListDataChanged()));
	connect(mCore::instance(),SIGNAL(playbackStarted(mPlayListItem*)),SLOT(playStarted(mPlayListItem*)));
	colorCut = -1;
}

int wdgPlayListModel::rowCount(const QModelIndex &parent) const
{
	if (NULL == playList) return 0;
	if (parent.isValid()) return 0; // No sub-tree

	if (mPlayList::LS_LOADING == playList->loadState)
	{
		return 1;
	}
	if (mPlayList::LS_ERROR == playList->loadState)
	{
		return 1;
	}
	if (mPlayList::LS_DONE == playList->loadState && 0 == playList->getItemsCount())
	{
		return 1;
	}

	return playList->getItemsCount();
}

QVariant wdgPlayListModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid()) return QVariant();
	if (NULL == playList) return QVariant();

	if (mPlayList::LS_LOADING == playList->loadState)
	{
		if (Qt::DisplayRole == role)
		{
			if (index.column() == 0)
			{
				if (mPlayList::FROM_VK_SEARCH == playList->origin)
				{
					// Set special text for pages loading
					int pagesLoaded = 0;
					int pagesTotal = 0;
					playList->getPagesProgress(pagesLoaded,pagesTotal);
					return tr("Загрузка...\n\nСтраница %1 из %2").arg(pagesLoaded).arg(pagesTotal);
				}

				QString loadText = tr("Загрузка...");
				if (-1 != playList->getLoadBytes())
				{
					loadText += tr("\n\n%1").arg(formatSize(playList->getLoadBytes()));
				}
				return loadText;
			}
		}
		if (Qt::TextAlignmentRole == role)
		{
			return QVariant(Qt::AlignHCenter | Qt::AlignVCenter);
		}
		if (Qt::SizeHintRole == role)
		{
			return QSize(100,200);
		}
		return QVariant();
	}
	if (mPlayList::LS_ERROR == playList->loadState)
	{
		if (Qt::DisplayRole == role)
		{
			if (index.column() == 0)
			{
				return tr("Ошибка получения списка!");
			}
		}
		if (Qt::TextColorRole == role)
		{
			return QColor(Qt::red);
		}
		if (Qt::TextAlignmentRole == role)
		{
			return QVariant(Qt::AlignHCenter | Qt::AlignVCenter);
		}
		if (Qt::SizeHintRole == role)
		{
			return QSize(100,100);
		}
		return QVariant();
	}
	if (mPlayList::LS_DONE == playList->loadState && 0 == playList->getItemsCount())
	{
		if (Qt::DisplayRole == role)
		{
			if (index.column() == 0)
			{
				return tr("Список пуст");
			}
		}
		if (Qt::TextAlignmentRole == role)
		{
			return QVariant(Qt::AlignHCenter | Qt::AlignVCenter);
		}
		if (Qt::SizeHintRole == role)
		{
			return QSize(100,200);
		}
		return QVariant();
	}

	// Get song item...
	mPlayListItem * item = static_cast<mPlayListItem*>(index.internalPointer());
	if (NULL == item)
		return QVariant();

	if (Qt::DisplayRole == role)
	{
		if (0 == index.column())
		{
			if (-1 != colorCut && index.row() > colorCut)
				return "*";
			return QString();
		}
		if (1 == index.column())
		{
			return QString(item->getAudioArtist() + " - " + item->getAudioTitle());
		}
		if (4 == index.column())
		{
			return item->getTimeFormatted();
		}
		return QVariant();
	}

	if (Qt::TextColorRole == role)
	{
		if (-1 != colorCut && index.row() > colorCut)
		{
			return QColor(Qt::red);
		}
		if (1 == index.column())
		{
			if (item->isError) return QColor("#990000");
		}
		if (4 == index.column())
		{
			return QColor(Qt::gray);
		}
		return QVariant();
	}

	if (Qt::DecorationRole == role)
	{
		if (0 == index.column())
		{
			if (item->isSearched)
			{
				return QIcon(":/icons/magnifier-small.png");
			}
			if (item->isPlaying)
			{
				return QIcon(":/icons/control-000-small.png");
			}
			if (item->isCached())
			{
				return QIcon(":/icons/tick-small.png");
			}
			if (item->isError)
			{
				return QIcon(":/icons/cross-small.png");
			}

			return QIcon();
		}

		// Last.Fm fav icon
		if (2 == index.column())
		{
			if (item->isLovedLastFm)
				return QIcon(":/icons/heart-small.png");
			return QIcon();
		}
		// In-vk icon
		if (3 == index.column())
		{
			if (item->vkRecord.isValid)
				return QIcon(":/icons/vk-small.png");
			return QIcon();
		}
		return QVariant();
	}

	if (Qt::ToolTipRole == role)
	{
		if (0 == index.column())
		{
			if (item->isSearched)
			{
				return tr("Поиск аудиозаписи");
			}
			if (item->isPlaying)
			{
				return tr("Играет сейчас");
			}
			if (item->isCached())
			{
				return tr("Аудиозапись закеширована");
			}
			if (item->isError)
			{
				return tr("Ошибка поиска");
			}
			return QVariant();
		}
		if (2 == index.column())
		{
			if (item->isLovedLastFm)
				return tr("Last.fm: избранное");
			return QVariant();
		}
		if (3 == index.column())
		{
			if (item->vkRecord.isValid)
				return tr("Аудиозапись найдена ВКонтакте");
			return QVariant();
		}
		return QVariant();
	}

	if (Qt::FontRole == role)
	{
		if (item->isPlaying)
		{
			QFont fnt;
			fnt.setBold(true);
			return fnt;
		}
		return QVariant();
	}

    if (Qt::TextAlignmentRole == role)
    {
		if (2 == index.column() || 3 == index.column() || 4 == index.column() || 0 == index.column())
        {
            return QVariant(Qt::AlignHCenter | Qt::AlignVCenter);
        }
    }

	if (Qt::SizeHintRole == role)
	{
        return QSize(100,LISTS_ITEMS_HEIGHT);
	}

	return QVariant();
}

int wdgPlayListModel::columnCount(const QModelIndex &parent) const
{
	if (NULL == playList) return 0;

	if (mPlayList::LS_LOADING == playList->loadState)
	{
		return 1;
	}
	if (mPlayList::LS_ERROR == playList->loadState)
	{
		return 1;
	}
	if (mPlayList::LS_DONE == playList->loadState && 0 == playList->getItemsCount())
	{
		return 1;
	}
	return 5;
}

QModelIndex wdgPlayListModel::parent(const QModelIndex &child) const
{
	return QModelIndex();
}

QModelIndex wdgPlayListModel::index(int row, int column, const QModelIndex &parent) const
{
	if (NULL == playList) return QModelIndex();

	if (!hasIndex(row, column, parent))
		return QModelIndex();

	if (!parent.isValid())
	{
		return createIndex(row,column,playList->getItem(row));
	} else {
		return createIndex(row,column,(mPlayListItem*)NULL);
	}
}

Qt::ItemFlags wdgPlayListModel::flags(const QModelIndex &) const
{
	if (NULL == playList) return Qt::NoItemFlags;

	if (mPlayList::LS_LOADING == playList->loadState)
	{
		return Qt::NoItemFlags;
	}
	if (mPlayList::LS_ERROR == playList->loadState)
	{
		return Qt::NoItemFlags;
	}
	if (mPlayList::LS_DONE == playList->loadState && 0 == playList->getItemsCount())
	{
		return Qt::NoItemFlags;
	}
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void wdgPlayListModel::setColorCut(int size)
{
	colorCut = size;
}

void wdgPlayListModel::playListStatusChanged(mPlayList::playListLoadState s)
{
	emit dataChanged(index(0,0,QModelIndex()),index(playList->getItemsCount()-1,3,QModelIndex()));
	reset();
	view->resizeEvent(NULL);
}

void wdgPlayListModel::playListDataChanged()
{
	emit dataChanged(index(0,0,QModelIndex()),index(playList->getItemsCount()-1,3,QModelIndex()));
}

void wdgPlayListModel::playStarted(mPlayListItem * item)
{
	emit dataChanged(index(0,0,QModelIndex()),index(1,0,QModelIndex()));
	emit itemStarted(item);
	if (!view->hasFocus() && playList->getItemIndex(item) != -1)
	{
		// Search index by item...
		view->scrollTo(index(playList->getItemIndex(item),0,QModelIndex()));
	}
}
