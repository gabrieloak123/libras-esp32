#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_camera.h"

#include "SharedData.h"
#include "WiFi.h"
#include "InferencerTask.h"
#include "WebServerTask.h"

// ############# MODELO DA CAMERA #############
//#define CAMERA_MODEL_AI_THINKER
#define CAMERA_MODEL_ESP32S3_EYE
#include "camera_pins.h"

// ############# MODO DO WIFI #############
#define STA_MODE false

// ############# OUTRAS CONSTANTES #############
#define MAXCLIENTSESP32 4
static const char *TAG = "app_main";

extern "C" void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    //esp_log_level_set("*", ESP_LOG_NONE);
    ESP_LOGI(TAG, "app_main start");
    ESP_LOGI(TAG, "iniciando as variaveis compartilhadas...");
    shared_buffer_init();

    ESP_LOGI(TAG, "iniciando a rede...");
#if defined(STA_MODE) && STA_MODE
    WiFi.init(WIFI_MODE_STA_ONLY_CONFIG);
    // ############### Modo STA ###############
    const char* ssid = "NPITI-IoT";
    const char* password = "NPITI-IoT";
    ESP_LOGI(TAG, "Solicitando conexão WiFi para SSID='%s'...", ssid);

    WiFi.begin(ssid, password);
    const TickType_t wait_step = pdMS_TO_TICKS(200);
    const TickType_t max_wait = pdMS_TO_TICKS(10000);
    TickType_t waited = 0;
    ESP_LOGI(TAG, "conectando...");
    while (!WiFi.isConnected() && waited < max_wait) {
        vTaskDelay(wait_step);
        waited += wait_step;
    }
    if (WiFi.isConnected()) {
        ESP_LOGI(TAG, "WiFi conectado. IP: %s", WiFi.localIP().c_str());
    } else {
        ESP_LOGW(TAG, "WiFi não conectou dentro do timeout; continue mesmo assim (podemos funcionar em AP ou fallback).");
    }
#else
    WiFi.init(WIFI_MODE_AP_ONLY_CONFIG);
    const char* ap_ssid = "meu-esp32";
    const char* ap_password = "12345678";
    ESP_LOGI(TAG, "WiFi AP com SSID='%s'...", ap_ssid);
    // ############### Modo AP ###############
    WiFi.softAP(ap_ssid, ap_password, MAXCLIENTSESP32);
    ESP_LOGI(TAG, "AP ativo em IP: %s", WiFi.APIP().c_str());
#endif
    camera_config_t config = {};
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_240X240;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    ESP_LOGI(TAG, "Inicializando camera...");
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar camera (0x%x). Abortando.", err);
       shared_buffer_deinit();
        return;
    }

    ESP_LOGI(TAG, "Iniciando InferencerTask...");
    inference_task_start(1);
    ESP_LOGI(TAG, "Inferencia iniciada");
    ESP_LOGI(TAG, "Inicializando webserver...");
    webServer_Task_start(0);
    ESP_LOGI(TAG, "webserver Iniciado...");
    ESP_LOGI(TAG, "Setup finalizado — tasks rodando.");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
