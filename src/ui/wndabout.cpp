#include "wndabout.h"
#include "ui_wndabout.h"
#include "core/common.h"
#include "version.h"

wndAbout::wndAbout(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::wndAbout)
{
	ui->setupUi(this);
	langButtons::setButtons(ui->buttonBox);

	int verMajor, verMinor, verBuild;	
	getVersion(verMajor, verMinor, verBuild);
	QString versionStr = QString("<b>%1.%2.%3</b>").arg(verMajor).arg(verMinor).arg(verBuild);
#ifdef CMAKE_CFG_DATA_REC
	versionStr += " (Recovery)";
#endif
	ui->lblVersion->setText(versionStr);
	//ui->lblBuildDate->setText(__DATE__" "__TIME__);
	ui->lblBuildDate->setText(getBuildTime().toLocalTime().toString(Qt::SystemLocaleDate));
}

wndAbout::~wndAbout()
{
	delete ui;
}
