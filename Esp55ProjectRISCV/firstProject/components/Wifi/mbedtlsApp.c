
#include <string.h>
#include "lwip/apps/mqtt.h"
#include "stdbool.h"
#include "mbedtlsApp_types.h"
#include "mbedtls/net_sockets.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sntp.h"

/* Includes ------------------------------------------------------------------*/
#include "mbedtlsApp.h"

#define MQTT_ANSWER_BYTES_4 (uint16_t)4u

#define MQTT_CONNECT_TYPE (uint8_t)0x10U
#define MQTT_CONNACK_TYPE (uint8_t)0x20U
#define MQTT_CONNECT_HEADER_DEFAULT_LEN (uint8_t)10U
#define MQTT_LEN_INDEX_DEFAULT (uint8_t)1u
#define MQTT_LENGTH (uint16_t)4U
#define MQTT_PROTOCOL_LEVEL (uint8_t)4U
#define MQTT_CONNECT_FLAG_FULL_CLEAN (uint8_t)0xC2U
#define MQTT_KEEPALIVE_60SECS (uint16_t)60U

#define MQTT_PUBLISH_TYPE_QOS0 (uint8_t)0X30
#define MQTT_PUBLISH_TYPE_QOS1 (uint8_t)0X32
#define MQTT_PUBLISH_TYPE_QOS2 (uint8_t)0X34

#define MQTT_PUBACK_TYPE (uint8_t)0x40

#define MQTT_PINGREQ_TYPE (uint8_t)0xC0
#define MQTT_PINGRESP_TYPE (uint8_t)0xD0

typedef enum
{
  WAIT_HANDSHAKE,
  CONNECT,
  CONNACK,
  WRITE,
  READ,
  PINGREQ,
  PINGRESP,
  PUBLISH,
  PUBACK
}MBEDTLS_MQTT_STATE;


/* Constnats   ------------------------------------------------------------------*/

/********************************** EDIT THIS *********************************** */
const char *adafruit_io_ca =
"-----BEGIN CERTIFICATE-----\n"
"mycert\n"
"-----END CERTIFICATE-----\n";


const char mqtt_clientName[] = "esp32p4client";
const char mqtt_userName[] = "myname";
const char mqtt_password[] = "mypass";
const char mqtt_protocolAsci[] = "MQTT";

static MQTT_CLIENT_ID mqtt_client;

static MQTT_USERNAME mqtt_user;

static MQTT_PASSWORD mqtt_pass;

const int mqtt_ciphers[] = {
		MBEDTLS_TLS_RSA_WITH_AES_256_GCM_SHA384,
		MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,
	  0
};

static bool mqtt_publishTest = false;


/********************************** EDIT THIS *********************************** */

static unsigned char mqtt_buffer[1500];

static MQTT_CONNECT_HEADER mqtt_connectHeader =
{
  MQTT_CONNECT_TYPE,
  {
    0x0,
    0x4
  },
  {'M', 'Q', 'T', 'T'},
  MQTT_PROTOCOL_LEVEL,
  MQTT_CONNECT_FLAG_FULL_CLEAN,
  {0x0, 0x3C} /* 60 Seconds for now */
};
static MQTT_PUBLISH_CFG mqtt_publishCfg = {
  MQTT_PUBLISH_TYPE_QOS1,
  0U,
  0U
};

static char publishpayload[] = "1312";

static MQTT_PAYLOAD MqttPayload;

static char MqttTopic[] = "mytopic";

static MBEDTLS_MQTT_STATE mqtt_state;
static MBEDTLS_MQTT_STATE mqtt_nextstate;
/* Global variables ---------------------------------------------------------*/
mbedtls_ssl_context ssl;
mbedtls_ssl_config conf;
mbedtls_x509_crt cert;
mbedtls_ctr_drbg_context ctr_drbg;
mbedtls_entropy_context entropy;

mbedtls_net_context server_fd;
size_t packet_len;
static MQTT_HANDLER_DATA_INFO mqtt_DataInfo;

void debug_cb(void *ctx, int level,
              const char *file, int line,
              const char *str)
{
    (void) ctx;
    printf("[%d] %s:%d: %s\n", level, file, line, str);
}

