#include <stdio.h>
#include "hub75_wrapper.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "driver/gpio.h"
#include "fontmatrix.h"
#include "small4pt7b.h"


MatrixPanel_I2S_DMA *matrix = nullptr;


void matrix_init() {
    // 1. Configurazione Matrix DMA
    HUB75_I2S_CFG mxconfig(64, 32, 1);

    mxconfig.clkphase = false; 
    
    // Mappatura manuale dei pin
    mxconfig.gpio.r1 = R1_PIN; mxconfig.gpio.g1 = G1_PIN; mxconfig.gpio.b1 = B1_PIN;
    mxconfig.gpio.r2 = R2_PIN; mxconfig.gpio.g2 = G2_PIN; mxconfig.gpio.b2 = B2_PIN;
    mxconfig.gpio.a = A_PIN; mxconfig.gpio.b = B_PIN; mxconfig.gpio.c = C_PIN; mxconfig.gpio.d = D_PIN;
    mxconfig.gpio.e = E_PIN;
    mxconfig.gpio.clk = CLK_PIN; mxconfig.gpio.lat = LAT_PIN; mxconfig.gpio.oe = OE_PIN;

    // 2. Inizializzazione
    matrix = new MatrixPanel_I2S_DMA(mxconfig);
    if (matrix->begin()) {
        printf("Matrice inizializzata correttamente!\n");
    }

}

int matrix_clear_area(int x1, int x2, int y1, int y2) {

    if(!matrix) return -1;

    //pulizia solo di una specifica area 
    for(int x = x1; x <= x2; x++){
        for(int y = y1; y <= y2; y ++){
            matrix->drawPixel(x, y, matrix->color565(0, 0, 0));
        }
    }

    return 0;
}

void matrix_clear() {
    if(matrix) matrix->fillScreen(0);
}

void matrix_set_pixel(int x, int y, uint16_t color) {
    if(matrix) matrix->drawPixel(x, y, color);
}


void draw_char(int x, int y, char c, uint16_t color) {
    if (c < 32 || c > 126) return; // Fuori range ASCII gestito

    int index = c - 32; // Calcola l'indice partendo dallo spazio

    for (int col = 0; col < 4; col++) {
        // Prendo colonna per colonna (8 bit) del char
        uint8_t col_data = font4x6[index][col]; 
        
        for (int row = 0; row < 8; row++) {
            // Se il bit alla posizione 'row' è 1, accendi il pixel
            if (col_data & (1 << row)) {
                matrix->drawPixel(x + col, y + row, color);
            }
        }
    }
}

// Funzione per stampare una stringa intera
void draw_text(int x, int y, const char* text, int r, int g, int b, int brightness) {
    int text_x = x;

    uint8_t sr = (r * brightness) >> 8;
    uint8_t sg = (g * brightness) >> 8;
    uint8_t sb = (b * brightness) >> 8;

    uint16_t color = matrix->color565(sr, sg, sb);

    while (*text) {
        draw_char(text_x, y, *text, color);

        // definisco la x per il carattere successivo
        // 4 pixel del carattere + 1 pixel di spazio, tranne per la lettera I che occupa meno
        if(*text == 'I')
            text_x += 4;
        else
            text_x += 5;

        text++;
    }
}

// Funzione per disegnare un logo RGB565 sulla matrice
// la funzione accetta immagini anche più gradi della matrice ed è possibe definirgli un area
// della matrice in cui è ammesso il disegno , inoltre si puo avere uno scostamento orizzontale
// e verticale dell immagine... i primi parametri width height e logo_array sono relativi
// all immagine, start_x start_y end_x end_y definiscono l area di ritaglio o area della matrice
// in cui è ammesso disegnare, gli ultimi due parametri invece definiscono lo scostamento. 
void draw_logo(int width, int height, const uint16_t *logo_array,
                int start_x, int end_x, int start_y, int end_y, 
                int offset_x, int offset_y) {

    for (int y = 0; y < height; y++) {

        for (int x = 0; x < width; x++) {
            // Legge il colore del pixel corrente dall'array
            // moltiplicando la larghezza dell immagine alla y corrente
            // e aggiungendogli la x
            uint16_t pixel_color = logo_array[y * width + x];
            int pixel_x = offset_x + x;
            int pixel_y = offset_y + y;
            bool x_area = pixel_x >= start_x && pixel_x <= end_x;
            bool y_area = pixel_y >= start_y && pixel_y <= end_y;
            // Se il pixel non è nero, lo disegna
            
            if (pixel_color != 0x0000 && x_area && y_area){
                matrix->drawPixel(pixel_x, pixel_y, pixel_color);
            }
        }
    }
}


