#include "mqtt.h"
#include "bsp_esp8266.h"

#if 1
typedef enum
{
	//名字 	    值 			报文流动方向 	描述
	M_RESERVED1	=0	,	//	禁止	保留
	M_CONNECT		,	//	客户端到服务端	客户端请求连接服务端
	M_CONNACK		,	//	服务端到客户端	连接报文确认
	M_PUBLISH		,	//	两个方向都允许	发布消息
	M_PUBACK		,	//	两个方向都允许	QoS 1消息发布收到确认
	M_PUBREC		,	//	两个方向都允许	发布收到（保证交付第一步）
	M_PUBREL		,	//	两个方向都允许	发布释放（保证交付第二步）
	M_PUBCOMP		,	//	两个方向都允许	QoS 2消息发布完成（保证交互第三步）
	M_SUBSCRIBE		,	//	客户端到服务端	客户端订阅请求
	M_SUBACK		,	//	服务端到客户端	订阅请求报文确认
	M_UNSUBSCRIBE	,	//	客户端到服务端	客户端取消订阅请求
	M_UNSUBACK		,	//	服务端到客户端	取消订阅报文确认
	M_PINGREQ		,	//	客户端到服务端	心跳请求
	M_PINGRESP		,	//	服务端到客户端	心跳响应
	M_DISCONNECT	,	//	客户端到服务端	客户端断开连接
	M_RESERVED2		,	//	禁止	保留
}_typdef_mqtt_message;



//连接成功服务器回应 20 02 00 00
//客户端主动断开连接 e0 00
const u8 parket_connetAck[] = {0x20,0x02,0x00,0x00};
const u8 parket_disconnet[] = {0xe0,0x00};
const u8 parket_heart[] = {0xc0,0x00};
const u8 parket_heart_reply[] = {0xc0,0x00};
const u8 parket_subAck[] = {0x90,0x03};

static void Mqtt_SendBuf(u8 *buf,u16 len);

static void Init(u8 *prx,u16 rxlen,u8 *ptx,u16 txlen);
static u8 Connect(char *ClientID,char *Username,char *Password);
static u8 SubscribeTopic(char *topic,u8 qos,u8 whether);
static u8 PublishData(char *topic, char *message, u8 qos);
static void SentHeart(void);
static void Disconnect(void);

_typdef_mqtt _mqtt = 
{
	0,0,
	0,0,
	Init,
	Connect,
	SubscribeTopic,
	PublishData,
	SentHeart,
	Disconnect,
};

static void Init(u8 *prx,u16 rxlen,u8 *ptx,u16 txlen)
{
	_mqtt.rxbuf = prx;_mqtt.rxlen = rxlen;
	_mqtt.txbuf = ptx;_mqtt.txlen = txlen;
	
	memset(_mqtt.rxbuf,0,_mqtt.rxlen);
	memset(_mqtt.txbuf,0,_mqtt.txlen);
	
	//无条件先主动断开
	_mqtt.Disconnect();Delay_ms(100);
	_mqtt.Disconnect();Delay_ms(100);
}


