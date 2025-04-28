#ifndef HASP_CUSTOM_H
#define HASP_CUSTOM_H
#if defined(HASP_USE_CUSTOM)

#include <ArduinoJson.h>
#include <PacketSerial.h>

// Define the packet types matching those in the RP2040 code
#define PKT_TYPE_SCD41_TEMPERATURE   0xB0
#define PKT_TYPE_SCD41_HUMIDITY      0xB1
#define PKT_TYPE_SCD41_CO2           0xB2
#define PKT_TYPE_AHT20_TEMPERATURE   0xB3
#define PKT_TYPE_AHT20_HUMIDITY      0xB4
#define PKT_TYPE_SGP40_TVOC          0xB5

void custom_setup();
void custom_loop();
void custom_every_second();
void custom_every_5seconds();
bool custom_pin_in_use(uint8_t pin);

// --- NOWE HAKI ---
void custom_state_subtopic(const char* subtopic, const char* payload);
void custom_topic_payload(const char* topic, const char* payload, uint8_t source);
void custom_get_sensors(JsonDocument& doc);

#endif // HASP_USE_CUSTOM
#endif // HASP_CUSTOM_H
