#define WIFI_SSID   "SPWN_H36_719355"
#define WIFI_PASSWD "4b8597qa9i2b6ia"

//////////////////////////////////////////////////////////////////////////////////////////////

#include <Wire.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_timer.h"

//////////////////////////////////////////////////////
//#elif defined(TTGO_T_CAMERA_V05) || defined(TTGO_T_CAMERA_V1_7_2)
#define PWDN_GPIO_NUM       26
#define RESET_GPIO_NUM      -1
#define XCLK_GPIO_NUM       32
#define SIOD_GPIO_NUM       13
#define SIOC_GPIO_NUM       12

#define Y7_GPIO_NUM         39
#define Y6_GPIO_NUM         36
#define Y5_GPIO_NUM         23
#define Y4_GPIO_NUM         18
#define Y3_GPIO_NUM         15
#define Y2_GPIO_NUM         4
#define Y1_GPIO_NUM         14
#define Y0_GPIO_NUM         5
#define VSYNC_GPIO_NUM      27
#define HREF_GPIO_NUM       25
#define PCLK_GPIO_NUM       19
#define AS312_PIN           33
#define BUTTON_1            34
#define I2C_SDA             21
#define I2C_SCL             22
#define SSD130_MODLE_TYPE   GEOMETRY_128_64
//////////////////////////////////////////////////////

// OLED
#include "SSD1306.h"
#include "OLEDDisplayUi.h"
#define SSD1306_ADDRESS 0x3c
SSD1306 oled(SSD1306_ADDRESS, I2C_SDA, I2C_SCL, SSD130_MODLE_TYPE);
//OLEDDisplayUi ui(&oled);

String ip;
//EventGroupHandle_t evGroup;

//////////////////////////////////////////////////////////////////////////////////////////////
#define PART_BOUNDARY "my_boundary_string"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// handler example    https://github.com/espressif/esp32-camera
// http server example  https://gist.github.com/yoggy/f598e4d2a7f1dad26ba31c7bc6c35ab4

esp_err_t jpg_stream_httpd_handler(httpd_req_t *req)
{
    esp_err_t     res;
    camera_fb_t   *fb;
    uint8_t       *_jpg_buf;
    size_t        _jpg_buf_len;
    char          *tmp_buf[64];

  // check http request
  if ((res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE)) != ESP_OK)
        return res;

  // capture and send
    while (1) {
    if ((fb = esp_camera_fb_get()) == NULL)
            return ESP_FAIL;
        else {
          if (fb->format != PIXFORMAT_JPEG) {
              if (!frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len)) {
                    esp_camera_fb_return(fb);
                    return ESP_FAIL;
                }
          }
          else {
                _jpg_buf_len  = fb->len;
                _jpg_buf    = fb->buf;
            }
        }
        size_t hlen = snprintf((char *)tmp_buf, 64, _STREAM_PART, _jpg_buf_len);
        if ((res = httpd_resp_send_chunk(req, (const char *)tmp_buf, hlen)) != ESP_OK)
      return res;
        if ((res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len)) != ESP_OK)
      return res;
        if ((res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY))) != ESP_OK)
      return res;
        if (fb->format != PIXFORMAT_JPEG)
            free(_jpg_buf);
        esp_camera_fb_return(fb);
    } // while
    return res;
}


static esp_err_t http_server_init()
{
  httpd_handle_t  server;
//    httpd_uri_t jpeg_uri = {
//        .uri = "/jpg",
//        .method = HTTP_GET,
//        .handler = jpg_httpd_handler,
//        .user_ctx = NULL
//    };

    httpd_uri_t jpeg_stream_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = jpg_stream_httpd_handler,
        .user_ctx = NULL
    };

    httpd_config_t  http_options  = HTTPD_DEFAULT_CONFIG();

    ESP_ERROR_CHECK(httpd_start(&server, &http_options));         // start server
//    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &jpeg_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &jpeg_stream_uri));  // start streaming

    return ESP_OK;
}


//////////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
//    Serial.begin(115200);
//    Serial.setDebugOutput(true);
    pinMode(AS312_PIN, INPUT);
    
  Wire.begin(I2C_SDA, I2C_SCL);

//    if (!(evGroup = xEventGroupCreate())) {
//        Serial.println("evGroup Fail");
//        while (1);
//    }
//    xEventGroupSetBits(evGroup, 1);

    // init camera
    camera_config_t config;

    config.pin_d0           = Y0_GPIO_NUM;
    config.pin_d1           = Y1_GPIO_NUM;
    config.pin_d2           = Y2_GPIO_NUM;
    config.pin_d3           = Y3_GPIO_NUM;
    config.pin_d4           = Y4_GPIO_NUM;
    config.pin_d5           = Y5_GPIO_NUM;
    config.pin_d6           = Y6_GPIO_NUM;
    config.pin_d7           = Y7_GPIO_NUM;
    config.pin_xclk         = XCLK_GPIO_NUM;
    config.pin_pclk         = PCLK_GPIO_NUM;
    config.pin_vsync        = VSYNC_GPIO_NUM;
    config.pin_href         = HREF_GPIO_NUM;

    config.pin_sscb_sda     = SIOD_GPIO_NUM;
    config.pin_sscb_scl     = SIOC_GPIO_NUM;
    config.pin_pwdn         = PWDN_GPIO_NUM;
    config.pin_reset        = RESET_GPIO_NUM;

    config.xclk_freq_hz     = 20000000;
    config.ledc_channel     = LEDC_CHANNEL_0;
    config.ledc_timer       = LEDC_TIMER_0;

    config.pixel_format     = PIXFORMAT_JPEG;   // YUV422/GRAYSCALE/RGB565/JPEG

    // jpeg only?
    if (psramFound()) {
        config.frame_size   = FRAMESIZE_UXGA;
        config.jpeg_quality = 10;
        config.fb_count     = 2;
    } else {
        config.frame_size   = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count     = 1;
    }

    esp_err_t err   = esp_camera_init(&config); // init

    //drop down frame size for higher initial frame rate
//    sensor_t *s = esp_camera_sensor_get();
//    s->set_framesize(s, FRAMESIZE_QVGA);

    WiFi.begin(WIFI_SSID, WIFI_PASSWD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
//        Serial.print(".");
    }
//    Serial.println("");
//    Serial.println("WiFi connected");
    ip = WiFi.localIP().toString();

    // display
    int x = oled.getWidth() / 2;
    int y = oled.getHeight() / 2;
    oled.init();
    oled.setFont(ArialMT_Plain_16);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    delay(50);
    if (digitalRead(AS312_PIN)){
      oled.drawString(x, y - 20, ip);
      oled.drawString(x, y - 2, "detect!");
      oled.display();
    } else{
      oled.drawString(x, y - 20, ip);
      oled.drawString(x, y - 2, "Not detect!");
      oled.display();
    }
     
  // http server
    http_server_init();

//    startCameraServer();
//    delay(50);

//    Serial.print("Camera Ready! Use 'http://");
//    Serial.print(ip);
//    Serial.println("' to connect");
}

void loop()
{
  
//      int x = oled.getWidth() / 2;
//      int y = oled.getHeight() / 2;
//    char strbuf[100];
//
//    delay(1000);
//
////    if (frame_time > 0) {
//      sprintf(strbuf, "%d", (int)frame_time);
////        Serial.println(strbuf);
//        oled.drawString(x, y - 3, strbuf);
//        oled.display();
////    }

}

// end of file
