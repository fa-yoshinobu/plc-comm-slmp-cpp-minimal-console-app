/**
 * @file console_tui.h
 * @brief Demo-first ANSI dashboard for W6300-EVB-Pico2.
 */

constexpr size_t kDemoLogCapacity = 24;
constexpr size_t kDemoWatchCapacity = 6;
constexpr size_t kDemoMainRowCount = 18;
constexpr size_t kDemoLineCapacity = 96;
constexpr size_t kDemoQuickResultLines = 10;
constexpr uint16_t kDemoWatchMaxWordPoints = 4;
constexpr uint16_t kDemoWatchMaxDWordPoints = 2;
constexpr uint16_t kDemoWatchMaxBitPoints = 16;
constexpr uint16_t kDemoBatchMaxWordPoints = 16;
constexpr uint16_t kDemoBatchMaxDWordPoints = 8;
constexpr uint16_t kDemoBatchMaxBitPoints = 16;
constexpr uint32_t kDemoRenderIntervalMs = 750;
constexpr uint32_t kDemoWatchPollMs = 900;
constexpr uint32_t kDemoBatchPollMs = 1300;
constexpr uint32_t kDemoMetricsWindowMs = 1000;
constexpr uint32_t kDemoDefaultStressIntervalMs = 50;

enum class DemoView : uint8_t {
    Dashboard = 0,
    Connect,
    QuickRead,
    QuickWrite,
    Watch,
    Batch,
    Stress,
    System,
    Log,
};

struct DemoLogEntry {
    uint32_t stamp_ms = 0;
    char text[kDemoLineCapacity] = {};
};

struct DemoReadState {
    WatchType type = WatchType::Word;
    slmp::DeviceAddress device = {slmp::DeviceCode::D, 100};
    uint16_t points = 4;
    bool has_result = false;
    uint8_t line_count = 0;
    char lines[kDemoQuickResultLines][kDemoLineCapacity] = {};
};

struct DemoWriteState {
    WatchType type = WatchType::Word;
    slmp::DeviceAddress device = {slmp::DeviceCode::D, 100};
    bool has_result = false;
    char last_status[kDemoLineCapacity] = {};
};

struct DemoWatchItem {
    bool active = false;
    bool healthy = false;
    WatchType type = WatchType::Word;
    slmp::DeviceAddress device = {slmp::DeviceCode::D, 0};
    uint16_t points = 1;
    uint16_t words[kDemoWatchMaxWordPoints] = {};
    uint32_t dwords[kDemoWatchMaxDWordPoints] = {};
    bool bits[kDemoWatchMaxBitPoints] = {};
    bool changed = false;
    uint32_t last_poll_ms = 0;
    slmp::Error last_error = slmp::Error::Ok;
    uint16_t last_end_code = 0;
};

struct DemoBatchState {
    WatchType type = WatchType::Word;
    slmp::DeviceAddress device = {slmp::DeviceCode::D, 100};
    uint16_t points = 8;
    bool healthy = false;
    bool has_result = false;
    uint32_t last_poll_ms = 0;
    uint16_t words[kDemoBatchMaxWordPoints] = {};
    uint32_t dwords[kDemoBatchMaxDWordPoints] = {};
    bool bits[kDemoBatchMaxBitPoints] = {};
    slmp::Error last_error = slmp::Error::Ok;
    uint16_t last_end_code = 0;
};

struct DemoStressState {
    bool active = false;
    WatchType type = WatchType::Word;
    slmp::DeviceAddress device = {slmp::DeviceCode::D, 100};
    uint16_t points = 4;
    uint32_t interval_ms = kDemoDefaultStressIntervalMs;
    uint32_t next_due_ms = 0;
    uint32_t cycles = 0;
    uint32_t ok = 0;
    uint32_t fail = 0;
    uint32_t last_latency_ms = 0;
    uint32_t min_latency_ms = 0;
    uint32_t max_latency_ms = 0;
    uint64_t total_latency_ms = 0;
    slmp::Error last_error = slmp::Error::Ok;
    uint16_t last_end_code = 0;
};

struct DemoSystemMetrics {
    bool memory_supported = false;
    uint32_t uptime_ms = 0;
    uint32_t free_heap_bytes = 0;
    uint32_t min_free_heap_bytes = 0;
    uint32_t loop_avg_us = 0;
    uint32_t loop_max_us = 0;
    uint8_t cpu_load_percent = 0;
    bool link_up = false;
    bool socket_open = false;
    uint32_t request_count = 0;
    uint32_t error_count = 0;
    uint32_t last_latency_ms = 0;
    uint32_t peak_latency_ms = 0;
};

struct DemoUiState {
    DemoView active_view = DemoView::Dashboard;
    bool ansi_enabled = true;
    bool force_render = true;
    uint32_t next_render_ms = 0;
};

DemoLogEntry demo_logs[kDemoLogCapacity] = {};
size_t demo_log_count = 0;
size_t demo_log_next = 0;
DemoReadState demo_read_state = {};
DemoWriteState demo_write_state = {};
DemoWatchItem demo_watch_items[kDemoWatchCapacity] = {};
DemoBatchState demo_batch_state = {};
DemoStressState demo_stress_state = {};
DemoSystemMetrics demo_system_metrics = {};
DemoUiState demo_ui = {};
char demo_last_write[kDemoLineCapacity] = "none";
char demo_last_error[kDemoLineCapacity] = "none";
uint64_t demo_busy_us_accum = 0;
uint64_t demo_loop_us_accum = 0;
uint32_t demo_loop_count = 0;
uint32_t demo_loop_max_us = 0;
uint32_t demo_metrics_window_started_ms = 0;

const char* demoViewText(DemoView view) {
    switch (view) {
        case DemoView::Dashboard: return "Dashboard";
        case DemoView::Connect: return "Connect";
        case DemoView::QuickRead: return "Quick Read";
        case DemoView::QuickWrite: return "Quick Write";
        case DemoView::Watch: return "Watch";
        case DemoView::Batch: return "Batch";
        case DemoView::Stress: return "Stress";
        case DemoView::System: return "System";
        case DemoView::Log: return "Log";
        default: return "Dashboard";
    }
}

const char* demoWatchTypeText(WatchType type) {
    switch (type) {
        case WatchType::Word: return "word";
        case WatchType::Bit: return "bit";
        case WatchType::DWord: return "dword";
        default: return "word";
    }
}

uint16_t demoMaxPointsForType(WatchType type, bool batch_mode) {
    if (batch_mode) {
        if (type == WatchType::Bit) return kDemoBatchMaxBitPoints;
        if (type == WatchType::DWord) return kDemoBatchMaxDWordPoints;
        return kDemoBatchMaxWordPoints;
    }
    if (type == WatchType::Bit) return kDemoWatchMaxBitPoints;
    if (type == WatchType::DWord) return kDemoWatchMaxDWordPoints;
    return kDemoWatchMaxWordPoints;
}

void requestDemoRender() {
    demo_ui.force_render = true;
    demo_ui.next_render_ms = 0;
}

void formatDeviceAddressText(const slmp::DeviceAddress& device, char* out, size_t capacity, uint32_t offset = 0) {
    if (!out || capacity == 0) return;
    const DeviceSpec* spec = findDeviceSpecByCode(device.code);
    if (!spec) {
        snprintf(out, capacity, "?%lu", static_cast<unsigned long>(device.number + offset));
        return;
    }
    if (spec->hex_address) {
        snprintf(out, capacity, "%s%lX", spec->name, static_cast<unsigned long>(device.number + offset));
    } else {
        snprintf(out, capacity, "%s%lu", spec->name, static_cast<unsigned long>(device.number + offset));
    }
}

void appendDemoLog(const char* fmt, ...) {
    if (!fmt) return;
    DemoLogEntry& entry = demo_logs[demo_log_next];
    entry.stamp_ms = millis();
    va_list args;
    va_start(args, fmt);
    vsnprintf(entry.text, sizeof(entry.text), fmt, args);
    va_end(args);
    demo_log_next = (demo_log_next + 1U) % kDemoLogCapacity;
    if (demo_log_count < kDemoLogCapacity) ++demo_log_count;
    requestDemoRender();
}

