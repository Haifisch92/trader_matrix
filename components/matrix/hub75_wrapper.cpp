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
        for(int y = y1; y < y2; y ++){
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


void draw_char(int x, int y, char c, int r, int g, int b, bool small) {
    if (c < 32 || c > 126) return; // Fuori range ASCII gestito

    uint8_t brightness = 125;
    uint8_t sr = (r * brightness) >> 8;
    uint8_t sg = (g * brightness) >> 8;
    uint8_t sb = (b * brightness) >> 8;

    int index = c - 32; // Calcola l'indice partendo dallo spazio
    int font_lenght = small ? 3 : 4; 

    for (int col = 0; col < font_lenght; col++) {
        // Prendo colonna per colonna (8 bit) del char
        uint8_t colData = small ? font3x5[index][col] : font4x6[index][col]; 
        
        for (int row = 0; row < 8; row++) {
            // Se il bit alla posizione 'row' è 1, accendi il pixel
            if (colData & (1 << row)) {
                matrix->drawPixel(x + col, y + row, matrix->color565(sr, sg, sb));
            }
        }
    }
}

// Funzione per stampare una stringa intera
void draw_text(int x, int y, const char* text, int r, int g, int b, bool small) {
    int currentX = x;
    while (*text) {
        draw_char(currentX, y, *text, r, g, b, small);

        if(small)
            currentX += 4; // 5 pixel del carattere + 1 pixel di spazio
        else
            currentX += 6; // 5 pixel del carattere + 1 pixel di spazio
        text++;
    }
}

// Funzione per disegnare un logo RGB565 sulla matrice
void draw_logo(int start_x, int start_y, int end_x, int end_y, 
                int width, int height, const uint16_t *logo_array) {

    for (int y = 0; y < height && y < end_y; y++) {
        for (int x = 0; x < width && x < end_x; x++) {
            // Legge il colore del pixel corrente dall'array
            // moltiplicando la larghezza dell immagine alla y corrente
            // e aggiungendogli la x
            uint16_t pixel_color = logo_array[y * width + x];

            // Se il pixel non è nero, lo disegna
            if (pixel_color != 0x0000) 
                matrix->drawPixel(start_x + x, start_y + y, pixel_color);
        }
    }
}





void draw_gfxtext(int x, int y, const char *text, int clip_x1, int clip_x2, 
                    int clip_y1, int clip_y2) {
    int16_t cursorX = x;
    int16_t cursorY = y;
    const GFXfont *gfxFont = &small4pt7b;

    uint8_t brightness = 75;
    uint8_t sr = (255 * brightness) >> 8;
    uint8_t sg = (255 * brightness) >> 8;
    uint8_t sb = (255 * brightness) >> 8;

    while (*text) {
        unsigned char c = *text;
        if (c >= gfxFont->first && c <= gfxFont->last) {

            // cosi ottengo l indice 
            c -= gfxFont->first;
            
            // Riga corretta senza residui
            GFXglyph *glyph = &(((GFXglyph *)gfxFont->glyph)[c]);
            
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
                        bits = gfxFont->bitmap[bo++];
                    }
                    //printf("%d\n",bits);
                    if (bits & 0x80) {
                        int pixel_x = cursorX + xo + xx;
                        int pixel_y = cursorY + yo + yy;
                        // controllo su aree di disegno consentite
                        bool check_y =  pixel_y >= clip_y1 && pixel_y <= clip_y2;
                        bool check_x = pixel_x >= clip_x1 && pixel_x <= clip_x2;

                        if(check_x && check_y)
                            matrix->drawPixel(pixel_x, pixel_y, matrix->color565(sr, sg, sb));
                    }
                    bits <<= 1;
                    bit++;
                }
            }
            cursorX += glyph->advance_x;
        }
        text++;
    }
}
void scanning_clock(const uint16_t *logo_array, int logo_width, int logo_height, 
                    int matrix_width, int matrix_height, char *old_time, char *new_time) {

    int offset = -10;
    uint8_t brightness = 55;
    uint8_t sr = (255 * brightness) >> 8;
    uint8_t sg = (255 * brightness) >> 8;
    uint8_t sb = (255 * brightness) >> 8;

    for(int s = 10; s <= 64; s++){
        matrix_clear();
        draw_gfxtext(19, 19, old_time, s, 64, 0, 32);

        draw_logo(offset, 0, matrix_width + abs(offset), matrix_height, 
                    logo_width, logo_height, logo_array);
        matrix->drawFastVLine(s, 0, matrix_height, matrix->color565(sr, sg, sb));
        vTaskDelay(pdMS_TO_TICKS(25));
        
    }

    for(int s = 64; s >= 10; s--){
        matrix_clear();
        draw_gfxtext(19, 19, new_time, s, 64, 0, 32);

        draw_logo(offset, 0, matrix_width + abs(offset), matrix_height, 
                    logo_width, logo_height, logo_array);
        matrix->drawFastVLine(s, 0, matrix_height, matrix->color565(sr, sg, sb));
        vTaskDelay(pdMS_TO_TICKS(25));
        
    }


}
void scanning_clock1(const uint16_t *logo_array, int logo_width, int logo_height, 
                    int matrix_width, int matrix_height) {

    int offset = -10;
    uint8_t brightness = 55;
    uint8_t sr = (255 * brightness) >> 8;
    uint8_t sg = (255 * brightness) >> 8;
    uint8_t sb = (255 * brightness) >> 8;

    for(int s = 10; s <= 64; s++){
        matrix_clear();
        draw_gfxtext(20, 19, "12:30", s, 64, 0, 32);
        draw_gfxtext(20, 19, "12:31", 0, s, 0, 32);
        draw_logo(offset, 0, matrix_width + abs(offset), matrix_height, 
                    logo_width, logo_height, logo_array);
        matrix->drawFastVLine(s, 0, matrix_height, matrix->color565(sr, sg, sb));
        vTaskDelay(pdMS_TO_TICKS(25));
        
    }

    for(int s = 64; s >= 10; s--){
        matrix_clear();
        draw_gfxtext(20, 19, "12:31", 0, 64, 0, 32);

        draw_logo(offset, 0, matrix_width + abs(offset), matrix_height, 
                    logo_width, logo_height, logo_array);
        matrix->drawFastVLine(s, 0, matrix_height, matrix->color565(sr, sg, sb));
        vTaskDelay(pdMS_TO_TICKS(25));
        
    }


}
void scanning_logo(const uint16_t *logo_array, int logo_width, int logo_height, 
                    int matrix_width, int matrix_height) {

    int offset = -10;
    int target = 10;

    uint8_t brightness = 55;
    uint8_t sr = (255 * brightness) >> 8;
    uint8_t sg = (255 * brightness) >> 8;
    uint8_t sb = (255 * brightness) >> 8;

    // avanzamento della linea scanner da 0 fino alla larghezza dello schermo
    for (int scanner_x = 0 + (offset); scanner_x <= matrix_width; scanner_x++) {
        
        matrix->fillScreen(0);

        // disegna la mappa colonna per colonna, ma SOLO fino a scanner_x
        draw_logo(offset, 0, scanner_x + abs(offset), matrix_height, 
                    logo_width, logo_height,  logo_array);

        matrix->drawFastVLine(scanner_x, 0, matrix_height, matrix->color565(sr, sg, sb));

        vTaskDelay(pdMS_TO_TICKS(25));
    }

    for (int scanner_x = matrix_width; scanner_x >= target; scanner_x--) {
        matrix->fillScreen(0);

        // disegna l'intera mappa fissa sullo sfondo
        draw_logo(offset, 0, matrix_width + abs(offset), matrix_height, 
                    logo_width, logo_height, logo_array);

        // disegna la riga che arretra verso la time zone
        matrix->drawFastVLine(scanner_x, 0, matrix_height, matrix->color565(sr, sg, sb));
        draw_gfxtext(20, 19, "12:30", scanner_x, 64, 0, 32);

        vTaskDelay(pdMS_TO_TICKS(25)); // Velocità di ritorno (puoi regolarla)
    }

}


