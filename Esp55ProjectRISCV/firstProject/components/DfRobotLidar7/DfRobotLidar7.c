#include "UartHandler.h"
#include "DfRobotLidar7.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "portmacro.h"

#define UART_TXD 5
#define UART_RXD 4
#define MAX_BUFFER_SIZE 256
#define LIDAR_PACKET_SIZE 24
#define LIDAR07_VERSION 0
#define LIDAR07_MEASURE 1

#define HEADER 0
#define REG 1
#define DISTANCE1 4
#define DISTANCE2 5
#define AMPLITUDE1 8
#define AMPLITUDE2 9
#define CRC1 20
#define CRC2 21
#define CRC3 22
#define CRC4 23

typedef void (*DFROBOTLIDAR07_READFUNC)(uint8_t data);

const uint32_t crc32MPEG2Table[256] = {
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005, 
    0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61, 0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75, 
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd, 
    0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039, 0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 
    0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d, 
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95, 
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1, 0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 
    0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072, 
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca, 
    0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde, 0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 
    0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba, 
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692, 
    0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6, 0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 
    0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2, 
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a, 
    0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637, 0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53, 
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b, 
    0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff, 0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 
    0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b, 
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3, 
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7, 0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 
    0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3, 
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c, 
    0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8, 0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 
    0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec, 
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654, 
    0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0, 0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 
    0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4, 
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c, 
    0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668, 0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4, 
};

static uint8_t DfRobotLidar7_Buffer[MAX_BUFFER_SIZE];

DFROBOTLIDAR07_PACKET DfRobotLidar7_ReadVersionPacket;
DFROBOTLIDAR07_PACKET DfRobotLidar7_SetIntervalPacket;
DFROBOTLIDAR07_PACKET DfRobotLidar7_SetModePacket;
DFROBOTLIDAR07_PACKET DfRobotLidar7_StartPacket;
DFROBOTLIDAR07_PACKET DfRobotLidar7_StopPacket;
DFROBOTLIDAR07_PACKET DfRobotLidar7_StartFilterPacket;
DFROBOTLIDAR07_PACKET DfRobotLidar7_StopFilterPacket;
DFROBOTLIDAR07_PACKET DfRobotLidar7_SetFreq;
DFROBOTLIDAR07_COLLECTMODE_t DfRobotLidar7_Mode;


static uart_config_t DfRobotLidar7_UartCfg =
{
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
};

static DFROBOTLIDAR07_INSTANCE_t DfRobotLidar7_Instance;
static uint16_t DfRobotLidar7_PacketData = LIDAR_PACKET_SIZE;
static uint16_t DfRobotLidar7_DataStored = 0;

static QueueHandle_t DfRobotLidar7_DataReadyQueue;


/* FUNCTION PROTOTYPES */
static uint32_t DfRobotLidar7_DoCrc32MPEG2Calculate(uint8_t *ptr,uint8_t len);

static void DfRobotLidar7_ReadValue(uint8_t *buff,uint8_t type);

void DfRobotLidar7_MainTask(void *arg);

/* CODE PART */
uint32_t DfRobotLidar7_Read(void* pbuf, size_t size, uint8_t reg)
{
    int len=size;
    uint32_t crc=0xFFFFFFFF;
    uint8_t temp;
    uint8_t *pbuf_ptr = (uint8_t*)pbuf;
    if(reg==0)
    {
        (void)UartHandler_ReadData(pbuf, 1, 0);
        return crc;
    }
    while (UartHandler_ReadData(&temp, 1, 10) == 1U)
    {
        if(temp==0xFA)
        {
            crc= 0xFFFFFFFF;
            *pbuf_ptr = temp;
            crc = (crc << 8) ^ crc32MPEG2Table[(crc >> 24 ^  *pbuf_ptr) & 0xff];
            pbuf_ptr++;
            len--;
            while (len > 0)
            {
                if(UartHandler_ReadData(&temp, 1, 2))
                {
                    *pbuf_ptr=(uint8_t)temp;
                    if(*pbuf_ptr != reg && (len == (size -1)))
                    {
                        return 0;
                    }
                    if(len > 4)
                    {
                        crc = (crc << 8) ^ crc32MPEG2Table[(crc >> 24 ^ *pbuf_ptr) & 0xff];
                    }
                    len--;
                    pbuf_ptr++;
                } 
            }
            break;
        }
    }
  
  return crc;
}

