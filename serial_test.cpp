#include <windows.h>
#include <stdio.h>

#include "common.h"
#include "serial_enum.h"

extern uint bytes_received ;

//************************************************************************
int main(void)
{
   int result ; 
   // char *outmsg = "\n\r";
   char outmsg[81];
	enumerate_serial_ports();
   select_serial_ports() ;
   result = init_serial_uart();
   if (result != 0) {
      return result ;
   }
   uint tx_state = 0 ;
   while (tx_state < 3) {
      bytes_received = 0 ;
      switch(tx_state) {
      case 0:
         sprintf(outmsg, "\n\r");
      	break;
      
      case 1:
         sprintf(outmsg, "output1\n\r");
      	break;
      
      case 2:
         sprintf(outmsg, "output2\n\r");
      	break;
      }
      printf("\nstate=%u, send %s", tx_state, outmsg);      
      send_serial_msg_ascii(outmsg);
      uint curr_rx = bytes_received ; 
      while (LOOP_FOREVER) {
         Sleep(1500);
         if (curr_rx == bytes_received)
            break ;
         curr_rx = bytes_received ;   
      }
      tx_state++ ;
   }
     
   return 0;
}
