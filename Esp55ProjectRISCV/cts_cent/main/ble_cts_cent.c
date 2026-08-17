/*
 * SPDX-FileCopyrightText: 2017-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "nvs_flash.h"
/* BLE */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "console/console.h"
#include "os/os_mbuf.h"
#include "services/gap/ble_svc_gap.h"
#include "ble_cts_cent.h"
#include "services/cts/ble_svc_cts.h"
#if MYNEWT_VAL(BLE_GATT_CACHING)
#include "host/ble_esp_gattc_cache.h"
#endif

typedef struct 
{
  bool current_state;
  int64_t last_time_changed; 
  bool previous_state;
}door_info_t;

static const ble_uuid16_t heart_rate_svc_uuid = BLE_UUID16_INIT(0x180D);
static const ble_uuid16_t heart_rate_chr_uuid = BLE_UUID16_INIT(0x2A37);
static const ble_uuid16_t auto_io_svc_uuid = BLE_UUID16_INIT(0x2020);
static const ble_uuid128_t led_chr_uuid = BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x04, 0x04);
static  ble_uuid16_t auto_door_svc_uuid = BLE_UUID16_INIT(0x2021);
static  ble_uuid128_t door_chr_uuid =
    BLE_UUID128_INIT(0x25, 0xd1, 0xbc, 0xea, 0x5d, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x04, 0x04);

static uint16_t door_chr_val_handle;

static door_info_t door_info_val;
static uint8_t led_rgb_vals[3] = {255, 0, 255};


static volatile bool disc_complete = false;
static volatile bool read_complete = false;
static volatile bool write_complete = false;
static volatile bool enable_encryption = false;

SemaphoreHandle_t door_notify_semaphore;



static TaskHandle_t ble_task_handle;

static const char *tag = "NimBLE_CTS_CENT";
static int ble_cts_cent_gap_event(struct ble_gap_event *event, void *arg);

static void ble_cts_cent_scan(void);
static void ble_poll_task(void *arg);

#if MYNEWT_VAL(BLE_GATTC)

/**
 * Application callback.  Called when the read of the cts current time
 * characteristic has completed.
 */
static int
ble_cts_cent_on_read(uint16_t conn_handle,
                     const struct ble_gatt_error *error,
                     struct ble_gatt_attr *attr,
                     void *arg)
{
    MODLOG_DFLT(INFO, "Read Current time complete; status=%d conn_handle=%d\n",
                error->status, conn_handle);
    if (error->status == 0) {
        MODLOG_DFLT(INFO, "read attr_handle=%d value=\n", attr->handle);
        print_mbuf(attr->om);
    }
    else {
        goto err;
    }
    MODLOG_DFLT(INFO, "\n");
    read_complete = true;
    return 0;
err:
    /* Terminate the connection. */
    return ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
}

/**
 * Application callback.  Called when the read of the cts current time
 * characteristic has completed.
 */
static int
ble_cts_cent_on_write(uint16_t conn_handle,
                     const struct ble_gatt_error *error,
                     struct ble_gatt_attr *attr,
                     void *arg)
{
    if (error->status == 0) {
        MODLOG_DFLT(INFO, "write attr_handle=%d value=\n", attr->handle);
    }
    else {
        goto err;
    }
    MODLOG_DFLT(INFO, "\n");
    write_complete = true;
    return 0;
err:
    /* Terminate the connection. */
    return ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
}

/**
 * Application callback.  Called when the notification is enabled
 */
static int
ble_cts_cent_on_notify(uint16_t conn_handle,
                     const struct ble_gatt_error *error,
                     struct ble_gatt_attr *attr,
                     void *arg)
{
    if (error->status == 0) {
        MODLOG_DFLT(INFO, "write attr_handle=%d value=\n", attr->handle);
    }
    else {
        goto err;
    }
    MODLOG_DFLT(INFO, "\n");
    write_complete = true;

    /* Notifications are enabled */
    xTaskCreate(ble_poll_task,
    "ble_poll",
    4096,
    (void *)(uintptr_t)conn_handle,
    5,
    &ble_task_handle);

    return 0;
err:
    /* Terminate the connection. */
    return ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
}

/**
 *  Performs read on the current time characteristic
 *
 */
