//  serial_iface.cpp
extern unsigned uart_baud_rate ;
extern unsigned uart_comm_num ;
extern unsigned debug_level ;

extern unsigned abort_thread ;

//  serial_iface.cpp
int init_serial_uart(void);
int send_serial_msg(char *txbfr, uint txbfr_len);
bool UART_hitc(void);
uint8 UART_getc(void);

//  config.cpp
LRESULT save_cfg_file(void);
LRESULT read_config_file(void);

