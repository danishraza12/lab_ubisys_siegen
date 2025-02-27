#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Arduino_JSON.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"

#define SDA_PIN 23 // Replace with your SDA pin
#define SCL_PIN 19 // Replace with your SCL pin

#define STEP_THRESHOLD 5.0      // Adjust this threshold based on sensitivity
#define ALTITUDE_THRESHOLD 0.03 // Change in meters that indicates stair climbing
#define SAMPLE_INTERVAL 100     // Sample interval in milliseconds
float previousPressure = 0;

int PORT = 80;
AsyncWebServer server(PORT);

// Create an SSE (Server-Sent Events) instance
AsyncEventSource events("/events");

// Json Variable to Hold Sensor Readings
JSONVar sensorData;

unsigned long lastGyroReadTime = 0;
unsigned long lastTempReadTime = 0;
unsigned long lastAccReadTime = 0;
unsigned long lastUserStatusTime = 0;
unsigned long lastBMPTime = 0;

unsigned long gyroDelay = 500;
unsigned long temperatureDelay = 500;
unsigned long accelerometerDelay = 500;
unsigned long bmpDelay = 500;
unsigned long userStatusDelay = 500;


// Create a sensor object
Adafruit_MPU6050 mpu;
Adafruit_BMP280 bmp;

sensors_event_t a, g, temp;

float gyroX, gyroY, gyroZ;
float accX, accY, accZ;
float temperature;
float bmpTemperature, bmpPressure;

// Gyroscope sensor deviation
float gyroXerror = 0.00;
float gyroYerror = 0.00;
float gyroZerror = 0.00;

// Gyro LowPass Filter constant
const float alpha = 0.5;

#define BUFFER_SIZE 15
float accHistory[3][HISTORY_SIZE];
int rowIdx = 0;
bool historyArrFull = false;

void initMPU6050()
{
  Serial.println("Initializing MPU6050...");
  if (!mpu.begin())
  {
    Serial.println("Could not find a valid MPU6050 sensor!");
    while (1); // Stop the program
  }

  Serial.println("MPU6050 Initialized. Ready to read data.");
}

void initBMP280()
{
  Serial.println("Initializing BMP280...");
  if (!bmp.begin(0x76)) // Default I²C address is 0x76
  {
    Serial.println("Could not find a valid BMP280 sensor!");
    while (1);
  }

  // Configure BMP280 settings
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);
  previousPressure = bmp.readPressure();
}

void initLittleFS()
{
  if (!LittleFS.begin())
  {
    Serial.println("Unable to mount LittleFS.");
    return;
  }
  Serial.println("LittleFS mounted successfully.");
}