static int
ble_cts_cent_read(uint16_t conn_handle)
{

    /* Subscribe to notifications for the Current Time Characteristic.
     * A central enables notifications by writing two bytes (1, 0) to the
     * characteristic's client-characteristic-configuration-descriptor (CCCD).
     */
    const struct peer_chr *chr;
    const struct peer *peer = peer_find(conn_handle);
    int rc;

    chr = peer_chr_find_uuid(peer,
                             (const ble_uuid_t*)&heart_rate_svc_uuid,
                             (const ble_uuid_t*)&heart_rate_chr_uuid);
    if (chr == NULL) {
        MODLOG_DFLT(ERROR, "Error: Peer doesn't support the CTS "
                    " characteristic\n");
        goto err;
    }
    rc = ble_gattc_read(peer->conn_handle, chr->chr.val_handle,
                        ble_cts_cent_on_read, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Error: Failed to read characteristic; rc=%d\n",
                    rc);
        goto err;
    }
    read_complete = false;
    return 0;
err:
    /* Terminate the connection. */
    //return ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        /* Terminate the connection. */
    //return ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    MODLOG_DFLT(ERROR, "Characteristic read failed \n");
    return BLE_HS_ENOTSUP;
}

/**
 *  Performs read on the current time characteristic
 *
 */
static int
ble_cts_cent_write(uint16_t conn_handle)
{

    /* Subscribe to notifications for the Current Time Characteristic.
     * A central enables notifications by writing two bytes (1, 0) to the
     * characteristic's client-characteristic-configuration-descriptor (CCCD).
     */
    const struct peer_chr *chr;
    const struct peer *peer = peer_find(conn_handle);
    int rc;

    chr = peer_chr_find_uuid(peer,
                             (const ble_uuid_t*)&auto_io_svc_uuid,
                             (const ble_uuid_t*)&led_chr_uuid);
    if (chr == NULL) {
        MODLOG_DFLT(ERROR, "Error: Peer doesn't support the CTS "
                    " characteristic\n");
        goto err;
    }
    /* Assign the leds */
    led_rgb_vals[0] -= 5;
    led_rgb_vals[1] += 5;
    led_rgb_vals[2] -= 5;
    struct os_mbuf *om;
    om = ble_hs_mbuf_from_flat(led_rgb_vals, sizeof(led_rgb_vals));
    if (!om) {
    MODLOG_DFLT(ERROR, "Failed to allocate mbuf\n");
    return BLE_HS_ENOMEM;
    }

    rc = ble_gattc_write(peer->conn_handle, chr->chr.val_handle, om,
                        ble_cts_cent_on_write, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Error: Failed to write characteristic; rc=%d\n",
                    rc);
        goto err;
    }
    write_complete = false;
    return 0;
err:
    /* Terminate the connection. */
    //return ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    MODLOG_DFLT(ERROR, "Characteristic write failed \n");
    return BLE_HS_ENOTSUP;
}


/**
 *  Performs read on the current time characteristic
 *
 */
static int
ble_cts_cent_enable_notify(uint16_t conn_handle)
{

    /* Subscribe to notifications for the Current Time Characteristic.
     * A central enables notifications by writing two bytes (1, 0) to the
     * characteristic's client-characteristic-configuration-descriptor (CCCD).
     */
    const struct peer_chr *chr;
    const struct peer_dsc *dsc;
    const struct peer *peer = peer_find(conn_handle);
    int rc;
    door_notify_semaphore = xSemaphoreCreateBinary();
    chr = peer_chr_find_uuid(peer,
                             (const ble_uuid_t*)&auto_door_svc_uuid,
                             (const ble_uuid_t*)&door_chr_uuid);
    if (chr == NULL) {
        MODLOG_DFLT(ERROR, "Error: Peer doesn't support the CTS "
                    " characteristic\n");
        goto err;
    }
    door_chr_val_handle = chr->chr.val_handle;

    dsc = peer_dsc_find_uuid(peer,
                             (const ble_uuid_t*)&auto_door_svc_uuid,
                             (const ble_uuid_t*)&door_chr_uuid,
                             BLE_UUID16_DECLARE(0x2902)); // CCCD);
    if (dsc == NULL) {
        MODLOG_DFLT(ERROR, "Error: Descriptor doesn't support the CTS "
                    " characteristic\n");
        goto err;
    }
    /* Assign the leds */
    uint8_t enable_notify_array[2] = {0x02, 0x00};

    rc = ble_gattc_write_flat(peer->conn_handle, dsc->dsc.handle, enable_notify_array,
                        sizeof(enable_notify_array), ble_cts_cent_on_notify, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Error: Failed to write characteristic; rc=%d\n",
                    rc);
        goto err;
    }
    write_complete = false;
    return 0;
