#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include "renderer.h"
#include "microui.h"

static char logbuf[64000];
static int logbuf_updated = 0;
static float bg[3] = {90, 95, 100};

static void write_log(const char *text)
{
  if (logbuf[0])
  {
    strcat(logbuf, "\n");
  }
  strcat(logbuf, text);
  logbuf_updated = 1;
}

static void test_window(mu_Context *context)
{
  /* do window */
  if (mu_begin_window(context, "Demo Window", mu_rect(40, 40, 300, 450)))
  {
    mu_Container *win = mu_get_current_container(context);
    win->rectangle.w = mu_max(win->rectangle.w, 240);
    win->rectangle.h = mu_max(win->rectangle.h, 300);

    /* window info */
    if (mu_header(context, "Window Info"))
    {
      mu_Container *win = mu_get_current_container(context);
      char buffer[64];
      mu_layout_row(context, 2, (int[]){54, -1}, 0);
      mu_label(context, "Position:");
      sprintf(buffer, "%d, %d", win->rectangle.x, win->rectangle.y);
      mu_label(context, buffer);
      mu_label(context, "Size:");
      sprintf(buffer, "%d, %d", win->rectangle.w, win->rectangle.h);
      mu_label(context, buffer);
    }

    /* labels + buttons */
    if (mu_header_ex(context, "Test Buttons", MU_OPT_EXPANDED))
    {
      mu_layout_row(context, 3, (int[]){86, -110, -1}, 0);
      mu_label(context, "Test buttons 1:");
      if (mu_button(context, "Button 1"))
      {
        write_log("Pressed button 1");
      }
      if (mu_button(context, "Button 2"))
      {
        write_log("Pressed button 2");
      }
      mu_label(context, "Test buttons 2:");
      if (mu_button(context, "Button 3"))
      {
        write_log("Pressed button 3");
      }
      if (mu_button(context, "Popup"))
      {
        mu_open_popup(context, "Test Popup");
      }
      if (mu_begin_popup(context, "Test Popup"))
      {
        mu_button(context, "Hello");
        mu_button(context, "World");
        mu_end_popup(context);
      }
    }

    /* tree */
    if (mu_header_ex(context, "Tree and Text", MU_OPT_EXPANDED))
    {
      mu_layout_row(context, 2, (int[]){140, -1}, 0);
      mu_layout_begin_column(context);
      if (mu_begin_treenode(context, "Test 1"))
      {
        if (mu_begin_treenode(context, "Test 1a"))
        {
          mu_label(context, "Hello");
          mu_label(context, "world");
          mu_end_treenode(context);
        }
        if (mu_begin_treenode(context, "Test 1b"))
        {
          if (mu_button(context, "Button 1"))
          {
            write_log("Pressed button 1");
          }
          if (mu_button(context, "Button 2"))
          {
            write_log("Pressed button 2");
          }
          mu_end_treenode(context);
        }
        mu_end_treenode(context);
      }
      if (mu_begin_treenode(context, "Test 2"))
      {
        mu_layout_row(context, 2, (int[]){54, 54}, 0);
        if (mu_button(context, "Button 3"))
        {
          write_log("Pressed button 3");
        }
        if (mu_button(context, "Button 4"))
        {
          write_log("Pressed button 4");
        }
        if (mu_button(context, "Button 5"))
        {
          write_log("Pressed button 5");
        }
        if (mu_button(context, "Button 6"))
        {
          write_log("Pressed button 6");
        }
        mu_end_treenode(context);
      }
      if (mu_begin_treenode(context, "Test 3"))
      {
        static int checks[3] = {1, 0, 1};
        mu_checkbox(context, "Checkbox 1", &checks[0]);
        mu_checkbox(context, "Checkbox 2", &checks[1]);
        mu_checkbox(context, "Checkbox 3", &checks[2]);
        mu_end_treenode(context);
      }
      mu_layout_end_column(context);

      mu_layout_begin_column(context);
      mu_layout_row(context, 1, (int[]){-1}, 0);
      mu_text(context, "Lorem ipsum dolor sit amet, consectetur adipiscing "
                       "elit. Maecenas lacinia, sem eu lacinia molestie, mi risus faucibus "
                       "ipsum, eu varius magna felis a nulla.");
      mu_layout_end_column(context);
    }

    /* background color sliders */
    if (mu_header_ex(context, "Background Color", MU_OPT_EXPANDED))
    {
      mu_layout_row(context, 2, (int[]){-78, -1}, 74);
      /* sliders */
      mu_layout_begin_column(context);
      mu_layout_row(context, 2, (int[]){46, -1}, 0);
      mu_label(context, "Red:");
      mu_slider(context, &bg[0], 0, 255);
      mu_label(context, "Green:");
      mu_slider(context, &bg[1], 0, 255);
      mu_label(context, "Blue:");
      mu_slider(context, &bg[2], 0, 255);
      mu_layout_end_column(context);
      /* color preview */
      mu_Rectangle renderer = mu_layout_next(context);
      mu_draw_rect(context, renderer, mu_color(bg[0], bg[1], bg[2], 255));
      char buffer[32];
      sprintf(buffer, "#%02X%02X%02X", (int)bg[0], (int)bg[1], (int)bg[2]);
      mu_draw_control_text(context, buffer, renderer, MU_COLOR_TEXT, MU_OPT_ALIGNCENTER);
    }

    mu_end_window(context);
  }
}

