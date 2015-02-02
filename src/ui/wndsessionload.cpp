#include "wndsessionload.h"
#include "ui_wndsessionload.h"

wndSessionLoad::wndSessionLoad(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::wndSessionLoad)
{
	ui->setupUi(this);
}

wndSessionLoad::~wndSessionLoad()
{
	delete ui;
}

void wndSessionLoad::setText(mCore::statusType, QString text)
{
	ui->lblStatus->setText(text);
}

void wndSessionLoad::setProgress(int p)
{
	ui->progressBar->setValue(p);
}
