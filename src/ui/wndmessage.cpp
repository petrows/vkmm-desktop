#include "core/core.h"

#include "wndmessage.h"
#include "ui_wndmessage.h"

#include <QDebug>

wndMessage::wndMessage(QWidget *parent) :
	QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
    ui(new Ui::wndMessage)
{
    ui->setupUi(this);
	resultButton = QMessageBox::Cancel;
	setDontShowAgain(false, "");
	setDefaultCancelButton(QMessageBox::NoButton);
	closeNormal = false;
	connect(this,SIGNAL(rejected()),SLOT(onReject()));
}

wndMessage::~wndMessage()
{
	delete ui;
}

void wndMessage::show()
{
	// Add buttons...
	for (int x=0; x<buttons.size(); x++)
	{
		QPushButton * btn = getDefaultButton(this, buttons.at(x));
		ui->buttonsLayout->addWidget(btn);
		connect(btn,SIGNAL(clicked()),this,SLOT(btnClicked()));
		if (buttons.at(x) == defaultButton)
		{
			btn->setFocus();
			resultButton = buttons.at(x); // Default...
		}
	}
	QDialog::show();
}

void wndMessage::closeEvent(QCloseEvent *e)
{
	if (!closeNormal)
	{
		onReject();
	}
	QDialog::closeEvent(e);
}

QPushButton *wndMessage::getDefaultButton(QWidget *parent, QMessageBox::StandardButton btnType)
{
	QPushButton * btn = new QPushButton(parent);
	btn->setProperty("button-type", (int)btnType);

	switch (btnType)
	{
		case QMessageBox::Ok:
			btn->setText(tr("OK"));
			btn->setIcon(QIcon(":/icons/tick.png"));
			break;
		case QMessageBox::Yes:
			btn->setText(tr("Да"));
			btn->setIcon(QIcon(":/icons/tick.png"));
			break;
		case QMessageBox::No:
			btn->setText(tr("Нет"));
			btn->setIcon(QIcon(":/icons/cross.png"));
			break;
		case QMessageBox::Cancel:
			btn->setText(tr("Отмена"));
			btn->setIcon(QIcon(":/icons/cross.png"));
			break;
		default:
			break;
	};
	return btn;
}

QMessageBox::StandardButton wndMessage::result()
{
	return resultButton;
}

void wndMessage::setText(QString text)
{
	ui->lblText->setText(text);
}

void wndMessage::setIcon(QMessageBox::Icon icon)
{
	if (QMessageBox::Question == icon)
	{
		ui->lblIcon->setPixmap(QPixmap(":/icons/dialog-question.png")); return;
	}
	ui->lblIcon->setPixmap(QMessageBox::standardIcon(icon));
}

void wndMessage::setDontShowAgain(bool visible, QString id)
{
	ui->chkDontAsk->setVisible(visible);
	wndId = id;
}

void wndMessage::addButton(QMessageBox::StandardButton btn)
{
	buttons.push_back(btn);
}

void wndMessage::addButtonYesNo()
{
	addButton(QMessageBox::Yes);
	addButton(QMessageBox::No);
	setDefaultButton(QMessageBox::Yes);
}

void wndMessage::setDefaultButton(QMessageBox::StandardButton btn)
{
	defaultButton = btn;
}

void wndMessage::setDefaultCancelButton(QMessageBox::StandardButton btn)
{
	defaultCancelButton = btn;
}

int wndMessage::confirm(QWidget *parent, QString id, QString text)
{
	if (!id.isEmpty())
	{
		// Check - must show?
		if (wndMessage::isDontAsked(id))
			return 2; // Do not show
	}
	wndMessage * wnd = new wndMessage(parent);
	wnd->setWindowTitle(tr("Vkontakte Music Mania"));
	wnd->setText(text);
	wnd->setIcon(QMessageBox::Question);
	wnd->setDontShowAgain(!id.isEmpty(), id);
	wnd->addButtonYesNo();
	wnd->show();
	wnd->exec();
	bool ret = wnd->result() == QMessageBox::Yes;
	qDebug() << (int)wnd->result();
	wnd->deleteLater();
	return ret;
}

bool wndMessage::isDontAsked(QString id)
{
	return mCore::instance()->settings->value(QString("no-ask-window/%1").arg(id),false).toBool();
}

void wndMessage::btnClicked()
{
	QObject * btn = sender();
	if (NULL != btn)
	{
		closeNormal = true;
		resultButton = (QMessageBox::StandardButton)btn->property("button-type").toInt();

		switch (resultButton)
		{
			case QMessageBox::Ok:
			case QMessageBox::Yes:
				// Save dont-ask?
				if (!wndId.isEmpty())
				{
					mCore::instance()->settings->setValue(QString("no-ask-window/%1").arg(wndId),ui->chkDontAsk->isChecked());
				}
				break;
			default:
				break;
		}
	}
	close();
}

void wndMessage::onReject()
{
	if (closeNormal) return;
	if (QMessageBox::NoButton == defaultCancelButton)
	{
		// Try to auto-detect it
		if (buttons.contains(QMessageBox::Cancel))
		{
			setDefaultCancelButton(QMessageBox::Cancel);
		} else if (buttons.contains(QMessageBox::No)) {
			setDefaultCancelButton(QMessageBox::No);
		} else {
			setDefaultCancelButton(resultButton);
		}
	}
	resultButton = defaultCancelButton;
}