void setDemoLastErrorText(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(demo_last_error, sizeof(demo_last_error), fmt, args);
    va_end(args);
    appendDemoLog("%s", demo_last_error);
}

void setDemoLastWriteText(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(demo_last_write, sizeof(demo_last_write), fmt, args);
    va_end(args);
    appendDemoLog("%s", demo_last_write);
}

void clearQuickReadLines() {
    demo_read_state.has_result = false;
    demo_read_state.line_count = 0;
    for (size_t i = 0; i < kDemoQuickResultLines; ++i) demo_read_state.lines[i][0] = '\0';
}

void noteDemoRequest(slmp::Error err, uint32_t latency_ms) {
    ++demo_system_metrics.request_count;
    demo_system_metrics.last_latency_ms = latency_ms;
    if (latency_ms > demo_system_metrics.peak_latency_ms) {
        demo_system_metrics.peak_latency_ms = latency_ms;
    }
    if (err != slmp::Error::Ok) {
        ++demo_system_metrics.error_count;
    }
}

void capturePlcModelIfConnected() {
    if (!plc.connected()) {
        copyText(cached_plc_model, sizeof(cached_plc_model), "disconnected");
        return;
    }
    slmp::TypeNameInfo info = {};
    if (plc.readTypeName(info) == slmp::Error::Ok) copyText(cached_plc_model, sizeof(cached_plc_model), info.model);
    else copyText(cached_plc_model, sizeof(cached_plc_model), "unknown");
}

bool demoReadFreeHeap(uint32_t& free_heap_bytes) {
    free_heap_bytes = 0;
    const struct mallinfo info = mallinfo();
    if (info.fordblks < 0) return false;
    free_heap_bytes = static_cast<uint32_t>(info.fordblks);
    return true;
}

void noteDemoLoopSample(uint32_t elapsed_us) {
    demo_busy_us_accum += elapsed_us;
    demo_loop_us_accum += elapsed_us;
    ++demo_loop_count;
    if (elapsed_us > demo_loop_max_us) demo_loop_max_us = elapsed_us;
}

void pollDemoSystemMetrics() {
    demo_system_metrics.uptime_ms = millis();
    demo_system_metrics.link_up = Ethernet.connected();
    demo_system_metrics.socket_open = plc.connected();

    const uint32_t now_ms = millis();
    if (demo_metrics_window_started_ms == 0U) demo_metrics_window_started_ms = now_ms;
    if (now_ms - demo_metrics_window_started_ms < kDemoMetricsWindowMs) return;

    const uint32_t elapsed_ms = now_ms - demo_metrics_window_started_ms;
    demo_system_metrics.loop_avg_us = (demo_loop_count == 0U) ? 0U : static_cast<uint32_t>(demo_loop_us_accum / demo_loop_count);
    demo_system_metrics.loop_max_us = demo_loop_max_us;
    const uint64_t window_us = static_cast<uint64_t>(elapsed_ms == 0U ? 1U : elapsed_ms) * 1000ULL;
    const uint64_t ratio = (demo_busy_us_accum * 100ULL) / window_us;
    demo_system_metrics.cpu_load_percent = static_cast<uint8_t>(ratio > 100ULL ? 100ULL : ratio);

    uint32_t free_heap = 0;
    demo_system_metrics.memory_supported = demoReadFreeHeap(free_heap);
    if (demo_system_metrics.memory_supported) {
        demo_system_metrics.free_heap_bytes = free_heap;
        if (demo_system_metrics.min_free_heap_bytes == 0U || free_heap < demo_system_metrics.min_free_heap_bytes) {
            demo_system_metrics.min_free_heap_bytes = free_heap;
        }
    }

    demo_busy_us_accum = 0;
    demo_loop_us_accum = 0;
    demo_loop_count = 0;
    demo_loop_max_us = 0;
    demo_metrics_window_started_ms = now_ms;
    requestDemoRender();
}

void changeDemoView(DemoView view) {
    demo_ui.active_view = view;
    requestDemoRender();
}

void populateDefaultDemoWatchItems() {
    demo_watch_items[0].active = true;
    demo_watch_items[0].type = WatchType::Word;
    demo_watch_items[0].device = {slmp::DeviceCode::D, 100};
    demo_watch_items[0].points = 2;

    demo_watch_items[1].active = true;
    demo_watch_items[1].type = WatchType::Bit;
    demo_watch_items[1].device = {slmp::DeviceCode::M, 100};
    demo_watch_items[1].points = 8;

    demo_watch_items[2].active = true;
    demo_watch_items[2].type = WatchType::Bit;
    demo_watch_items[2].device = {slmp::DeviceCode::SM, 400};
    demo_watch_items[2].points = 1;

    demo_watch_items[3].active = true;
    demo_watch_items[3].type = WatchType::DWord;
    demo_watch_items[3].device = {slmp::DeviceCode::D, 200};
    demo_watch_items[3].points = 1;
}

void initializeDemoTui() {
    demo_ui = DemoUiState();
    demo_read_state = DemoReadState();
    demo_write_state = DemoWriteState();
    demo_batch_state = DemoBatchState();
    demo_stress_state = DemoStressState();
    demo_system_metrics = DemoSystemMetrics();
    for (size_t i = 0; i < kDemoWatchCapacity; ++i) demo_watch_items[i] = DemoWatchItem();
    copyText(demo_last_write, sizeof(demo_last_write), "none");
    copyText(demo_last_error, sizeof(demo_last_error), "none");
    copyText(cached_plc_model, sizeof(cached_plc_model), "disconnected");
    demo_log_count = 0;
    demo_log_next = 0;
    demo_busy_us_accum = 0;
    demo_loop_us_accum = 0;
    demo_loop_count = 0;
    demo_loop_max_us = 0;
    demo_metrics_window_started_ms = millis();
    populateDefaultDemoWatchItems();
    appendDemoLog("Demo dashboard initialized");
}

bool runDemoConnect(bool verbose) {
    if (connectPlc(verbose)) {
        capturePlcModelIfConnected();
        appendDemoLog("Connected to %s:%u (%s)", plc_host, console_link.plc_port, cached_plc_model);
        requestDemoRender();
        return true;
    }
    setDemoLastErrorText("Connect failed: %s end=0x%X", slmp::errorString(plc.lastError()), plc.lastEndCode());
    requestDemoRender();
    return false;
}

void runDemoDisconnect() {
    closePlc();
    copyText(cached_plc_model, sizeof(cached_plc_model), "disconnected");
    appendDemoLog("Connection closed");
    requestDemoRender();
}

bool parseDemoPointsToken(char* token, uint16_t max_points, uint16_t& points) {
    if (!token) return false;
    return parsePositiveCount(token, max_points, points);
}

void setDemoHost(char* host_token) {
    if (!host_token || *host_token == '\0') {
        Serial.print(F("host="));
        Serial.println(plc_host);
        return;
    }
    copyText(plc_host, sizeof(plc_host), host_token);
    appendDemoLog("Host set to %s", plc_host);
    requestDemoRender();
}

