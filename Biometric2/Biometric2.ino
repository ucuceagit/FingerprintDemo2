/*
 E-Gizmo Fingerprint Scanner Arduino Demo
 Written by: Tiktak 
 December 1, 2016
 
 Board: Gizduino 5 (Atmega 328)
 Connections: FPS -> Gizduino HW Serial
 Serial Debug: RX -> D4, TX->D5
 Required Libraries: Software Serial
*/

#include <SoftwareSerial.h>

/*Debug Port*/
SoftwareSerial DEBUG(4,5);


/*FPS Commands and Response Packets*/
#define FPS_RESPONSE_SIZE     (48)
#define FPS_PACKET_SIZE       (24)
#define COMMAND_PACKET        (0xAA55)
#define RESPONSE_PACKET       (0x55AA)
#define COMMAND_DATA_PACKET   (0xA55A)
#define RESPONSE_DATA_PACKET  (0x5AA5)
 
#define CMD_IDENTIFY      (0x0102)
#define CMD_ENROLL        (0x0103)
#define CMD_ENROLL_ONE    (0x0104)
#define CMD_CLEAR_ADDR    (0x0105)
#define CMD_CLEAR_ALL     (0x0106)
#define CMD_ENROLL_COUNT  (0x0128)
#define CMD_CANCEL        (0x0130)
#define CMD_LED_CONTROL   (0x0124)


#define ERR_SUCCESS        (0x00)
#define ERR_FAIL           (0x01)
#define ERR_VERIFY         (0x11)
#define ERR_IDENTIFY       (0x12)
#define ERR_TMPL_NOT_EMPTY (0x14)
#define ERR_ALL_EMPTY      (0x15)

/*Global Variables*/
uint8_t FPS_RESPONSE_PACKET[FPS_RESPONSE_SIZE];
uint8_t FPSPacket[FPS_PACKET_SIZE] = {0};   
boolean replied = false;
boolean enrolling = false;
uint16_t Checksum = 0;
uint8_t id = 0;

/*Used to print response packet of the FPS*/
//#define PRINT_RESPONSE 



void setup()
{
  Serial.begin(115200);
  DEBUG.begin(9600);
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);
}

void loop(){
  
  /*Commands to Identify, Enroll and Delete via Debug Serial Port*/
  if(DEBUG.available() > 0){
    char rx = DEBUG.read();
    
    if(rx ==  'i'){
      DEBUG.println("Please place finger to identify");
      memset(FPSPacket,0,24);
      sendCommand(COMMAND_PACKET,  CMD_IDENTIFY , 0, FPSPacket);
      getReply();
      printResponse();
      rx = 0;
      
  }else if(rx == 'e'){
      DEBUG.println("Enroll a finger");
      DEBUG.println("Please enter record number and press Enter");
     id = readnumber();
     DEBUG.print("Enrolling ID number: ");
     DEBUG.println(id);
     enrolling = false;
     FPSPacket[0] = id;
     sendCommand(COMMAND_PACKET,  CMD_ENROLL , 2, FPSPacket);
     getReply();     
     printResponse();
     memset(FPSPacket,0,24);
       while(enrolling == false){
          getReply();
          printResponse();

       }
       
  }else if(rx == 'd'){
     DEBUG.println("Delete a Record");
     DEBUG.println("Please enter record number and press Enter");
     id = readnumber();
     FPSPacket[0] = id;
     sendCommand(COMMAND_PACKET,  CMD_CLEAR_ADDR , 2, FPSPacket);
     getReply();
     memset(FPSPacket,0,24);
     DEBUG.println("Record Deleted");

  }else if(rx == 'a'){
    DEBUG.println("Delete all Entries!");
    FPSPacket[0] = 0;
     sendCommand(COMMAND_PACKET,  CMD_CLEAR_ALL , 0, FPSPacket);
     getReply();
     printResponse();
  }
 }
}