err:
    /* Terminate the connection. */
    //return ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    MODLOG_DFLT(ERROR, "Characteristic write failed \n");
    return BLE_HS_ENOTSUP;
}


/**
 * Called when service discovery of the specified peer has completed.
 */
static void
ble_cts_cent_on_disc_complete(const struct peer *peer, int status, void *arg)
{
    uint16_t conn = (uint16_t)(uintptr_t)arg;
    if (status != 0) {
        /* Service discovery failed.  Terminate the connection. */
        MODLOG_DFLT(ERROR, "Error: Service discovery failed; status=%d "
                    "conn_handle=%d\n", status, peer->conn_handle);
        ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return;
    }
    else
    {
        /* Service discovery has completed successfully.  Now we have a complete
        * list of services, characteristics, and descriptors that the peer
        * supports.
        */
        MODLOG_DFLT(INFO, "Service discovery complete; status=%d "
                "conn_handle=%d\n", conn);
        disc_complete = true;

        /* Enable notifications */
        ble_cts_cent_enable_notify(peer->conn_handle);
    }
}
#endif

/**
 * Starts the discovery procedure
 */
static void start_disc_task(void *arg)
{
    uint16_t conn = (uint16_t)(uintptr_t)arg;
    int rc;

    vTaskDelay(pdMS_TO_TICKS(150)); // 🔥 key fix
        /*** Go for service discovery after encryption has been successfully enabled ***/
        rc = peer_disc_all(conn,
                           ble_cts_cent_on_disc_complete, arg);
        if (rc != 0) {
            MODLOG_DFLT(ERROR, "Failed to discover services; rc=%d\n", rc);
        }

    vTaskDelete(NULL);
}


/**
 * Initiates the GAP general discovery procedure.
 */
static void
ble_cts_cent_scan(void)
{
    uint8_t own_addr_type;
    struct ble_gap_disc_params disc_params = {0};
    int rc;

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Tell the controller to filter duplicates; we don't want to process
     * repeated advertisements from the same device.
     */
    disc_params.filter_duplicates = 1;

    /**
     * Perform a passive scan.  I.e., don't send follow-up scan requests to
     * each advertiser.
     */
    disc_params.passive = 1;

    /* Use defaults for the rest of the parameters. */
    disc_params.itvl = 0;
    disc_params.window = 0;
    disc_params.filter_policy = 0;
    disc_params.limited = 0;

    rc = ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &disc_params,
                      ble_cts_cent_gap_event, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Error initiating GAP discovery procedure; rc=%d\n",
                    rc);
    }
}

/**
 * Indicates whether we should try to connect to the sender of the specified
 * advertisement.  The function returns a positive result if the device
 * advertises connectability and support for the Current Time Service.
 */

#if CONFIG_EXAMPLE_EXTENDED_ADV
static int
ext_ble_cts_cent_should_connect(const struct ble_gap_ext_disc_desc *disc)
{
    int offset = 0;
    int ad_struct_len = 0;
    uint8_t test_addr[6];
    uint32_t peer_addr[6];

    memset(peer_addr, 0x0, sizeof peer_addr);

    if (disc->legacy_event_type != BLE_HCI_ADV_RPT_EVTYPE_ADV_IND &&
            disc->legacy_event_type != BLE_HCI_ADV_RPT_EVTYPE_DIR_IND) {
        return 0;
    }
    if (strlen(CONFIG_EXAMPLE_PEER_ADDR) && (strncmp(CONFIG_EXAMPLE_PEER_ADDR, "ADDR_ANY", strlen("ADDR_ANY")) != 0)) {
        ESP_LOGI(tag, "Peer address from menuconfig: %s", CONFIG_EXAMPLE_PEER_ADDR);

	/* Convert string to address */
        sscanf(CONFIG_EXAMPLE_PEER_ADDR, "%lx:%lx:%lx:%lx:%lx:%lx",
               &peer_addr[5], &peer_addr[4], &peer_addr[3],
               &peer_addr[2], &peer_addr[1], &peer_addr[0]);

        /* Conversion */
        for(int i=0; i<6; i++) {
            test_addr[i] = (uint8_t )peer_addr[i];
        }

        if (memcmp(test_addr, disc->addr.val, sizeof(disc->addr.val)) != 0) {
            return 0;
        }
    }

    /* The device has to advertise support for the CTS
    * service (0x1805).
    */
    do {
        ad_struct_len = disc->data[offset];

        if (!ad_struct_len) {
            break;
        }

        /* Search if cts UUID is advertised */
        if (disc->data[offset + 1] == 0x03) {
            int temp = 2;
            while (temp < ad_struct_len) {
                if(disc->data[offset + temp] == 0x05 &&
                   disc->data[offset + temp + 1] == 0x18) {
                    return 1;
                }
                temp += 2;
            }
        }
        offset += ad_struct_len + 1;
    } while ( offset < disc->length_data );

    return 0;
}
#else

