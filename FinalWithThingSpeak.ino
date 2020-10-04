/* CONNECTION:
  DHT11:
  VCC - Vin
  GND- GND
  DATA - GPIO0/D3

  BMP180:
  VCC - 3.3V 
  GND - GND
  SCL- D1
  SDA - D2
  (WARNING: do not connect + to 5V or the sensor will be damaged!)

  TSL2561:
  VCC - 3.3V
  GND -GND
  SCL - D1
  SDA - D2


*/
#include <ThingSpeak.h>               // add librery
#include <ESP8266WiFi.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>   // including the library of DHT11 temperature and humidity sensor
#include <Adafruit_BMP085_U.h>
#include <Adafruit_TSL2561_U.h>
#include <SFE_BMP180.h>
#include <Wire.h>



WiFiClient  client;
unsigned long counterChannelNumber = 1171764;                // Channel ID
const char * myCounterReadAPIKey = "7CK4XE2KOM6QIJC1";      // Read API Key
const char * myCounterWriteAPIKey = "9IE05N1E0YDM79K3";     // Write API Key
const int FieldNumber1 = 1;
const int FieldNumber2 = 2;
const int FieldNumber3 = 3;
const int FieldNumber4 = 4;
const int FieldNumber5 = 5;
const int FieldNumber6 = 6;   

SFE_BMP180 pressure; // You will need to create an SFE_BMP180 object, here called "pressure":
#define ALTITUDE 1655.0 // Altitude in meters

//By modifying the I2C address we can have up to three TSL2561 sensors connected on the same board:
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);  // 12345 default address

DHT dht(0, DHT11); // GPIO0, DHT11
char status;
double T, P, p0, a;


void setup(void)
{
  Serial.begin(115200);
  Serial.println();

  WiFi.begin("Redmi", "123456789");                 // write wifi name & password

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  ThingSpeak.begin(client);
  
  dht.begin();
  Serial.println("Humidity and temperature\n\n");
  delay(700);

  Serial.println("Light Sensor Test");
  Serial.println("");
  
  tsl.enableAutoRange(true);  //Auto-gain ... switches automatically between 1x and 16x

  // Changing the integration time gives you better sensor resolution (402ms = 16-bit data)

  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);   /* fast but low resolution */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

  Serial.println(" ");

  if (pressure.begin())
    Serial.println("BMP180 init success");
  else
  {
    Serial.println("BMP180 init fail\n\n");
  }
}


void loop() {
  /*temperature sensor*/
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  Serial.print("Humidity = ");
  Serial.print(h);
  Serial.print("%  ");
  ThingSpeak.setField(1 , h);           // hum
  Serial.print("temperature = ");
  Serial.print(t);
  Serial.println("C  ");
  Serial.println(" ");
  ThingSpeak.setField(2 , t);           // temp
  

  

  /*light intensity sensor */
  sensors_event_t event;    //where the sensor data will be stored
  tsl.getEvent(&event);

  /* Display the results (light is measured in lux) */
  if (event.light)
  {
    Serial.print("light = ");
    Serial.print(event.light);
    Serial.println(" lux");
    ThingSpeak.setField(3 , event.light);
  }
  else
  {
    /* If event.light = 0 lux the sensor is probably saturated9and no reliable data could be generated! */
    Serial.println("Sensor overload");
  }
  Serial.println(" ");

  /*pressure sensor*/
  // If you want sea-level-compensated pressure, as used in weather reports,
  // you will need to know the altitude at which your measurements are taken.
  // We're using a constant called ALTITUDE in this sketch:

  Serial.print("provided altitude: ");
  Serial.print(ALTITUDE, 0);  //0 indicates digits after decimal point
  Serial.print(" meters, ");
  Serial.print(ALTITUDE * 3.28084, 0);
  Serial.println(" feet");
 

  // If you want to measure altitude, and not pressure, you will instead need
  // to provide a known baseline pressure. This is shown at the end of the sketch.

  // You must first get a temperature measurement to perform a pressure reading.

  // Start a temperature measurement:
  // If request is successful, the number of ms to wait is returned.
  // If request is unsuccessful, 0 is returned.

  status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Function returns 1 if successful, 0 if failure.

    status = pressure.getTemperature(T);
    if (status != 0)
    {
      // Print out the measurement:
      Serial.print("temperature: ");
      Serial.print(T, 2);
      Serial.print(" deg C, ");
      Serial.print((9.0 / 5.0)*T + 32.0, 2);
      Serial.println(" deg F");

      // Start a pressure measurement:
      // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
      // If request is successful, the number of ms to wait is returned.
      // If request is unsuccessful, 0 is returned.

      status = pressure.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = pressure.getPressure(P, T);
        if (status != 0)
        {
          float x = P;
          // Print out the measurement:
          Serial.print("Absolute Pressure: ");
          //Serial.print(P, 2);
          Serial.print(x);
          Serial.print(" mb, ");
          Serial.print(P * 0.0295333727, 2);
          Serial.println(" inHg");
          
          ThingSpeak.setField(4 , x );                         /************************************************/

          // The pressure sensor returns absolute pressure, which varies with altitude.
          // To remove the effects of altitude, use the sea level function and your current altitude.
          // This number is commonly used in weather reports.
          // Parameters: P = absolute pressure in mb, ALTITUDE = current altitude in m.
          // Result: p0 = sea-level compensated pressure in mb

          p0 = pressure.sealevel(P, ALTITUDE); // we're at 1655 meters (Boulder, CO)
          float y = p0;
          Serial.print("relative (sea-level) pressure: ");
          //Serial.print(p0, 2);
          Serial.print(y);
          Serial.print(" mb, ");
          Serial.print(p0 * 0.0295333727, 2);
          Serial.println(" inHg");
          ThingSpeak.setField(5 , y);     /************************************/

          // On the other hand, if you want to determine your altitude from the pressure reading,
          // use the altitude function along with a baseline pressure (sea-level or other).
          // Parameters: P = absolute pressure in mb, p0 = baseline pressure in mb.
          // Result: a = altitude in m.

          a = pressure.altitude(P, p0);
          float z = a;
          Serial.print("computed altitude: ");
          //Serial.print(a, 0);
          Serial.print(z);
          Serial.print(" meters, ");
          Serial.print(a * 3.28084, 0);
          Serial.println(" feet");
          ThingSpeak.setField(6 , z);    /**********************************/
        }
        else Serial.println("error retrieving pressure measurement\n");
      }
      else Serial.println("error starting pressure measurement\n");
    }
    else Serial.println("error retrieving temperature measurement\n");
  }
  else Serial.println("error starting temperature measurement\n");


  ThingSpeak.writeFields(counterChannelNumber , myCounterWriteAPIKey);          // Send All value to thinkSpeak Server
  
  Serial.println("Done! ");
  Serial.println(" ");

  delay(5000);  // Pause for 2 seconds.
}