void DfRobotLidar7_Write(void* pbuf, size_t size, uint32_t millisecs)
{
    uint8_t * pbuf_ptr = (uint8_t *)pbuf;
    UartHandler_WriteData(pbuf_ptr, size, millisecs);
}

static void DfRobotLidar7_InitPackets(void)
{
    DfRobotLidar7_ReadVersionPacket.head = 0xF5;
    DfRobotLidar7_ReadVersionPacket.command = 0x43;
    DfRobotLidar7_ReadVersionPacket.data[0] = 0x00;
    DfRobotLidar7_ReadVersionPacket.data[1] = 0x00;
    DfRobotLidar7_ReadVersionPacket.data[2] = 0x00;
    DfRobotLidar7_ReadVersionPacket.data[3] = 0x00;
    DfRobotLidar7_ReadVersionPacket.checkData[0] = 0xAC;
    DfRobotLidar7_ReadVersionPacket.checkData[1] = 0x45;
    DfRobotLidar7_ReadVersionPacket.checkData[2] = 0x62;
    DfRobotLidar7_ReadVersionPacket.checkData[3] = 0x3B;

    DfRobotLidar7_SetModePacket.head = 0xF5;
    DfRobotLidar7_SetModePacket.command = 0xE1;
    DfRobotLidar7_SetModePacket.data[0] = 0x00;
    DfRobotLidar7_SetModePacket.data[1] = 0x00;
    DfRobotLidar7_SetModePacket.data[2] = 0x00;
    DfRobotLidar7_SetModePacket.data[3] = 0x00;
    DfRobotLidar7_SetModePacket.checkData[0] = 0xA5;
    DfRobotLidar7_SetModePacket.checkData[1] = 0x8D;
    DfRobotLidar7_SetModePacket.checkData[2] = 0x89;
    DfRobotLidar7_SetModePacket.checkData[3] = 0xA7;
  
    DfRobotLidar7_StartPacket.head = 0xF5;
    DfRobotLidar7_StartPacket.command = 0xE0;
    DfRobotLidar7_StartPacket.data[0] = 0x01;
    DfRobotLidar7_StartPacket.data[1] = 0x00;
    DfRobotLidar7_StartPacket.data[2] = 0x00;
    DfRobotLidar7_StartPacket.data[3] = 0x00;
    DfRobotLidar7_StartPacket.checkData[0] = 0x9F;
    DfRobotLidar7_StartPacket.checkData[1] = 0x70;
    DfRobotLidar7_StartPacket.checkData[2] = 0xE9;
    DfRobotLidar7_StartPacket.checkData[3] = 0x32;

    DfRobotLidar7_StopPacket.head = 0xF5;
    DfRobotLidar7_StopPacket.command = 0xE0;
    DfRobotLidar7_StopPacket.data[0] = 0x00;
    DfRobotLidar7_StopPacket.data[1] = 0x00;
    DfRobotLidar7_StopPacket.data[2] = 0x00;
    DfRobotLidar7_StopPacket.data[3] = 0x00;
    DfRobotLidar7_StopPacket.checkData[0] = 0x28;
    DfRobotLidar7_StopPacket.checkData[1] = 0xEA;
    DfRobotLidar7_StopPacket.checkData[2] = 0x84;
    DfRobotLidar7_StopPacket.checkData[3] = 0xEE;

    DfRobotLidar7_StartFilterPacket.head = 0xF5;
    DfRobotLidar7_StartFilterPacket.command = 0xD9;
    DfRobotLidar7_StartFilterPacket.data[0] = 0x01;
    DfRobotLidar7_StartFilterPacket.data[1] = 0x00;
    DfRobotLidar7_StartFilterPacket.data[2] = 0x00;
    DfRobotLidar7_StartFilterPacket.data[3] = 0x00;
    DfRobotLidar7_StartFilterPacket.checkData[0] = 0xB7;
    DfRobotLidar7_StartFilterPacket.checkData[1] = 0x1F;
    DfRobotLidar7_StartFilterPacket.checkData[2] = 0xBA;
    DfRobotLidar7_StartFilterPacket.checkData[3] = 0xBA;
   
    DfRobotLidar7_StopFilterPacket.head = 0xF5;
    DfRobotLidar7_StopFilterPacket.command = 0xD9;
    DfRobotLidar7_StopFilterPacket.data[0] = 0x00;
    DfRobotLidar7_StopFilterPacket.data[1] = 0x00;
    DfRobotLidar7_StopFilterPacket.data[2] = 0x00;
    DfRobotLidar7_StopFilterPacket.data[3] = 0x00;
    DfRobotLidar7_StopFilterPacket.checkData[0] = 0x00;
    DfRobotLidar7_StopFilterPacket.checkData[1] = 0x85;
    DfRobotLidar7_StopFilterPacket.checkData[2] = 0xD7;
    DfRobotLidar7_StopFilterPacket.checkData[3] = 0x66;

    DfRobotLidar7_SetFreq.head = 0xF5;
    DfRobotLidar7_SetFreq.command = 0xE2;
    DfRobotLidar7_SetFreq.data[0] = 0x00;
    DfRobotLidar7_SetFreq.data[1] = 0x00;
    DfRobotLidar7_SetFreq.data[2] = 0x00;
    DfRobotLidar7_SetFreq.data[3] = 0x00;
    DfRobotLidar7_SetFreq.checkData[0] = 0x00;
    DfRobotLidar7_SetFreq.checkData[1] = 0x00;
    DfRobotLidar7_SetFreq.checkData[2] = 0x00;
    DfRobotLidar7_SetFreq.checkData[3] = 0x00;
}

