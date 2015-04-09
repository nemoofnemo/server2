#ifndef CMDFLAG
#define CMDFLAG

/********************************************************************************
**
**Function:
**Args    :
**Return  :
**Detail  :数据帧格式
**         
**         所有字符, 字符串用UTF-8编码. 各个域没有结束符.
**         
**         字节序号    0         1              2-9          10-21       22-31       32-?
**         说    明   指令    数据格式    整个数据帧长度    MAC地址     保留        数据
**                   |---------------------------------------------------------||-----------|
**                                             数据帧头部                           数据
**         0: 指令:
**             'P':
**                 对于控制端: 发送请求, 请求受控端拍照
**                 对于受控端: 收到请求, 进行拍照
**             'Q':
**                 对于控制端: 收到请求, 显示图片在屏幕上
**                 对于受控端: 发送请求, 返回照片数据 
**             'L':
**                 对于控制端: 发送请求, 请求获取受控端的位置信息
**             'W':
**                 对于受控端: 发送请求, 上传Wifi信号信息给服务器.
**                 对于服务器: 收到请求, 收到Wifi信号信息
**             'G':
**                 对于受控端: 发送请求, 上传GPS位置信息
**                 对于服务器: 收到请求, 收到GPS位置信息
**             'B':
**                 对于控制端: 收到请求, 收到受控端的位置信息.
**             'R':
**                 对于控制端: 上传控制信息等, 服务器以此注册控制端受控端身份.
**             'D':
**             	对于所有角色: 无操作, 如果环境允许, dump出全帧内容. 可以直接抛弃
**         1: 数据格式:
**             未定义, 以UTF-8码的'0'填充
**         2-9:整个数据帧长度:
**             以数字串表示数据长度, 高位填'0'补齐. 只能为正数.
**             例如, 长度为2, 则此域(2-9)的字节内容, 依次为'0','0','0', '0', '0', '0', '0', '2'.
**         10-21: MAC地址:
**             以ASCII码字符串表示MAC地址, 全大写表示.
**             如MAC地址为12:34:56:78:90:AB, 则此域由10-21依次为: '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'A', 'B'.
**         22-31: 保留
**             全部以'0'填充
**         32-?:
**             数据部分, 二进制数据
**
**Author  :
**Date    :2015/4/8
**
********************************************************************************/

//被控端简称target，控制端检测ward
//test cmd

//DEBUG：服务器强制关闭
#define SRV_QUIT 'q'
//DEBUG：接收ward传送的数据并转发给target
#define SENDTEXT 'a'
//DEBUG：预留
#define FMT_TEXT 't'

//clients' cmd 

//由ward发送，经过服务器转发到target，通过target的摄像头得到图片
#define TAKEPIC 'P'
//由target发送，经过服务器转发到ward ， 由target上传图片数据到服务器 ，再由服务器发送至ward
#define SENDPIC 'Q'
//由ward发送，经过服务器转发到target，使target进入传感器信息接收状态
#define GETLOCATION 'L'
//由target发送，将wifi相关数据上传至服务器
#define WIFI_SIG 'W'
//由target发送，将GPS相关数据上传至服务器
#define GPS_SIG 'G'
//由服务器发送，将得到的target位置信息返回给ward
#define SENDLOCATION 'B'
//配置。注册，建立连接
#define REGISTER 'R'
//dump
#define DUMP 'D'


#endif // !CMDFLAG
