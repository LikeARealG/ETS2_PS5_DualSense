#define NOMINMAX
#include <iostream>
#include <Windows.h>
#include <string>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <fstream>
#include <sstream>
#include <cctype>
#include <vector>
#include "ds5w.h"
#include "scs-telemetry-common.hpp"

const LPCWSTR ETS2_SHARED_MEMORY_NAME = SCS_PLUGIN_MMF_NAME;
const float KMPH_CONVERSION = 3.6f;
const float PI = 3.14159265359f;
const float IDLE_RPM = 550.0f;
// Animation phase for the moving road dashes
static int road_anim_phase = 0;

void set_cursor_position(int x, int y);
void set_console_cursor_visibility(bool visible);

struct DisplayData {
    uint8_t leftRumble = 0;
    uint8_t rightRumble = 0;
    uint8_t lt_force = 0;
    uint8_t rt_freq = 0;
    std::string micLedState = "OFF";
    DS5W::Color lightbar = { 0, 0, 0 };
    bool fine_alert_active = false;
    bool gear_jolt_active = false;
    bool engine_cranking_active = false;
    bool startup_lurch_active = false;
    bool hard_braking_active = false;
    bool body_roll_active = false;
    bool refueling_active = false;
    bool fined = false;
    bool hard_brake = false;
    bool low_fuel = false;
    bool body_roll = false;
    bool engine_rumble = false;
    bool gear_jolt = false;
    bool braking_lightbar = false;
    float speed;
    float accel_x;
    float fuel;
    float fuel_capacity;
    float rpm;
    float body_roll_angle;
    int gear;
    int retarder;
    bool left_blinker;
    bool right_blinker;
};

struct TruckTemplate {
    std::vector<std::string> lines;
    int contentStartCol = -1;
    int contentEndCol = -1;
    int contentStartRow = -1;
    int contentEndRow = -1;
};

static void overlay_text_into_truck(std::vector<std::string>& frame, const TruckTemplate& tt, const std::vector<std::string>& textLines);
static void animate_road_in_frame(std::vector<std::string>& frame, const TruckTemplate& tt, int phase);

struct AppConfig {
    bool fine_alert = true;
    bool park_brake_lightbar = true;
    bool retarder_lightbar = true;
    bool blinkers_lightbar = true;
    bool warnings_mic_led = true;
    bool fuel_player_leds = true;
    bool refuel_rumble = true;
    float refuel_rumble_multiplier = 1.0f;
    bool gear_jolt = true;
    float gear_jolt_multiplier = 1.0f;
    bool engine_start_effects = true; // cranking + startup lurch
    float engine_start_multiplier = 1.0f;
    bool hard_braking_rumble = true;
    float hard_braking_multiplier = 1.0f;
    bool body_roll_rumble = true;
    float body_roll_multiplier = 1.0f;
    bool brake_trigger_resistance = true;
    float brake_trigger_resistance_multiplier = 1.0f;
    bool throttle_trigger_vibration = true;
    float throttle_trigger_vibration_multiplier = 1.0f;
};

static std::string trim_copy(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) start++;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) end--;
    return s.substr(start, end - start);
}

// Animate the road line by creating a moving gap of dashes (only affecting '-' characters).
// A gap of width 5 moves left-to-right across the positions that contain dashes.
static void animate_road_in_frame(std::vector<std::string>& frame, const TruckTemplate& tt, int phase) {
    if (tt.lines.empty()) return;

    // Find the row that contains the road marker (contains the sequence `---')
    const std::string road_marker = "`---'";
    int road_row = -1;
    for (int r = 0; r < static_cast<int>(frame.size()); ++r) {
        if (frame[r].find(road_marker) != std::string::npos) { road_row = r; break; }
    }
    if (road_row < 0) return;

    const std::string& orig = frame[road_row];
    std::vector<int> dash_positions;
    for (int i = 0; i < static_cast<int>(orig.size()); ++i) {
        if (orig[i] == '-') dash_positions.push_back(i);
    }
    if (dash_positions.empty()) return;

    // Pattern: 10 white dashes followed by 5 gray dashes (group=15, gray_count=5).
    // Use phase to shift the pattern across the dash positions.
    const int group = 15;
    const int gray_count = 5;

    // Precompute which dash indices should be gray
    std::vector<char> is_gray(dash_positions.size(), 0);
    for (size_t di = 0; di < dash_positions.size(); ++di) {
        int idx = static_cast<int>(di);
        int v = (idx - phase) % group; // invert phase to change movement direction
        if (v < 0) v += group; // ensure non-negative remainder
        // gray dashes are the last 'gray_count' in each group (10 white then 5 gray)
        if (v >= (group - gray_count)) is_gray[di] = 1;
    }

    // Rebuild the line, inserting ANSI gray for selected dashes
    std::string rebuilt;
    rebuilt.reserve(orig.size() * 2);
    size_t next_dash_idx = 0;
    for (size_t i = 0; i < orig.size(); ++i) {
        char c = orig[i];
        if (c == '-') {
            if (next_dash_idx < is_gray.size() && is_gray[next_dash_idx]) {
                rebuilt += "\033[90m-\033[0m"; // gray dash
            }
            else {
                rebuilt.push_back('-');
            }
            next_dash_idx++;
        }
        else {
            rebuilt.push_back(c);
        }
    }

    frame[road_row] = rebuilt;
}

static void enable_ansi_colors() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode))
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

