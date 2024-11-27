#include <CAN.h> 
#include <OBD2.h>
#include "LoRa_E220.h"
#include <ArduinoJson.h>
#define DESTINATION_ADDL 2

// PIDS
const int PIDS[] = {
  CALCULATED_ENGINE_LOAD,
  ENGINE_RPM,
  VEHICLE_SPEED,
  THROTTLE_POSITION,
  FUEL_AIR_COMMANDED_EQUIVALENCE_RATE 
};

const int NUM_PIDS = sizeof(PIDS) / sizeof(PIDS[0]);

//LoRa Ebyte E220
LoRa_E220 e220ttl(7, 8, 6, 10, 9); // TX E220, RX E220, AUX, M0, M1
void printParameters(struct Configuration configuration);
void printModuleInformation(struct ModuleInformation moduleInformation);

//JSON
StaticJsonDocument<200> json;
const int TAMANHO_PACOTE = 58;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  //START Setup LoRa Ebyte E220 -----------------------------------------------------
  digitalWrite(10, LOW);
  digitalWrite(9, LOW);

  // Startup all pins and UART
  e220ttl.begin();

  ResponseStructContainer c;
  c = e220ttl.getConfiguration();
  // It's important get configuration pointer before all other operation
  Configuration configuration = *(Configuration*) c.data;
  Serial.println("[LoRa]: Configuração Atual: ");
  Serial.println(c.status.getResponseDescription());
  Serial.println(c.status.code);

  printParameters(configuration);
  //  ----------------------- DEFAULT TRANSPARENT WITH RSSI -----------------------
  configuration.ADDL = 0x02;
  configuration.ADDH = 0x00;

  configuration.CHAN = 23;
  
  configuration.SPED.uartBaudRate = UART_BPS_9600;
  configuration.SPED.airDataRate = AIR_DATA_RATE_010_24;
  configuration.SPED.uartParity = MODE_00_8N1;
  
  configuration.OPTION.subPacketSetting = SPS_200_00;
  configuration.OPTION.RSSIAmbientNoise = RSSI_AMBIENT_NOISE_ENABLED;
  configuration.OPTION.transmissionPower = POWER_22;
  
  configuration.TRANSMISSION_MODE.enableRSSI = RSSI_ENABLED;
  configuration.TRANSMISSION_MODE.fixedTransmission = FT_FIXED_TRANSMISSION;
  configuration.TRANSMISSION_MODE.enableLBT = LBT_DISABLED;
  configuration.TRANSMISSION_MODE.WORPeriod = WOR_500_000;

  configuration.CRYPT.CRYPT_H = 0x01;
	configuration.CRYPT.CRYPT_L = 0x01;

  Serial.println("[LoRa]: Setando Configuração: ");
  ResponseStatus rs = e220ttl.setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);
  Serial.println(rs.getResponseDescription());
  Serial.println(rs.code);
 
  Serial.println("[LoRa]: Configuração Após Setar: ");
  c = e220ttl.getConfiguration();
  // It's important get configuration pointer before all other operation
  configuration = *(Configuration*) c.data;
  Serial.println(c.status.getResponseDescription());
  Serial.println(c.status.code);
 
  printParameters(configuration);
 
  c.close();

  //END Setup LoRa Ebyte E220 ----------------------------------------------------------

  //START Setup MCP2515-----------------------------------------------------------------
  Serial.println(F("[OBD2]: Key Stats"));

  /*
  while (true) {
    Serial.print(F("[OBD2]: Tentando Conexão... "));

    if (!OBD2.begin()) {
      Serial.println(F("[OBD2]: ERROR!"));

      delay(1000);
    } else {
      Serial.println(F("[OBD2]: SUCESSO!"));
      break;
    }
  }
  */

  Serial.println();
  //END Setup MCP2515-----------------------------------------------------------------

  json["id"] = "1";
  
  Serial.println("[LoRa]: Iniciando transmissão de dados");
}

void loop() {
  String jsonOut;
  
  int count = 1;
  for (int i = 0; i < NUM_PIDS; i++) {
    int pid = PIDS[i];
    //float pidValue = readPID(pid);
    float pidValue = i * count + 10;
    toJson(pidValue, pid);
    count ++;
  }

  serializeJson(json, jsonOut);
  Serial.println("[LoRa]: " + jsonOut);
  //particionarEEnviarMensagem(jsonOut);
  ResponseStatus rs = e220ttl.sendFixedMessage(0, 3, 23, jsonOut);
  Serial.println(rs.getResponseDescription());
  if(rs.getResponseDescription() != "Success"){
    Serial.print("[LoRa]: ERROR!");
  }

  delay(1000);
}

float readPID(int pid) {
  // read the PID value
  float pidValue = OBD2.pidRead(pid);

  if (isnan(pidValue)) {
    Serial.print("[OBD2]: ERROR -> " + OBD2.pidName(pid));
  } else {
    return pidValue;
  }

  Serial.println();
}

void toJson(float pidValue, int pid){
  switch(pid){
    case CALCULATED_ENGINE_LOAD:
      json["eng_load"] = pidValue;
      break;
    case ENGINE_RPM:
      json["rpm"] = pidValue;
      break;
    case VEHICLE_SPEED: 
      json["spd"] = pidValue;
      break;
    case THROTTLE_POSITION:
      json["tps"] = pidValue;
      break;
    case FUEL_AIR_COMMANDED_EQUIVALENCE_RATE : 
      json["lambda"] = pidValue;
      break;
  }
}

void particionarEEnviarMensagem(String mensagemJSON){
  int totalPacotes = (mensagemJSON.length() / (TAMANHO_PACOTE - 10)) + 1; // Ajusta para incluir metadados
  Serial.println("Total Pacotes: " + String(totalPacotes));
  for (int i = 0; i < totalPacotes; i++) {
    String fragmento = mensagemJSON.substring(i * (TAMANHO_PACOTE - 10), (i + 1) * (TAMANHO_PACOTE - 10));
    // checksum = calcularChecksum(fragmento);
    char pacote1[58];  // Defina o tamanho conforme necessário
    //snprintf(pacote1, sizeof(pacote1), "%d/%d:%s#%u", i + 1, totalPacotes, fragmento.c_str(), checksum);
    snprintf(pacote1, sizeof(pacote1), "%d/%d:%s", i + 1, totalPacotes, fragmento.c_str());

    Serial.print("PACOTE COMPLETO: ");
    Serial.println(pacote1);

    ResponseStatus rs = e220ttl.sendFixedMessage(0, 3, 23, pacote1);
    Serial.println(rs.getResponseDescription());
    if(rs.getResponseDescription() != "Success"){
      Serial.print("[LoRa]: ERROR!");
    }

    delay(100); // Atraso para garantir que o receptor possa processar cada pacote
  }

  Serial.println("----------------------------------------");
}

byte calcularChecksum(String pacote) {
  byte checksum = 0;
  for (int i = 0; i < pacote.length(); i++) {
    checksum ^= pacote[i]; // XOR de cada caractere para calcular o checksum
  }
  return checksum;
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