/*
** Copyright (c) 2024 rxi
**
** This library is free software; you can redistribute it and/or modify it
** under the terms of the MIT license. See `microui.c` for details.
*/

/**
 * @file microui.h
 * @brief MicroUI - A tiny, portable, immediate-mode UI library written in ANSI C
 *
 * MicroUI is a minimal UI framework designed for embedded applications and games.
 * It provides core UI controls (buttons, sliders, text boxes, windows, etc.) without
 * allocating any dynamic memory beyond a fixed-size buffer provided by the user.
 *
 * The library uses an immediate-mode API where UI is specified in code each frame,
 * and the library generates drawing commands that the user can render using their
 * own graphics system.
 *
 * Key features:
 * - Tiny: ~1100 lines of ANSI C
 * - No dynamic allocation (uses fixed-size memory regions)
 * - Works with any rendering system
 * - Immediate-mode API (UI specified each frame)
 * - Built-in controls: windows, panels, buttons, sliders, text boxes, checkboxes, etc.
 * - Customizable layout system
 * - Easy to extend with custom controls
 */

#ifndef MICROUI_H
#define MICROUI_H

/** @brief Version string for MicroUI */
#define MU_VERSION "2.02"

/** @defgroup Config Configuration Macros
 * @brief Memory and buffer size configuration constants
 * @{
 */

/** @brief Maximum size of the command list buffer (256 KB) */
#define MU_COMMANDLIST_SIZE (256 * 1024)
/** @brief Maximum number of root containers */
#define MU_ROOTLIST_SIZE 32
/** @brief Maximum depth of nested containers */
#define MU_CONTAINERSTACK_SIZE 32
/** @brief Maximum depth of clipping rectangle stack */
#define MU_CLIPSTACK_SIZE 32
/** @brief Maximum depth of ID stack for widget identification */
#define MU_IDSTACK_SIZE 32
/** @brief Maximum depth of layout state stack */
#define MU_LAYOUTSTACK_SIZE 16
/** @brief Maximum number of retained containers (windows, panels) */
#define MU_CONTAINERPOOL_SIZE 48
/** @brief Maximum number of retained tree node states */
#define MU_TREENODEPOOL_SIZE 48
/** @brief Maximum number of column widths in a single layout row */
#define MU_MAX_WIDTHS 16

/** @brief Data type for floating-point values (can be float or double) */
#define MU_REAL float
/** @brief Format string for printing floating-point values */
#define MU_REAL_FMT "%.3g"
/** @brief Format string for slider values */
#define MU_SLIDER_FMT "%.2f"
/** @brief Maximum length for formatted number strings */
#define MU_MAX_FMT 127

/** @} */

/** @defgroup Macros Utility Macros
 * @brief Commonly used helper macros
 * @{
 */

/** @brief Define a bounded stack structure for type T with n items */
#define mu_stack(T, n) \
  struct               \
  {                    \
    int idx;           \
    T items[n];        \
  }
/** @brief Get minimum of two values */
#define mu_min(a, b) ((a) < (b) ? (a) : (b))
/** @brief Get maximum of two values */
#define mu_max(a, b) ((a) > (b) ? (a) : (b))
/** @brief Clamp a value between minimum and maximum */
#define mu_clamp(x, a, b) mu_min(b, mu_max(a, x))

/** @} */

/** @defgroup Enums Enumeration Types
 * @brief Constants for various UI properties and states
 * @{
 */

/** @brief Clipping behavior flags */
enum
{
  MU_CLIP_PART = 1, /**< Partial clipping - apply clip command */
  MU_CLIP_ALL       /**< Full clipping - skip rendering entirely */
};

/** @brief Drawing command types */
enum
{
  MU_COMMAND_JUMP = 1, /**< Jump to another command list location */
  MU_COMMAND_CLIP,     /**< Set clipping rectangle */
  MU_COMMAND_RECT,     /**< Draw filled rectangle */
  MU_COMMAND_TEXT,     /**< Draw text string */
  MU_COMMAND_ICON,     /**< Draw icon */
  MU_COMMAND_MAX       /**< Sentinel value */
};

