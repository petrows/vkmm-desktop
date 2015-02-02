#ifndef WNDSESSIONLOAD_H
#define WNDSESSIONLOAD_H

#include <QDialog>
#include "core/core.h"

namespace Ui {
class wndSessionLoad;
}

class wndSessionLoad : public QDialog
{
	Q_OBJECT
	
public:
	explicit wndSessionLoad(QWidget *parent = 0);
	~wndSessionLoad();
	
private:
	Ui::wndSessionLoad *ui;

public slots:
	void setText(mCore::statusType, QString text);
	void setProgress(int p);
};

#endif // WNDSESSIONLOAD_H