static int
ble_cts_cent_should_connect(const struct ble_gap_disc_desc *disc)
{
    struct ble_hs_adv_fields fields;
    int rc;
    int i;
    uint32_t peer_addr[6];

    memset(peer_addr, 0x0, sizeof peer_addr);

    /* The device has to be advertising connectability. */
    if (disc->event_type != BLE_HCI_ADV_RPT_EVTYPE_ADV_IND &&
            disc->event_type != BLE_HCI_ADV_RPT_EVTYPE_DIR_IND) {

        return 0;
    }

    rc = ble_hs_adv_parse_fields(&fields, disc->data, disc->length_data);
    if (rc != 0) {
        return 0;
    }

    if ((fields.name != NULL) && (fields.name_len == strlen("Spyros_Gatt")) &&
        (memcmp(fields.name, "Spyros_Gatt", fields.name_len) == 0)) {
        ESP_LOGI(tag, "Vrika to server magka");
        /* The device has to advertise support for the Current Time
        * service (0x1805).
        */
        for (i = 0; i < fields.num_uuids16; i++) {
            if (ble_uuid_u16(&fields.uuids16[i].u) == ble_uuid_u16(&auto_io_svc_uuid.u)) {
                return 1;
        }
    }
    }

    return 0;
}
#endif

/**
 * Connects to the sender of the specified advertisement of it looks
 * interesting.  A device is "interesting" if it advertises connectability and
 * support for the Current Time service.
 */
static void
ble_cts_cent_connect_if_interesting(void *disc)
{
    uint8_t own_addr_type;
    int rc;
    ble_addr_t *addr;

    /* Don't do anything if we don't care about this advertiser. */
#if CONFIG_EXAMPLE_EXTENDED_ADV
    if (!ext_ble_cts_cent_should_connect((struct ble_gap_ext_disc_desc *)disc)) {
        return;
    }
#else
    if (!ble_cts_cent_should_connect((struct ble_gap_disc_desc *)disc)) {
        return;
    }
#endif

#if !(MYNEWT_VAL(BLE_HOST_ALLOW_CONNECT_WITH_SCAN))
    /* Scanning must be stopped before a connection can be initiated. */
    rc = ble_gap_disc_cancel();
    if (rc != 0) {
        MODLOG_DFLT(DEBUG, "Failed to cancel scan; rc=%d\n", rc);
        return;
    }
#endif

    /* Figure out address to use for connect (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Try to connect the the advertiser.  Allow 30 seconds (30000 ms) for
     * timeout.
     */
#if CONFIG_EXAMPLE_EXTENDED_ADV
    addr = &((struct ble_gap_ext_disc_desc *)disc)->addr;
#else
    addr = &((struct ble_gap_disc_desc *)disc)->addr;
#endif
    rc = ble_gap_connect(own_addr_type, addr, 30000, NULL,
                         ble_cts_cent_gap_event, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Error: Failed to connect to device; addr_type=%d "
                    "addr=%s; rc=%d\n",
                    addr->type, addr_str(addr->val), rc);
        return;
    }
}

/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that is
 * established.  ble_cts_cent uses the same callback for all connections.
 *
 * @param event                 The event being signalled.
 * @param arg                   Application-specified argument; unused by
 *                                  ble_cts_cent.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
