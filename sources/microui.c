/*
** Copyright (c) 2024 rxi
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to
** deal in the Software without restriction, including without limitation the
** rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
** sell copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
*/

/**
 * @file microui.c
 * @brief Implementation of MicroUI - immediate-mode UI library
 *
 * This file contains the core implementation of the MicroUI library.
 * It handles:
 * - Context initialization and frame management
 * - Input event processing
 * - Command list generation and iteration
 * - Widget layout and rendering
 * - Container (window/panel) management
 * - Interactive control handling
 *
 * The library uses an immediate-mode API where the UI is specified
 * each frame, and drawing commands are collected for rendering.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "microui.h"

/** @brief Mark parameter as intentionally unused to suppress warnings */
#define unused(x) ((void)(x))

/**
 * @brief Assert macro - verifies a condition and exits if false
 *
 * Used for internal error checking. In production, this could be
 * modified to handle errors differently.
 */
#define expect(x)                                                    \
  do                                                                 \
  {                                                                  \
    if (!(x))                                                        \
    {                                                                \
      fprintf(stderr, "Fatal error: %s:%d: assertion '%s' failed\n", \
              __FILE__, __LINE__, #x);                               \
      abort();                                                       \
    }                                                                \
  } while (0)

/**
 * @brief Push a value onto a stack
 *
 * Adds an item to a stack and increments the index.
 * Asserts if stack is full.
 */
#define push(stk, val)                                                     \
  do                                                                       \
  {                                                                        \
    expect((stk).idx < (int)(sizeof((stk).items) / sizeof(*(stk).items))); \
    (stk).items[(stk).idx] = (val);                                        \
    (stk).idx++; /* incremented after incase `val` uses this value */      \
  } while (0)

/**
 * @brief Pop a value from a stack
 *
 * Decrements stack index. Asserts if stack is empty.
 */
#define pop(stk)           \
  do                       \
  {                        \
    expect((stk).idx > 0); \
    (stk).idx--;           \
  } while (0)

/* ========================================================================
 * GLOBALS
 * ======================================================================== */

/** @brief Maximum clipping rectangle (used for unclipped rendering) */
static mu_Rectangle unclipped_rect = {0, 0, 0x1000000, 0x1000000};

/** @brief Default color theme and style settings */
static mu_Style default_style = {
    /* font | size | padding | spacing | indentation */
    NULL,
    {68, 10},
    5,
    4,
    24,
    /* title_height | scrollbar_size | thumb_size */
    24,
    12,
    8,
    {
        {230, 230, 230, 255}, /* MU_COLOR_TEXT */
        {25, 25, 25, 255},    /* MU_COLOR_BORDER */
        {50, 50, 50, 255},    /* MU_COLOR_WINDOWBG */
        {25, 25, 25, 255},    /* MU_COLOR_TITLEBG */
        {240, 240, 240, 255}, /* MU_COLOR_TITLETEXT */
        {0, 0, 0, 0},         /* MU_COLOR_PANELBG */
        {75, 75, 75, 255},    /* MU_COLOR_BUTTON */
        {95, 95, 95, 255},    /* MU_COLOR_BUTTONHOVER */
        {115, 115, 115, 255}, /* MU_COLOR_BUTTONFOCUS */
        {30, 30, 30, 255},    /* MU_COLOR_BASE */
        {35, 35, 35, 255},    /* MU_COLOR_BASEHOVER */
        {40, 40, 40, 255},    /* MU_COLOR_BASEFOCUS */
        {43, 43, 43, 255},    /* MU_COLOR_SCROLLBASE */
        {30, 30, 30, 255}     /* MU_COLOR_SCROLLTHUMB */
    }};

/* ========================================================================
 * BASIC TYPES
 * ======================================================================== */

/** @brief Create a 2D vector with integer coordinates */
mu_Vector2 mu_vec2(int x, int y)
{
  mu_Vector2 res;
  res.x = x;
  res.y = y;
  return res;
}

/** @brief Create a rectangle with integer bounds */
mu_Rectangle mu_rect(int x, int y, int w, int h)
{
  mu_Rectangle res;
  res.x = x;
  res.y = y;
  res.w = w;
  res.h = h;
  return res;
}

/** @brief Create a color with RGBA components */
mu_Color mu_color(int renderer, int g, int b, int a)
{
  mu_Color res;
  res.red = renderer;
  res.green = g;
  res.blue = b;
  res.alpha = a;
  return res;
}

static mu_Rectangle expand_rect(mu_Rectangle rectangle, int n)
{
  return mu_rect(rectangle.x - n, rectangle.y - n, rectangle.w + n * 2, rectangle.h + n * 2);
}

static mu_Rectangle intersect_rects(mu_Rectangle r1, mu_Rectangle r2)
{
  int x1 = mu_max(r1.x, r2.x);
  int y1 = mu_max(r1.y, r2.y);
  int x2 = mu_min(r1.x + r1.w, r2.x + r2.w);
  int y2 = mu_min(r1.y + r1.h, r2.y + r2.h);
  if (x2 < x1)
  {
    x2 = x1;
  }
  if (y2 < y1)
  {
    y2 = y1;
  }
  return mu_rect(x1, y1, x2 - x1, y2 - y1);
}

static int rect_overlaps_vec2(mu_Rectangle renderer, mu_Vector2 p)
{
  return p.x >= renderer.x && p.x < renderer.x + renderer.w && p.y >= renderer.y && p.y < renderer.y + renderer.h;
}

static void draw_frame(mu_Context *context, mu_Rectangle rectangle, int colorid)
{
  mu_draw_rect(context, rectangle, context->style->colors[colorid]);
  if (colorid == MU_COLOR_SCROLLBASE ||
      colorid == MU_COLOR_SCROLLTHUMB ||
      colorid == MU_COLOR_TITLEBG)
  {
    return;
  }
  /* draw border */
  if (context->style->colors[MU_COLOR_BORDER].alpha)
  {
    mu_draw_box(context, expand_rect(rectangle, 1), context->style->colors[MU_COLOR_BORDER]);
  }
}

void mu_init(mu_Context *context)
{
  memset(context, 0, sizeof(*context));
  context->draw_frame = draw_frame;
  context->_style = default_style;
  context->style = &context->_style;
}

void mu_begin(mu_Context *context)
{
  expect(context->text_width && context->text_height);
  context->command_list.idx = 0;
  context->root_list.idx = 0;
  context->scroll_target = NULL;
  context->hover_root = context->next_hover_root;
  context->next_hover_root = NULL;
  context->mouse_delta.x = context->mouse_pos.x - context->last_mouse_pos.x;
  context->mouse_delta.y = context->mouse_pos.y - context->last_mouse_pos.y;
  context->frame++;
}

static int compare_zindex(const void *a, const void *b)
{
  return (*(mu_Container **)a)->zindex - (*(mu_Container **)b)->zindex;
}

void mu_end(mu_Context *context)
{
  int i, n;
  /* check stacks */
  expect(context->container_stack.idx == 0);
  expect(context->clip_stack.idx == 0);
  expect(context->id_stack.idx == 0);
  expect(context->layout_stack.idx == 0);

  /* handle scroll input */
  if (context->scroll_target)
  {
    context->scroll_target->scroll.x += context->scroll_delta.x;
    context->scroll_target->scroll.y += context->scroll_delta.y;
  }

  /* unset focus if focus identifier was not touched this frame */
  if (!context->updated_focus)
  {
    context->focus = 0;
  }
  context->updated_focus = 0;

  /* bring hover root to front if mouse was pressed */
  if (context->mouse_pressed && context->next_hover_root &&
      context->next_hover_root->zindex < context->last_zindex &&
      context->next_hover_root->zindex >= 0)
  {
    mu_bring_to_front(context, context->next_hover_root);
  }

  /* reset input state */
  context->key_pressed = 0;
  context->input_text[0] = '\0';
  context->mouse_pressed = 0;
  context->scroll_delta = mu_vec2(0, 0);
  context->last_mouse_pos = context->mouse_pos;

  /* sort root containers by zindex */
  n = context->root_list.idx;
  qsort(context->root_list.items, n, sizeof(mu_Container *), compare_zindex);

  /* set root container jump commands */
  for (i = 0; i < n; i++)
  {
    mu_Container *cnt = context->root_list.items[i];
    /* if this is the first container then make the first command jump to it.
    ** otherwise set the previous container's tail to jump to this one */
    if (i == 0)
    {
      mu_Command *command = (mu_Command *)context->command_list.items;
      command->jump.dst = (char *)cnt->head + sizeof(mu_JumpCommand);
    }
    else
    {
      mu_Container *prev = context->root_list.items[i - 1];
      prev->tail->jump.dst = (char *)cnt->head + sizeof(mu_JumpCommand);
    }
    /* make the last container's tail jump to the end of command list */
    if (i == n - 1)
    {
      cnt->tail->jump.dst = context->command_list.items + context->command_list.idx;
    }
  }
}

void mu_set_focus(mu_Context *context, mu_Identifier identifier)
{
  context->focus = identifier;
  context->updated_focus = 1;
}

/* 32bit fnv-1a hash */
#define HASH_INITIAL 2166136261

static void hash(mu_Identifier *hash, const void *data, int size)
{
  const unsigned char *p = data;
  while (size--)
  {
    *hash = (*hash ^ *p++) * 16777619;
  }
}

mu_Identifier mu_get_id(mu_Context *context, const void *data, int size)
{
  int idx = context->id_stack.idx;
  mu_Identifier res = (idx > 0) ? context->id_stack.items[idx - 1] : HASH_INITIAL;
  hash(&res, data, size);
  context->last_identifier = res;
  return res;
}

void mu_push_id(mu_Context *context, const void *data, int size)
{
  push(context->id_stack, mu_get_id(context, data, size));
}

void mu_pop_id(mu_Context *context)
{
  pop(context->id_stack);
}

void mu_push_clip_rect(mu_Context *context, mu_Rectangle rectangle)
{
  mu_Rectangle last = mu_get_clip_rect(context);
  push(context->clip_stack, intersect_rects(rectangle, last));
}

void mu_pop_clip_rect(mu_Context *context)
{
  pop(context->clip_stack);
}

mu_Rectangle mu_get_clip_rect(mu_Context *context)
{
  expect(context->clip_stack.idx > 0);
  return context->clip_stack.items[context->clip_stack.idx - 1];
}

int mu_check_clip(mu_Context *context, mu_Rectangle renderer)
{
  mu_Rectangle cr = mu_get_clip_rect(context);
  if (renderer.x > cr.x + cr.w || renderer.x + renderer.w < cr.x ||
      renderer.y > cr.y + cr.h || renderer.y + renderer.h < cr.y)
  {
    return MU_CLIP_ALL;
  }
  if (renderer.x >= cr.x && renderer.x + renderer.w <= cr.x + cr.w &&
      renderer.y >= cr.y && renderer.y + renderer.h <= cr.y + cr.h)
  {
    return 0;
  }
  return MU_CLIP_PART;
}

static void push_layout(mu_Context *context, mu_Rectangle body, mu_Vector2 scroll)
{
  mu_Layout layout;
  int width = 0;
  memset(&layout, 0, sizeof(layout));
  layout.body = mu_rect(body.x - scroll.x, body.y - scroll.y, body.w, body.h);
  layout.max = mu_vec2(-0x1000000, -0x1000000);
  push(context->layout_stack, layout);
  mu_layout_row(context, 1, &width, 0);
}

static mu_Layout *get_layout(mu_Context *context)
{
  return &context->layout_stack.items[context->layout_stack.idx - 1];
}

static void pop_container(mu_Context *context)
{
  mu_Container *cnt = mu_get_current_container(context);
  mu_Layout *layout = get_layout(context);
  cnt->content_size.x = layout->max.x - layout->body.x;
  cnt->content_size.y = layout->max.y - layout->body.y;
  /* pop container, layout and identifier */
  pop(context->container_stack);
  pop(context->layout_stack);
  mu_pop_id(context);
}

mu_Container *mu_get_current_container(mu_Context *context)
{
  expect(context->container_stack.idx > 0);
  return context->container_stack.items[context->container_stack.idx - 1];
}

static mu_Container *get_container(mu_Context *context, mu_Identifier identifier, int opt)
{
  mu_Container *cnt;
  /* try to get existing container from pool */
  int idx = mu_pool_get(context, context->container_pool, MU_CONTAINERPOOL_SIZE, identifier);
  if (idx >= 0)
  {
    if (context->containers[idx].open || ~opt & MU_OPT_CLOSED)
    {
      mu_pool_update(context, context->container_pool, idx);
    }
    return &context->containers[idx];
  }
  if (opt & MU_OPT_CLOSED)
  {
    return NULL;
  }
  /* container not found in pool: init new container */
  idx = mu_pool_init(context, context->container_pool, MU_CONTAINERPOOL_SIZE, identifier);
  cnt = &context->containers[idx];
  memset(cnt, 0, sizeof(*cnt));
  cnt->open = 1;
  mu_bring_to_front(context, cnt);
  return cnt;
}

mu_Container *mu_get_container(mu_Context *context, const char *name)
{
  mu_Identifier identifier = mu_get_id(context, name, strlen(name));
  return get_container(context, identifier, 0);
}

void mu_bring_to_front(mu_Context *context, mu_Container *cnt)
{
  cnt->zindex = ++context->last_zindex;
}

/*============================================================================
** pool
**============================================================================*/

int mu_pool_init(mu_Context *context, mu_PoolItem *items, int length, mu_Identifier identifier)
{
  int i, n = -1, f = context->frame;
  for (i = 0; i < length; i++)
  {
    if (items[i].last_update < f)
    {
      f = items[i].last_update;
      n = i;
    }
  }
  expect(n > -1);
  items[n].identifier = identifier;
  mu_pool_update(context, items, n);
  return n;
}

int mu_pool_get(mu_Context *context, mu_PoolItem *items, int length, mu_Identifier identifier)
{
  int i;
  unused(context);
  for (i = 0; i < length; i++)
  {
    if (items[i].identifier == identifier)
    {
      return i;
    }
  }
  return -1;
}

void mu_pool_update(mu_Context *context, mu_PoolItem *items, int idx)
{
  items[idx].last_update = context->frame;
}

/*============================================================================
** input handlers
**============================================================================*/

void mu_input_mousemove(mu_Context *context, int x, int y)
{
  context->mouse_pos = mu_vec2(x, y);
}

void mu_input_mousedown(mu_Context *context, int x, int y, int btn)
{
  mu_input_mousemove(context, x, y);
  context->mouse_down |= btn;
  context->mouse_pressed |= btn;
}

void mu_input_mouseup(mu_Context *context, int x, int y, int btn)
{
  mu_input_mousemove(context, x, y);
  context->mouse_down &= ~btn;
}

void mu_input_scroll(mu_Context *context, int x, int y)
{
  context->scroll_delta.x += x;
  context->scroll_delta.y += y;
}

void mu_input_keydown(mu_Context *context, int key)
{
  context->key_pressed |= key;
  context->key_down |= key;
}

void mu_input_keyup(mu_Context *context, int key)
{
  context->key_down &= ~key;
}

void mu_input_text(mu_Context *context, const char *text)
{
  int length = strlen(context->input_text);
  int size = strlen(text) + 1;
  expect(length + size <= (int)sizeof(context->input_text));
  memcpy(context->input_text + length, text, size);
}

/*============================================================================
** commandlist
**============================================================================*/

mu_Command *mu_push_command(mu_Context *context, int type, int size)
{
  mu_Command *command = (mu_Command *)(context->command_list.items + context->command_list.idx);
  expect(context->command_list.idx + size < MU_COMMANDLIST_SIZE);
  command->base.type = type;
  command->base.size = size;
  context->command_list.idx += size;
  return command;
}

int mu_next_command(mu_Context *context, mu_Command **command)
{
  if (*command)
  {
    *command = (mu_Command *)(((char *)*command) + (*command)->base.size);
  }
  else
  {
    *command = (mu_Command *)context->command_list.items;
  }
  while ((char *)*command != context->command_list.items + context->command_list.idx)
  {
    if ((*command)->type != MU_COMMAND_JUMP)
    {
      return 1;
    }
    *command = (*command)->jump.dst;
  }
  return 0;
}

static mu_Command *push_jump(mu_Context *context, mu_Command *dst)
{
  mu_Command *command;
  command = mu_push_command(context, MU_COMMAND_JUMP, sizeof(mu_JumpCommand));
  command->jump.dst = dst;
  return command;
}

void mu_set_clip(mu_Context *context, mu_Rectangle rectangle)
{
  mu_Command *command;
  command = mu_push_command(context, MU_COMMAND_CLIP, sizeof(mu_ClipCommand));
  command->clip.rectangle = rectangle;
}

void mu_draw_rect(mu_Context *context, mu_Rectangle rectangle, mu_Color color)
{
  mu_Command *command;
  rectangle = intersect_rects(rectangle, mu_get_clip_rect(context));
  if (rectangle.w > 0 && rectangle.h > 0)
  {
    command = mu_push_command(context, MU_COMMAND_RECT, sizeof(mu_RectCommand));
    command->rectangle.rectangle = rectangle;
    command->rectangle.color = color;
  }
}

void mu_draw_box(mu_Context *context, mu_Rectangle rectangle, mu_Color color)
{
  mu_draw_rect(context, mu_rect(rectangle.x + 1, rectangle.y, rectangle.w - 2, 1), color);
  mu_draw_rect(context, mu_rect(rectangle.x + 1, rectangle.y + rectangle.h - 1, rectangle.w - 2, 1), color);
  mu_draw_rect(context, mu_rect(rectangle.x, rectangle.y, 1, rectangle.h), color);
  mu_draw_rect(context, mu_rect(rectangle.x + rectangle.w - 1, rectangle.y, 1, rectangle.h), color);
}

void mu_draw_text(mu_Context *context, mu_Font font, const char *str, int length,
                  mu_Vector2 position, mu_Color color)
{
  mu_Command *command;
  mu_Rectangle rectangle = mu_rect(
      position.x, position.y, context->text_width(font, str, length), context->text_height(font));
  int clipped = mu_check_clip(context, rectangle);
  if (clipped == MU_CLIP_ALL)
  {
    return;
  }
  if (clipped == MU_CLIP_PART)
  {
    mu_set_clip(context, mu_get_clip_rect(context));
  }
  /* add command */
  if (length < 0)
  {
    length = strlen(str);
  }
  command = mu_push_command(context, MU_COMMAND_TEXT, sizeof(mu_TextCommand) + length);
  memcpy(command->text.str, str, length);
  command->text.str[length] = '\0';
  command->text.position = position;
  command->text.color = color;
  command->text.font = font;
  /* reset clipping if it was set */
  if (clipped)
  {
    mu_set_clip(context, unclipped_rect);
  }
}

void mu_draw_icon(mu_Context *context, int identifier, mu_Rectangle rectangle, mu_Color color)
{
  mu_Command *command;
  /* do clip command if the rectangle isn't fully contained within the cliprect */
  int clipped = mu_check_clip(context, rectangle);
  if (clipped == MU_CLIP_ALL)
  {
    return;
  }
  if (clipped == MU_CLIP_PART)
  {
    mu_set_clip(context, mu_get_clip_rect(context));
  }
  /* do icon command */
  command = mu_push_command(context, MU_COMMAND_ICON, sizeof(mu_IconCommand));
  command->icon.identifier = identifier;
  command->icon.rectangle = rectangle;
  command->icon.color = color;
  /* reset clipping if it was set */
  if (clipped)
  {
    mu_set_clip(context, unclipped_rect);
  }
}

/*============================================================================
** layout
**============================================================================*/

enum
{
  RELATIVE = 1,
  ABSOLUTE = 2
};

void mu_layout_begin_column(mu_Context *context)
{
  push_layout(context, mu_layout_next(context), mu_vec2(0, 0));
}

void mu_layout_end_column(mu_Context *context)
{
  mu_Layout *a, *b;
  b = get_layout(context);
  pop(context->layout_stack);
  /* inherit position/next_row/max from child layout if they are greater */
  a = get_layout(context);
  a->position.x = mu_max(a->position.x, b->position.x + b->body.x - a->body.x);
  a->next_row = mu_max(a->next_row, b->next_row + b->body.y - a->body.y);
  a->max.x = mu_max(a->max.x, b->max.x);
  a->max.y = mu_max(a->max.y, b->max.y);
}

void mu_layout_row(mu_Context *context, int items, const int *widths, int height)
{
  mu_Layout *layout = get_layout(context);
  if (widths)
  {
    expect(items <= MU_MAX_WIDTHS);
    memcpy(layout->widths, widths, items * sizeof(widths[0]));
  }
  layout->items = items;
  layout->position = mu_vec2(layout->indentation, layout->next_row);
  layout->size.y = height;
  layout->item_index = 0;
}

void mu_layout_width(mu_Context *context, int width)
{
  get_layout(context)->size.x = width;
}

void mu_layout_height(mu_Context *context, int height)
{
  get_layout(context)->size.y = height;
}

void mu_layout_set_next(mu_Context *context, mu_Rectangle renderer, int relative)
{
  mu_Layout *layout = get_layout(context);
  layout->next = renderer;
  layout->next_type = relative ? RELATIVE : ABSOLUTE;
}

mu_Rectangle mu_layout_next(mu_Context *context)
{
  mu_Layout *layout = get_layout(context);
  mu_Style *style = context->style;
  mu_Rectangle res;

  if (layout->next_type)
  {
    /* handle rectangle set by `mu_layout_set_next` */
    int type = layout->next_type;
    layout->next_type = 0;
    res = layout->next;
    if (type == ABSOLUTE)
    {
      return (context->last_rect = res);
    }
  }
  else
  {
    /* handle next row */
    if (layout->item_index == layout->items)
    {
      mu_layout_row(context, layout->items, NULL, layout->size.y);
    }

    /* position */
    res.x = layout->position.x;
    res.y = layout->position.y;

    /* size */
    res.w = layout->items > 0 ? layout->widths[layout->item_index] : layout->size.x;
    res.h = layout->size.y;
    if (res.w == 0)
    {
      res.w = style->size.x + style->padding * 2;
    }
    if (res.h == 0)
    {
      res.h = style->size.y + style->padding * 2;
    }
    if (res.w < 0)
    {
      res.w += layout->body.w - res.x + 1;
    }
    if (res.h < 0)
    {
      res.h += layout->body.h - res.y + 1;
    }

    layout->item_index++;
  }

  /* update position */
  layout->position.x += res.w + style->spacing;
  layout->next_row = mu_max(layout->next_row, res.y + res.h + style->spacing);

  /* apply body offset */
  res.x += layout->body.x;
  res.y += layout->body.y;

  /* update max position */
  layout->max.x = mu_max(layout->max.x, res.x + res.w);
  layout->max.y = mu_max(layout->max.y, res.y + res.h);

  return (context->last_rect = res);
}

/*============================================================================
** controls
**============================================================================*/

static int in_hover_root(mu_Context *context)
{
  int i = context->container_stack.idx;
  while (i--)
  {
    if (context->container_stack.items[i] == context->hover_root)
    {
      return 1;
    }
    /* only root containers have their `head` field set; stop searching if we've
    ** reached the current root container */
    if (context->container_stack.items[i]->head)
    {
      break;
    }
  }
  return 0;
}

void mu_draw_control_frame(mu_Context *context, mu_Identifier identifier, mu_Rectangle rectangle,
                           int colorid, int opt)
{
  if (opt & MU_OPT_NOFRAME)
  {
    return;
  }
  colorid += (context->focus == identifier) ? 2 : (context->hover == identifier) ? 1
                                                                         : 0;
  context->draw_frame(context, rectangle, colorid);
}

void mu_draw_control_text(mu_Context *context, const char *str, mu_Rectangle rectangle,
                          int colorid, int opt)
{
  mu_Vector2 position;
  mu_Font font = context->style->font;
  int tw = context->text_width(font, str, -1);
  mu_push_clip_rect(context, rectangle);
  position.y = rectangle.y + (rectangle.h - context->text_height(font)) / 2;
  if (opt & MU_OPT_ALIGNCENTER)
  {
    position.x = rectangle.x + (rectangle.w - tw) / 2;
  }
  else if (opt & MU_OPT_ALIGNRIGHT)
  {
    position.x = rectangle.x + rectangle.w - tw - context->style->padding;
  }
  else
  {
    position.x = rectangle.x + context->style->padding;
  }
  mu_draw_text(context, font, str, -1, position, context->style->colors[colorid]);
  mu_pop_clip_rect(context);
}

int mu_mouse_over(mu_Context *context, mu_Rectangle rectangle)
{
  return rect_overlaps_vec2(rectangle, context->mouse_pos) &&
         rect_overlaps_vec2(mu_get_clip_rect(context), context->mouse_pos) &&
         in_hover_root(context);
}

void mu_update_control(mu_Context *context, mu_Identifier identifier, mu_Rectangle rectangle, int opt)
{
  int mouseover = mu_mouse_over(context, rectangle);

  if (context->focus == identifier)
  {
    context->updated_focus = 1;
  }
  if (opt & MU_OPT_NOINTERACT)
  {
    return;
  }
  if (mouseover && !context->mouse_down)
  {
    context->hover = identifier;
  }

  if (context->focus == identifier)
  {
    if (context->mouse_pressed && !mouseover)
    {
      mu_set_focus(context, 0);
    }
    if (!context->mouse_down && ~opt & MU_OPT_HOLDFOCUS)
    {
      mu_set_focus(context, 0);
    }
  }

  if (context->hover == identifier)
  {
    if (context->mouse_pressed)
    {
      mu_set_focus(context, identifier);
    }
    else if (!mouseover)
    {
      context->hover = 0;
    }
  }
}

void mu_text(mu_Context *context, const char *text)
{
  const char *start, *end, *p = text;
  int width = -1;
  mu_Font font = context->style->font;
  mu_Color color = context->style->colors[MU_COLOR_TEXT];
  mu_layout_begin_column(context);
  mu_layout_row(context, 1, &width, context->text_height(font));
  do
  {
    mu_Rectangle renderer = mu_layout_next(context);
    int w = 0;
    start = end = p;
    do
    {
      const char *word = p;
      while (*p && *p != ' ' && *p != '\n')
      {
        p++;
      }
      w += context->text_width(font, word, p - word);
      if (w > renderer.w && end != start)
      {
        break;
      }
      w += context->text_width(font, p, 1);
      end = p++;
    } while (*end && *end != '\n');
    mu_draw_text(context, font, start, end - start, mu_vec2(renderer.x, renderer.y), color);
    p = end + 1;
  } while (*end);
  mu_layout_end_column(context);
}

void mu_label(mu_Context *context, const char *text)
{
  mu_draw_control_text(context, text, mu_layout_next(context), MU_COLOR_TEXT, 0);
}

int mu_button_ex(mu_Context *context, const char *label, int icon, int opt)
{
  int res = 0;
  mu_Identifier identifier = label ? mu_get_id(context, label, strlen(label))
                                   : mu_get_id(context, &icon, sizeof(icon));
  mu_Rectangle renderer = mu_layout_next(context);
  mu_update_control(context, identifier, renderer, opt);
  /* handle click */
  if (context->mouse_pressed == MU_MOUSE_LEFT && context->focus == identifier)
  {
    res |= MU_RES_SUBMIT;
  }
  /* draw */
  mu_draw_control_frame(context, identifier, renderer, MU_COLOR_BUTTON, opt);
  if (label)
  {
    mu_draw_control_text(context, label, renderer, MU_COLOR_TEXT, opt);
  }
  if (icon)
  {
    mu_draw_icon(context, icon, renderer, context->style->colors[MU_COLOR_TEXT]);
  }
  return res;
}

int mu_checkbox(mu_Context *context, const char *label, int *state)
{
  int res = 0;
  mu_Identifier identifier = mu_get_id(context, &state, sizeof(state));
  mu_Rectangle renderer = mu_layout_next(context);
  mu_Rectangle box = mu_rect(renderer.x, renderer.y, renderer.h, renderer.h);
  mu_update_control(context, identifier, renderer, 0);
  /* handle click */
  if (context->mouse_pressed == MU_MOUSE_LEFT && context->focus == identifier)
  {
    res |= MU_RES_CHANGE;
    *state = !*state;
  }
  /* draw */
  mu_draw_control_frame(context, identifier, box, MU_COLOR_BASE, 0);
  if (*state)
  {
    mu_draw_icon(context, MU_ICON_CHECK, box, context->style->colors[MU_COLOR_TEXT]);
  }
  renderer = mu_rect(renderer.x + box.w, renderer.y, renderer.w - box.w, renderer.h);
  mu_draw_control_text(context, label, renderer, MU_COLOR_TEXT, 0);
  return res;
}

int mu_textbox_raw(mu_Context *context, char *buffer, int bufsz, mu_Identifier identifier, mu_Rectangle renderer,
                   int opt)
{
  int res = 0;
  mu_update_control(context, identifier, renderer, opt | MU_OPT_HOLDFOCUS);

  if (context->focus == identifier)
  {
    /* handle text input */
    int length = strlen(buffer);
    int n = mu_min(bufsz - length - 1, (int)strlen(context->input_text));
    if (n > 0)
    {
      memcpy(buffer + length, context->input_text, n);
      length += n;
      buffer[length] = '\0';
      res |= MU_RES_CHANGE;
    }
    /* handle backspace */
    if (context->key_pressed & MU_KEY_BACKSPACE && length > 0)
    {
      /* skip utf-8 continuation bytes */
      while ((buffer[--length] & 0xc0) == 0x80 && length > 0)
        ;
      buffer[length] = '\0';
      res |= MU_RES_CHANGE;
    }
    /* handle return */
    if (context->key_pressed & MU_KEY_RETURN)
    {
      mu_set_focus(context, 0);
      res |= MU_RES_SUBMIT;
    }
  }

  /* draw */
  mu_draw_control_frame(context, identifier, renderer, MU_COLOR_BASE, opt);
  if (context->focus == identifier)
  {
    mu_Color color = context->style->colors[MU_COLOR_TEXT];
    mu_Font font = context->style->font;
    int textw = context->text_width(font, buffer, -1);
    int texth = context->text_height(font);
    int ofx = renderer.w - context->style->padding - textw - 1;
    int textx = renderer.x + mu_min(ofx, context->style->padding);
    int texty = renderer.y + (renderer.h - texth) / 2;
    mu_push_clip_rect(context, renderer);
    mu_draw_text(context, font, buffer, -1, mu_vec2(textx, texty), color);
    mu_draw_rect(context, mu_rect(textx + textw, texty, 1, texth), color);
    mu_pop_clip_rect(context);
  }
  else
  {
    mu_draw_control_text(context, buffer, renderer, MU_COLOR_TEXT, opt);
  }

  return res;
}

static int number_textbox(mu_Context *context, mu_Real *value, mu_Rectangle renderer, mu_Identifier identifier)
{
  if (context->mouse_pressed == MU_MOUSE_LEFT && context->key_down & MU_KEY_SHIFT &&
      context->hover == identifier)
  {
    context->number_edit = identifier;
    sprintf(context->number_edit_buf, MU_REAL_FMT, *value);
  }
  if (context->number_edit == identifier)
  {
    int res = mu_textbox_raw(
        context, context->number_edit_buf, sizeof(context->number_edit_buf), identifier, renderer, 0);
    if (res & MU_RES_SUBMIT || context->focus != identifier)
    {
      *value = strtod(context->number_edit_buf, NULL);
      context->number_edit = 0;
    }
    else
    {
      return 1;
    }
  }
  return 0;
}

int mu_textbox_ex(mu_Context *context, char *buffer, int bufsz, int opt)
{
  mu_Identifier identifier = mu_get_id(context, &buffer, sizeof(buffer));
  mu_Rectangle renderer = mu_layout_next(context);
  return mu_textbox_raw(context, buffer, bufsz, identifier, renderer, opt);
}

int mu_slider_ex(mu_Context *context, mu_Real *value, mu_Real low, mu_Real high,
                 mu_Real step, const char *fmt, int opt)
{
  char buffer[MU_MAX_FMT + 1];
  mu_Rectangle thumb;
  int x, w, res = 0;
  mu_Real last = *value, v = last;
  mu_Identifier identifier = mu_get_id(context, &value, sizeof(value));
  mu_Rectangle base = mu_layout_next(context);

  /* handle text input mode */
  if (number_textbox(context, &v, base, identifier))
  {
    return res;
  }

  /* handle normal mode */
  mu_update_control(context, identifier, base, opt);

  /* handle input */
  if (context->focus == identifier &&
      (context->mouse_down | context->mouse_pressed) == MU_MOUSE_LEFT)
  {
    v = low + (context->mouse_pos.x - base.x) * (high - low) / base.w;
    if (step)
    {
      v = ((long long)((v + step / 2) / step)) * step;
    }
  }
  /* clamp and store value, update res */
  *value = v = mu_clamp(v, low, high);
  if (last != v)
  {
    res |= MU_RES_CHANGE;
  }

  /* draw base */
  mu_draw_control_frame(context, identifier, base, MU_COLOR_BASE, opt);
  /* draw thumb */
  w = context->style->thumb_size;
  x = (v - low) * (base.w - w) / (high - low);
  thumb = mu_rect(base.x + x, base.y, w, base.h);
  mu_draw_control_frame(context, identifier, thumb, MU_COLOR_BUTTON, opt);
  /* draw text  */
  sprintf(buffer, fmt, v);
  mu_draw_control_text(context, buffer, base, MU_COLOR_TEXT, opt);

  return res;
}

int mu_number_ex(mu_Context *context, mu_Real *value, mu_Real step,
                 const char *fmt, int opt)
{
  char buffer[MU_MAX_FMT + 1];
  int res = 0;
  mu_Identifier identifier = mu_get_id(context, &value, sizeof(value));
  mu_Rectangle base = mu_layout_next(context);
  mu_Real last = *value;

  /* handle text input mode */
  if (number_textbox(context, value, base, identifier))
  {
    return res;
  }

  /* handle normal mode */
  mu_update_control(context, identifier, base, opt);

  /* handle input */
  if (context->focus == identifier && context->mouse_down == MU_MOUSE_LEFT)
  {
    *value += context->mouse_delta.x * step;
  }
  /* set flag if value changed */
  if (*value != last)
  {
    res |= MU_RES_CHANGE;
  }

  /* draw base */
  mu_draw_control_frame(context, identifier, base, MU_COLOR_BASE, opt);
  /* draw text  */
  sprintf(buffer, fmt, *value);
  mu_draw_control_text(context, buffer, base, MU_COLOR_TEXT, opt);

  return res;
}

static int header(mu_Context *context, const char *label, int istreenode, int opt)
{
  mu_Rectangle renderer;
  int active, expanded;
  mu_Identifier identifier = mu_get_id(context, label, strlen(label));
  int idx = mu_pool_get(context, context->treenode_pool, MU_TREENODEPOOL_SIZE, identifier);
  int width = -1;
  mu_layout_row(context, 1, &width, 0);

  active = (idx >= 0);
  expanded = (opt & MU_OPT_EXPANDED) ? !active : active;
  renderer = mu_layout_next(context);
  mu_update_control(context, identifier, renderer, 0);

  /* handle click */
  active ^= (context->mouse_pressed == MU_MOUSE_LEFT && context->focus == identifier);

  /* update pool ref */
  if (idx >= 0)
  {
    if (active)
    {
      mu_pool_update(context, context->treenode_pool, idx);
    }
    else
    {
      memset(&context->treenode_pool[idx], 0, sizeof(mu_PoolItem));
    }
  }
  else if (active)
  {
    mu_pool_init(context, context->treenode_pool, MU_TREENODEPOOL_SIZE, identifier);
  }

  /* draw */
  if (istreenode)
  {
    if (context->hover == identifier)
    {
      context->draw_frame(context, renderer, MU_COLOR_BUTTONHOVER);
    }
  }
  else
  {
    mu_draw_control_frame(context, identifier, renderer, MU_COLOR_BUTTON, 0);
  }
  mu_draw_icon(
      context, expanded ? MU_ICON_EXPANDED : MU_ICON_COLLAPSED,
      mu_rect(renderer.x, renderer.y, renderer.h, renderer.h), context->style->colors[MU_COLOR_TEXT]);
  renderer.x += renderer.h - context->style->padding;
  renderer.w -= renderer.h - context->style->padding;
  mu_draw_control_text(context, label, renderer, MU_COLOR_TEXT, 0);

  return expanded ? MU_RES_ACTIVE : 0;
}

int mu_header_ex(mu_Context *context, const char *label, int opt)
{
  return header(context, label, 0, opt);
}

int mu_begin_treenode_ex(mu_Context *context, const char *label, int opt)
{
  int res = header(context, label, 1, opt);
  if (res & MU_RES_ACTIVE)
  {
    get_layout(context)->indentation += context->style->indentation;
    push(context->id_stack, context->last_identifier);
  }
  return res;
}

void mu_end_treenode(mu_Context *context)
{
  get_layout(context)->indentation -= context->style->indentation;
  mu_pop_id(context);
}

#define scrollbar(context, cnt, b, cs, x, y, w, h)                              \
  do                                                                        \
  {                                                                         \
    /* only add scrollbar if content size is larger than body */            \
    int maxscroll = cs.y - b->h;                                            \
                                                                            \
    if (maxscroll > 0 && b->h > 0)                                          \
    {                                                                       \
      mu_Rectangle base, thumb;                                             \
      mu_Identifier identifier = mu_get_id(context, "!scrollbar" #y, 11);       \
                                                                            \
      /* get sizing / positioning */                                        \
      base = *b;                                                            \
      base.x = b->x + b->w;                                                 \
      base.w = context->style->scrollbar_size;                                  \
                                                                            \
      /* handle input */                                                    \
      mu_update_control(context, identifier, base, 0);                          \
      if (context->focus == identifier && context->mouse_down == MU_MOUSE_LEFT)     \
      {                                                                     \
        cnt->scroll.y += context->mouse_delta.y * cs.y / base.h;                \
      }                                                                     \
      /* clamp scroll to limits */                                          \
      cnt->scroll.y = mu_clamp(cnt->scroll.y, 0, maxscroll);                \
                                                                            \
      /* draw base and thumb */                                             \
      context->draw_frame(context, base, MU_COLOR_SCROLLBASE);                      \
      thumb = base;                                                         \
      thumb.h = mu_max(context->style->thumb_size, base.h * b->h / cs.y);       \
      thumb.y += cnt->scroll.y * (base.h - thumb.h) / maxscroll;            \
      context->draw_frame(context, thumb, MU_COLOR_SCROLLTHUMB);                    \
                                                                            \
      /* set this as the scroll_target (will get scrolled on mousewheel) */ \
      /* if the mouse is over it */                                         \
      if (mu_mouse_over(context, *b))                                           \
      {                                                                     \
        context->scroll_target = cnt;                                           \
      }                                                                     \
    }                                                                       \
    else                                                                    \
    {                                                                       \
      cnt->scroll.y = 0;                                                    \
    }                                                                       \
  } while (0)

static void scrollbars(mu_Context *context, mu_Container *cnt, mu_Rectangle *body)
{
  int sz = context->style->scrollbar_size;
  mu_Vector2 cs = cnt->content_size;
  cs.x += context->style->padding * 2;
  cs.y += context->style->padding * 2;
  mu_push_clip_rect(context, *body);
  /* resize body to make room for scrollbars */
  if (cs.y > cnt->body.h)
  {
    body->w -= sz;
  }
  if (cs.x > cnt->body.w)
  {
    body->h -= sz;
  }
  /* to create a horizontal or vertical scrollbar almost-identical code is
  ** used; only the references to `x|y` `w|h` need to be switched */
  scrollbar(context, cnt, body, cs, x, y, w, h);
  scrollbar(context, cnt, body, cs, y, x, h, w);
  mu_pop_clip_rect(context);
}

static void push_container_body(
    mu_Context *context, mu_Container *cnt, mu_Rectangle body, int opt)
{
  if (~opt & MU_OPT_NOSCROLL)
  {
    scrollbars(context, cnt, &body);
  }
  push_layout(context, expand_rect(body, -context->style->padding), cnt->scroll);
  cnt->body = body;
}

static void begin_root_container(mu_Context *context, mu_Container *cnt)
{
  push(context->container_stack, cnt);
  /* push container to roots list and push head command */
  push(context->root_list, cnt);
  cnt->head = push_jump(context, NULL);
  /* set as hover root if the mouse is overlapping this container and it has a
  ** higher zindex than the current hover root */
  if (rect_overlaps_vec2(cnt->rectangle, context->mouse_pos) &&
      (!context->next_hover_root || cnt->zindex > context->next_hover_root->zindex))
  {
    context->next_hover_root = cnt;
  }
  /* clipping is reset here in case a root-container is made within
  ** another root-containers's begin/end block; this prevents the inner
  ** root-container being clipped to the outer */
  push(context->clip_stack, unclipped_rect);
}

static void end_root_container(mu_Context *context)
{
  /* push tail 'goto' jump command and set head 'skip' command. the final steps
  ** on initing these are done in mu_end() */
  mu_Container *cnt = mu_get_current_container(context);
  cnt->tail = push_jump(context, NULL);
  cnt->head->jump.dst = context->command_list.items + context->command_list.idx;
  /* pop base clip rectangle and container */
  mu_pop_clip_rect(context);
  pop_container(context);
}

int mu_begin_window_ex(mu_Context *context, const char *title, mu_Rectangle rectangle, int opt)
{
  mu_Rectangle body;
  mu_Identifier identifier = mu_get_id(context, title, strlen(title));
  mu_Container *cnt = get_container(context, identifier, opt);
  if (!cnt || !cnt->open)
  {
    return 0;
  }
  push(context->id_stack, identifier);

  if (cnt->rectangle.w == 0)
  {
    cnt->rectangle = rectangle;
  }
  begin_root_container(context, cnt);
  rectangle = body = cnt->rectangle;

  /* draw frame */
  if (~opt & MU_OPT_NOFRAME)
  {
    context->draw_frame(context, rectangle, MU_COLOR_WINDOWBG);
  }

  /* do title bar */
  if (~opt & MU_OPT_NOTITLE)
  {
    mu_Rectangle tr = rectangle;
    tr.h = context->style->title_height;
    context->draw_frame(context, tr, MU_COLOR_TITLEBG);

    /* do title text */
    if (~opt & MU_OPT_NOTITLE)
    {
      mu_Identifier identifier = mu_get_id(context, "!title", 6);
      mu_update_control(context, identifier, tr, opt);
      mu_draw_control_text(context, title, tr, MU_COLOR_TITLETEXT, opt);
      if (identifier == context->focus && context->mouse_down == MU_MOUSE_LEFT)
      {
        cnt->rectangle.x += context->mouse_delta.x;
        cnt->rectangle.y += context->mouse_delta.y;
      }
      body.y += tr.h;
      body.h -= tr.h;
    }

    /* do `close` button */
    if (~opt & MU_OPT_NOCLOSE)
    {
      mu_Identifier identifier = mu_get_id(context, "!close", 6);
      mu_Rectangle renderer = mu_rect(tr.x + tr.w - tr.h, tr.y, tr.h, tr.h);
      tr.w -= renderer.w;
      mu_draw_icon(context, MU_ICON_CLOSE, renderer, context->style->colors[MU_COLOR_TITLETEXT]);
      mu_update_control(context, identifier, renderer, opt);
      if (context->mouse_pressed == MU_MOUSE_LEFT && identifier == context->focus)
      {
        cnt->open = 0;
      }
    }
  }

  push_container_body(context, cnt, body, opt);

  /* do `resize` handle */
  if (~opt & MU_OPT_NORESIZE)
  {
    int sz = context->style->title_height;
    mu_Identifier identifier = mu_get_id(context, "!resize", 7);
    mu_Rectangle renderer = mu_rect(rectangle.x + rectangle.w - sz, rectangle.y + rectangle.h - sz, sz, sz);
    mu_update_control(context, identifier, renderer, opt);
    if (identifier == context->focus && context->mouse_down == MU_MOUSE_LEFT)
    {
      cnt->rectangle.w = mu_max(96, cnt->rectangle.w + context->mouse_delta.x);
      cnt->rectangle.h = mu_max(64, cnt->rectangle.h + context->mouse_delta.y);
    }
  }

  /* resize to content size */
  if (opt & MU_OPT_AUTOSIZE)
  {
    mu_Rectangle renderer = get_layout(context)->body;
    cnt->rectangle.w = cnt->content_size.x + (cnt->rectangle.w - renderer.w);
    cnt->rectangle.h = cnt->content_size.y + (cnt->rectangle.h - renderer.h);
  }

  /* close if this is a popup window and elsewhere was clicked */
  if (opt & MU_OPT_POPUP && context->mouse_pressed && context->hover_root != cnt)
  {
    cnt->open = 0;
  }

  mu_push_clip_rect(context, cnt->body);
  return MU_RES_ACTIVE;
}

void mu_end_window(mu_Context *context)
{
  mu_pop_clip_rect(context);
  end_root_container(context);
}

void mu_open_popup(mu_Context *context, const char *name)
{
  mu_Container *cnt = mu_get_container(context, name);
  /* set as hover root so popup isn't closed in begin_window_ex()  */
  context->hover_root = context->next_hover_root = cnt;
  /* position at mouse cursor, open and bring-to-front */
  cnt->rectangle = mu_rect(context->mouse_pos.x, context->mouse_pos.y, 1, 1);
  cnt->open = 1;
  mu_bring_to_front(context, cnt);
}

int mu_begin_popup(mu_Context *context, const char *name)
{
  int opt = MU_OPT_POPUP | MU_OPT_AUTOSIZE | MU_OPT_NORESIZE |
            MU_OPT_NOSCROLL | MU_OPT_NOTITLE | MU_OPT_CLOSED;
  return mu_begin_window_ex(context, name, mu_rect(0, 0, 0, 0), opt);
}

void mu_end_popup(mu_Context *context)
{
  mu_end_window(context);
}

void mu_begin_panel_ex(mu_Context *context, const char *name, int opt)
{
  mu_Container *cnt;
  mu_push_id(context, name, strlen(name));
  cnt = get_container(context, context->last_identifier, opt);
  cnt->rectangle = mu_layout_next(context);
  if (~opt & MU_OPT_NOFRAME)
  {
    context->draw_frame(context, cnt->rectangle, MU_COLOR_PANELBG);
  }
  push(context->container_stack, cnt);
  push_container_body(context, cnt, cnt->rectangle, opt);
  mu_push_clip_rect(context, cnt->body);
}

void mu_end_panel(mu_Context *context)
{
  mu_pop_clip_rect(context);
  pop_container(context);
}
