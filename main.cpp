#include <QApplication>
#include <QThread>
#include <QMutex>
#include <QMessageBox>

#include <QColor>
#include <QLabel>
#include <QtDebug>
#include <QString>
#include <QPushButton>

#include "LeptonThread.h"
#include "MyLabel.h"

int main( int argc, char **argv )
{
	//create the app
	QApplication a( argc, argv );
	
	QWidget *myWidget = new QWidget;
	myWidget->setGeometry(400, 300, 340*2, 290*2);

	//create an image placeholder for myLabel
	//fill the top left corner with red, just bcuz
	QImage myImage;
	myImage = QImage(320*2, 240*2, QImage::Format_RGB888);
	QRgb red = qRgb(255,0,0);
	
	for(int i=0;i<160;i++) {
		for(int j=0;j<120;j++) {
			myImage.setPixel(i, j, red);
		}
	}

	//create a label, and set it's image to the placeholder
	MyLabel myLabel(myWidget);
	myLabel.setGeometry(10, 10, 320*2, 240*2);
	myLabel.setPixmap(QPixmap::fromImage(myImage));

	//create a FFC button
	QPushButton *button1 = new QPushButton("Calibrar", myWidget);
	button1->setGeometry((320*2)/2-100, (290*2)-35, 100, 30);
	
	//create a Snapshot button
	QPushButton *button2 = new QPushButton("Tirar foto", myWidget);
	button2->setGeometry((320*2)/2+50, (290*2)-35, 100, 30);

	//create a thread to gather SPI data
	//when the thread emits updateImage, the label should update its image accordingly
	LeptonThread *thread = new LeptonThread();
	QObject::connect(thread, SIGNAL(updateImage(QImage)), &myLabel, SLOT(setImage(QImage)));
	
	//connect ffc button to the thread's ffc action
	QObject::connect(button1, SIGNAL(clicked()), thread, SLOT(performFFC()));
	//connect snapshot button to the thread's snapshot action
	QObject::connect(button2, SIGNAL(clicked()), thread, SLOT(snapshot()));

	thread->start();
	
	myWidget->show();

	return a.exec();
}