//连接服务器的打包函数
static u8 Connect(char *ClientID,char *Username,char *Password)
{	
	u8 cnt=3;
	u8 wait;
	
	int ClientIDLen = strlen(ClientID);
	int UsernameLen = strlen(Username);
	int PasswordLen = strlen(Password);
	int DataLen;
	_mqtt.txlen=0;
	//可变报头+Payload  每个字段包含两个字节的长度标识
	DataLen = 10 + (ClientIDLen+2) + (UsernameLen+2) + (PasswordLen+2);
	
	//固定报头
	//控制报文类型
	_mqtt.txbuf[_mqtt.txlen++] = 0x10;		
	
	//剩余长度(不包括固定头部)
	do
	{
		u8 encodedByte = DataLen % 128;
		DataLen = DataLen / 128;
		// if there are more data to encode, set the top bit of this byte
		if ( DataLen > 0 )
			encodedByte = encodedByte | 128;
		_mqtt.txbuf[_mqtt.txlen++] = encodedByte;
	}while ( DataLen > 0 );
    	
	//可变报头
	//协议名
	_mqtt.txbuf[_mqtt.txlen++] = 0;        		// Protocol Name Length MSB    
	_mqtt.txbuf[_mqtt.txlen++] = 4;        		// Protocol Name Length LSB    
	_mqtt.txbuf[_mqtt.txlen++] = 'M';        	// ASCII Code for M    
	_mqtt.txbuf[_mqtt.txlen++] = 'Q';        	// ASCII Code for Q    
	_mqtt.txbuf[_mqtt.txlen++] = 'T';        	// ASCII Code for T    
	_mqtt.txbuf[_mqtt.txlen++] = 'T';        	// ASCII Code for T    
	//协议级别
	_mqtt.txbuf[_mqtt.txlen++] = 4;        		// MQTT Protocol version = 4    
	//连接标志
	_mqtt.txbuf[_mqtt.txlen++] = 0xc2;        	// conn flags 
	_mqtt.txbuf[_mqtt.txlen++] = 0;        		// Keep-alive Time Length MSB    
	_mqtt.txbuf[_mqtt.txlen++] = 60;        	// Keep-alive Time Length LSB  60S心跳包  

	_mqtt.txbuf[_mqtt.txlen++] = BYTE1(ClientIDLen);// Client ID length MSB    
	_mqtt.txbuf[_mqtt.txlen++] = BYTE0(ClientIDLen);// Client ID length LSB  	
	memcpy(&_mqtt.txbuf[_mqtt.txlen],ClientID,ClientIDLen);
	_mqtt.txlen += ClientIDLen;
    
	if(UsernameLen > 0)
	{   
		_mqtt.txbuf[_mqtt.txlen++] = BYTE1(UsernameLen);		//username length MSB    
		_mqtt.txbuf[_mqtt.txlen++] = BYTE0(UsernameLen);    	//username length LSB    
		memcpy(&_mqtt.txbuf[_mqtt.txlen],Username,UsernameLen);
		_mqtt.txlen += UsernameLen;
	}
	
	if(PasswordLen > 0)
	{    
		_mqtt.txbuf[_mqtt.txlen++] = BYTE1(PasswordLen);		//password length MSB    
		_mqtt.txbuf[_mqtt.txlen++] = BYTE0(PasswordLen);    	//password length LSB  
		memcpy(&_mqtt.txbuf[_mqtt.txlen],Password,PasswordLen);
		_mqtt.txlen += PasswordLen; 
	}    
	

	while(cnt--)
	{
		memset(_mqtt.rxbuf,0,_mqtt.rxlen);
		Mqtt_SendBuf(_mqtt.txbuf,_mqtt.txlen);
//		for(i=0;i<_mqtt.txlen;i++)
//		{
//			printf("%c",_mqtt.txbuf[i]);
//		}

		wait=30;//等待3s时间
		while(wait--)
		{
//			printf("%d %d,",_mqtt.rxbuf[0],_mqtt.rxbuf[1]);
			//CONNECT
			if(_mqtt.rxbuf[0]==parket_connetAck[0] && _mqtt.rxbuf[1]==parket_connetAck[1]) //连接成功			   
			{
				return 1;//连接成功
			}
//			if(strEsp8266_Fram_Record .Data_RX_BUF [0]  == parket_connetAck[0] 
//				&& strEsp8266_Fram_Record .Data_RX_BUF [1] == parket_connetAck[1])
//			{
//				return 1;
//			}
			Delay_ms(100);			
		}
	}
	return 0;
}


//MQTT订阅/取消订阅数据打包函数
//topic       主题 
//qos         消息等级 
//whether     订阅/取消订阅请求包
static u8 SubscribeTopic(char *topic,u8 qos,u8 whether)
{    	
	u8 cnt=2;
	u8 wait;
	int topiclen = strlen(topic);
	
	int DataLen = 2 + (topiclen+2) + (whether?1:0);//可变报头的长度（2字节）加上有效载荷的长度
	_mqtt.txlen=0;

	//固定报头
	//控制报文类型
	if(whether) _mqtt.txbuf[_mqtt.txlen++] = 0x82; //消息类型和标志订阅
	else	_mqtt.txbuf[_mqtt.txlen++] = 0xA2;    //取消订阅

	//剩余长度
	do
	{
		u8 encodedByte = DataLen % 128;
		DataLen = DataLen / 128;
		// if there are more data to encode, set the top bit of this byte
		if ( DataLen > 0 )
			encodedByte = encodedByte | 128;
		_mqtt.txbuf[_mqtt.txlen++] = encodedByte;
	}while ( DataLen > 0 );	
	
	//可变报头
	_mqtt.txbuf[_mqtt.txlen++] = 0;				//消息标识符 MSB
	_mqtt.txbuf[_mqtt.txlen++] = 0x01;           //消息标识符 LSB
	//有效载荷
	_mqtt.txbuf[_mqtt.txlen++] = BYTE1(topiclen);//主题长度 MSB
	_mqtt.txbuf[_mqtt.txlen++] = BYTE0(topiclen);//主题长度 LSB   
	memcpy(&_mqtt.txbuf[_mqtt.txlen],topic,topiclen);
	_mqtt.txlen += topiclen;
	
	if(whether)
	{
			_mqtt.txbuf[_mqtt.txlen++] = qos;//QoS级别
	}

	while(cnt--)
	{
		memset(_mqtt.rxbuf,0,_mqtt.rxlen);
		Mqtt_SendBuf(_mqtt.txbuf,_mqtt.txlen);
		wait=30;//等待3s时间
		while(wait--)
		{
			if(_mqtt.rxbuf[0]==parket_subAck[0] && _mqtt.rxbuf[1]==parket_subAck[1]) //订阅成功			   
			{
				return 1;//订阅成功
			}
			Delay_ms(100);			
		}
	}
	if(cnt) return 1;	//订阅成功
	return 0;
}

