#define childroom_width 64
#define childroom_height 64
#define floor_width 64
#define floor_height 64
#define heart_width 128
#define heart_height 64
#define holiday_width 64
#define holiday_height 64
#define hysteresis_width 123
#define hysteresis_height 64
#define radiator_width 64
#define radiator_height 64
#define sofa_width 64
#define sofa_height 64
#define thermometer_width 64
#define thermometer_height 64

const char* ssid      = "";
const char* password  = "";
const char* host[2]      = {"http://192.168.178.66/","http://192.168.178.56/"}; //Temperature transmitter
const char* auth = ""; //Bylink auth


static unsigned char childroom_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00,
   0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x80, 0xff, 0x00, 0x00,
   0x00, 0x80, 0xff, 0x00, 0x80, 0xff, 0x00, 0x00, 0x00, 0x80, 0xff, 0x00,
   0x80, 0xff, 0x01, 0x00, 0x00, 0x80, 0xff, 0x01, 0xc0, 0xff, 0x01, 0x00,
   0x00, 0xc0, 0xff, 0x09, 0x80, 0xff, 0x01, 0x00, 0x00, 0x80, 0xff, 0xff,
   0x80, 0xff, 0x00, 0x00, 0x00, 0x80, 0xff, 0x7e, 0x00, 0xff, 0x00, 0x00,
   0x00, 0x80, 0xff, 0x3c, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00,
   0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x80, 0x03, 0x00, 0x00,
   0x00, 0x00, 0xe0, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x03,
   0xe0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x07, 0xf0, 0x0f, 0x00, 0x00,
   0xc0, 0x03, 0xf8, 0x07, 0xf0, 0x0f, 0x00, 0x00, 0xc0, 0x07, 0xfc, 0x07,
   0xf0, 0x0f, 0x00, 0x00, 0xe0, 0x03, 0xfe, 0x07, 0xf0, 0x1f, 0x20, 0x00,
   0xc0, 0x1f, 0xff, 0x0f, 0xf0, 0x7f, 0xe0, 0x00, 0x00, 0xff, 0xff, 0x0f,
   0xf8, 0xff, 0xff, 0x01, 0x00, 0xff, 0xff, 0x0f, 0xf8, 0xff, 0xff, 0x01,
   0x00, 0xfe, 0xff, 0x0f, 0xf8, 0xff, 0xff, 0x00, 0x00, 0xf8, 0xff, 0x0f,
   0xf8, 0xe7, 0xff, 0x00, 0x01, 0xc0, 0xff, 0x0f, 0xf8, 0x07, 0x3c, 0xc0,
   0x03, 0x00, 0xfc, 0x1f, 0xfc, 0x07, 0x00, 0xe0, 0x0f, 0x00, 0xfc, 0x1f,
   0xfc, 0x07, 0x00, 0xf0, 0x0f, 0x00, 0xfe, 0x1f, 0xfc, 0x07, 0x00, 0x00,
   0x00, 0x00, 0xfe, 0x1f, 0xfc, 0x0f, 0x80, 0xff, 0x00, 0x00, 0xfe, 0x1f,
   0xf8, 0x3f, 0x80, 0xff, 0x00, 0x00, 0xfe, 0x3f, 0xf8, 0xff, 0x80, 0xff,
   0x00, 0x00, 0xff, 0x3f, 0xfc, 0xff, 0x01, 0x00, 0x00, 0xc0, 0xff, 0x3f,
   0xfe, 0xff, 0x07, 0x78, 0x3e, 0xe0, 0xff, 0x7f, 0xff, 0xff, 0x07, 0x78,
   0x3e, 0xf0, 0xff, 0x7f, 0xff, 0xff, 0x07, 0x78, 0x3e, 0xf0, 0xff, 0x7f,
   0xff, 0xff, 0x07, 0x78, 0x3e, 0xe0, 0xff, 0x7f, 0xfc, 0xff, 0x03, 0x00,
   0x00, 0xc0, 0xff, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static unsigned char floor_bits[] = {
   0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0xfc, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0x3f, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0x1f, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0xf8, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0,
   0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0,
   0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xcf, 0x00, 0x1c, 0x00,
   0x18, 0x00, 0x38, 0xf0, 0xcf, 0x00, 0x7f, 0x00, 0x7e, 0x00, 0xfe, 0xf0,
   0xcf, 0x80, 0x7f, 0x00, 0xff, 0x00, 0xff, 0xf1, 0xcf, 0x80, 0xe3, 0x80,
   0xe7, 0x00, 0xc7, 0xf1, 0xcf, 0xc0, 0xc1, 0x80, 0xc3, 0x81, 0x83, 0xf1,
   0xcf, 0xc0, 0xc1, 0x80, 0xc1, 0x81, 0x83, 0xf1, 0xcf, 0xc0, 0xc1, 0x81,
   0x81, 0x81, 0x83, 0xf1, 0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1,
   0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81,
   0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1,
   0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81,
   0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1,
   0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81,
   0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1,
   0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81,
   0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1,
   0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81,
   0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1,
   0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81,
   0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1,
   0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81,
   0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1,
   0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81,
   0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1,
   0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81,
   0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1,
   0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81,
   0x81, 0x81, 0x81, 0xf1, 0xcf, 0xc0, 0xc0, 0x81, 0x81, 0x81, 0x81, 0xf1,
   0xcf, 0xe1, 0x80, 0xc1, 0x81, 0x81, 0x81, 0xf1, 0xcf, 0xe1, 0x80, 0xe3,
   0x80, 0xc3, 0x81, 0xf1, 0x8f, 0x7f, 0x80, 0xff, 0x00, 0xff, 0x80, 0xf1,
   0x0f, 0x3f, 0x00, 0x7f, 0x00, 0xfe, 0x80, 0xf1, 0x0f, 0x1e, 0x00, 0x1c,
   0x00, 0x3c, 0x00, 0xf0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0,
   0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0,
   0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0xf8, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c,
   0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xfe, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0x3f, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f,
   0xe0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x03 }; 

