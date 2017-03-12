#include "LeptonThread.h"
#include "Palettes.h"
#include "SPI.h"
#include "Lepton_I2C.h"


static const char *device = "/dev/spidev0.0";
static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 16000000;
int snapshotCount = 0;
static void pabort(const char *s)
{
	perror(s);
	abort();
}

LeptonThread::LeptonThread() : QThread()
{
SpiOpenPort(0);
}

LeptonThread::~LeptonThread() {
}

void LeptonThread::run()
{
	//create the initial image
	QRgb red = qRgb(255,0,0);
	myImage = QImage(160, 120, QImage::Format_RGB888);
	for(int i=0;i<160;i++) {
		for(int j=0;j<120;j++) {
			myImage.setPixel(i, j, red);
		}
	}
	int ret = 0;
	int fd;

	fd = open(device, O_RDWR);
	if (fd < 0)
	{
		pabort("can't open device");
	}

	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
	{
		pabort("can't set spi mode");
	}

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
	{
		pabort("can't get spi mode");
	}

	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
	{
		pabort("can't set bits per word");
	}

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
	{
		pabort("can't get bits per word");
	}

	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
	{
		pabort("can't set max speed hz");
	}

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
	{
		pabort("can't get max speed hz");
	}


	//open spi port
	emit updateImage(myImage);

	while(true) {
		int resets = 0;
		int segmentNumber = 0;
		for(int i = 0; i < NUMBER_OF_SEGMENTS; i++){
			for(int j=0;j<PACKETS_PER_SEGMENT;j++) {
				
				//read data packets from lepton over SPI
				read(spi_cs0_fd, result+sizeof(uint8_t)*PACKET_SIZE*(i*PACKETS_PER_SEGMENT+j), sizeof(uint8_t)*PACKET_SIZE);
				int packetNumber = result[((i*PACKETS_PER_SEGMENT+j)*PACKET_SIZE)+1];
				
				//if it's a drop packet, reset j to 0, set to -1 so he'll be at 0 again loop
				if(packetNumber != j) {
					j = -1;
					resets += 1;
					usleep(1000);
					continue;
					if(resets == 750) {
						SpiClosePort(0);
						qDebug() << "restarting spi...";
						usleep(5000);
						SpiOpenPort(0);
					}
				} else			
				if(packetNumber == 20) {
					//reads the "ttt" number
					segmentNumber = result[(i*PACKETS_PER_SEGMENT+j)*PACKET_SIZE] >> 4;
						//if it's not the segment expected reads again
						if(segmentNumber == 0)
							j = -1;
				}
			}		
			usleep(1000/106);
		}

		frameBuffer = (uint16_t *)result;
		int row, column;
		uint16_t value;
		uint16_t minValue = 65535;
		uint16_t maxValue = 0;

		
		for(int i=0;i<FRAME_SIZE_UINT16;i++) {
			//skip the first 2 uint16_t's of every packet, they're 4 header bytes
			if(i % PACKET_SIZE_UINT16 < 2) {
				continue;
			}
			
			//flip the MSB and LSB at the last second
			int temp = result[i*2];
			result[i*2] = result[i*2+1];
			result[i*2+1] = temp;
			
			value = frameBuffer[i];
			if(value> maxValue) {
				maxValue = value;
			}
			if(value < minValue) {
				if(value != 0)
					minValue = value;
			}		
		}
		
		std::cout << "Minima: " << raw2Celsius(minValue) <<" °C"<<std::endl;	
		std::cout << "Maximo: " << raw2Celsius(maxValue) <<" °C"<<std::endl;	
				
		float diff = maxValue - minValue;
		float scale = 255/diff;
		QRgb color;
		float valueCenter = 0;
	for(int k=0; k<FRAME_SIZE_UINT16; k++) {
			if(k % PACKET_SIZE_UINT16 < 2) {
				continue;
			}
			value = (frameBuffer[k] - minValue) * scale;

			
			const int *colormap = colormap_ironblack;
			color = qRgb(colormap[3*value], colormap[3*value+1], colormap[3*value+2]);
			
				//Left side of the screen
				if((k/PACKET_SIZE_UINT16) % 2 == 0){
					column = (k % PACKET_SIZE_UINT16 - 2);
					row = (k / PACKET_SIZE_UINT16)/2;
				}
				//Right side of the screen
				else{
					column = ((k % PACKET_SIZE_UINT16 - 2))+(PACKET_SIZE_UINT16-2);
					row = (k / PACKET_SIZE_UINT16)/2;
				}	
				//saves the pixel in the center of the screen
				if(column == 80 && row == 60)
					valueCenter = frameBuffer[k];
								
				myImage.setPixel(column, row, color);
				
		}
		//draws the square in the center of the screen
		drawSquare(valueCenter);
		//Emit the signal for update
		emit updateImage(myImage);
	}
	
	//Close SPI port
	SpiClosePort(0);
}

void LeptonThread::drawSquare(float value){
	//Draws the square
	QPainter paint(&myImage);
	paint.setPen(Qt::blue);
	paint.drawRect(79,59,2,2);
	
	//Writes the temperature
	paint.setPen(Qt::black);
	paint.setFont(QFont("Arial", 8));
	paint.drawText(82,62, QString("%1 C").arg(raw2Celsius(value)));
}

void LeptonThread::snapshot(){
	++snapshotCount;
	//Saves the photo
	myImage.save(QString("rgb%1.png").arg(snapshotCount), "PNG", 100);
}

void LeptonThread::performFFC() {
	//perform FFC
	lepton_perform_ffc();
}