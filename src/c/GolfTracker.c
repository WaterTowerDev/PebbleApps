#include <pebble.h>
#include <stdio.h>

static Window *s_main_window;
static TextLayer *s_hole_layer;
static TextLayer *s_stroke_layer;

// Application state variables
static int s_current_hole = 1;
static int s_current_strokes = 0;

// Buffers to hold the text strings dynamically
static char s_hole_buffer[16];
static char s_stroke_buffer[16];

// Updates the UI text elements with current values
static void update_score_ui() {
  snprintf(s_hole_buffer, sizeof(s_hole_buffer), "Hole %d", s_current_hole);
  text_layer_set_text(s_hole_layer, s_hole_buffer);

  snprintf(s_stroke_buffer, sizeof(s_stroke_buffer), "%d", s_current_strokes);
  text_layer_set_text(s_stroke_layer, s_stroke_buffer);
}

// Click handler: UP button increases strokes
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_current_strokes++;
  update_score_ui();
}

// Click handler: DOWN button decreases strokes (floor at 0)
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_current_strokes > 0) {
    s_current_strokes--;
    update_score_ui();
  }
}

// Click handler: SELECT button advances to the next hole
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Clear strokes for the next hole
  s_current_strokes = 0;
  // Loop back after 18 holes, or simply increment
  if (s_current_hole >= 18) {
    s_current_hole = 1;
  } else {
    s_current_hole++;
  }
  update_score_ui();
}

// Configures the button clicks to map to handlers
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

// UI Setup layout when the Window loads
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // 1. Configure the Hole Text Layer (Top part of screen)
  s_hole_layer = text_layer_create(GRect(0, 20, bounds.size.w, 40));
  text_layer_set_background_color(s_hole_layer, GColorClear);
  text_layer_set_text_color(s_hole_layer, GColorBlack);
  text_layer_set_font(s_hole_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_hole_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_hole_layer));

  // 2. Configure the Stroke Text Layer (Large number in center)
  s_stroke_layer = text_layer_create(GRect(0, 65, bounds.size.w, 70));
  text_layer_set_background_color(s_stroke_layer, GColorClear);
  text_layer_set_text_color(s_stroke_layer, GColorDarkGreen); // Pebble Time 2 supports color!
  text_layer_set_font(s_stroke_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(s_stroke_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_stroke_layer));

  // Initialize display
  update_score_ui();
}

// Memory clean up when app unloads
static void main_window_unload(Window *window) {
  text_layer_destroy(s_hole_layer);
  text_layer_destroy(s_stroke_layer);
}

static void init() {
  s_main_window = window_create();

  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Attach button click configs to the window
  window_set_click_config_provider(s_main_window, click_config_provider);

  // Push the window onto the visual stack, animated
  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
