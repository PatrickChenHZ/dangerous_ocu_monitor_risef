//Replace with your Wifi SSID; example: #define ssid "MyWifi"
#define ssid "hub"

//Replace with your Wifi password; example: #define password "12345678"
#define password "20040317"

//Replace with a human-friendly host name. Must not contain spaces or special characters and be unique on your network
#define hostname "esp32_hub_proto1"

//Replace with your MQTT Broker address; example: #define mqttHost IPAddress(192, 168, 1, 195)
#define mqttHost IPAddress(192, 168, 123, 16)

//Replace with your MQTT Broker port; example: #define mqttPort 1883
#define mqttPort 1883

//Replace with your MQTT Broker user; example: #define mqttUser "homeassistant"
#define mqttUser "admin"

//Replace with your MQTT Broker password; example: #define mqttPassword "12345678"
#define mqttPassword "admin123"

//Replace with the room name where the node will be placed; example: #define room "living-room"
#define room "HUB1"

//Specify the LED pin. For most dev boards, this is GPIO2
#define LED_BUILTIN 2

// Logic level for turning the led on. Most boards use active low, meaning LED_ON should be set to 0
#define LED_ON 0

//Define the base topic for room detection. Usually "room_presence"
#define channel "room_presence"

//Define the topic for publishing availability
#define availabilityTopic "presence_nodes/" room

//Define the topic for publishing JSON attributes
#define telemetryTopic "presence_nodes/" hostname "/tele"

// Define bluetooth scan parameters
#define scanInterval 5 // Define the interval in seconds between scans
#define singleScanTime 5 // Define the duration of a single scan in seconds
#define activeScan true // Active scan uses more power, but get results faster
#define bleScanInterval 0x80 // Used to determine antenna sharing between Bluetooth and WiFi. Do not modify unless you are confident you know what you're doing
#define bleScanWindow 0x10 // Used to determine antenna sharing between Bluetooth and WiFi. Do not modify unless you are confident you know what you're doing

// Maximum distance (in meters) to report. Devices that are calculated to be further than this distance in meters will not be reported
#define maxDistance 2

// MQTT topic for sensor values from HTU21D temperature and humidity sensor
//#define htuSensorTopic "presence_nodes/" hostname "/sensor"


//List of allowed MAC Addresses for MQTT Publish. All others will be ignored.
//Feature is disabled by default.
#define allowedListCheck false
String allowedList[] = {"11223344aabb", "11223344aabb"};
uint32_t allowedListNumberOfItems = 2;