static unsigned char heart_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 
  0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xF0, 0x7F, 0x00, 0x00, 0xFC, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xFF, 0x01, 0x00, 0xFE, 0x7F, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0x03, 
  0x80, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xFF, 0xFF, 0x07, 0xC0, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x80, 0xFF, 0xFF, 0x0F, 0xE0, 0xFF, 0xFF, 0x03, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0xFF, 0x1F, 
  0xF0, 0xFF, 0xFF, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xC0, 0xFF, 0xFF, 0x3F, 0xF0, 0xFF, 0xFF, 0x0F, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xE0, 0xFF, 0xFF, 0x3F, 0xF8, 0xFF, 0xFF, 0x0F, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF, 0xFF, 0x7F, 
  0xFC, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xF0, 0xFF, 0xFF, 0x7F, 0xFC, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0x3F, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xFF, 0xFF, 0xFF, 
  0xFE, 0xFF, 0xFF, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xF8, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0x3F, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xE0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xC0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xFF, 0xFF, 
  0xFF, 0xFF, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0xFF, 
  0xFF, 0xFF, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 
  0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0xFC, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xFF, 0xFF, 0x3F, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF, 
  0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0xE0, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xFF, 0xFF, 0x07, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xFF, 
  0xFF, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0xFE, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 
  0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0xF8, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x1F, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 
  0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0xC0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, }; 

