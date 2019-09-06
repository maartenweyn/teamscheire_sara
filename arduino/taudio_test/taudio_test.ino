#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// Load Wi-Fi library
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include "i2s.h"

#include "WM8978.h"

#define LEDPIN 22

#define NUM_LEDS 19

#define BRIGHTNESS 50
const int SAMPLE_RATE  = 48000;   

// Replace with your network credentials
const char* ssid     = "ESP32-Access-Point";
const char* password = "123456789";
const int chipSelect = 13;

const byte I2S_NUM = 0; 

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LEDPIN, NEO_GRB + NEO_KHZ800);


// Set web server port number to 80
WiFiServer server(80);

// Auxiliar variables to store the current output state
int led_state[NUM_LEDS] = {0};

#define CCCC(c1, c2, c3, c4)    ((c4 << 24) | (c3 << 16) | (c2 << 8) | c1)

/* these are data structures to process wav file */
typedef enum headerState_e {
    HEADER_RIFF, HEADER_FMT, HEADER_DATA, DATA
} headerState_t;

typedef struct wavRiff_s {
    uint32_t chunkID;
    uint32_t chunkSize;
    uint32_t format;
} wavRiff_t;

typedef struct wavProperties_s {
    uint32_t chunkID;
    uint32_t chunkSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
} wavProperties_t;
/* variables hold file, state of process wav file and wav file properties */    
File root;
headerState_t state = HEADER_RIFF;
wavProperties_t wavProps;

int lVolume = 45;
int rVolume = 45;

i2s_config_t i2s_config = {
     .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
     .sample_rate = SAMPLE_RATE,
     .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
     .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
     .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
     .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // high interrupt priority
     .dma_buf_count = 3,
     .dma_buf_len = 1024,
     .use_apll = false
};
    
//updated I2S pins for tAudio
i2s_pin_config_t pin_config = {
    .bck_io_num = 33, //this is BCK pin
    .ws_io_num = 25, // this is LRCK pin
    .data_out_num = 26, // this is DATA output pin
    .data_in_num = 27   //
};


/* write sample data to I2S */
int i2s_write_sample_nb(uint8_t sample){
  //return i2s_write_bytes((i2s_port_t)i2s_num, (const char *)&sample, sizeof(uint8_t), 100);
}
/* read 4 bytes of data from wav file */
int read4bytes(File file, uint32_t *chunkId){
  int n = file.read((uint8_t *)chunkId, sizeof(uint32_t));
  return n;
}

int readbyte(File file, uint8_t *chunkId){
  int n = file.read((uint8_t *)chunkId, sizeof(uint8_t));
  return n;
}

/* these are function to process wav file */
int readRiff(File file, wavRiff_t *wavRiff){
  int n = file.read((uint8_t *)wavRiff, sizeof(wavRiff_t));
  return n;
}
int readProps(File file, wavProperties_t *wavProps){
  int n = file.read((uint8_t *)wavProps, sizeof(wavProperties_t));
  return n;
}

