#include <c8051f310.h>

void SYSCLK_Init();
void sendData(unsigned char a, unsigned char d);

sbit CLK       = P0^0;
sbit DATA	   = P1^0;
sbit LOAD	   = P1^1;

void main()
{
	PCA0MD &= ~0x40;

	XBR1     = 0x40;
	P0MDOUT |= 0x01;
	P1MDOUT |= 0x03;
	SYSCLK_Init();
	DATA = 0;
	LOAD = 0;
	CLK = 0;

	while (1)
	{
		sendData(0x0F, 0xFF);
	}
}

void SYSCLK_Init ()
{
   OSCICN = 0xc3;                         // configure internal oscillator for its lowest frequency
   RSTSRC = 0x04;                         // enable missing clock detector
}

void sendData(unsigned char a, unsigned char d)
{
	//EA = 0;
	unsigned char b;

	LOAD = 0;
	for(b=0x80; b>0; b=b>>1)
	{
		//DATA = (a&b)?1:0;
		DATA = 1;
		CLK = 0;
		CLK = 1;
	}
	for(b=0x80; b>0; b=b>>1)
	{
		//DATA = (d&b)?1:0;
		DATA = 1;
		CLK = 0;
		CLK = 1;
	}
	LOAD = 1;
	//EA = 1;
}
