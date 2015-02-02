#ifndef WNDPLAYLISTS_H
#define WNDPLAYLISTS_H

#include <QDialog>

namespace Ui {
class wndPlayLists;
}

class wndPlayLists : public QDialog
{
	Q_OBJECT
	
public:
	explicit wndPlayLists(QWidget *parent = 0);
	~wndPlayLists();
	
private:
	Ui::wndPlayLists *ui;
};

#endif // WNDPLAYLISTS_H