bool DfRobotLidar7_SetMeasureMode(DFROBOTLIDAR07_COLLECTMODE_t mode)
{
    uint8_t buff[12];
    bool ret = false;
    if(mode==eLidar07Continuous)
    {
        DfRobotLidar7_SetModePacket.data[0]=0x01;
        uint32_t modeCrcData=0;
        modeCrcData=DfRobotLidar7_DoCrc32MPEG2Calculate((uint8_t*)&DfRobotLidar7_SetModePacket,6);
        DfRobotLidar7_SetModePacket.checkData[0]=modeCrcData &0xFF;
        DfRobotLidar7_SetModePacket.checkData[1]=(modeCrcData>>8) &0xFF;
        DfRobotLidar7_SetModePacket.checkData[2]=(modeCrcData>>16) &0xFF;
        DfRobotLidar7_SetModePacket.checkData[3]=(modeCrcData>>24) &0xFF;
    }
    DfRobotLidar7_Write((void *)&DfRobotLidar7_SetModePacket,sizeof(DFROBOTLIDAR07_PACKET), 20);
    uint32_t crc=DfRobotLidar7_Read((void *)buff,12,DfRobotLidar7_SetModePacket.command);
    uint32_t checkData = (uint32_t)buff[8] | ((uint32_t)buff[9]<<8) | ((uint32_t)buff[10]<<16) | ((uint32_t)buff[11]<<24);
    if(crc==checkData)
    {
        ret = true;
        ESP_LOGW("DfRobotLidar", "Set mode %d", mode);
    }
    return ret;
}