void setup() {
  Serial.begin(115200);
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code
  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server.begin();

  strip.setPixelColor(0, strip.Color(255,255,255));

  strip.show(); 

  
  SPI.begin(14, 2, 15, 13);

  while(1) {
    Serial.print("\nInitializing SD card...");
  
    // we'll use the initialization code from the utility libraries
    // since we're just testing if the card is working!
    if (!SD.begin(chipSelect)) {
        Serial.println("Card Mount Failed");
        delay(2000);
        continue;
    } else {
        if (!SD.begin(chipSelect)) {
            Serial.println("Card failed, or not present");
            delay(2000);
            continue;
        } else {
          Serial.println("Card card ok");
        }
        break;
    }
  }

  /* open wav file and process it */
  root = SD.open("/001K.WAV");
  if (root) {    
    int c = 0;
    int n;
    while (root.available()) {
      switch(state){
        case HEADER_RIFF:
        wavRiff_t wavRiff;
        n = readRiff(root, &wavRiff);
        if(n == sizeof(wavRiff_t)){
          if(wavRiff.chunkID == CCCC('R', 'I', 'F', 'F') && wavRiff.format == CCCC('W', 'A', 'V', 'E')){
            state = HEADER_FMT;
            Serial.println("HEADER_RIFF");
          }
        }
        break;
        case HEADER_FMT:
        n = readProps(root, &wavProps);
        if(n == sizeof(wavProperties_t)){
          state = HEADER_DATA;
        }
        break;
        case HEADER_DATA:
        uint32_t chunkId, chunkSize;
        n = read4bytes(root, &chunkId);
        if(n == 4){
          if(chunkId == CCCC('d', 'a', 't', 'a')){
            Serial.println("HEADER_DATA");
          }
        }
        n = read4bytes(root, &chunkSize);
        if(n == 4){
          Serial.println("prepare data");
          state = DATA;
        }
        //initialize i2s with configurations above
        i2s_driver_install((i2s_port_t)I2S_NUM, &i2s_config, 0, NULL);
        i2s_set_pin((i2s_port_t)I2S_NUM, NULL);
        //set sample rates of i2s to sample rate of wav file
        i2s_set_sample_rates((i2s_port_t)I2S_NUM, wavProps.sampleRate); 

//        WM8978_Init();
//        WM8978_HPvol_Set(lVolume,rVolume);
//        WM8978_SPKvol_Set(0);
//      
//      
//        WM8978_ADDA_Cfg(1,0);
//        WM8978_Input_Cfg(0,0,0);
//        WM8978_Output_Cfg(1,0);
//      
//        WM8978_I2S_Cfg(2,0);
{

  unsigned char dacen = 1;
  unsigned char adcen = 1;
  unsigned char micen = 1;
  unsigned char lineinen = 0;
  unsigned char auxen = 0;
  unsigned char bpsen = 1;
        unsigned char initVal = WM8978_Init();

        WM8978_ADDA_Cfg(dacen, adcen);
        WM8978_Input_Cfg(micen, lineinen, auxen);
        WM8978_Output_Cfg(dacen, bpsen);
        WM8978_MIC_Gain(50);
        WM8978_SPKvol_Set(30);
        WM8978_HPvol_Set(50,50);
        //WM8978_ALC_Set(1,7,3);
        WM8978_Write_Reg(3,0xFF);
        WM8978_I2S_Cfg(2,0); //format, bit-length
        //WM8978_Write_Reg(7,4<<1);//sets sample rate to 12kHz
        WM8978_Write_Reg(11,0xFF);
        WM8978_Write_Reg(12,0xFF|1<<8);
        
        //uint32_t freq=I2S_Open(I2S, I2S_MODE_MASTER, wavctrl.samplerate, I2S_DATABIT_16, nchannels, I2S_FORMAT_I2S);
        //I2S_EnableMCLK(I2S, 256*wavctrl.samplerate);

}

        break; 
        /* after processing wav file, it is time to process music data */
        case DATA:
        uint8_t data; 
        n = readbyte(root, &data);
        i2s_write_sample_nb(data); 
        break;
      }
    }
    root.close();
  } else {
    Serial.println("error opening test.txt");
  }
  //i2s_driver_uninstall((i2s_port_t)I2S_NUM); //stop & destroy i2s driver 
  Serial.println("done!");
}