static void print_truck_line(const std::string& line) {
    // Color the opening '(' on the truck's second row and the first '[' and final ']' on
    // the axle/wheel row. Detection uses unique substrings so only those rows are affected.
    const std::string wheel_row_marker = "[__.'.---.   |[Y";
    const std::string cab_row_marker = "(>_____.----'||";
    bool is_wheel_row = (line.find(wheel_row_marker) != std::string::npos);
    bool is_cab_row = (line.find(cab_row_marker) != std::string::npos);
    size_t first_bracket_pos = std::string::npos;
    size_t last_bracket_pos = std::string::npos;
    size_t open_paren_pos = std::string::npos;
    if (is_wheel_row) {
        first_bracket_pos = line.find('[');
        last_bracket_pos = line.rfind(']');
    }
    if (is_cab_row) {
        open_paren_pos = line.find('(');
    }

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (is_cab_row && i == open_paren_pos && c == '(') {
            std::cout << "\033[93m(\033[0m"; // bright yellow for cab opening paren
        }
        else if (is_wheel_row && i == first_bracket_pos && c == '[') {
            std::cout << "\033[93m[\033[0m"; // bright yellow
        }
        else if (is_wheel_row && i == last_bracket_pos && c == ']') {
            std::cout << "\033[91m]\033[0m"; // bright red
        }
        else {
            std::cout << c;
        }
    }
    std::cout << '\n';
}

static void update_startup_display(const std::string& telemetry_msg, const std::string& controller_msg, const TruckTemplate& tt) {
    set_cursor_position(0, 0);
    std::vector<std::string> frame = tt.lines;

    int width = (tt.contentEndCol >= tt.contentStartCol) ? (tt.contentEndCol - tt.contentStartCol + 1) : 0;
    int height = (tt.contentEndRow >= tt.contentStartRow) ? (tt.contentEndRow - tt.contentStartRow + 1) : 0;
    if (width <= 0 || height < 6) {
        // fallback to simple message
        std::vector<std::string> single = { telemetry_msg };
        overlay_text_into_truck(frame, tt, single);
        for (const auto& l : frame) print_truck_line(l);
        return;
    }

    auto wrap_text = [&](const std::string& src, int maxLines) {
        std::vector<std::string> out;
        std::istringstream iss(src);
        std::string word;
        std::string line;
        while (iss >> word) {
            if (static_cast<int>(line.size()) + (line.empty() ? 0 : 1) + static_cast<int>(word.size()) <= width) {
                if (!line.empty()) line += ' ';
                line += word;
            }
            else {
                if (!line.empty()) out.push_back(line);
                if (static_cast<int>(word.size()) > width) {
                    int pos = 0;
                    while (pos < static_cast<int>(word.size())) {
                        out.push_back(word.substr(pos, width));
                        pos += width;
                        if (static_cast<int>(out.size()) >= maxLines) break;
                    }
                    line.clear();
                }
                else {
                    line = word;
                }
            }
            if (static_cast<int>(out.size()) >= maxLines) break;
        }
        if (!line.empty() && static_cast<int>(out.size()) < maxLines) out.push_back(line);
        // pad to maxLines
        while (static_cast<int>(out.size()) < maxLines) out.push_back(std::string());
        if (static_cast<int>(out.size()) > maxLines) out.resize(maxLines);
        return out;
        };

    // Top 3 lines for telemetry
    std::vector<std::string> top = wrap_text(telemetry_msg, 3);
    // Separator on 4th line (exact string as requested)
    std::string sep = "--------------------";
    if (static_cast<int>(sep.size()) < width) sep.append(width - static_cast<int>(sep.size()), ' ');
    else if (static_cast<int>(sep.size()) > width) sep.resize(width);
    // Bottom 2 lines for controller
    std::vector<std::string> bottom = wrap_text(controller_msg, 2);

    // Assemble final 6 lines
    std::vector<std::string> finalLines;
    for (int i = 0; i < 3; ++i) finalLines.push_back(top[i]);
    finalLines.push_back(sep);
    for (int i = 0; i < 2; ++i) finalLines.push_back(bottom[i]);

    // Pad/truncate each to width
    for (auto& s : finalLines) {
        if (static_cast<int>(s.size()) > width) s.resize(width);
        else if (static_cast<int>(s.size()) < width) s.append(width - static_cast<int>(s.size()), ' ');
    }

    overlay_text_into_truck(frame, tt, finalLines);
    // Animate the road before printing (move twice as fast)
    animate_road_in_frame(frame, tt, road_anim_phase);
    road_anim_phase += 2;
    for (const auto& l : frame) print_truck_line(l);
}

static void update_console_display_message(const std::string& message, const TruckTemplate& tt) {
    set_cursor_position(0, 0);
    std::vector<std::string> frame = tt.lines;
    // Determine interior size
    int width = (tt.contentEndCol >= tt.contentStartCol) ? (tt.contentEndCol - tt.contentStartCol + 1) : 0;
    int height = (tt.contentEndRow >= tt.contentStartRow) ? (tt.contentEndRow - tt.contentStartRow + 1) : 0;

    // Helper: wrap text into lines not exceeding 'width'
    auto wrap_text = [&](const std::string& src) {
        std::vector<std::string> out;
        if (width <= 0) return out;

        std::istringstream iss(src);
        std::string word;
        std::string line;
        while (iss >> word) {
            if (static_cast<int>(line.size()) + (line.empty() ? 0 : 1) + static_cast<int>(word.size()) <= width) {
                if (!line.empty()) line += ' ';
                line += word;
            }
            else {
                if (!line.empty()) out.push_back(line);
                // if single word longer than width, break it
                if (static_cast<int>(word.size()) > width) {
                    int pos = 0;
                    while (pos < static_cast<int>(word.size())) {
                        out.push_back(word.substr(pos, width));
                        pos += width;
                    }
                    line.clear();
                }
                else {
                    line = word;
                }
            }
        }
        if (!line.empty()) out.push_back(line);
        return out;
        };

    std::vector<std::string> wrapped = wrap_text(message);
    // ensure at least height lines (pad with empty strings)
    while (static_cast<int>(wrapped.size()) < height) wrapped.push_back(std::string());

    std::vector<std::string> clipped;
    for (int r = 0; r < height; ++r) {
        std::string s = (r < static_cast<int>(wrapped.size())) ? wrapped[r] : std::string();
        if (static_cast<int>(s.size()) > width) s.resize(width);
        else if (static_cast<int>(s.size()) < width) s.append(width - static_cast<int>(s.size()), ' ');
        clipped.push_back(s);
    }
    overlay_text_into_truck(frame, tt, clipped);
    // Animate the road before printing (move twice as fast)
    animate_road_in_frame(frame, tt, road_anim_phase);
    road_anim_phase += 2;
    for (const auto& l : frame) print_truck_line(l);
}