bool DfRobotLidar7_SetConMeasurePeriod(uint32_t period)
{
    uint8_t buff[12];
    bool ret = false;
    uint32_t periodCrcData=0;
    DfRobotLidar7_SetFreq.data[0]=period & 0xFF;
    DfRobotLidar7_SetFreq.data[1]=(period>>8) & 0xFF;
    DfRobotLidar7_SetFreq.data[2]=(period>>16) & 0xFF;
    DfRobotLidar7_SetFreq.data[3]=(period>>24) & 0xFF;
    periodCrcData=DfRobotLidar7_DoCrc32MPEG2Calculate((uint8_t*)&DfRobotLidar7_SetFreq,6);
    DfRobotLidar7_SetFreq.checkData[0]=periodCrcData &0xFF;
    DfRobotLidar7_SetFreq.checkData[1]=(periodCrcData>>8) &0xFF;
    DfRobotLidar7_SetFreq.checkData[2]=(periodCrcData>>16) &0xFF;
    DfRobotLidar7_SetFreq.checkData[3]=(periodCrcData>>24) &0xFF;
    DfRobotLidar7_Write((void *)&DfRobotLidar7_SetFreq,sizeof(DFROBOTLIDAR07_PACKET), 20);
    uint32_t crc=DfRobotLidar7_Read((void *)buff,12,DfRobotLidar7_SetFreq.command);
    uint32_t checkData = (uint32_t)buff[8] | ((uint32_t)buff[9]<<8) | ((uint32_t)buff[10]<<16) | ((uint32_t)buff[11]<<24);
    if(crc==checkData){
        ret = true;
        ESP_LOGW("DfRobotLidar", "Set measurement frequency");
    }
    return ret;
}


bool DfRobotLidar7_StartFilter(void)
{
    uint8_t buff[12];
    bool ret = false;
    DfRobotLidar7_Write((void *)&DfRobotLidar7_StartFilterPacket,sizeof(DFROBOTLIDAR07_PACKET), 20);
    uint32_t crc= DfRobotLidar7_Read((void *)buff,12,DfRobotLidar7_StartFilterPacket.command);
    uint32_t checkData = (uint32_t)buff[8] | ((uint32_t)buff[9]<<8) | ((uint32_t)buff[10]<<16) | ((uint32_t)buff[11]<<24);

    if(crc==checkData)
    {
        ret = true;
        ESP_LOGW("DfRobotLidar", "FILTER STARTED");
    }
    return ret;
}

bool DfRobotLidar7_StopFilter(void)
{
    uint8_t buff[12];
    bool ret = false;
    DfRobotLidar7_Write((void *)&DfRobotLidar7_StopFilterPacket,sizeof(DFROBOTLIDAR07_PACKET), 20);
    uint32_t crc=DfRobotLidar7_Read((void *)buff,12,DfRobotLidar7_StopFilterPacket.command);
    uint32_t checkData = (uint32_t)buff[8] | ((uint32_t)buff[9]<<8) | ((uint32_t)buff[10]<<16) | ((uint32_t)buff[11]<<24);
    if(crc==checkData)
    {
        ret = true;
        ESP_LOGW("DfRobotLidar", "FILTER STOPPED");
    }
    return ret;
}

void DfRobotLidar7_StartMeasure(void)
{
    DfRobotLidar7_Write((void *)&DfRobotLidar7_StartPacket, sizeof(DFROBOTLIDAR07_PACKET), 20);
}


