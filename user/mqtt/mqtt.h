#ifndef __MQTT_H_
#define __MQTT_H_

#if 1
#include "stm32f10x.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"
#include "bsp_Systick.h"
#include "bsp_usart1.h"
#include "bsp_esp8266.h"

#define BYTE0(dwTemp)       (*( char *)(&dwTemp))
#define BYTE1(dwTemp)       (*((char *)(&dwTemp) + 1))
#define BYTE2(dwTemp)       (*((char *)(&dwTemp) + 2))
#define BYTE3(dwTemp)       (*((char *)(&dwTemp) + 3))
	
			/*mqttClientId: clientId+"|securemode=3,signmethod=hmacsha1,timestamp=132323232|"
			mqttUsername: deviceName+"&"+productKey
			mqttPassword: sign_hmac(deviceSecret,content)*/
#define mqttClientId        "111|securemode=3,signmethod=hmacsha1,timestamp=999|"
#define mqttUsername				"smartmask&a104fcohulX"
#define	mqttPassword				"135FE86509310ECC1863420917F4DBB9815575D5"
#define mqttTopicProPost_reply			"*****************"   //星号填需要订阅的topic
#define mqttTopicProPost						"*****************"
#define mqttTopicStopFan    				"*****************"
#define mqttTopicStopMotor    			"*****************"
#define mqttPayload                 "*****************"

typedef struct
{
	u8 *rxbuf;u16 rxlen;
	u8 *txbuf;u16 txlen;
	void (*Init)(u8 *prx,u16 rxlen,u8 *ptx,u16 txlen);
	u8 (*Connect)(char *ClientID,char *Username,char *Password);
	u8 (*SubscribeTopic)(char *topic,u8 qos,u8 whether);
	u8 (*PublishData)(char *topic, char *message, u8 qos);
	void (*SendHeart)(void);
	void (*Disconnect)(void);
}_typdef_mqtt;

extern _typdef_mqtt _mqtt;
#else

#include "stm32f10x.h"
#include <string.h>
#define		MQTT_TypeCONNECT							1//请求连接  
#define		MQTT_TypeCONNACK							2//请求应答  
#define		MQTT_TypePUBLISH							3//发布消息  
#define		MQTT_TypePUBACK								4//发布应答  
#define		MQTT_TypePUBREC								5//发布已接收，保证传递1  
#define		MQTT_TypePUBREL								6//发布释放，保证传递2  
#define		MQTT_TypePUBCOMP							7//发布完成，保证传递3  
#define		MQTT_TypeSUBSCRIBE						8//订阅请求  
#define		MQTT_TypeSUBACK								9//订阅应答  
#define		MQTT_TypeUNSUBSCRIBE					10//取消订阅  
#define		MQTT_TypeUNSUBACK							11//取消订阅应答  
#define		MQTT_TypePINGREQ							12//ping请求  
#define		MQTT_TypePINGRESP							13//ping响应  
#define		MQTT_TypeDISCONNECT 					14//断开连接  
 
#define		MQTT_StaCleanSession					1	//清理会话 
#define		MQTT_StaWillFlag							0	//遗嘱标志
#define		MQTT_StaWillQoS								0	//遗嘱QoS连接标志的第4和第3位。
#define		MQTT_StaWillRetain						0	//遗嘱保留
#define		MQTT_StaUserNameFlag					1	//用户名标志 User Name Flag
#define		MQTT_StaPasswordFlag					1	//密码标志 Password Flag
#define		MQTT_KeepAlive								60
#define		MQTT_ClientIdentifier  "Tan1"	//客户端标识符 Client Identifier
#define		MQTT_WillTopic			""				//遗嘱主题 Will Topic
#define		MQTT_WillMessage		""				//遗嘱消息 Will Message
#define		MQTT_UserName			"admin"			//用户名 User Name
#define		MQTT_Password			"password"	//密码 Password
 
unsigned char GetDataFixedHead(unsigned char MesType,unsigned char DupFlag,unsigned char QosLevel,unsigned char Retain);
void GetDataPUBLISH(unsigned char *buff,unsigned char dup, unsigned char qos,unsigned char retain,const char *topic ,const char *msg);//获取发布消息的数据包		 	
void GetDataSUBSCRIBE(unsigned char *buff,const char *dat,unsigned int Num,unsigned char RequestedQoS);//订阅主题的数据包 Num:主题序号 RequestedQoS:服务质量要求0,1或2
void GetDataDisConnet(unsigned char *buff);//获取断开连接的数据包
void GetDataConnet(unsigned char *buff);//获取连接的数据包正确连接返回20 02 00 00
void GetDataPINGREQ(unsigned char *buff);//心跳请求的数据包成功返回d0 00


#endif


#endif

