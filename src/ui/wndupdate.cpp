#include "wndupdate.h"
#include "ui_wndupdate.h"

#include <QAbstractButton>
#include <QDebug>

wndUpdate::wndUpdate(QWidget *parent) :
    QWizard(parent),
    ui(new Ui::wndUpdate)
{
    ui->setupUi(this);
    connect(this,SIGNAL(currentIdChanged(int)),SLOT(onCurrentIdChanged(int)));

}

wndUpdate::~wndUpdate()
{
    delete ui;
}

void wndUpdate::onCurrentIdChanged(int id)
{
    button(NextButton)->setText(tr("Далее >"));
    button(BackButton)->setText(tr("< Назад"));
    button(FinishButton)->setText(tr("Готово"));
    button(CancelButton)->setText(tr("Отмена"));    qDebug() << "pageid" << id;
    if (1 == id)
    {
        // First page
        return;
    }
    if (2 == id)
    {
        // Second page
        button(BackButton)->setDisabled(true);
        button(FinishButton)->setDisabled(true);
        return;
    }
}