static void log_window(mu_Context *context)
{
  if (mu_begin_window(context, "Log Window", mu_rect(350, 40, 300, 200)))
  {
    /* output text panel */
    mu_layout_row(context, 1, (int[]){-1}, -25);
    mu_begin_panel(context, "Log Output");
    mu_Container *panel = mu_get_current_container(context);
    mu_layout_row(context, 1, (int[]){-1}, -1);
    mu_text(context, logbuf);
    mu_end_panel(context);
    if (logbuf_updated)
    {
      panel->scroll.y = panel->content_size.y;
      logbuf_updated = 0;
    }

    /* input textbox + submit button */
    static char buffer[128];
    int submitted = 0;
    mu_layout_row(context, 2, (int[]){-70, -1}, 0);
    if (mu_textbox(context, buffer, sizeof(buffer)) & MU_RES_SUBMIT)
    {
      mu_set_focus(context, context->last_identifier);
      submitted = 1;
    }
    if (mu_button(context, "Submit"))
    {
      submitted = 1;
    }
    if (submitted)
    {
      write_log(buffer);
      buffer[0] = '\0';
    }

    mu_end_window(context);
  }
}

static int uint8_slider(mu_Context *context, unsigned char *value, int low, int high)
{
  static float tmp;
  mu_push_id(context, &value, sizeof(value));
  tmp = *value;
  int res = mu_slider_ex(context, &tmp, low, high, 0, "%.0f", MU_OPT_ALIGNCENTER);
  *value = tmp;
  mu_pop_id(context);
  return res;
}

static void style_window(mu_Context *context)
{
  static struct
  {
    const char *label;
    int idx;
  } colors[] = {
      {"text:", MU_COLOR_TEXT},
      {"border:", MU_COLOR_BORDER},
      {"windowbg:", MU_COLOR_WINDOWBG},
      {"titlebg:", MU_COLOR_TITLEBG},
      {"titletext:", MU_COLOR_TITLETEXT},
      {"panelbg:", MU_COLOR_PANELBG},
      {"button:", MU_COLOR_BUTTON},
      {"buttonhover:", MU_COLOR_BUTTONHOVER},
      {"buttonfocus:", MU_COLOR_BUTTONFOCUS},
      {"base:", MU_COLOR_BASE},
      {"basehover:", MU_COLOR_BASEHOVER},
      {"basefocus:", MU_COLOR_BASEFOCUS},
      {"scrollbase:", MU_COLOR_SCROLLBASE},
      {"scrollthumb:", MU_COLOR_SCROLLTHUMB},
      {NULL}};

  if (mu_begin_window(context, "Style Editor", mu_rect(350, 250, 300, 240)))
  {
    int sw = mu_get_current_container(context)->body.w * 0.14;
    mu_layout_row(context, 6, (int[]){80, sw, sw, sw, sw, -1}, 0);
    for (int i = 0; colors[i].label; i++)
    {
      mu_label(context, colors[i].label);
      uint8_slider(context, &context->style->colors[i].red, 0, 255);
      uint8_slider(context, &context->style->colors[i].green, 0, 255);
      uint8_slider(context, &context->style->colors[i].blue, 0, 255);
      uint8_slider(context, &context->style->colors[i].alpha, 0, 255);
      mu_draw_rect(context, mu_layout_next(context), context->style->colors[i]);
    }
    mu_end_window(context);
  }
}

