#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "renderer.h"

/* Atlas definitions for icons */
enum
{
  ATLAS_WHITE = MU_ICON_MAX,
  ATLAS_FONT
};
enum
{
  ATLAS_WIDTH = 128,
  ATLAS_HEIGHT = 128
};

static unsigned char atlas_texture[ATLAS_WIDTH * ATLAS_HEIGHT];
static mu_Rectangle atlas[256];

/* System font paths for different platforms */
static const char *font_paths[] = {
    /* macOS */
    "/System/Library/Fonts/Helvetica.ttc",
    "/System/Library/Fonts/Monaco.ttc",
    "/System/Library/Fonts/Helvetica.ttc",
    /* Linux */
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
    "/usr/share/fonts/liberation/LiberationMono-Regular.ttf",
    /* Windows (if running via WSL or similar) */
    "/mnt/c/Windows/Fonts/consola.ttf",
    "C:\\Windows\\Fonts\\consola.ttf",
    NULL};

Renderer *renderer_init(void)
{
  Renderer *renderer = malloc(sizeof(Renderer));
  if (!renderer)
  {
    fprintf(stderr, "Failed to allocate Renderer\n");
    exit(1);
  }

  renderer->width = 800;
  renderer->height = 600;
  renderer->window = NULL;
  renderer->renderer = NULL;
  renderer->atlas_texture = NULL;
  renderer->font = NULL;

  /* Initialize SDL */
  if (!SDL_Init(SDL_INIT_VIDEO))
  {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    free(renderer);
    exit(1);
  }

  /* Initialize SDL_ttf */
  if (!TTF_Init())
  {
    fprintf(stderr, "TTF_Init failed: %s\n", SDL_GetError());
    free(renderer);
    exit(1);
  }

  /* Create SDL3 window */
  if (!SDL_CreateWindowAndRenderer("MicroUI Demo", renderer->width, renderer->height, 0, &renderer->window, &renderer->renderer))
  {
    fprintf(stderr, "SDL_CreateWindowAndRenderer failed: %s\n", SDL_GetError());
    free(renderer);
    exit(1);
  }

  /* Set blend mode for alpha blending */
  SDL_SetRenderDrawBlendMode(renderer->renderer, SDL_BLENDMODE_BLEND);

  /* Load system font - try multiple paths */
  int font_size = 14;
  for (int i = 0; font_paths[i] != NULL; i++)
  {
    renderer->font = TTF_OpenFont(font_paths[i], font_size);
    if (renderer->font)
    {
      printf("Loaded font: %s\n", font_paths[i]);
      break;
    }
  }

  if (!renderer->font)
  {
    fprintf(stderr, "Failed to load any system font. Tried:\n");
    for (int i = 0; font_paths[i] != NULL; i++)
    {
      fprintf(stderr, "  - %s\n", font_paths[i]);
    }
    fprintf(stderr, "Error: %s\n", SDL_GetError());
    free(renderer);
    exit(1);
  }

  /* Create texture from atlas data (for icons only) */
  unsigned char *rgba_data = malloc(ATLAS_WIDTH * ATLAS_HEIGHT * 4);
  for (int i = 0; i < ATLAS_WIDTH * ATLAS_HEIGHT; i++)
  {
    rgba_data[i * 4 + 0] = 255;              /* R */
    rgba_data[i * 4 + 1] = 255;              /* G */
    rgba_data[i * 4 + 2] = 255;              /* B */
    rgba_data[i * 4 + 3] = atlas_texture[i]; /* A */
  }

  renderer->atlas_texture = SDL_CreateTexture(renderer->renderer, SDL_PIXELFORMAT_RGBA32,
                                              SDL_TEXTUREACCESS_STATIC, ATLAS_WIDTH, ATLAS_HEIGHT);
  SDL_UpdateTexture(renderer->atlas_texture, NULL, rgba_data, ATLAS_WIDTH * 4);
  SDL_SetTextureBlendMode(renderer->atlas_texture, SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(renderer->atlas_texture, SDL_SCALEMODE_NEAREST);

  free(rgba_data);
  return renderer;
}

void renderer_destroy(Renderer *renderer)
{
  if (!renderer)
    return;

  if (renderer->atlas_texture)
    SDL_DestroyTexture(renderer->atlas_texture);
  if (renderer->font)
    TTF_CloseFont(renderer->font);
  if (renderer->renderer)
    SDL_DestroyRenderer(renderer->renderer);
  if (renderer->window)
    SDL_DestroyWindow(renderer->window);

  TTF_Quit();
  SDL_Quit();
  free(renderer);
}

void renderer_draw_rect(Renderer *renderer, mu_Rectangle rectangle, mu_Color color)
{
  SDL_FRect frect = {rectangle.x, rectangle.y, rectangle.w, rectangle.h};
  SDL_SetRenderDrawColor(renderer->renderer, color.red, color.green, color.blue, color.alpha);
  SDL_RenderFillRect(renderer->renderer, &frect);
}

void renderer_draw_text(Renderer *renderer, const char *text, mu_Vector2 position, mu_Color color)
{
  if (!renderer->font || !text || !*text)
    return;

  /* Render text to surface using TTF */
  SDL_Color sdl_color = {color.red, color.green, color.blue, color.alpha};
  SDL_Surface *surface = TTF_RenderText_Blended(renderer->font, text, strlen(text), sdl_color);
  if (!surface)
  {
    fprintf(stderr, "TTF_RenderText_Blended failed: %s\n", SDL_GetError());
    return;
  }

  /* Create texture from surface */
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer->renderer, surface);
  if (!texture)
  {
    fprintf(stderr, "SDL_CreateTextureFromSurface failed: %s\n", SDL_GetError());
    SDL_DestroySurface(surface);
    return;
  }

  /* Render texture */
  SDL_FRect dst_rect = {position.x, position.y, surface->w, surface->h};
  SDL_RenderTexture(renderer->renderer, texture, NULL, &dst_rect);

  /* Cleanup */
  SDL_DestroyTexture(texture);
  SDL_DestroySurface(surface);
}