static bool DfRobotLidar7_InitCrc(void)
{
    bool ret = false;
    uint8_t buff[12]={0};
    DfRobotLidar7_Read((void *)buff,12,0);
    DfRobotLidar7_SetMeasureMode(eLidar07Single);
    DfRobotLidar7_Read((void *)buff,12,0);
    DfRobotLidar7_Write((void *)(&DfRobotLidar7_ReadVersionPacket),sizeof(DFROBOTLIDAR07_PACKET), 20);
    uint32_t crc=DfRobotLidar7_Read((void *)buff,12,DfRobotLidar7_ReadVersionPacket.command);
    uint32_t checkData = (uint32_t)buff[8] | ((uint32_t)buff[9]<<8) | ((uint32_t)buff[10]<<16) | ((uint32_t)buff[11]<<24);
    if(crc==checkData)
    {
        DfRobotLidar7_ReadValue(buff,LIDAR07_VERSION);
        ret = true;
        ESP_LOGW("DfRobotLidar", "CRC HAS BEEN INITIALIZED");
    }
    return  ret;
}

static uint32_t DfRobotLidar7_DoCrc32MPEG2Calculate(uint8_t *ptr,uint8_t len)
{  
    uint8_t data;
    uint32_t crc = 0xFFFFFFFF;
    int i;

    for (; len > 0; len--){
        data = *ptr++;
        crc = crc ^ ((uint32_t)data<<24);
        for (i = 0; i < 8; i++){
            if (crc & 0x80000000)
                crc = (crc << 1) ^0x04C11DB7;
            else
                crc <<= 1;
        }
    }

    crc = crc^0x00;
    return(crc);
}

static void DfRobotLidar7_ReadValue(uint8_t *buff,uint8_t type)
{
    if(type == LIDAR07_VERSION)
    {
      DfRobotLidar7_Instance.version = (uint32_t)buff[4] | ((uint32_t)buff[5]<<8) | ((uint32_t)buff[6]<<16) | ((uint32_t)buff[7]<<24);
    } 
    else if(type == LIDAR07_MEASURE)
    {
      DfRobotLidar7_Instance.distance = (uint16_t)buff[4] | ((uint16_t)buff[5]<<8);
      DfRobotLidar7_Instance.amplitude = (uint16_t)buff[8] | ((uint16_t)buff[9]<<8);
    }
}

static void DfRobotLidar7_HEADER(uint8_t data)
{
    if (data == 0xFA) 
    {
        DfRobotLidar7_Instance.crc = (0xFFFFFFFF << 8) ^ crc32MPEG2Table[((0xFFFFFFFF >> 24) ^  data) & 0xff];
        DfRobotLidar7_DataStored++;
    }
}

static void DfRobotLidar7_REG(uint8_t data)
{
    if (data == DfRobotLidar7_StartPacket.command) 
    {
        DfRobotLidar7_Instance.crc = (DfRobotLidar7_Instance.crc << 8) ^ crc32MPEG2Table[((DfRobotLidar7_Instance.crc >> 24) ^ data) & 0xff];
        DfRobotLidar7_DataStored++;
    }
    else
    {
        DfRobotLidar7_DataStored = HEADER;
    }
}

static void DfRobotLidar7_DIAGNOSTICS(uint8_t data)
{
    static uint8_t diag_index = 0;
    DfRobotLidar7_Instance.crc = (DfRobotLidar7_Instance.crc << 8) ^ crc32MPEG2Table[((DfRobotLidar7_Instance.crc >> 24) ^ data) & 0xff];
    DfRobotLidar7_Instance.diagnostics[diag_index] = (uint16_t)data;
    if (diag_index == 1)
    {
        diag_index = 0;
    }
    else
    {
        diag_index++;
    }
    DfRobotLidar7_DataStored++;
}

static void DfRobotLidar7_DISTANCE1(uint8_t data)
{
    DfRobotLidar7_Instance.crc = (DfRobotLidar7_Instance.crc << 8) ^ crc32MPEG2Table[((DfRobotLidar7_Instance.crc >> 24) ^ data) & 0xff];
    DfRobotLidar7_Instance.distance = (uint16_t)data;
    DfRobotLidar7_DataStored++;
}