/** @brief Color indices for theme customization */
enum
{
  MU_COLOR_TEXT,        /**< Text color */
  MU_COLOR_BORDER,      /**< Border/outline color */
  MU_COLOR_WINDOWBG,    /**< Window background color */
  MU_COLOR_TITLEBG,     /**< Window title bar background */
  MU_COLOR_TITLETEXT,   /**< Window title text color */
  MU_COLOR_PANELBG,     /**< Panel background color */
  MU_COLOR_BUTTON,      /**< Button color (normal) */
  MU_COLOR_BUTTONHOVER, /**< Button color (hovered) */
  MU_COLOR_BUTTONFOCUS, /**< Button color (focused) */
  MU_COLOR_BASE,        /**< Base/input widget color */
  MU_COLOR_BASEHOVER,   /**< Base widget color (hovered) */
  MU_COLOR_BASEFOCUS,   /**< Base widget color (focused) */
  MU_COLOR_SCROLLBASE,  /**< Scrollbar background color */
  MU_COLOR_SCROLLTHUMB, /**< Scrollbar thumb/slider color */
  MU_COLOR_MAX          /**< Sentinel value */
};

/** @brief Icon identifiers */
enum
{
  MU_ICON_CLOSE = 1, /**< Window close button icon */
  MU_ICON_CHECK,     /**< Checkbox checked state icon */
  MU_ICON_COLLAPSED, /**< Treenode collapsed state icon */
  MU_ICON_EXPANDED,  /**< Treenode expanded state icon */
  MU_ICON_MAX        /**< Sentinel value */
};

/** @brief Widget result flags - indicate what happened to a widget this frame */
enum
{
  MU_RES_ACTIVE = (1 << 0), /**< Widget is currently active/focused */
  MU_RES_SUBMIT = (1 << 1), /**< User submitted the widget (e.green., pressed Enter) */
  MU_RES_CHANGE = (1 << 2)  /**< Widget value changed this frame */
};

/** @brief Widget option flags - control widget behavior and appearance */
enum
{
  MU_OPT_ALIGNCENTER = (1 << 0), /**< Center text horizontally */
  MU_OPT_ALIGNRIGHT = (1 << 1),  /**< Align text to right */
  MU_OPT_NOINTERACT = (1 << 2),  /**< Widget doesn't respond to input */
  MU_OPT_NOFRAME = (1 << 3),     /**< Don't draw frame/background */
  MU_OPT_NORESIZE = (1 << 4),    /**< Window cannot be resized */
  MU_OPT_NOSCROLL = (1 << 5),    /**< Disable scrollbars */
  MU_OPT_NOCLOSE = (1 << 6),     /**< Hide close button */
  MU_OPT_NOTITLE = (1 << 7),     /**< Hide title bar */
  MU_OPT_HOLDFOCUS = (1 << 8),   /**< Keep focus after mouse release */
  MU_OPT_AUTOSIZE = (1 << 9),    /**< Auto-size window to content */
  MU_OPT_POPUP = (1 << 10),      /**< Window is a popup (closes on outside click) */
  MU_OPT_CLOSED = (1 << 11),     /**< Window starts closed */
  MU_OPT_EXPANDED = (1 << 12)    /**< Treenode starts expanded */
};

/** @brief Mouse button flags */
enum
{
  MU_MOUSE_LEFT = (1 << 0),  /**< Left mouse button */
  MU_MOUSE_RIGHT = (1 << 1), /**< Right mouse button */
  MU_MOUSE_MIDDLE = (1 << 2) /**< Middle mouse button */
};

/** @brief Keyboard modifier and key flags */
enum
{
  MU_KEY_SHIFT = (1 << 0),     /**< Shift key */
  MU_KEY_CTRL = (1 << 1),      /**< Control key */
  MU_KEY_ALT = (1 << 2),       /**< Alt key */
  MU_KEY_BACKSPACE = (1 << 3), /**< Backspace key */
  MU_KEY_RETURN = (1 << 4)     /**< Return/Enter key */
};

/** @} */

/** @defgroup Types Type Definitions
 * @brief Core data structures and types
 * @{
 */

typedef struct mu_Context mu_Context; /**< Forward declaration of UI context */
typedef unsigned mu_Identifier;       /**< Widget unique identifier type */
typedef MU_REAL mu_Real;              /**< Floating-point type for values */
typedef void *mu_Font;                /**< Opaque font handle */

