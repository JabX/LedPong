#include <c8051f310.h>

void SYSCLK_Init();
void init_display();
void clear_display();
void display_matrix(int[8][8]);
void write_data(unsigned char a, unsigned char d);
void sendData(unsigned char a, unsigned char d, int isLine);
void test_matrix();

sbit CLK 	= P0^0;
sbit DATA 	= P1^0;
sbit LOAD	= P1^1;

unsigned char init;
xdata int m[8][8];

void main()
{
	PCA0MD &= ~0x40;

	OSCICN	= 0xc3;	// configure internal oscillator for its lowest frequency
	RSTSRC 	= 0x04;	// enable missing clock detector

	XBR1     = 0x40;
	P0MDOUT |= 0x01;
	P1MDOUT |= 0x03;

	init_display();

	//write_data(0x01, 0x20);
	//write_data(0x07, 0xFE);
	//write_data(0x06, 0xF0);
	//write_data(0x04, 0xB0);
	//write_data(0x02, 0xCE);

	test_matrix();

	while (1) {}
}

void test_matrix()
{
	int i;
	int j;

	for(i=0; i<8; i++)
	{
		for(j=0; j<8; j++)
		{
			m[i][j]=0;
		}
	}

	m[4][5] = 1;
	m[1][4] = 1;
	display_matrix(m);
}

void init_display()
{
	DATA 	= 0;
	LOAD 	= 0;
	CLK 	= 0;

	sendData(0x0C, 0x00, 0); 	// shutdown
	sendData(0x0C, 0x01, 0); 	// Normal operation mode
	sendData(0x0A, 0x0F, 0); 	// Intensity
	sendData(0x0B, 0x07, 0); 	// No scan limit
	sendData(0x09, 0x00, 0); 	// No decode
	clear_display();
}

void clear_display()
{
	for (init = 0x01;init<=0x08;init++)
		sendData(init, 0x00, 1);
}

void write_data(unsigned char a, unsigned char d)
{
	sendData(a, d, 1);
}

void display_matrix(int m[8][8])
{
	int i;
	int j;
	int k;
	unsigned char a;
	unsigned char d;
	unsigned char b;
	for (i=0; i<8; i++)
	{
		k = i+1;
		a = '0' + k;
		d = 0x00;
		b = 0x80;
		for (j=0; j<8; j++)
		{
			if (m[i][j])
				d |= b;
			b=b>>1;
		}
		write_data(a, d);
	}
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
