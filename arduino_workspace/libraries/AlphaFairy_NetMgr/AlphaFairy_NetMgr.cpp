#include "AlphaFairy_NetMgr.h"

#include <WiFi.h>
#include "esp_wifi.h"

#ifndef WIFI_STRING_LEN
#define WIFI_STRING_LEN 30
#endif

typedef struct
{
    uint32_t ip;
    uint8_t  mac[6];
    uint32_t flags;
}
wificli_classifier_t;

static uint8_t wifi_op_mode = WIFIOPMODE_NONE;

static int last_sta_status = WL_IDLE_STATUS;
static char NetMgr_ssid[WIFI_STRING_LEN + 2];
static char NetMgr_password[WIFI_STRING_LEN + 2];

static uint32_t gateway_ip = 0;

static void (*callback)(void) = NULL;
static void (*disconnect_callback)(uint8_t, int) = NULL;

static uint32_t last_sta_reconn_time = 0;

//                                                     0,  1,  2, 3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
const int8_t wifipwr_table[WIFI_PWR_TABLE_MAX + 1] = { 0, -4, -4, 8, 20, 28, 34, 44, 52, 60, 68, 74, 76, 78, 78, };

WiFiUDP ssdp_sock;

void NetMgr_taskAP(void);
void NetMgr_taskSTA(void);
void NetMgr_eventHandler(WiFiEvent_t event, WiFiEventInfo_t info);

int8_t NetMgr_findInTableSta(void* sta);
int8_t NetMgr_insertInTableSta(void* sta);
int8_t NetMgr_findInTableIp(uint32_t ip);
void NetMgr_tableSync(void* lst);

#define WIFICLI_TABLE_SIZE 4
wificli_classifier_t wificli_table[WIFICLI_TABLE_SIZE];

void NetMgr_beginAP(char* ssid, char* password)
{
    if (wifi_op_mode != WIFIOPMODE_NONE)
    {
        esp_wifi_disconnect();
    }

    wifi_op_mode = WIFIOPMODE_AP;
    NetMgr_reset();

    strncpy(NetMgr_ssid    , ssid    , WIFI_STRING_LEN);
    strncpy(NetMgr_password, password, WIFI_STRING_LEN);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
}

void NetMgr_beginSTA(char* ssid, char* password)
{
    if (wifi_op_mode != WIFIOPMODE_NONE)
    {
        esp_wifi_disconnect();
    }

    wifi_op_mode = WIFIOPMODE_STA;
    NetMgr_reset();

    strncpy(NetMgr_ssid    , ssid    , WIFI_STRING_LEN);
    strncpy(NetMgr_password, password, WIFI_STRING_LEN);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    last_sta_reconn_time = millis();
}

