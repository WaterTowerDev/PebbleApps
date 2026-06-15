#include <pebble.h>
#include <stdio.h>

// ============================================================================
// PERSISTENT STORAGE KEYS
// ============================================================================
#define STORAGE_KEY_STROKES 1
#define STORAGE_KEY_TOTAL_HOLES 2
#define STORAGE_KEY_CURRENT_HOLE 3
#define STORAGE_KEY_IN_PROGRESS 4

// ============================================================================
// UI WINDOWS AND LAYERS
// ============================================================================
static Window *s_main_window;
static Window *s_menu_window;
static Window *s_summary_window;
static TextLayer *s_hole_layer;
static TextLayer *s_stroke_layer;
static TextLayer *s_instruction_layer;
static ScrollLayer *s_summary_scroll_layer;
static TextLayer *s_summary_text_layer;
static TextLayer *s_menu_title_layer;
static TextLayer *s_menu_option_layer;

// ============================================================================
// APPLICATION STATE VARIABLES
// ============================================================================
static int s_current_hole = 1;
static int s_current_strokes = 0;
static int s_total_holes = 18;  // Default to 18 holes
static bool s_game_in_progress = false;

// Array to store stroke counts for each hole (max 18)
static int s_hole_strokes[18] = {0};

// Buffers to hold the text strings dynamically
static char s_hole_buffer[32];
static char s_stroke_buffer[32];
static char s_instruction_buffer[64];
static char s_summary_buffer[512];

// ============================================================================
// PERSISTENT STORAGE FUNCTIONS
// ============================================================================

static void save_game_state() {
  // Save stroke counts
  persist_write_data(STORAGE_KEY_STROKES, s_hole_strokes, sizeof(s_hole_strokes));
  // Save total holes
  persist_write_int(STORAGE_KEY_TOTAL_HOLES, s_total_holes);
  // Save current hole
  persist_write_int(STORAGE_KEY_CURRENT_HOLE, s_current_hole);
  // Save game in progress flag
  persist_write_bool(STORAGE_KEY_IN_PROGRESS, s_game_in_progress);
}

static void load_game_state() {
  if (persist_exists(STORAGE_KEY_IN_PROGRESS)) {
    s_game_in_progress = persist_read_bool(STORAGE_KEY_IN_PROGRESS);
    if (s_game_in_progress) {
      persist_read_data(STORAGE_KEY_STROKES, s_hole_strokes, sizeof(s_hole_strokes));
      s_total_holes = persist_read_int(STORAGE_KEY_TOTAL_HOLES);
      s_current_hole = persist_read_int(STORAGE_KEY_CURRENT_HOLE);
      s_current_strokes = s_hole_strokes[s_current_hole - 1];
    }
  }
}

static void reset_game() {
  s_current_hole = 1;
  s_current_strokes = 0;
  s_game_in_progress = false;
  for (int i = 0; i < 18; i++) {
    s_hole_strokes[i] = 0;
  }
}

// ============================================================================
// UI UPDATE FUNCTIONS
// ============================================================================

static void update_score_ui() {
  snprintf(s_hole_buffer, sizeof(s_hole_buffer), "Hole %d / %d", s_current_hole, s_total_holes);
  text_layer_set_text(s_hole_layer, s_hole_buffer);

  snprintf(s_stroke_buffer, sizeof(s_stroke_buffer), "%d", s_current_strokes);
  text_layer_set_text(s_stroke_layer, s_stroke_buffer);

  snprintf(s_instruction_buffer, sizeof(s_instruction_buffer), 
           "UP/DOWN: Strokes | SELECT: Next | BACK: Previous");
  text_layer_set_text(s_instruction_layer, s_instruction_buffer);
}

