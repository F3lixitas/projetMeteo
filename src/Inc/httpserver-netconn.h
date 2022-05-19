#ifndef __HTTPSERVER_NETCONN_H__
#define __HTTPSERVER_NETCONN_H__

#include "lwip/api.h"
#include "stm32f7xx_hal.h"

void http_server_netconn_init(void);
void DynWebPage(struct netconn *conn);

void setHI2C(I2C_HandleTypeDef* hi2c1);

#endif /* __HTTPSERVER_NETCONN_H__ */
