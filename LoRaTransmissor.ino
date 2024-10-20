#include <SoftwareSerial.h>
#include "LoRa_E220.h"
 
#define LED_BUTTON_PIN 7
#define MOT_BUTTON_PIN 8
 
LoRa_E220 e220ttl(4, 5, 6, 2, 3);
void printParameters(struct Configuration configuration);
void printModuleInformation(struct ModuleInformation moduleInformation);
 
//Contador
int count = 0;
unsigned long previousMillis = 0UL;
unsigned long interval = 1000UL;

//LED / BOTÃO
byte lastButtonState = LOW;
byte ledState = LOW;
unsigned long debounceDuration = 50; // millis
unsigned long lastTimeButtonStateChanged = 0;
String turnOn = "on";
String turnOff = "off";

//MOTOR / BOTÃO
byte lastButtonMotState = LOW;
unsigned long lastTimeButtonMotStateChanged = 0;
String motor = "motor";

//SONIC
const int TRIG_PIN = 9;
const int ECHO_PIN = 10;

void setup() {
  pinMode(LED_BUTTON_PIN, INPUT);
  pinMode(MOT_BUTTON_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(2, LOW);
  digitalWrite(3, LOW);

  Serial.begin(9600);
 
  // Startup all pins and UART
  e220ttl.begin();

  ResponseStructContainer c;
  c = e220ttl.getConfiguration();
  // It's important get configuration pointer before all other operation
  Configuration configuration = *(Configuration*) c.data;
  Serial.println(">> Configuração Atual: ");
  Serial.println(c.status.getResponseDescription());
  Serial.println(c.status.code);

  printParameters(configuration);
  //  ----------------------- DEFAULT TRANSPARENT WITH RSSI -----------------------
  configuration.ADDL = 0x03;
  configuration.ADDH = 0x00;

  configuration.CHAN = 23;
  
  configuration.SPED.uartBaudRate = UART_BPS_9600;
  configuration.SPED.airDataRate = AIR_DATA_RATE_010_24;
  configuration.SPED.uartParity = MODE_00_8N1;
  
  configuration.OPTION.subPacketSetting = SPS_200_00;
  configuration.OPTION.RSSIAmbientNoise = RSSI_AMBIENT_NOISE_ENABLED;
  configuration.OPTION.transmissionPower = POWER_22;
  
  configuration.TRANSMISSION_MODE.enableRSSI = RSSI_ENABLED;
  configuration.TRANSMISSION_MODE.fixedTransmission = FT_TRANSPARENT_TRANSMISSION;
  configuration.TRANSMISSION_MODE.enableLBT = LBT_DISABLED;
  configuration.TRANSMISSION_MODE.WORPeriod = WOR_2000_011;

  configuration.CRYPT.CRYPT_H = 0x01;
	configuration.CRYPT.CRYPT_L = 0x01;

  Serial.println(">> Setando Configuração: ");
  ResponseStatus rs = e220ttl.setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);
  Serial.println(rs.getResponseDescription());
  Serial.println(rs.code);
 
  Serial.println(">> Configuração Após Setar: ");
  c = e220ttl.getConfiguration();
  // It's important get configuration pointer before all other operation
  configuration = *(Configuration*) c.data;
  Serial.println(c.status.getResponseDescription());
  Serial.println(c.status.code);
 
  printParameters(configuration);
 
  c.close();

  Serial.println(">> Iniciando transmissão de dados");
}
 
void loop() {
  unsigned long currentMillis = millis();

  if(currentMillis - previousMillis > interval){
	  pulse();
 	  previousMillis = currentMillis;
  }
  
  //MOTOR ACTION 
  if (millis() - lastTimeButtonStateChanged > debounceDuration) {
      byte buttonState = digitalRead(MOT_BUTTON_PIN);
      if (buttonState != lastButtonMotState) {
        lastTimeButtonMotStateChanged = millis();
        lastButtonMotState = buttonState;
        if (buttonState == LOW) {
            Serial.println("Sending: motor");
            e220ttl.sendMessage(motor);
        }
      }
  }

  //LED ACTION
  if (millis() - lastTimeButtonStateChanged > debounceDuration) {
      byte buttonState = digitalRead(LED_BUTTON_PIN);
      if (buttonState != lastButtonState) {
        lastTimeButtonStateChanged = millis();
        lastButtonState = buttonState;
        if (buttonState == LOW) {
          ledState = (ledState == HIGH) ? LOW: HIGH;
          if(ledState == HIGH){
            Serial.println("Enviando: On");
            e220ttl.sendMessage(turnOn);
          } else {
            Serial.println("Enviando: Off");
            e220ttl.sendMessage(turnOff);
          }
        }
      }
  }
  
}