void loop() {  
//  WiFiClient client = server.available();   // Listen for incoming clients
//
//  if (client) {                             // If a new client connects,
//    show_menu(client);
//  }


  Serial.println("opening /001K.WAV");
  root = SD.open("/001K.WAV");
  if (root) {    
    int c = 0;
    int n;
    while (root.available()) {
      switch(state){
        case HEADER_RIFF:
        wavRiff_t wavRiff;
        n = readRiff(root, &wavRiff);
        if(n == sizeof(wavRiff_t)){
          if(wavRiff.chunkID == CCCC('R', 'I', 'F', 'F') && wavRiff.format == CCCC('W', 'A', 'V', 'E')){
            state = HEADER_FMT;
            Serial.println("HEADER_RIFF");
          }
        }
        break;
        case HEADER_FMT:
        n = readProps(root, &wavProps);
        if(n == sizeof(wavProperties_t)){
          state = HEADER_DATA;
        }
        break;
        case HEADER_DATA:
        uint32_t chunkId, chunkSize;
        n = read4bytes(root, &chunkId);
        if(n == 4){
          if(chunkId == CCCC('d', 'a', 't', 'a')){
            Serial.println("HEADER_DATA");
          }
        }
        n = read4bytes(root, &chunkSize);
        if(n == 4){
          Serial.println("prepare data");
          state = DATA;
        }
        //initialize i2s with configurations above
        i2s_driver_install((i2s_port_t)I2S_NUM, &i2s_config, 0, NULL);
        i2s_set_pin((i2s_port_t)I2S_NUM, NULL);
        //set sample rates of i2s to sample rate of wav file
        i2s_set_sample_rates((i2s_port_t)I2S_NUM, wavProps.sampleRate); 

        WM8978_Init();
        WM8978_HPvol_Set(lVolume,rVolume);
        WM8978_SPKvol_Set(0);
      
      
        WM8978_ADDA_Cfg(1,0);
        WM8978_Input_Cfg(0,0,0);
        WM8978_Output_Cfg(1,0);
      
        WM8978_I2S_Cfg(2,0);
        
        //uint32_t freq=I2S_Open(I2S, I2S_MODE_MASTER, wavctrl.samplerate, I2S_DATABIT_16, nchannels, I2S_FORMAT_I2S);
        //I2S_EnableMCLK(I2S, 256*wavctrl.samplerate);



        break; 
        /* after processing wav file, it is time to process music data */
        case DATA:
        uint8_t data; 
        n = readbyte(root, &data);
        i2s_write_sample_nb(data); 
        break;
      }
    }
    Serial.println("root.close");
    root.close();
  } else {
    Serial.println("error opening 100k.wav");
  }
  //i2s_driver_uninstall((i2s_port_t)I2S_NUM); //stop & destroy i2s driver

  sleep(1000);

//  int ret;
//  //assign music file and trigger transport dma
//  wav.play("/001K.WAV");
//  do {
//    //We must continue to decode to provide data to the player
//    ret = wav.decode(); 
//    //user code
//  } while(ret == WAV_DECODING);
}

void show_menu(WiFiClient client) {
  // Variable to store the HTTP request
  String header;
  
  Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            int command_index = header.indexOf("/led");
            Serial.print("command_index ");
            Serial.println(command_index);

            if (header.indexOf("GET /led") >= 0) {
              int command_index = header.indexOf("/", 5);
              int led_id = header.substring(8, command_index).toInt();
              int value = header.substring(command_index + 1).toInt();
              Serial.println("Led" + String(led_id) + ": " + String(value));

              strip.setPixelColor(led_id, value);
              led_state[led_id] = value;
              strip.show(); 
            }

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 6px 10px;");
            client.println("text-decoration: none; font-size: 20px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>Sara Web Server</h1>");

            for (int i = 1; i < 5; i++) {
              // Display current state, and ON/OFF buttons
              //client.println("<p>LED" + String(i) + " - State " + led_state[i] + "</p>");
              if (led_state[i]== 0) {
              client.println("<a href=\"/led" + String(i) + "/"+ String(strip.Color(255,0,0)) + "\"><button class=\"button button2\">OFF</button></a>");
              } else {
                client.println("<a href=\"/led" + String(i) + "/0\"><button class=\"button\">ON</button></a>");
              }
            }
               
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
}
