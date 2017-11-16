/*
 LIFX bulb emulator by Kayne Richens (kayno@kayno.net)

 Emulates a LIFX bulb. Connect an RGB LED (or LED strip via drivers)
 to redPin, greenPin and bluePin as you normally would on an
 ethernet-ready Arduino and control it from the LIFX app!

 Notes:
 - Only one client (e.g. app) can connect to the bulb at once

 Set the following variables below to suit your Arduino and network
 environment:
 - mac (unique mac address for your arduino)
 - redPin (PWM pin for RED)
 - greenPin  (PWM pin for GREEN)
 - bluePin  (PWM pin for BLUE)

 Made possible by the work of magicmonkey:
 https://github.com/magicmonkey/lifxjs/ - you can use this to control
 your arduino bulb as well as real LIFX bulbs at the same time!
/*
 LIFX bulb emulator by Kayne Richens (kayno@kayno.net)

 Emulates a LIFX bulb. Connect an RGB LED (or LED strip via drivers)
 to redPin, greenPin and bluePin as you normally would on an
 ethernet-ready Arduino and control it from the LIFX app!

 Notes:
 - Only one client (e.g. app) can connect to the bulb at once

 Set the following variables below to suit your Arduino and network
 environment:
 - mac (unique mac address for your arduino)
 - redPin (PWM pin for RED)
 - greenPin  (PWM pin for GREEN)
 - bluePin  (PWM pin for BLUE)

 Made possible by the work of magicmonkey:
 https://github.com/magicmonkey/lifxjs/ - you can use this to control
 your arduino bulb as well as real LIFX bulbs at the same time!

 And also the RGBMood library by Harold Waterkeyn, which was modified
 slightly to support powering down the LED
 */

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266TrueRandom.h>
#include <time.h>
#include <WiFiUDP.h>
#include <WiFiManager.h>

#include "lifx.h"
#include "color.h"
#include <NeoPixelBus.h>

const uint16_t PixelCount = 60; // this example assumes 4 pixels, making it smaller will cause a failure

// define to output debug messages (including packet dumps) to serial (38400 baud)
#define DEBUG

#ifdef DEBUG
 #define debug_print(x, ...) Serial.print (x, ## __VA_ARGS__)
 #define debug_println(x, ...) Serial.println (x, ## __VA_ARGS__)
#else
 #define debug_print(x, ...)
 #define debug_println(x, ...)
#endif

#define WIFI_SSID "XXSSIDXX"
#define WIFI_PASS "XXPASSXX"

// mac address will be assigned the address fom the wifi adapter
// this address is sent as a part of the protocol
byte mac[] = {
  0xd0, 0x73, 0xd5, 0x2a, 0xbb, 0x8c
};
byte site_mac[] = {
  0x4c, 0x49, 0x46, 0x58, 0x56, 0x32
}; // spells out "LIFXV2" - version 2 of the app changes the site address to this...

// Dim curve
// Used to make 'dimming' look more natural. 
uint8_t dc1[256] = {
    0,   1,   1,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,
    3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   4,   4,   4,   4,
    4,   4,   4,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   6,   6,   6,
    6,   6,   6,   6,   6,   7,   7,   7,   7,   7,   7,   7,   8,   8,   8,   8,
    8,   8,   9,   9,   9,   9,   9,   9,   10,  10,  10,  10,  10,  11,  11,  11,
    11,  11,  12,  12,  12,  12,  12,  13,  13,  13,  13,  14,  14,  14,  14,  15,
    15,  15,  16,  16,  16,  16,  17,  17,  17,  18,  18,  18,  19,  19,  19,  20,
    20,  20,  21,  21,  22,  22,  22,  23,  23,  24,  24,  25,  25,  25,  26,  26,
    27,  27,  28,  28,  29,  29,  30,  30,  31,  32,  32,  33,  33,  34,  35,  35,
    36,  36,  37,  38,  38,  39,  40,  40,  41,  42,  43,  43,  44,  45,  46,  47,
    48,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,
    63,  64,  65,  66,  68,  69,  70,  71,  73,  74,  75,  76,  78,  79,  81,  82,
    83,  85,  86,  88,  90,  91,  93,  94,  96,  98,  99,  101, 103, 105, 107, 109,
    110, 112, 114, 116, 118, 121, 123, 125, 127, 129, 132, 134, 136, 139, 141, 144,
    146, 149, 151, 154, 157, 159, 162, 165, 168, 171, 174, 177, 180, 183, 186, 190,
    193, 196, 200, 203, 207, 211, 214, 218, 222, 226, 230, 234, 238, 242, 248, 255,
};

