#include <rpcWiFi.h>
#include <AWS_IOT.h>
#include <Wire.h>
#include <SPI.h>
#include "TFT_eSPI.h"
#include "MLX90640_API.h"
#include "MLX9064X_I2C_Driver.h"
#include <camper_inferencing.h>
#include "config.h"

#define AWS_PUSH_INTERVAL_IN_SECONDS 60

AWS_IOT camper;
char payload[500];
String label = "Background";
float score = 0.0;
long lastSent = 0;

float features[768];

//float data[768];

int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

#define TA_SHIFT 8 //Default shift for MLX90640 in open air

paramsMLX90640 mlx90640;
TFT_eSPI tft;

const byte MLX90640_address = 0x33;
static float mlx90640To[768];

void subscriberCallBackHandler (char *topicName, int payloadLen, char *rcvdPayload)
{
  Serial.print("Message received:");
  Serial.println(rcvdPayload);
  

}

void setup() {
  lastSent = millis();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Wire.begin();
  Wire.setClock(400000);  //Increase I2C clock speed to 400kHz

  Serial.begin(115200);
  delay(1000);
  // while (!Serial); //Wait for user to open terminal

  if (isConnected() == false) {
    Serial.println("MLX90640 not detected at default I2C address. Please check wiring. Freezing.");
    while (1)
      ;
  }
  Serial.println("MLX90640 online!");

  //Get device parameters - We only have to do this once
  int status;
  uint16_t eeMLX90640[832];
  status = MLX90640_DumpEE(MLX90640_address, eeMLX90640);
  if (status != 0)
    Serial.println("Failed to load system parameters");

  status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
  if (status != 0)
    Serial.println("Parameter extraction failed");

  //Once params are extracted, we can release eeMLX90640 array
  tft.begin();
  tft.setRotation(3);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Connecting to WiFi...", 20, 120);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
  
  tft.fillScreen(TFT_BLACK);  

  if (camper.connect(HOST_ADDRESS, CLIENT_ID,
                         aws_root_ca_pem, certificate_pem_crt, private_pem_key) == 0)
  {
    Serial.println("Connected to AWS");
    tft.fillScreen(TFT_BLACK);
    tft.drawString("Connected to AWS IOT", 20, 120);
    delay(5000);

    if (0 == camper.subscribe(ALERT_TOPIC_NAME, subscriberCallBackHandler))
    {
      Serial.println("Subscribe Successfull");
    }
    else
    {
      Serial.println("Subscribe Failed, Check the Thing Name and Certificates. RESET.");
      while (1);
    }
  }
  else
  {
    Serial.println("AWS connection failed, Check the HOST Address. RESET.");
    tft.fillScreen(TFT_BLACK);
    tft.drawString("Failed to connect to AWS IOT", 20, 120);
    while (1);
  }

  pinMode(WIO_BUZZER, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(5000);
}

void loop() {
  for (byte x = 0; x < 2; x++)  //Read both subpages
  {
    uint16_t mlx90640Frame[834];
    int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);
    if (status < 0) {
      Serial.print("GetFrame Error: ");
      Serial.println(status);
    }

    float vdd = MLX90640_GetVdd(mlx90640Frame, &mlx90640);
    float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);

    float tr = Ta - TA_SHIFT;  //Reflected temperature based on the sensor ambient temperature
    float emissivity = 0.95;

    MLX90640_CalculateTo(mlx90640Frame, &mlx90640, emissivity, tr, mlx90640To);
  }

  uint32_t color;
  uint8_t label_id = 0;
  //A - Animal , B- Human , C - Background


  String data;
  int counter = 0;

  for (uint8_t x = 0; x < 32; x++) {
    for (uint8_t y = 0; y < 24; y++) {
      if (x == 0 && y == 0) {
        data = mlx90640To[24 * x + y];
      } else {
        data = data + "," + mlx90640To[24 * x + y];
      }
      features[counter++] = mlx90640To[24 * x + y];
      

      float val = mlx90640To[32 * (23 - y) + x];


      if (val > 99.99) val = 99.99;

      if (val > 32.0) {
        color = TFT_MAGENTA;
      } else if (val > 29.0) {
        color = TFT_RED;
      } else if (val > 26.0) {
        color = TFT_YELLOW;
      } else if (val > 20.0) {
        color = TFT_GREENYELLOW;
      } else if (val > 17.0) {
        color = TFT_GREEN;
      } else if (val > 10.0) {
        color = TFT_CYAN;
      } else {
        color = TFT_BLUE;
      }

      tft.fillRect(x * 10, y * 10, 10, 10, color);
    }
  }
  //delay(500);
  ei_printf("Counter %d \n", counter);
  ei_printf("Edge Impulse standalone inferencing (Arduino)\n");

  if (sizeof(features) / sizeof(float) != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
    ei_printf("The size of your 'features' array is not correct. Expected %lu items, but had %lu\n",
              EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, sizeof(features) / sizeof(float));
    delay(1000);
    return;
  }
  ei_impulse_result_t result = { 0 };

  // the features are stored into flash, and we don't want to load everything into RAM
  signal_t features_signal;
  features_signal.total_length = sizeof(features) / sizeof(features[0]);
  features_signal.get_data = &raw_feature_get_data;

  // invoke the impulse
  EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false /* debug */);
  ei_printf("run_classifier returned: %d\n", res);

  if (res != 0) return;

  // print the predictions
  score = 0.0;
  //ei_printf("Predictions ");
  

  // human-readable predictions
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    //ei_printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
    if ( result.classification[ix].value >= score) {
            score = result.classification[ix].value;
            label = result.classification[ix].label;
    }
  }

  if (label != "Background"){
      analogWrite(WIO_BUZZER, 128);
      delay(2000);
      analogWrite(WIO_BUZZER, 0); 
  }
  ei_printf("Predicted label = %s with score %f\n", label.c_str(), score);
  delay(1000);
  if(millis()-lastSent > AWS_PUSH_INTERVAL_IN_SECONDS*1000){
    lastSent  = millis(); 
    publishToAWS(); 
  }
  
  
}

//Returns true if the MLX90640 is detected on the I2C bus
boolean isConnected() {
  Wire.beginTransmission((uint8_t)MLX90640_address);
  if (Wire.endTransmission() != 0)
    return (false);  //Sensor did not ACK
  return (true);
}

void ei_printf(const char *format, ...) {
  static char print_buf[1024] = { 0 };

  va_list args;
  va_start(args, format);
  int r = vsnprintf(print_buf, sizeof(print_buf), format, args);
  va_end(args);

  if (r > 0) {
    Serial.write(print_buf);
  }
}

void publishToAWS() {
  sprintf(payload, "{\"mac\":\"%s\",\"label\":\"%s\",\"score\":%f}", WiFi.macAddress().c_str() ,  label.c_str(), score);
  Serial.println(payload);

  
  int rc = camper.publish(TOPIC_NAME, payload);  
  if (rc == 0)
  {
    Serial.println("Published Message to AWS IoT:");
    
  }
  else
  {
    Serial.printf("Publish failed with rc=%d\n",rc);
  }
}


