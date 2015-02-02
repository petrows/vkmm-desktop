#ifndef WNDSETTINGS_H
#define WNDSETTINGS_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
class wndSettings;
}

class wndSettings : public QDialog
{
	Q_OBJECT
	
public:
	explicit wndSettings(QWidget *parent = 0);
	~wndSettings();

	void saveSettings();
	void loadSettings();

	
private slots:
	void formChanged();
	void on_buttonBox_clicked(QAbstractButton *button);
	void updateItems();

	void on_resetDontAskAgain_clicked();

	void on_cacheClean_clicked();

private:
	Ui::wndSettings *ui;
};

#endif // WNDSETTINGS_H