bool runDemoRead(WatchType type, const slmp::DeviceAddress& device, uint16_t points) {
    clearQuickReadLines();
    demo_read_state.type = type;
    demo_read_state.device = device;
    demo_read_state.points = points;

    if (!connectPlc(false)) {
        setDemoLastErrorText("Quick read failed: plc not connected");
        return false;
    }

    const uint32_t started_ms = millis();
    char addr[24] = {};
    formatDeviceAddressText(device, addr, sizeof(addr));

    if (type == WatchType::Word) {
        uint16_t values[kDemoBatchMaxWordPoints] = {};
        if (plc.readWords(device, points, values, kDemoBatchMaxWordPoints) != slmp::Error::Ok) {
            noteDemoRequest(plc.lastError(), millis() - started_ms);
            setDemoLastErrorText("Quick read failed: %s end=0x%X", slmp::errorString(plc.lastError()), plc.lastEndCode());
            return false;
        }
        for (uint16_t i = 0; i < points && i < kDemoQuickResultLines; ++i) {
            char dev_text[24] = {};
            formatDeviceAddressText(device, dev_text, sizeof(dev_text), i);
            snprintf(demo_read_state.lines[demo_read_state.line_count++], kDemoLineCapacity, "%s = %u (0x%04X)", dev_text, values[i], values[i]);
        }
    } else if (type == WatchType::Bit) {
        bool values[kDemoBatchMaxBitPoints] = {};
        if (plc.readBits(device, points, values, kDemoBatchMaxBitPoints) != slmp::Error::Ok) {
            noteDemoRequest(plc.lastError(), millis() - started_ms);
            setDemoLastErrorText("Quick read failed: %s end=0x%X", slmp::errorString(plc.lastError()), plc.lastEndCode());
            return false;
        }
        for (uint16_t i = 0; i < points && i < kDemoQuickResultLines; ++i) {
            char dev_text[24] = {};
            formatDeviceAddressText(device, dev_text, sizeof(dev_text), i);
            snprintf(demo_read_state.lines[demo_read_state.line_count++], kDemoLineCapacity, "%s = %s", dev_text, values[i] ? "ON" : "OFF");
        }
    } else {
        uint32_t values[kDemoBatchMaxDWordPoints] = {};
        if (plc.readDWords(device, points, values, kDemoBatchMaxDWordPoints) != slmp::Error::Ok) {
            noteDemoRequest(plc.lastError(), millis() - started_ms);
            setDemoLastErrorText("Quick read failed: %s end=0x%X", slmp::errorString(plc.lastError()), plc.lastEndCode());
            return false;
        }
        for (uint16_t i = 0; i < points && i < kDemoQuickResultLines; ++i) {
            char dev_text[24] = {};
            formatDeviceAddressText(device, dev_text, sizeof(dev_text), i * 2U);
            snprintf(
                demo_read_state.lines[demo_read_state.line_count++],
                kDemoLineCapacity,
                "%s = %lu (0x%08lX)",
                dev_text,
                static_cast<unsigned long>(values[i]),
                static_cast<unsigned long>(values[i]));
        }
    }

    demo_read_state.has_result = true;
    const uint32_t latency_ms = millis() - started_ms;
    noteDemoRequest(slmp::Error::Ok, latency_ms);
    appendDemoLog("Quick read %s %s points=%u latency=%lums", demoWatchTypeText(type), addr, points, static_cast<unsigned long>(latency_ms));
    changeDemoView(DemoView::QuickRead);
    return true;
}

bool runDemoWriteWords(const slmp::DeviceAddress& device, char* tokens[], int token_count, int start_index) {
    const int value_count = token_count - start_index;
    if (value_count <= 0 || value_count > static_cast<int>(kDemoBatchMaxWordPoints)) {
        Serial.println(F("qw usage: qw <device> <value...>"));
        return false;
    }
    uint16_t values[kDemoBatchMaxWordPoints] = {};
    for (int i = 0; i < value_count; ++i) {
        if (!parseWordValue(tokens[start_index + i], values[i])) {
            Serial.println(F("qw values must fit in 16 bits"));
            return false;
        }
    }
    if (!connectPlc(false)) {
        setDemoLastErrorText("Quick write failed: plc not connected");
        return false;
    }
    const uint32_t started_ms = millis();
    const slmp::Error err = (value_count == 1) ? plc.writeOneWord(device, values[0]) : plc.writeWords(device, values, static_cast<size_t>(value_count));
    const uint32_t latency_ms = millis() - started_ms;
    noteDemoRequest(err, latency_ms);
    demo_write_state.type = WatchType::Word;
    demo_write_state.device = device;
    if (err != slmp::Error::Ok) {
        setDemoLastErrorText("Quick write failed: %s end=0x%X", slmp::errorString(plc.lastError()), plc.lastEndCode());
        return false;
    }
    char addr[24] = {};
    formatDeviceAddressText(device, addr, sizeof(addr));
    snprintf(demo_write_state.last_status, sizeof(demo_write_state.last_status), "word write %s count=%d latency=%lums", addr, value_count, static_cast<unsigned long>(latency_ms));
    demo_write_state.has_result = true;
    setDemoLastWriteText("Quick write %s count=%d", addr, value_count);
    changeDemoView(DemoView::QuickWrite);
    return true;
}

bool runDemoWriteBits(const slmp::DeviceAddress& device, char* tokens[], int token_count, int start_index) {
    const int value_count = token_count - start_index;
    if (value_count <= 0 || value_count > static_cast<int>(kDemoBatchMaxBitPoints)) {
        Serial.println(F("qwb usage: qwb <device> <0|1...>"));
        return false;
    }
    bool values[kDemoBatchMaxBitPoints] = {};
    for (int i = 0; i < value_count; ++i) {
        if (!parseBoolValue(tokens[start_index + i], values[i])) {
            Serial.println(F("qwb values must be 0/1, on/off, true/false"));
            return false;
        }
    }
    if (!connectPlc(false)) {
        setDemoLastErrorText("Quick write failed: plc not connected");
        return false;
    }
    const uint32_t started_ms = millis();
    const slmp::Error err = (value_count == 1) ? plc.writeOneBit(device, values[0]) : plc.writeBits(device, values, static_cast<size_t>(value_count));
    const uint32_t latency_ms = millis() - started_ms;
    noteDemoRequest(err, latency_ms);
    demo_write_state.type = WatchType::Bit;
    demo_write_state.device = device;
    if (err != slmp::Error::Ok) {
        setDemoLastErrorText("Quick write failed: %s end=0x%X", slmp::errorString(plc.lastError()), plc.lastEndCode());
        return false;
    }
    char addr[24] = {};
    formatDeviceAddressText(device, addr, sizeof(addr));
    snprintf(demo_write_state.last_status, sizeof(demo_write_state.last_status), "bit write %s count=%d latency=%lums", addr, value_count, static_cast<unsigned long>(latency_ms));
    demo_write_state.has_result = true;
    setDemoLastWriteText("Quick bit write %s count=%d", addr, value_count);
    changeDemoView(DemoView::QuickWrite);
    return true;
}

bool runDemoWriteDWords(const slmp::DeviceAddress& device, char* tokens[], int token_count, int start_index) {
    const int value_count = token_count - start_index;
    if (value_count <= 0 || value_count > static_cast<int>(kDemoBatchMaxDWordPoints)) {
        Serial.println(F("qwd usage: qwd <device> <value...>"));
        return false;
    }
    uint32_t values[kDemoBatchMaxDWordPoints] = {};
    for (int i = 0; i < value_count; ++i) {
        if (!parseDWordValue(tokens[start_index + i], values[i])) {
            Serial.println(F("qwd values must fit in 32 bits"));
            return false;
        }
    }
    if (!connectPlc(false)) {
        setDemoLastErrorText("Quick write failed: plc not connected");
        return false;
    }
    const uint32_t started_ms = millis();
    const slmp::Error err = (value_count == 1) ? plc.writeOneDWord(device, values[0]) : plc.writeDWords(device, values, static_cast<size_t>(value_count));
    const uint32_t latency_ms = millis() - started_ms;
    noteDemoRequest(err, latency_ms);
    demo_write_state.type = WatchType::DWord;
    demo_write_state.device = device;
    if (err != slmp::Error::Ok) {
        setDemoLastErrorText("Quick write failed: %s end=0x%X", slmp::errorString(plc.lastError()), plc.lastEndCode());
        return false;
    }
    char addr[24] = {};
    formatDeviceAddressText(device, addr, sizeof(addr));
    snprintf(demo_write_state.last_status, sizeof(demo_write_state.last_status), "dword write %s count=%d latency=%lums", addr, value_count, static_cast<unsigned long>(latency_ms));
    demo_write_state.has_result = true;
    setDemoLastWriteText("Quick dword write %s count=%d", addr, value_count);
    changeDemoView(DemoView::QuickWrite);
    return true;
}

int findFreeDemoWatchSlot() {
    for (size_t i = 0; i < kDemoWatchCapacity; ++i) {
        if (!demo_watch_items[i].active) return static_cast<int>(i);
    }
    return static_cast<int>(kDemoWatchCapacity - 1U);
}