static uint32_t MbedTls_Mqtt_Connect_CreateMessage(unsigned char *buf)
{
  uint32_t len_prv = MQTT_CONNECT_HEADER_DEFAULT_LEN + mqtt_client.actl_size + 2
		            + mqtt_user.actl_size + 2 + mqtt_pass.actl_size + 2;
  uint8_t lengthBytesNum = 0;

  *buf++ = mqtt_connectHeader.type;
  /* this will be the len */
  uint32_t x = len_prv;
  do {
      uint8_t encoded = x % 128;
      x /= 128;
      if (x > 0)
          encoded |= 0x80;
      *buf++ = encoded;
      lengthBytesNum++;
  } while (x > 0);

  memcpy(buf, mqtt_connectHeader.protocol_name_length, 2);
  buf += 2;
  memcpy(buf, mqtt_connectHeader.protocolName, 4);
  buf += 4;
  *buf++ = mqtt_connectHeader.protocol_level;
  *buf++ = mqtt_connectHeader.connect_flags;
  memcpy(buf, mqtt_connectHeader.keep_alive, 2);
  buf += 2;

  memcpy(buf, mqtt_client.size, 2);
  buf += 2;
  memcpy(buf, mqtt_client.name, mqtt_client.actl_size);
  buf +=   mqtt_client.actl_size;

  memcpy(buf, mqtt_user.size, 2);
  buf += 2;
  memcpy(buf, mqtt_user.name, mqtt_user.actl_size);
  buf +=  mqtt_user.actl_size;

  memcpy(buf, mqtt_pass.size, 2);
  buf += 2;
  memcpy(buf, mqtt_pass.name, mqtt_pass.actl_size);
  buf +=   mqtt_pass.actl_size;

  len_prv += lengthBytesNum + 1u;

  return len_prv;
}

static bool MbedTls_Mqtt_Connack(unsigned char *buf)
{
	return ((buf[0] == MQTT_CONNACK_TYPE) /* Connack */
			&& (buf[1] == 0x02) /* length 2 */
			&& (buf[2] == 0x00)); /* new session */

}

static uint32_t MbetTls_Mqtt_Publish_CreatePacket(unsigned char *buf, MQTT_PAYLOAD *payload, char *topic, uint16_t topicLen)
{
  uint32_t len_prv = 0u;
  uint8_t lengthBytesNum = 0;
  mqtt_publishCfg.PayloadSize = payload->size;
  len_prv = mqtt_publishCfg.PayloadSize + topicLen + 2 + 2; /* 2 bytes payload, 2 bytes topic and 2 bytes identifier */

  /* Store the type */
  *buf++ = mqtt_publishCfg.Type;

  /* Evaluate how big is the size */
  // MQTT Variable-Length Remaining Length
  uint32_t x = len_prv;
  do {
      uint8_t encoded = x % 128;
      x /= 128;
      if (x > 0)
          encoded |= 0x80;
      *buf++ = encoded;
      lengthBytesNum++;
  } while (x > 0);

  /* Store the topic len and then the topic */
  *buf++ = (uint8_t)(topicLen >> 8);
  *buf++ = (uint8_t)(topicLen);
  memcpy(buf, topic, (int)topicLen);
  buf+= topicLen;

  /* Store the packet id and after that increase it*/
  *buf++ = (uint8_t)(mqtt_publishCfg.PacketId >> 8);
  *buf++ = (uint8_t)(mqtt_publishCfg.PacketId);

  /* Store the payload */
  memcpy(buf, payload->payload, mqtt_publishCfg.PayloadSize);
  buf += mqtt_publishCfg.PayloadSize;

  /* Increase to get the type and the len */
  len_prv += 1u + lengthBytesNum;
  return len_prv;
}

static bool MbetTls_Mqtt_Puback(uint8_t *buf)
{
  bool ret = true;
  if (mqtt_publishCfg.Type != MQTT_PUBLISH_TYPE_QOS0)
  {
    ret =  ((buf[0] == MQTT_PUBACK_TYPE) &&
            (buf[1] == 0x02) &&
            (buf[2] == (uint8_t)(mqtt_publishCfg.PacketId >> 8)) &&
            (buf[3] == (uint8_t)(mqtt_publishCfg.PacketId & 0x00FF)));
  }
  else
  {
    ret = true;
  }
  /* Increase the packetId for the next message */
  mqtt_publishCfg.PacketId++;

  return ret;
}

static uint8_t MbedTls_Mqtt_PingReq_Create(unsigned char *buf)
{
    buf[0] = MQTT_PINGREQ_TYPE; // PINGREQ type (1100 0000)
    buf[1] = 0x00; // Remaining length = 0
    return 2;      // Always 2 bytes
}

static bool MbedTls_Mqtt_PingResp_Received(unsigned char *buf, uint16_t len)
{
    // PINGRESP type is 0xD0, remaining length is 0
    return ((len == 2) &&
           (buf[0] == MQTT_PINGRESP_TYPE) &&
           (buf[1] == 0x00));
}

