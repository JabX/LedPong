#include <c8051f310.h>

void SYSCLK_Init();
void sendData(unsigned char a, unsigned char d, int isLine);

sbit CLK 	= P0^0;
sbit DATA 	= P1^0;
sbit LOAD	= P1^1;

unsigned char init;

void main()
{
	PCA0MD &= ~0x40;

	OSCICN	= 0xc3;	// configure internal oscillator for its lowest frequency
	RSTSRC 	= 0x04;	// enable missing clock detector

	XBR1     = 0x40;
	P0MDOUT |= 0x01;
	P1MDOUT |= 0x03;

	DATA 	= 0;
	LOAD 	= 0;
	CLK 	= 0;

	sendData(0x0C, 0x00, 0); 	// shutdown
	sendData(0x0C, 0x01, 0); 	// Normal operation mode
	sendData(0x0A, 0x0F, 0); 	// Intensity
	sendData(0x0B, 0x07, 0); 	// No scan limit
	sendData(0x09, 0x00, 0); 	// No decode
	for (init = 0x01;init<=0x08;init++)
		sendData(init, 0x00, 1);

	sendData(0x01, 0x20, 1);
	sendData(0x07, 0xFE, 1);
	sendData(0x06, 0xF0, 1);
	sendData(0x04, 0xB0, 1);
	sendData(0x02, 0xCE, 1);

	while (1) {}
}

void sendData(unsigned char a, unsigned char d, int isLine)
{
	//EA = 0;
	unsigned char b;
	unsigned char p;

	LOAD = 0;
	CLK = 0;
	for(b=0x80; b>0; b=b>>1)
	{
		DATA = (a&b)?1:0;
		CLK = 1;
		CLK = 0;
	}

	if(isLine)
	{
		p = (d&0x01);
		d >>= 1;
		d &= 0x7F;
		if(p) d |= 0x80;
	}

	for(b=0x80; b>0; b=b>>1)
	{
		DATA = (d&b)?1:0;
		CLK = 1;
		CLK = 0;
	}
	LOAD = 1;
	//EA = 1;
}