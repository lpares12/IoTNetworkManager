#include "network_manager.h"

#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"

#define LOCAL_PORT 8089
#define SSID_MAX_LENGTH 32
#define DATA_MAX_SIZE 1024

// Parsing defines. The received data must be in SSID,PASS\x03 format
#define DATA_EOT 0x03
#define DATA_DIVIDER ','

// Variable holding the current state of the network manager
static NetworkManagerState nmState = 0x00;
static os_timer_t nmConnectionTimer; // Timer for checking the connection state continuously

/* Variables for handling the TCP connectivity */
static struct espconn nmEspConn;
static esp_tcp nmEspTcp;
static char fullData[DATA_MAX_SIZE] = {0}; // Data received from remote host
static size_t dataLength = 0; // Length of data received from remote host

static void ICACHE_FLASH_ATTR nmSetWifiCredentials(const char * const ssid, const size_t ssidLength, const char * const pass, const size_t passLength)
{
    struct station_config stationConf;

    os_memset(stationConf.ssid, 0, SSID_MAX_LENGTH);
    os_memset(stationConf.password, 0, 64);
    stationConf.bssid_set = 0; // make sure it isn't looking for an specific MAC Address

    os_memcpy(&stationConf.ssid, ssid, ssidLength);
    os_memcpy(&stationConf.password, pass, passLength);
    wifi_station_set_config(&stationConf); // This saves stationConf to flash, use set_config_current for not saving    
}

static uint8_t ICACHE_FLASH_ATTR nmParseBuffer(const char * const buffer, const size_t length)
{
    size_t divider = 0;
    bool dividerFound = false;
    size_t i = 0;

    for(i = 0; i < length; i++)
    {
        if(buffer[i] == DATA_DIVIDER)
        {
            divider = i;
            dividerFound = true;
            break;
        }
    }

    if(dividerFound)
    {
        nmSetWifiCredentials(buffer, divider, buffer + (divider+1), (length-divider+1));
        nmSetInfosetState();

        return 0;
    }
    else
    {
        os_printf("SSID and pass not received correctly\n");
        return 1;
    }
}

static void ICACHE_FLASH_ATTR nmTcpServerRecvCb(void *arg, char *data, unsigned short length)
{
    struct espconn *ptrespconn = (struct espconn *) arg;

    if( ((length + dataLength) < DATA_MAX_SIZE) && ((length + dataLength) > 0) )
    {
        // Update the data received (could be in multiple packets!)
        os_memcpy(fullData+dataLength, data, length-1); // -1 to avoid copying the '\0' value
        dataLength += (length-1); // to avoid adding the '\0' value

        if(fullData[dataLength-1] == DATA_EOT)
        {
            nmParseBuffer(fullData, dataLength);

            // Update the variables for future connections
            os_memset(fullData, 0, dataLength); // fill non '\0' characters to '\0'
            dataLength = 0;

            // Disconnect this tcp connection
            espconn_disconnect(ptrespconn);
        } // else we will keep reading data from this connection
    }
    else
    {
        os_printf("Too much data received. Closing connection.\n");
        os_memset(fullData, 0, dataLength); // fill non '\0' characters to '\0'
        dataLength = 0;

        espconn_disconnect(ptrespconn);
    }
}

static void ICACHE_FLASH_ATTR nmTcpServerConnectedCb(void *arg)
{
    os_printf("Received a tcp connection!\n");

    struct espconn *ptrespconn = (struct espconn *) arg;

    espconn_regist_recvcb(ptrespconn, nmTcpServerRecvCb);
}

static void ICACHE_FLASH_ATTR nmTcpClose(void)
{
    espconn_delete(&nmEspConn);
}

static void ICACHE_FLASH_ATTR nmTcpInit(void)
{
    nmEspTcp.local_port = LOCAL_PORT;
    nmEspConn.type = ESPCONN_TCP;
    nmEspConn.state = ESPCONN_NONE;
    nmEspConn.proto.tcp = &nmEspTcp;

    espconn_regist_connectcb(&nmEspConn, nmTcpServerConnectedCb);

    // accept connections
    espconn_accept(&nmEspConn);

}

static void ICACHE_FLASH_ATTR nmSetNewState(void)
{
    os_printf("Setting state new\n");

    nmState = NM_STATE_NEW;

    wifi_set_opmode(SOFTAP_MODE);

    // Open TCP server and wait for info
    nmTcpInit();
}

static void ICACHE_FLASH_ATTR nmSetErrorState(void)
{
    os_printf("Setting state error\n");

    nmState = NM_STATE_ERROR;

    // will keep trying to connect in station mode, but will set ap mode to allow users to change config
    wifi_set_opmode(STATIONAP_MODE);

    // Open TCP server and wait for info
    nmTcpInit();    

    // Poll to check connection (in case we lost connection with the router for a second)
    os_timer_disarm(&nmConnectionTimer);
    os_timer_setfn(&nmConnectionTimer, (os_timer_func_t *) nmCheckConnection, NULL);
    os_timer_arm(&nmConnectionTimer, 500, 1); // Now we will poll every 500 ms instead of 100ms
}

static void ICACHE_FLASH_ATTR nmSetConnectedState(void)
{
    os_printf("Setting state connected\n");

    nmState = NM_STATE_CONNECTED;

    if(wifi_get_opmode() == SOFTAP_MODE || wifi_get_opmode() == STATIONAP_MODE)
    {
        wifi_set_opmode(STATION_MODE);
    }

    os_timer_disarm(&nmConnectionTimer);
    os_timer_setfn(&nmConnectionTimer, (os_timer_func_t *) nmCheckConnection, NULL);
    os_timer_arm(&nmConnectionTimer, 1000, 1); // Now we will poll every 1 second instead of 100ms
}

static void ICACHE_FLASH_ATTR nmSetInfosetState(void)
{
    os_printf("Setting state info set\n");

    if(nmState == NM_STATE_ERROR || nmState == NM_STATE_NEW) nmTcpClose();

    nmState = NM_STATE_SET;

    wifi_set_opmode(STATION_MODE);
    wifi_station_connect();

    os_timer_disarm(&nmConnectionTimer);
    os_timer_setfn(&nmConnectionTimer, (os_timer_func_t *) nmCheckConnection, NULL);
    os_timer_arm(&nmConnectionTimer, 100, 1);
}

static void ICACHE_FLASH_ATTR nmCheckConnection(void)
{
    switch(wifi_station_get_connect_status())
    {
        case STATION_IDLE:
            os_printf("Status IDLE: Should not get here\n");
            break;
        case STATION_CONNECTING:
            break;
        case STATION_GOT_IP:
            os_printf("Status GOT IP: Connected\n");
            if(nmState != NM_STATE_CONNECTED) nmSetConnectedState();
            break;
        default:
            os_printf("Status ERROR: Go to net_error mode\n");
            if(nmState != NM_STATE_ERROR) nmSetErrorState();
            break;
    }
}

void ICACHE_FLASH_ATTR nmInit(void)
{
    struct station_config stationConf;

    bool configFound = wifi_station_get_config(&stationConf);

    if(!configFound) nmSetNewState();
    else nmSetInfosetState();
}

void ICACHE_FLASH_ATTR nmConfig(void)
{
    wifi_set_opmode(STATION_MODE);
    wifi_station_set_auto_connect(0);
}

NetworkManagerState ICACHE_FLASH_ATTR nmGetState(void)
{
    return nmState;
}