static unsigned char holiday_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xf0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x0f, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x03, 0x00, 0x00, 0x00, 0x00, 0xfc,
   0x0f, 0xfe, 0x01, 0x00, 0x00, 0x00, 0x00, 0xff, 0x1f, 0xfe, 0x01, 0x00,
   0x00, 0x00, 0x80, 0xff, 0x3f, 0x3e, 0xfe, 0x01, 0x00, 0x00, 0x00, 0xff,
   0x7f, 0xfe, 0xff, 0x07, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x1f,
   0x00, 0x00, 0x00, 0x80, 0xff, 0xf7, 0xff, 0x3f, 0x00, 0x00, 0x00, 0x00,
   0xfc, 0xfb, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xfd, 0xff, 0xff,
   0x00, 0x00, 0x00, 0x00, 0xff, 0xfc, 0xff, 0x7f, 0x00, 0x00, 0x00, 0xe0,
   0xff, 0x3f, 0xef, 0x3f, 0x00, 0x00, 0x00, 0xf8, 0x7f, 0x1f, 0xc0, 0x01,
   0x00, 0x00, 0x00, 0xfc, 0x7f, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe,
   0x1f, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x07, 0xf8, 0x00, 0x00,
   0x00, 0x00, 0x00, 0xff, 0x07, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
   0x07, 0xf0, 0x01, 0x00, 0x00, 0x00, 0x80, 0xff, 0x00, 0xe0, 0x03, 0x00,
   0x00, 0x00, 0x80, 0xff, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x00, 0x80, 0x7f,
   0x00, 0x80, 0x07, 0x00, 0x00, 0x00, 0xc0, 0x7f, 0x00, 0x80, 0x0f, 0x00,
   0x00, 0x00, 0xc0, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0xc0, 0x07,
   0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x1e, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x78, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x10, 0x7e, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0xff, 0xf0, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x7c, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xfc, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0xf0, 0x01,
   0x00, 0x00, 0x00, 0x00, 0x18, 0xff, 0xf0, 0x01, 0x00, 0x00, 0x00, 0x00,
   0x0c, 0x7f, 0xe0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x8f, 0x7f, 0xe0, 0x01,
   0x00, 0x00, 0x00, 0x80, 0xef, 0x03, 0xe0, 0x01, 0x00, 0x00, 0x00, 0xc0,
   0xff, 0x03, 0xe0, 0x01, 0x00, 0x00, 0x00, 0xe0, 0xff, 0x03, 0xe0, 0x01,
   0x00, 0x00, 0x00, 0xf0, 0xff, 0xc3, 0xe1, 0x03, 0x00, 0x00, 0x00, 0xf8,
   0xff, 0xf3, 0xe1, 0x03, 0x00, 0xe0, 0x0f, 0xf8, 0xff, 0xfb, 0xe1, 0x03,
   0x00, 0xf8, 0xff, 0xf1, 0xff, 0xfd, 0xe0, 0x03, 0x00, 0xfe, 0xff, 0xff,
   0xff, 0x7e, 0xe0, 0x03, 0x00, 0xff, 0xff, 0xff, 0x7f, 0x3f, 0xf0, 0x03,
   0xc0, 0xff, 0xff, 0xff, 0x9f, 0x1f, 0xf0, 0x03, 0xf0, 0x3f, 0xfc, 0xff,
   0xcf, 0x0f, 0xf0, 0x03, 0xf8, 0xef, 0xc0, 0xff, 0xe7, 0x07, 0xf0, 0x03,
   0xfe, 0xfb, 0x0f, 0xfe, 0xf1, 0x03, 0xf0, 0x03, 0xff, 0xfe, 0xff, 0x00,
   0xf8, 0x01, 0xf0, 0x03, 0xbf, 0xff, 0xff, 0x0f, 0xfc, 0x00, 0xf0, 0x03,
   0xef, 0x3f, 0xff, 0xff, 0x7f, 0x00, 0xf8, 0x03, 0xf8, 0x0f, 0xf8, 0xff,
   0x3f, 0x00, 0xf8, 0x01, 0xfe, 0x03, 0x80, 0xff, 0x1f, 0x00, 0xf8, 0x01,
   0xff, 0x00, 0x00, 0xf0, 0x07, 0xe0, 0xff, 0x07, 0x3f, 0x00, 0x00, 0x00,
   0x00, 0xf8, 0xff, 0x1f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0x1f,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x07 }; 

static unsigned char hysteresis_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00,
   0x80, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xc0, 0x00, 0x00, 0xc0, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0xc0, 0x36, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00,
   0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00,
   0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00,
   0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00,
   0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00,
   0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00,
   0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00,
   0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00,
   0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00,
   0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00,
   0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x06, 0x00,
   0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xf8, 0x07, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x03, 0x00, 0x00, 0x06, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x01, 0x00,
   0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00 }; 

