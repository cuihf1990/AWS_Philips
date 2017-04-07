/**
 * @file mqtt_config.h
 * @brief MQTT specific configuration file
 */

#ifndef MQTT_CONFIG_H_
#define MQTT_CONFIG_H_

// Get from console
// =================================================
#define MQTT_HOST              "a1lqshc4oegz64.iot.us-west-2.amazonaws.com" ///< Customer specific MQTT HOST. The same will be used for Thing Shadow
#define MQTT_PORT              8883 ///< default port for MQTT/S
#define MQTT_CLIENT_ID         "MiCO" ///< MQTT client ID should be unique for every device
#define MQTT_ROOT_CA_FILENAME       "root-CA.crt" ///< Root CA file name
#define MQTT_CERTIFICATE_FILENAME   "5d8b040ac0-certificate.pem.crt" ///< device signed certificate file name
#define MQTT_PRIVATE_KEY_FILENAME   "5d8b040ac0-private.pem.key" ///< Device private key filename
// =================================================

// MQTT PubSub
#define MQTT_TX_BUF_LEN 512 ///< Any time a message is sent out through the MQTT layer. The message is copied into this buffer anytime a publish is done. This will also be used in the case of Thing Shadow
#define MQTT_RX_BUF_LEN 512 ///< Any message that comes into the device should be less than this buffer size. If a received message is bigger than this buffer size the message will be dropped.
#define MQTT_NUM_SUBSCRIBE_HANDLERS 5 ///< Maximum number of topic filters the MQTT client can handle at any given time. This should be increased appropriately when using Thing Shadow

// Auto Reconnect specific config
#define MQTT_MIN_RECONNECT_WAIT_INTERVAL 1000 ///< Minimum time before the First reconnect attempt is made as part of the exponential back-off algorithm
#define MQTT_MAX_RECONNECT_WAIT_INTERVAL 128000 ///< Maximum time interval after which exponential back-off will stop attempting to reconnect.

// rtos config
#define _ENABLE_THREAD_SUPPORT_

// debug config
#define ENABLE_IOT_DEBUG
//#define ENABLE_IOT_TRACE
#define ENABLE_IOT_INFO
#define ENABLE_IOT_WARN
#define ENABLE_IOT_ERROR

#endif /* SRC_SHADOW_IOT_SHADOW_CONFIG_H_ */
