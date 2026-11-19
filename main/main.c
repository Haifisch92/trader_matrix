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

// -1 poichè i pixel della matrice vengono indicizzati da 0  0 -> 63 = 64 pixel utilizzati
#define MATRIX_H 32 - 1
#define MATRIX_W 64 - 1

bool ap_mode = false;
int wallstreet_pos = 0;
int wallstreet1_pos = 64;
bool update_face = false;
int matrix_face = 0;


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

static void matrix_face_handler(const char *view){

    ESP_LOGI("MAIN", "CALLBACK ATTIVATA NEL MAIN");
    ESP_LOGI("MAIN", "VIEW aggiornato %s", view);

    if(strcmp(view, "clock_view") == 0){
        update_face = true;
        matrix_face = 0;
    }else if(strcmp(view, "wallstreet_view") == 0){
        update_face = true;
        matrix_face = 1;
    }else if(strcmp(view, "chart_view") == 0){
        update_face = true;
        matrix_face = 2;
    }

}

/*
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

}*/

void clock_face(){
    uint16_t weak_white = 0x31a6;
    scanning_logo(_WORLD_MAP, 120, 56, 0, MATRIX_W, 0, MATRIX_H, 0);   
}

void wallstreet_face(int wallstreet_mode, bool update){
        
        if(wallstreet_mode == 0 || wallstreet_mode == 2) matrix_clear_area(0, MATRIX_W, 26, MATRIX_H);
        if(wallstreet_mode == 1 || wallstreet_mode == 2) matrix_clear_area(0, MATRIX_W, 0, 7);
        //accetta solo caratteri maiuscoli
        //deve avere uno spazio finale 
        // i caratteri + e meno definiscono il colore rosso o verde e non vengono mai printati
        char testo[] = "-NASDAQ <3% +EURUSD >2% -XAUUSD <1% -GBPUSD <27% +SP500 >10% ";

        if(wallstreet_mode == 0 || wallstreet_mode == 2)
            wallstreet_pos = wallstreet_text(testo, wallstreet_pos, 26);

        if(wallstreet_mode == 1 || wallstreet_mode == 2)
            wallstreet1_pos = wallstreet_text(testo, wallstreet1_pos, 0);

        if(update){
            switch(wallstreet_mode){
                case 0:
                    scanning_logo(_WORLD_MAP, 120, 56, 0, MATRIX_W, 0, MATRIX_H - 7, 0);
                    break;
                case 1:
                    scanning_logo(_WORLD_MAP, 120, 56, 0, MATRIX_W, 7, MATRIX_H, 0);
                    break;
                case 2:
                    scanning_logo(_WORLD_MAP, 120, 56, 0, MATRIX_W, 7, MATRIX_H - 7, 0);
                    break;
            }
        }
}

void chart_face(){
    draw_text(0,0, "EURUSD 1.1097",0, 255, 0, 125);
    draw_chart(eurusd, sizeof(eurusd), 20, MATRIX_W, (MATRIX_H - 20) - 1, 0);
}

void main_task(){

    int last_minute = -1;
    char last_strtime[64] = "";
    clock_face();

    while(1){
        
        //gestione orario
        time_t now;
        struct tm timeinfo;

        char strtime[64];

        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(strtime, sizeof(strtime), "%H:%M", &timeinfo);

        if(timeinfo.tm_min != last_minute){
            if(matrix_face == 0 || matrix_face == 1){
                update_clock(last_strtime, strtime, 18, 14 , 1);
            }
            last_minute = timeinfo.tm_min;
            strcpy(last_strtime, strtime);

        }

        switch(matrix_face){
            case 0:
                if(update_face){
                    matrix_clear();
                    last_minute = -1;
                    clock_face();
                    update_face = false;
                }
                
                break;
            case 1:
                if(update_face){
                    matrix_clear();
                    last_minute = -1;
                }
                wallstreet_face(2, update_face);
                update_face = false;
                break;
            case 2:
                if(update_face){
                    matrix_clear();
                    update_face = false;
                }
                chart_face();
                break;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


void app_main(void) {
    set_time();
    wifi_prepare();
    wifi_init_softap();
    vTaskDelay(pdMS_TO_TICKS(1000));
    ap_mode = true;

    start_webserver();
    matrix_init();

    set_wifi_callback(wifi_credential_handler);
    set_matrixface_callback(matrix_face_handler);

    TaskHandle_t maintask_handle = NULL;
    xTaskCreate(main_task, "main_task", 4092, NULL, 5, &maintask_handle);

}