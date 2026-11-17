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

void wallstreet_text(const char* text, const int x, int y);
int ws_text_draw(const char* text, const int x, int y); 
void draw_chart(float price[][4], int size, int chart_h, int chart_w, int top_padding);


void draw_text(int x, int y, const char* text, int r, int g, int b, bool small);
void draw_char(int x, int y, char c, int r, int g, int b, bool small);

void draw_logo(int start_x, int start_y, int end_x, int end_y, 
				int width, int height, const uint16_t *logo_array);

void scanning_logo(const uint16_t *logo_array, int logo_width, int logo_height, 
                    int matrix_width, int matrix_height);

void scanning_clock(const uint16_t *logo_array, int logo_width, int logo_height, 
                    int matrix_width, int matrix_height, char *old_time, char *new_time);

void scanning_clock1(const uint16_t *logo_array, int logo_width, int logo_height, 
                    int matrix_width, int matrix_height);

void draw_gfxtext(int x, int y, const char *text, int clip_x1, int clip_x2, 
                    int clip_y1, int clip_y2);



#ifdef __cplusplus
}
#endif

#endif