bool addDemoWatchItem(WatchType type, char* device_token, char* points_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) {
        Serial.println(F("watch add: device parse failed"));
        return false;
    }
    uint16_t points = 1;
    const uint16_t max_points = demoMaxPointsForType(type, false);
    if (points_token && !parseDemoPointsToken(points_token, max_points, points)) {
        Serial.print(F("watch add: points must be 1.."));
        Serial.println(max_points);
        return false;
    }
    const int slot = findFreeDemoWatchSlot();
    demo_watch_items[slot] = DemoWatchItem();
    demo_watch_items[slot].active = true;
    demo_watch_items[slot].type = type;
    demo_watch_items[slot].device = device;
    demo_watch_items[slot].points = points;
    char addr[24] = {};
    formatDeviceAddressText(device, addr, sizeof(addr));
    appendDemoLog("Watch slot %d = %s %s points=%u", slot + 1, demoWatchTypeText(type), addr, points);
    changeDemoView(DemoView::Watch);
    return true;
}

void clearDemoWatchItems() {
    for (size_t i = 0; i < kDemoWatchCapacity; ++i) demo_watch_items[i] = DemoWatchItem();
    appendDemoLog("Watch list cleared");
    changeDemoView(DemoView::Watch);
}

void removeDemoWatchItem(char* index_token) {
    unsigned long parsed = 0;
    if (!parseUnsignedValue(index_token, parsed, 10) || parsed == 0U || parsed > kDemoWatchCapacity) {
        Serial.print(F("wdel usage: wdel <1.."));
        Serial.print(kDemoWatchCapacity);
        Serial.println(F(">"));
        return;
    }
    demo_watch_items[parsed - 1U] = DemoWatchItem();
    appendDemoLog("Watch slot %lu cleared", parsed);
    changeDemoView(DemoView::Watch);
}

bool configureDemoBatch(WatchType type, char* device_token, char* points_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) {
        Serial.println(F("batch: device parse failed"));
        return false;
    }
    uint16_t points = type == WatchType::Bit ? 16U : 8U;
    const uint16_t max_points = demoMaxPointsForType(type, true);
    if (points_token && !parseDemoPointsToken(points_token, max_points, points)) {
        Serial.print(F("batch: points must be 1.."));
        Serial.println(max_points);
        return false;
    }
    demo_batch_state.type = type;
    demo_batch_state.device = device;
    demo_batch_state.points = points;
    demo_batch_state.has_result = false;
    demo_batch_state.last_poll_ms = 0;
    char addr[24] = {};
    formatDeviceAddressText(device, addr, sizeof(addr));
    appendDemoLog("Batch target = %s %s points=%u", demoWatchTypeText(type), addr, points);
    changeDemoView(DemoView::Batch);
    return true;
}

bool refreshDemoBatch() {
    if (!connectPlc(false)) {
        demo_batch_state.healthy = false;
        demo_batch_state.has_result = false;
        return false;
    }
    const uint32_t started_ms = millis();
    slmp::Error err = slmp::Error::Ok;
    if (demo_batch_state.type == WatchType::Word) {
        err = plc.readWords(demo_batch_state.device, demo_batch_state.points, demo_batch_state.words, kDemoBatchMaxWordPoints);
    } else if (demo_batch_state.type == WatchType::Bit) {
        err = plc.readBits(demo_batch_state.device, demo_batch_state.points, demo_batch_state.bits, kDemoBatchMaxBitPoints);
    } else {
        err = plc.readDWords(demo_batch_state.device, demo_batch_state.points, demo_batch_state.dwords, kDemoBatchMaxDWordPoints);
    }
    const uint32_t latency_ms = millis() - started_ms;
    noteDemoRequest(err == slmp::Error::Ok ? slmp::Error::Ok : plc.lastError(), latency_ms);
    demo_batch_state.last_poll_ms = millis();
    demo_batch_state.last_error = err == slmp::Error::Ok ? slmp::Error::Ok : plc.lastError();
    demo_batch_state.last_end_code = err == slmp::Error::Ok ? 0U : plc.lastEndCode();
    demo_batch_state.healthy = err == slmp::Error::Ok;
    demo_batch_state.has_result = err == slmp::Error::Ok;
    if (err != slmp::Error::Ok) {
        setDemoLastErrorText("Batch read failed: %s end=0x%X", slmp::errorString(plc.lastError()), plc.lastEndCode());
        return false;
    }
    requestDemoRender();
    return true;
}

void pollDemoBatch() {
    if (demo_ui.active_view != DemoView::Batch && demo_ui.active_view != DemoView::Dashboard) return;
    if (millis() - demo_batch_state.last_poll_ms < kDemoBatchPollMs) return;
    (void)refreshDemoBatch();
}

bool refreshOneDemoWatch(DemoWatchItem& item) {
    if (!item.active) return false;
    const uint32_t started_ms = millis();
    slmp::Error err = slmp::Error::Ok;
    item.changed = false;

    if (item.type == WatchType::Word) {
        uint16_t values[kDemoWatchMaxWordPoints] = {};
        err = plc.readWords(item.device, item.points, values, kDemoWatchMaxWordPoints);
        if (err == slmp::Error::Ok) {
            for (uint16_t i = 0; i < item.points; ++i) {
                if (item.words[i] != values[i]) item.changed = true;
                item.words[i] = values[i];
            }
        }
    } else if (item.type == WatchType::Bit) {
        bool values[kDemoWatchMaxBitPoints] = {};
        err = plc.readBits(item.device, item.points, values, kDemoWatchMaxBitPoints);
        if (err == slmp::Error::Ok) {
            for (uint16_t i = 0; i < item.points; ++i) {
                if (item.bits[i] != values[i]) item.changed = true;
                item.bits[i] = values[i];
            }
        }
    } else {
        uint32_t values[kDemoWatchMaxDWordPoints] = {};
        err = plc.readDWords(item.device, item.points, values, kDemoWatchMaxDWordPoints);
        if (err == slmp::Error::Ok) {
            for (uint16_t i = 0; i < item.points; ++i) {
                if (item.dwords[i] != values[i]) item.changed = true;
                item.dwords[i] = values[i];
            }
        }
    }

    noteDemoRequest(err == slmp::Error::Ok ? slmp::Error::Ok : plc.lastError(), millis() - started_ms);
    item.last_poll_ms = millis();
    item.healthy = err == slmp::Error::Ok;
    item.last_error = err == slmp::Error::Ok ? slmp::Error::Ok : plc.lastError();
    item.last_end_code = err == slmp::Error::Ok ? 0U : plc.lastEndCode();
    if (!item.healthy) {
        setDemoLastErrorText("Watch read failed: %s end=0x%X", slmp::errorString(plc.lastError()), plc.lastEndCode());
        return false;
    }
    return true;
}

void pollDemoWatchList() {
    bool has_active = false;
    for (size_t i = 0; i < kDemoWatchCapacity; ++i) {
        if (demo_watch_items[i].active) { has_active = true; break; }
    }
    if (!has_active) return;
    if (!connectPlc(false)) {
        for (size_t i = 0; i < kDemoWatchCapacity; ++i) if (demo_watch_items[i].active) demo_watch_items[i].healthy = false;
        return;
    }
    bool changed_any = false;
    for (size_t i = 0; i < kDemoWatchCapacity; ++i) {
        DemoWatchItem& item = demo_watch_items[i];
        if (!item.active) continue;
        if (millis() - item.last_poll_ms < kDemoWatchPollMs) continue;
        if (refreshOneDemoWatch(item) && item.changed) changed_any = true;
    }
    if (changed_any) requestDemoRender();
}

void startDemoStress(WatchType type, const slmp::DeviceAddress& device, uint16_t points, uint32_t interval_ms) {
    stopEndurance(false, false);
    stopReconnect(false);
    stopWatch();
    demo_stress_state = DemoStressState();
    demo_stress_state.active = true;
    demo_stress_state.type = type;
    demo_stress_state.device = device;
    demo_stress_state.points = points;
    demo_stress_state.interval_ms = interval_ms < 10U ? 10U : interval_ms;
    demo_stress_state.next_due_ms = millis();
    char addr[24] = {};
    formatDeviceAddressText(device, addr, sizeof(addr));
    appendDemoLog("Stress on %s %s points=%u interval=%lu", demoWatchTypeText(type), addr, points, static_cast<unsigned long>(demo_stress_state.interval_ms));
    changeDemoView(DemoView::Stress);
}

