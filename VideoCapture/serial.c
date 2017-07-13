/* SERIAL.C: An interrupt driven serial port buffer

			The following code shows how to take advantage of some of
			the Turbo C extensions to the C language to do asynchronous
			communications without having to write supporting assembly-
			language routines.

			This program bypasses the less-than-adequate PC BIOS
			communications routines and installs a serial interrupt
			handler. Direct access to PC hardware allows the program to
			run at faster baud rates and eliminates the need for
			the main program to continuously poll the serial port for
			data; thus implementing background communications. Data that
			enters the serial port is stored in a circular buffer.

			* Compile this program with Test Stack Overflow OFF.
*/

#include <dos.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include "serial.h"

#define VERSION 0x0101

#define FALSE           0
#define TRUE           (!FALSE)

#define NOERROR         0       // No error
#define BUFOVFL         1       // Buffer overflowed

#define ESC             0x1B    // ASCII Escape character
#define ASCII           0x007F  // Mask ASCII characters
#define SBUFSIZ         0x4000  // Serial buffer size

int            SError          = NOERROR;
int            portbase        = 0;

// A user defined type for use with getvect() & setvect()
typedef void interrupt far (*intvect)(...);

intvect 			 oldvects[2];

static   char  ccbuf[SBUFSIZ];
unsigned int   startbuf        = 0;
unsigned int   endbuf          = 0;

/*-------------------------------------------------------------------
	 com_int - Handle communications interrupts and put them in ccbuf
*/

void interrupt com_int(void) {
	disable();
	if ((inportb(portbase + IIR) & RX_MASK) == RX_ID) {
		if (((endbuf + 1) & SBUFSIZ - 1) == startbuf)
			SError = BUFOVFL;

			ccbuf[endbuf++] = inportb(portbase + RXR);
			endbuf &= SBUFSIZ - 1;
	}

	// Signal end of hardware interrupt
	outportb(ICR, EOI);
	enable();
} // end of com_int()

/*-------------------------------------------------------------------
	SerialOut - Output a character to the serial port
*/

int SerialOut(char x) {
	long int timeout = 0x0000FFFFL;

	outportb(portbase + MCR,  MC_INT | DTR | RTS);

	// Wait for Clear To Send from modem
	while ((inportb(portbase + MSR) & CTS) == 0)
		if (!(--timeout))
			return (-1);

		timeout = 0x0000FFFFL;

		// Wait for transmitter to clear
		while ((inportb(portbase + LSR) & XMTRDY) == 0)
			if (!(--timeout))
				return (-1);

		disable();
		outportb(portbase + TXR, x);
		enable();

		return (0);
} // end of serialOut()

/*-------------------------------------------------------------------
	 SerialString - Output a string to the serial port
*/

void SerialString(char *string) {
	while (*string)
		SerialOut(*string++);
} // end of SerailString()

/*-------------------------------------------------------------------
	 getccb - This routine returns the current value in the buffer
*/

int getccb(void) {
	int res;

	if (endbuf == startbuf)
		return (-1);

	res = (int) ccbuf[startbuf++];
	startbuf %= SBUFSIZ;
	return (res);
} // end of getccb()

/*-------------------------------------------------------------------
	 setvects - Install our functions to handle communications
*/

void setvects(void) {
	oldvects[0] = (intvect) getvect(0x0B);
	oldvects[1] = (intvect) getvect(0x0C);
	setvect(0x0B, (intvect)com_int);
	setvect(0x0C, (intvect)com_int);
} // end of setvects()

/*-------------------------------------------------------------------
	 resvects - Uninstall our vectors before exiting the program
*/

void resvects(void) {
	setvect(0x0B, (intvect)oldvects[0]);
	setvect(0x0C, (intvect)oldvects[1]);
} // end of resvects()

/*-------------------------------------------------------------------
	 i_enable - Turn on communications interrupts
*/

void i_enable(int pnum) {
	int c;

	disable();
	c = inportb(portbase + MCR) | MC_INT;
	outportb(portbase + MCR, c);
	outportb(portbase + IER, RX_INT);
	c = inportb(IMR) & (pnum == COM1 ? IRQ4 : IRQ3);
	outportb(IMR, c);
	enable();
} // end of i_enable()

/*-------------------------------------------------------------------
		i_disable - Turn off communications interrupts
*/

void i_disable(void) {
	int c;

	disable();
	c = inportb(IMR) | ~IRQ3 | ~IRQ4;
	outportb(IMR, c);
	outportb(portbase + IER, 0);
	c = inportb(portbase + MCR) & ~MC_INT;
	outportb(portbase + MCR, c);
	enable();
} // end of i_disable()

/*-------------------------------------------------------------------
	 comm_on - Tell modem that we're ready to go
*/

void comm_on(void) {
	int c, pnum;

	pnum = (portbase == COM1BASE ? COM1 : COM2);
	i_enable(pnum);
	c = inportb(portbase + MCR) | DTR | RTS;
	outportb(portbase + MCR, c);
} // end of comm_on()