/** @brief 2D vector with integer coordinates */
typedef struct
{
  int x, y;
} mu_Vector2;

/** @brief 2D rectangle with integer coordinates and dimensions */
typedef struct
{
  int x, y, w, h;
} mu_Rectangle;

/** @brief RGBA color value */
typedef struct
{
  unsigned char red, green, blue, alpha;
} mu_Color;

/** @brief Pool item - tracks retained widget state with timestamps */
typedef struct
{
  mu_Identifier identifier;
  int last_update;
} mu_PoolItem;

/* Command structures - for drawing commands generated by the UI */

/** @brief Base command structure - shared by all command types */
typedef struct
{
  int type, size;
} mu_BaseCommand;

/** @brief Jump command - skips to another location in command list */
typedef struct
{
  mu_BaseCommand base;
  void *dst;
} mu_JumpCommand;

/** @brief Clipping rectangle command */
typedef struct
{
  mu_BaseCommand base;
  mu_Rectangle rectangle;
} mu_ClipCommand;

/** @brief Filled rectangle drawing command */
typedef struct
{
  mu_BaseCommand base;
  mu_Rectangle rectangle;
  mu_Color color;
} mu_RectCommand;

/** @brief Text drawing command */
typedef struct
{
  mu_BaseCommand base;
  mu_Font font;
  mu_Vector2 position;
  mu_Color color;
  char str[1];
} mu_TextCommand;

/** @brief Icon drawing command */
typedef struct
{
  mu_BaseCommand base;
  mu_Rectangle rectangle;
  int identifier;
  mu_Color color;
} mu_IconCommand;

/** @brief Union of all command types for polymorphic access */
typedef union
{
  int type;
  mu_BaseCommand base;
  mu_JumpCommand jump;
  mu_ClipCommand clip;
  mu_RectCommand rectangle;
  mu_TextCommand text;
  mu_IconCommand icon;
} mu_Command;

/** @brief Layout state - tracks positioning and sizing of widgets in a container */
typedef struct
{
  mu_Rectangle body;         /**< Current layout area */
  mu_Rectangle next;         /**< Next widget position (when using mu_layout_set_next) */
  mu_Vector2 position;       /**< Current position for next widget */
  mu_Vector2 size;           /**< Size of current item */
  mu_Vector2 max;            /**< Maximum position reached by any widget */
  int widths[MU_MAX_WIDTHS]; /**< Column widths for row layout */
  int items;                 /**< Number of items in current row */
  int item_index;            /**< Current item index in row */
  int next_row;              /**< Y position of next row */
  int next_type;             /**< Type of next positioning (absolute/relative) */
  int indentation;           /**< Current indentation level */
} mu_Layout;

/** @brief Container - represents a window, panel, or popup */
typedef struct
{
  mu_Command *head, *tail; /**< Head and tail commands for this container */
  mu_Rectangle rectangle;  /**< Container bounds */
  mu_Rectangle body;       /**< Content area (excluding title bar, scrollbars) */
  mu_Vector2 content_size; /**< Size of all content within container */
  mu_Vector2 scroll;       /**< Current scroll offset */
  int zindex;              /**< Drawing order (higher = drawn last) */
  int open;                /**< Whether container is visible */
} mu_Container;

/** @brief Style/theme configuration - colors, fonts, sizes */
typedef struct
{
  mu_Font font;                  /**< Font for text rendering */
  mu_Vector2 size;               /**< Default widget size */
  int padding;                   /**< Padding inside widgets */
  int spacing;                   /**< Spacing between widgets */
  int indentation;               /**< Indentation per nesting level */
  int title_height;              /**< Height of window title bar */
  int scrollbar_size;            /**< Width of scrollbars */
  int thumb_size;                /**< Minimum scrollbar thumb height */
  mu_Color colors[MU_COLOR_MAX]; /**< Color palette */
} mu_Style;

/** @brief Main UI context - holds all state for the UI system
 *
 * This structure should be initialized once and reused across all frames.
 * It maintains input state, widget state, command buffers, and layout information.
 */
struct mu_Context
{
  /* Callbacks - these must be set by the user */