void stopDemoStress() {
    if (!demo_stress_state.active) return;
    appendDemoLog("Stress stopped cycles=%lu ok=%lu fail=%lu", static_cast<unsigned long>(demo_stress_state.cycles), static_cast<unsigned long>(demo_stress_state.ok), static_cast<unsigned long>(demo_stress_state.fail));
    demo_stress_state.active = false;
    requestDemoRender();
}

void pollDemoStress() {
    if (!demo_stress_state.active) return;
    if (millis() < demo_stress_state.next_due_ms) return;
    if (!connectPlc(false)) {
        ++demo_stress_state.fail;
        demo_stress_state.last_error = plc.lastError();
        demo_stress_state.last_end_code = plc.lastEndCode();
        demo_stress_state.next_due_ms = millis() + demo_stress_state.interval_ms;
        requestDemoRender();
        return;
    }
    const uint32_t started_ms = millis();
    slmp::Error err = slmp::Error::Ok;
    if (demo_stress_state.type == WatchType::Word) {
        uint16_t values[kDemoBatchMaxWordPoints] = {};
        err = plc.readWords(demo_stress_state.device, demo_stress_state.points, values, kDemoBatchMaxWordPoints);
    } else if (demo_stress_state.type == WatchType::Bit) {
        bool values[kDemoBatchMaxBitPoints] = {};
        err = plc.readBits(demo_stress_state.device, demo_stress_state.points, values, kDemoBatchMaxBitPoints);
    } else {
        uint32_t values[kDemoBatchMaxDWordPoints] = {};
        err = plc.readDWords(demo_stress_state.device, demo_stress_state.points, values, kDemoBatchMaxDWordPoints);
    }
    const uint32_t latency_ms = millis() - started_ms;
    noteDemoRequest(err == slmp::Error::Ok ? slmp::Error::Ok : plc.lastError(), latency_ms);
    ++demo_stress_state.cycles;
    demo_stress_state.last_latency_ms = latency_ms;
    if (demo_stress_state.min_latency_ms == 0U || latency_ms < demo_stress_state.min_latency_ms) demo_stress_state.min_latency_ms = latency_ms;
    if (latency_ms > demo_stress_state.max_latency_ms) demo_stress_state.max_latency_ms = latency_ms;
    demo_stress_state.total_latency_ms += latency_ms;
    if (err == slmp::Error::Ok) {
        ++demo_stress_state.ok;
        demo_stress_state.last_error = slmp::Error::Ok;
        demo_stress_state.last_end_code = 0U;
    } else {
        ++demo_stress_state.fail;
        demo_stress_state.last_error = plc.lastError();
        demo_stress_state.last_end_code = plc.lastEndCode();
    }
    demo_stress_state.next_due_ms = millis() + demo_stress_state.interval_ms;
    requestDemoRender();
}

void refreshDemoDashboardData(bool force_batch) {
    pollDemoSystemMetrics();
    pollDemoWatchList();
    if (force_batch) {
        demo_batch_state.last_poll_ms = 0U;
        (void)refreshDemoBatch();
    }
}

void demoPrintFixed(const char* text, size_t width) {
    const char* src = text ? text : "";
    size_t len = strlen(src);
    if (len > width) {
        for (size_t i = 0; i + 1U < width; ++i) Serial.print(src[i]);
        if (width > 0U) Serial.print('~');
        return;
    }
    Serial.print(src);
    for (size_t i = len; i < width; ++i) Serial.print(' ');
}

void demoPrintRow(const char* left, const char* main, const char* right) {
    demoPrintFixed(left, 18);
    Serial.print(" | ");
    demoPrintFixed(main, 54);
    Serial.print(" | ");
    demoPrintFixed(right, 24);
    Serial.println();
}

void buildMenuLine(size_t row, char* out, size_t capacity) {
    static const DemoView views[] = {
        DemoView::Dashboard, DemoView::Connect, DemoView::QuickRead, DemoView::QuickWrite,
        DemoView::Watch, DemoView::Batch, DemoView::Stress, DemoView::System, DemoView::Log,
    };
    static const char* labels[] = {
        "1 Home", "2 Connect", "3 Quick Read", "4 Quick Write", "5 Watch",
        "6 Batch", "7 Stress", "8 System", "9 Log",
    };
    if (row >= sizeof(labels) / sizeof(labels[0])) {
        out[0] = '\0';
        return;
    }
    snprintf(out, capacity, "%c %s", demo_ui.active_view == views[row] ? '>' : ' ', labels[row]);
}

void formatDemoWatchSummary(const DemoWatchItem& item, char* out, size_t capacity, size_t index) {
    if (!item.active) {
        snprintf(out, capacity, "[%u] <empty>", static_cast<unsigned>(index + 1U));
        return;
    }
    char addr[24] = {};
    formatDeviceAddressText(item.device, addr, sizeof(addr));
    if (!item.healthy) {
        snprintf(out, capacity, "[%u] %s %s !ERR", static_cast<unsigned>(index + 1U), demoWatchTypeText(item.type), addr);
        return;
    }
    if (item.type == WatchType::Word) {
        snprintf(out, capacity, "[%u] %s = %u%s", static_cast<unsigned>(index + 1U), addr, item.words[0], item.changed ? " *" : "");
    } else if (item.type == WatchType::Bit) {
        snprintf(out, capacity, "[%u] %s = %s%s", static_cast<unsigned>(index + 1U), addr, item.bits[0] ? "ON" : "OFF", item.changed ? " *" : "");
    } else {
        snprintf(out, capacity, "[%u] %s = %lu%s", static_cast<unsigned>(index + 1U), addr, static_cast<unsigned long>(item.dwords[0]), item.changed ? " *" : "");
    }
}

void buildRightLines(char lines[kDemoMainRowCount][kDemoLineCapacity]) {
    for (size_t i = 0; i < kDemoMainRowCount; ++i) lines[i][0] = '\0';
    snprintf(lines[0], kDemoLineCapacity, "Model: %s", cached_plc_model);
    snprintf(lines[1], kDemoLineCapacity, "PLC: %s", plc.connected() ? "connected" : "disconnected");
    snprintf(lines[2], kDemoLineCapacity, "Ethernet: %s", Ethernet.connected() ? "up" : "down");
    snprintf(lines[3], kDemoLineCapacity, "Requests: %lu", static_cast<unsigned long>(demo_system_metrics.request_count));
    snprintf(lines[4], kDemoLineCapacity, "Errors: %lu", static_cast<unsigned long>(demo_system_metrics.error_count));
    snprintf(lines[5], kDemoLineCapacity, "Last latency: %lums", static_cast<unsigned long>(demo_system_metrics.last_latency_ms));
    snprintf(lines[6], kDemoLineCapacity, "Peak latency: %lums", static_cast<unsigned long>(demo_system_metrics.peak_latency_ms));
    copyText(lines[7], kDemoLineCapacity, "Last write:");
    copyText(lines[8], kDemoLineCapacity, demo_last_write);
    copyText(lines[9], kDemoLineCapacity, "Last error:");
    copyText(lines[10], kDemoLineCapacity, demo_last_error);
    copyText(lines[11], kDemoLineCapacity, "Watch slots:");
    size_t summary_row = 12U;
    for (size_t i = 0; i < kDemoWatchCapacity && summary_row < kDemoMainRowCount; ++i, ++summary_row) {
        formatDemoWatchSummary(demo_watch_items[i], lines[summary_row], kDemoLineCapacity, i);
    }
}

