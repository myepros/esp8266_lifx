#pragma pack(push, 1)
typedef union {
	struct {
	  /* frame */
	  uint16_t size;
	  uint16_t protocol:12;
	  uint8_t  addressable:1;
	  uint8_t  tagged:1;
	  uint8_t  origin:2;
	  uint32_t source;
	  /* frame address */
	  uint8_t  target[8];
	  uint8_t  site_mac[6]; 		//reserved[6];
	  uint8_t  res_required:1;
	  uint8_t  ack_required:1;
	  uint8_t  :6;
	  uint8_t  sequence;
	  /* protocol header */
	  uint64_t timestamp;
	  uint16_t type;
	  uint16_t :16;
	  /* variable length payload follows */
	  byte data[128 - 36];
	} lx_protocol_header;
	struct {
		byte data[128];
	} lx_raw;
} LifxPacket;


//DEVICE MESSAGE DATA PACKETS
// Sevice Data
typedef union {
	struct {
	  /* frame */
	  uint8_t service;
	  uint32_t port;
	} lx_service;
	struct {
		byte data[5];
	} lx_raw;
} LifxServiceData;

// HostInfo Data
typedef union {
	struct {
	  /* frame */
	  float signal;
	  uint32_t tx;
	  uint32_t rx;
	  int16_t reserved;
	} lx_host_info;
	struct {
		byte data[14];
	} lx_raw;
} LifxHostInfoData;

// HostFirmware Data
typedef union {
	struct {
	  /* frame */
	  uint64_t build1;
	  uint64_t build2;
	  uint32_t version;
	} lx_host_fimware;
	struct {
		byte data[20];
	} lx_raw;
} LifxHostFirmwareData;

// WifiInfo Data
typedef union {
	struct {
	  /* frame */
	  float signal;
	  uint32_t tx;
	  uint32_t rx;
	  int16_t reserved;
	} lx_wifi_info;
	struct {
		byte data[14];
	} lx_raw;
} LifxWifiInfoData;

// WifiFirmware Data
typedef union {
	struct {
	  /* frame */
	  uint64_t build1;
	  uint64_t build2;
	  uint32_t version;
	} lx_wifi_fimware;
	struct {
		byte data[20];
	} lx_raw;
} LifxWifiFirmwareData;

// Power Data
typedef union {
	struct {
	  /* frame */
	  uint16_t level;
	} lx_power;
	struct {
		byte data[2];
	} lx_raw;
} LifxPowerData;

// Label Data
typedef union {
	struct {
	  /* frame */
	  byte label[32];
	} lx_label;
	struct {
		byte data[32];
	} lx_raw;
} LifxLabelData;

// Version Data
typedef union {
	struct {
	  /* frame */
	  uint32_t vendor;
	  uint32_t product;
	  uint32_t version;
	} lx_version;
	struct {
		byte data[12];
	} lx_raw;
} LifxVersionData;

// Info Data
typedef union {
	struct {
	  /* frame */
	  uint64_t time;
	  uint64_t uptime;
	  uint64_t downtime;
	} lx_info;
	struct {
		byte data[24];
	} lx_raw;
} LifxInfoData;

// Location Data
typedef union {
	struct {
	  /* frame */
	  byte location[16];
	  byte label[32];
	  uint64_t updated_at;
	} lx_location;
	struct {
		byte data[56];
	} lx_raw;
} LifxLocationData;

// Group Data
typedef union {
	struct {
	  /* frame */
	  byte group[16];
	  byte label[32];
	  uint64_t updated_at;
	} lx_group;
	struct {
		byte data[56];
	} lx_raw;
} LifxGroupData;


// Unknown Data
typedef union {
	struct {
	  /* frame */
	  byte r1[16];
	  byte r2[32];
	  uint64_t updated_at;
	} lx_unknown;
	struct {
		byte data[56];
	} lx_raw;
} LifxUnknownData;

// Echo Data
typedef union {
	struct {
	  /* frame */
	  byte group[64];
	} lx_echo;
	struct {
		byte data[64];
	} lx_raw;
} LifxEchoData;

//LIGHT MESSAGE DATA PACKETS
// HSBK data fomat
typedef struct{
	uint16_t hue;
	uint16_t saturation;
	uint16_t brightness;
	uint16_t kelvin;
}HSBK;

// Color Data
typedef union {
	struct {
	  /* frame */
	  uint8_t reserved;
	  HSBK color;	// 16 bytes
	  uint32_t duration;
	} lx_light_color;
	struct {
		byte data[21];
	} lx_raw;
} LifxLightColorData;

// Waveform Data
typedef union {
	struct {
	  /* frame */
	  uint8_t reserved;
	  uint8_t transient;
	  HSBK color;	// 16 bytes
	  uint32_t period;
	  float cycles;
	  int16_t skew_ratio;
	  uint8_t waveform;
	} lx_light_wavefom;
	struct {
		byte data[29];
	} lx_raw;
} LifxLightWaveformData;

