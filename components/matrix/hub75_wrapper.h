#ifndef HUB75_WRAPPER_H
#define HUB75_WRAPPER_H

// Definizione Pin 
#define R1_PIN 42
#define G1_PIN 40
#define B1_PIN 41
#define R2_PIN 38
#define G2_PIN 37
#define B2_PIN 39

#define A_PIN  45
#define B_PIN  36
#define C_PIN  48
#define D_PIN  35
#define E_PIN  21 

#define CLK_PIN   2
#define LAT_PIN   47
#define OE_PIN    14

#define BUTTON_A (gpio_num_t)6
#define BUTTON_B (gpio_num_t)7

#ifdef __cplusplus
extern "C" {
#endif

// Qui dichiari solo funzioni C "pure"
void matrix_init();
void matrix_set_pixel(int x, int y, uint16_t color);
void matrix_clear();
int matrix_clear_area(int x1, int x2, int y1, int y2);
void matrix_update_text(const char* text);

int wallstreet_text(const char* text, const int x, int y);
int ws_text_draw(const char* text, int x, int y); 
void draw_chart(float price[][4], int size, int chart_h, int chart_w, int top_padding, int chart_mode);


void draw_text(int x, int y, const char* text, int r, int g, int b, int brightness);
void draw_char(int x, int y, char c, uint8_t color);

void draw_logo(int width, int height, const uint16_t *logo_array,
                int start_x, int end_x, int start_y, int end_y, 
                int offset_x, int offset_y);

void scanning_logo(const uint16_t *logo_array, int logo_width, int logo_height, 
                    int start_x, int end_x, int start_y, int end_y, uint16_t scanner_color);

void update_clock(char *old_time, char *new_time, int clock_start_x, int clock_start_y, int clock_mode);

void draw_gfxtext(int x, int y, const char *text, int clip_x1, int clip_x2, 
                    int clip_y1, int clip_y2, uint16_t color);

int gfxtext_len(const char *text);



#ifdef __cplusplus
}
#endif

#endif