  /** @brief Callback to measure text width
   * @param font Font to measure with
   * @param str String to measure
   * @param length Length of string (-1 for null-terminated)
   * @return Width in pixels
   */
  int (*text_width)(mu_Font font, const char *str, int length);

  /** @brief Callback to get text height
   * @param font Font to measure
   * @return Height in pixels
   */
  int (*text_height)(mu_Font font);

  /** @brief Callback to draw a styled frame
   * @param context UI context
   * @param rectangle Frame rectangle
   * @param colorid Color index from mu_Color enums
   */
  void (*draw_frame)(mu_Context *context, mu_Rectangle rectangle, int colorid);

  /* Core state */
  mu_Style _style;                  /**< Default style (internal) */
  mu_Style *style;                  /**< Current active style */
  mu_Identifier hover;              /**< ID of widget under mouse cursor */
  mu_Identifier focus;              /**< ID of focused/active widget */
  mu_Identifier last_identifier;    /**< ID of last created widget */
  mu_Rectangle last_rect;           /**< Rectangle of last widget */
  int last_zindex;                  /**< Z-index of last container */
  int updated_focus;                /**< Whether focus was updated this frame */
  int frame;                        /**< Current frame number */
  mu_Container *hover_root;         /**< Root container under mouse */
  mu_Container *next_hover_root;    /**< Root container to be under mouse next */
  mu_Container *scroll_target;      /**< Container to receive scroll input */
  char number_edit_buf[MU_MAX_FMT]; /**< Buffer for number editing */
  mu_Identifier number_edit;        /**< ID of widget currently editing number */

  /* Stacks - for managing nested state */
  mu_stack(char, MU_COMMANDLIST_SIZE) command_list;                 /**< Drawing command buffer */
  mu_stack(mu_Container *, MU_ROOTLIST_SIZE) root_list;             /**< Root containers */
  mu_stack(mu_Container *, MU_CONTAINERSTACK_SIZE) container_stack; /**< Nested containers */
  mu_stack(mu_Rectangle, MU_CLIPSTACK_SIZE) clip_stack;             /**< Clipping rectangles */
  mu_stack(mu_Identifier, MU_IDSTACK_SIZE) id_stack;                /**< ID generation stack */
  mu_stack(mu_Layout, MU_LAYOUTSTACK_SIZE) layout_stack;            /**< Layout state */

  /* Retained state pools - maintains state across frames */
  mu_PoolItem container_pool[MU_CONTAINERPOOL_SIZE]; /**< Container state tracking */
  mu_Container containers[MU_CONTAINERPOOL_SIZE];    /**< Container objects */
  mu_PoolItem treenode_pool[MU_TREENODEPOOL_SIZE];   /**< Tree node state tracking */

  /* Input state - updated by input callbacks */
  mu_Vector2 mouse_pos;      /**< Current mouse position */
  mu_Vector2 last_mouse_pos; /**< Previous frame mouse position */
  mu_Vector2 mouse_delta;    /**< Mouse movement this frame */
  mu_Vector2 scroll_delta;   /**< Mouse wheel scroll this frame */
  int mouse_down;            /**< Currently pressed mouse buttons */
  int mouse_pressed;         /**< Mouse buttons pressed this frame */
  int key_down;              /**< Currently pressed keys */
  int key_pressed;           /**< Keys pressed this frame */
  char input_text[32];       /**< Text input this frame */
};

/** @} */

/* ========================================================================
 * PUBLIC API - Core Functions
 * ======================================================================== */

/** @defgroup Core Core Functions
 * @brief Main UI context and state management
 * @{
 */

/** @brief Create a 2D vector
 * @param x X coordinate
 * @param y Y coordinate
 * @return Vector with given coordinates
 */
mu_Vector2 mu_vec2(int x, int y);

/** @brief Create a rectangle
 * @param x X coordinate
 * @param y Y coordinate
 * @param w Width
 * @param h Height
 * @return Rectangle with given bounds
 */
mu_Rectangle mu_rect(int x, int y, int w, int h);

/** @brief Create a color
 * @param renderer Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @param a Alpha component (0-255)
 * @return Color with given RGBA values
 */
mu_Color mu_color(int renderer, int g, int b, int a);