// Initialize WiFi
void initWiFi()
{
  const char *ssid = "test";
  const char *password = "test123";

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    int i = 0;
    delay(1000);
    Serial.print("WiFi status: ");
    Serial.println(WiFi.status());
    Serial.println("Connecting to WiFi... ");
  }

  Serial.println("Connected to Wi-Fi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void calibrateGyro()
{
  Serial.println("Calibrating gyro...");
  const int numReadings = 100;
  float x = 0.0, y = 0.0, z = 0.0;

  for (int i = 0; i < numReadings; i++)
  {
    mpu.getEvent(&a, &g, &temp);
    x += g.gyro.x;
    y += g.gyro.y;
    z += g.gyro.z;
    delay(10);
  }

  gyroXerror = x / numReadings;
  gyroYerror = y / numReadings;
  gyroZerror = z / numReadings;

  Serial.print("Gyro calibration done. Offsets: ");
  Serial.print("X: ");
  Serial.print(gyroXerror);
  Serial.print(" Y: ");
  Serial.print(gyroYerror);
  Serial.print(" Z: ");
  Serial.println(gyroZerror);
}

String getGyroReadings()
{
  mpu.getEvent(&a, &g, &temp);

  // Apply calibration offsets
  float gyroX_temp = g.gyro.x - gyroXerror;
  float gyroY_temp = g.gyro.y - gyroYerror;
  float gyroZ_temp = g.gyro.z - gyroZerror;

  // Apply low-pass filter
  gyroX = alpha * gyroX + (1 - alpha) * gyroX_temp;
  gyroY = alpha * gyroY + (1 - alpha) * gyroY_temp;
  gyroZ = alpha * gyroZ + (1 - alpha) * gyroZ_temp;

  sensorData["gyroX"] = String(gyroX);
  sensorData["gyroY"] = String(gyroY);
  sensorData["gyroZ"] = String(gyroZ);

  String jsonString = JSON.stringify(readings);
  return jsonString;
}

String getTemperature()
{
  mpu.getEvent(&a, &g, &temp);
  temperature = temp.temperature;
  return String(temperature);
}

String getBMPReadings()
{
  bmpTemperature = bmp.readTemperature();
  bmpPressure = bmp.readPressure() / 100.0F; // Convert Pa to hPa

  sensorData["bmpTemperature"] = String(bmpTemperature);
  sensorData["bmpPressure"] = String(bmpPressure);

  String bmpString = JSON.stringify(readings);
  return bmpString;
}

String getAccReadings()
{
  mpu.getEvent(&a, &g, &temp);

  // Get current acceleration values
  accX = a.acceleration.x;
  accY = a.acceleration.y;
  accZ = a.acceleration.z;

  sensorData["accX"] = String(accX);
  sensorData["accY"] = String(accY);
  sensorData["accZ"] = String(accZ);

  // Buffer holds last 15 readings for each axis
  accHistory[0][rowIdx] = accX;
  accHistory[1][rowIdx] = accY;
  accHistory[2][rowIdx] = accZ;
  if (rowIdx >= HISTORY_SIZE)
  {
    rowIdx = 0;
    historyArrFull = true;
  }
  rowIdx++;

  String accString = JSON.stringify(readings);
  return accString;
}

bool detectStep()
{
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Calculate the magnitude of acceleration
  float accMagnitude = sqrt(a.acceleration.x * a.acceleration.x +
                              a.acceleration.y * a.acceleration.y +
                              a.acceleration.z * a.acceleration.z);

  Serial.print("Accel Magnitude: ");
  Serial.println(accMagnitude);

  if (accMagnitude > STEP_THRESHOLD)
  {
    return true;
  }
  return false;
}

bool detectAltitudeChange()
{
  float currentPressure = bmp.readPressure();
  float pressureChange = previousPressure - currentPressure;

  previousPressure = currentPressure;

  if (abs(pressureChange) > ALTITUDE_THRESHOLD)
  {
    return true;
  }
  return false;
}

bool detectRotation()
{
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float gyroX = g.gyro.x;
  float gyroY = g.gyro.y;
  float gyroZ = g.gyro.z;

  // threshold for significant rotation
  float GYRO_THRESHOLD = 0.3;

  // detecting rotation
  if (abs(gyroX) > GYRO_THRESHOLD || abs(gyroY) > GYRO_THRESHOLD || abs(gyroZ) > GYRO_THRESHOLD)
  {
    return true;
  }
  return false;
}

void initServer()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LittleFS, "/index.html", "text/html"); 
    }
  );

  server.serveStatic("/", LittleFS, "/");

  // Handle Web Server Events
  events.onConnect([](AsyncEventSourceClient *client) {
      if(client->lastId()) {
        Serial.printf("Events reconnected with message ID: %u\n", client->lastId());
      }
      client->send("First message!", NULL, millis(), 10000); 
    }
  );

  server.addHandler(&events);
  server.begin();
  Serial.println("Server started on PORT: %u", PORT);
}

void initDetection()
{
  // only get gyro data after the gyro delay has passed
  if ((millis() - lastGyroReadTime) > gyroDelay)
  {
    // Send Events to the Web Server with the Sensor Readings
    events.send(getGyroReadings().c_str(), "gyroData", millis());
    lastGyroReadTime = millis();
  }

  // only get accelerometer data after the accelerometer delay has passed
  if ((millis() - lastAccReadTime) > accelerometerDelay)
  {
    // Send Events to the Web Server with the Sensor Readings
    events.send(getAccReadings().c_str(), "accelerometerData", millis());

    lastAccReadTime = millis();
  }

  if ((millis() - lastUserStatusTime) > userStatusDelay)
  {

    char *flag = "REST";

    if (detectStep() && !detectAltitudeChange() && detectRotation())
    {
      flag = "WALKING";
      Serial.println("User is walking");
    }
    else if (detectStep() && detectAltitudeChange() && detectRotation())
    {
      flag = "STAIRS";
      Serial.println("User is taking the stairs");
    }
    else if (!detectStep() && detectAltitudeChange() && !detectRotation())
    {
      flag = "ELEVATOR";
      Serial.println("User is taking the elevator");
    }

    events.send(flag, "user_status", millis());
    Serial.println(flag);
    lastUserStatusTime = millis();
  }

  if ((millis() - lastTempReadTime) > temperatureDelay)
  {
    events.send(getTemperature().c_str(), "temperatureData", millis());
    lastTempReadTime = millis();
  }
  
  if ((millis() - lastTimeBMP) > bmpDelay)
  {
    events.send(getBMPReadings().c_str(), "bmpData", millis());
    lastTimeBMP = millis();
  }
}

void setup()
{
  // There was some bug where flash did not get erased so added delay before and after starting
  delay(1000);
  Serial.begin(115200);
  delay(3000);

  Wire.begin(SDA_PIN, SCL_PIN); // Initialize I²C with custom SDA and SCL pins
  Serial.println("Starting Stairs vs Left Detection System...");

  initBMP280();
  initMPU6050();
  calibrateGyro();
  initWiFi();
  initLittleFS();
  initServer();
}

void loop()
{
  initDetection();

  Serial.print("Sensor Data: ");
  Serial.println(JSON.stringify(sensorData));

  events.send("initialPing", NULL, millis());
  events.send(JSON.stringify(sensorData).c_str(), "sensorData", millis());

  delay(1000); // Wait for 1 second before the next reading
}