void NetMgr_regCallback(void(*cb_evt)(void), void(*cb_disconn)(uint8_t, int))
{
    WiFi.onEvent(NetMgr_eventHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED);
    WiFi.onEvent(NetMgr_eventHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
    WiFi.onEvent(NetMgr_eventHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(NetMgr_eventHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent(NetMgr_eventHandler, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_LOST_IP);
    callback = cb_evt;
    disconnect_callback = cb_disconn;
}

void NetMgr_taskAP()
{
    wifi_sta_list_t          wifi_sta_list;
    // tcpip_adapter_sta_list_t adapter_sta_list;
    memset(&wifi_sta_list   , 0, sizeof(wifi_sta_list   ));
    // memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));
    esp_wifi_ap_get_sta_list  (&wifi_sta_list);
    // tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);

    // NetMgr_tableSync(&adapter_sta_list);
    if (NetMgr_getConnectableClient() != 0)
    {
        // start SSDP listening as soon as possible
        ssdp_sock.beginMulticast(IPAddress(239,255,255,250), 1900);
        ssdp_sock.beginMulticastPacket();
        ssdp_sock.endPacket();
    }
    if (callback != NULL) {
        callback();
    }
}

void NetMgr_taskSTA()
{
    int status = WiFi.status();
    if (status == WL_CONNECTED)
    {
        IPAddress gateway = WiFi.gatewayIP();
        IPAddress localIp = WiFi.localIP();
        if (gateway != 0 && localIp != 0)
        {
            // start SSDP listening as soon as possible
            ssdp_sock.beginMulticast(IPAddress(239,255,255,250), 1900);
            ssdp_sock.beginMulticastPacket();
            ssdp_sock.endPacket();
            if (status != last_sta_status)
            {
                last_sta_status = status;
                gateway_ip = gateway;
                if (callback != NULL) {
                    callback();
                }
            }
        }
    }
    else
    {
        if (status != last_sta_status && last_sta_status == WL_CONNECTED && disconnect_callback != NULL) {
            disconnect_callback(WIFIDISCON_NORMAL, 0);
        }
        last_sta_status = status;
    }
}

void NetMgr_eventHandler(WiFiEvent_t event, WiFiEventInfo_t info)
{
    if (wifi_op_mode == WIFIOPMODE_AP) {
        NetMgr_taskAP();
    }
    else if (wifi_op_mode == WIFIOPMODE_STA) {
        int reason = (int)info.wifi_sta_disconnected.reason;
        #if 0
        if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED && last_sta_status == WL_CONNECTED) {
            WiFi.reconnect();
            last_sta_reconn_time = millis();
        }
        else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED && (millis() - last_sta_reconn_time) > 200) {
            WiFi.disconnect();
            WiFi.begin();
            last_sta_reconn_time = millis();
        }
        #endif
        if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED && (reason == 202 || reason == 203 || reason == 23 || reason == 15)) {
            Serial.printf("STA disconnect auth fail %d\r\n", reason);
            if (disconnect_callback != NULL) {
                disconnect_callback(WIFIDISCON_AUTH_FAIL, reason);
            }
        }
        else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED && (reason < 200 && reason != 3 && reason != 8 && reason != 7)) {
            Serial.printf("STA disconnect error %d\r\n", reason);
            if (disconnect_callback != NULL) {
                disconnect_callback(WIFIDISCON_AUTH_ERROR, reason);
            }
        }
        else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED && (reason == 3 || reason == 8)) {
            Serial.printf("STA disconnect normal %d\r\n", reason);
            if (disconnect_callback != NULL) {
                disconnect_callback(WIFIDISCON_NORMAL, reason);
            }
            WiFi.reconnect();
            last_sta_reconn_time = millis();
        }
        else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED && (reason == 7)) {
            // do nothing
        }
        #if 0
        else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
            if (disconnect_callback != NULL) {
                disconnect_callback(WIFIDISCON_NORMAL, reason);
            }
            WiFi.disconnect();
            WiFi.begin();
            last_sta_reconn_time = millis();
        }
        #endif
        NetMgr_taskSTA();
    }
}

void NetMgr_task()
{
    #if 0
    if (wifi_op_mode == WIFIOPMODE_STA && WiFi.status() != WL_CONNECTED && last_sta_status != WL_CONNECTED)
    {
        uint32_t now = millis();
        if ((now - last_sta_reconn_time) > 1000) {
            WiFi.reconnect();
            last_sta_reconn_time = now;
        }
    }
    #endif
    if (callback != NULL) {
        return;
    }
    if (wifi_op_mode == WIFIOPMODE_AP) {
        NetMgr_taskAP();
    }
    else if (wifi_op_mode == WIFIOPMODE_STA) {
        NetMgr_taskSTA();
    }
}

int8_t NetMgr_findInTableSta(void* sta)
{
    int i;
    for (i = 0; i < WIFICLI_TABLE_SIZE; i++) {
        wificli_classifier_t* t = &(wificli_table[i]);
        // if (memcmp(t->mac, sta->mac, 6) == 0) {
        //     return i;
        // }
    }
    return -1;
}