/** @brief Initialize a UI context
 *
 * Must be called once before using the context. The context must persist
 * for the lifetime of the UI. After calling this, set the text_width,
 * text_height, and draw_frame callbacks.
 *
 * @param context UI context to initialize
 */
void mu_init(mu_Context *context);

/** @brief Begin a new UI frame
 *
 * Must be called at the start of each frame before processing any UI.
 * Clears input state and prepares for new widgets.
 *
 * @param context UI context
 */
void mu_begin(mu_Context *context);

/** @brief End the current UI frame
 *
 * Must be called after processing all UI elements. Finalizes the command
 * list and cleans up temporary state.
 *
 * @param context UI context
 */
void mu_end(mu_Context *context);

/** @brief Set focus to a specific widget
 * @param context UI context
 * @param identifier Widget ID to focus (0 to unfocus)
 */
void mu_set_focus(mu_Context *context, mu_Identifier identifier);

/** @brief Generate a unique ID from arbitrary data
 * @param context UI context
 * @param data Data to hash
 * @param size Size of data
 * @return Generated ID
 */
mu_Identifier mu_get_id(mu_Context *context, const void *data, int size);

/** @brief Push an ID onto the ID stack for widget hierarchies
 * @param context UI context
 * @param data Data to include in ID
 * @param size Size of data
 */
void mu_push_id(mu_Context *context, const void *data, int size);

/** @brief Pop an ID from the ID stack
 * @param context UI context
 */
void mu_pop_id(mu_Context *context);

/** @brief Push a clipping rectangle
 * @param context UI context
 * @param rectangle Clipping bounds
 */
void mu_push_clip_rect(mu_Context *context, mu_Rectangle rectangle);

/** @brief Pop a clipping rectangle
 * @param context UI context
 */
void mu_pop_clip_rect(mu_Context *context);

/** @brief Get the current clipping rectangle
 * @param context UI context
 * @return Current clip rectangle
 */
mu_Rectangle mu_get_clip_rect(mu_Context *context);

/** @brief Check if a rectangle overlaps with the current clip rectangle
 * @param context UI context
 * @param renderer Rectangle to test
 * @return MU_CLIP_ALL, MU_CLIP_PART, or 0
 */
int mu_check_clip(mu_Context *context, mu_Rectangle renderer);

/** @brief Get the current container
 * @param context UI context
 * @return Current container pointer
 */
mu_Container *mu_get_current_container(mu_Context *context);

/** @brief Get a container by name
 * @param context UI context
 * @param name Container name
 * @return Container pointer or NULL
 */
mu_Container *mu_get_container(mu_Context *context, const char *name);

/** @brief Bring a container to front (highest z-index)
 * @param context UI context
 * @param cnt Container to bring to front
 */
void mu_bring_to_front(mu_Context *context, mu_Container *cnt);

/** @} */

/** @defgroup Pool Retained State Pool Functions
 * @brief Manage persistent widget state across frames
 * @{
 */

/** @brief Initialize a pool item with an ID
 * @param context UI context
 * @param items Pool item array
 * @param length Array length
 * @param identifier ID to register
 * @return Index of initialized item or -1
 */
int mu_pool_init(mu_Context *context, mu_PoolItem *items, int length, mu_Identifier identifier);

/** @brief Get a pool item by ID
 * @param context UI context
 * @param items Pool item array
 * @param length Array length
 * @param identifier ID to look up
 * @return Index of pool item or -1
 */
int mu_pool_get(mu_Context *context, mu_PoolItem *items, int length, mu_Identifier identifier);

/** @brief Update the timestamp of a pool item
 * @param context UI context
 * @param items Pool item array
 * @param idx Index of item to update
 */
void mu_pool_update(mu_Context *context, mu_PoolItem *items, int idx);

/** @} */

/** @defgroup Input Input Handling Functions
 * @brief Pass user input to the UI system
 *
 * These functions should be called once per frame for each input event.
 * @{
 */

/** @brief Report mouse position change
 * @param context UI context
 * @param x New X position
 * @param y New Y position
 */
void mu_input_mousemove(mu_Context *context, int x, int y);

/** @brief Report mouse button press
 * @param context UI context
 * @param x X position
 * @param y Y position
 * @param btn Button flag (MU_MOUSE_LEFT, etc.)
 */