//MQTT发布数据打包函数
//topic   主题 
//message 消息
//qos     消息等级 
static u8 PublishData(char *topic, char *message, u8 qos)
{  
	int topicLength = strlen(topic);    
	int messageLength = strlen(message);     
	static u16 id=0;
	int DataLen;
	_mqtt.txlen=0;
	//有效载荷的长度这样计算：用固定报头中的剩余长度字段的值减去可变报头的长度
	//QOS为0时没有标识符
	//数据长度             主题名   报文标识符   有效载荷
	if(qos)	DataLen = (2+topicLength) + 2 + messageLength;       
	else	DataLen = (2+topicLength) + messageLength;   

	//固定报头
	//控制报文类型
	_mqtt.txbuf[_mqtt.txlen++] = 0x30;    // MQTT Message Type PUBLISH  

	//剩余长度
	do
	{
		u8 encodedByte = DataLen % 128;
		DataLen = DataLen / 128;
		// if there are more data to encode, set the top bit of this byte
		if ( DataLen > 0 )
			encodedByte = encodedByte | 128;
		_mqtt.txbuf[_mqtt.txlen++] = encodedByte;
	}while ( DataLen > 0 );	
	
	_mqtt.txbuf[_mqtt.txlen++] = BYTE1(topicLength);//主题长度MSB
	_mqtt.txbuf[_mqtt.txlen++] = BYTE0(topicLength);//主题长度LSB 
	memcpy(&_mqtt.txbuf[_mqtt.txlen],topic,topicLength);//拷贝主题
	_mqtt.txlen += topicLength;
        
	//报文标识符
	if(qos)
	{
		_mqtt.txbuf[_mqtt.txlen++] = BYTE1(id);
		_mqtt.txbuf[_mqtt.txlen++] = BYTE0(id);
		id++;
	}
	memcpy(&_mqtt.txbuf[_mqtt.txlen],message,messageLength);
	_mqtt.txlen += messageLength;
        
	Mqtt_SendBuf(_mqtt.txbuf,_mqtt.txlen);
	return _mqtt.txlen;
}

static void SentHeart(void)
{
	Mqtt_SendBuf((u8 *)parket_heart,sizeof(parket_heart));
}

static void Disconnect(void)
{
	Mqtt_SendBuf((u8 *)parket_disconnet,sizeof(parket_disconnet));
}