int8_t NetMgr_insertInTableSta(void* sta)
{
    int8_t i = NetMgr_findInTableSta(sta);
    wificli_classifier_t* t;
    if (i < 0) {
        for (i = 0; i < WIFICLI_TABLE_SIZE; i++) {
            t = &(wificli_table[i]);
            // if (t->ip == 0) {
            //     t->ip = sta->ip.addr;
            //     memcpy(t->mac, sta->mac, 6);
            //     t->flags = 0;
            //     return i;
            // }
        }
    }
    else {
        t = &(wificli_table[i]);
        // t->ip = sta->ip.addr;
        // memcpy(t->mac, sta->mac, 6);
        return i;
    }
    return -1;
}

int8_t NetMgr_findInTableIp(uint32_t ip)
{
    int i;
    for (i = 0; i < WIFICLI_TABLE_SIZE; i++) {
        wificli_classifier_t* t = &(wificli_table[i]);
        if (t->ip == ip) {
            return i;
        }
    }
    return -1;
}

void NetMgr_tableSync(void* lst)
{
    int i, j;
    for (i = 0; i < WIFICLI_TABLE_SIZE; i++) {
        wificli_classifier_t* t = &(wificli_table[i]);
        if (t->ip != 0) {
            bool found = false;
            // for (j = 0; j < lst->num; j++) {
            //     if (lst->sta[j].ip.addr != 0 && memcmp(lst->sta[j].mac, t->mac, 6) == 0) {
            //         t->ip = lst->sta[j].ip.addr;
            //         found = true;
            //     }
            // }
            if (found == false) {
                t->ip = 0;
            }
        }
    }
    // for (j = 0; j < lst->num; j++) {
    //     if (lst->sta[j].ip.addr != 0) {
    //         NetMgr_insertInTableSta(&(lst->sta[j]));
    //     }
    // }
}

uint32_t NetMgr_getConnectableClient(void)
{
    if (wifi_op_mode == WIFIOPMODE_STA) {
        return gateway_ip;
    }

    int i;
    for (i = 0; i < WIFICLI_TABLE_SIZE; i++) {
        wificli_classifier_t* t = &(wificli_table[i]);
        if (t->ip != 0 && t->flags == 0) {
            return t->ip;
        }
    }
    return 0;
}

void NetMgr_markClientCameraPtp(uint32_t ip)
{
    int8_t i = NetMgr_findInTableIp(ip);
    if (i < 0) {
        return;
    }
    wificli_classifier_t* t = &(wificli_table[i]);
    t->flags |= WIFICLIFLAG_IS_CAMPTP;
}

void NetMgr_markClientCameraHttp(uint32_t ip)
{
    int8_t i = NetMgr_findInTableIp(ip);
    if (i < 0) {
        return;
    }
    wificli_classifier_t* t = &(wificli_table[i]);
    t->flags |= WIFICLIFLAG_IS_CAMHTTP;
}

void NetMgr_markClientPhoneHttp(uint32_t ip)
{
    int8_t i = NetMgr_findInTableIp(ip);
    if (i < 0) {
        return;
    }
    wificli_classifier_t* t = &(wificli_table[i]);
    t->flags |= WIFICLIFLAG_IS_PHONEHTTP;
}

void NetMgr_markClientError(uint32_t ip)
{
    int8_t i = NetMgr_findInTableIp(ip);
    if (i < 0) {
        return;
    }
    wificli_classifier_t* t = &(wificli_table[i]);
    t->flags |= WIFICLIFLAG_IS_ERROR;
}

void NetMgr_markClientDisconnect(uint32_t ip)
{
    int8_t i = NetMgr_findInTableIp(ip);
    if (i < 0) {
        return;
    }
    wificli_classifier_t* t = &(wificli_table[i]);
    t->ip = 0;
    t->flags = WIFICLIFLAG_NONE;
}