void printResponse()
{
  if(FPS_RESPONSE_PACKET[30] == ERR_FAIL && FPS_RESPONSE_PACKET[32] == ERR_IDENTIFY){
    DEBUG.println("You are not Registered");
  }else if(FPS_RESPONSE_PACKET[30] == ERR_SUCCESS && FPS_RESPONSE_PACKET[26] == 0x02 && FPS_RESPONSE_PACKET[27] == 0x01 && FPS_RESPONSE_PACKET[8] != ERR_ALL_EMPTY) { 
      DEBUG.print("Hello User "); DEBUG.println(FPS_RESPONSE_PACKET[32]);
      digitalWrite(13,HIGH); delay(1000); digitalWrite(13,LOW); //Toggle On Board LED when Use is authenticated
  }else if(FPS_RESPONSE_PACKET[8] == 0xF1){
    DEBUG.println("Waiting for Finger");
  }else if(FPS_RESPONSE_PACKET[32] == 0xF2){
    DEBUG.println("Please press the same finger");
  }else if(FPS_RESPONSE_PACKET[32] == 0xF3){
    DEBUG.println("Please press the same finger again");
  }else if(FPS_RESPONSE_PACKET[28] == 0x06){
    DEBUG.print("Finger with id number ");
    DEBUG.print(FPS_RESPONSE_PACKET[32]);DEBUG.println(" has been enrolled");
    enrolling = true;
  }else if(FPS_RESPONSE_PACKET[28] == 0x05){
    DEBUG.print("Successfully Deleted id ");
    DEBUG.println(FPS_RESPONSE_PACKET[8]);
  }else if(FPS_RESPONSE_PACKET[8] == ERR_TMPL_NOT_EMPTY){
    DEBUG.println("Location not Empty!");
    enrolling = true;
  }else if(FPS_RESPONSE_PACKET[8] == ERR_ALL_EMPTY){
     DEBUG.println("No Fingerprints Enrolled");
  }
  
  
}


void getReply()
{
 uint8_t rx;
 uint8_t i=0;
 
 replied = false;
 memset(FPS_RESPONSE_PACKET,0,24);

 
 while(replied == false)
 {
  
   if(Serial.available() >= 24){
     Serial.readBytes((char*) FPS_RESPONSE_PACKET,48);
     replied = true;    
   }
  }

 #ifdef PRINT_RESPONSE
  for(i=0; i<48; i++)
    {
               DEBUG.print(FPS_RESPONSE_PACKET[i],HEX);
               DEBUG.print(" ");
     }
     DEBUG.println();
  #endif 
}

void sendCommand(uint16_t Prefix, uint16_t Command, uint16_t DataSize, uint8_t Data[])
{  
  uint8_t DataBuffer[FPS_PACKET_SIZE] = {0};
  uint8_t i = 0;
 
  DataBuffer[0] = (uint8_t)(Prefix & 0xFF);
  DataBuffer[1] = (uint8_t)((Prefix>>8) & 0xFF);
  DataBuffer[2] = (uint8_t)(Command & 0xFF);
  DataBuffer[3] = (uint8_t)((Command>>8) & 0xFF);
  DataBuffer[4] = (uint8_t)(DataSize & 0xFF);
  DataBuffer[5] = (uint8_t)((DataSize>>8) & 0xFF);
 
  for(i = 6; i <= 21; i++){
    DataBuffer[i] += Data[i-6];
  }
 
  //Compute Checksum
  Checksum = 0;
  for(i = 0; i <= 21; i++){
    Checksum += DataBuffer[i];
  }
 
  DataBuffer[22] = (uint8_t)(Checksum & 0xFF);
  DataBuffer[23] = (uint8_t)((Checksum>>8) & 0xFF);
 

  //Clear the buffer first
  while(Serial.available()){
    Serial.read();
  }
 
  //Send
 Serial.write(DataBuffer, sizeof(DataBuffer));
 //delay(1000); 
}

uint8_t readnumber(void) {
  uint8_t num = 0;
  boolean validnum = false; 
  while (1) {
    while (! DEBUG.available());
    char c = DEBUG.read();
    if (isdigit(c)) {
       num *= 10;
       num += c - '0';
       validnum = true;
    } else if (validnum) {
      return num;
    }
  }
}
