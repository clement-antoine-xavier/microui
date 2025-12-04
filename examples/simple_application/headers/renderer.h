#ifndef RENDERER_H
#define RENDERER_H

#include "microui.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

typedef struct Renderer {
  int width;
  int height;
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *atlas_texture;
  TTF_Font *font;
} Renderer;

Renderer *renderer_init(void);
void renderer_destroy(Renderer *renderer);
void renderer_draw_rect(Renderer *renderer, mu_Rectangle rectangle, mu_Color color);
void renderer_draw_text(Renderer *renderer, const char *text, mu_Vector2 position, mu_Color color);
void renderer_draw_icon(Renderer *renderer, int identifier, mu_Rectangle rectangle, mu_Color color);
int renderer_get_text_width(Renderer *renderer, const char *text, int length);
int renderer_get_text_height(Renderer *renderer);
void renderer_set_clip_rect(Renderer *renderer, mu_Rectangle rectangle);
void renderer_clear(Renderer *renderer, mu_Color color);
void renderer_present(Renderer *renderer);

#endif