static void DfRobotLidar7_DISTANCE2(uint8_t data)
{
    DfRobotLidar7_Instance.crc = (DfRobotLidar7_Instance.crc << 8) ^ crc32MPEG2Table[((DfRobotLidar7_Instance.crc >> 24) ^ data) & 0xff];
    DfRobotLidar7_Instance.distance |= ((uint16_t)data << 8);
    DfRobotLidar7_DataStored++;
    ESP_LOGW("DfRobotLidar7", "exw error?  %u", DfRobotLidar7_Instance.distance);
}

static void DfRobotLidar7_AMPLITUDE1(uint8_t data)
{
    DfRobotLidar7_Instance.crc = (DfRobotLidar7_Instance.crc << 8) ^ crc32MPEG2Table[((DfRobotLidar7_Instance.crc >> 24) ^ data) & 0xff];
    DfRobotLidar7_Instance.amplitude = (uint16_t)data;
    DfRobotLidar7_DataStored++;
}

static void DfRobotLidar7_AMPLITUDE2(uint8_t data)
{
    DfRobotLidar7_Instance.crc = (DfRobotLidar7_Instance.crc << 8) ^ crc32MPEG2Table[((DfRobotLidar7_Instance.crc >> 24) ^ data) & 0xff];
    DfRobotLidar7_Instance.amplitude |= ((uint16_t)data << 8);
    DfRobotLidar7_DataStored++;
}

static void DfRobotLidar7_AMBIENT1(uint8_t data)
{
    DfRobotLidar7_Instance.crc = (DfRobotLidar7_Instance.crc << 8) ^ crc32MPEG2Table[((DfRobotLidar7_Instance.crc >> 24) ^ data) & 0xff];
    DfRobotLidar7_Instance.ambient_light = (uint16_t)data;
    DfRobotLidar7_DataStored++;
}

static void DfRobotLidar7_AMBIENT2(uint8_t data)
{
    DfRobotLidar7_Instance.crc = (DfRobotLidar7_Instance.crc << 8) ^ crc32MPEG2Table[((DfRobotLidar7_Instance.crc >> 24) ^ data) & 0xff];
    DfRobotLidar7_Instance.ambient_light |= ((uint16_t)data << 8);
    DfRobotLidar7_DataStored++;
}

static void DfRobotLidar7_TOF(uint8_t data)
{
    static uint8_t tof_index = 0;
    DfRobotLidar7_Instance.crc = (DfRobotLidar7_Instance.crc << 8) ^ crc32MPEG2Table[((DfRobotLidar7_Instance.crc >> 24) ^ data) & 0xff];
    DfRobotLidar7_Instance.tof_info[tof_index] = (uint16_t)data;
    if (tof_index == 7)
    {
        tof_index = 0;
    }
    else
    {
        tof_index++;
    }
    DfRobotLidar7_DataStored++;
}

static void DfRobotLidar7_CRC1(uint8_t data)
{
    DfRobotLidar7_Instance.crcCalc = (uint32_t)data;
    DfRobotLidar7_DataStored++;
}

static void DfRobotLidar7_CRC2(uint8_t data)
{
    DfRobotLidar7_Instance.crcCalc |= ((uint32_t)data << 8);
    DfRobotLidar7_DataStored++;
}

static void DfRobotLidar7_CRC3(uint8_t data)
{
    DfRobotLidar7_Instance.crcCalc |= ((uint32_t)data << 16);
    DfRobotLidar7_DataStored++;
}

static void DfRobotLidar7_CRC4(uint8_t data)
{
    DfRobotLidar7_Instance.crcCalc |= ((uint32_t)data << 24);
    DfRobotLidar7_DataStored = HEADER;
    bool crc_check = (DfRobotLidar7_Instance.crcCalc == DfRobotLidar7_Instance.crc) ? true : false;
    if (xQueueSend(DfRobotLidar7_DataReadyQueue, &crc_check, 0) != pdTRUE)
    {
        ESP_LOGW("DfRobotLidar7", "Queue is full");
    }
}