void renderer_draw_icon(Renderer *renderer, int identifier, mu_Rectangle rectangle, mu_Color color)
{
  mu_Rectangle src = atlas[identifier];
  int x = rectangle.x + (rectangle.w - src.w) / 2;
  int y = rectangle.y + (rectangle.h - src.h) / 2;

  SDL_FRect src_rect = {src.x, src.y, src.w, src.h};
  SDL_FRect dst_rect = {x, y, src.w, src.h};

  SDL_SetTextureColorMod(renderer->atlas_texture, color.red, color.green, color.blue);
  SDL_SetTextureAlphaMod(renderer->atlas_texture, color.alpha);
  SDL_RenderTexture(renderer->renderer, renderer->atlas_texture, &src_rect, &dst_rect);
}

int renderer_get_text_width(Renderer *renderer, const char *text, int length)
{
  if (!renderer->font || !text || length == 0)
    return 0;

  /* Create a null-terminated string if length is specified */
  char *temp = NULL;
  if (length > 0 && length < strlen(text))
  {
    temp = malloc(length + 1);
    strncpy(temp, text, length);
    temp[length] = '\0';
    text = temp;
  }

  int width = 0;
  int height = 0;
  if (!TTF_GetStringSize(renderer->font, text, strlen(text), &width, &height))
  {
    fprintf(stderr, "TTF_SizeText failed: %s\n", SDL_GetError());
    width = 0;
  }

  if (temp)
    free(temp);
  return width;
}

int renderer_get_text_height(Renderer *renderer)
{
  if (!renderer->font)
    return 14;
  return TTF_GetFontHeight(renderer->font);
}

void renderer_set_clip_rect(Renderer *renderer, mu_Rectangle rectangle)
{
  SDL_Rect clip_rect = {rectangle.x, rectangle.y, rectangle.w, rectangle.h};
  SDL_SetRenderClipRect(renderer->renderer, &clip_rect);
}

void renderer_clear(Renderer *renderer, mu_Color clr)
{
  SDL_SetRenderDrawColor(renderer->renderer, clr.red, clr.green, clr.blue, clr.alpha);
  SDL_RenderClear(renderer->renderer);
}

void renderer_present(Renderer *renderer)
{
  SDL_RenderPresent(renderer->renderer);
}