void formatBatchLine(size_t row, char* out, size_t capacity) {
    out[0] = '\0';
    if (!demo_batch_state.has_result) {
        if (row == 0U) copyText(out, capacity, "No batch data yet. Use brun.");
        return;
    }
    if (demo_batch_state.type == WatchType::Word) {
        if (row >= demo_batch_state.points) return;
        char addr[24] = {};
        formatDeviceAddressText(demo_batch_state.device, addr, sizeof(addr), static_cast<uint32_t>(row));
        snprintf(out, capacity, "%s = %u (0x%04X)", addr, demo_batch_state.words[row], demo_batch_state.words[row]);
        return;
    }
    if (demo_batch_state.type == WatchType::DWord) {
        if (row >= demo_batch_state.points) return;
        char addr[24] = {};
        formatDeviceAddressText(demo_batch_state.device, addr, sizeof(addr), static_cast<uint32_t>(row * 2U));
        snprintf(out, capacity, "%s = %lu (0x%08lX)", addr, static_cast<unsigned long>(demo_batch_state.dwords[row]), static_cast<unsigned long>(demo_batch_state.dwords[row]));
        return;
    }
    if (row == 0U) {
        char addr[24] = {};
        formatDeviceAddressText(demo_batch_state.device, addr, sizeof(addr));
        char bits_text[64] = {};
        size_t used = 0U;
        for (uint16_t i = 0; i < demo_batch_state.points && used + 4U < sizeof(bits_text); ++i) {
            used += static_cast<size_t>(snprintf(bits_text + used, sizeof(bits_text) - used, "%u:%u ", i, demo_batch_state.bits[i] ? 1U : 0U));
        }
        snprintf(out, capacity, "%s bits %s", addr, bits_text);
    }
}

void buildMainLines(char lines[kDemoMainRowCount][kDemoLineCapacity]) {
    for (size_t i = 0; i < kDemoMainRowCount; ++i) lines[i][0] = '\0';
    switch (demo_ui.active_view) {
        case DemoView::Dashboard:
            copyText(lines[0], kDemoLineCapacity, "Demo dashboard ready");
            snprintf(lines[1], kDemoLineCapacity, "Host=%s Port=%u Transport=%s Frame=%s Compat=%s", plc_host, console_link.plc_port, transportModeText(console_link.transport_mode), frameTypeText(console_link.frame_type), compatibilityModeText(console_link.compatibility_mode));
            snprintf(lines[2], kDemoLineCapacity, "Target=%u/%u/0x%04X/%u Timeout=%lu Monitor=%u", plc.target().network, plc.target().station, plc.target().module_io, plc.target().multidrop, static_cast<unsigned long>(plc.timeoutMs()), plc.monitoringTimer());
            copyText(lines[4], kDemoLineCapacity, "Quick read: qr/qrb/qrd <dev> [pts]");
            copyText(lines[5], kDemoLineCapacity, "Quick write: qw/qwb/qwd <dev> <values...>");
            copyText(lines[6], kDemoLineCapacity, "Watch edit: wa/wab/wad, wdel, wclear");
            copyText(lines[7], kDemoLineCapacity, "Batch: batch/batchb/batchd then brun");
            copyText(lines[8], kDemoLineCapacity, "Stress: stressw/stressb/stressd <dev> [pts] [ms]");
            for (size_t i = 0; i < 4U && i + 10U < kDemoMainRowCount; ++i) {
                formatDemoWatchSummary(demo_watch_items[i], lines[i + 10U], kDemoLineCapacity, i);
            }
            break;

        case DemoView::Connect:
            copyText(lines[0], kDemoLineCapacity, "Connection editor");
            snprintf(lines[1], kDemoLineCapacity, "host %s", plc_host);
            snprintf(lines[2], kDemoLineCapacity, "port %u", console_link.plc_port);
            snprintf(lines[3], kDemoLineCapacity, "transport %s", transportModeText(console_link.transport_mode));
            snprintf(lines[4], kDemoLineCapacity, "frame %s", frameTypeText(console_link.frame_type));
            snprintf(lines[5], kDemoLineCapacity, "compat %s", compatibilityModeText(console_link.compatibility_mode));
            snprintf(lines[6], kDemoLineCapacity, "target %u %u 0x%04X %u", plc.target().network, plc.target().station, plc.target().module_io, plc.target().multidrop);
            snprintf(lines[7], kDemoLineCapacity, "timeout %lu", static_cast<unsigned long>(plc.timeoutMs()));
            snprintf(lines[8], kDemoLineCapacity, "monitor %u", plc.monitoringTimer());
            copyText(lines[10], kDemoLineCapacity, "Commands:");
            copyText(lines[11], kDemoLineCapacity, "host, port, transport, frame, compat");
            copyText(lines[12], kDemoLineCapacity, "target, timeout, monitor, connect, close");
            break;

        case DemoView::QuickRead: {
            copyText(lines[0], kDemoLineCapacity, "Quick read target");
            char addr[24] = {};
            formatDeviceAddressText(demo_read_state.device, addr, sizeof(addr));
            snprintf(lines[1], kDemoLineCapacity, "%s %s points=%u", demoWatchTypeText(demo_read_state.type), addr, demo_read_state.points);
            copyText(lines[2], kDemoLineCapacity, "Commands: qr/qrb/qrd <dev> [pts]");
            for (uint8_t i = 0; i < demo_read_state.line_count && i + 4U < kDemoMainRowCount; ++i) {
                copyText(lines[i + 4U], kDemoLineCapacity, demo_read_state.lines[i]);
            }
            break;
        }

        case DemoView::QuickWrite: {
            copyText(lines[0], kDemoLineCapacity, "Quick write target");
            char addr[24] = {};
            formatDeviceAddressText(demo_write_state.device, addr, sizeof(addr));
            snprintf(lines[1], kDemoLineCapacity, "%s %s", demoWatchTypeText(demo_write_state.type), addr);
            copyText(lines[2], kDemoLineCapacity, "Commands: qw/qwb/qwd <dev> <values...>");
            if (demo_write_state.has_result) copyText(lines[4], kDemoLineCapacity, demo_write_state.last_status);
            break;
        }

        case DemoView::Watch:
            copyText(lines[0], kDemoLineCapacity, "Watch list");
            copyText(lines[1], kDemoLineCapacity, "Commands: wa/wab/wad, wdel <index>, wclear");
            for (size_t i = 0; i < kDemoWatchCapacity && i + 3U < kDemoMainRowCount; ++i) {
                formatDemoWatchSummary(demo_watch_items[i], lines[i + 3U], kDemoLineCapacity, i);
            }
            break;

        case DemoView::Batch: {
            char addr[24] = {};
            formatDeviceAddressText(demo_batch_state.device, addr, sizeof(addr));
            snprintf(lines[0], kDemoLineCapacity, "Batch target = %s %s points=%u", demoWatchTypeText(demo_batch_state.type), addr, demo_batch_state.points);
            copyText(lines[1], kDemoLineCapacity, "Commands: batch/batchb/batchd <dev> [pts], brun");
            for (size_t i = 0; i + 3U < kDemoMainRowCount; ++i) {
                formatBatchLine(i, lines[i + 3U], kDemoLineCapacity);
            }
            break;
        }

        case DemoView::Stress: {
            char addr[24] = {};
            formatDeviceAddressText(demo_stress_state.device, addr, sizeof(addr));
            snprintf(lines[0], kDemoLineCapacity, "Stress %s", demo_stress_state.active ? "running" : "stopped");
            snprintf(lines[1], kDemoLineCapacity, "%s %s points=%u interval=%lu", demoWatchTypeText(demo_stress_state.type), addr, demo_stress_state.points, static_cast<unsigned long>(demo_stress_state.interval_ms));
            copyText(lines[2], kDemoLineCapacity, "Commands: stressw/stressb/stressd <dev> [pts] [ms]");
            copyText(lines[3], kDemoLineCapacity, "stress start [ms] | stress stop | stress status");
            snprintf(lines[5], kDemoLineCapacity, "cycles=%lu ok=%lu fail=%lu", static_cast<unsigned long>(demo_stress_state.cycles), static_cast<unsigned long>(demo_stress_state.ok), static_cast<unsigned long>(demo_stress_state.fail));
            snprintf(lines[6], kDemoLineCapacity, "latency last/min/max=%lu/%lu/%lu ms", static_cast<unsigned long>(demo_stress_state.last_latency_ms), static_cast<unsigned long>(demo_stress_state.min_latency_ms), static_cast<unsigned long>(demo_stress_state.max_latency_ms));
            snprintf(lines[7], kDemoLineCapacity, "avg latency=%lu ms", demo_stress_state.cycles == 0U ? 0UL : static_cast<unsigned long>(demo_stress_state.total_latency_ms / demo_stress_state.cycles));
            break;
        }

        case DemoView::System:
            copyText(lines[0], kDemoLineCapacity, "System metrics");
            snprintf(lines[1], kDemoLineCapacity, "uptime=%lu ms", static_cast<unsigned long>(demo_system_metrics.uptime_ms));
            if (demo_system_metrics.memory_supported) {
                snprintf(lines[2], kDemoLineCapacity, "free_heap=%lu bytes", static_cast<unsigned long>(demo_system_metrics.free_heap_bytes));
                snprintf(lines[3], kDemoLineCapacity, "min_free_heap=%lu bytes", static_cast<unsigned long>(demo_system_metrics.min_free_heap_bytes));
            } else {
                copyText(lines[2], kDemoLineCapacity, "free_heap=n/a");
                copyText(lines[3], kDemoLineCapacity, "min_free_heap=n/a");
            }
            snprintf(lines[4], kDemoLineCapacity, "loop_avg=%lu us", static_cast<unsigned long>(demo_system_metrics.loop_avg_us));
            snprintf(lines[5], kDemoLineCapacity, "loop_max=%lu us", static_cast<unsigned long>(demo_system_metrics.loop_max_us));
            snprintf(lines[6], kDemoLineCapacity, "estimated_cpu=%u%%", demo_system_metrics.cpu_load_percent);
            snprintf(lines[7], kDemoLineCapacity, "ethernet=%s socket=%s", demo_system_metrics.link_up ? "up" : "down", demo_system_metrics.socket_open ? "open" : "closed");
            snprintf(lines[8], kDemoLineCapacity, "requests=%lu errors=%lu", static_cast<unsigned long>(demo_system_metrics.request_count), static_cast<unsigned long>(demo_system_metrics.error_count));
            snprintf(lines[9], kDemoLineCapacity, "latency last/peak=%lu/%lu ms", static_cast<unsigned long>(demo_system_metrics.last_latency_ms), static_cast<unsigned long>(demo_system_metrics.peak_latency_ms));
            break;

        case DemoView::Log:
            copyText(lines[0], kDemoLineCapacity, "Log view");
            copyText(lines[1], kDemoLineCapacity, "Command: log clear");
            for (size_t i = 0; i + 2U < kDemoMainRowCount && i < demo_log_count; ++i) {
                const size_t reverse_index = (demo_log_next + kDemoLogCapacity - 1U - i) % kDemoLogCapacity;
                snprintf(lines[i + 2U], kDemoLineCapacity, "[%lu] %s", static_cast<unsigned long>(demo_logs[reverse_index].stamp_ms), demo_logs[reverse_index].text);
            }
            break;
    }
}

