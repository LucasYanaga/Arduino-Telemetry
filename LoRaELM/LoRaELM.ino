#include <SoftwareSerial.h>
#include <ELMduino.h>

#define EN_PIN 7
// Configuração do SoftwareSerial para o HC-05
SoftwareSerial BTSerial(8, 9); // 8 - RX Arduino -> TX BT && 9 - TX Arduino -> RX BT

// Instanciar o objeto ELM327
ELM327 myELM327;

// Variáveis para armazenar RPM e Velocidade
float rpm = 0;
float speed = 0;

// Estado da conexão
bool isConnected = false;

void setup() {
  // Entrar no modo AT manualmente
  pinMode(EN_PIN, OUTPUT); // Pino 9 para controlar o EN (Enable) do HC-05
  digitalWrite(EN_PIN, HIGH); // Colocar o pino EN em HIGH para modo AT

  // Comunicação com o PC
  Serial.begin(9600);
  
  // Comunicação com o HC-05 (modo AT)
  BTSerial.begin(38400); // Modo AT do HC-05
  
  delay(5000); // Aguardar um pouco

  // Iniciar configuração
  Serial.println("Iniciando configuração do HC-05...");
  
  // Verificar comunicação AT
  if (!enviarComandoAT("AT")) {
    Serial.println("Erro na comunicação AT.");
    return; // Sai se não conseguir comunicar
  }

  // Limpar dispositivos emparelhados
  Serial.println("Removendo dispositivos previamente emparelhados...");
  enviarComandoAT("AT+RMAAD"); // Remove todos os dispositivos emparelhados

  // Configurar como master
  enviarComandoAT("AT+ROLE=1");

  // Buscar dispositivos próximos
  Serial.println("Buscando dispositivos próximos...");
  if (!enviarComandoAT("AT+INQ")) {
    Serial.println("Erro ao buscar dispositivos.");
    return; // Sai se não conseguir buscar
  }

  // Escolher o dispositivo ELM327
  String elm327Mac = buscarELM327();
  
  if (elm327Mac != "") {
    // Emparelhar com o ELM327
    if (enviarComandoAT("AT+BIND=" + elm327Mac)) {
      // Conectar ao ELM327
      if (enviarComandoAT("AT+LINK=" + elm327Mac)) {
        Serial.println("Conexão com o ELM327 estabelecida!");

        // Sair do modo AT e preparar para comunicação normal
        digitalWrite(9, LOW); // Tirar o pino EN do modo AT
        BTSerial.begin(9600); // Mudar a velocidade para comunicação padrão
        Serial.println("Saindo do modo AT.");

        // Iniciar comunicação com o ELM327
        if (!myELM327.begin(BTSerial)) {
          Serial.println("Não foi possível iniciar a comunicação com o ELM327.");
          while (1); // Trava o código aqui se não conectar
        }

        isConnected = true; // Atualiza o estado da conexão
        Serial.println("Conexão com o ELM327 e HC-05 estabelecida!");
      } else {
        Serial.println("Falha ao conectar ao ELM327.");
      }
    } else {
      Serial.println("Falha ao emparelhar com o ELM327.");
    }
  } else {
    Serial.println("ELM327 não encontrado.");
    while (1); // Trava o código se não encontrar o ELM327
  }
}

void loop() {
  if (isConnected) {
    // Ler RPM do motor
    rpm = myELM327.rpm();
    if (myELM327.nb_rx_state == ELM_SUCCESS) {
      Serial.print("RPM: ");
      Serial.println(rpm);
    } else {
      Serial.println("Erro ao ler RPM.");
    }

    // Ler velocidade do veículo
    speed = myELM327.kph();
    if (myELM327.nb_rx_state == ELM_SUCCESS) {
      Serial.print("Velocidade (km/h): ");
      Serial.println(speed);
    } else {
      Serial.println("Erro ao ler a velocidade.");
    }
  } else {
    Serial.println("Aguardando conexão com ELM327...");
  }

  delay(1000); // Esperar 1 segundo antes de fazer a próxima leitura
}

// Função para enviar os comandos AT e ler a resposta
bool enviarComandoAT(String comando) {
  BTSerial.println(comando);
  Serial.print("Enviado: ");
  Serial.println(comando);
  
  delay(1000); // Aguardar resposta
  
  if (BTSerial.available()) {
    String resposta = "";
    while (BTSerial.available()) {
      char c = BTSerial.read();
      resposta += c; // Armazenar a resposta
    }
    Serial.print("Resposta: ");
    Serial.println(resposta);
    return resposta.indexOf("OK") != -1; // Verifica se a resposta contém "OK"
  } else {
    Serial.println("Sem resposta.");
    return false;
  }
}

// Função para buscar o dispositivo ELM327
String buscarELM327() {
  String resposta = "";
  
  // Esperar e ler a resposta do comando INQ
  delay(5000); // Aguardar a busca (5 segundos)
  
  if (BTSerial.available()) {
    while (BTSerial.available()) {
      char c = BTSerial.read();
      resposta += c;
    }
  }
  
  Serial.println("Resposta INQ:");
  Serial.println(resposta);
  
  // Analisar a resposta e encontrar o ELM327
  if (resposta.indexOf("ELM327") != -1) {
    int index = resposta.indexOf("ELM327");
    String mac = resposta.substring(index - 17, index - 2); // O MAC vem antes do nome
    Serial.println("ELM327 encontrado: " + mac);
    return mac;
  } else {
    return "";
  }
}
