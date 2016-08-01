#include <stdio.h>
#include <string.h>
#include <sys/io.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>

#define POWERPORT 0x378
#define DATAPORT 0x378+2
#define DATAPIN 0  //STROBE
#define POWERPIN 0 //D0
#define DHT11_SET outb( inb(DATAPORT) & ~(1<<DATAPIN), DATAPORT)
#define DHT11_CLR outb( inb(DATAPORT) | (1<<DATAPIN), DATAPORT)
#define DHT11_INP !(inb(DATAPORT) & 1<<DATAPIN)
#define NOP10() asm("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop")
#define delay_us(x) for(tmpTime=0;tmpTime<250*(x);tmpTime++) NOP10();
#define DHT11_ERROR 255
int tmpTime;

void check_sleep()
{
	struct timeval start, end;
	long mtime, secs, usecs;
	gettimeofday(&start, NULL);
	delay_us(100);
	gettimeofday(&end, NULL);
	secs  = end.tv_sec  - start.tv_sec;
	usecs = end.tv_usec - start.tv_usec;
	mtime = ((secs) * 1000000 + usecs);
	printf("Elapsed time: %ld usecs\n", mtime);
}
void dht11_poweron()
{
	outb(1<<POWERPIN, POWERPORT);
}
void dht11_poweroff()
{
	outb(0, POWERPORT);
}

uint8_t dht11_getdata(uint8_t *bits) 
{
	uint8_t i,j = 0;
	memset(bits, 0, sizeof(bits));
	//reset port
	DHT11_SET;
	usleep(300000);
	//send request
	DHT11_CLR;
	usleep(18*1000);
	DHT11_SET;
	delay_us(40);	
	//check start condition 1
	if(DHT11_INP) 
	{
		printf("Start error1\n");
		return DHT11_ERROR;
	}
	delay_us(80);
	//check start condition 2
	if(!DHT11_INP) 
	{
		printf("Start error2\n");
		return DHT11_ERROR;
	}
	delay_us(80);

	//read the data
	for (j=0; j<5; j++) 
	{ //read 5 byte
		uint8_t result=0;
		for(i=0; i<8; i++) 
		{//read every bit
			while(!DHT11_INP); //wait for an high input
			delay_us(30);
			if(DHT11_INP) //if input is high after 30 us, get result
			{	result |= (1<<(7-i));}
			while(DHT11_INP); //wait until input get low

		}
		bits[j] = result;
	}
	//check checksum
	if (bits[0] + bits[1] + bits[2] + bits[3] == bits[4]) 
		return 0;
	return DHT11_ERROR;
}

// get temperature (0..50C)
float dht11_gettemperature() 
{
	uint8_t data[5];
	uint8_t ret = dht11_getdata(data);
	if(ret == DHT11_ERROR)
		return -1;
	else
		return data[2]+data[3]/10;
}

// get humidity (20..90%)
float dht11_gethumidity() 
{
	uint8_t data[5];
	uint8_t ret = dht11_getdata(data);
	if(ret == DHT11_ERROR)
		return -1;
	else
		return data[0]+data[1]/10;
}

int main()
{
	ioperm(POWERPORT,1,1);   //Data register for power pin
	ioperm(DATAPORT,1,1);    //Control register for data pin
	ioperm(0x0778+2,1,1);
	outb(inb(0x778+2)&0x1F, 0x778+2);       //Switch to standard mode
	printf("Temperatura: %f\n" ,  dht11_gettemperature());
	printf("Wilgotność: %f\n" ,  dht11_gethumidity());
}

