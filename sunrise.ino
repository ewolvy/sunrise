#include <Adafruit_NeoPixel.h>
#include <SimpleTimer.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define STASSID "TU_SSID"       // TODO: Poner la SSID de tu WiFi
#define STAPSK  "TU_PASSWORD"   // TODO: Poner la password de tu WiFi

#define HOSTNAME "HOSTNAME"     // TODO: Poner el hostname para el Wemos mini
#define OTAPASSWD "TU_PASS"     // TODO: La password para actualizar vía OTA

#define LED_PIN 5       // TODO: Elegir el pin donde estará conectada la tira
#define NUM_LEDS 45     // TODO: Mostrar el número de LED que tiene la tira en total (o los que se van a usar si no se ha cortado)
#define BRIGHTNESS 255  // TODO: Por si se quiere bajar el brillo total (255 = máximo)
#define SUNSIZE 30      // TODO: Porcentaje de la tira para el sol, siempre estará en el centro

const char* ssid = STASSID;       // NO TOCAR
const char* password = STAPSK;    // NO TOCAR

SimpleTimer timer;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

bool running = false;
int sun = (SUNSIZE * NUM_LEDS)/100;
int aurora = NUM_LEDS;
int sunPhase = 100;
int whiteLevel = 100;
byte red = 127; 
byte green = 127;
byte blue = 127;
byte white = 127;
int wakeDelay = 1000;
int fadeStep = 98;
int oldFadeStep = 0;
int currentAurora = 100;
int oldAurora = 0;
int currentSun = 100;
int oldSun = 0;
int sunFadeStep = 98;

void OTASetup(){
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Por defecto es el puerto 8266
  // ArduinoOTA.setPort(8266);

  // Hostname por defecto es esp8266-[ChipID]
  ArduinoOTA.setHostname(HOSTNAME);

  // Sin autenticación por defecto
  ArduinoOTA.setPassword(OTAPASSWD);

  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
}

void setup() {
  Serial.begin(115200);
  OTASetup();
  
  strip.setBrightness(BRIGHTNESS);
  strip.begin();
}

void increaseSunPhase()
{
  if (sunPhase < 100)
  {
    sunPhase++;
    timer.setTimeout(wakeDelay, increaseSunPhase);
    timer.setTimeout((wakeDelay/80), increaseFadeStep);
    timer.setTimeout((wakeDelay/80), increaseSunFadeStep);
  }
}

void increaseSunFadeStep()
{
  if (sunFadeStep < 98)
  {
    sunFadeStep++;
    timer.setTimeout((wakeDelay/80), increaseSunFadeStep);
  }
}

void increaseFadeStep()
{
  if (fadeStep < 98)
  {
    fadeStep++;
    timer.setTimeout((wakeDelay/80), increaseFadeStep);
  }
}

void increaseWhiteLevel()
{
  if(whiteLevel < 100)
  {
    whiteLevel++;
    timer.setTimeout(wakeDelay, increaseWhiteLevel);
  }
}

void drawSun()
{
  currentSun = map(sunPhase, 0, 100, 0, sun);
  if(currentSun % 2 != 0)
  {
    currentSun--;
  }
  if (currentSun != oldSun)
  {
    sunFadeStep = 0;
  }
  
  int sunStart = (NUM_LEDS/2)-(currentSun/2);
  int newSunLeft = sunStart-1;
  int newSunRight = sunStart+currentSun;
  if(newSunLeft >= 0 && newSunRight <= NUM_LEDS && sunPhase > 0)
  {
    int redValue =  map(sunFadeStep, 0, 100, 127, 255);
    int whiteValue = map(sunFadeStep, 0, 100, 0, whiteLevel);
    int greenValue = map(sunFadeStep, 0, 100, 25, 63);
    strip.setPixelColor(newSunLeft, redValue, greenValue, 0);
    strip.setPixelColor(newSunRight, redValue, greenValue, 0);
  }
  for(int i = sunStart; i < sunStart+currentSun; i++)
  {
    strip.setPixelColor(i, 255, 64,0); 
  }
  oldSun = currentSun;
}


void drawAurora()
{
  currentAurora = map(sunPhase, 0, 100, 0, aurora);
  if(currentAurora % 2 != 0)
  {
    currentAurora--;
  }
  if (currentAurora != oldAurora)
  {
    fadeStep = 0;
  }
  int sunStart = (NUM_LEDS/2)-(currentAurora/2);
  int newAuroraLeft = sunStart-1;
  int newAuroraRight = sunStart+currentAurora;
  if(newAuroraLeft >= 0 && newAuroraRight <= NUM_LEDS)
  {
   int redValue =  map(fadeStep, 0, 100, whiteLevel, 127);
   int greenValue =  map(fadeStep, 0, 100, 0, 25);
   strip.setPixelColor(newAuroraRight, redValue, greenValue,0);
   strip.setPixelColor(newAuroraLeft, redValue, greenValue,0);
  }
  for(int i = sunStart; i < sunStart+currentAurora; i++)
  {
    strip.setPixelColor(i, 192, 25,0); 
  }
  oldFadeStep = fadeStep;
  oldAurora = currentAurora;
}

void drawAmbient()
{
  for(int i = 0; i < NUM_LEDS; i++)
  {
    int hueValue = map (whiteLevel, 0, 100, 0, 32);
    strip.setPixelColor(i, strip.ColorHSV(hueValue, 255, whiteLevel)); 
  }
}

void sunRise()
{
  drawAmbient();
  drawAurora();
  drawSun();
}

void off()
{
  for(int i = 0; i < NUM_LEDS; i++)
  {
    strip.setPixelColor(i, 0,0,0); 
  }
}

void beginSunrise(){
  running = true;
  whiteLevel = 0;
  sunPhase = 0;
  fadeStep = 0;
  sunFadeStep = 0;
  timer.setTimeout(wakeDelay, increaseSunPhase);
  timer.setTimeout(wakeDelay, increaseWhiteLevel);
  timer.setTimeout((wakeDelay/80), increaseFadeStep);
  timer.setTimeout((wakeDelay/80), increaseSunFadeStep);
}

void loop() {
  ArduinoOTA.handle();
  if (!running){
    beginSunrise();
  }

  timer.run();
  sunRise();
  strip.show();
}