bool NetMgr_shouldReportError(void)
{
    bool has_cli = false, rpt_err = true;
    int i;
    for (i = 0; i < WIFICLI_TABLE_SIZE; i++) {
        wificli_classifier_t* t = &(wificli_table[i]);
        if (t->ip != 0) {
            has_cli = true;
            if ((t->flags & WIFICLIFLAG_IS_ERROR) == 0 || (t->flags & WIFICLIFLAG_IS_PHONEHTTP) != 0 || (t->flags & WIFICLIFLAG_IS_CAMHTTP) != 0) {
                rpt_err = false;
            }
        }
    }
    if (has_cli == false) {
        return false;
    }
    return rpt_err;
}

bool NetMgr_hasActiveClients(void)
{
    int i;
    for (i = 0; i < WIFICLI_TABLE_SIZE; i++) {
        wificli_classifier_t* t = &(wificli_table[i]);
        if (t->ip != 0 && (t->flags & WIFICLIFLAG_IS_ERROR) == 0) {
            return true;
        }
    }
    return false;
}

uint8_t NetMgr_getOpMode()
{
    return wifi_op_mode;
}

void NetMgr_reset()
{
    last_sta_status = WL_IDLE_STATUS;
    gateway_ip = 0;
    memset((void*)wificli_table, 0, sizeof(wificli_classifier_t) * WIFICLI_TABLE_SIZE);
}

void NetMgr_reboot()
{
    NetMgr_reset();
    esp_wifi_disconnect();
    if (wifi_op_mode == WIFIOPMODE_AP) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(NetMgr_ssid, NetMgr_password);
    }
    else if (wifi_op_mode == WIFIOPMODE_STA) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(NetMgr_ssid, NetMgr_password);
        last_sta_reconn_time = millis();
    }
}

void NetMgr_setWifiPower(wifi_power_t pwr)
{
    while (WiFi.setTxPower(pwr) == false) {
        pwr = (wifi_power_t)(((int)pwr) - 1);
    }
}

bool NetMgr_getRssi(uint32_t ip, int* outres)
{
    int i;
    uint8_t* mac = NULL;
    if (ip == 0) {
        return false;
    }
    if (wifi_op_mode == WIFIOPMODE_AP)
    {
        wifi_sta_list_t          wifi_sta_list;
        // tcpip_adapter_sta_list_t adapter_sta_list;
        memset(&wifi_sta_list   , 0, sizeof(wifi_sta_list   ));
        // memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));
        esp_wifi_ap_get_sta_list  (&wifi_sta_list);
        // tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);
        // for (i = 0; i < adapter_sta_list.num; i++)
        // {
        //     tcpip_adapter_sta_info_t* adapter = &(adapter_sta_list.sta[i]);
        //     if (adapter->ip.addr == ip)
        //     {
        //         mac = adapter->mac;
        //         break;
        //     }
        // }
        for (i == 0; i < wifi_sta_list.num && mac != NULL; i++)
        {
            wifi_sta_info_t* nfo = &(wifi_sta_list.sta[i]);
            if (memcmp(nfo->mac, mac, 6) == 0)
            {
                int rssi32 = nfo->rssi;
                *outres = rssi32;
                return true;
            }
        }
    }
    else if (wifi_op_mode == WIFIOPMODE_STA)
    {
        wifi_ap_record_t aprec;
        if (esp_wifi_sta_get_ap_info(&aprec) == 0) {
            int rssi32 = aprec.rssi;
            *outres = rssi32;
            return true;
        }
    }
    return false;
}

char* NetMgr_getSSID() {
    return NetMgr_ssid;
}

char* NetMgr_getPassword() {
    return NetMgr_password;
}

WiFiUDP* NetMgr_getSsdpSock() {
    // this socket is already listening, pass it to the camera object
    return &ssdp_sock;
}
