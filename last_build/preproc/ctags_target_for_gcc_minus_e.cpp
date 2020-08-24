# 1 "/Users/karsten/Documents/00_Project_Support/10_arduino/Projects/sensor_hub_git/sensor_hub_git.ino"
/*
 * Hub to send sensor values via MQTT
 * BME280 for P,H,T
 * Copyright K.Schmolders 08/2020
 * Updated to show SW version on webserver /
 * Configurable via config file
 */

# 10 "/Users/karsten/Documents/00_Project_Support/10_arduino/Projects/sensor_hub_git/sensor_hub_git.ino" 2
# 11 "/Users/karsten/Documents/00_Project_Support/10_arduino/Projects/sensor_hub_git/sensor_hub_git.ino" 2
 //required for MQTT
# 13 "/Users/karsten/Documents/00_Project_Support/10_arduino/Projects/sensor_hub_git/sensor_hub_git.ino" 2
 //required for OTA updater
# 15 "/Users/karsten/Documents/00_Project_Support/10_arduino/Projects/sensor_hub_git/sensor_hub_git.ino" 2
# 16 "/Users/karsten/Documents/00_Project_Support/10_arduino/Projects/sensor_hub_git/sensor_hub_git.ino" 2
# 17 "/Users/karsten/Documents/00_Project_Support/10_arduino/Projects/sensor_hub_git/sensor_hub_git.ino" 2
# 18 "/Users/karsten/Documents/00_Project_Support/10_arduino/Projects/sensor_hub_git/sensor_hub_git.ino" 2
 //end OTA requirements
# 20 "/Users/karsten/Documents/00_Project_Support/10_arduino/Projects/sensor_hub_git/sensor_hub_git.ino" 2
# 21 "/Users/karsten/Documents/00_Project_Support/10_arduino/Projects/sensor_hub_git/sensor_hub_git.ino" 2


 // An IR detector/demodulator is connected to GPIO pin 14 on Sonoff
 uint16_t IR_SEND_PIN = 14;
 uint16_t BME_SCL = 1;
 uint16_t BME_SDA = 3;

 //timer
 int timer_update_state_count;
 int timer_update_state = 60000; //update status via MQTT every minute


 //MQTT
 WiFiClient espClient;
 PubSubClient client(espClient);
 //all wifi credential and MQTT Server importet through wifi_credential.h

 //mqtt topics imported through config.h

 //BME280

 Adafruit_BME280 bme280; // I2C
 float bme280_temperature, bme280_pressure, bme280_humidity, bme280_height;
 float bme280_temp_offset = 1.5;

 //OTA
 ESP8266WebServer httpServer(80);
 ESP8266HTTPUpdateServer httpUpdater;

 void setup_wifi() {
   delay(10);
   // We start by connecting to a WiFi network
   Serial.println();
   Serial.print("Connecting to ");
   Serial.println(ssid);
   WiFi.persistent(false);
   WiFi.mode(WIFI_OFF);
   WiFi.mode(WIFI_STA);
   WiFi.begin(ssid, password);
   while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print(".");
   }

   Serial.println("");
   Serial.println("WiFi connected");
   Serial.println("IP address: ");
   Serial.println(WiFi.localIP());

   httpUpdater.setup(&httpServer);
   httpServer.begin();
 }


 //callback function for MQTT client
 void callback(char* topic, byte* payload, unsigned int length) {
   payload[length]='\0'; // Null terminator used to terminate the char array
   String message = (char*)payload;

   Serial.print("Message arrived on topic: [");
   Serial.print(topic);
   Serial.print("]: ");
   Serial.println(message);

   //get last part of topic 
   char* cmnd = "test";
   char* cmnd_tmp=strtok(topic, "/");

   while(cmnd_tmp !=__null) {
     cmnd=cmnd_tmp; //take over last not NULL string
     cmnd_tmp=strtok(__null, "/"); //passing Null continues on string
     //Serial.println(cmnd_tmp);    
   }



   if (!strcmp(cmnd, "tset")) {
     Serial.print("Received new temperature setpoint: ");
     //dummy only.
   }
 }


 //send Sensor Values via MQTT
 void sendSensorValues(){

    char outTopic_status[50];
    char msg[50];


   //roomtemp from BME280
     strcpy(outTopic_status,outTopic);
    dtostrf(bme280_temperature,2,2,msg);
    strcat(outTopic_status,"temperature");
    client.publish(outTopic_status, msg);

   //BME280 Humidity
    strcpy(outTopic_status,outTopic);
    dtostrf(bme280_humidity,2,2,msg);
    strcat(outTopic_status,"humidity");
    client.publish(outTopic_status, msg);

    //BME280 Pressure
    strcpy(outTopic_status,outTopic);
    dtostrf(bme280_pressure,2,2,msg);
    strcat(outTopic_status,"pressure");
    client.publish(outTopic_status, msg);

     //IP Address
    strcpy(outTopic_status,outTopic);
    strcat(outTopic_status,"ip_address");
    WiFi.localIP().toString().toCharArray(msg,50);
    client.publish(outTopic_status,msg );

 }

 void reconnect() {
   // Loop until we're reconnected

   while (!client.connected()) {
     Serial.print("Attempting MQTT connection...");
     // Attempt to connect
     if (client.connect(mqtt_id)) {
       Serial.println("connected");

       client.publish(outTopic, "Sensor station booted");

       //send current Status via MQTT to world
       sendSensorValues();
       // ... and resubscribe
       client.subscribe(inTopic);

     } else {
       Serial.print("failed, rc=");
       Serial.print(client.state());
       Serial.println(" try again in 5 seconds");
       delay(5000);
     }
   }
 }

 void update_sensors() {
   bme280_temperature=bme280.readTemperature()-bme280_temp_offset; //C
   bme280_pressure=bme280.readPressure() / 100.0F; //in hPA
   bme280_humidity=bme280.readHumidity(); //%
   bme280_height=bme280.readAltitude((1013.25)); //m

 }

 void handle_root() {
   //print sw version on root
   httpServer.send(200, "text/plain", sw_version);
 }

 void setup() {
   // Status message will be sent to the PC at 115200 baud
   Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);

   //INIT TIMERS
   timer_update_state_count=millis();

   //INIT BME280
   //SDA, SCL
   Wire.begin(BME_SDA, BME_SCL);
   bool status;
   status = bme280.begin();
   if (!status) {
       Serial.println("Could not find a valid BME280 sensor, check wiring!");
       while (1);
   }
   update_sensors();

   //WIFI and MQTT
   setup_wifi(); // Connect to wifi 
   client.setServer(mqtt_server, 1883);
   client.setCallback(callback);

  //http server
  httpServer.on("/", handle_root);

 }


 void loop() {
   if (!client.connected()) {
     reconnect();
   }
   client.loop();

   update_sensors();

   //http Updater for OTA
   httpServer.handleClient();

   //send status update via MQTT every minute
   if(millis()-timer_update_state_count > timer_update_state) {
    //addLog_P(LOG_LEVEL_INFO, PSTR("Serial Timer triggerd."));
    timer_update_state_count=millis();
    sendSensorValues();

   }

 }