/* MBEDTLS init function */
void MX_MBEDTLS_Init(void)
{
  mbedtls_ssl_init(&ssl);
  mbedtls_ssl_config_init(&conf);
  mbedtls_x509_crt_init(&cert);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  mbedtls_entropy_init( &entropy );
  uint8_t test = 0;
   /* Initialize the test payload */
  MqttPayload.payload = publishpayload;
  MqttPayload.size = strlen(publishpayload);

  /* Initialize the password, user and client */
  mqtt_client.actl_size = strlen(mqtt_clientName);
  mqtt_client.name = mqtt_clientName;
  mqtt_client.size[0] = (uint8_t)(mqtt_client.actl_size >> 8);
  mqtt_client.size[1] = (uint8_t)(mqtt_client.actl_size & 0x00FF);

  mqtt_user.actl_size = strlen(mqtt_userName);
  mqtt_user.name = mqtt_userName;
  mqtt_user.size[0] = (uint8_t)(mqtt_user.actl_size >> 8);
  mqtt_user.size[1] = (uint8_t)(mqtt_user.actl_size & 0x00FF);

  mqtt_pass.actl_size = strlen(mqtt_password);
  mqtt_pass.name = mqtt_password;
  mqtt_pass.size[0] = (uint8_t)(mqtt_pass.actl_size >> 8);
  mqtt_pass.size[1] = (uint8_t)(mqtt_pass.actl_size & 0x00FF);
  int ret;
  ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);
  if (ret != 0) {
    test |= 1;
  } else {
    test |= 0;
  }
   /* Initialize the data needed for the MBEDTLS */
  ret = mbedtls_x509_crt_parse(&cert, (const unsigned char *)adafruit_io_ca, strlen(adafruit_io_ca) + 1);
  if (ret != 0) {
    test |= 2;
  } else {
    test |= 0;
  }
  // Set up SSL config
  mbedtls_ssl_config_defaults(&conf,
	MBEDTLS_SSL_IS_CLIENT,
	MBEDTLS_SSL_TRANSPORT_STREAM,
	MBEDTLS_SSL_PRESET_DEFAULT);

  mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
  mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
  mbedtls_ssl_conf_ca_chain(&conf, &cert, NULL);
  mbedtls_ssl_conf_ciphersuites(&conf, mqtt_ciphers);
}

void MX_MBEDTLS_INIT_HW(void)
{
	mbedtls_net_init(&server_fd);
}

static void MX_MBEDTLS_INIT_TASK(void)
{
	int ret = 0;
	mqtt_state = WAIT_HANDSHAKE;
	mqtt_nextstate = WAIT_HANDSHAKE;
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

  /* Wait till the time is ready */
    while(sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED)
    {}
	mbedtls_ssl_conf_dbg(&conf, debug_cb, NULL);
	mbedtls_debug_set_threshold(4);
	mbedtls_ssl_setup(&ssl, &conf);
	ret = mbedtls_ssl_set_hostname(&ssl, "io.adafruit.com");
	if (ret != 0)
	{
		printf("Bullshit it failed \n");
		return;
	}
	ret = mbedtls_net_connect(&server_fd, "io.adafruit.com", "8883", MBEDTLS_NET_PROTO_TCP);
	if(ret != 0) {
	    printf("TCP connect failed: %d\n", ret);
	    return;
	}
    // Optional non-blocking
    mbedtls_net_set_nonblock(&server_fd);

    // Link TLS to transport
    mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);
}


static bool MbedTls_MqttWriteHandler(unsigned char *buf)
{
	bool status_success = false;
	uint16_t bufIdx = mqtt_DataInfo.dataSize - mqtt_DataInfo.dataRemain;
	/* Here we check if we sent all the data */
	int ret = mbedtls_ssl_write(&ssl, &buf[bufIdx], mqtt_DataInfo.dataRemain);
    if (ret > 0)
    {
    	if (mqtt_DataInfo.dataRemain <= ret)
    	{
    		mqtt_DataInfo.dataRemain = 0;
    		status_success = true;
    	}
    	else
    	{
        	mqtt_DataInfo.dataRemain -= ret;
    	}
    }
    else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE || ret == MBEDTLS_ERR_SSL_WANT_READ)
    {
    	mqtt_DataInfo.writefail++;
    }
    else
    {
       /* Do nothing there please */
    }
    return status_success;
}

