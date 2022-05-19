/**
  ******************************************************************************
  * @file    LwIP/LwIP_HTTP_Server_Netconn_RTOS/Src/httpser-netconn.c 
  * @author  MCD Application Team
  * @brief   Basic http server implementation using LwIP netconn API  
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/apps/fs.h"
#include "string.h"
#include "httpserver-netconn.h"
#include "cmsis_os.h"


#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define WEBSERVER_THREAD_PRIO    ( osPriorityAboveNormal )

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

uint16_t i=0;
float t = 0;
float p = 0;
float h = 0;

typedef enum PressureDataRate{
	PRESSURE_DATA_RATE_ONE_SHOT = 0b00000000,
	PRESSURE_DATA_RATE_1Hz = 0b00010000,
	PRESSURE_DATA_RATE_7Hz = 0b00100000,
	PRESSURE_DATA_RATE_12_5Hz = 0b00110000,
	PRESSURE_DATA_RATE_25Hz = 0b01000000
}PressureDataRate;

#define PRESSURE_SENSOR_ADDR 0b10111010
#define HUMIDITY_TEMPERATURE_SENSOR_ADDR 0b10111110

#define POWER_DOWN 0b10000000

I2C_HandleTypeDef* hi2c;

uint8_t c=0;
uint8_t status=0;
int j=0;
char I2C_add[20];
uint8_t I2CBuf[10];

uint8_t ret;
uint8_t data;
uint8_t rawPressureData[3];
int32_t pressureData;
float pressure;
uint8_t rawTemperatureData[2];
int16_t temperatureData;
float temperature;

uint8_t rawTemperatureData2[2];
int16_t temperatureData2;
float temperature2;

uint8_t rawT0[2];
uint8_t rawT1[2];
uint8_t Tdeg[2];
uint8_t t0t1MSB;
float RealTdeg0;
float RealTdeg1;
int16_t T0;
int16_t T1;

uint8_t rawHumidityData[2];
int16_t humidityData;
float humidity;

uint8_t rawH0[2];
uint8_t rawH1[2];
uint8_t Hdeg[2];
int16_t H0;
int16_t H1;
float Halpha;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
float lerp(float a, float b, float alpha){
	return (b - a) * alpha + a;
}
/**
  * @brief serve tcp connection  
  * @param conn: pointer on connection structure 
  * @retval None
  */

void setHI2C(I2C_HandleTypeDef* hi2c1){
	hi2c = hi2c1;
}

static void http_server_serve(struct netconn *conn) 
{
  struct netbuf *inbuf;
  err_t recv_err;
  char* buf;
  u16_t buflen;
  struct fs_file file;
  
  /* Read the data from the port, blocking if nothing yet there. 
   We assume the request (the part we care about) is in one netbuf */
  recv_err = netconn_recv(conn, &inbuf);
  
  if (recv_err == ERR_OK)
  {
    if (netconn_err(conn) == ERR_OK) 
    {
      netbuf_data(inbuf, (void**)&buf, &buflen);
    
      /* Is this an HTTP GET command? (only check the first 5 chars, since
      there are other formats for GET, and we're keeping it very simple )*/
      if ((buflen >=5) && (strncmp(buf, "GET /", 5) == 0))
      {
        /* Check if request to get ST.gif */ 
        if (strncmp((char const *)buf,"GET /STM32F7xx_files/ST.gif",27)==0)
        {
          fs_open(&file, "/STM32F7xx_files/ST.gif"); 
          netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
          fs_close(&file);
        }   
        /* Check if request to get stm32.jpeg */
        else if (strncmp((char const *)buf,"GET /STM32F7xx_files/stm32.jpg",30)==0)
        {
          fs_open(&file, "/STM32F7xx_files/stm32.jpg"); 
          netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
          fs_close(&file);
        }
        else if (strncmp((char const *)buf,"GET /STM32F7xx_files/logo.jpg", 29) == 0)                                           
        {
          /* Check if request to get ST logo.jpg */
          fs_open(&file, "/STM32F7xx_files/logo.jpg"); 
          netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
          fs_close(&file);
        }
        else if(strncmp(buf, "GET /STM32F7xxTASKS.html", 24) == 0)
        {
           /* Load dynamic page */
           DynWebPage(conn);
        }
        else if((strncmp(buf, "GET /index.html", 19) == 0)||(strncmp(buf, "GET / ", 6) == 0))
        {
        	DynWebPage(conn);
        }
        else 
        {
          /* Load Error page */
          fs_open(&file, "/404.html"); 
          netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
          fs_close(&file);
        }
      }      
    }
  }
  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);
  
  /* Delete the buffer (netconn_recv gives us ownership,
   so we have to make sure to deallocate the buffer) */
  netbuf_delete(inbuf);
}


