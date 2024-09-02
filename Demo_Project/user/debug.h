#ifndef __DEBUG__H__
#define __DEBUG__H__

void debug_init(void);
void log_info(const char * format, ...);
void log_array(const char * buf, uint32_t len);
void debug_tx_cb(void);

#endif  //!__DEBUG__H__