int ws_text_draw(const char* text, const int x, int y) {

    // colori di default al verde cosi se uno dimentica di impostarli almeno si vede
    int i = 0, r = 0, g = 255, b = 0, char_x = x;

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

        if(text[i] != ' ' && text[i] != '+' && text[i] != '-')
            draw_char(char_x, y, text[i], r, g, b, false);

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

void draw_chart(float price[][4], int size, int chart_h, int chart_w, 
                            int top_padding) {
    
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
        matrix_set_pixel(pixel, y, matrix->color565(0, 255, 0));
        /*

        uint8_t brightness = 0;
        for(int y2 = chart_h; y2 >= y; y2--){
            if(y2 == y) brightness = 120;
            else brightness = 10;
            uint8_t sr = (0 * brightness) >> 8;
            uint8_t sg = (255 * brightness) >> 8;
            uint8_t sb = (0 * brightness) >> 8;
            //brightness += 10;
            matrix_set_pixel(pixel, y2, matrix->color565(sr, sg, sb));
        }*/
        //matrix_set_pixel(pixel, 0, 0, 255, 0);
    }

}

void wallstreet_text(const char* text, const int x, int y) {

    //scroll_x variabile statica per spostare il testo verso sinistra
    //ogni chiamata di funzione
    static int scroll_x;
    static int first_call = 1;

    if(first_call){
        scroll_x = x;
        first_call = 0;
    }

    // primo ciclo di stampa caratteri sulla matrice
    int text_len = ws_text_draw(text, scroll_x, y);

    // secondo ciclo per dare la continuità al wall text
    ws_text_draw(text, scroll_x + text_len, y);

    scroll_x--;
    if(scroll_x <= -(text_len)) scroll_x = 0;

}

