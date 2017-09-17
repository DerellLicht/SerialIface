#include "stdint.h"
#include <windows.h>
#include <process.h> //  _beginthreadex()
#include <stdio.h>

#include "common.h"
#include "serial_enum.h"

//******************************************************************************
static HANDLE h_com_port = 0;
// static HANDLE h_rx_event = 0;

static HANDLE h_uart_rx_thread;
// static HANDLE h_run_rx_thread;

static unsigned abort_thread = 0 ;

//****************************************************************************
int send_serial_msg(char *txbfr, uint txbfr_len)
{
   int dwWritten = 0;

   OVERLAPPED osWrite;
   ZeroMemory (&osWrite, sizeof (OVERLAPPED));
   HANDLE writeEvent = CreateEvent(0, TRUE, FALSE, 0);
   osWrite.hEvent = writeEvent;

   // Issue write.
   // bool fRes = false ;
   if (!WriteFile(h_com_port, txbfr, txbfr_len, (DWORD *) &dwWritten, &osWrite)) {
      if (GetLastError() != ERROR_IO_PENDING) {
      // if (GetLastError() != ERROR_WRITE_FAULT) {
         // WriteFile failed, but isn't delayed. Report error and abort.
         dwWritten = GetLastError() ;
         syslog("WriteFile: %s\n", get_system_message(dwWritten)) ;
         dwWritten = -dwWritten ;
         // fRes = false;
      }
      else {
         // Write is pending.
         // DWORD dwRes = WaitForSingleObject(osWrite.hEvent, INFINITE);
         
         uint wait_seconds = 0 ;
         BOOL done = FALSE ;
         while (!done) {
            DWORD dwRes = WaitForSingleObject(osWrite.hEvent, 1000);
            switch(dwRes) {
            case WAIT_TIMEOUT:
               if (++wait_seconds > 5) {
                  syslog("send_serial_msg: write did not complete!\n") ;
                  dwWritten = -(int) WAIT_TIMEOUT ;
                  done = TRUE ;
               }
               break;

            // OVERLAPPED structure's event has been signaled.
            case WAIT_OBJECT_0:
               if (!GetOverlappedResult(h_com_port, &osWrite, (DWORD *) &dwWritten, FALSE)) {
                  dwWritten = GetLastError() ;
                  syslog("GetOverlappedResult: %s\n", get_system_message(dwWritten)) ;
                  dwWritten = -dwWritten ;
               }
               else {
                  // Write operation completed successfully.
                  // syslog("Write: %u bytes sent\n", dwWritten) ;
                  // fRes = true;
               }
               done = TRUE ;
               break;

            default:
               // An error has occurred in WaitForSingleObject.
               // This usually indicates a problem with the
               // OVERLAPPED structure's event handle.
               dwWritten = GetLastError() ;
               syslog("Write (W4SO): %s\n", get_system_message(dwWritten)) ;
               dwWritten = -dwWritten ;
               done = TRUE ;
               break;
            }  //  switch()
         }
      }
   }
   // else {
      // syslog ("WriteFile succeeded...\n");
      // WriteFile completed immediately.
      // fRes = true;
   // }

   CloseHandle (writeEvent);
   return dwWritten;
}

//******************************************************************************
int send_serial_msg_ascii(char *txbfr)
{
   return send_serial_msg(txbfr, strlen(txbfr));
}

//******************************************************************************
// ignore serial port COM6 on name scans
// Port   Opens  Enums  Device description
// =====  =====  =====  ============================
// COM6    YES    YES   USB Serial Port (COM6)                  // debug_port
// COM4    YES    YES   USB Serial Port (COM4)                  // input_port ("COMn")
// COM10   YES    no    TI CC2540 USB CDC Serial Port (COM10)   // BLE dongle
// COM3    YES    no    Cypress USB UART (COM3)                 // debug-board port
//******************************************************************************
static char input_port_str[10] ;
static unsigned serial_uart_num = 0 ;

void select_serial_ports(void)
{
   unsigned port_num ;
   //***********************************************************************************
   //  Try to find the Reader serial port.
   //  Be aware that if the debug-board monitor port is connected,
   //  it will have the same description as this port.
   //  In that case, user will need to either block the monitor port number
   //  via --debug-port argument, or just specify this port using the --input-port argument.
   //***********************************************************************************
   port_num = get_serial_port_number(L"USB Serial Port");
   if (port_num != 0) {
      printf("found input port=%u\n", port_num);
      sprintf(input_port_str, "COM%u", port_num);
      // options->input_port = input_port_str ; 
   }
   //***********************************************************************************
   //  Try to find the serial UART port 
   //***********************************************************************************
   port_num = get_serial_port_number(L"Cypress USB UART");
   if (port_num != 0) {
      printf("found serial UART port=%u\n", port_num);
      serial_uart_num = port_num ; 
      // sprintf(input_port_str, "COM%u", port_num);
      // options->input_port = input_port_str ; 
   }
}  