int gfxtext_len(const char *text) {


    int len = 0;
    const GFXfont *gfx_font = &small4pt7b;

    while (*text) {
        unsigned char c = *text;
        if (c >= gfx_font->first && c <= gfx_font->last) {

            // cosi ottengo l indice 
            c -= gfx_font->first;          
            // Riga corretta senza residui
            GFXglyph *glyph = &(((GFXglyph *)gfx_font->glyph)[c]);
            
            len += glyph->advance_x;
        }
        text++;
    }

    return len;
}


void draw_gfxtext(int x, int y, const char *text, int clip_x1, int clip_x2, 
                    int clip_y1, int clip_y2, uint16_t color) {
    int16_t cursor_x = x;
    int16_t cursor_y = y;
    const GFXfont *gfx_font = &small4pt7b;

    while (*text) {
        unsigned char c = *text;
        if (c >= gfx_font->first && c <= gfx_font->last) {

            // cosi ottengo l indice 
            c -= gfx_font->first;
            
            // Riga corretta senza residui
            GFXglyph *glyph = &(((GFXglyph *)gfx_font->glyph)[c]);
            
            uint8_t   w  = glyph->width;
            uint8_t   h  = glyph->height;
            int8_t    xo = glyph->d_x;
            int8_t    yo = glyph->d_y;
            uint16_t  bo = glyph->bitmapOffset;

            uint8_t bits = 0;
            uint8_t bit = 0;

            for (int yy = 0; yy < h; yy++) {
                for (int xx = 0; xx < w; xx++) {
                    if (!(bit & 7)) {
                        bits = gfx_font->bitmap[bo++];
                    }
                    //printf("%d\n",bits);
                    if (bits & 0x80) {
                        int pixel_x = cursor_x + xo + xx;
                        int pixel_y = cursor_y + yo + yy;
                        // controllo su aree di disegno consentite
                        bool check_y =  pixel_y >= clip_y1 && pixel_y <= clip_y2;
                        bool check_x = pixel_x >= clip_x1 && pixel_x <= clip_x2;
                        
                        if(check_x && check_y)
                            matrix->drawPixel(pixel_x, pixel_y, color);
                    }
                    bits <<= 1;
                    bit++;
                }
            }
            cursor_x += glyph->advance_x;
        }
        text++;
    }
}


