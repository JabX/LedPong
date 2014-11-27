#include <c8051f310.h>
#include <stdlib.h>

// SFR16 definitions
sfr16 TMR2RL   = 0xca;                    // Timer2 reload value
sfr16 TMR2     = 0xcc;                    // Timer2 counter

// Same clock, same data for all 4 panels, but different loads
sbit CLK 	= P0^0;
sbit DATA	= P1^0;
sbit LOAD1	= P1^1;
sbit LOAD2	= P1^2;
sbit LOAD3	= P1^3;
sbit LOAD4	= P1^4;

xdata char m[16][16];	// Matrix
int isFrame = 0;		// True : draw frame

int paddleL = 3;
int paddleR = 4;
int paddleSize = 2;		// True size = 1 + 2*paddleSize

int scoreL = 0;
int scoreR = 0;

int ball[2] = (7,7*10);		// *10 in order to have subpositions for the ball, without using floats (10 subpositions per led)
int ballXway = 1;
int ballYway = 1;
int angle = 0;		// not a real angle, juste an Y movement at each frame

unsigned char init;		// Byte iterator
int i;					// Int iterator
int j;					// Int iterator


void drawPaddle(int, int);

void clearBall();
void moveBall();
void drawBall();

void initMIC();

void initTimer2(int);
void timer2_ISR();

void initDisplay();
void clearDisplay();

void clearMatrix();
void displayMatrix();

// Matrices definition
//  	3	4
//		1	2

void writeData(int, unsigned char, unsigned char);	// Set all LEDs on matrix 'int'
void sendDataAll(unsigned char, unsigned char); 	// Send a non-LED message to all matrixes
void sendData1(unsigned char, unsigned char, int);
void sendData2(unsigned char, unsigned char, int);
void sendData3(unsigned char, unsigned char, int);
void sendData4(unsigned char, unsigned char, int);
void sendData(unsigned char, unsigned char, int);


void main()
{
	int R1;
	int R2;

	initTimer2(1);
	initMIC();
	initDisplay();

	while (1)
	{
		if(isFrame)
		{
			int Ldir = 1;
			int Rdir = -1;

			isFrame = 0;
			paddleL += Ldir;
			Ldir = -Ldir;
			paddleR += Rdir;
			Rdir = -Rdir;
			drawPaddle(0,0);
			drawPaddle(1,7);
			moveBall();

			displayMatrix();
		}
	}
}

void initMIC()
{
	PCA0MD	&= ~0x40;	// Turn off watchdog

	OSCICN	 = 0xc3;	// Configure internal oscillator for its lowest frequency
	RSTSRC 	 = 0x04;	// Enable missing clock detector

	XBR1     = 0x40;	// Enable crossbar
	P0MDOUT |= 0x01;	// Push-pull for P0.0
	P1MDOUT |= 0x1F;	// Push-pull for P1.0 -> P1.4

	EA = 1; 			// Enable interruptions
}

void initDisplay()
{
	CLK 	= 0;
	DATA 	= 0;
	LOAD1 	= 0;
	LOAD2 	= 0;
	LOAD3 	= 0;
	LOAD4 	= 0;

	sendDataAll(0x0C, 0x00); 	// Shutdown
	sendDataAll(0x0C, 0x01); 	// Normal operation mode
	sendDataAll(0x0A, 0x0F); 	// Intensity
	sendDataAll(0x0B, 0x07); 	// No scan limit
	sendDataAll(0x09, 0x00); 	// No decode

	clearDisplay();
}


void clearMatrix()
{
	for(i = 0; i <= 16; i++)
		for(j = 0; j <= 16; j++)
			m[i][j] = 0;
}

void clearDisplay()
{
	clearMatrix();
	displayMatrix();
}

void displayMatrix()
{
	int k;
	unsigned char a;
	unsigned char d;
	unsigned char b;

	for (i = 0; i < 16; i++)
	{
		k = i + 1;
		a = '0' + k;

		// Left matrixes
		d = 0x00;
		b = 0x80;
		for (j = 0; j < 8; j++)
		{
			if(m[i][j])
				d |= b;
			b = b >> 1;
		}
		if(i < 8)
			writeData(1, a, d);
		else
			writeData(3, a, d);

		// Right matrixes
		d = 0x00;
		b = 0x80;
		for (j = 8; j < 16; j++)
		{
			if(m[i][j])
				d |= b;
			b = b >> 1;
		}
		if(i < 8)
			writeData(2, a, d);
		else
			writeData(4, a, d);
	}
}

void writeData(int n, unsigned char a, unsigned char d)
{
	switch(n)
	{
	case 1:
		sendData1(a, d, 1);
		break;
	case 2:
		sendData2(a, d, 1);
		break;
	case 3:
		sendData3(a, d, 1);
		break;
	case 4:
		sendData4(a, d, 1);
		break;
	}
}

void sendDataAll(unsigned char a, unsigned char d)
{
	sendData1(a, d, 0);
	sendData2(a, d, 0);
	sendData3(a, d, 0);
	sendData4(a, d, 0);
}

void sendData1(unsigned char a, unsigned char d, int isLine)
{
	LOAD1 = 0;
	sendData(a, d, isLine);
	LOAD1 = 1;
}

void sendData2(unsigned char a, unsigned char d, int isLine)
{
	LOAD2 = 0;
	sendData(a, d, isLine);
	LOAD2 = 1;
}

void sendData3(unsigned char a, unsigned char d, int isLine)
{
	LOAD3 = 0;
	sendData(a, d, isLine);
	LOAD3 = 1;
}

void sendData4(unsigned char a, unsigned char d, int isLine)
{
	LOAD4 = 0;
	sendData(a, d, isLine);
	LOAD4 = 1;
}

void sendData(unsigned char a, unsigned char d, int isLine)
{
	unsigned char b;
	unsigned char p;

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
}

void initTimer2(int counts)
{
	TMR2CN = 0x00;
	CKCON  &= ~0x60;
	TMR2RL = -counts;
	TMR2   = 0xffff;
	ET2    = 1;
	TR2    = 1;
}

void timer2_ISR() interrupt 5
{
	TF2H = 0;
	isFrame = 1;
}


//////////////////////
///// Game logic /////
//////////////////////

void drawPaddle(int n, int col)
{
	int paddle;
	int oobb;
	int oobt;

	if(!n) paddle = paddleL; else paddle = paddleR;

	oobb = paddle - paddleSize - 1;
	oobt = paddle + paddleSize + 1;

	if(oobb < 0) oobb = 0;
	if(oobt > 14) oobt = 14;

	m[oobb][col] = 0;
	m[oobt][col] = 0;

	for(i = paddle - paddleSize; i <= paddle + paddleSize; i++)
	{
		m[i][col] = 1;
	}
}

void clearBall()
{
	m[ball[0]/10][ball[1]/10] = 0;
}

void moveBall()
{
	clearBall();
	// Test movements, no real collision yet
	if (ball[0] <= 1 || ball[0] >= 6)
	{
		ballXway = -ballXway;
	}
	if (ball[1] <= 10 || ball[1] >= 60)
	{
		ballYway = -ballYway;
	}
	ball[0] += ballXway;
	ball[1] += angle*ballYway;
	drawBall();
}

void drawBall()
{
	m[ball[0]/10][ball[1]/10] = 1;
}
