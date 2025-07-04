// 2 Channel Transmitter & Trims 
  #include <esp_now.h>
  #include <WiFi.h>
  #include <EEPROM.h> 

  // define the number of bytes you want to access
  #define EEPROM_SIZE 1

  // REPLACE WITH YOUR RECEIVER MAC Address
  uint8_t receiverMacAddress[] = {0xD8,0xBC,0x38,0x79,0xB9,0x7C};  //D8:BC:38:79:B9:7C  
  //uint8_t receiverMacAddress[] = {0x84,0xFC,0xE6,0x08,0xEE,0x70};  //84:FC:E6:08:EE:70  //ESP32-C3 (Hint: good soldering)
  //uint8_t receiverMacAddress[] = {0x84,0xFC,0xE6,0x04,0x0D,0x70};  //84:FC:E6:04:0D:70  //ESP32-C3 (Hint: bad soldering)
    
 #define trimbut_1 23                      // Trim button 1 / Pin 23
 #define trimbut_2 22                      // Trim button 2 / Pin 22 
 
 int tvalue1 = EEPROM.read(0) * 16;        // Reading trim values from Eprom

 int throttle_offset = 0;                  //throttle offset
         
 typedef struct PacketData{
  byte throttle;
  byte roll; 
};
 PacketData data;

 esp_now_peer_info_t peerInfo;

  void ResetData() 
{
  data.throttle = 127;                      // Signal lost position 
  data.roll = 127;
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t ");
  Serial.println(status);
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Message sent" : "Message failed");
}

  void setup()
{
  // Initializing Serial Monitor 
  Serial.begin(115200);
  // initialize EEPROM with predefined size
  EEPROM.begin(EEPROM_SIZE);
  WiFi.mode(WIFI_STA);

  // Initializing ESP-NOW
  if (esp_now_init() != ESP_OK) 
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  else
  {
    Serial.println("Succes: Initialized ESP-NOW");
  }
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, receiverMacAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }
  else
  {
    Serial.println("Succes: Added peer");
  } 
  ResetData();
 
  pinMode(trimbut_1, INPUT_PULLUP); 
  pinMode(trimbut_2, INPUT_PULLUP);
  tvalue1= EEPROM.read(0) * 16;
}
// Joystick center and its borders 
  int Border_Map(int val, int lower, int middle, int upper, bool reverse)
{
  val = constrain(val, lower, upper);
  if ( val < middle )
  val = map(val, lower, middle, 0, 128);
  else
  val = map(val, middle, upper, 128, 255);
  return ( reverse ? 255 - val : val );
}
  void loop()
{
// Trims and Limiting trim values 
  if(digitalRead(trimbut_1)==LOW and tvalue1 < 2520) {
    tvalue1=tvalue1+60;
    EEPROM.write(0,tvalue1/16);
    EEPROM.commit(); 
    delay (130);
  }   
  if(digitalRead(trimbut_2)==LOW and tvalue1 > 1120){
    tvalue1=tvalue1-60;
    EEPROM.write(0,tvalue1/16);
    EEPROM.commit();
    delay (130);
  }
// Throttle offset for center
  if(analogRead(32) > 1775 && analogRead(32) < 1975){
  data.throttle = 128;
  }
  else{
    data.throttle = Border_Map( analogRead(32),0, 1875, 4095, true );   // For Bidirectional ESC  

  }
  
// Control Stick Calibration for channels         
  data.roll = Border_Map( analogRead(33), 0, tvalue1, 4095, false );     // "true" or "false" for signal direction       
  //data.throttle = Border_Map( analogRead(32),2280, 3200, 4095, false );    // For Single side ESC 
  //data.throttle = Border_Map( analogRead(32),0, 1875, 4095, true );   // For Bidirectional ESC  
  
  // Default value of throttle in this range
  if(analogRead(32) > 1600 && analogRead(32) < 2100){
    data.throttle = 129;
  }
  else{
    data.throttle = Border_Map( analogRead(32),0, 1875, 4095, true );   // For Bidirectional ESC  

  }
   
  esp_err_t result = esp_now_send(receiverMacAddress, (uint8_t *) &data, sizeof(data));
  if (result == ESP_OK) 
  {
    Serial.println("Sent with success");
  }
  else 
  {
    Serial.println("Error sending the data");
  }
  Serial.println(EEPROM.read(0));    
  
  delay(50);
}