void pulse(){
  long duration, cm;

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);

  cm = microsToCm(duration);

  Serial.print("Enviando: ");
  Serial.print(cm);
  Serial.println("cm");
  e220ttl.sendMessage(String(cm) + " ");
}

long microsToCm(long microsc){
  return microsc / 29 / 2;
}

void printParameters(struct Configuration configuration) {
    Serial.println("----------------------------------------");
 
    Serial.print(F("HEAD : "));  Serial.print(configuration.COMMAND, HEX);Serial.print(" ");Serial.print(configuration.STARTING_ADDRESS, HEX);Serial.print(" ");Serial.println(configuration.LENGHT, HEX);
    Serial.println(F(" "));
    Serial.print(F("AddH : "));  Serial.println(configuration.ADDH, HEX);
    Serial.print(F("AddL : "));  Serial.println(configuration.ADDL, HEX);
    Serial.println(F(" "));
    Serial.print(F("Chan : "));  Serial.print(configuration.CHAN, DEC); Serial.print(" -> "); Serial.println(configuration.getChannelDescription());
    Serial.println(F(" "));
    Serial.print(F("SpeedParityBit     : "));  Serial.print(configuration.SPED.uartParity, BIN);Serial.print(" -> "); Serial.println(configuration.SPED.getUARTParityDescription());
    Serial.print(F("SpeedUARTDatte     : "));  Serial.print(configuration.SPED.uartBaudRate, BIN);Serial.print(" -> "); Serial.println(configuration.SPED.getUARTBaudRateDescription());
    Serial.print(F("SpeedAirDataRate   : "));  Serial.print(configuration.SPED.airDataRate, BIN);Serial.print(" -> "); Serial.println(configuration.SPED.getAirDataRateDescription());
    Serial.println(F(" "));
    Serial.print(F("OptionSubPacketSett: "));  Serial.print(configuration.OPTION.subPacketSetting, BIN);Serial.print(" -> "); Serial.println(configuration.OPTION.getSubPacketSetting());
    Serial.print(F("OptionTranPower    : "));  Serial.print(configuration.OPTION.transmissionPower, BIN);Serial.print(" -> "); Serial.println(configuration.OPTION.getTransmissionPowerDescription());
    Serial.print(F("OptionRSSIAmbientNo: "));  Serial.print(configuration.OPTION.RSSIAmbientNoise, BIN);Serial.print(" -> "); Serial.println(configuration.OPTION.getRSSIAmbientNoiseEnable());
    Serial.println(F(" "));
    Serial.print(F("TransModeWORPeriod : "));  Serial.print(configuration.TRANSMISSION_MODE.WORPeriod, BIN);Serial.print(" -> "); Serial.println(configuration.TRANSMISSION_MODE.getWORPeriodByParamsDescription());
    Serial.print(F("TransModeEnableLBT : "));  Serial.print(configuration.TRANSMISSION_MODE.enableLBT, BIN);Serial.print(" -> "); Serial.println(configuration.TRANSMISSION_MODE.getLBTEnableByteDescription());
    Serial.print(F("TransModeEnableRSSI: "));  Serial.print(configuration.TRANSMISSION_MODE.enableRSSI, BIN);Serial.print(" -> "); Serial.println(configuration.TRANSMISSION_MODE.getRSSIEnableByteDescription());
    Serial.print(F("TransModeFixedTrans: "));  Serial.print(configuration.TRANSMISSION_MODE.fixedTransmission, BIN);Serial.print(" -> "); Serial.println(configuration.TRANSMISSION_MODE.getFixedTransmissionDescription());
 
 
    Serial.println("----------------------------------------");
}
void printModuleInformation(struct ModuleInformation moduleInformation) {
    Serial.println("----------------------------------------");
    Serial.print(F("HEAD: "));  Serial.print(moduleInformation.COMMAND, HEX);Serial.print(" ");Serial.print(moduleInformation.STARTING_ADDRESS, HEX);Serial.print(" ");Serial.println(moduleInformation.LENGHT, DEC);
 
    Serial.print(F("Model no.: "));  Serial.println(moduleInformation.model, HEX);
    Serial.print(F("Version  : "));  Serial.println(moduleInformation.version, HEX);
    Serial.print(F("Features : "));  Serial.println(moduleInformation.features, HEX);
    Serial.println("----------------------------------------");
 
}