void update_clock(char *old_time, char *new_time, int clock_start_x, int clock_start_y, int clock_mode) {

    // area in pixel tolgo 1 perchè è tutto indicizzato da 0 
    int clock_area_width = 25 - 1;
    int clock_area_height = 5 - 1;
    int colon_x = clock_area_width / 2;
    uint16_t white = 0x4a49;

    int clock_end_x = clock_start_x + clock_area_width;
    int clock_end_y = clock_start_y + clock_area_height;

    // i due punti : dell orario devono avere poszione fissa a una determinata x
    // trovo quanti pixel usa la parte dell ora e definisco la x dell orario completo, 
    // scostandomi verso sinistra dai : di quanti pixel usa l ora 

    char old_hour[3];
    char old_copy[10];
    strncpy(old_copy, old_time, sizeof(old_copy) - 1);

    if(!sscanf(old_copy, "%2[^:]", old_hour))
        ESP_LOGI("matrix", "errore sscanf nell estrarre l ora");
    int old_hour_len = gfxtext_len(old_hour);

    char new_hour[3];
    char new_copy[10];
    strncpy(new_copy, new_time, sizeof(new_copy) - 1);

    if(!sscanf(new_copy, "%2[^:]", new_hour))
        ESP_LOGI("matrix", "errore sscanf nell estrarre l ora");
    int new_hour_len = gfxtext_len(new_hour);


    int old_time_x = (clock_start_x + colon_x) - old_hour_len;
    int new_time_x = (clock_start_x + colon_x) - new_hour_len;


    for(int s = clock_start_x; s <= clock_end_x; s++){
        matrix_clear_area(clock_start_x, clock_end_x, clock_start_y, clock_end_y);
        // i gfx text hanno la base del carattere sulla cordinata x
        // al contrario degli altri text che hanno la vetta del carattere sulla x
        draw_gfxtext(old_time_x, clock_end_y, old_time, s, clock_end_x, clock_start_y, clock_end_y, white);
        vTaskDelay(pdMS_TO_TICKS(25));  
    }

    for(int s = clock_end_x; s >= clock_start_x; s--){
        matrix_clear_area(clock_start_x, clock_end_x, clock_start_y, clock_end_y);   
        draw_gfxtext(new_time_x, clock_end_y, new_time, s, clock_end_x, clock_start_y, clock_end_y, white);
        vTaskDelay(pdMS_TO_TICKS(25));
        
    }


}

void scanning_logo(const uint16_t *logo_array, int logo_width, int logo_height, 
                    int start_x, int end_x, int start_y, int end_y, uint16_t scanner_color) {

    int target = -1;

    // avanzamento della linea scanner da 0 fino alla larghezza dello schermo
    for (int scanner_x = 0; scanner_x <= end_x; scanner_x++) {   
        matrix_clear_area(start_x, end_x, start_y, end_y);

        // disegna la mappa colonna per colonna, ma SOLO fino a scanner_x
        draw_logo(logo_width, logo_height, logo_array, start_x, scanner_x, start_y, end_y, -10, 0);

        if(scanner_color != 0x0000)
            matrix->drawFastVLine(scanner_x, start_y, (end_y - start_y), scanner_color);

        vTaskDelay(pdMS_TO_TICKS(25));
    }

    if(scanner_color != 0x0000){
        for (int scanner_x = end_x; scanner_x >= target; scanner_x--) {
            matrix_clear_area(start_x, end_x, start_y, end_y);

            // disegna l'intera mappa fissa sullo sfondo
            draw_logo(logo_width, logo_height, logo_array, start_x, end_x, start_y, end_y, -10, 0);

            // disegna la riga che arretra verso la time zone
            matrix->drawFastVLine(scanner_x, start_y, (end_y - start_y), scanner_color);
            //draw_gfxtext(20, 19, "12:30", scanner_x, 64, 0, 32);

            vTaskDelay(pdMS_TO_TICKS(25));
        }
    }

}


int ws_text_draw(const char* text, const int x, int y) {

    // colori di default al verde cosi se uno dimentica di impostarli almeno si vede
    int i = 0, r = 0, g = 255, b = 0, char_x = x;
    int brightness = 240;

    // primo ciclo di stampa caratteri sulla matrice
    while(text[i] != '\0'){

        if(text[i] == '+'){
            r = 0;
            g = 255;
            b = 0;
        }

        if(text[i] == '-'){
            r = 255;
            g = 0;
            b = 0;
        }

        if(text[i] != ' ' && text[i] != '+' && text[i] != '-'){
            uint8_t sr = (r * brightness) >> 8;
            uint8_t sg = (g * brightness) >> 8;
            uint8_t sb = (b * brightness) >> 8;

            uint16_t color = matrix->color565(sr, sg, sb);
            draw_char(char_x, y, text[i], color);
        }

        //scostamento della x tra un carattere e un altro
        //5 pixel per ogni carattere + 1 di spazio
        //gli spazi tra una parola e un altra li faccio di 2 pixel
        //metto due pixel di spazio anche coi definitori dei colori
        //così danno ulteriore spaziatura tra un asset e un altro
        if(text[i] == ' ' || text[i] == '+' || text[i] == '-')
            char_x += 2;
        else
            char_x += 5; 

        i++;
    }

    int text_len = char_x - x;
    return text_len;
}