void mu_input_mousedown(mu_Context *context, int x, int y, int btn);

/** @brief Report mouse button release
 * @param context UI context
 * @param x X position
 * @param y Y position
 * @param btn Button flag (MU_MOUSE_LEFT, etc.)
 */
void mu_input_mouseup(mu_Context *context, int x, int y, int btn);

/** @brief Report mouse wheel scroll
 * @param context UI context
 * @param x X position
 * @param y Y position (positive = scroll up)
 */
void mu_input_scroll(mu_Context *context, int x, int y);

/** @brief Report key press
 * @param context UI context
 * @param key Key flag (MU_KEY_SHIFT, etc.)
 */
void mu_input_keydown(mu_Context *context, int key);

/** @brief Report key release
 * @param context UI context
 * @param key Key flag (MU_KEY_SHIFT, etc.)
 */
void mu_input_keyup(mu_Context *context, int key);

/** @brief Report text input
 * @param context UI context
 * @param text Text string to add
 */
void mu_input_text(mu_Context *context, const char *text);

/** @} */

/** @defgroup Commands Command Functions
 * @brief Generate and iterate drawing commands
 * @{
 */

/** @brief Add a drawing command to the command list
 * @param context UI context
 * @param type Command type (MU_COMMAND_*)
 * @param size Size of command structure
 * @return Pointer to new command
 */
mu_Command *mu_push_command(mu_Context *context, int type, int size);

/** @brief Get next drawing command from the list
 * @param context UI context
 * @param command Current command (NULL to get first)
 * @return 1 if a valid command was retrieved, 0 if at end of list
 */
int mu_next_command(mu_Context *context, mu_Command **command);

/** @brief Set the clipping rectangle for subsequent drawing
 * @param context UI context
 * @param rectangle Clipping bounds
 */
void mu_set_clip(mu_Context *context, mu_Rectangle rectangle);

/** @brief Queue a filled rectangle to be drawn
 * @param context UI context
 * @param rectangle Rectangle bounds
 * @param color Fill color
 */
void mu_draw_rect(mu_Context *context, mu_Rectangle rectangle, mu_Color color);

/** @brief Queue a rectangle outline to be drawn
 * @param context UI context
 * @param rectangle Rectangle bounds
 * @param color Outline color
 */
void mu_draw_box(mu_Context *context, mu_Rectangle rectangle, mu_Color color);

/** @brief Queue text to be drawn
 * @param context UI context
 * @param font Font to use
 * @param str Text string
 * @param length String length (-1 for null-terminated)
 * @param position Position
 * @param color Text color
 */
void mu_draw_text(mu_Context *context, mu_Font font, const char *str, int length, mu_Vector2 position, mu_Color color);

/** @brief Queue an icon to be drawn
 * @param context UI context
 * @param identifier Icon ID (MU_ICON_*)
 * @param rectangle Icon bounds
 * @param color Icon color
 */
void mu_draw_icon(mu_Context *context, int identifier, mu_Rectangle rectangle, mu_Color color);

/** @} */

/** @defgroup Layout Layout Functions
 * @brief Control widget positioning and sizing
 * @{
 */

/** @brief Begin a row with specified column widths
 * @param context UI context
 * @param items Number of items in row
 * @param widths Array of column widths (negative = fill remaining space)
 * @param height Row height (0 = auto)
 */
void mu_layout_row(mu_Context *context, int items, const int *widths, int height);

/** @brief Set width for the next row
 * @param context UI context
 * @param width Width in pixels
 */
void mu_layout_width(mu_Context *context, int width);

/** @brief Set height for items
 * @param context UI context
 * @param height Height in pixels
 */
void mu_layout_height(mu_Context *context, int height);

/** @brief Begin a column layout
 * @param context UI context
 */
void mu_layout_begin_column(mu_Context *context);

/** @brief End a column layout
 * @param context UI context
 */
void mu_layout_end_column(mu_Context *context);

/** @brief Set the position of the next widget
 * @param context UI context
 * @param renderer Rectangle for next widget
 * @param relative If 1, relative to current position; if 0, absolute
 */
void mu_layout_set_next(mu_Context *context, mu_Rectangle renderer, int relative);

