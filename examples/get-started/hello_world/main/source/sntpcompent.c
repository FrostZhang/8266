
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sntpcompent.h"

static const char *TAG = "sntp";
static time_t now = 0;
struct tm timeinfo = {0};

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "ntp.aliyun.com");
    sntp_setservername(1, "cn.pool.ntp.org");
    sntp_setservername(2, "time.windows.com");
    sntp_init();
}

static void obtain_time(void)
{
    initialize_sntp();
    int retry = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count)
    {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}

static void sntp_task(void *arg)
{
    sntp_event_callback_t callback = arg;
    sntp_event event_t;
    char strftime_buf[64];

    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900))
    {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
    }

    // Set timezone to Eastern Standard Time and print local time
    //setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    //tzset();

    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();

    // update 'now' variable with current time
    time(&now);
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_year < (2016 - 1900))
    {
        ESP_LOGE(TAG, "The current date/time error");
        event_t.mestype = SNTP_EVENT_CONNNECTFAILED;
        callback(&event_t);
    }
    else
    {
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
        //ESP_LOGI(TAG, "Free heap size: %d\n", esp_get_free_heap_size());
        event_t.timeinfo = &timeinfo;
        event_t.mestype = SNTP_EVENT_SUCCESS;
        callback(&event_t);
        event_t.mestype = SNTP_EVENT_TIMING;
        while (true)
        {
            time(&now);
            localtime_r(&now, &timeinfo);
            // strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
            // ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
            //system_sntp_callback(&timeinfo);
            callback(&event_t);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
    vTaskDelete(NULL);
}

//获取时间 也是在检测是否能连接互联网
extern void sntp_start(sntp_event_callback_t event_handle)
{
    xTaskCreate(sntp_task, "sntptask", 2048, event_handle, 8, NULL);
}