static void DfRobotLidar7_DEFAULT(uint8_t data)
{
    DfRobotLidar7_Instance.crc = (DfRobotLidar7_Instance.crc << 8) ^ crc32MPEG2Table[((DfRobotLidar7_Instance.crc >> 24) ^ data) & 0xff];
    DfRobotLidar7_DataStored++;
}

DFROBOTLIDAR07_READFUNC DfRoboLidar7_ReadFuncArray[LIDAR_PACKET_SIZE] = 
{
    DfRobotLidar7_HEADER,
    DfRobotLidar7_REG,
    DfRobotLidar7_DIAGNOSTICS,
    DfRobotLidar7_DIAGNOSTICS,
    DfRobotLidar7_DISTANCE1,
    DfRobotLidar7_DISTANCE2,
    DfRobotLidar7_DEFAULT,
    DfRobotLidar7_DEFAULT,
    DfRobotLidar7_AMPLITUDE1,
    DfRobotLidar7_AMPLITUDE2,
    DfRobotLidar7_AMBIENT1,
    DfRobotLidar7_AMBIENT2,
    DfRobotLidar7_TOF,
    DfRobotLidar7_TOF,
    DfRobotLidar7_TOF,
    DfRobotLidar7_TOF,
    DfRobotLidar7_TOF,
    DfRobotLidar7_TOF,
    DfRobotLidar7_TOF,
    DfRobotLidar7_TOF,
    DfRobotLidar7_CRC1,
    DfRobotLidar7_CRC2,
    DfRobotLidar7_CRC3,
    DfRobotLidar7_CRC4
};

static void DfRobotLidar7_RxCallback(uint8_t* data, int size)
{
    for (uint8_t i = 0; i < size; i++)
    {
        DfRoboLidar7_ReadFuncArray[DfRobotLidar7_DataStored](data[i]);
    }
}

void DfRobotLidar7_Init(DFROBOTLIDAR07_COLLECTMODE_t mode)
{
    DfRobotLidar7_DataReadyQueue = xQueueCreate(1, sizeof(bool));
    if (DfRobotLidar7_DataReadyQueue == NULL)
    {
        ESP_LOGE("DfRobotLidar7", "Queue creation failed!");
    }
    DfRobotLidar7_InitPackets();
    UartHandler_Init(&DfRobotLidar7_UartCfg, UART_NUM_1, DfRobotLidar7_Buffer, UART_TXD, UART_RXD, MAX_BUFFER_SIZE, DfRobotLidar7_RxCallback, &DfRobotLidar7_PacketData);
    while(!DfRobotLidar7_InitCrc());
    if (mode == eLidar07Continuous)
    {
        while(!DfRobotLidar7_StopFilter());
        while(!DfRobotLidar7_StartFilter());

        while(!DfRobotLidar7_SetMeasureMode(eLidar07Continuous));

        /* 100 MS TASK */
        while(!DfRobotLidar7_SetConMeasurePeriod(100));

        DfRobotLidar7_StartMeasure();
    }
    xTaskCreate(DfRobotLidar7_MainTask, "DfRobotLidar7_MainTask", 2048, NULL, 9, NULL);
    UartHandler_TaskInit();
}


void DfRobotLidar7_MainTask(void *arg)
{
    bool crc_check;
    TickType_t lastWakeTime = xTaskGetTickCount();

    while(1)
    {
        if (xQueueReceive(DfRobotLidar7_DataReadyQueue, &crc_check, portMAX_DELAY))
        {
            TickType_t now = xTaskGetTickCount();
            TickType_t diff = now - lastWakeTime;
            lastWakeTime = now;

            ESP_LOGI("DfRobotLidar7", "CRC: %d, Interval (ticks): %u, Interval (ms): %u", 
                      crc_check, (unsigned int)diff, (unsigned int)(diff * portTICK_PERIOD_MS));
        }
    }
}