static void generate_summary_text() {
  int total_strokes = 0;
  snprintf(s_summary_buffer, sizeof(s_summary_buffer), "=== GAME SUMMARY ===\n\n");
  
  for (int i = 0; i < s_total_holes; i++) {
    total_strokes += s_hole_strokes[i];
    char hole_line[32];
    snprintf(hole_line, sizeof(hole_line), "Hole %d: %d\n", i + 1, s_hole_strokes[i]);
    strncat(s_summary_buffer, hole_line, sizeof(s_summary_buffer) - strlen(s_summary_buffer) - 1);
  }
  
  char total_line[64];
  snprintf(total_line, sizeof(total_line), "\n--- TOTAL: %d ---\n", total_strokes);
  strncat(s_summary_buffer, total_line, sizeof(s_summary_buffer) - strlen(s_summary_buffer) - 1);
}

// ============================================================================
// CLICK HANDLERS - PLAY SCREEN
// ============================================================================

// UP button: increase strokes
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_current_strokes++;
  s_hole_strokes[s_current_hole - 1] = s_current_strokes;
  update_score_ui();
  save_game_state();
}

// DOWN button: decrease strokes (floor at 0)
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_current_strokes > 0) {
    s_current_strokes--;
    s_hole_strokes[s_current_hole - 1] = s_current_strokes;
    update_score_ui();
    save_game_state();
  }
}

// SELECT button: advance to next hole
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_current_hole >= s_total_holes) {
    // Game finished - show summary
    s_game_in_progress = false;
    save_game_state();
    window_stack_push(s_summary_window, true);
  } else {
    // Move to next hole
    s_current_hole++;
    s_current_strokes = s_hole_strokes[s_current_hole - 1];
    update_score_ui();
    save_game_state();
  }
}

// BACK button (long press on SELECT): go to previous hole
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_current_hole > 1) {
    s_current_hole--;
    s_current_strokes = s_hole_strokes[s_current_hole - 1];
    update_score_ui();
    save_game_state();
  }
}

// Configures the button clicks to map to handlers
static void play_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 500, select_long_click_handler, NULL);
}

// ============================================================================
// PLAY SCREEN WINDOW HANDLERS
// ============================================================================

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // 1. Configure the Hole Text Layer (Top part of screen)
  s_hole_layer = text_layer_create(GRect(0, 10, bounds.size.w, 35));
  text_layer_set_background_color(s_hole_layer, GColorClear);
  text_layer_set_text_color(s_hole_layer, GColorBlack);
  text_layer_set_font(s_hole_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_hole_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_hole_layer));

  // 2. Configure the Stroke Text Layer (Large number in center)
  s_stroke_layer = text_layer_create(GRect(0, 50, bounds.size.w, 70));
  text_layer_set_background_color(s_stroke_layer, GColorClear);
  text_layer_set_text_color(s_stroke_layer, GColorDarkGreen);
  text_layer_set_font(s_stroke_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(s_stroke_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_stroke_layer));

  // 3. Configure the Instructions Text Layer (Bottom)
  s_instruction_layer = text_layer_create(GRect(0, 130, bounds.size.w, 38));
  text_layer_set_background_color(s_instruction_layer, GColorClear);
  text_layer_set_text_color(s_instruction_layer, GColorBlack);
  text_layer_set_font(s_instruction_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_instruction_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_instruction_layer, GTextOverflowModeWordWrap);
  layer_add_child(window_layer, text_layer_get_layer(s_instruction_layer));

  // Initialize display
  update_score_ui();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_hole_layer);
  text_layer_destroy(s_stroke_layer);
  text_layer_destroy(s_instruction_layer);
}

// ============================================================================
// MENU SCREEN (SELECT 9 OR 18 HOLES)
// ============================================================================

static void menu_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_total_holes = 18;
  s_game_in_progress = true;
  save_game_state();
  
  // Transition seamlessly to the score tracker
  window_stack_push(s_main_window, true);
  window_stack_remove(s_menu_window, false); 
}

static void menu_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_total_holes = 9;
  s_game_in_progress = true;
  save_game_state();
  
  // Transition seamlessly to the score tracker
  window_stack_push(s_main_window, true);
  window_stack_remove(s_menu_window, false); 
}

