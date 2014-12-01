#include <c8051f310.h>
#include <stdlib.h>
#include <math.h>

// Same clock, same data for all 4 panels, but different loads
sbit CLK 	= P0^0;
sbit DATA	= P1^0;
sbit LOAD1	= P1^3;
sbit LOAD2	= P1^4;
sbit LOAD3	= P1^1;
sbit LOAD4	= P1^2;

sbit J1UP	= P2^1;
sbit J1DOWN	= P2^0;
sbit J2UP 	= P2^3;
sbit J2DOWN	= P2^2;

// Timer 2 definitions
sfr16 TMR2RL   = 0xca;	// Reload value
sfr16 TMR2     = 0xcc;  // Counter

xdata char m[16][16];	// Matrix

char isFrame = 0;		// True : draw frame

char paddleL;
char paddleR;
char paddleSize = 2;	// True size = 1 + 2*paddleSize

char scoreL[8] = (0, 0, 0, 0, 0, 0, 0, 0);
char scoreR[8] = (0, 0, 0, 0, 0, 0, 0, 0);

unsigned char ball[2]; 	// *10 to avoid using floats
char ballSpeed = 50;
char ballYway = 1;
char deltaX;
char deltaY;
char angle;

unsigned char init;		// Byte iterator
char i;					// Int iterator
char j;					// Int iterator

void collide(char, char);
char round10(char);
void drawPaddle(char, char);
void clearPaddles();

void setAngle(char);
void clearBall();
void moveBall();
void drawBall();
void incScore(char[8]);
void drawScore();

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

void writeData(char, unsigned char, unsigned char);	// Set all LEDs on matrix 'char'
void sendDataAll(unsigned char, unsigned char); 	// Send a non-LED message to all matrixes
void sendData1(unsigned char, unsigned char, char);
void sendData2(unsigned char, unsigned char, char);
void sendData3(unsigned char, unsigned char, char);
void sendData4(unsigned char, unsigned char, char);
void sendData(unsigned char, unsigned char, char);


void main()
{
	initTimer2(65535); // Max timer counts
	initMIC();
	initDisplay();

	paddleL = 7;
	paddleR = 7;

	ball[0] = 70;
	ball[1] = 70;
	setAngle(0);

	while (1)
	{
		if(isFrame)
		{
			unsigned char ballx;
			unsigned char bally;

		 	isFrame = 0;

			if(J1DOWN == 0 && paddleL > paddleSize)
				paddleL--;
			if(J1UP == 0 && paddleL < 14 - paddleSize)
				paddleL++;

			if(J2DOWN == 0 && paddleR > paddleSize)
				paddleR--;
			if(J2UP == 0 && paddleR < 14 - paddleSize)
				paddleR++;

			drawPaddle(0,0);
			drawPaddle(1,15);
			moveBall();

			ballx = round10(ball[0]);
			bally = round10(ball[1]);

			if(bally == 0 && (ballx < paddleL - paddleSize - (angle > 2) || ballx > paddleL + paddleSize + (angle > 2)))
			{
				incScore(scoreR);
				clearBall();
				ball[0] = 70;
				ball[1] = 40;
				setAngle(0);
				ballYway = 1;
				clearPaddles();
				paddleL = 7;
				paddleR = 7;
			}
			else if (bally == 0)
				collide(ballx, paddleL);

			if(bally == 15 && (ballx < paddleR - paddleSize || ballx > paddleR + paddleSize))
			{
				incScore(scoreL);
				clearBall();
				ball[0] = 70;
				ball[1] = 110;
				setAngle(0);
				ballYway = -1;
				clearPaddles();
				paddleL = 7;
				paddleR = 7;
			}
			else if (bally == 15)
				collide(ballx, paddleR);

			drawScore();
		 	displayMatrix();
		}
	}
}

////////////////////
//// Game logic ////
////////////////////

// Updates ball angle on collision with a paddle (left or right)
void collide(unsigned char ballx, char paddlePosition)
{
	// 6 is the maximum angle the ball can have
	if (ballx < paddlePosition)
		setAngle(((angle - (paddlePosition - ballx)) < -6) ? -6 : angle - (paddlePosition - ballx));
	else if (ballx > paddlePosition)
		setAngle(((angle + (ballx - paddlePosition)) > 6) ? 6 : angle + (ballx - paddlePosition));
}

char round10(unsigned char value)
{
	char result;
	result = value / 10;
	result += ((value - result * 10) < 5) ? 0 : 1;

	return result;
}

