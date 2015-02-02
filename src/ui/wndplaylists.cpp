#include "wndplaylists.h"
#include "ui_wndplaylists.h"

wndPlayLists::wndPlayLists(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::wndPlayLists)
{
	ui->setupUi(this);
}

wndPlayLists::~wndPlayLists()
{
	delete ui;
}
