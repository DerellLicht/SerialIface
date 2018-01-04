#include <windows.h>
#include <stdio.h>
#ifdef _lint
#include <stdlib.h>  //  atoi
#include <math.h>
#endif

#include "common.h"
#include "serial_enum.h"
#include "serial_iface.h"

//************************************************************************
static void usage(void)
{
    // fprintf(fd, "commport=%u\n", uart_comm_num) ;
    // fprintf(fd, "baudrate=%u\n", uart_baud_rate) ;
    // fprintf(fd, "debug=%u\n", (debug_level) ? 1U : 0U) ;
    puts("Usage: serial_test -cCommPortNum -bBaudRate");
    puts("If -c and/or -b are omitted, default values will be used");
    puts("") ;
    puts("serial_test -c15 -b115200") ;
}

//************************************************************************
int main(int argc, char **argv)
{
    int result ; 
    int idx ;
    bool params_changed = false ;
    uint uvalue ;
    load_exec_filename();
    read_config_file();

    //  parse command line
    for (idx=1; idx<argc; idx++)
    {
        char *p = argv[idx];
        if (*p == '-')
        {
            p++ ;
            switch (*p)
            {
            case 'c':
                p++ ;
                uvalue = (uint) atoi(p);  //lint !e746 !e1055
                if (uvalue != 0)
                {
                    uart_comm_num = uvalue ;
                    params_changed = true ;
                }
                break;
            case 'b':
                p++ ;
                uvalue = (uint) atoi(p);
                if (uvalue != 0)
                {
                    uart_baud_rate = uvalue ;
                    params_changed = true ;
                }
                break;
            default:
                usage();
                return 1;
            }

        }
        else
        {
            usage() ;
            return 1;
        }
    }

    if (params_changed)
    {
        save_cfg_file();
    }
    //  show settings summary
    printf("serial_test: baudrate=%u, uart=COM%u\n", uart_baud_rate, uart_comm_num);

    // char *outmsg = "\n\r";
    uint timeout_msec = 2000 ;
    uint elapsed_time = 0 ;
    char outmsg[81];
    enumerate_serial_ports();
    result = init_serial_uart();
    if (result == 0) {
       sprintf(outmsg, "derelict\n\r");
       printf("\nsend %s", outmsg);      
       uint outlen = strlen(outmsg);
       send_serial_msg(outmsg, outlen);

       //  wait and see if we get teh same number of characters back
       uint inpidx = 0 ;
       while (LOOP_FOREVER) {
          if (UART_hitc()) {
             uint8 inchr = UART_getc();
             if (inchr == outmsg[inpidx]) {
                inpidx++ ;
                if (inpidx == outlen) {
                   printf("good response received\n");
                   break;
                }
             }
             else
             {
                 printf("bad rx: idx %u: %02X vs %02X\n", inpidx, inchr, outmsg[inpidx]);
                 break;
             }
          }
          Sleep(100);
          elapsed_time += 100 ;
          if (elapsed_time >= timeout_msec)
             break ;
       }
    }

    abort_thread = 1 ;
    while (abort_thread != 0) {
        Sleep(100);
    }
      
    return 0;
}