void renderDemoScreen() {
    if (!demo_ui.ansi_enabled) return;

    char main_lines[kDemoMainRowCount][kDemoLineCapacity] = {};
    char right_lines[kDemoMainRowCount][kDemoLineCapacity] = {};
    buildMainLines(main_lines);
    buildRightLines(right_lines);

    Serial.print("\x1B[2J\x1B[H");
    Serial.print("\x1B[36m");
    Serial.println("==============================================================================================");
    Serial.print("\x1B[0m");
    Serial.print(" SLMP Demo | View=");
    Serial.print(demoViewText(demo_ui.active_view));
    Serial.print(" | Host=");
    Serial.print(plc_host);
    Serial.print(":");
    Serial.print(console_link.plc_port);
    Serial.print(" | Model=");
    Serial.print(cached_plc_model);
    Serial.print(" | Link=");
    Serial.print(Ethernet.connected() ? "UP" : "DOWN");
    Serial.print(" | PLC=");
    Serial.print(plc.connected() ? "ONLINE" : "OFFLINE");
    Serial.print(" | Last=");
    Serial.print(demo_system_metrics.last_latency_ms);
    Serial.println("ms");
    Serial.println("----------------------------------------------------------------------------------------------");

    for (size_t row = 0; row < kDemoMainRowCount; ++row) {
        char menu_line[kDemoLineCapacity] = {};
        buildMenuLine(row, menu_line, sizeof(menu_line));
        demoPrintRow(menu_line, main_lines[row], right_lines[row]);
    }

    Serial.println("----------------------------------------------------------------------------------------------");
    Serial.print(" CPU=");
    Serial.print(demo_system_metrics.cpu_load_percent);
    Serial.print("% | Uptime=");
    Serial.print(demo_system_metrics.uptime_ms);
    Serial.print("ms | FreeHeap=");
    if (demo_system_metrics.memory_supported) {
        Serial.print(demo_system_metrics.free_heap_bytes);
        Serial.print("B");
    } else {
        Serial.print("n/a");
    }
    Serial.print(" | Req=");
    Serial.print(demo_system_metrics.request_count);
    Serial.print(" | Err=");
    Serial.print(demo_system_metrics.error_count);
    Serial.print(" | Stress=");
    Serial.println(demo_stress_state.active ? "ON" : "OFF");
    Serial.println(" Cmd: home | 1..9 | host <ip> | qr/qw | wa/wdel | batch/brun | stressw/stressb/stressd");
    Serial.print("> ");

    demo_ui.force_render = false;
    demo_ui.next_render_ms = millis() + kDemoRenderIntervalMs;
}

void renderDemoScreenIfNeeded() {
    if (!demo_ui.ansi_enabled) return;
    if (serial_line_length != 0U) return;
    if (!demo_ui.force_render && millis() < demo_ui.next_render_ms) return;
    renderDemoScreen();
}

bool parseDemoView(char* token, DemoView& view) {
    if (!token) return false;
    uppercaseInPlace(token);
    if (strcmp(token, "HOME") == 0 || strcmp(token, "DASHBOARD") == 0) { view = DemoView::Dashboard; return true; }
    if (strcmp(token, "CONNECT") == 0 || strcmp(token, "CONFIG") == 0) { view = DemoView::Connect; return true; }
    if (strcmp(token, "READ") == 0 || strcmp(token, "QUICKREAD") == 0 || strcmp(token, "QR") == 0) { view = DemoView::QuickRead; return true; }
    if (strcmp(token, "WRITE") == 0 || strcmp(token, "QUICKWRITE") == 0 || strcmp(token, "QW") == 0) { view = DemoView::QuickWrite; return true; }
    if (strcmp(token, "WATCH") == 0) { view = DemoView::Watch; return true; }
    if (strcmp(token, "BATCH") == 0) { view = DemoView::Batch; return true; }
    if (strcmp(token, "STRESS") == 0) { view = DemoView::Stress; return true; }
    if (strcmp(token, "SYSTEM") == 0) { view = DemoView::System; return true; }
    if (strcmp(token, "LOG") == 0) { view = DemoView::Log; return true; }
    return false;
}

bool handleDemoNumericShortcut(const char* cmd) {
    if (strcmp(cmd, "1") == 0) { changeDemoView(DemoView::Dashboard); return true; }
    if (strcmp(cmd, "2") == 0) { changeDemoView(DemoView::Connect); return true; }
    if (strcmp(cmd, "3") == 0) { changeDemoView(DemoView::QuickRead); return true; }
    if (strcmp(cmd, "4") == 0) { changeDemoView(DemoView::QuickWrite); return true; }
    if (strcmp(cmd, "5") == 0) { changeDemoView(DemoView::Watch); return true; }
    if (strcmp(cmd, "6") == 0) { changeDemoView(DemoView::Batch); return true; }
    if (strcmp(cmd, "7") == 0) { changeDemoView(DemoView::Stress); return true; }
    if (strcmp(cmd, "8") == 0) { changeDemoView(DemoView::System); return true; }
    if (strcmp(cmd, "9") == 0) { changeDemoView(DemoView::Log); return true; }
    return false;
}