static unsigned char radiator_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x01, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x1F, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0xF0, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0x1F, 0x8F, 0xCF, 0xC7, 0xE7, 0xE3, 0xF1, 0xF1, 0x1F, 0x8F, 0xCF, 0xC7, 
  0xE7, 0xE3, 0xF1, 0xF1, 0x1F, 0x8F, 0xCF, 0xC7, 0xE7, 0xE3, 0xF1, 0xF1, 
  0x1F, 0x8F, 0xCF, 0xC7, 0xE7, 0xE3, 0xF1, 0xF1, 0x1F, 0x8F, 0xCF, 0xC7, 
  0xE7, 0xE3, 0xF1, 0xF1, 0x1F, 0x8F, 0xCF, 0xC7, 0xE7, 0xE3, 0xF1, 0xF1, 
  0x1F, 0x8F, 0xCF, 0xC7, 0xE7, 0xE3, 0xF1, 0xF1, 0x1F, 0x8F, 0xCF, 0xC7, 
  0xE7, 0xE3, 0xF1, 0xF1, 0x1F, 0x8F, 0xCF, 0xC7, 0xE7, 0xE3, 0xF1, 0xF1, 
  0x1F, 0x8F, 0xCF, 0xC7, 0xE7, 0xE3, 0xF1, 0xF1, 0x1F, 0x8F, 0xCF, 0xC7, 
  0xE7, 0xE3, 0xF1, 0xF1, 0x1F, 0x8F, 0xCF, 0xC7, 0xE7, 0xE3, 0xF1, 0xF1, 
  0x1F, 0x8F, 0xCF, 0xC7, 0xE7, 0xE3, 0xF1, 0xF1, 0x1F, 0x8F, 0xCF, 0xC7, 
  0xE7, 0xE3, 0xF1, 0xF1, 0x1F, 0x8F, 0xCF, 0xC7, 0xE7, 0xE3, 0xF1, 0xF1, 
  0x1F, 0x8F, 0xCF, 0xC7, 0xE7, 0xE3, 0xF1, 0xF1, 0x1F, 0x8F, 0xCF, 0xC7, 
  0xE7, 0xE3, 0xF1, 0xF1, 0x1F, 0x8F, 0xCF, 0xC7, 0xE7, 0xE3, 0xF1, 0xF1, 
  0x1F, 0x8F, 0xCF, 0xC7, 0xE7, 0xE3, 0xF1, 0xF1, 0x1F, 0x8F, 0xCF, 0xC7, 
  0xE7, 0xE3, 0xF1, 0xF1, 0x1F, 0x8F, 0xCF, 0xC7, 0xE7, 0xE3, 0xF1, 0xF1, 
  0x1F, 0x8F, 0xCF, 0xC7, 0xE7, 0xE3, 0xF1, 0xF1, 0x1F, 0x8F, 0xCF, 0xC7, 
  0xE7, 0xE3, 0xF1, 0xF1, 0x1F, 0x8F, 0xCF, 0xC7, 0xE7, 0xE3, 0xF1, 0xF1, 
  0x1F, 0x8F, 0xCF, 0xC7, 0xE7, 0xE3, 0xF1, 0xF1, 0x1F, 0x8F, 0xCF, 0xC7, 
  0xE7, 0xE3, 0xF1, 0xF1, 0x1F, 0x8F, 0xCF, 0xC7, 0xE7, 0xE3, 0xF1, 0xF1, 
  0x1F, 0x8F, 0xCF, 0xC7, 0xE7, 0xE3, 0xF1, 0xF1, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x1F, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0xF0, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 
  0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0x00, 0x0F, 0x00, 0x00, 0x00, 0xE0, 0xF1, 0x01, 0x00, 0x0F, 0x00, 0x00, 
  0x00, 0xE0, 0xF1, 0x01, 0x00, 0x0F, 0x00, 0x00, 0x00, 0xE0, 0xF1, 0x01, 
  0x00, 0x0F, 0x00, 0x00, 0x00, 0xE0, 0xF1, 0x01, }; 

static unsigned char sofa_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0x3f,
   0xfc, 0xff, 0x1f, 0x00, 0x00, 0xfc, 0xff, 0xff, 0xff, 0xff, 0x3f, 0x00,
   0x00, 0x0e, 0x00, 0xc0, 0x03, 0x00, 0x70, 0x00, 0x00, 0x03, 0x00, 0x80,
   0x01, 0x00, 0xc0, 0x00, 0x80, 0x03, 0x00, 0x80, 0x01, 0x00, 0xc0, 0x00,
   0x80, 0x01, 0x00, 0x80, 0x01, 0x00, 0x80, 0x01, 0x80, 0x01, 0x00, 0x80,
   0x01, 0x00, 0x80, 0x01, 0x80, 0x01, 0x3c, 0x80, 0x01, 0x3c, 0x80, 0x01,
   0x80, 0x01, 0x7e, 0x80, 0x01, 0x7e, 0x80, 0x01, 0x80, 0x01, 0x66, 0x80,
   0x01, 0x66, 0x80, 0x01, 0x80, 0x01, 0x66, 0x80, 0x01, 0x66, 0x80, 0x01,
   0x80, 0x01, 0x7e, 0x80, 0x01, 0x7e, 0x80, 0x01, 0xc0, 0x03, 0x3c, 0x80,
   0x01, 0x3c, 0xc0, 0x03, 0xf0, 0x0f, 0x00, 0x80, 0x01, 0x00, 0xf0, 0x0f,
   0x38, 0x1c, 0x00, 0x80, 0x01, 0x00, 0x38, 0x1c, 0x18, 0x18, 0x00, 0x80,
   0x01, 0x00, 0x18, 0x18, 0x0c, 0x30, 0x00, 0x80, 0x01, 0x00, 0x0c, 0x30,
   0x0c, 0x30, 0x00, 0x80, 0x01, 0x00, 0x0c, 0x30, 0x0c, 0xf0, 0xff, 0xff,
   0xff, 0xff, 0x0f, 0x30, 0x0c, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x30,
   0x18, 0x18, 0x00, 0x80, 0x01, 0x00, 0x18, 0x18, 0x38, 0x1c, 0x00, 0x80,
   0x01, 0x00, 0x38, 0x1c, 0xf0, 0x0f, 0x00, 0x80, 0x01, 0x00, 0xf0, 0x0f,
   0xf0, 0x03, 0x00, 0x80, 0x01, 0x00, 0xc0, 0x0f, 0x30, 0x03, 0x00, 0x80,
   0x01, 0x00, 0xc0, 0x0c, 0x30, 0x03, 0x00, 0x80, 0x01, 0x00, 0xc0, 0x0c,
   0x30, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0c, 0x30, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0x0c, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c,
   0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x30, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x0c, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c,
   0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xf0, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0x0f, 0x80, 0x19, 0x00, 0x00, 0x00, 0x00, 0x98, 0x01,
   0x80, 0x19, 0x00, 0x00, 0x00, 0x00, 0x98, 0x01, 0x80, 0x1f, 0x00, 0x00,
   0x00, 0x00, 0xf8, 0x01, 0x80, 0x1f, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x01,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; 


