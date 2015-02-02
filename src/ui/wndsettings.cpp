#include "wndsettings.h"
#include "ui_wndsettings.h"

#include "core/common.h"
#include "core/core.h"
#include "core/mcacheaudio.h"

#include <QList>
#include <QNetworkProxy>

wndSettings::wndSettings(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::wndSettings)
{
	ui->setupUi(this);
	ui->tabMain->setCurrentIndex(0);
	ui->tabMainInner->setCurrentIndex(0);
	langButtons::setButtons(ui->buttonBox);

	connect(ui->lastfmLogin,SIGNAL(textChanged(QString)),SLOT(formChanged()));
	connect(ui->lastfmPasswd,SIGNAL(textChanged(QString)),SLOT(formChanged()));
	connect(ui->vkSendOnline,SIGNAL(stateChanged(int)),SLOT(formChanged()));
	connect(ui->vkSendStatus,SIGNAL(stateChanged(int)),SLOT(formChanged()));

	connect(ui->lastfmScrobbingEnable,SIGNAL(stateChanged(int)),SLOT(formChanged()));
	connect(ui->lastfmLogin,SIGNAL(textChanged(QString)),SLOT(formChanged()));
	connect(ui->lastfmPasswd,SIGNAL(textChanged(QString)),SLOT(formChanged()));
	connect(ui->lastfmScrobbingSize,SIGNAL(valueChanged(int)),SLOT(formChanged()));

	connect(ui->updatesEnable,SIGNAL(stateChanged(int)),SLOT(formChanged()));

	ui->proxyType->addItem(tr("Без прокси"), (int)QNetworkProxy::NoProxy);
	ui->proxyType->addItem(tr("HTTP"), (int)QNetworkProxy::HttpProxy);
	ui->proxyType->addItem(tr("SOCKS-5"), (int)QNetworkProxy::Socks5Proxy);
	connect(ui->proxyType,SIGNAL(currentIndexChanged(int)),SLOT(updateItems()));
	connect(ui->proxyType,SIGNAL(currentIndexChanged(int)),SLOT(formChanged()));
	connect(ui->proxyAddr,SIGNAL(textChanged(QString)),SLOT(formChanged()));
	connect(ui->proxyPort,SIGNAL(valueChanged(int)),SLOT(formChanged()));
	connect(ui->proxyLogin,SIGNAL(textChanged(QString)),SLOT(formChanged()));
	connect(ui->proxyPasswd,SIGNAL(textChanged(QString)),SLOT(formChanged()));

	connect(ui->cacheEnable,SIGNAL(stateChanged(int)),SLOT(formChanged()));
	connect(ui->cacheCountLimit,SIGNAL(stateChanged(int)),SLOT(formChanged()));
	connect(ui->cacheCountLimitSize,SIGNAL(valueChanged(int)),SLOT(formChanged()));
	connect(ui->cacheSizeLimit,SIGNAL(stateChanged(int)),SLOT(formChanged()));
	connect(ui->cacheSizeLimitSize,SIGNAL(valueChanged(int)),SLOT(formChanged()));
	connect(ui->cacheEnable,SIGNAL(stateChanged(int)),SLOT(updateItems()));
	connect(ui->cacheCountLimit,SIGNAL(stateChanged(int)),SLOT(updateItems()));
	connect(ui->cacheSizeLimit,SIGNAL(stateChanged(int)),SLOT(updateItems()));

	loadSettings();
}

wndSettings::~wndSettings()
{
	delete ui;
}

void wndSettings::saveSettings()
{
	mCore::instance()->settings->setValue("vk/sendOnline", ui->vkSendOnline->isChecked());
	mCore::instance()->settings->setValue("vk/sendStatus", ui->vkSendStatus->isChecked());

	mCore::instance()->settings->setValue("lastfm/scrobbingEnable", ui->lastfmScrobbingEnable->isChecked());
	mCore::instance()->settings->setValue("lastfm/scrobbingSize", ui->lastfmScrobbingSize->value());
	mCore::instance()->settings->setValue("lastfm/login", ui->lastfmLogin->text());
	// Generate token?
	if (!ui->lastfmPasswd->text().isEmpty() && !ui->lastfmLogin->text().isEmpty())
	{
		// Generate and save new lastfm token
		QString token = mApiLastfm::genToken(ui->lastfmLogin->text(), ui->lastfmPasswd->text());
		// New token?
		if (token != mCore::instance()->settings->value("lastfm/token").toString())
		{
			// Re-init session
			mCore::instance()->settings->setValue("lastfm/token", token);
			mCore::instance()->settings->setValue("lastfm/sid", "");
			mCore::instance()->init_lastfm();
		}
	} else {
		if (ui->lastfmLogin->text().isEmpty())
		{
			mCore::instance()->settings->setValue("lastfm/token", "");
			mCore::instance()->settings->setValue("lastfm/sid", "");
			mCore::instance()->init_lastfm();
		}
	}

	// Proxy settings
	mCore::instance()->settings->setValue("proxy/type", ui->proxyType->itemData(ui->proxyType->currentIndex()).toInt());
	mCore::instance()->settings->setValue("proxy/addr", ui->proxyAddr->text());
	mCore::instance()->settings->setValue("proxy/port", ui->proxyPort->value());
	mCore::instance()->settings->setValue("proxy/login", ui->proxyLogin->text());
	mCore::instance()->settings->setValue("proxy/passw", ui->proxyPasswd->text());

	mCore::instance()->settings->setValue("updateAllow", ui->updatesEnable->isChecked());

	// Cache
	mCore::instance()->settings->setValue("cache/enable", ui->cacheEnable->isChecked());
	mCore::instance()->settings->setValue("cache/limitCount", ui->cacheCountLimit->isChecked());
	mCore::instance()->settings->setValue("cache/limitCountSize", ui->cacheCountLimitSize->value());
	mCore::instance()->settings->setValue("cache/limitSize", ui->cacheSizeLimit->isChecked());
	mCore::instance()->settings->setValue("cache/limitSizeSize", ui->cacheSizeLimitSize->value());

	mCore::instance()->reloadSettings();
	ui->buttonBox->button(QDialogButtonBox::Apply)->setDisabled(true);
}

