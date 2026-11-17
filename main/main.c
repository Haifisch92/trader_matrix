#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi_manager.h"
#include "esp_server.h"
#include "esp_log.h"
#include "hub75_wrapper.h"
#include "stockprice.h"
#include "loghi.h"
#include <time.h>
#include <sys/time.h>

#define MATRIX_H 32
#define MATRIX_W 64
#define CHART_H  20 

bool ap_mode = false;

void set_time(){
    struct tm tm = {
        .tm_year = 2026 - 1900, //anni trascorsi dal 1900 (2026 - 1900 = 126)
        .tm_mon  = 11 - 10,     //mese da 0 a 11 (novembre = 10)
        .tm_mday = 15,          //giorno del mese 1 - 31
        .tm_hour = 16,
        .tm_min  = 24,
        .tm_sec  = 0 
    };

    time_t t = mktime(&tm);

    struct timeval now = {.tv_sec = t};
    settimeofday(&now, NULL);
    ESP_LOGI("main","orario impostato correttamente");
}
static void wifi_credential_handler(const char *ssid, const char *password){

    ESP_LOGI("MAIN", "CALLBACK ATTIVATA NEL MAIN");
    ESP_LOGI("MAIN", "SSID aggiornato %s", ssid);
    ESP_LOGI("MAIN", "PASSWORD aggiornata %s", password);

    //wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(5000));
    //wifi_init_sta(ssid, password);     
    //ap_mode = false;
}

void slotmachine_clock(){
    //i gfx hanno le cordinate del disegno caratteri che partono dal basso 
    //i disegni normali o le clear area partono dall alto 
    //bisogna gestire questa cosa e oltretutto c'è una differenza di 1 pixel
    //probabilemte perche i gfx hanno dei delta x e delta y per ogni carattere
    matrix_clear_area(13, 35, 10, 27); 
    
    draw_gfxtext(13, 20, "1", 0, 64, 0, 32); 
    draw_gfxtext(16, 20, "8", 0, 64, 0, 32); 
    draw_gfxtext(22, 20, ":", 0, 64, 0, 32);
    draw_gfxtext(24, 20, "3", 0, 64, 0, 32);  
    draw_gfxtext(30, 20, "6", 0, 64, 0, 32); 
    for(int offset = 0; offset <= 7; offset++){
        matrix_clear_area(30, 35, 8 + offset, 15 + offset);  
         
        draw_gfxtext(30, (20 - 7) + offset, "7", 0, 64, 0, 32); 
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }

}
void main_task(){
    matrix_init();

    //draw_chart("EURUSD", eurusd, sizeof(eurusd), CHART_H, MATRIX_W, 3);

    // orario ammesso da pixel x 19 a pixel x 23
    // area in cui il mappamondo non viene disegnato
    scanning_logo(_WORLD_MAP, 120, 56, MATRIX_W, MATRIX_H);
    //draw_gfxtext(20, 19, "12:30", 0, 64, 0, 32);
    int last_minute = -1;
    char last_strtime[64] = "";


    while(1){

        //gestione orario
        time_t now;
        struct tm timeinfo;

        char strtime[64];

        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(strtime, sizeof(strtime), "%H:%M", &timeinfo);
        //ESP_LOGI("main","ora esatta: %s", strtime);

        if(timeinfo.tm_min != last_minute){
            scanning_clock(_WORLD_MAP, 120, 56, MATRIX_W, MATRIX_H, last_strtime, strtime);
            last_minute = timeinfo.tm_min;
            strcpy(last_strtime, strtime);

        }

        //matrix_clear();
        
        //draw_gfxtext(0, 30, "EUR USD  1.1109", 0, 64, 0, 32);
        //draw_chart(eurusd, sizeof(eurusd), MATRIX_H - 1, MATRIX_W, 0);
        /*
        matrix_clear_area(0, 64, 26, 32);

        //accetta solo caratteri maiuscoli
        //deve avere uno spazio finale 
        // i caratteri + e meno definiscono il colore rosso o verde e non vengono mai printati
        char testo[] = "-NASDAQ <3% +EURUSD >2% -XAUUSD <1% -GBPUSD <27% +SP500 >10% ";
        wallstreet_text(testo, 0, 26);
        */
        
    
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


void app_main(void) {
    set_time();
    wifi_prepare();
    wifi_init_softap();
    vTaskDelay(pdMS_TO_TICKS(1000));
    ap_mode = true;

    start_webserver();
    set_wifi_callback(wifi_credential_handler);
    TaskHandle_t maintask_handle = NULL;
    xTaskCreate(main_task, "main_task", 4092, NULL, 5, &maintask_handle);

}