#include "hasplib.h"
#include "my_custom.h"
#include <PacketSerial.h>

// --- Create COBS PacketSerial
typedef COBSPacketSerial PacketSerial_t;
PacketSerial_t sensorPacketSerial;

// Sensor data variables
float aht20_temperature = 0.0;
float aht20_humidity    = 0.0;
float co2_value         = 0.0;
float tvoc_value        = 0.0;

// Last update time
unsigned long lastUpdate = 0;

/* ─────────────────────────  POMOCNICZE  ────────────────────────── */
/* Zamiennik dawnego haspSetText()
 * Obsługuje dwa warianty adresowania:
 *   • "#tag.text"   – gdy podajemy sam tag („temp”)
 *   • "pXbY.text"   – gdy podajemy ścieżkę stronę/id („p0b8”)
 */
static void setTextByTag(const char* tag, const char* txt)
{
    char cmd[64];

    // jeśli pierwszy znak to 'p' i dalej występuje 'b' => traktujemy jako pXbY
    if(tag[0] == 'p' && strchr(tag, 'b')) {
        snprintf(cmd, sizeof(cmd), "%s.text=\"%s\"", tag, txt);      // pXbY
    } else {
        snprintf(cmd, sizeof(cmd), "#%s.text=\"%s\"", tag, txt);     // #tag
    }

    dispatch_text_line(cmd, TAG_CUSTOM);
}

void handle_packet(const uint8_t* buffer, size_t size);

/* ─────────────────────────  SETUP / LOOP  ─────────────────────── */
void custom_setup()
{
    // ESP32-S3: GPIO20 (RX)  ←→  RP2040 GPIO16
    // ESP32-S3: GPIO19 (TX)  ←→  RP2040 GPIO17
    Serial1.begin(115200, SERIAL_8N1, 20, 19);

    sensorPacketSerial.setStream(&Serial1);
    sensorPacketSerial.setPacketHandler([](const uint8_t* buffer, size_t size) {
        // Zrzut surowych bajtów RP2040
        char raw[48] = {0};
        size_t len = size>16?16:size; // tylko pierwsze 16 bajtów
        for(size_t i=0; i<len; i++) {
            snprintf(raw + i*3, sizeof(raw)-i*3, "%02X ", buffer[i]);
        }
        LOG_DEBUG(TAG_CUSTOM, "RP RAW[%u]: %s", size, raw);
        // dalej normalnie
        handle_packet(buffer, size);
    });
    

    LOG_INFO(TAG_CUSTOM, F("SenseCAP Indicator sensor integration initialized"));
}

void custom_loop()
{
    sensorPacketSerial.update();

    // 30 sekund → 30000 ms
    if(millis() - lastUpdate > 30000UL) {
        if(lastUpdate) {
            LOG_WARNING(TAG_CUSTOM, F("No sensor data received for 30+ seconds"));
            lastUpdate = 0;
        }
    }
}

/* ───────────────────  co sekundę  ─────────────────── */
void custom_every_second()
{
    char strVal[16];

    snprintf(strVal, sizeof(strVal), "%.1f°C", aht20_temperature);
    setTextByTag("temp", strVal);

    snprintf(strVal, sizeof(strVal), "%.1f%%", aht20_humidity);
    setTextByTag("humid", strVal);

    snprintf(strVal, sizeof(strVal), "%.0f ppm", co2_value);
    setTextByTag("co2", strVal);

    snprintf(strVal, sizeof(strVal), "%.0f ppb", tvoc_value);
    setTextByTag("tvoc", strVal);

    /* ───── MQTT ───── */
    if(mqttIsConnected()) {
        const char* node = haspDevice.get_hostname();
        char topic[64], payload[16];

        snprintf(payload, sizeof(payload), "%.1f", aht20_temperature);
        snprintf(topic, sizeof(topic), "hasp/%s/state/temperature", node);
        mqttPublish(topic, payload, strlen(payload), true);

        snprintf(payload, sizeof(payload), "%.1f", aht20_humidity);
        snprintf(topic, sizeof(topic), "hasp/%s/state/humidity", node);
        mqttPublish(topic, payload, strlen(payload), true);

        snprintf(payload, sizeof(payload), "%.0f", co2_value);
        snprintf(topic, sizeof(topic), "hasp/%s/state/co2", node);
        mqttPublish(topic, payload, strlen(payload), true);

        snprintf(payload, sizeof(payload), "%.0f", tvoc_value);
        snprintf(topic, sizeof(topic), "hasp/%s/state/tvoc", node);
        mqttPublish(topic, payload, strlen(payload), true);
    }
}

/* ─────────────────  Packet handler  ───────────────── */
void handle_packet(const uint8_t* buffer, size_t size)
{
    if(size != 5) {
        LOG_ERROR(TAG_CUSTOM, F("Invalid packet size: %d"), size);
        return;
    }

    uint8_t type = buffer[0];
    float   value;
    memcpy(&value, &buffer[1], sizeof(float));

    lastUpdate = millis();

    switch(type) {
        case PKT_TYPE_AHT20_TEMPERATURE:  aht20_temperature = value; break;
        case PKT_TYPE_AHT20_HUMIDITY:     aht20_humidity    = value; break;
        case PKT_TYPE_SGP40_TVOC:         tvoc_value        = value; break;
        case PKT_TYPE_SCD41_CO2:          co2_value         = value; break;
        case PKT_TYPE_SCD41_TEMPERATURE:
        case PKT_TYPE_SCD41_HUMIDITY:     break;                      // ignorujemy
        default:
            LOG_WARNING(TAG_CUSTOM, F("Unknown packet type: 0x%02X"), type);
    }
}

/* ---------- haki wymagane przez hasp_dispatch.cpp ---------- */

bool custom_pin_in_use(uint8_t /*pin*/)        { return false; }
void custom_every_5seconds()                   { }
void custom_state_subtopic(const char*, const char*)           { }
void custom_topic_payload(const char*, const char*, uint8_t)   { }
void custom_get_sensors(JsonDocument&)                         { }