/** @brief Get the rectangle for the next widget
 * @param context UI context
 * @return Rectangle for next widget
 */
mu_Rectangle mu_layout_next(mu_Context *context);

/** @} */

/** @defgroup Control Control Functions
 * @brief Drawing and interaction for widgets
 * @{
 */

/** @brief Draw a styled control frame
 * @param context UI context
 * @param identifier Widget ID
 * @param rectangle Widget bounds
 * @param colorid Base color ID
 * @param opt Options (MU_OPT_*)
 */
void mu_draw_control_frame(mu_Context *context, mu_Identifier identifier, mu_Rectangle rectangle, int colorid, int opt);

/** @brief Draw text for a control
 * @param context UI context
 * @param str Text to draw
 * @param rectangle Control bounds
 * @param colorid Text color ID
 * @param opt Options (MU_OPT_ALIGN*)
 */
void mu_draw_control_text(mu_Context *context, const char *str, mu_Rectangle rectangle, int colorid, int opt);

/** @brief Check if mouse is over a rectangle
 * @param context UI context
 * @param rectangle Rectangle to test
 * @return 1 if mouse is over the rectangle, 0 otherwise
 */
int mu_mouse_over(mu_Context *context, mu_Rectangle rectangle);

/** @brief Update control interaction state
 * @param context UI context
 * @param identifier Widget ID
 * @param rectangle Widget bounds
 * @param opt Options (MU_OPT_*)
 */
void mu_update_control(mu_Context *context, mu_Identifier identifier, mu_Rectangle rectangle, int opt);

/** @} */

/* ========================================================================
 * PUBLIC API - Widget Functions (with macro shortcuts)
 * ======================================================================== */

/** @defgroup Widgets Widget Functions
 * @brief High-level UI widgets for user interaction
 * @{
 */

/** @brief Display non-interactive text with word wrapping
 * @param context UI context
 * @param text Text to display
 */
void mu_text(mu_Context *context, const char *text);

/** @brief Display a label (non-interactive text)
 * @param context UI context
 * @param text Label text
 */
void mu_label(mu_Context *context, const char *text);

/** @brief Create a clickable button with extended options
 * @param context UI context
 * @param label Button text
 * @param icon Optional icon ID (0 for none)
 * @param opt Options (MU_OPT_*)
 * @return Result flags (MU_RES_*)
 */
int mu_button_ex(mu_Context *context, const char *label, int icon, int opt);

/** @brief Macro: Create a centered button
 * @param context UI context
 * @param label Button text
 * @return Result flags (MU_RES_*)
 */
#define mu_button(context, label) mu_button_ex(context, label, 0, MU_OPT_ALIGNCENTER)

/** @brief Create a checkbox for boolean state
 * @param context UI context
 * @param label Label text
 * @param state Pointer to boolean state
 * @return Result flags (MU_RES_*)
 */
int mu_checkbox(mu_Context *context, const char *label, int *state);

/** @brief Create a text input box (low-level version)
 * @param context UI context
 * @param buffer Text buffer
 * @param bufsz Buffer size
 * @param identifier Widget ID
 * @param renderer Widget bounds
 * @param opt Options (MU_OPT_*)
 * @return Result flags (MU_RES_*)
 */
int mu_textbox_raw(mu_Context *context, char *buffer, int bufsz, mu_Identifier identifier, mu_Rectangle renderer, int opt);

/** @brief Create a text input box with extended options
 * @param context UI context
 * @param buffer Text buffer
 * @param bufsz Buffer size
 * @param opt Options (MU_OPT_*)
 * @return Result flags (MU_RES_*)
 */
int mu_textbox_ex(mu_Context *context, char *buffer, int bufsz, int opt);

/** @brief Macro: Create a standard text input box
 * @param context UI context
 * @param buffer Text buffer
 * @param bufsz Buffer size
 * @return Result flags (MU_RES_*)
 */
#define mu_textbox(context, buffer, bufsz) mu_textbox_ex(context, buffer, bufsz, 0)

/** @brief Create a slider for numeric value selection with extended options
 * @param context UI context
 * @param value Pointer to value
 * @param low Minimum value
 * @param high Maximum value
 * @param step Step size (0 for continuous)
 * @param fmt Format string for value display
 * @param opt Options (MU_OPT_*)
 * @return Result flags (MU_RES_*)
 */