int wallstreet_text(const char* text, int x, int y) {
    // posizione della x del testo diminuisce a ogni chiamata di func 
    int scroll_x = x;

    // primo ciclo di stampa caratteri sulla matrice
    int text_len = ws_text_draw(text, scroll_x, y);

    // secondo ciclo per dare la continuità al wall text
    ws_text_draw(text, scroll_x + text_len, y);

    scroll_x--;
    if(scroll_x <= -(text_len)) scroll_x = 0;

    return scroll_x;

}

void draw_chart(float price[][4], int size, int chart_h, int chart_w, 
                            int top_padding, int chart_mode) {
    
    //divido 4 perchè so che l array multidimensionale contiene 4 elementi ciascuno 
    //da 4 byte (float = 4byte)
    //size è la dimensione totale dell array diviso (4) la grandezza dei float 
    //ho il numero totale di elementi diviso ulteriormente 4 perchè so che ogni indice contiene 4
    //elementi ottengo il numero degli indici
    //i prezzi sono ordinati con ordine open high low close
    int total_data = (size / 4) / 4;

    int i;
    float max = price[0][0], min = price[0][0];

    for(i = 0; i < total_data; i++){
        float hight = price[i][1];
        float low = price[i][2];

        if(hight > max)
            max = hight;

        if(low < min)
            min = low;
    }

    float range = max - min;

    //FUNC draw_graph()
    int padding_y = top_padding;
    //dimension è la nuova scala in pixel, l ampiezza sull asse y
    //normalizzo il valore e lo rapporto alla nuova dimension
    int dimension = chart_h;
    int lowest_y = chart_h;
    int lowest_x = 0;

    for(int pixel = chart_w; pixel >= 0; pixel--){
        //siccome la matrice e il buffer dei dati non sono cooncordi
        //la matrice ha il pixel 0 a sinistra e il buffer dati ha l ultimo dato all index 0
        //reverso l index per il buffer dati
        int index_data = chart_w - pixel;

        float close = price[index_data][3];
        float normalized_value = (close - min) / range;
        int y = normalized_value * dimension;

        //trovo la x del punto piu basso nel grafico 
        //e li mettero il titolo del grafico
        if(y < lowest_y){
            lowest_y = y;
            lowest_x = pixel;
        }

        //siccome l asse y sulla matrice parte dall alto, quindi y = 0 è il punto piu alto
        //devo reversare pure il valore y
        y = (dimension + padding_y) - y;

        if(chart_mode == 0){
            matrix_set_pixel(pixel, y, matrix->color565(0, 255, 0));   
        }else if(chart_mode == 1){
            uint8_t brightness = 0;
            for(int y2 = chart_h; y2 >= y; y2--){
                if(y2 == y) brightness = 120;
                else brightness = 10;
                uint8_t sr = (0 * brightness) >> 8;
                uint8_t sg = (255 * brightness) >> 8;
                uint8_t sb = (0 * brightness) >> 8;
                //brightness += 10;
                matrix_set_pixel(pixel, y2, matrix->color565(sr, sg, sb));
            }
        }else{
            uint8_t brightness = 10;
            for(int y2 = chart_h; y2 >= y; y2--){
                uint8_t sr = (0 * brightness) >> 8;
                uint8_t sg = (255 * brightness) >> 8;
                uint8_t sb = (0 * brightness) >> 8;

                brightness += 10;
                if(brightness >= 245) brightness = 245;

                matrix_set_pixel(pixel, y2, matrix->color565(sr, sg, sb));
            }           
        }
    }
}