/*-------------------------------------------------------------------
	 comm_off - Go off-line
*/

void comm_off(void) {
	i_disable();
	outportb(portbase + MCR, 0);
} // end of comm_off()

/*-------------------------------------------------------------------
	 initserial -
*/

void initserial(void) {
	endbuf = startbuf = 0;
	setvects();
	comm_on();
} // end of initserial()

/*-------------------------------------------------------------------
	 closeserial -
*/

void closeserial(void) {
	comm_off();
	resvects();
} // end of closerial()

/*-------------------------------------------------------------------
	 SetPort - Set the port number to use
*/

int SetPort(int Port) {
	int Offset, far *RS232_Addr;

	switch (Port) { // Sort out the base address
		case COM1 : Offset = 0x0000;
								break;
		case COM2 : Offset = 0x0002;
								break;
		default   : return (-1);
	}

	// Find out where the port is.
	RS232_Addr = (int far*) MK_FP(0x0040, Offset);
	if (*RS232_Addr == NULL)
		return (-1);											 // If NULL then port not used.
	portbase = *RS232_Addr;              // Otherwise set portbase

	return (0);
} // end of SetPort()

/*-------------------------------------------------------------------
	 SetSpeed - This routine sets the speed; will accept funny baud
							rates. Setting the speed requires that the DLAB be set
							on.
*/

int SetSpeed(int Speed) {
	char c;
	int divisor;

	if (Speed == 0)            // Avoid divide by zero
		return (-1);
	else
		divisor = (int) (115200L/Speed);

	if (portbase == 0)
		return (-1);

	disable();
	c = inportb(portbase + LCR);
	outportb(portbase + LCR, (c | 0x80)); // Set DLAB
	outportb(portbase + DLL, (divisor & 0x00FF));
	outportb(portbase + DLH, ((divisor >> 8) & 0x00FF));
	outportb(portbase + LCR, c);          // Reset DLAB
	enable();
	return (0);
} // end of SetSpeed()

/*-------------------------------------------------------------------
	 SetOthers - Set other communications parameters
*/

int SetOthers(int Parity, int Bits, int StopBit) {
	int setting;

	if (portbase == 0)
		return (-1);
	if (Bits < 5 || Bits > 8)
		return (-1);
	if (StopBit != 1 && StopBit != 2)
		return (-1);
	if (Parity != NO_PARITY && Parity != ODD_PARITY && Parity != EVEN_PARITY)
		return (-1);

	setting = Bits-5;
	setting |= ((StopBit == 1) ? 0x00 : 0x04);
	setting |= Parity;

	disable();
	outportb(portbase + LCR, setting);
	enable();
	return (0);
} // end of SetOthers()

/*-------------------------------------------------------------------
	 SetSerial - Set up the port
*/

int SetSerial(int Port, int Speed, int Parity, int Bits, int StopBit) {
	if (SetPort(Port))                    return (-1);
	if (SetSpeed(Speed))                  return (-1);
	if (SetOthers(Parity, Bits, StopBit)) return (-1);
	return 0;
} // end of SetSerial()

/*
	 c_break - Control-Break interrupt handler
*/

int c_break(void) {
	i_disable();
	fprintf(stderr, "\nStill online.\n");
	return 0;
} // end of c_bresk()

//*******************************************************************
main()
{
	// Communications parameters
	int        port     = COM1;
	int        speed    = 1200;
	int        parity   = NO_PARITY;
	int        bits     = 7;
	int        stopbits = 1;

	int        c, done  = FALSE;

	if (SetSerial(port, speed, parity, bits, stopbits) != 0) {
		fprintf(stderr, "Serial Port setup error.\n");
		return (99);
	}

	initserial();
	ctrlbrk(c_break);

	fprintf(stdout, "TURBO C TERMINAL\n"
									"...You're now in terminal mode, "
									"press [ESC] to quit...\n\n");

	/* The main loop acts as a dumb terminal. We repeatedly
		 check the keyboard buffer, and communications buffer. */
	do {
		if (kbhit()) {
			// Look for an Escape key
			switch (c=getch()) {
				case ESC:
					done = TRUE;  // Exit program
					break;

					//* You may want to handle other keys here...
			}

			if (!done)
				SerialOut(c);
		}
		if ((c=getccb()) != -1)
			fputc(c & ASCII, stdout);

	} while (!done && !SError);

	// Check for errors
	switch (SError) {
		case NOERROR:
			fprintf(stderr, "\nbye.\n");
			closeserial();
			return 0;

		case BUFOVFL:
			fprintf(stderr, "\nBuffer Overflow.\n");
			closeserial();
			return (99);

		default:
			fprintf(stderr, "\nUnknown Error, SError = %d\n", SError);
			closeserial();
		 return (99);
	}
} // end of main()