static int
ble_cts_cent_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    struct ble_hs_adv_fields fields;
    int rc;

    switch (event->type) {
    case BLE_GAP_EVENT_DISC:
        rc = ble_hs_adv_parse_fields(&fields, event->disc.data,
                                     event->disc.length_data);
        if (rc != 0) {
            return 0;
        }

        /* An advertisement report was received during GAP discovery. */
        print_adv_fields(&fields);

        /* Try to connect to the advertiser if it looks interesting. */
        ble_cts_cent_connect_if_interesting(&event->disc);
        return 0;

    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed. */
        if (event->connect.status == 0) {
            /* Connection successfully established. */
            MODLOG_DFLT(INFO, "Connection established ");

            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);
            print_conn_desc(&desc);
            MODLOG_DFLT(INFO, "\n");

            /* Remember peer. */
            rc = peer_add(event->connect.conn_handle);
            if (rc != 0) {
                MODLOG_DFLT(ERROR, "Failed to add peer; rc=%d\n", rc);
                return 0;
            }
            
#if CONFIG_EXAMPLE_ENCRYPTION
            /** Initiate security - It will perform
             * Pairing (Exchange keys)
             * Bonding (Store keys)
             * Encryption (Enable encryption)
             * Will invoke event BLE_GAP_EVENT_ENC_CHANGE
             **/
            rc = ble_gap_security_initiate(event->connect.conn_handle);
            if (rc != 0) {
                MODLOG_DFLT(INFO, "Security could not be initiated, rc = %d\n", rc);
                return ble_gap_terminate(event->connect.conn_handle,
                                         BLE_ERR_REM_USER_CONN_TERM);
            } else {
                MODLOG_DFLT(INFO, "Connection secured\n");
            }
#else
#if MYNEWT_VAL(BLE_GATT_CACHING_ASSOC_ENABLE)
            rc =  ble_gattc_cache_assoc(desc.peer_id_addr);
            if (rc != 0) {
                MODLOG_DFLT(ERROR, "Cache Association Failed; rc=%d\n", rc);
                return 0;
            }
#else
#if MYNEWT_VAL(BLE_GATTC)
            /* Perform service discovery */
            rc = peer_disc_all(event->connect.conn_handle,
                               ble_cts_cent_on_disc_complete, NULL);
            if (rc != 0) {
                MODLOG_DFLT(ERROR, "Failed to discover services; rc=%d\n", rc);
                return 0;
            }
#endif
#endif // BLE_GATT_CACHING_ASSOC_ENABLE
#endif
        } else {
            /* Connection attempt failed; resume scanning. */
            MODLOG_DFLT(ERROR, "Error: Connection failed; status=%d\n",
                        event->connect.status);
            ble_cts_cent_scan();
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        /* Connection terminated. */
        vTaskDelete(ble_task_handle);
        enable_encryption = false;
        disc_complete = false;
        MODLOG_DFLT(INFO, "disconnect; reason=%d ", event->disconnect.reason);
        print_conn_desc(&event->disconnect.conn);
        MODLOG_DFLT(INFO, "\n");

        /* Forget about peer. */
        peer_delete(event->disconnect.conn.conn_handle);

        /* Resume scanning. */
        ble_cts_cent_scan();
        return 0;

    case BLE_GAP_EVENT_DISC_COMPLETE:
        MODLOG_DFLT(INFO, "discovery complete; reason=%d\n",
                    event->disc_complete.reason);
        return 0;

    case BLE_GAP_EVENT_ENC_CHANGE:
        /* Encryption has been enabled or disabled for this connection. */
        MODLOG_DFLT(INFO, "encryption change event; status=%d ",
                    event->enc_change.status);
        if (event->enc_change.status == 0) {
        enable_encryption = true;
        }
        rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
        assert(rc == 0);
        print_conn_desc(&desc);
#if CONFIG_EXAMPLE_ENCRYPTION
#if MYNEWT_VAL(BLE_GATT_CACHING_ASSOC_ENABLE)
        rc =  ble_gattc_cache_assoc(desc.peer_id_addr);
        if (rc != 0) {
            MODLOG_DFLT(ERROR, "Cache Association Failed; rc=%d\n", rc);
            return 0;
        }
#else
#if MYNEWT_VAL(BLE_GATTC)
        read_complete = false; /* We start with the read complete */
        write_complete = false; /* Write is disabled */
        xTaskCreate(start_disc_task, "disc", 4096,
                    (void *)(uintptr_t)event->enc_change.conn_handle,
                    5, NULL);
#endif
#endif // BLE_GATT_CACHING_ASSOC_ENABLE
#endif
        return 0;

    case BLE_GAP_EVENT_CACHE_ASSOC:
#if MYNEWT_VAL(BLE_GATT_CACHING_ASSOC_ENABLE)
          /* Cache association result for this connection */
          MODLOG_DFLT(INFO, "cache association; conn_handle=%d status=%d cache_state=%s\n",
                      event->cache_assoc.conn_handle,
                      event->cache_assoc.status,
                      (event->cache_assoc.cache_state == 0) ? "INVALID" : "LOADED");
          /* Perform service discovery */
          rc = peer_disc_all(event->cache_assoc.conn_handle,
                             ble_cts_cent_on_disc_complete, NULL);
          if(rc != 0) {
                MODLOG_DFLT(ERROR, "Failed to discover services; rc=%d\n", rc);
                return 0;
          }
#endif
          return 0;

    case BLE_GAP_EVENT_NOTIFY_RX:
        /* Peer sent us a notification or indication. */
        MODLOG_DFLT(INFO, "received %s; conn_handle=%d attr_handle=%d "
                    "attr_len=%d\n",
                    event->notify_rx.indication ?
                    "indication" :
                    "notification",
                    event->notify_rx.conn_handle,
                    event->notify_rx.attr_handle,
                    OS_MBUF_PKTLEN(event->notify_rx.om));
        if ((event->notify_rx.attr_handle == door_chr_val_handle) && disc_complete)
        {
          os_mbuf_copydata(event->notify_rx.om, 0, sizeof(door_info_t), (void*)&door_info_val);
          xSemaphoreGive(door_notify_semaphore);
        }
        /* Attribute data is contained in event->notify_rx.om. Use
         * `os_mbuf_copydata` to copy the data received in notification mbuf */
        return 0;

    case BLE_GAP_EVENT_MTU:
        MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
                    event->mtu.conn_handle,
                    event->mtu.channel_id,
                    event->mtu.value);
        return 0;

    case BLE_GAP_EVENT_REPEAT_PAIRING:
        /* We already have a bond with the peer, but it is attempting to
         * establish a new secure link.  This app sacrifices security for
         * convenience: just throw away the old bond and accept the new link.
         */
        /* Delete the old bond. */
        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        assert(rc == 0);
        ble_store_util_delete_peer(&desc.peer_id_addr);

        /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
         * continue with the pairing operation.
         */
        return BLE_GAP_REPEAT_PAIRING_RETRY;

    /* Passkey action event */
    case BLE_GAP_EVENT_PASSKEY_ACTION:
        /* Display action */
        MODLOG_DFLT(INFO, "Mpike sto passkey");
        if (event->passkey.params.action == BLE_SM_IOACT_INPUT) {
            /* Generate passkey */
            struct ble_sm_io pkey = {0};
            pkey.action = event->passkey.params.action;
            pkey.passkey = 123456;
            MODLOG_DFLT(INFO, "enter passkey %" PRIu32 " on the client side",
                     pkey.passkey);
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            if (rc != 0) {
                MODLOG_DFLT(INFO,
                         "failed to inject security manager io, error code: %d",
                         rc);
                return rc;
            }
        }
        return 0;

