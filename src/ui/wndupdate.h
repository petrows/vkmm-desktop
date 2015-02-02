#ifndef WNDUPDATE_H
#define WNDUPDATE_H

#include <QWizard>

namespace Ui {
class wndUpdate;
}

class wndUpdate : public QWizard
{
    Q_OBJECT
    
public:
    explicit wndUpdate(QWidget *parent = 0);
    ~wndUpdate();
private slots:
    void onCurrentIdChanged(int id);
private:
    Ui::wndUpdate *ui;
};

#endif // WNDUPDATE_H
