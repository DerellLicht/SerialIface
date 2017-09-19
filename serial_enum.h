/******************************************************************************
 *  Copyright (c) 2003-2016  Daniel D Miller
 ******************************************************************************/

//  serial_enum.c
int enumerate_serial_ports(void);
TCHAR *get_dev_path(unsigned port_num);
unsigned get_serial_port_number(TCHAR *refname);

//  serial_iface.c
int init_serial_uart(void);
int send_serial_msg_ascii(char *txbfr);