int mu_slider_ex(mu_Context *context, mu_Real *value, mu_Real low, mu_Real high, mu_Real step, const char *fmt, int opt);

/** @brief Macro: Create a standard slider
 * @param context UI context
 * @param value Pointer to value
 * @param lo Minimum value
 * @param hi Maximum value
 * @return Result flags (MU_RES_*)
 */
#define mu_slider(context, value, lo, hi) mu_slider_ex(context, value, lo, hi, 0, MU_SLIDER_FMT, MU_OPT_ALIGNCENTER)

/** @brief Create a number input with extended options
 * @param context UI context
 * @param value Pointer to value
 * @param step Step size for mouse drag adjustment
 * @param fmt Format string for value display
 * @param opt Options (MU_OPT_*)
 * @return Result flags (MU_RES_*)
 */
int mu_number_ex(mu_Context *context, mu_Real *value, mu_Real step, const char *fmt, int opt);

/** @brief Macro: Create a standard number input
 * @param context UI context
 * @param value Pointer to value
 * @param step Step size
 * @return Result flags (MU_RES_*)
 */
#define mu_number(context, value, step) mu_number_ex(context, value, step, MU_SLIDER_FMT, MU_OPT_ALIGNCENTER)

/** @brief Create a collapsible header with extended options
 * @param context UI context
 * @param label Header text
 * @param opt Options (MU_OPT_*)
 * @return MU_RES_ACTIVE if expanded
 */
int mu_header_ex(mu_Context *context, const char *label, int opt);

/** @brief Macro: Create a standard header
 * @param context UI context
 * @param label Header text
 * @return MU_RES_ACTIVE if expanded
 */
#define mu_header(context, label) mu_header_ex(context, label, 0)

/** @brief Create a tree node (collapsible section) with extended options
 * @param context UI context
 * @param label Node label
 * @param opt Options (MU_OPT_*)
 * @return MU_RES_ACTIVE if expanded
 */
int mu_begin_treenode_ex(mu_Context *context, const char *label, int opt);

/** @brief Macro: Create a standard tree node
 * @param context UI context
 * @param label Node label
 * @return MU_RES_ACTIVE if expanded
 */
#define mu_begin_treenode(context, label) mu_begin_treenode_ex(context, label, 0)

/** @brief End a tree node
 * @param context UI context
 */
void mu_end_treenode(mu_Context *context);

/** @brief Begin a window with extended options
 * @param context UI context
 * @param title Window title
 * @param rectangle Window bounds
 * @param opt Options (MU_OPT_*)
 * @return MU_RES_ACTIVE if window is open
 */
int mu_begin_window_ex(mu_Context *context, const char *title, mu_Rectangle rectangle, int opt);

/** @brief Macro: Create a standard window
 * @param context UI context
 * @param title Window title
 * @param rectangle Window bounds
 * @return MU_RES_ACTIVE if window is open
 */
#define mu_begin_window(context, title, rectangle) mu_begin_window_ex(context, title, rectangle, 0)

/** @brief End a window
 * @param context UI context
 */
void mu_end_window(mu_Context *context);

/** @brief Open a popup window at mouse position
 * @param context UI context
 * @param name Popup name
 */
void mu_open_popup(mu_Context *context, const char *name);

/** @brief Begin rendering a popup
 * @param context UI context
 * @param name Popup name
 * @return MU_RES_ACTIVE if popup is open
 */
int mu_begin_popup(mu_Context *context, const char *name);

/** @brief End a popup
 * @param context UI context
 */
void mu_end_popup(mu_Context *context);

/** @brief Begin a scrollable panel with extended options
 * @param context UI context
 * @param name Panel name
 * @param opt Options (MU_OPT_*)
 */
void mu_begin_panel_ex(mu_Context *context, const char *name, int opt);

/** @brief Macro: Create a standard panel
 * @param context UI context
 * @param name Panel name
 */
#define mu_begin_panel(context, name) mu_begin_panel_ex(context, name, 0)

/** @brief End a panel
 * @param context UI context
 */
void mu_end_panel(mu_Context *context);

/** @} */

#endif
