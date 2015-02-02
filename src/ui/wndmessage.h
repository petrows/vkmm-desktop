#ifndef WNDMESSAGE_H
#define WNDMESSAGE_H

#include <QDialog>
#include <QList>
#include <QMessageBox>
#include <QPushButton>

namespace Ui {
class wndMessage;
}

class wndMessage : public QDialog
{
    Q_OBJECT
    
public:
    explicit wndMessage(QWidget *parent = 0);
    ~wndMessage();

	void show();
	void closeEvent(QCloseEvent * e);

	static QPushButton * getDefaultButton(QWidget * parent, QMessageBox::StandardButton btnType);

	QMessageBox::StandardButton result();
	void setText(QString text);
	void setIcon(QMessageBox::Icon icon);
	void setDontShowAgain(bool visible, QString id);
	void addButton(QMessageBox::StandardButton btn);
	void addButtonYesNo();

	void setDefaultButton(QMessageBox::StandardButton btn);
	void setDefaultCancelButton(QMessageBox::StandardButton btn);

	static int confirm(QWidget *parent, QString id, QString text);

	static bool isDontAsked(QString id);
public slots:
	void btnClicked();
	void onReject();
    
private:
    Ui::wndMessage *ui;
	QMessageBox::StandardButton resultButton;
	QMessageBox::StandardButton defaultButton;
	QMessageBox::StandardButton defaultCancelButton;
	QList<QMessageBox::StandardButton> buttons;
	bool closeNormal;
	QString wndId;
};

#endif // WNDMESSAGE_H
