#ifndef MPLAYSLIDER_H
#define MPLAYSLIDER_H

#include <QWidget>

class wdgPlaySlider : public QWidget
{
	Q_OBJECT
public:
	explicit wdgPlaySlider(QWidget *parent = 0);
	void resetTrack();
	void mousePressEvent(QMouseEvent * event);
	void mouseMoveEvent(QMouseEvent * event);

protected:
	void paintEvent(QPaintEvent * event);
	void drawIdle();
	void drawPlaying();

signals:
	void positionChange(uint time);

public slots:
	void setLength(uint time);
	void setSize(uint size);
	void setPlayPos(uint time);
	void setDownloadPos(uint from, uint size);

private:
	uint trackTime;
	uint trackSize;
	uint trackPos;
	uint trackBufferStart;
	uint trackBufferSize;

	QRect bgRect;
};

#endif // MPLAYSLIDER_H