static void Mqtt_SendBuf(u8 *buf,u16 len)
{
	macESP8266_USARTx_SendBuf(buf,len);
}
#else
//固定报头格式：10
unsigned char GetDataFixedHead(unsigned char MesType,unsigned char DupFlag,unsigned char QosLevel,unsigned char Retain)
{
	unsigned char dat = 0;
	dat = (MesType & 0x0f) << 4;
	dat |= (DupFlag & 0x01) << 3;
	dat |= (QosLevel & 0x03) << 1;
	dat |= (Retain & 0x01);
	return dat;//1
}
//unsigned char 8位
void GetDataConnet(unsigned char *buff)//获取连接的数据包正确连接返回20 02 00 00
{
	unsigned int i,len,lennum = 0;
	unsigned char *msg;
	buff[0] = GetDataFixedHead(MQTT_TypeCONNECT,0,0,0);//MQTT_TypeCONNECT：1
	buff[2] = 0x00;
	buff[3] = 0x04;
	buff[4] = 'M';
	buff[5] = 'Q';
	buff[6] = 'T';
	buff[7] = 'T';
	buff[8] = 0x04;//协议级别 Protocol Level
	buff[9] = 0 | (MQTT_StaCleanSession << 1) | (MQTT_StaWillFlag << 1)
							| (MQTT_StaWillQoS << 3) | (MQTT_StaWillRetain << 5) 
							|	(MQTT_StaPasswordFlag << 6) |(MQTT_StaUserNameFlag << 7);//连接标志
	buff[10] = MQTT_KeepAlive >> 8;
	buff[11] = MQTT_KeepAlive;
	len = strlen(MQTT_ClientIdentifier);
	buff[12] = len >> 8;
	buff[13] = len;
	msg = MQTT_ClientIdentifier;
	for(i = 0;i<len;i++)
	{
		buff[14+i] =  msg[i];
	}
	lennum += len;
	if(MQTT_StaWillFlag)
	{
		len = strlen(MQTT_WillTopic);
		buff[13 + lennum + 1] = len >> 8;
		buff[13 + lennum + 2] = len;
		lennum += 2;
		msg = MQTT_WillTopic;
		for(i = 0;i<len;i++)
		{
			buff[14+lennum+i] =  msg[i];
		}
		lennum += len;
		len = strlen(MQTT_WillMessage);
		buff[12] = len >> 8;
		buff[13] = len;
		lennum += 2;
		msg = MQTT_WillMessage;
		for(i = 0;i<len;i++)
		{
			buff[14+lennum+i] =  msg[i];
		}
		lennum += len;
	}
	if(MQTT_StaUserNameFlag)
	{
		len = strlen(MQTT_UserName);
		buff[13 + lennum + 1] = len >> 8;
		buff[13 + lennum + 2] = len;
		lennum += 2;
		msg = MQTT_UserName;
		for(i = 0;i<len;i++)
		{
			buff[14+lennum+i] =  msg[i];
		}
		lennum += len;
		
	}
	if(MQTT_StaPasswordFlag)
	{
		len = strlen(MQTT_Password);
		buff[13 + lennum + 1] = len >> 8;
		buff[13 + lennum + 2] = len;
		lennum += 2;
		msg = MQTT_Password;
		for(i = 0;i<len;i++)
		{
			buff[14+lennum+i] =  msg[i];
		}
		lennum += len;
	}
	buff[1] = 13 + lennum - 1;
}
void GetDataDisConnet(unsigned char *buff)//获取断开连接的数据包
{
	buff[0] = 0xe0;
	buff[1] = 0;
}
void GetDataPINGREQ(unsigned char *buff)//心跳请求的数据包成功返回d0 00
{
	buff[0] = 0xc0;
	buff[1] = 0;
}
/*
	成功返回90 0x 00 Num RequestedQoS
*/
void GetDataSUBSCRIBE(unsigned char *buff,const char *dat,unsigned int Num,unsigned char RequestedQoS)//订阅主题的数据包 Num:主题序号 RequestedQoS:服务质量要求0,1或2
{
	unsigned int i,len = 0,lennum = 0; 
	buff[0] = 0x82;
	len = strlen(dat);
	buff[2] = Num>>8;
	buff[3] = Num;
	buff[4] = len>>8;
	buff[5] = len;
	for(i = 0;i<len;i++)
	{
		buff[6+i] = dat[i];
	}
	lennum = len;
	buff[6 + lennum ] = RequestedQoS;
	buff[1] = lennum + 5;
}
void GetDataPUBLISH(unsigned char *buff,unsigned char dup, unsigned char qos,unsigned char retain,const char *topic ,const char *msg)//获取发布消息的数据包
{
	unsigned int i,len=0,lennum=0;
	buff[0] = GetDataFixedHead(MQTT_TypePUBLISH,dup,qos,retain);
	len = strlen(topic);
	buff[2] = len>>8;
	buff[3] = len;
	for(i = 0;i<len;i++)
	{
		buff[4+i] = topic[i];
	}
	lennum = len;
	len = strlen(msg);
	for(i = 0;i<len;i++)
	{
		buff[4+i+lennum] = msg[i];
	}
	lennum += len;
	buff[1] = lennum + 2;
}

#endif

/*********************************************END OF FILE********************************************/