static bool MbedTls_MqttReadHandler(unsigned char *buf)
{
	bool status_success = false;
	uint16_t bufIdx = mqtt_DataInfo.dataSize - mqtt_DataInfo.dataRemain;
	/* Here we check if we sent all the data */
	int ret = mbedtls_ssl_read(&ssl, &buf[bufIdx], mqtt_DataInfo.dataRemain);
    if (ret > 0)
    {
    	if (mqtt_DataInfo.dataRemain <= ret)
    	{
    		mqtt_DataInfo.dataRemain = 0;
    		status_success = true;
    	}
    	else
    	{
        	mqtt_DataInfo.dataRemain -= ret;
    	}
    }
    else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE || ret == MBEDTLS_ERR_SSL_WANT_READ)
    {
    	mqtt_DataInfo.readfail++;
    }
    else
    {
       /* Do nothing there please */
    }
    return status_success;
}

/* Big stack right there this is the reason the task will be huge */
static bool MbedTls_HandShake(void)
{
    int ret = mbedtls_ssl_handshake(&ssl);
    return (0 == ret);
}


void Mqtt_Main_Task(void * argument)
{
	MX_MBEDTLS_INIT_TASK();
    TickType_t xLastWakeTime;
    (void)argument;
    xLastWakeTime = xTaskGetTickCount();
	for (;;)
	{
    switch (mqtt_state)
    {
    case WAIT_HANDSHAKE:
      {
        if (MbedTls_HandShake())
        {
          mqtt_state = CONNECT;
        }
      }
      break;

    case CONNECT:
      {
          mqtt_DataInfo.dataRemain = MbedTls_Mqtt_Connect_CreateMessage(&mqtt_buffer[0]);
          /* Store the data that we are about to send */
          mqtt_DataInfo.dataSize = mqtt_DataInfo.dataRemain;
          mqtt_state = WRITE;
          mqtt_DataInfo.dataRead = 4;
          mqtt_nextstate = CONNACK;
      }
      break;

    case CONNACK:
      {
        if (MbedTls_Mqtt_Connack(&mqtt_buffer[0]))
        {
          mqtt_state = PINGREQ;
        }
        else
        {
          mqtt_state = CONNECT;
        }
      }
      break;

    case PINGREQ:
      {
        mqtt_DataInfo.dataRemain = MbedTls_Mqtt_PingReq_Create(&mqtt_buffer[0]);
        mqtt_DataInfo.dataSize = mqtt_DataInfo.dataRemain;
        mqtt_state = WRITE;
        mqtt_DataInfo.dataRead = 2;
        mqtt_nextstate = PINGRESP;
      }
      break;

    case PINGRESP:
      {
        if (MbedTls_Mqtt_PingResp_Received(&mqtt_buffer[0], mqtt_DataInfo.dataRead))
        {
          if (!mqtt_publishTest)
          {
            mqtt_publishTest = true;
            mqtt_state = PUBLISH;
            mqtt_nextstate = PUBACK;
          }
          else 
          {
            mqtt_state = PINGREQ;
            mqtt_nextstate = PINGRESP;
          }
        }
        else
        {
          mqtt_state = WAIT_HANDSHAKE;
        }
      }
      break;

    case PUBLISH:
      {
        mqtt_DataInfo.dataRemain = MbetTls_Mqtt_Publish_CreatePacket(&mqtt_buffer[0], &MqttPayload, MqttTopic, strlen(MqttTopic));
        mqtt_DataInfo.dataSize = mqtt_DataInfo.dataRemain;
        mqtt_state = WRITE;
        mqtt_DataInfo.dataRead = 4;
        mqtt_nextstate = PUBACK;
      }
      break;

    case PUBACK:
      {
        if (MbetTls_Mqtt_Puback(&mqtt_buffer[0]))
        {
          mqtt_state = PINGREQ;
        }
        else
        {
          mqtt_state = WAIT_HANDSHAKE;
        }
      }
      break;

    case WRITE:
      {
        if (MbedTls_MqttWriteHandler(&mqtt_buffer[0]))
        {
          /* WRITE IS FINISHED READ ANSWER */
          mqtt_DataInfo.dataSize = mqtt_DataInfo.dataRead;
          mqtt_DataInfo.dataRemain = mqtt_DataInfo.dataRead;
          mqtt_state = READ;
        }
      }
      break;

    case READ:
      {
        if (MbedTls_MqttReadHandler(&mqtt_buffer[0]))
        {
          mqtt_state = mqtt_nextstate;
        }
      }
      break;

      default:
      {
        break;
      }
    }
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
	}
}