bool handleDemoCommand(char* tokens[], int token_count) {
    if (token_count <= 0) return false;
    const char* cmd = tokens[0];

    if (handleDemoNumericShortcut(cmd)) return true;
    if (strcmp(cmd, "HOME") == 0 || strcmp(cmd, "DASHBOARD") == 0) {
        changeDemoView(DemoView::Dashboard);
        return true;
    }
    if (strcmp(cmd, "VIEW") == 0 || strcmp(cmd, "MENU") == 0) {
        DemoView view = DemoView::Dashboard;
        if (token_count < 2 || !parseDemoView(tokens[1], view)) {
            Serial.println(F("view usage: view <home|connect|read|write|watch|batch|stress|system|log>"));
            return true;
        }
        changeDemoView(view);
        return true;
    }
    if (strcmp(cmd, "REFRESH") == 0) {
        refreshDemoDashboardData(true);
        requestDemoRender();
        return true;
    }
    if (strcmp(cmd, "ANSI") == 0) {
        if (token_count == 1) {
            Serial.print(F("ansi="));
            Serial.println(demo_ui.ansi_enabled ? F("on") : F("off"));
            return true;
        }
        uppercaseInPlace(tokens[1]);
        if (strcmp(tokens[1], "ON") == 0) {
            demo_ui.ansi_enabled = true;
            requestDemoRender();
            return true;
        }
        if (strcmp(tokens[1], "OFF") == 0) {
            demo_ui.ansi_enabled = false;
            Serial.println(F("ansi=off"));
            return true;
        }
        Serial.println(F("ansi usage: ansi [on|off]"));
        return true;
    }
    if (strcmp(cmd, "HOST") == 0) {
        setDemoHost(token_count > 1 ? tokens[1] : nullptr);
        return true;
    }
    if (strcmp(cmd, "QR") == 0 || strcmp(cmd, "QRB") == 0 || strcmp(cmd, "QRD") == 0) {
        WatchType type = WatchType::Word;
        if (strcmp(cmd, "QRB") == 0) type = WatchType::Bit;
        if (strcmp(cmd, "QRD") == 0) type = WatchType::DWord;
        if (token_count < 2) {
            Serial.println(F("qr usage: qr/qrb/qrd <device> [points]"));
            return true;
        }
        slmp::DeviceAddress device = {};
        if (!parseDeviceAddress(tokens[1], device)) {
            Serial.println(F("qr: device parse failed"));
            return true;
        }
        uint16_t points = type == WatchType::Bit ? 8U : 4U;
        const uint16_t max_points = demoMaxPointsForType(type, true);
        if (token_count >= 3 && !parseDemoPointsToken(tokens[2], max_points, points)) {
            Serial.print(F("qr: points must be 1.."));
            Serial.println(max_points);
            return true;
        }
        (void)runDemoRead(type, device, points);
        return true;
    }
    if (strcmp(cmd, "QW") == 0 || strcmp(cmd, "QWB") == 0 || strcmp(cmd, "QWD") == 0) {
        if (token_count < 3) {
            Serial.println(F("qw usage: qw/qwb/qwd <device> <value...>"));
            return true;
        }
        slmp::DeviceAddress device = {};
        if (!parseDeviceAddress(tokens[1], device)) {
            Serial.println(F("qw: device parse failed"));
            return true;
        }
        if (strcmp(cmd, "QWB") == 0) {
            (void)runDemoWriteBits(device, tokens, token_count, 2);
            return true;
        }
        if (strcmp(cmd, "QWD") == 0) {
            (void)runDemoWriteDWords(device, tokens, token_count, 2);
            return true;
        }
        (void)runDemoWriteWords(device, tokens, token_count, 2);
        return true;
    }
    if (strcmp(cmd, "WA") == 0 || strcmp(cmd, "WAB") == 0 || strcmp(cmd, "WAD") == 0) {
        WatchType type = WatchType::Word;
        if (strcmp(cmd, "WAB") == 0) type = WatchType::Bit;
        if (strcmp(cmd, "WAD") == 0) type = WatchType::DWord;
        if (token_count < 2) {
            Serial.println(F("wa usage: wa/wab/wad <device> [points]"));
            return true;
        }
        (void)addDemoWatchItem(type, tokens[1], token_count > 2 ? tokens[2] : nullptr);
        return true;
    }
    if (strcmp(cmd, "WDEL") == 0) {
        if (token_count < 2) {
            Serial.print(F("wdel usage: wdel <1.."));
            Serial.print(kDemoWatchCapacity);
            Serial.println(F(">"));
            return true;
        }
        removeDemoWatchItem(tokens[1]);
        return true;
    }
    if (strcmp(cmd, "WCLEAR") == 0) {
        clearDemoWatchItems();
        return true;
    }
    if (strcmp(cmd, "BATCH") == 0 || strcmp(cmd, "BATCHB") == 0 || strcmp(cmd, "BATCHD") == 0) {
        WatchType type = WatchType::Word;
        if (strcmp(cmd, "BATCHB") == 0) type = WatchType::Bit;
        if (strcmp(cmd, "BATCHD") == 0) type = WatchType::DWord;
        if (token_count < 2) {
            Serial.println(F("batch usage: batch/batchb/batchd <device> [points]"));
            return true;
        }
        (void)configureDemoBatch(type, tokens[1], token_count > 2 ? tokens[2] : nullptr);
        return true;
    }
    if (strcmp(cmd, "BRUN") == 0) {
        (void)refreshDemoBatch();
        changeDemoView(DemoView::Batch);
        return true;
    }
    if (strcmp(cmd, "STRESS") == 0) {
        if (token_count == 1) {
            changeDemoView(DemoView::Stress);
            return true;
        }
        uppercaseInPlace(tokens[1]);
        if (strcmp(tokens[1], "STATUS") == 0) {
            changeDemoView(DemoView::Stress);
            return true;
        }
        if (strcmp(tokens[1], "STOP") == 0 || strcmp(tokens[1], "OFF") == 0) {
            stopDemoStress();
            return true;
        }
        if (strcmp(tokens[1], "START") == 0) {
            uint32_t interval_ms = demo_stress_state.interval_ms == 0U ? kDemoDefaultStressIntervalMs : demo_stress_state.interval_ms;
            if (token_count > 2) {
                unsigned long parsed = 0;
                if (!parseUnsignedValue(tokens[2], parsed, 10)) {
                    Serial.println(F("stress usage: stress start [interval_ms]"));
                    return true;
                }
                interval_ms = static_cast<uint32_t>(parsed);
            }
            startDemoStress(demo_read_state.type, demo_read_state.device, demo_read_state.points, interval_ms);
            return true;
        }
        Serial.println(F("stress usage: stress [start|stop|status]"));
        return true;
    }
    if (strcmp(cmd, "STRESSW") == 0 || strcmp(cmd, "STRESSB") == 0 || strcmp(cmd, "STRESSD") == 0) {
        WatchType type = WatchType::Word;
        if (strcmp(cmd, "STRESSB") == 0) type = WatchType::Bit;
        if (strcmp(cmd, "STRESSD") == 0) type = WatchType::DWord;
        if (token_count < 2) {
            Serial.println(F("stressw usage: stressw/stressb/stressd <device> [points] [interval_ms]"));
            return true;
        }
        slmp::DeviceAddress device = {};
        if (!parseDeviceAddress(tokens[1], device)) {
            Serial.println(F("stress: device parse failed"));
            return true;
        }
        uint16_t points = type == WatchType::Bit ? 8U : 4U;
        const uint16_t max_points = demoMaxPointsForType(type, true);
        if (token_count >= 3 && !parseDemoPointsToken(tokens[2], max_points, points)) {
            Serial.print(F("stress: points must be 1.."));
            Serial.println(max_points);
            return true;
        }
        uint32_t interval_ms = kDemoDefaultStressIntervalMs;
        if (token_count >= 4) {
            unsigned long parsed = 0;
            if (!parseUnsignedValue(tokens[3], parsed, 10)) {
                Serial.println(F("stress: interval_ms must be numeric"));
                return true;
            }
            interval_ms = static_cast<uint32_t>(parsed);
        }
        startDemoStress(type, device, points, interval_ms);
        return true;
    }
    if (strcmp(cmd, "LOG") == 0 && token_count >= 2) {
        uppercaseInPlace(tokens[1]);
        if (strcmp(tokens[1], "CLEAR") == 0) {
            demo_log_count = 0;
            demo_log_next = 0;
            appendDemoLog("Log cleared");
            changeDemoView(DemoView::Log);
            return true;
        }
    }
    if (strcmp(cmd, "CONNECT") == 0) {
        (void)runDemoConnect(true);
        return true;
    }
    if (strcmp(cmd, "CLOSE") == 0) {
        runDemoDisconnect();
        return true;
    }
    return false;
}