uint32_t LifxRxBytes = 0;
uint32_t LifxTxBytes = 0;

NeoPixelBus<NeoRgbwFeature, NeoEsp8266Dma800KbpsMethod> led_strip(PixelCount, 3);

const char AP_SSID[] = "Lifx-2aBB8C";

// label (name) for this bulb
char bulbLabel[LifxBulbLabelLength] = "";
LifxLocationData bulbLocation;
LifxGroupData bulbGroup;
LifxUnknownData unknownData;

uint16_t LifxPowerLevel = 0;

// tags for this bulb
char bulbTags[LifxBulbTagsLength] = {
  0, 0, 0, 0, 0, 0, 0, 0
};
char bulbTagLabels[LifxBulbTagLabelsLength] = "";

// initial bulb values - warm white!
long power_status = 65535;
long hue = 0;
long sat = 0;
long bri = 65535;
long kel = 2000;
long dim = 0;

// Ethernet instances, for UDP broadcasting, and TCP server and client
WiFiUDP Udp;
WiFiServer TcpServer(LifxPort);
WiFiClient client;

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

/*
 * Connecting to network succeeded
 * Color = blink green twice
 */
void connectingSuccess() {
  debug_println("connected!");
  Serial.print ("IP address: ");
  Serial.println (WiFi.localIP());

  led_strip.ClearTo(RgbwColor(0, 255, 0, 0));
  led_strip.Show ();
  delay(500);
  led_strip.ClearTo(RgbwColor(0, 0, 0, 0));
  led_strip.Show ();
  delay(500);
  led_strip.ClearTo(RgbwColor(0, 255, 0, 0));
  led_strip.Show ();
  delay(500);
  led_strip.ClearTo(RgbwColor(0, 0, 0, 0));
  led_strip.Show ();
}

void wifiSetup() {

	wifi_set_macaddr(STATION_IF, &mac[0]);

    // Set WIFI module to STA mode
    WiFi.mode(WIFI_STA);

    // Connect
    Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Wait
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    Serial.println();

    // Connected!
    Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

    // Get mac address
    WiFi.macAddress(mac);
}