static void process_frame(mu_Context *context)
{
  mu_begin(context);
  style_window(context);
  log_window(context);
  test_window(context);
  mu_end(context);
}

static const char button_map[256] = {
    [SDL_BUTTON_LEFT & 0xff] = MU_MOUSE_LEFT,
    [SDL_BUTTON_RIGHT & 0xff] = MU_MOUSE_RIGHT,
    [SDL_BUTTON_MIDDLE & 0xff] = MU_MOUSE_MIDDLE,
};

static const char key_map[256] = {
    [SDLK_LSHIFT & 0xff] = MU_KEY_SHIFT,
    [SDLK_RSHIFT & 0xff] = MU_KEY_SHIFT,
    [SDLK_LCTRL & 0xff] = MU_KEY_CTRL,
    [SDLK_RCTRL & 0xff] = MU_KEY_CTRL,
    [SDLK_LALT & 0xff] = MU_KEY_ALT,
    [SDLK_RALT & 0xff] = MU_KEY_ALT,
    [SDLK_RETURN & 0xff] = MU_KEY_RETURN,
    [SDLK_BACKSPACE & 0xff] = MU_KEY_BACKSPACE,
};

static int text_width(mu_Font font, const char *text, int length)
{
  Renderer *renderer = (Renderer *)font;
  if (length == -1)
  {
    length = strlen(text);
  }
  return renderer_get_text_width(renderer, text, length);
}

static int text_height(mu_Font font)
{
  Renderer *renderer = (Renderer *)font;
  return renderer_get_text_height(renderer);
}

int main(int argc, char **argv)
{
  /* init SDL and renderer */
  Renderer *renderer = renderer_init();

  /* init microui */
  mu_Context *context = malloc(sizeof(mu_Context));
  mu_init(context);
  context->text_width = text_width;
  context->text_height = text_height;
  /* Use Renderer pointer as the font handle */
  context->style->font = (mu_Font)renderer;

  /* main loop */
  for (;;)
  {
    /* handle SDL events */
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
      switch (e.type)
      {
      case SDL_EVENT_QUIT:
        exit(EXIT_SUCCESS);
        break;
      case SDL_EVENT_MOUSE_MOTION:
        mu_input_mousemove(context, e.motion.x, e.motion.y);
        break;
      case SDL_EVENT_MOUSE_WHEEL:
        mu_input_scroll(context, 0, e.wheel.y * -30);
        break;
      case SDL_EVENT_TEXT_INPUT:
        mu_input_text(context, e.text.text);
        break;

      case SDL_EVENT_MOUSE_BUTTON_DOWN:
      case SDL_EVENT_MOUSE_BUTTON_UP:
      {
        int b = button_map[e.button.button & 0xff];
        if (b && e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
          mu_input_mousedown(context, e.button.x, e.button.y, b);
        }
        if (b && e.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
          mu_input_mouseup(context, e.button.x, e.button.y, b);
        }
        break;
      }

      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP:
      {
        int c = key_map[e.key.key & 0xff];
        if (c && e.type == SDL_EVENT_KEY_DOWN)
        {
          mu_input_keydown(context, c);
        }
        if (c && e.type == SDL_EVENT_KEY_UP)
        {
          mu_input_keyup(context, c);
        }
        break;
      }
      }
    }

    /* process frame */
    process_frame(context);

    /* render */
    renderer_clear(renderer, mu_color(bg[0], bg[1], bg[2], 255));
    mu_Command *command = NULL;
    while (mu_next_command(context, &command))
    {
      switch (command->type)
      {
      case MU_COMMAND_TEXT:
        renderer_draw_text(renderer, command->text.str, command->text.position, command->text.color);
        break;
      case MU_COMMAND_RECT:
        renderer_draw_rect(renderer, command->rectangle.rectangle, command->rectangle.color);
        break;
      case MU_COMMAND_ICON:
        renderer_draw_icon(renderer, command->icon.identifier, command->icon.rectangle, command->icon.color);
        break;
      case MU_COMMAND_CLIP:
        renderer_set_clip_rect(renderer, command->clip.rectangle);
        break;
      }
    }
    renderer_present(renderer);
  }

  return 0;
}
