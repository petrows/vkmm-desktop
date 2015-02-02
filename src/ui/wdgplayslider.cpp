#include "wdgplayslider.h"

#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

wdgPlaySlider::wdgPlaySlider(QWidget *parent) :
	QWidget(parent)
{
    setMinimumSize(100,17);
    setMaximumSize(65536,17);
	setMouseTracking(true);
	resetTrack();
}

void wdgPlaySlider::resetTrack()
{
	trackTime = 0;
	trackSize = 0;
	trackBufferStart = 0;
	trackBufferSize = 0;
	trackPos = 0;
	repaint();
}

void wdgPlaySlider::mousePressEvent(QMouseEvent * event)
{
	// Calc click pos
	int clickW = event->pos().x() - bgRect.left();
	if (clickW >= 0 && clickW <= bgRect.width())
	{
		// Click new pos!
		double pxPerSecond = (double)bgRect.width()/(double)trackTime;
        int newSecond = (int)(clickW/pxPerSecond);

        if (newSecond != (int)trackPos)
		{
			setPlayPos(newSecond);
			emit positionChange(newSecond);
		}
	}

	event->accept();
}

void wdgPlaySlider::mouseMoveEvent(QMouseEvent *event)
{
	if (event->buttons() & Qt::LeftButton)
	{
		// olololo!
		mousePressEvent(event);
		return;
	}
	event->accept();
}

void wdgPlaySlider::paintEvent(QPaintEvent * event)
{
	int w = size().width();

	bgRect = QRect(5,5,w-10,5);

	if (0 == trackTime)
	{
		// 'Idle' status
		drawIdle();
	} else {
		// 'Normal' status
		drawPlaying();
	}

	QWidget::paintEvent(event);
}

void wdgPlaySlider::drawIdle()
{
	QPainter painter(this);

	painter.fillRect(bgRect, QColor(Qt::white));
	painter.setPen(QColor(Qt::darkGray));
	painter.drawRect(bgRect);
}

void wdgPlaySlider::drawPlaying()
{
	QPainter painter(this);

	painter.fillRect(bgRect, QColor(Qt::white));
	// Calc buffer pos
	if (0 != trackBufferSize && 0 != trackSize)
	{
		if (trackBufferStart > trackSize)
			trackBufferStart = trackSize;

		if (trackBufferSize > (trackSize-trackBufferStart))
			trackBufferSize = trackSize-trackBufferStart;

		// Calc width & pos
		double pxPerByte = (double)bgRect.width()/(double)trackSize;

		QRect bufRect = bgRect;
		bufRect.setLeft(bufRect.left() + (int)(pxPerByte * (double)trackBufferStart));
		bufRect.setWidth((int)(pxPerByte * (double)trackBufferSize));

		if (bufRect.width() > 0)
		{
			painter.fillRect(bufRect, QColor("#008080"));
		}
	}
	painter.setPen(QColor(Qt::darkGray));
	painter.drawRect(bgRect);

	// Draw playing pos time
	double pxPerSecond = (double)bgRect.width()/(double)trackTime;
	int sliderPos = (pxPerSecond * (double)trackPos);

	// qDebug() << sliderPos << bgRect.width() << pxPerSecond;

	QRect sliderRect = QRect(sliderPos+2,0,5,15);
	painter.fillRect(sliderRect,QColor("#909090"));
	painter.setPen(QColor("#303030"));
	painter.drawRect(sliderRect);
}

void wdgPlaySlider::setLength(uint time)
{
	trackTime = time;
	repaint();
}

void wdgPlaySlider::setSize(uint size)
{
	trackSize = size;
	repaint();
}

void wdgPlaySlider::setPlayPos(uint time)
{
	trackPos = time;
	if (trackTime < trackPos)
		trackPos = trackTime;

	repaint();
}

void wdgPlaySlider::setDownloadPos(uint from, uint size)
{
	trackBufferStart = from;
	trackBufferSize = size;
	repaint();
}