void setup() {

  Serial.begin(115200);
  debug_println(F("LIFX bulb emulator for Esp8266 starting up..."));

  // WIFI1
#if 0
    // Wifi
    wifiSetup();
#else
  WiFiManager wifiManager;
  wifiManager.setDebugOutput(true);
  wifiManager.setAPCallback(configModeCallback);
  
  if (!wifiManager.autoConnect(AP_SSID)) {
    debug_println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }
#endif

  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("\nWaiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  
  // LEDS
  led_strip.Begin();
  led_strip.ClearTo(RgbwColor(0, 0, 0, 255));
  led_strip.Show();
  debug_println("LEDS initalized");
  
  // Connecting is succeeded
  connectingSuccess();
 
  // set up a UDP and TCP port ready for incoming
  Udp.begin(LifxPort);
  TcpServer.begin();
  EEPROM.begin(256);

  debug_println(F("EEPROM dump:"));
  for (int i = 0; i < 256; i++) {
    debug_print(EEPROM.read(i));
    debug_print(SPACE);
  }
  debug_println();
  
  // read in settings from EEPROM (if they exist) for bulb label and tags
  if (EEPROM.read(EEPROM_CONFIG_START) == EEPROM_CONFIG[0]
      && EEPROM.read(EEPROM_CONFIG_START + 1) == EEPROM_CONFIG[1]
      && EEPROM.read(EEPROM_CONFIG_START + 2) == EEPROM_CONFIG[2]) {
    debug_println(F("Config exists in EEPROM, reading..."));
    debug_print(F("Bulb label: "));

    for (int i = 0; i < LifxBulbLabelLength; i++) {
      bulbLabel[i] = EEPROM.read(EEPROM_BULB_LABEL_START + i);
      debug_print(bulbLabel[i]);
    }

    debug_println();
    debug_print(F("Bulb tags: "));

    for (int i = 0; i < LifxBulbTagsLength; i++) {
      bulbTags[i] = EEPROM.read(EEPROM_BULB_TAGS_START + i);
      debug_print(bulbTags[i]);
    }

    debug_println();
    debug_print(F("Bulb tag labels: "));

    for (int i = 0; i < LifxBulbTagLabelsLength; i++) {
      bulbTagLabels[i] = EEPROM.read(EEPROM_BULB_TAG_LABELS_START + i);
      debug_print(bulbTagLabels[i]);
    }

	for (int i = 0; i < sizeof(LifxLocationData); i++) {
      bulbLocation.lx_raw.data[i] = EEPROM.read(EEPROM_BULB_LOCATION_START + i);
      debug_print(bulbLocation.lx_raw.data[i]);
    }

	for (int i = 0; i < sizeof(LifxGroupData); i++) {
      bulbGroup.lx_raw.data[i] = EEPROM.read(EEPROM_BULB_GROUP_START + i);
      debug_print(bulbGroup.lx_raw.data[i]);
    }
	
    debug_println();
    debug_println(F("Done reading EEPROM config."));
  } else {
    // first time sketch has been run, set defaults into EEPROM
    debug_println(F("Config does not exist in EEPROM, writing..."));

    EEPROM.write(EEPROM_CONFIG_START, EEPROM_CONFIG[0]);
    EEPROM.write(EEPROM_CONFIG_START + 1, EEPROM_CONFIG[1]);
    EEPROM.write(EEPROM_CONFIG_START + 2, EEPROM_CONFIG[2]);

	memset(bulbLabel, 0, sizeof(bulbLabel));
    for (int i = 0; i < LifxBulbLabelLength; i++) {
      EEPROM.write(EEPROM_BULB_LABEL_START + i, bulbLabel[i]);
    }

    for (int i = 0; i < LifxBulbTagsLength; i++) {
      EEPROM.write(EEPROM_BULB_TAGS_START + i, bulbTags[i]);
    }

    for (int i = 0; i < LifxBulbTagLabelsLength; i++) {
      EEPROM.write(EEPROM_BULB_TAG_LABELS_START + i, bulbTagLabels[i]);
    }
    
	//ESP8266TrueRandom.uuid(bulbLocation.lx_location.location);
	memset(bulbLocation.lx_location.location, 0, 16);
	memset(bulbLocation.lx_location.label, 0, sizeof(bulbLocation.lx_location.label));
	for (int i = 0; i < sizeof(LifxLocationData); i++) {
      EEPROM.write(EEPROM_BULB_LOCATION_START + i, bulbLocation.lx_raw.data[i]);
    }

	//ESP8266TrueRandom.uuid(bulbGroup.lx_group.location);
	memset(bulbGroup.lx_group.group, 0, 16);
	memset(bulbGroup.lx_group.label, 0, sizeof(bulbGroup.lx_group.label));
	for (int i = 0; i < sizeof(LifxGroupData); i++) {
      EEPROM.write(EEPROM_BULB_GROUP_START + i, bulbGroup.lx_raw.data[i]);
    }

	memset(unknownData.lx_raw.data, 0, sizeof(LifxUnknownData));
    debug_println(F("Done writing EEPROM config."));
  }
  EEPROM.commit();
  debug_println(F("EEPROM dump:"));
  for (int i = 0; i < 256; i++) {
    debug_print(EEPROM.read(i));
    debug_print(SPACE);
  }
  debug_println();

  // set the bulb based on the initial colors
  setLight();
}

void loop() {
  if (1)
  {
    // buffers for receiving and sending data
    byte PacketBuffer[128]; //buffer to hold incoming packet,

    client = TcpServer.available();
    if (client == true) {
      // read incoming data
      int packetSize = 0;
      while (client.available()) {
        byte b = client.read();
        PacketBuffer[packetSize] = b;
        packetSize++;
      }

      debug_print(F("-TCP "));
      for (int i = 0; i < LifxHeaderSize; i++) {
        debug_print(PacketBuffer[i], HEX);
        debug_print(SPACE);
      }

      for (int i = LifxHeaderSize; i < packetSize; i++) {
        debug_print(PacketBuffer[i], HEX);
        debug_print(SPACE);
      }
      debug_println();

      // push the data into the LifxPacket structure
      LifxPacket request;
      processRequest(PacketBuffer, packetSize, request);

      //respond to the request
      handleRequest(request);
    }

    // if there's UDP data available, read a packet
    int packetSize = Udp.parsePacket();
    if (packetSize) {
      Udp.read(PacketBuffer, 128);

      debug_print(F("-UDP "));
      for (int i = 0; i < LifxHeaderSize; i++) {
        debug_print(PacketBuffer[i], HEX);
        debug_print(SPACE);
      }

      for (int i = LifxHeaderSize; i < packetSize; i++) {
        debug_print(PacketBuffer[i], HEX);
        debug_print(SPACE);
      }
      debug_println();

      // push the data into the LifxPacket structure
      LifxPacket request;
      processRequest(PacketBuffer, sizeof(PacketBuffer), request);

      //respond to the request
      handleRequest(request);

    }
  }

  delay (0);
}

void processRequest(byte *packetBuffer, int packetSize, LifxPacket &request) {

	packetSize = (packetSize < sizeof(request.lx_raw.data)) ? packetSize : sizeof(request.lx_raw.data);
	memcpy(&request.lx_raw.data, packetBuffer, packetSize);
}

void handleRequest(LifxPacket &request) {
	debug_print(F("  Received packet type "));
	debug_println(request.lx_protocol_header.type, DEC);
	if(request.lx_protocol_header.ack_required)
	{
		debug_println("ACK Required");
	}
	LifxRxBytes += request.lx_protocol_header.size;
	
	LifxPacket response;
	memset( response.lx_raw.data, 0, sizeof(response.lx_raw.data));

	response.lx_protocol_header.protocol = 1024;
	response.lx_protocol_header.addressable = 1;
	response.lx_protocol_header.tagged = 0;
	response.lx_protocol_header.origin = 0;
	response.lx_protocol_header.source = request.lx_protocol_header.source;
	memcpy(response.lx_protocol_header.target, mac, 6);
	memcpy(response.lx_protocol_header.site_mac, site_mac, 6);
	response.lx_protocol_header.res_required = 0;
	response.lx_protocol_header.ack_required = 0;
	response.lx_protocol_header.sequence = request.lx_protocol_header.sequence + 1;
	response.lx_protocol_header.timestamp = time(nullptr) & 0xffffffffffffffc0;
	
	LifxPowerData *ppData;
	LifxLabelData *plData;
	LifxLocationData *plocData;
	LifxGroupData *pgrpData;
	LifxLightColorData *pscData;
	LifxLightSetPowerData *plspData;
	LifxUnknownData *puData;
	LifxLightWaveformOptionalData *pwoData;
	
	switch (request.lx_protocol_header.type) {

		case GET_SERVICE:
			response.lx_protocol_header.origin = 1;
			response.lx_protocol_header.type = STATE_SEVICE;
			response.lx_protocol_header.source = 0x05c028e7;
			LifxServiceData ssData;
			// respond with the UDP port
			ssData.lx_service.service = SERVICE_UDP;
			ssData.lx_service.port = LifxPort;
			memcpy(response.lx_protocol_header.data, ssData.lx_raw.data, sizeof(LifxServiceData));
			response.lx_protocol_header.size = LifxHeaderSize + sizeof(LifxServiceData);
			sendPacket(response);

			ssData.lx_service.service = SERVICE_UNKNOWN;
			memcpy(response.lx_protocol_header.data, ssData.lx_raw.data, sizeof(LifxServiceData));
			sendPacket(response);
			break;

		case GET_HOST_INFO:
			response.lx_protocol_header.type = STATE_HOST_INFO;
			
			LifxHostInfoData hiData;
			memset(&hiData, 0, sizeof(hiData));
			hiData.lx_host_info.signal = 1;		// rx signal in mW
			hiData.lx_host_info.tx = LifxTxBytes;
			hiData.lx_host_info.rx = LifxRxBytes;
			memcpy(response.lx_protocol_header.data, hiData.lx_raw.data, sizeof(LifxHostInfoData));
			response.lx_protocol_header.size = LifxHeaderSize + sizeof(LifxHostInfoData);
			sendPacket(response);			
			break;

		case GET_HOST_FIRMWARE:
			response.lx_protocol_header.type = STATE_HOST_FIRMWARE;

			LifxHostFirmwareData hfData;
			memset(&hfData, 0, sizeof(hfData));
			hfData.lx_host_fimware.build1 = 0x149274518f14ea00;		// epoc build time
			hfData.lx_host_fimware.build2 = 0x149274518f14ea00;		// epoc build time
			hfData.lx_host_fimware.version = (LifxFirmwareVersionMajor << 16) + LifxFirmwareVersionMinor;
			memcpy(response.lx_protocol_header.data, hfData.lx_raw.data, sizeof(LifxHostFirmwareData));
			response.lx_protocol_header.size = LifxHeaderSize + sizeof(LifxHostFirmwareData);
			sendPacket(response);			
			break;

		case GET_WIFI_INFO:
			response.lx_protocol_header.type = STATE_WIFI_INFO;
			
			LifxWifiInfoData wiData;
			memset(&wiData, 0, sizeof(wiData));
			wiData.lx_wifi_info.signal = 1;		// rx signal in mW
			wiData.lx_wifi_info.tx = LifxTxBytes;
			wiData.lx_wifi_info.rx = LifxRxBytes;
			memcpy(response.lx_protocol_header.data, wiData.lx_raw.data, sizeof(LifxWifiInfoData));
			response.lx_protocol_header.size = LifxHeaderSize + sizeof(LifxWifiInfoData);
			sendPacket(response);			
			break;

		case GET_WIFI_FIRMWARE:
			response.lx_protocol_header.type = STATE_WIFI_FIRMWARE;

			LifxWifiFirmwareData wfData;
			memset(&wfData, 0, sizeof(wfData));
			wfData.lx_wifi_fimware.build1 = 0x149274518f14ea00;		// epoc build time
			wfData.lx_wifi_fimware.build2 = 0x149274518f14ea00;		// epoc build time
			wfData.lx_wifi_fimware.version = (LifxFirmwareVersionMajor << 16) + LifxFirmwareVersionMinor;
			memcpy(response.lx_protocol_header.data, wfData.lx_raw.data, sizeof(LifxWifiFirmwareData));
			response.lx_protocol_header.size = LifxHeaderSize + sizeof(LifxWifiFirmwareData);
			sendPacket(response);			
			break;

		case SET_POWER:
			ppData = (LifxPowerData *)(&request.lx_protocol_header.data[0]);
			LifxPowerLevel = ppData->lx_power.level;
			EEPROM.commit();
			if(!request.lx_protocol_header.res_required) return;
		case GET_POWER:
			response.lx_protocol_header.type = STATE_POWER;

			LifxPowerData pData;
			memset(&pData, 0, sizeof(pData));
			pData.lx_power.level = LifxPowerLevel;
			memcpy(response.lx_protocol_header.data, pData.lx_raw.data, sizeof(LifxPowerData));
			response.lx_protocol_header.size = LifxHeaderSize + sizeof(LifxPowerData);
			sendPacket(response);			
			break;

		case SET_LABEL:
			plData = (LifxLabelData *)(request.lx_protocol_header.data);
			for (int i = 0; i < LifxBulbLabelLength; i++) {
				if (bulbLabel[i] != plData->lx_label.label[i]) {
					bulbLabel[i] = plData->lx_label.label[i];
					EEPROM.write(EEPROM_BULB_LABEL_START + i, bulbLabel[i]);
				}
			}
			EEPROM.commit();
			if(!request.lx_protocol_header.res_required) return;
		case GET_LABEL:
			response.lx_protocol_header.type = STATE_LABEL;

			LifxLabelData lData;
			memset(&lData, 0, sizeof(lData));
			memcpy(lData.lx_label.label, bulbLabel, LifxBulbLabelLength);
			memcpy(response.lx_protocol_header.data, lData.lx_raw.data, sizeof(LifxLabelData));
			response.lx_protocol_header.size = LifxHeaderSize + sizeof(LifxLabelData);
			sendPacket(response);				
			break;

		case GET_VERSION:
			response.lx_protocol_header.type = STATE_VERSION;

			LifxVersionData vData;
			memset(&vData, 0, sizeof(vData));
			vData.lx_version.vendor = LifxBulbVendor;
			vData.lx_version.product = LifxBulbProduct;
			vData.lx_version.version = LifxBulbVersion;
			memcpy(response.lx_protocol_header.data, vData.lx_raw.data, sizeof(LifxVersionData));
			response.lx_protocol_header.size = LifxHeaderSize + sizeof(LifxVersionData);
			sendPacket(response);			
			break;			

		case GET_INFO:
			response.lx_protocol_header.type = STATE_INFO;

			LifxInfoData iData;
			memset(&iData, 0, sizeof(iData));
			iData.lx_info.time = 0;
			iData.lx_info.uptime = 0;
			iData.lx_info.downtime = 0;
			memcpy(response.lx_protocol_header.data, iData.lx_raw.data, sizeof(LifxInfoData));
			response.lx_protocol_header.size = LifxHeaderSize + sizeof(LifxInfoData);
			sendPacket(response);				
			break;

			//ACKNOWLEDGEMENT = 45;

		case SET_LOCATION:
			plocData = (LifxLocationData *)(request.lx_protocol_header.data);
			
			for (int i = 0; i < sizeof(LifxLocationData); i++) {
				if (bulbLocation.lx_raw.data[i] != plocData->lx_raw.data[i]) {
					bulbLocation.lx_raw.data[i] = plocData->lx_raw.data[i];
					EEPROM.write(EEPROM_BULB_LOCATION_START + i, bulbLocation.lx_raw.data[i]);
				}
			}		
			EEPROM.commit();
			if(!request.lx_protocol_header.res_required) return;
		case GET_LOCATION:
			response.lx_protocol_header.type = STATE_LOCATION;
			
			memcpy(response.lx_protocol_header.data, bulbLocation.lx_raw.data, sizeof(LifxLocationData));
			response.lx_protocol_header.size = LifxHeaderSize + sizeof(LifxLocationData);
			sendPacket(response);				
			break;			

		case SET_GROUP:
			pgrpData = (LifxGroupData *)(request.lx_protocol_header.data);
			
			for (int i = 0; i < sizeof(LifxGroupData); i++) {
				if (bulbGroup.lx_raw.data[i] != pgrpData->lx_raw.data[i]) {
					bulbGroup.lx_raw.data[i] = pgrpData->lx_raw.data[i];
					EEPROM.write(EEPROM_BULB_LOCATION_START + i, bulbGroup.lx_raw.data[i]);
				}
			}
			EEPROM.commit();			
			if(!request.lx_protocol_header.res_required) return;		
		case GET_GROUP:
			response.lx_protocol_header.type = STATE_GROUP;

			memcpy(response.lx_protocol_header.data, bulbGroup.lx_raw.data, sizeof(LifxGroupData));
			response.lx_protocol_header.size = LifxHeaderSize + sizeof(LifxGroupData);
			sendPacket(response);
			break;

		case SET_UNKNOWN:
			puData = (LifxUnknownData *)(request.lx_protocol_header.data);
			memcpy(unknownData.lx_raw.data, puData, sizeof(LifxUnknownData));
			EEPROM.commit();
			if(!request.lx_protocol_header.res_required) return;	
		case GET_UNKNOWN:
			response.lx_protocol_header.type = STATE_UNKNOWN;

			LifxUnknownData uData;
			memcpy(&uData, unknownData.lx_raw.data, sizeof(LifxUnknownData));
			memcpy(response.lx_protocol_header.data, uData.lx_raw.data, sizeof(LifxUnknownData));
			response.lx_protocol_header.size = LifxHeaderSize + sizeof(LifxUnknownData);
			sendPacket(response);				
			break;
			
		case ECHO_REQUEST:
			response.lx_protocol_header.type = ECHO_RESPONSE;

			memcpy(response.lx_protocol_header.data, request.lx_protocol_header.data, sizeof(LifxEchoData));
			response.lx_protocol_header.size = LifxHeaderSize + sizeof(LifxEchoData);
			sendPacket(response);	
			break;

//LIGHT MESSAGES
		case LIGHT_SET_COLOR:
		case LIGHT_SET_WAVEFORM:
		case LIGHT_SET_WAVEFORM_OPTIONAL:			
			switch (request.lx_protocol_header.type) {
				case LIGHT_SET_COLOR:
					pscData = (LifxLightColorData *)(request.lx_protocol_header.data);
					hue = pscData->lx_light_color.color.hue;
					sat = pscData->lx_light_color.color.saturation;
					bri = pscData->lx_light_color.color.brightness;
					kel = pscData->lx_light_color.color.kelvin;
					setLight();				
					break;
				case LIGHT_SET_WAVEFORM:
					break;
				case LIGHT_SET_WAVEFORM_OPTIONAL:	
					pwoData = (LifxLightWaveformOptionalData *)(request.lx_protocol_header.data);
					if(pwoData->lx_light_wavefom_optional.set_hue) hue = pwoData->lx_light_wavefom_optional.color.hue;
					if(pwoData->lx_light_wavefom_optional.set_saturation) sat = pwoData->lx_light_wavefom_optional.color.saturation;
					if(pwoData->lx_light_wavefom_optional.set_brightness) bri = pwoData->lx_light_wavefom_optional.color.brightness;
					if(pwoData->lx_light_wavefom_optional.set_kelvin) kel = pwoData->lx_light_wavefom_optional.color.kelvin;
					setLight();					
					break;
			}
			if(!request.lx_protocol_header.res_required) return;			
		case LIGHT_GET:
			response.lx_protocol_header.type = LIGHT_STATE;
			response.lx_protocol_header.origin = 1;
			LifxLightStateData lsData;
			memset(&lsData, 0, sizeof(lsData));
			lsData.lx_light_state.color.hue = hue;
			lsData.lx_light_state.color.saturation = sat;
			lsData.lx_light_state.color.brightness = bri;
			lsData.lx_light_state.color.kelvin = kel;
			lsData.lx_light_state.power = power_status;
			memcpy(lsData.lx_light_state.label, bulbLabel, LifxBulbLabelLength);
			memcpy(response.lx_protocol_header.data, lsData.lx_raw.data, sizeof(LifxLightStateData));
			response.lx_protocol_header.size = LifxHeaderSize + sizeof(LifxLightStateData);
			sendPacket(response);				
			break;			

		case LIGHT_SET_POWER:
			plspData = (LifxLightSetPowerData *)(request.lx_protocol_header.data);
			power_status = plspData->lx_light_set_power.level;
			setLight();
			EEPROM.commit();
			if(!request.lx_protocol_header.res_required) return;
		case LIGHT_GET_POWER:
			response.lx_protocol_header.type = LIGHT_STATE_POWER;

			LifxLightPowerData lpData;
			memset(&lpData, 0, sizeof(lpData));
			lpData.lx_light_power.level = power_status;
			memcpy(response.lx_protocol_header.data, lpData.lx_raw.data, sizeof(LifxLightPowerData));
			response.lx_protocol_header.size = LifxHeaderSize + sizeof(LifxLightPowerData);
			sendPacket(response);	
			break;

		case LIGHT_SET_IR:
			if(!request.lx_protocol_header.res_required) return;
		case LIGHT_GET_IR:
			//LIGHT_STATE_IR = 118;
			break;

		default:
			debug_print(F("Unknown packet type: "));
			debug_println(request.lx_protocol_header.type, DEC);
			break;
	}
}

void sendPacket(LifxPacket &pkt) {
  sendUDPPacket(pkt);
  LifxTxBytes += pkt.lx_protocol_header.size;

  if (client.connected()) {
    sendTCPPacket(pkt);
  }
}

unsigned int sendUDPPacket(LifxPacket &pkt) {
  // broadcast packet on local subnet
  IPAddress remote_addr(Udp.remoteIP());
  IPAddress broadcast_addr(remote_addr[0], remote_addr[1], remote_addr[2], 255);

  debug_print(F("+UDP "));
  printLifxPacket(pkt);
  debug_println();

  Udp.beginPacket(remote_addr, Udp.remotePort());
  Udp.write(pkt.lx_raw.data, pkt.lx_protocol_header.size);
  Udp.endPacket();

  return pkt.lx_protocol_header.size;
}

unsigned int sendTCPPacket(LifxPacket &pkt) {

  debug_print(F("+TCP "));
  printLifxPacket(pkt);
  debug_println();

  return pkt.lx_protocol_header.size;
}

// print out a LifxPacket data structure as a series of hex bytes - used for DEBUG
void printLifxPacket(LifxPacket &pkt) {
	for (int i = 0; i < pkt.lx_protocol_header.size; i++) {
		debug_print(pkt.lx_raw.data[i], HEX);
		debug_print(SPACE);
	}
}

void setLight() {
  debug_print(F("Set light - "));
  debug_print(F("hue: "));
  debug_print(hue);
  debug_print(F(", sat: "));
  debug_print(sat);
  debug_print(F(", bri: "));
  debug_print(bri);
  debug_print(F(", kel: "));
  debug_print(kel);
  debug_print(F(", power: "));
  debug_print(power_status);
  debug_println(power_status ? " (on)" : "(off)");
  uint8_t white = 0;
  if (power_status) {
    //int this_hue = map(hue, 0, 65535, 0, 767);
	int this_hue = map(hue, 0, 65535, 0, 359);
    int this_sat = map(sat, 0, 65535, 0, 255);
    int this_bri = map(bri, 0, 65535, 0, 255);
    
    // if we are setting a "white" colour (kelvin temp)
    if (kel > 0 && this_sat < 1) {
      // convert kelvin to RGB
      rgb kelvin_rgb;
      kelvin_rgb = kelvinToRGB(kel);

      // convert the RGB into HSV
      hsv kelvin_hsv;
      kelvin_hsv = rgb2hsv(kelvin_rgb);

      // set the new values ready to go to the bulb (brightness does not change, just hue and saturation)
      //this_hue = map(kelvin_hsv.h, 0, 359, 0, 767);
	  this_hue = map(kelvin_hsv.h, 0, 359, 0, 359);
      this_sat = map(kelvin_hsv.s * 1000, 0, 1000, 0, 255); //multiply the sat by 1000 so we can map the percentage value returned by rgb2hsv
	  white =  this_bri;
    }


	uint8_t r2, g2, b2;
    hsb2rgb2(this_hue, this_sat, this_bri, r2, g2, b2);
	  debug_print(F("r: "));
	  debug_print(r2);
	  debug_print(F(", g: "));
	  debug_print(g2);
	  debug_print(F(", b: "));
	  debug_println(b2);;
	
    // LIFXBulb.fadeHSB(this_hue, this_sat, this_bri);
    led_strip.ClearTo(RgbwColor(g2, r2, b2, white));
  }
  else {

    // LIFXBulb.fadeHSB(0, 0, 0);
    led_strip.ClearTo(RgbwColor (0, 0, 0, 0));
  }
  //led_strip.ClearTo(RgbwColor(0, 0, 0, 64));
  led_strip.Show ();
}

/******************************************************************************
 * accepts hue, saturation and brightness values and outputs three 8-bit color
 * values in an array (color[])
 *
 * saturation (sat) and brightness (bright) are 8-bit values.
 *
 * hue (index) is a value between 0 and 767. hue values out of range are
 * rendered as 0.
 *
 *****************************************************************************/
void hsb2rgb(uint16_t index, uint8_t sat, uint8_t bright, uint8_t color[3])
{
	
  uint16_t r_temp, g_temp, b_temp;
  uint8_t index_mod;
  uint8_t inverse_sat = (sat ^ 255);

  index = index % 768;
  index_mod = index % 256;

  if (index < 256)
  {
    r_temp = index_mod ^ 255;
    g_temp = index_mod;
    b_temp = 0;
  }

  else if (index < 512)
  {
    r_temp = 0;
    g_temp = index_mod ^ 255;
    b_temp = index_mod;
  }

  else if ( index < 768)
  {
    r_temp = index_mod;
    g_temp = 0;
    b_temp = index_mod ^ 255;
  }

  else
  {
    r_temp = 0;
    g_temp = 0;
    b_temp = 0;
  }

  r_temp = ((r_temp * sat) / 255) + inverse_sat;
  g_temp = ((g_temp * sat) / 255) + inverse_sat;
  b_temp = ((b_temp * sat) / 255) + inverse_sat;

  r_temp = (r_temp * bright) / 255;
  g_temp = (g_temp * bright) / 255;
  b_temp = (b_temp * bright) / 255;

  color[0]  = (uint8_t)r_temp;
  color[1]  = (uint8_t)g_temp;
  color[2] = (uint8_t)b_temp;
}

void hsb2rgb2(uint16_t hue, uint16_t sat, uint16_t val, uint8_t& red, uint8_t& green, uint8_t& blue) {
  val = dc1[val];
  sat = 255-dc1[255-sat];
  hue = hue % 360;

  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t base;

  if (sat == 0) { // Acromatic color (gray). Hue doesn't mind.
    red   = val;
    green = val;
    blue  = val;
  } else  {
    base = ((255 - sat) * val)>>8;
    switch(hue/60) {
      case 0:
		    r = val;
        g = (((val-base)*hue)/60)+base;
        b = base;
        break;
	
      case 1:
        r = (((val-base)*(60-(hue%60)))/60)+base;
        g = val;
        b = base;
        break;
	
      case 2:
        r = base;
        g = val;
        b = (((val-base)*(hue%60))/60)+base;
        break;

      case 3:
        r = base;
        g = (((val-base)*(60-(hue%60)))/60)+base;
        b = val;
        break;
	
      case 4:
        r = (((val-base)*(hue%60))/60)+base;
        g = base;
        b = val;
        break;
	
      case 5:
        r = val;
        g = base;
        b = (((val-base)*(60-(hue%60)))/60)+base;
        break;
    }  
    red   = r;
    green = g;
    blue  = b; 
  }
}