void wndSettings::loadSettings()
{
	int selectedProxyType = mCore::instance()->settings->value("proxy/type").toInt();
	for (int x=0; x<ui->proxyType->count(); x++)
	{
		if (ui->proxyType->itemData(x).toInt() == selectedProxyType)
		{
			ui->proxyType->setCurrentIndex(x); break;
		}
	}

	ui->proxyAddr->setText(mCore::instance()->settings->value("proxy/addr","").toString());
	ui->proxyPort->setValue(mCore::instance()->settings->value("proxy/port","").toInt());
	ui->proxyLogin->setText(mCore::instance()->settings->value("proxy/login","").toString());
	ui->proxyPasswd->setText(mCore::instance()->settings->value("proxy/passw","").toString());

	ui->vkSendOnline->setChecked(mCore::instance()->settings->value("vk/sendOnline",true).toBool());
	ui->vkSendStatus->setChecked(mCore::instance()->settings->value("vk/sendStatus",true).toBool());

	ui->lastfmLogin->setText(mCore::instance()->settings->value("lastfm/login","").toString());
	ui->lastfmScrobbingEnable->setChecked(mCore::instance()->settings->value("lastfm/scrobbingEnable", true).toBool());
	ui->lastfmScrobbingSize->setValue(mCore::instance()->settings->value("lastfm/scrobbingSize", 30).toInt());

	ui->updatesEnable->setChecked(mCore::instance()->settings->value("updateAllow", true).toBool());

	ui->cacheEnable->setChecked(mCore::instance()->settings->value("cache/enable", true).toBool());
	ui->cacheCountLimit->setChecked(mCore::instance()->settings->value("cache/limitCount", true).toBool());
	ui->cacheCountLimitSize->setValue(mCore::instance()->settings->value("cache/limitCountSize",100).toInt());
	ui->cacheSizeLimit->setChecked(mCore::instance()->settings->value("cache/limitSize", true).toBool());
	ui->cacheSizeLimitSize->setValue(mCore::instance()->settings->value("cache/limitSizeSize",100).toInt());

	updateItems();
	ui->buttonBox->button(QDialogButtonBox::Apply)->setDisabled(true);
}

void wndSettings::formChanged()
{
	ui->buttonBox->button(QDialogButtonBox::Apply)->setDisabled(false);
}

void wndSettings::on_buttonBox_clicked(QAbstractButton *button)
{
	if (ui->buttonBox->standardButton(button) == QDialogButtonBox::Apply)
	{
		saveSettings(); return;
	}
	if (ui->buttonBox->standardButton(button) == QDialogButtonBox::Ok)
	{
		saveSettings(); close(); return;
	}
	if (ui->buttonBox->standardButton(button) == QDialogButtonBox::Cancel)
	{
		close(); return;
	}
}

void wndSettings::updateItems()
{
	bool proxySelected = !(ui->proxyType->itemData(ui->proxyType->currentIndex()).toInt() == (int)QNetworkProxy::NoProxy);
	ui->proxyAddr->setEnabled(proxySelected);
	ui->proxyPort->setEnabled(proxySelected);
	ui->proxyLogin->setEnabled(proxySelected);
	ui->proxyPasswd->setEnabled(proxySelected);

	// Check - we have some windows with 'Dont Ask Again'?
	mCore::instance()->settings->beginGroup("no-ask-window");
	QStringList wndKeys = mCore::instance()->settings->allKeys();
	bool hasWndHidden = false;
	for (int x=0; x<wndKeys.size(); x++)
	{
		if (mCore::instance()->settings->value(wndKeys.at(x), false).toBool())
		{
			// has hidden!
			hasWndHidden = true;
			break;
		}
	}
	mCore::instance()->settings->endGroup();
	ui->resetDontAskAgain->setEnabled(hasWndHidden);

	ui->cacheLimitsGroup->setEnabled(ui->cacheEnable->isChecked());
	ui->cacheCountLimitSize->setEnabled(ui->cacheCountLimit->isChecked());
	ui->cacheSizeLimitSize->setEnabled(ui->cacheSizeLimit->isChecked());
	ui->cacheCurrentSize->setText(tr("Текущий размер: <b>%1</b>, файлов: <b>%2</b>").arg(formatSize(mCacheAudio::instance()->getTotalSize())).arg(mCacheAudio::instance()->getTotalCount()));
}

void wndSettings::on_resetDontAskAgain_clicked()
{
	mCore::instance()->settings->beginGroup("no-ask-window");
	QStringList wndKeys = mCore::instance()->settings->allKeys();
	for (int x=0; x<wndKeys.size(); x++)
	{
		mCore::instance()->settings->setValue(wndKeys.at(x), false);
	}
	mCore::instance()->settings->endGroup();
	updateItems();
}

void wndSettings::on_cacheClean_clicked()
{
	mCacheAudio::instance()->cleanAll();
	updateItems();
}