static void show_fatal_error_and_exit(const std::string& message, const TruckTemplate& tt) {
    if (!tt.lines.empty()) {
        update_startup_display(message, "Please connect your PS5 DualSense Controller", tt);
    }
    else std::cerr << message << std::endl;
    system("pause");
}

static TruckTemplate load_truck_template(const std::wstring& exeDir) {
    (void)exeDir;
    TruckTemplate tt;
    static const char* kTruck =
        "                 ____________________________________________________\n"
        "  (>_____.----'||                                                    |\n"
        "   /           || ************************************************** |\n"
        "  |---.   = /  || ************************************************** |\n"
        "  |    |  ( '  || ************************************************** |\n"
        "  |    |   `   || ************************************************** |\n"
        "  |---'  ETS2  || ************************************************** |\n"
        "  | Dual Sense || ************************************************** |\n"
        "  [    ________||____________________________________________________|\n"
        "  [__.'.---.   |[Y__Y__Y__Y__Y__Y__Y__Y__Y__Y__Y__Y__Y__Y__Y__Y__Y__Y]\n"
        "  [   //.-.\\\\__| `.__//.-.\\\\//.-.\\\\_________________//.-.\\\\//.-.\\\\_.'\\\n"
        "  [__/( ( ) )`      '( ( ) )( ( ) )`               '( ( ) )( ( ) )`\n"
        "-------`---'----------`---'--`---'-------------------`---'--`---'------\n";

    std::istringstream in(kTruck);
    std::string line;
    int row = 0;
    while (std::getline(in, line)) {
        tt.lines.push_back(line);

        const auto first = line.find('*');
        const auto last = line.rfind('*');
        if (first != std::string::npos && last != std::string::npos && last >= first) {
            if (tt.contentStartRow < 0) {
                tt.contentStartRow = row;
                tt.contentStartCol = static_cast<int>(first);
                tt.contentEndCol = static_cast<int>(last);
            }
            tt.contentEndRow = row;
            tt.contentStartCol = std::min(tt.contentStartCol, static_cast<int>(first));
            tt.contentEndCol = std::max(tt.contentEndCol, static_cast<int>(last));
        }

        row++;
    }

    // Enforce exact interior dimensions: 6 lines x 50 chars
    const int desired_height = 6;
    const int desired_width = 50;

    // Find first row/col where '*' appears if detection above failed
    int firstRow = -1, firstCol = -1;
    for (size_t r = 0; r < tt.lines.size(); ++r) {
        size_t p = tt.lines[r].find('*');
        if (p != std::string::npos) {
            firstRow = static_cast<int>(r);
            firstCol = static_cast<int>(p);
            break;
        }
    }

    if (firstRow >= 0 && firstCol >= 0) {
        tt.contentStartRow = firstRow;
        tt.contentEndRow = std::min(static_cast<int>(tt.lines.size()) - 1, firstRow + desired_height - 1);
        tt.contentStartCol = firstCol;
        // Ensure we have at least desired_width; clamp to available line length
        int maxLineLen = 0;
        for (int r = tt.contentStartRow; r <= tt.contentEndRow; ++r) maxLineLen = std::max<int>(maxLineLen, static_cast<int>(tt.lines[r].size()));
        if (firstCol + desired_width <= maxLineLen) {
            tt.contentEndCol = firstCol + desired_width - 1;
        }
        else {
            // shift left if necessary
            tt.contentEndCol = maxLineLen - 1;
            tt.contentStartCol = std::max(0, tt.contentEndCol - desired_width + 1);
        }
    }

    return tt;
}

static void overlay_text_into_truck(std::vector<std::string>& frame, const TruckTemplate& tt, const std::vector<std::string>& textLines) {
    if (tt.lines.empty() || tt.contentStartRow < 0 || tt.contentStartCol < 0 || tt.contentEndCol < tt.contentStartCol) return;

    const int width = tt.contentEndCol - tt.contentStartCol + 1;
    const int height = tt.contentEndRow - tt.contentStartRow + 1;

    for (int r = 0; r < height; r++) {
        const int targetRow = tt.contentStartRow + r;
        if (targetRow < 0 || targetRow >= static_cast<int>(frame.size())) continue;

        std::string src = (r < static_cast<int>(textLines.size())) ? textLines[r] : "";
        if (static_cast<int>(src.size()) > width) src.resize(width);
        else if (static_cast<int>(src.size()) < width) src.append(width - src.size(), ' ');

        for (int c = 0; c < width; c++) {
            const int col = tt.contentStartCol + c;
            if (col < 0 || col >= static_cast<int>(frame[targetRow].size())) continue;
            if (frame[targetRow][col] == '*') frame[targetRow][col] = src[c];
        }
    }
}

