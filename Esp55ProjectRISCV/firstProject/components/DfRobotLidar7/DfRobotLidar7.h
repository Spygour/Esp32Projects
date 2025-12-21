#ifndef DFROBOTLIDAR7_H_
#define DFROBOTLIDAR7_H_

#include <stdint.h>


typedef struct {
  uint8_t head;
  uint8_t command;
  uint8_t data[4];
  uint8_t checkData[4];
}DFROBOTLIDAR07_PACKET;

typedef enum{
  eLidar07Single = 0,         /**< A single collection*/
  eLidar07Continuous = 1,     /**< Continuous acquisition*/
}DFROBOTLIDAR07_COLLECTMODE_t;


typedef struct
{
  uint8_t diagnostics[2];
  uint16_t distance;
  uint16_t amplitude;
  uint16_t ambient_light;
  uint8_t tof_info[8];
  uint32_t version;
  uint32_t _type;
  uint32_t crc;
  uint32_t crcCalc;
}DFROBOTLIDAR07_INSTANCE_t;

void DfRobotLidar7_Init(DFROBOTLIDAR07_COLLECTMODE_t mode);
bool DfRobotLidar7_StartFilter(void);
bool DfRobotLidar7_SetMeasureMode(DFROBOTLIDAR07_COLLECTMODE_t mode);
bool DfRobotLidar7_SetConMeasureFreq(uint32_t frqe);
void DfRobotLidar7_StartMeasure(void);
#endif