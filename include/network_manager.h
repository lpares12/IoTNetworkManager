/**
* @file
* @author  Lluc Pares
* @date    20/01/2018
* @brief   ESP8266 network manager
*
* API used for managing ESP8266 connectivity. The device will
* go through different states depending on the configuration
* set and the connectivity status.
**/
#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "user_interface.h"

// Enum holding all the possible states
typedef enum {
    NM_STATE_NEW = 0x0,
    NM_STATE_SET = 0x1,
    NM_STATE_ERROR = 0x2,
    NM_STATE_CONNECTED = 0x3
} NetworkManagerState;

// State methods
/**
 * @brief Sets the new device state. Set wifi mode as
 * AP and open the tcp server to listen for new config
 * info.
 */
static void ICACHE_FLASH_ATTR nmSetNewState(void);

/**
 * @brief Sets the error state. Set wifi mode as AP and
 * Station to try and connect to current set network and
 * to allow users setting the new mode.
 */
static void ICACHE_FLASH_ATTR nmSetErrorState(void);
/**
 * @brief Set connected state. Sets the check connection
 * timer with less frequency, since we are already connected.
 */
static void ICACHE_FLASH_ATTR nmSetConnectedState(void);

/**
 * @brief Set Info set state. Sets wifi as only station and
 * tries to given network. Also closes the TCP server.
 */
static void ICACHE_FLASH_ATTR nmSetInfosetState(void);

// Timer methods
/**
 * @brief Checks the connection status for any changes and
 * sets the state according to those changes.
 */
static void ICACHE_FLASH_ATTR nmCheckConnection(void);

// TCP Methods
/**
 * @brief Inits the TCP server and accepts incoming connections.
 */
static void ICACHE_FLASH_ATTR nmTcpInit(void);

/**
 * @brief Closes the TCP server.
 */
static void ICACHE_FLASH_ATTR nmTcpClose(void);

/**
 * @brief Callback for incoming TCP connections.
 * @param arg Contains the espconn struct of the connection.
 */
static void ICACHE_FLASH_ATTR nmTcpServerConnectedCb(void *arg);

/**
 * @brief Callback for the received TCP data
 * @param arg Pointer to the connection espconn struct
 * @param data Array containing the data received
 * @param length Length of the data received
 *
 * This function acts as a callback for data received from the
 * tcp connection. It will add the new data to the buffer data
 * stream called fullData. It will try to parse the recieved buf
 * only if the EOT character was received.
 * Only holds connection if data was received with no EOT char.
 */
static void ICACHE_FLASH_ATTR nmTcpServerRecvCb(void *arg, char *data, unsigned short length);

/**
 * @brief Inits the network manager.
 *
 * Check for entry state, either new device or info set, depending
 * if wifi_station_get_config returns any configuration. 
 */
void ICACHE_FLASH_ATTR nmInit(void);

/**
 * @brief Set the correct config for network manager. Must be called
 * at the user_init method.
 */
void ICACHE_FLASH_ATTR nmConfig(void);

/**
 * @brief Returns the current device network manager state
 */
NetworkManagerState ICACHE_FLASH_ATTR nmGetState(void);


// Utility methods
/**
 * @brief Sets the credentials of ESP8266 wifi
 * @param ssid Pointer to the beggining of the ssid string
 * @param ssidLength Size of the ssid string
 * @param pass Pointer to the beggining of the password string
 * @param passLength Size of the password string
 *
 * Sets the wifi credentials (ssid and password) of the network
 * we want to connect.
 */
static void ICACHE_FLASH_ATTR nmSetWifiCredentials(const char * const ssid, const size_t ssidLength, const char * const pass, const size_t passLength);

/**
 * @brief Parse the received data buffer to find network ssid and pass
 * @param buffer Contains the buffer to parse
 * @param length Contains the length of the buffer str
 * @return 0 if parse successful, 1 otherwise
 *
 * Parses the buffer for network credentials.
 * Expects ssid,pass structure in the buffer and if it's correctly found
 * it will call nmSetWifiCredentials to set the credentials.
 * Returns 0 when parsing was successful, 1 otherwise.
 */
static uint8_t ICACHE_FLASH_ATTR nmParseBuffer(const char * const buffer, const size_t length);

#endif