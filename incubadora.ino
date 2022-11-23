#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
//#include "EspMQTTClient.h"



// const char* ssid = "Heraclio's Galaxy M32";
// const char* password = "Konectar";
const char* ssid = "a";
const char* password = "12345677";
const char* mqttServer = "node02.myqtthub.com";
const int mqttPort = 1883;
// const char* mqttUser = "incubadora";
// const char* mqttPassword = "6b1ViXeg-1yArLyhH";
const char* mqttUser = "ESP32-1";
const char* mqttPassword = "ESP32-1";

int tempMax = 30;
int dhtPort = 14;
int ventiladorPort = 7; 
int ledAmarilloPort = 33;
int ledverdePort = 32;
int ledRojoPort = 35;
int ledCalefaccionPort = 0;
int puertaPort = 5;

char msg[25];
long count = 0;


const char* TOPIC_HUMEDAD = "humedad";
const char* TOPIC_PUERTA = "puerta";
const char* TOPIC_TEMPERATURA = "temperatura";
const char* TOPIC_TEMPERATURA_MAX = "temperatura-max";

void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void setup_wifi();

WiFiClient espClient;
DHT dht(dhtPort, DHT11);
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  setup_wifi();
  // Inicia Sensor Temperatura
  dht.begin();


  digitalWrite(ledverdePort, HIGH);
  digitalWrite(ledAmarilloPort, HIGH);
  digitalWrite(ledRojoPort, HIGH);
  digitalWrite(puertaPort, HIGH);

  delay(5000);
  digitalWrite(ledverdePort, LOW);
  digitalWrite(ledAmarilloPort, LOW);
  digitalWrite(ledRojoPort, LOW);
  digitalWrite(puertaPort, LOW);


  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}


//*****
//*    CONEXION WIFI      *
//*****
void setup_wifi() {
  delay(5000);
  // Nos conectamos a nuestra red Wifi
  Serial.println();
  Serial.print("Conectando a ssid: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("./");
  }

  Serial.println("");
  Serial.println("Conectado a red WiFi!");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());
}


void connectTopics() {
  // Nos suscribimos
  if (client.subscribe(TOPIC_TEMPERATURA)) {
    Serial.println("Suscripcion a topic " + String(TOPIC_TEMPERATURA));
  } else {
    Serial.println("fallo Suscripciión a topic " + String(TOPIC_TEMPERATURA));
  }

  if (client.subscribe(TOPIC_TEMPERATURA_MAX)) {
    Serial.println("Suscripcion a topic " + String(TOPIC_TEMPERATURA_MAX));
  } else {
    Serial.println("fallo Suscripciión a topic " + String(TOPIC_TEMPERATURA_MAX));
  }

  if (client.subscribe(TOPIC_HUMEDAD)) {
    Serial.println("Suscripcion a topic " + String(TOPIC_HUMEDAD));
  } else {
    Serial.println("fallo Suscripciión a topic " + String(TOPIC_HUMEDAD));
  }

  if (client.subscribe(TOPIC_PUERTA)) {
    Serial.println("Suscripcion a topic " + String(TOPIC_PUERTA));
  } else {
    Serial.println("fallo Suscripciión a topic " + String(TOPIC_PUERTA));
  }
}

void indicadoresLed(String temp) {
  int tempInt = temp.toInt();
  int rango = 2;

  if (tempInt >= tempMax - rango && tempInt < tempMax) {
    digitalWrite(ledverdePort, HIGH);
    digitalWrite(ledRojoPort, LOW);
    digitalWrite(ledAmarilloPort, LOW);
    //    Apagamos Calefacción
    digitalWrite(ledCalefaccionPort, LOW);
  }
  if (tempInt < tempMax - rango) {
    digitalWrite(ledAmarilloPort, HIGH);
    digitalWrite(ledverdePort, LOW);
    digitalWrite(ledRojoPort, LOW);

    //    Encendemos Calefacción
    digitalWrite(ledCalefaccionPort, HIGH);
  }

  if (tempInt >= tempMax) {
    digitalWrite(ledRojoPort, HIGH);
    digitalWrite(ledverdePort, LOW);
    digitalWrite(ledAmarilloPort, LOW);
  }
}

void getTemperature() {
  float humedad = dht.readHumidity();
  float temp = dht.readTemperature();
  Serial.print("Temperatura: ");
  Serial.print(temp);
  Serial.print("ºC Humedad: ");
  Serial.print(humedad);
  Serial.println("%");
  delay(10000);

  // Comprobamos si ha habido algún error en la lectura
  if (isnan(humedad) || isnan(temp)) {
    Serial.println("Error obteniendo los datos del sensor DHT11");
    // return;  //Comentar si no lee temperatura
  }

  temp = random(10, 50);  //Descomentar para hacer pruebas
  humedad = random(10, 50);


  Serial.print("Temperatura Prueba: ");
  Serial.print(temp);
  Serial.print("ºC Humedad Prueba: ");
  Serial.print(humedad);
  Serial.println("%");
  delay(10000);


  // Encender/Apagar LEDs
  indicadoresLed(String(temp));

  // Enviar la temp al MQTT
  sendTemperature(String(temp), String(humedad));


  if (temp >= tempMax) {
    // Encender ventilador
    digitalWrite(ventiladorPort, HIGH);
  } else {
    // Apagar Ventilador
    digitalWrite(ventiladorPort, LOW);
  }
}



void sendTemperature(String temp, String hum) {

  // String stringOne = "A string";
  char hum2[hum.length() + 1];
  char temp2[hum.length() + 1];
  hum.toCharArray(hum2, hum.length() + 1);
  temp.toCharArray(temp2, hum.length() + 1);


  client.publish(TOPIC_HUMEDAD, hum2);
  client.publish(TOPIC_TEMPERATURA, temp2);
}


void callback(char* topic, byte* payload, unsigned int length) {
  String incoming = "";
  Serial.print("Mensaje recibido desde -> ");
  Serial.print(topic);

  Serial.println("");
  for (int i = 0; i < length; i++) {
    incoming += (char)payload[i];
  }
  incoming.trim();
  Serial.println("Mensaje -> " + incoming);

  // Temp Max
  if (topic == TOPIC_TEMPERATURA_MAX) {
    tempMax = incoming.toInt();
  }

  // Puerta!!!
  if (topic == TOPIC_PUERTA) {
    if (incoming == "open") {
      Serial.println("Se Abrió la puerta.");
      digitalWrite(puertaPort, HIGH);
      delay(1000);
      digitalWrite(puertaPort, LOW);
    }
    if (incoming == "close") {
      digitalWrite(puertaPort, HIGH);
      delay(1000);
      digitalWrite(puertaPort, LOW);
      client.publish(TOPIC_PUERTA, "closed");
      Serial.println("Se Cerró la puerta");
    }
  }
}

void loop() {
  getTemperature();
  if (!client.connected()) {
    reconnect();
  }

  // if (client.connected()) {
  //   String str = "La cuenta es -> " + String(count);
  //   str.toCharArray(msg, 25);
  //   client.publish(TOPIC_TEMPERATURA, msg);
  //   // count++;
  //   Serial.println(msg);
  //   delay(3000);
  // }

  client.loop();
  delay(500);
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32-1", mqttUser, mqttPassword)) {
      Serial.println("connected");

      connectTopics();

    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}