/**
  * @brief  http server thread 
  * @param arg: pointer on argument(not used here) 
  * @retval None
  */
static void http_server_netconn_thread(void *arg)
{ 
  struct netconn *conn, *newconn;
  err_t err, accept_err;
  
  /* Create a new TCP connection handle */
  conn = netconn_new(NETCONN_TCP);
  
  if (conn!= NULL)
  {
    /* Bind to port 80 (HTTP) with default IP address */
    err = netconn_bind(conn, NULL, 80);
    
    if (err == ERR_OK)
    {
      /* Put the connection into LISTEN state */
      netconn_listen(conn);
  
      while(1) 
      {
        /* accept any icoming connection */
        accept_err = netconn_accept(conn, &newconn);
        if(accept_err == ERR_OK)
        {
          /* serve connection */
          http_server_serve(newconn);

          /* delete connection */
          netconn_delete(newconn);
        }
      }
    }
  }
}

/**
  * @brief  Initialize the HTTP server (start its thread) 
  * @param  none
  * @retval None
  */
void http_server_netconn_init()
{
	data = (uint8_t)(PRESSURE_DATA_RATE_7Hz | POWER_DOWN);
	  	  HAL_I2C_Mem_Write(hi2c, PRESSURE_SENSOR_ADDR, 0x20, 1, &data, 1, 50);
	  	  data = 0;
	  	  HAL_I2C_Mem_Write(hi2c, PRESSURE_SENSOR_ADDR, 0x10, 1, &data, 1, 50);

	  	  data = POWER_DOWN | 0b00000010;
	  	  HAL_I2C_Mem_Write(hi2c, HUMIDITY_TEMPERATURE_SENSOR_ADDR, 0x20, 1, &data, 1, 50);

	  	  HAL_I2C_Mem_Read(hi2c, HUMIDITY_TEMPERATURE_SENSOR_ADDR, 0x3C, 1, &rawT0[0], 1, 50);
	  	  HAL_I2C_Mem_Read(hi2c, HUMIDITY_TEMPERATURE_SENSOR_ADDR, 0x3D, 1, &rawT0[1], 1, 50);
	  	  HAL_I2C_Mem_Read(hi2c, HUMIDITY_TEMPERATURE_SENSOR_ADDR, 0x3E, 1, &rawT1[0], 1, 50);
	  	  HAL_I2C_Mem_Read(hi2c, HUMIDITY_TEMPERATURE_SENSOR_ADDR, 0x3F, 1, &rawT1[1], 1, 50);
	  	  HAL_I2C_Mem_Read(hi2c, HUMIDITY_TEMPERATURE_SENSOR_ADDR, 0x32, 1, &Tdeg[0], 1, 50);
	  	  HAL_I2C_Mem_Read(hi2c, HUMIDITY_TEMPERATURE_SENSOR_ADDR, 0x33, 1, &Tdeg[1], 1, 50);
	  	  HAL_I2C_Mem_Read(hi2c, HUMIDITY_TEMPERATURE_SENSOR_ADDR, 0x35, 1, &t0t1MSB, 1, 50);

	  	  T0 = rawT0[0] + (rawT0[1] << 8);
	  	  T1 = rawT1[0] + (rawT1[1] << 8);

	  	  HAL_I2C_Mem_Read(hi2c, HUMIDITY_TEMPERATURE_SENSOR_ADDR, 0x36, 1, &rawH0[0], 1, 50);
	  	  HAL_I2C_Mem_Read(hi2c, HUMIDITY_TEMPERATURE_SENSOR_ADDR, 0x37, 1, &rawH0[1], 1, 50);
	  	  HAL_I2C_Mem_Read(hi2c, HUMIDITY_TEMPERATURE_SENSOR_ADDR, 0x3A, 1, &rawH1[0], 1, 50);
	  	  HAL_I2C_Mem_Read(hi2c, HUMIDITY_TEMPERATURE_SENSOR_ADDR, 0x3B, 1, &rawH1[1], 1, 50);
	  	  HAL_I2C_Mem_Read(hi2c, HUMIDITY_TEMPERATURE_SENSOR_ADDR, 0x30, 1, &Hdeg[0], 1, 50);
	  	  HAL_I2C_Mem_Read(hi2c, HUMIDITY_TEMPERATURE_SENSOR_ADDR, 0x31, 1, &Hdeg[1], 1, 50);

	  	  H0 = rawH0[0] + (rawH0[1] << 8);
	  	  H1 = rawH1[0] + (rawH1[1] << 8);
	sys_thread_new("HTTP", http_server_netconn_thread, NULL, DEFAULT_THREAD_STACKSIZE, WEBSERVER_THREAD_PRIO);
}