static void menu_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Do nothing - up/down makes the selection
}

static void menu_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, menu_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, menu_down_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, menu_select_click_handler);
}

static void menu_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_menu_title_layer = text_layer_create(GRect(0, 20, bounds.size.w, 50));
  text_layer_set_background_color(s_menu_title_layer, GColorClear);
  text_layer_set_text_color(s_menu_title_layer, GColorBlack);
  text_layer_set_font(s_menu_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_menu_title_layer, GTextAlignmentCenter);
  text_layer_set_text(s_menu_title_layer, "New Game");
  layer_add_child(window_layer, text_layer_get_layer(s_menu_title_layer));

  s_menu_option_layer = text_layer_create(GRect(0, 80, bounds.size.w, 60));
  text_layer_set_background_color(s_menu_option_layer, GColorClear);
  text_layer_set_text_color(s_menu_option_layer, GColorBlack);
  text_layer_set_font(s_menu_option_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_menu_option_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_menu_option_layer, GTextOverflowModeWordWrap);
  text_layer_set_text(s_menu_option_layer, "UP: 18 Holes\nDOWN: 9 Holes");
  layer_add_child(window_layer, text_layer_get_layer(s_menu_option_layer));
}

static void menu_window_unload(Window *window) {
  text_layer_destroy(s_menu_title_layer);
  text_layer_destroy(s_menu_option_layer);
}

// ============================================================================
// SUMMARY SCREEN (GAME REVIEW)
// ============================================================================

static void summary_back_click_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_pop(true);
}

static void summary_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Start a new game - go to menu
  reset_game();
  window_stack_pop(true);
}

static void summary_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_BACK, summary_back_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, summary_select_click_handler);
}

static void summary_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  generate_summary_text();

  s_summary_scroll_layer = scroll_layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  scroll_layer_set_shadow_hidden(s_summary_scroll_layer, true);

  s_summary_text_layer = text_layer_create(GRect(0, 0, bounds.size.w, 2000));
  text_layer_set_background_color(s_summary_text_layer, GColorClear);
  text_layer_set_text_color(s_summary_text_layer, GColorBlack);
  text_layer_set_font(s_summary_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(s_summary_text_layer, s_summary_buffer);
  text_layer_set_overflow_mode(s_summary_text_layer, GTextOverflowModeWordWrap);

  scroll_layer_add_child(s_summary_scroll_layer, text_layer_get_layer(s_summary_text_layer));
  layer_add_child(window_layer, scroll_layer_get_layer(s_summary_scroll_layer));
}

static void summary_window_unload(Window *window) {
  scroll_layer_destroy(s_summary_scroll_layer);
  text_layer_destroy(s_summary_text_layer);
}

// ============================================================================
// MAIN WINDOW SETUP
// ============================================================================

static void init() {
  // Load saved game state
  load_game_state();

  // Create main play window
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_set_click_config_provider(s_main_window, play_click_config_provider);

  // Create menu window for game mode selection
  s_menu_window = window_create();
  window_set_window_handlers(s_menu_window, (WindowHandlers) {
    .load = menu_window_load,
    .unload = menu_window_unload
  });
  window_set_click_config_provider(s_menu_window, menu_click_config_provider);

  // Create summary window for game review
  s_summary_window = window_create();
  window_set_window_handlers(s_summary_window, (WindowHandlers) {
    .load = summary_window_load,
    .unload = summary_window_unload
  });
  window_set_click_config_provider(s_summary_window, summary_click_config_provider);

  // If game in progress, show play screen; otherwise show menu
  if (s_game_in_progress) {
    window_stack_push(s_main_window, true);
  } else {
    reset_game();
    window_stack_push(s_menu_window, true);
  }
}

static void deinit() {
  window_destroy(s_main_window);
  window_destroy(s_menu_window);
  window_destroy(s_summary_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
  return 0;
}