void clearPaddles()
{
	for(i = 0; i < 15 ; i++)
	{
		m[i][0] = 0;
		m[i][15] = 0;
	}
}

void drawPaddle(char n, char col)
{
	char paddle;
	char oobb;
	char oobt;

	if(!n) paddle = paddleL; else paddle = paddleR;

	oobb = paddle - paddleSize - 1;
	oobt = paddle + paddleSize + 1;

	if(oobb < 0) oobb = 0;
	if(oobt > 14) oobt = 14;

	m[oobb][col] = 0;
	m[oobt][col] = 0;

	for(i = paddle - paddleSize; i <= paddle + paddleSize; i++)
		m[i][col] = 1;
}

void setAngle(char a)
{
	deltaX = a;
	deltaY = sqrt(ballSpeed - deltaX*deltaX);
	angle = a;
}

void clearBall()
{
	m[round10(ball[0])][round10(ball[1])] = 0;
}

void moveBall()
{
	clearBall();

	if (ball[0] < 5 || ball[0] >= 135)
		setAngle(-angle);

	if (ball[1] < 5)
		ballYway = 1;
	else if (ball[1] >= 145)
		ballYway = -1;

	ball[0] += deltaX;
	ball[1] += ballYway*deltaY;

	// The ball array is an unsigned char, and the previous calculation may result in a negative number
	// The following lines correct this.
	if(ball[0] > 200) ball[0] = 0;
	if(ball[1] > 200) ball[1] = 0;

	drawBall();
}

void drawBall()
{
	m[round10(ball[0])][round10(ball[1])] = 1;
}

void incScore(char score[8])
{
	if(!score[1])
	{
		char remainder = 1;
		char digit = 7;
		while (remainder)
		{
			if(score[digit] == 0)
			{
				score[digit] = 1;
				remainder = 0;
			}
			else
			{
				score[digit] = 0;
				digit--;
			}
		}
	}
}

void drawScore()
{
	for(j = 0; j < 8; j++)
	{
		m[15][j] = scoreL[7-j];
		m[15][j+8] = scoreR[j];
	}
}

/////////////////////
//// LOWER-LEVEL ////
/////////////////////

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
	sendDataAll(0x0F, 0x00);	// Test mode off
	sendDataAll(0x0C, 0x01); 	// Normal operation mode
	sendDataAll(0x0A, 0x0F); 	// Max intensity
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
	char k;
	unsigned char a;
	unsigned char d;
	unsigned char b;

	for (i = 0; i < 16; i++)
	{
		k = (i % 8) + 1;	// Lines are 0-7 / 8-15 in our matrix and 1-8 in the driver
		a = '0' + k;		// Formatting as a hex char

		// Left matrixes
		d = 0x00;			// Data
		b = 0x80; 			// Mask
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

void writeData(char n, unsigned char a, unsigned char d)
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

void sendData1(unsigned char a, unsigned char d, char isLine)
{
	LOAD1 = 0;
	sendData(a, d, isLine);
	LOAD1 = 1;
}

void sendData2(unsigned char a, unsigned char d, char isLine)
{
	LOAD2 = 0;
	sendData(a, d, isLine);
	LOAD2 = 1;
}

void sendData3(unsigned char a, unsigned char d, char isLine)
{
	LOAD3 = 0;
	sendData(a, d, isLine);
	LOAD3 = 1;
}

void sendData4(unsigned char a, unsigned char d, char isLine)
{
	LOAD4 = 0;
	sendData(a, d, isLine);
	LOAD4 = 1;
}

void sendData(unsigned char a, unsigned char d, char isLine)
{
	unsigned char b;
	unsigned char p;

	EA = 0;
	CLK = 0;

	for(b = 0x80; b > 0; b = b >> 1)
	{
		DATA = (a & b) ? 1 : 0;
		CLK = 1;
		CLK = 0;
	}

	// Line messages are different from regular messages because the 8th LED is at the wrong end
	if(isLine)
	{
		p = (d & 0x01);
		d >>= 1;
		d &= 0x7F;
		if(p) d |= 0x80;
	}

	for(b = 0x80; b > 0; b = b >> 1)
	{
		DATA = (d & b) ? 1 : 0;
		CLK = 1;
		CLK = 0;
	}

	EA = 1;
}

void initTimer2(int counts)
{
	TMR2CN	= 0x00;
	CKCON  &= ~0x60;
	TMR2RL 	= -counts;
	TMR2   	= 0xffff;
	ET2    	= 1;
	TR2    	= 1;
}

void timer2_ISR() interrupt 5
{
	TF2H = 0;
	isFrame = 1;
}