// Waveform Optional Data
typedef union {
	struct {
	  /* frame */
	  uint8_t reserved;
	  uint8_t transient;
	  HSBK color;	// 16 bytes
	  uint32_t period;
	  float cycles;
	  int16_t skew_ratio;
	  uint8_t waveform;
	  uint8_t set_hue;
	  uint8_t set_saturation;
	  uint8_t set_brightness;
	  uint8_t set_kelvin;
	} lx_light_wavefom_optional;
	struct {
		byte data[33];
	} lx_raw;
} LifxLightWaveformOptionalData;

// State Data
typedef union {
	struct {
	  /* frame */
	  HSBK color;	// 16 bytes
	  int16_t reserved1;
	  uint16_t power;
	  byte label[32];
	  //uint64_t reserved2;
	} lx_light_state;
	struct {
		byte data[52];
	} lx_raw;
} LifxLightStateData;

// SetPower Data
typedef union {
	struct {
	  /* frame */
	  uint16_t level;
	  uint32_t duration;
	} lx_light_set_power;
	struct {
		byte data[6];
	} lx_raw;
} LifxLightSetPowerData;

// Power Data
typedef union {
	struct {
	  /* frame */
	  uint16_t level;
	} lx_light_power;
	struct {
		byte data[2];
	} lx_raw;
} LifxLightPowerData;

// IR Data
typedef union {
	struct {
	  /* frame */
	  uint16_t brightness;
	} lx_light_ir;
	struct {
		byte data[2];
	} lx_raw;
} LifxLightIRData;

#pragma pack(pop)

enum{
	SAW = 0, SINE, HALF_SINE, TRIANGLE, PULSE
}WAVEFORM;

const unsigned int LifxHeaderSize      = 36;
const unsigned int LifxPort            = 56700;  // local port to listen on
const unsigned int LifxBulbLabelLength = 32;
const unsigned int LifxBulbTagsLength = 8;
const unsigned int LifxBulbTagLabelsLength = 32;

// firmware versions, etc
const unsigned int LifxBulbVendor = 1;
const unsigned int LifxBulbProduct = 0x1b;
const unsigned int LifxBulbVersion = 0;
const unsigned int LifxFirmwareVersionMajor = 2;
const unsigned int LifxFirmwareVersionMinor = 9;

const byte SERVICE_UDP = 0x01;
const byte SERVICE_TCP = 0x02;
const byte SERVICE_UNKNOWN = 0x05;		// from potocol dump

//DEVICE MESSAGES
// packet types
const byte GET_SERVICE = 2;
const byte STATE_SEVICE = 3;

const byte GET_HOST_INFO = 12;
const byte STATE_HOST_INFO = 13;

const byte GET_HOST_FIRMWARE = 14;
const byte STATE_HOST_FIRMWARE = 15;

const byte GET_WIFI_INFO = 16;
const byte STATE_WIFI_INFO = 17;

const byte GET_WIFI_FIRMWARE = 18;
const byte STATE_WIFI_FIRMWARE = 19;

const byte GET_POWER = 20;
const byte SET_POWER = 21;
const byte STATE_POWER = 22;

const byte GET_LABEL = 23;
const byte SET_LABEL = 24;
const byte STATE_LABEL = 25;

const byte GET_VERSION = 32;
const byte STATE_VERSION = 33;

const byte GET_INFO = 34;
const byte STATE_INFO = 35;

const byte ACKNOWLEDGEMENT = 45;

const byte GET_LOCATION = 48;
const byte SET_LOCATION = 49;
const byte STATE_LOCATION = 50;

const byte GET_GROUP = 51;
const byte SET_GROUP = 52;
const byte STATE_GROUP = 53;

const byte GET_UNKNOWN = 54;
const byte SET_UNKNOWN = 55;
const byte STATE_UNKNOWN = 56;

const byte ECHO_REQUEST = 58;
const byte ECHO_RESPONSE = 59;

//LIGHT MESSAGES
const byte LIGHT_GET = 101;

const byte LIGHT_SET_COLOR = 102;
const byte LIGHT_SET_WAVEFORM = 103;
const byte LIGHT_STATE = 107;

const byte LIGHT_GET_POWER = 116;
const byte LIGHT_SET_POWER = 117;
const byte LIGHT_STATE_POWER = 118;

const byte LIGHT_SET_WAVEFORM_OPTIONAL = 119;

const byte LIGHT_GET_IR = 120;
const byte LIGHT_SET_IR = 121;
const byte LIGHT_STATE_IR = 122;

#define EEPROM_BULB_LABEL_START 0 // 32 bytes long
#define EEPROM_BULB_TAGS_START 32 // 8 bytes long
#define EEPROM_BULB_TAG_LABELS_START 40 // 32 bytes long
#define EEPROM_BULB_LOCATION_START 72 //56 bytes long
#define EEPROM_BULB_GROUP_START 128 //56 bytes long
// future data for EEPROM will start at 174...

#define EEPROM_CONFIG "AL1" // 3 byte identifier for this sketch's EEPROM settings
#define EEPROM_CONFIG_START 253 // store EEPROM_CONFIG at the end of EEPROM

// helpers
#define SPACE " "