static unsigned char thermometer_bits[] = {
   0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x01, 0x00, 0x00, 0x00,
   0x00, 0x00, 0xc0, 0xc1, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x80,
   0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x03, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x60, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00,
   0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x07, 0xff, 0x03, 0x00,
   0x00, 0x00, 0x60, 0x00, 0x07, 0xff, 0x03, 0x00, 0x00, 0x00, 0x60, 0x00,
   0x07, 0xff, 0x03, 0x00, 0x00, 0x00, 0x60, 0x00, 0x07, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x60, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00,
   0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x07, 0x3f, 0x00, 0x00,
   0x00, 0x00, 0x60, 0x00, 0x07, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00,
   0x07, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x07, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x60, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00,
   0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x07, 0xff, 0x03, 0x00,
   0x00, 0x00, 0x60, 0x00, 0x07, 0xff, 0x03, 0x00, 0x00, 0x00, 0x60, 0x00,
   0x07, 0xff, 0x03, 0x00, 0x00, 0x00, 0x60, 0x00, 0x07, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x60, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x1c,
   0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x1c, 0x07, 0x3f, 0x00, 0x00,
   0x00, 0x00, 0x60, 0x1c, 0x07, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x60, 0x1c,
   0x07, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x60, 0x1c, 0x07, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x60, 0x1c, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x1c,
   0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x1c, 0x07, 0xff, 0x03, 0x00,
   0x00, 0x00, 0x60, 0x1c, 0x07, 0xff, 0x03, 0x00, 0x00, 0x00, 0x60, 0x1c,
   0x07, 0xff, 0x03, 0x00, 0x00, 0x00, 0x60, 0x1c, 0x07, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x60, 0x1c, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x1c,
   0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x1c, 0x07, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x78, 0x1c, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x1c,
   0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x3c, 0x3c, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x8f, 0xff, 0x78, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc7, 0xf8,
   0xf1, 0x00, 0x00, 0x00, 0x00, 0x80, 0x63, 0xf0, 0xe3, 0x00, 0x00, 0x00,
   0x00, 0x80, 0x33, 0xf0, 0xc7, 0x01, 0x00, 0x00, 0x00, 0xc0, 0x11, 0xfc,
   0xcf, 0x01, 0x00, 0x00, 0x00, 0xc0, 0x19, 0xfe, 0xcf, 0x01, 0x00, 0x00,
   0x00, 0xc0, 0x09, 0xff, 0xcf, 0x01, 0x00, 0x00, 0x00, 0xc0, 0x08, 0xff,
   0x8f, 0x01, 0x00, 0x00, 0x00, 0xc0, 0x08, 0xff, 0x8f, 0x01, 0x00, 0x00,
   0x00, 0xc0, 0x99, 0xff, 0x8f, 0x01, 0x00, 0x00, 0x00, 0xc0, 0xf9, 0xff,
   0xcf, 0x01, 0x00, 0x00, 0x00, 0xc0, 0xf9, 0xff, 0xcf, 0x01, 0x00, 0x00,
   0x00, 0xc0, 0xf1, 0xff, 0xc7, 0x01, 0x00, 0x00, 0x00, 0x80, 0xe3, 0xff,
   0xe7, 0x00, 0x00, 0x00, 0x00, 0x80, 0xe3, 0xff, 0xe3, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x87, 0xff, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x3e,
   0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x3c, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x7c, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff,
   0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0x03, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00 }; 
