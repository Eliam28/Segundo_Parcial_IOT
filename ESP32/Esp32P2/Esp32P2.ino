#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>


// CAMBIAR ESTO
const char* ssid = "INFINITUMD8A4_2.4";

// CAMBIAR ESTO
const char* password = "wRwQR6e52m";


// CAMBIAR ESTO
const char* mqtt_server = "192.168.1.198";

// Puerto MQTT por defecto
const int mqtt_port = 1883;

// Topics
const char* topic_sensores = "invernadero/sensores";
const char* topic_riego = "invernadero/control/riego";
const char* topic_ventilador = "invernadero/control/ventilador";
const char* topic_ack = "invernadero/ack";


// DHT22
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Relays
#define RELAY_RIEGO 32
#define RELAY_VENTILADOR 33

// Cliente WiFi
WiFiClient espClient;

// Cliente MQTT
PubSubClient client(espClient);
unsigned long ultimoEnvio = 0;
const long intervalo = 5000;


void conectarWiFi() {

  Serial.println("================================");
  Serial.println("Conectando a WiFi...");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  // Esperar conexión
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado correctamente");
  Serial.println("IP del ESP32:");
  Serial.println(WiFi.localIP());

  Serial.println("================================");
}

void callback(char* topic, byte* payload, unsigned int length) {

  Serial.println("");
  Serial.println("========== MENSAJE RECIBIDO ==========");

  // Convertir payload a String
  String mensaje = "";

  for (int i = 0; i < length; i++) {
    mensaje += (char)payload[i];
  }

  Serial.print("Topic: ");
  Serial.println(topic);

  Serial.print("Mensaje: ");
  Serial.println(mensaje);

  if (String(topic) == topic_riego) {

    // Encender relay
    if (mensaje == "ON") {

      digitalWrite(RELAY_RIEGO, LOW);

      Serial.println("Riego ACTIVADO");

      enviarACK("riego", "ON");
    }
    else if (mensaje == "OFF") {

      digitalWrite(RELAY_RIEGO, HIGH);

      Serial.println("Riego DESACTIVADO");

      enviarACK("riego", "OFF");
    }
  }

  if (String(topic) == topic_ventilador) {

    // Encender relay
    if (mensaje == "ON") {

      digitalWrite(RELAY_VENTILADOR, LOW);

      Serial.println("Ventilador ACTIVADO");

      enviarACK("ventilador", "ON");
    }
    else if (mensaje == "OFF") {

      digitalWrite(RELAY_VENTILADOR, HIGH);

      Serial.println("Ventilador DESACTIVADO");

      enviarACK("ventilador", "OFF");
    }
  }
  Serial.println("======================================");
}


void enviarACK(String actuador, String estado) {

  StaticJsonDocument<200> doc;

  doc["actuador"] = actuador;
  doc["estado"] = estado;
  doc["ts"] = millis();
  char buffer[200];

  serializeJson(doc, buffer);
  client.publish(topic_ack, buffer);

  Serial.println("ACK enviado:");
  Serial.println(buffer);
}


void reconnectMQTT() {

  while (!client.connected()) {

    Serial.println("Conectando a MQTT...");
    String clientId = "ESP32-Invernadero";
    if (client.connect(clientId.c_str())) {

      Serial.println("MQTT conectado correctamente");

      client.subscribe(topic_riego);
      client.subscribe(topic_ventilador);

      Serial.println("Suscrito a:");
      Serial.println(topic_riego);
      Serial.println(topic_ventilador);
    }

    else {

      Serial.print("Error MQTT, rc=");
      Serial.print(client.state());

      Serial.println(" Reintentando en 5 segundos...");

      delay(5000);
    }
  }
}


void setup() {

  Serial.begin(9600);

  Serial.println("");
  Serial.println("================================");
  Serial.println("INICIANDO SISTEMA INVERNADERO");
  Serial.println("================================");


  pinMode(RELAY_RIEGO, OUTPUT);
  pinMode(RELAY_VENTILADOR, OUTPUT);

  digitalWrite(RELAY_RIEGO, HIGH);
  digitalWrite(RELAY_VENTILADOR, HIGH);

  dht.begin();

  Serial.println("DHT22 iniciado");


  conectarWiFi();

  client.setServer(mqtt_server, mqtt_port);

  client.setCallback(callback);

  Serial.println("Sistema listo");
}

void loop() {

  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  unsigned long tiempoActual = millis();

  if (tiempoActual - ultimoEnvio >= intervalo) {

    ultimoEnvio = tiempoActual;


    float temperatura = dht.readTemperature();

    float humedad = dht.readHumidity();

    if (isnan(temperatura) || isnan(humedad)) {

      Serial.println("Error leyendo DHT22");

      return;
    }

    int co2 = random(300, 901);

    StaticJsonDocument<256> doc;

    doc["temp"] = temperatura;
    doc["hum"] = humedad;
    doc["co2"] = co2;

    doc["dispositivo"] = "esp32-01";

    char jsonBuffer[256];

    serializeJson(doc, jsonBuffer);

    client.publish(topic_sensores, jsonBuffer);

    Serial.println("");
    Serial.println("========== DATOS ENVIADOS ==========");

    Serial.print("Temperatura: ");
    Serial.print(temperatura);
    Serial.println(" C");

    Serial.print("Humedad: ");
    Serial.print(humedad);
    Serial.println(" %");

    Serial.print("CO2: ");
    Serial.print(co2);
    Serial.println(" ppm");

    Serial.println("JSON enviado:");

    Serial.println(jsonBuffer);

    Serial.println("====================================");
  }
}