/**
  * @brief  Create and send a dynamic Web Page. This page contains the list of 
  *         running tasks and the number of page hits. 
  * @param  conn pointer on connection structure 
  * @retval None
  */

int findCharacter(char* str, char c){
	int i = 0;
	while(str[i] != '\0'){
		if (str[i] == c) return i;
		i++;
	}
	return -1;
}

const portCHAR header[] = "<!DOCTYPE HTML><head><meta http-equiv=\"refresh\" content=\"2\"><title>Station meteo</title></head><body><div class=\"Section1\" style=\"width: 100%; height: 100%\"><div style=\"margin-left: 30pt;\">"
		  "<p class=\"MsoNormal\" style=\"text-align: center;\" align=\"center\"><b><i><span style=\"font-size: 36pt; font-family: "
		  "Calibri; font-style: normal\"><strong>Station meteo domestique</strong></span></i></b><span style=\"font-size: 13.5pt;\">"
		  "</span></p></div><div class=\"MsoNormal\" style=\"text-align: center;\" align=\"center\"><hr align=\"center\" size=\"3\" "
		  "width=\"100%\"></div><div style=\"position:absolute; width: 100%;\">";

void DynWebPage(struct netconn *conn)
{
  //struct fs_file file;
  portCHAR PAGE_BODY[1024];
  portCHAR valeur[10] = {0};

  memset(PAGE_BODY, 0,1024);
  //fs_open(&file, "/index.html");

  //memcpy(PAGE_BODY, file.data, findCharacter(file.data, '^'));

  //fs_close(&file);

  /* Update the hit count */
  //nPageHits++;
  //sprintf(pagehits, "%d", (int)nPageHits);
  //strcat(PAGE_BODY, pagehits);
  //strcat((char *)PAGE_BODY, "<pre><br>Name          State  Priority  Stack   Num" );
  //strcat((char *)PAGE_BODY, "<br>---------------------------------------------<br>");
    
  /* The list of tasks and their status */
  //osThreadList((unsigned char *)(PAGE_BODY + strlen(PAGE_BODY)));
  //strcat((char *)PAGE_BODY, "<br><br>---------------------------------------------");
  //strcat((char *)PAGE_BODY, "<br>B : Blocked, R : Ready, D : Deleted, S : Suspended<br>");

  /* Send the dynamically generated page */
  //netconn_write(conn, PAGE_START, strlen((char*)PAGE_START), NETCONN_COPY);
  //<meta http-equiv=\"refresh\" content=\"2\">

  //HAL_Delay(1000);

  	  	  HAL_I2C_Mem_Read(hi2c, PRESSURE_SENSOR_ADDR, 0x28, 1, &rawPressureData[0], 1, 50);
  	  	  HAL_I2C_Mem_Read(hi2c, PRESSURE_SENSOR_ADDR, 0x29, 1, &rawPressureData[1], 1, 50);
  	  	  HAL_I2C_Mem_Read(hi2c, PRESSURE_SENSOR_ADDR, 0x2A, 1, &rawPressureData[2], 1, 50);

  	  	  HAL_I2C_Mem_Read(hi2c, PRESSURE_SENSOR_ADDR, 0x2B, 1, &rawTemperatureData[0], 1, 50);
  	  	  HAL_I2C_Mem_Read(hi2c, PRESSURE_SENSOR_ADDR, 0x2C, 1, &rawTemperatureData[1], 1, 50);

  	  	  pressureData = rawPressureData[0];
  	  	  pressureData |= (rawPressureData[1] << 8);
  	  	  if(rawPressureData[2] & 0x80){
  	  		  pressureData |= (rawPressureData[2] << 16);
  	  		  pressureData |= (0xFF << 24);
  	  	  }else{
  	  		  pressureData |= (rawPressureData[2] << 16);
  	  	  }
  	  	  pressure = pressureData/4096.0;

  	  	  temperatureData = rawTemperatureData[0] + (rawTemperatureData[1] << 8);
  	  	  temperature = 42.5 + temperatureData/480.0;


  	  	  HAL_I2C_Mem_Read(hi2c, HUMIDITY_TEMPERATURE_SENSOR_ADDR, 0x2A, 1, &rawTemperatureData2[0], 1, 50);
  	  	  HAL_I2C_Mem_Read(hi2c, HUMIDITY_TEMPERATURE_SENSOR_ADDR, 0x2B, 1, &rawTemperatureData2[1], 1, 50);

  	  	  temperatureData2 = rawTemperatureData2[0] + (rawTemperatureData2[1] << 8);
  	  	  RealTdeg0 = (((uint16_t)Tdeg[0] + (((uint16_t)t0t1MSB & 0b11) << 8)))/8.0;
  	  	  RealTdeg1 = (((uint16_t)Tdeg[1] + (((uint16_t)t0t1MSB & 0b1100) << 6)))/8.0;
  	  	  temperature2 = lerp(RealTdeg0, RealTdeg1, (float)(temperatureData2 - T0)/(T1-T0));

  	  	  HAL_I2C_Mem_Read(hi2c, HUMIDITY_TEMPERATURE_SENSOR_ADDR, 0x28, 1, &rawHumidityData[0], 1, 50);
  	  	  HAL_I2C_Mem_Read(hi2c, HUMIDITY_TEMPERATURE_SENSOR_ADDR, 0x29, 1, &rawHumidityData[1], 1, 50);

  	  	  humidityData = rawHumidityData[0] + (rawHumidityData[1] << 8);
  	  	  Halpha = (float)(humidityData - H0)/(H1-H0);
  	  	  humidity = lerp(Hdeg[0]/2.0, Hdeg[1]/2.0, Halpha);


  float t1 = 0.5;
  float t2 = 0.7;
  float t3 = 5.1;

  sprintf(valeur, "%.1f %s", temperature2, "deg C");
  strcat((char *)PAGE_BODY, "<div style=\"position: relative; top:0px; float: left; width: 33%; text-align: center; font-size:30px;\"> Temperature : ");
  strcat((char *)PAGE_BODY, valeur);
  strcat((char *)PAGE_BODY, "</div>");
  sprintf(valeur, "%.1f %s", pressure, "Pa");
  strcat((char *)PAGE_BODY, "<div style=\"position: relative; top:0px; float: left; width: 33%; text-align: center; font-size:30px;\"> Pression : ");
  strcat((char *)PAGE_BODY, valeur);
  strcat((char *)PAGE_BODY, "</div>");
  sprintf(valeur, "%.1f %s", humidity, "%");
  strcat((char *)PAGE_BODY, "<div style=\"position: relative; top:0px; float: left; width: 33%; text-align: center; font-size:30px;\"> Humidite : ");
  strcat((char *)PAGE_BODY, valeur);
  strcat((char *)PAGE_BODY, "</div>");
  strcat((char *)PAGE_BODY, "</div>");

  strcat((char *)PAGE_BODY, "<div style=\"padding-top: 150px; text-align: center; font-size: 40px\">Prevision :");
  strcat((char *)PAGE_BODY, "<div style=\"padding-top: 150px; text-align: center; font-size: 40px; width 50%; float: left;position: relative;\">Il fait ");
  strcat((char *)PAGE_BODY, "</div>");
  strcat((char *)PAGE_BODY, "<div style=\"padding-top: 150px; text-align: center; font-size: 40px; width 50%; float: left;position: relative;\">Il va faire ");
    strcat((char *)PAGE_BODY, "</div>");
  strcat((char *)PAGE_BODY, "</div>");

  strcat((char *)PAGE_BODY, "</body></html>");
  netconn_write(conn, header, strlen(header), NETCONN_COPY);
  netconn_write(conn, PAGE_BODY, strlen(PAGE_BODY), NETCONN_COPY);

}