//*************************************************************************************
uint bytes_received = 0 ;
static void buffer_rx_char(char readBuffer)
{
   putchar(readBuffer);
   bytes_received++ ;
}

//*************************************************************************************
static unsigned __stdcall uart_rx_thread(void *p_args)
{
   char readBuffer;
   DWORD numRead;
   DWORD s;
   OVERLAPPED overlappedRead;
   HANDLE readEvent ;
   
   // Set up for overlapped reading and writing on the port
   overlappedRead.Offset = overlappedRead.OffsetHigh = 0;
   readEvent = CreateEvent(0, TRUE, FALSE, 0);
   overlappedRead.hEvent = readEvent;

   // Prime the pump by getting the read process started
   ReadFile (h_com_port, &readBuffer, 1, &numRead, &overlappedRead);
   do {
      // Wait for either a keystroke or a modem character.
      // Time out after 1 second
      // s = WaitForMultipleObjects (2, handles, false, 1000000);
      // OutputDebugStringA("enter WaitForSingleObject\n") ;
      s = WaitForSingleObject(readEvent, 1000);   //  serial only
      if (abort_thread) 
         break;
      // wsprintfA(tempstr, "s=%u\n", (unsigned) s) ;
      // OutputDebugStringA(tempstr) ;
      // If it is a character from the keyboard then...
      if (s == WAIT_OBJECT_0) {
         // read and buffer RX character
         // success = GetOverlappedResult (this_port->comHandle, &overlappedRead, &numRead, true);
         GetOverlappedResult (h_com_port, &overlappedRead, &numRead, TRUE);
         overlappedRead.Offset += numRead;
         buffer_rx_char (readBuffer);
         // WriteFile (consoleStdout, &readBuffer, 1, &numWrite, 0);
         ResetEvent (readEvent);
         // Wait for the next character
         // from the comm port
         ReadFile (h_com_port, &readBuffer, 1, &numRead, &overlappedRead);
      }
      //  timeout here is just so we can detect shutdown request
      //  (rx_running gets cleared)
      // else if (s == WAIT_TIMEOUT) {
      //    // OutputDebugStringA("W4MO: timeout\n") ;
      //    continue;
      // }
   } while (abort_thread == 0);
   CloseHandle (readEvent);
   return 0;   
}

//*****************************************************************************
void abort_serial_uart_thread(void)
{
   abort_thread = 1 ;
}

//*****************************************************************************
uint32_t uart_init(unsigned port_num, uint32_t baud_rate)
{
	// char sz_com_port[16];
	DCB dcb;
	// DWORD dwEvtMask;
	COMMTIMEOUTS commTimeouts;
   
   TCHAR *port_ptr = get_dev_path(port_num);
   if (port_ptr == NULL) {
      return 1 ;
   }
	h_com_port = CreateFile(port_ptr, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 
                           FILE_FLAG_OVERLAPPED, 0);
	if (INVALID_HANDLE_VALUE == h_com_port)
	{
		return 2;
	}

	if (!SetupComm(h_com_port, 10240, 10240))
	{
		CloseHandle(h_com_port);
		return 3;
	}

	ZeroMemory(&dcb, sizeof(dcb));
	dcb.DCBlength = sizeof(dcb);
	dcb.BaudRate = baud_rate;
	dcb.fBinary = TRUE;
	dcb.ByteSize = 8;
	dcb.StopBits = ONESTOPBIT;
	if (!SetCommState(h_com_port, &dcb))
	{
		CloseHandle(h_com_port);
		return 4;
	}

	commTimeouts.ReadIntervalTimeout = MAXDWORD;
	commTimeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
	commTimeouts.ReadTotalTimeoutConstant = 5000;
	commTimeouts.WriteTotalTimeoutMultiplier = 0;
	commTimeouts.WriteTotalTimeoutConstant = 0;
	if (!SetCommTimeouts(h_com_port, &commTimeouts))
	{
		CloseHandle(h_com_port);
		return 6;
	}


	//  spawn separate thread to listen on serial port
	h_uart_rx_thread = (HANDLE)_beginthreadex(NULL, 0, uart_rx_thread, NULL, 0, NULL);
	if (0 == h_uart_rx_thread)
	{
		return 9;
	}

	return 0;
}

//*****************************************************************************
unsigned uart_baud_rate = 115200 ;
unsigned debug_level = 0 ;

int init_serial_uart(void)
{
   if (serial_uart_num == 0) {
      printf("serial uart is not initialized\n");
      return 1;
   }
   return (int) uart_init(serial_uart_num, uart_baud_rate) ;
}