static std::string to_lower_copy(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static bool parse_bool_value(const std::string& raw, bool defaultValue) {
    std::string v = to_lower_copy(trim_copy(raw));
    if (v == "1" || v == "true" || v == "on" || v == "yes") return true;
    if (v == "0" || v == "false" || v == "off" || v == "no") return false;
    return defaultValue;
}

static float clamp01(float v) {
    return (v < 0.0f) ? 0.0f : (v > 1.0f) ? 1.0f : v;
}

static bool try_parse_float(const std::string& raw, float& out) {
    std::string t = trim_copy(raw);
    if (t.empty()) return false;
    try {
        size_t idx = 0;
        float v = std::stof(t, &idx);
        while (idx < t.size() && std::isspace(static_cast<unsigned char>(t[idx]))) idx++;
        if (idx != t.size()) return false;
        out = v;
        return true;
    }
    catch (...) {
        return false;
    }
}

static void parse_bool_or_multiplier(const std::string& raw, bool& enabled, float& multiplier) {
    // Only accept numeric values for multiplier (range 0.0 - 1.0).
    // Do not accept textual booleans here; leave fields unchanged if parsing fails.
    float f = 0.0f;
    if (try_parse_float(raw, f)) {
        multiplier = clamp01(f);
        enabled = (multiplier > 0.0f);
    }
}

static std::wstring get_exe_directory_w() {
    wchar_t path[MAX_PATH] = {};
    DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return L".";
    std::wstring full(path);
    size_t pos = full.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return L".";
    return full.substr(0, pos);
}

static std::wstring get_config_path_w() {
    return get_exe_directory_w() + L"\\ETS2_PS5_Adaptive_Triggers.cfg";
}

static void write_default_config_if_missing(const std::wstring& configPath) {
    DWORD attrs = GetFileAttributesW(configPath.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES) return;

    std::ofstream out(configPath);
    out << "# ETS2_PS5_Adaptive_Triggers config\n";
    out << "Fine Alert (flash + rumble): 1              # 0=OFF, 1=ON\n";
    out << "Park Brake Lightbar: 1                      # 0=OFF, 1=ON\n";
    out << "Retarder Lightbar Pulse: 1                  # 0=OFF, 1=ON\n";
    out << "Blinkers Lightbar: 1                        # 0=OFF, 1=ON\n";
    out << "Warnings Mic LED: 1                         # 0=OFF, 1=ON\n";
    out << "Fuel Player LEDs: 1                         # 0=OFF, 1=ON\n";
    out << "Refuel Rumble: 1                            # multiplier 0.0-1.0 (0 disables)\n";
    out << "Gear Jolt Rumble: 1                         # multiplier 0.0-1.0 (0 disables)\n";
    out << "Engine Start Effects (cranking + lurch): 1  # multiplier 0.0-1.0 (0 disables)\n";
    out << "Hard Braking Rumble: 1                      # multiplier 0.0-1.0 (0 disables)\n";
    out << "Body Roll Rumble: 1                         # multiplier 0.0-1.0 (0 disables)\n";
    out << "Brake Trigger Resistance: 1                 # multiplier 0.0-1.0 (0 disables)\n";
    out << "Throttle Trigger Vibration: 1               # multiplier 0.0-1.0 (0 disables)\n";
}

static AppConfig load_config(const std::wstring& configPath) {
    AppConfig cfg;
    std::ifstream in(configPath);
    if (!in.is_open()) return cfg;

    auto strip_inline_comment = [](std::string s) {
        // Remove trailing comments like: "key: value # comment" or "key: value // comment"
        size_t hash = s.find('#');
        size_t slashes = s.find("//");
        size_t cut = std::string::npos;
        if (hash != std::string::npos) cut = hash;
        if (slashes != std::string::npos) cut = (cut == std::string::npos) ? slashes : std::min(cut, slashes);
        if (cut != std::string::npos) s.resize(cut);
        return trim_copy(s);
        };

    std::string line;
    while (std::getline(in, line)) {
        std::string t = trim_copy(line);
        if (t.empty() || t.rfind("#", 0) == 0 || t.rfind("//", 0) == 0) continue;

        size_t sep = t.find(':');
        if (sep == std::string::npos) sep = t.find('=');
        if (sep == std::string::npos) continue;

        std::string key = to_lower_copy(trim_copy(t.substr(0, sep)));
        std::string val = strip_inline_comment(t.substr(sep + 1));

        auto setb = [&](bool& field) { field = parse_bool_value(val, field); };
        auto setbm = [&](bool& enabled, float& mult) { parse_bool_or_multiplier(val, enabled, mult); };

        if (key == "fine alert (flash + rumble)" || key == "fine alert") setb(cfg.fine_alert);
        else if (key == "park brake lightbar") setb(cfg.park_brake_lightbar);
        else if (key == "retarder lightbar pulse" || key == "retarder lightbar") setb(cfg.retarder_lightbar);
        else if (key == "blinkers lightbar" || key == "blinker lightbar") setb(cfg.blinkers_lightbar);
        else if (key == "warnings mic led" || key == "warning mic led") setb(cfg.warnings_mic_led);
        else if (key == "fuel player leds" || key == "fuel leds") setb(cfg.fuel_player_leds);
        else if (key == "refuel rumble") setbm(cfg.refuel_rumble, cfg.refuel_rumble_multiplier);
        else if (key == "gear jolt rumble" || key == "gear jolt") setbm(cfg.gear_jolt, cfg.gear_jolt_multiplier);
        else if (key == "engine start effects (cranking + lurch)" || key == "engine start effects") setbm(cfg.engine_start_effects, cfg.engine_start_multiplier);
        else if (key == "hard braking rumble" || key == "hard breaking rumble") setbm(cfg.hard_braking_rumble, cfg.hard_braking_multiplier);
        else if (key == "body roll rumble" || key == "body roll") setbm(cfg.body_roll_rumble, cfg.body_roll_multiplier);
        else if (key == "brake trigger resistance" || key == "left trigger resistance") setbm(cfg.brake_trigger_resistance, cfg.brake_trigger_resistance_multiplier);
        else if (key == "throttle trigger vibration" || key == "right trigger vibration") setbm(cfg.throttle_trigger_vibration, cfg.throttle_trigger_vibration_multiplier);
    }

    return cfg;
}

void set_cursor_position(int x, int y) {
    static const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    std::cout.flush();
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(hOut, coord);
}

void set_console_cursor_visibility(bool visible) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci;
    if (GetConsoleCursorInfo(hOut, &ci)) {
        ci.bVisible = visible ? TRUE : FALSE;
        SetConsoleCursorInfo(hOut, &ci);
    }
}