#if CONFIG_EXAMPLE_EXTENDED_ADV
    case BLE_GAP_EVENT_EXT_DISC:
        /* An advertisement report was received during GAP discovery. */
        ext_print_adv_report(&event->ext_disc);

        ble_cts_cent_connect_if_interesting(&event->ext_disc);
        return 0;
#endif

    default:
        return 0;
    }
}

void
ble_cts_cent_on_reset(int reason)
{
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

void
ble_cts_cent_on_sync(void)
{
    int rc;

    /* Make sure we have proper identity address set (public preferred) */
    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    /* Begin scanning for a peripheral to connect to. */
    ble_cts_cent_scan();
}

void ble_cts_cent_host_task(void *param)
{
    ESP_LOGI(tag, "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    disc_complete = false;
    enable_encryption = false;

    nimble_port_run();

    nimble_port_freertos_deinit();
}

static void ble_poll_task(void *arg)
{
    uint16_t conn_handle = (uint16_t)(uintptr_t)arg;
    int rc;
    while (1) {
        // use conn_handle here
        if (disc_complete && enable_encryption)
        {
            if (xSemaphoreTake(door_notify_semaphore, 0) == pdTRUE)
            {
              ESP_LOGI(tag, "Door state is changed %d and it was %d \n", door_info_val.current_state, door_info_val.previous_state);
            }
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}