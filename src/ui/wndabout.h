#ifndef WNDABOUT_H
#define WNDABOUT_H

#include <QDialog>

namespace Ui {
class wndAbout;
}

class wndAbout : public QDialog
{
	Q_OBJECT
	
public:
	explicit wndAbout(QWidget *parent = 0);
	~wndAbout();
	
private:
	Ui::wndAbout *ui;
};

#endif // WNDABOUT_H