float map_value(float value, float in_min, float in_max, float out_min, float out_max) {
    if (value < in_min) value = in_min;
    if (value > in_max) value = in_max;
    if (in_max == in_min) return out_min;
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static std::vector<std::string> build_display_lines(const DisplayData& data) {
    std::vector<std::string> lines;
    std::ostringstream oss;

    oss << std::fixed;

    oss << std::setprecision(1);
    oss << "Speed: " << std::setw(5) << data.speed << " km/h";
    lines.push_back(oss.str());
    oss.str("");
    oss.clear();

    oss << "Accel: " << std::setw(5) << data.accel_x << " m/s^2";
    lines.push_back(oss.str());
    oss.str("");
    oss.clear();

    float fuel_pct = (data.fuel_capacity > 0) ? (data.fuel / data.fuel_capacity * 100.0f) : 0.0f;
    oss << "Fuel : " << std::setw(5) << data.fuel << "/" << std::setw(5) << data.fuel_capacity << "L (" << std::setw(4) << fuel_pct << "%)";
    lines.push_back(oss.str());
    oss.str("");
    oss.clear();

    oss << "RPM  : " << std::setw(5) << data.rpm;
    lines.push_back(oss.str());
    oss.str("");
    oss.clear();

    oss << "Gear : " << std::setw(5) << data.gear << "   Ret: " << data.retarder;
    lines.push_back(oss.str());
    oss.str("");
    oss.clear();

    oss << std::setprecision(2);
    oss << "Roll : " << std::setw(6) << data.body_roll_angle << " deg";
    lines.push_back(oss.str());
    oss.str("");
    oss.clear();

    std::string blinker_state;
    if (data.left_blinker && data.right_blinker) blinker_state = "HAZARDS";
    else if (data.left_blinker) blinker_state = "LEFT";
    else if (data.right_blinker) blinker_state = "RIGHT";
    else blinker_state = "OFF";
    oss << "Blinkers: " << blinker_state;
    lines.push_back(oss.str());
    oss.str("");
    oss.clear();

    // separator removed to save space
    oss << "Fine: " << (data.fined ? "ON" : "OFF")
        << "  Brake: " << (data.hard_brake ? "ON" : "OFF")
        << "  Roll: " << (data.body_roll ? "ON" : "OFF");
    lines.push_back(oss.str());
    oss.str("");
    oss.clear();

    oss << "Refuel: " << (data.low_fuel ? "LOW" : "OK")
        << "  Eng: " << (data.engine_rumble ? "ON" : "OFF")
        << "  Jolt: " << (data.gear_jolt ? "ON" : "OFF");
    lines.push_back(oss.str());
    oss.str("");
    oss.clear();

    oss << "Rumble L/R: " << (int)data.leftRumble << "/" << (int)data.rightRumble;
    lines.push_back(oss.str());
    oss.str("");
    oss.clear();

    oss << "LT force: " << (int)data.lt_force << "  RT freq: " << (int)data.rt_freq;
    lines.push_back(oss.str());
    oss.str("");
    oss.clear();

    // removed exit hint to save space in the truck UI

    return lines;
}

static void update_console_display(const DisplayData& data, const TruckTemplate& tt) {
    set_cursor_position(0, 0);

    std::vector<std::string> frame = tt.lines;
    std::vector<std::string> text = build_display_lines(data);

    int width = (tt.contentEndCol >= tt.contentStartCol) ? (tt.contentEndCol - tt.contentStartCol + 1) : 0;
    int height = (tt.contentEndRow >= tt.contentStartRow) ? (tt.contentEndRow - tt.contentStartRow + 1) : 0;
    std::vector<std::string> clipped;
    for (int r = 0; r < height; ++r) {
        std::string s = (r < static_cast<int>(text.size())) ? text[r] : std::string();
        if (static_cast<int>(s.size()) > width) s.resize(width);
        else if (static_cast<int>(s.size()) < width) s.append(width - static_cast<int>(s.size()), ' ');
        clipped.push_back(s);
    }

    overlay_text_into_truck(frame, tt, clipped);

    // Animate the road before printing (move twice as fast)
    animate_road_in_frame(frame, tt, road_anim_phase);
    road_anim_phase += 2;
    for (const auto& l : frame) print_truck_line(l);
}

int main() {
    enable_ansi_colors();
    const std::wstring exeDir = get_exe_directory_w();
    const std::wstring configPath = exeDir + L"\\ETS2_PS5_Adaptive_Triggers.cfg";
    write_default_config_if_missing(configPath);
    AppConfig config = load_config(configPath);

    TruckTemplate truckTemplate = load_truck_template(exeDir);
    if (truckTemplate.lines.empty()) std::cerr << "Warning: Could not initialize truck UI. Falling back to plain text UI." << std::endl;

    HANDLE hMapFile = NULL;
    scsTelemetryMap_t* telemetry = NULL;
    DS5W::DeviceEnumInfo infos[16];
    unsigned int controllersCount = 0;
    DS5W::DeviceContext con;
    bool controller_ready = false;

    std::string telemetryStatus = "Waiting for ETS2 telemetry (start the game)...";
    std::string controllerStatus = "Please connect your PS5 DualSense Controller";

    // Hide cursor during updates to avoid flicker
    set_console_cursor_visibility(false);

    // Startup loop: keep updating UI until both telemetry and controller are ready
    while (true) {
        // Allow exit via PS button if controller is connected
        if (controller_ready) {
            DS5W::DS5InputState inState;
            DS5W::getDeviceInputState(&con, &inState);
            if (inState.buttonsB & DS5W_ISTATE_BTN_B_PLAYSTATION_LOGO) {
                DS5W::freeDeviceContext(&con);
                if (telemetry) UnmapViewOfFile(telemetry);
                if (hMapFile) CloseHandle(hMapFile);
                return 0;
            }
        }

        // Telemetry detection
        if (telemetry == NULL) {
            if (hMapFile == NULL) {
                hMapFile = OpenFileMapping(FILE_MAP_READ, FALSE, ETS2_SHARED_MEMORY_NAME);
                if (hMapFile == NULL) {
                    telemetryStatus = "ETS2 telemetry not found. Make sure you copied .dll to 'plugins' folder, then start the game.";
                }
            }

            if (hMapFile != NULL && telemetry == NULL) {
                telemetry = (scsTelemetryMap_t*)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, sizeof(scsTelemetryMap_t));
                if (telemetry == NULL) {
                    telemetryStatus = "ETS2 telemetry found but cannot map. Ensure game is running.";
                }
                else {
                    telemetryStatus = "ETS2 telemetry connected!";
                }
            }
        }

        // Controller detection (dynamic: updates on connect/disconnect)
        controllersCount = 0;
        DS5W::enumDevices(infos, 16, &controllersCount);
        const bool controller_now_present = (controllersCount > 0);

        if (!controller_now_present) {
            controllerStatus = "Please connect your PS5 DualSense Controller!";
            if (controller_ready) {
                DS5W::freeDeviceContext(&con);
                controller_ready = false;
            }
        }
        else {
            controllerStatus = "PS5 DualSense Controller found!";
            if (!controller_ready) {
                DS5W::initDeviceContext(&infos[0], &con);
                controller_ready = true;
            }
        }

        if (!truckTemplate.lines.empty()) {
            update_startup_display(telemetryStatus, controllerStatus, truckTemplate);
        }
        else {
            set_cursor_position(0, 0);
            std::cout << telemetryStatus << "\n" << controllerStatus << std::endl;
        }

        if (telemetry != NULL && controller_ready) break;
        Sleep(250);
    }

    if (!truckTemplate.lines.empty()) update_startup_display("Telemetry + controller ready.", "Press PS button to exit", truckTemplate);
    else std::cout << "Telemetry + controller ready." << std::endl;
    Sleep(500);
    system("cls");
    // Hide cursor during game-data rendering to avoid cursor flicker
    set_console_cursor_visibility(false);

    const unsigned char led_bitmasks[6] = {
        0, DS5W_OSTATE_PLAYER_LED_MIDDLE, DS5W_OSTATE_PLAYER_LED_MIDDLE_LEFT | DS5W_OSTATE_PLAYER_LED_MIDDLE_RIGHT,
        DS5W_OSTATE_PLAYER_LED_LEFT | DS5W_OSTATE_PLAYER_LED_MIDDLE | DS5W_OSTATE_PLAYER_LED_RIGHT,
        DS5W_OSTATE_PLAYER_LED_LEFT | DS5W_OSTATE_PLAYER_LED_MIDDLE_LEFT | DS5W_OSTATE_PLAYER_LED_MIDDLE_RIGHT | DS5W_OSTATE_PLAYER_LED_RIGHT,
        DS5W_OSTATE_PLAYER_LED_LEFT | DS5W_OSTATE_PLAYER_LED_MIDDLE_LEFT | DS5W_OSTATE_PLAYER_LED_MIDDLE | DS5W_OSTATE_PLAYER_LED_MIDDLE_RIGHT | DS5W_OSTATE_PLAYER_LED_RIGHT
    };

    bool exitApp = false;
    int pulse_timer = 0;
    float previous_speed = 0.0f;
    bool previous_engine_state = false;
    bool is_in_startup_effect = false;
    std::chrono::steady_clock::time_point engine_start_time;
    float previous_rpm = 0.0f;
    int previous_gear = 0;
    int jolt_state = 0;
    const float FINE_EFFECT_DURATION_S = 5.0f;
    bool is_in_fine_effect = false;
    bool previous_fined_state = false;
    std::chrono::steady_clock::time_point fine_effect_start_time;
    const float rpm_range = telemetry->config_f.engineRpmMax - IDLE_RPM;
    const float low_rpm_end = IDLE_RPM + (rpm_range * 0.20f);
    const float high_rpm_start = IDLE_RPM + (rpm_range * 0.70f);

    while (!exitApp) {
        DS5W::DS5InputState inState;
        DS5W::getDeviceInputState(&con, &inState);
        if (inState.buttonsB & DS5W_ISTATE_BTN_B_PLAYSTATION_LOGO) { exitApp = true; }

        DS5W::DS5OutputState outState;
        ZeroMemory(&outState, sizeof(DS5W::DS5OutputState));
        DisplayData display_data;

        float fuel_percentage = (telemetry->config_f.fuelCapacity > 0) ? (telemetry->truck_f.fuel / telemetry->config_f.fuelCapacity) * 100.0f : 0.0f;
        float current_speed_kmph = telemetry->truck_f.speed * KMPH_CONVERSION;
        const float time_delta = 0.0166f;
        float calculated_acceleration = (telemetry->truck_f.speed - previous_speed) / time_delta;

        if (!telemetry->paused) {
            pulse_timer++;

            if (config.fine_alert && telemetry->special_b.fined != previous_fined_state) {
                is_in_fine_effect = true;
                fine_effect_start_time = std::chrono::steady_clock::now();
            }

            if (config.fine_alert && is_in_fine_effect) {
                display_data.fine_alert_active = true;
                float elapsed_s = std::chrono::duration<float>(std::chrono::steady_clock::now() - fine_effect_start_time).count();
                if (elapsed_s > FINE_EFFECT_DURATION_S) {
                    is_in_fine_effect = false;
                }
                else {
                    int flash_phase = static_cast<int>(elapsed_s / 0.25f) % 8;
                    if (flash_phase == 0 || flash_phase == 2) { outState.lightbar.r = 255; }
                    else if (flash_phase == 4 || flash_phase == 6) { outState.lightbar.b = 255; }

                    const int RUMBLE_CYCLE_PERIOD = 20;
                    const uint8_t RUMBLE_STRENGTH = 200;
                    if ((pulse_timer % RUMBLE_CYCLE_PERIOD) < (RUMBLE_CYCLE_PERIOD / 2)) {
                        outState.leftRumble = std::max(outState.leftRumble, RUMBLE_STRENGTH);
                    }
                    else {
                        outState.rightRumble = std::max(outState.rightRumble, RUMBLE_STRENGTH);
                    }
                }
            }
            else if (config.park_brake_lightbar && telemetry->truck_b.parkBrake) {
                outState.lightbar.r = 255;
            }
            else if (config.retarder_lightbar && telemetry->truck_b.engineEnabled && telemetry->truck_ui.retarderBrake > 0) {
                outState.lightbar.b = static_cast<uint8_t>((sin(pulse_timer * 0.1f * telemetry->truck_ui.retarderBrake) + 1.0f) / 2.0f * 255.0f);
            }
            else if (config.blinkers_lightbar && telemetry->truck_b.engineEnabled && telemetry->truck_b.blinkerLeftOn && telemetry->truck_b.blinkerRightOn) {
                outState.lightbar.r = 255; outState.lightbar.g = 255;
            }
            else if (config.blinkers_lightbar && telemetry->truck_b.engineEnabled && (telemetry->truck_b.blinkerLeftOn || telemetry->truck_b.blinkerRightOn)) {
                outState.lightbar.g = 255;
            }

            bool is_critical_warning = telemetry->truck_b.oilPressureWarning || telemetry->truck_b.waterTemperatureWarning || telemetry->truck_b.airPressureWarning || telemetry->truck_f.wearChassis > 0.25f;
            bool is_minor_warning = telemetry->truck_b.adblueWarning || telemetry->truck_b.batteryVoltageWarning;
            if (config.warnings_mic_led) {
                outState.microphoneLed = is_critical_warning ? DS5W::MicLed::PULSE : (is_minor_warning ? DS5W::MicLed::ON : outState.microphoneLed);
            }

            if (config.fuel_player_leds) {
                int num_leds_to_light = (fuel_percentage > 80.0f) ? 5 : (fuel_percentage > 60.0f) ? 4 : (fuel_percentage > 40.0f) ? 3 : (fuel_percentage > 20.0f) ? 2 : (fuel_percentage > 0.1f) ? 1 : 0;
                outState.playerLeds.bitmask = led_bitmasks[num_leds_to_light];
                const float BRACKET_SIZE = 20.0f;
                float bracket_midpoint = num_leds_to_light * BRACKET_SIZE - (BRACKET_SIZE / 2.0f);
                if (num_leds_to_light > 0 && fuel_percentage < bracket_midpoint) {
                    const int BLINK_HALF_PERIOD_FRAMES = 30;
                    outState.playerLeds.brightness = ((pulse_timer % (BLINK_HALF_PERIOD_FRAMES * 2)) < BLINK_HALF_PERIOD_FRAMES) ? DS5W::LedBrightness::HIGH : DS5W::LedBrightness::LOW;
                }
                else {
                    outState.playerLeds.brightness = DS5W::LedBrightness::HIGH;
                }
            }

            if (config.refuel_rumble && telemetry->special_b.refuel) {
                display_data.refueling_active = true;
                float rumble_strength = map_value(fuel_percentage, 0.0f, 100.0f, 255.0f, 0.0f);
                rumble_strength *= config.refuel_rumble_multiplier;
                const int CHUG_PERIOD_FRAMES = 16;
                if ((pulse_timer % CHUG_PERIOD_FRAMES) < (CHUG_PERIOD_FRAMES / 2)) {
                    uint8_t chug_rumble = static_cast<uint8_t>(rumble_strength);
                    outState.leftRumble = std::max(outState.leftRumble, chug_rumble);
                    outState.rightRumble = std::max(outState.rightRumble, chug_rumble);
                }
            }

            if (config.gear_jolt) {
                if (telemetry->truck_i.gear != previous_gear && telemetry->truck_i.gear != 0 && previous_gear != 0) { jolt_state = 1; }
            }
            if (config.gear_jolt && jolt_state > 0) {
                display_data.gear_jolt_active = true;
                if (jolt_state == 1) {
                    outState.leftRumble = std::max(outState.leftRumble, static_cast<uint8_t>(255.0f * config.gear_jolt_multiplier));
                    outState.rightRumble = std::max(outState.rightRumble, static_cast<uint8_t>(100.0f * config.gear_jolt_multiplier));
                    jolt_state = 2;
                }
                else {
                    outState.leftRumble = std::max(outState.leftRumble, static_cast<uint8_t>(100.0f * config.gear_jolt_multiplier));
                    outState.rightRumble = std::max(outState.rightRumble, static_cast<uint8_t>(255.0f * config.gear_jolt_multiplier));
                    jolt_state = 0;
                }
            }

            if (config.engine_start_effects && telemetry->truck_b.engineEnabled && !previous_engine_state) {
                is_in_startup_effect = true;
                engine_start_time = std::chrono::steady_clock::now();
            }
            if (config.engine_start_effects && !telemetry->truck_b.engineEnabled && telemetry->truck_f.engineRpm > 0 && telemetry->truck_f.engineRpm >= previous_rpm) {
                display_data.engine_cranking_active = true;
                const int STARTER_PULSE_PERIOD = 10;
                if ((pulse_timer % STARTER_PULSE_PERIOD) < (STARTER_PULSE_PERIOD / 2)) {
                    outState.rightRumble = std::max(outState.rightRumble, static_cast<uint8_t>(200.0f * config.engine_start_multiplier));
                }
            }
            if (config.engine_start_effects && is_in_startup_effect) {
                display_data.startup_lurch_active = true;
                const float STARTUP_EFFECT_DURATION_S = 1.5f;
                float elapsed_s = std::chrono::duration<float>(std::chrono::steady_clock::now() - engine_start_time).count();
                if (elapsed_s < STARTUP_EFFECT_DURATION_S) {
                    float progress = elapsed_s / STARTUP_EFFECT_DURATION_S;
                    outState.leftRumble = std::max(outState.leftRumble, static_cast<uint8_t>(sin(progress * PI) * 255.0f * config.engine_start_multiplier));
                }
                else {
                    is_in_startup_effect = false;
                }
            }

            if (telemetry->truck_b.engineEnabled) {
                if (config.hard_braking_rumble && telemetry->truck_f.userBrake > 0.8f && current_speed_kmph > 10.0f && calculated_acceleration < -10.0f) {
                    display_data.hard_braking_active = true;
                    const int PULSE_PERIOD_FRAMES = 6;
                    if ((pulse_timer % PULSE_PERIOD_FRAMES) < (PULSE_PERIOD_FRAMES / 2)) {
                        outState.leftRumble = std::max(outState.leftRumble, static_cast<uint8_t>(255.0f * config.hard_braking_multiplier));
                        outState.rightRumble = std::max(outState.rightRumble, static_cast<uint8_t>(255.0f * config.hard_braking_multiplier));
                    }
                }
                else if (config.body_roll_rumble && current_speed_kmph > 10.0f) {
                    float roll_abs = std::abs(telemetry->truck_dp.rotationZ * 100);
                    if (roll_abs > 0.1f) {
                        display_data.body_roll_active = true;
                        float pulse_amplitude = map_value(roll_abs, 0.1f, 2.0f, 30.0f, 255.0f);
                        pulse_amplitude *= config.body_roll_multiplier;
                        uint8_t final_rumble = static_cast<uint8_t>((sin(pulse_timer * 0.2f) + 1.0f) / 2.0f * pulse_amplitude);
                        if ((telemetry->truck_dp.rotationZ * 100) > 0) {
                            outState.leftRumble = std::max(outState.leftRumble, final_rumble);
                        }
                        else {
                            outState.rightRumble = std::max(outState.rightRumble, final_rumble);
                        }
                    }
                }

                if (config.brake_trigger_resistance) {
                    outState.leftTriggerEffect.effectType = DS5W::TriggerEffectType::ContinuousResitance;
                    outState.leftTriggerEffect.Continuous.startPosition = 0;
                    outState.leftTriggerEffect.Continuous.force = static_cast<uint8_t>(map_value(current_speed_kmph, 0.0f, 90.0f, 0.0f, 200.0f) * config.brake_trigger_resistance_multiplier);
                }

                if (config.throttle_trigger_vibration) {
                    float vibration_frequency = (telemetry->truck_f.engineRpm <= low_rpm_end) ? map_value(telemetry->truck_f.engineRpm, IDLE_RPM, low_rpm_end, 200.0f, 0.0f) :
                        (telemetry->truck_f.engineRpm >= high_rpm_start) ? map_value(telemetry->truck_f.engineRpm, high_rpm_start, telemetry->config_f.engineRpmMax, 0.0f, 255.0f) : 0.0f;
                    vibration_frequency *= config.throttle_trigger_vibration_multiplier;
                    outState.rightTriggerEffect.effectType = DS5W::TriggerEffectType::EffectEx;
                    outState.rightTriggerEffect.EffectEx.startPosition = 0;
                    outState.rightTriggerEffect.EffectEx.keepEffect = true;
                    outState.rightTriggerEffect.EffectEx.beginForce = 0;
                    outState.rightTriggerEffect.EffectEx.middleForce = 0;
                    outState.rightTriggerEffect.EffectEx.endForce = 0;
                    outState.rightTriggerEffect.EffectEx.frequency = static_cast<uint8_t>(vibration_frequency);
                }
            }
        }

        DS5W::setDeviceOutputState(&con, &outState);

        display_data.leftRumble = outState.leftRumble;
        display_data.rightRumble = outState.rightRumble;
        display_data.lt_force = outState.leftTriggerEffect.Continuous.force;
        display_data.rt_freq = outState.rightTriggerEffect.EffectEx.frequency;
        display_data.lightbar = outState.lightbar;
        display_data.micLedState = (outState.microphoneLed == DS5W::MicLed::ON) ? "ON" : (outState.microphoneLed == DS5W::MicLed::PULSE) ? "PULSE" : "OFF";
        display_data.speed = current_speed_kmph;
        display_data.accel_x = calculated_acceleration;
        display_data.fuel = telemetry->truck_f.fuel;
        display_data.fuel_capacity = telemetry->config_f.fuelCapacity;
        display_data.rpm = telemetry->truck_f.engineRpm;
        display_data.body_roll_angle = telemetry->truck_dp.rotationZ * 57.2958f;
        display_data.gear = telemetry->truck_i.gear;
        display_data.retarder = telemetry->truck_ui.retarderBrake;
        display_data.left_blinker = telemetry->truck_b.blinkerLeftOn;
        display_data.right_blinker = telemetry->truck_b.blinkerRightOn;
        display_data.fined = display_data.fine_alert_active;
        display_data.hard_brake = display_data.hard_braking_active;
        display_data.low_fuel = fuel_percentage < 20.0f;
        display_data.body_roll = display_data.body_roll_active;
        display_data.engine_rumble = display_data.engine_cranking_active || display_data.startup_lurch_active;
        display_data.gear_jolt = display_data.gear_jolt_active;
        display_data.braking_lightbar = telemetry->truck_ui.retarderBrake > 0;

        if (!truckTemplate.lines.empty()) update_console_display(display_data, truckTemplate);
        else {
            set_cursor_position(0, 0);
            for (const auto& l : build_display_lines(display_data)) std::cout << l << "\n";
        }

        previous_speed = telemetry->truck_f.speed;
        previous_engine_state = telemetry->truck_b.engineEnabled;
        previous_rpm = telemetry->truck_f.engineRpm;
        previous_gear = telemetry->truck_i.gear;
        previous_fined_state = telemetry->special_b.fined;

        Sleep(16);
    }

    // Restore cursor before exiting so console prompt behaves normally
    set_console_cursor_visibility(true);
    std::cout << "\nExiting application. Resetting controller..." << std::endl;
    DS5W::DS5OutputState resetState;
    ZeroMemory(&resetState, sizeof(DS5W::DS5OutputState));
    DS5W::setDeviceOutputState(&con, &resetState);
    DS5W::freeDeviceContext(&con);
    UnmapViewOfFile(telemetry);
    CloseHandle(hMapFile);
    std::cout << "Cleanup complete. Goodbye!" << std::endl;
    Sleep(1000);

    return 0;
}