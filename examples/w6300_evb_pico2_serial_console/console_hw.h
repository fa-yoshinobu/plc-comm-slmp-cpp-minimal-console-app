/**
 * @file console_hw.h
 * @brief Ethernet/connection management and link-setting commands.
 *
 * Included by w6300_evb_pico2_serial_console.h inside its namespace.
 * All constants, types, and globals from the main header are visible here.
 */

// ---------------------------------------------------------------------------
// Parser / print utilities
// ---------------------------------------------------------------------------
const DeviceSpec* findDeviceSpecByName(const char* token) {
    const DeviceSpec* match = nullptr;
    size_t match_len = 0;
    for (size_t i = 0; i < sizeof(kDeviceSpecs) / sizeof(kDeviceSpecs[0]); ++i) {
        const size_t name_len = strlen(kDeviceSpecs[i].name);
        if (name_len < match_len) continue;
        if (strncmp(token, kDeviceSpecs[i].name, name_len) == 0) {
            match = &kDeviceSpecs[i];
            match_len = name_len;
        }
    }
    return match;
}

const DeviceSpec* findDeviceSpecByCode(slmp::DeviceCode code) {
    for (size_t i = 0; i < sizeof(kDeviceSpecs) / sizeof(kDeviceSpecs[0]); ++i) {
        if (kDeviceSpecs[i].code == code) return &kDeviceSpecs[i];
    }
    return nullptr;
}

void uppercaseInPlace(char* text) {
    if (!text) return;
    for (; *text != '\0'; ++text) *text = static_cast<char>(toupper(static_cast<unsigned char>(*text)));
}

bool parseUnsignedValue(const char* token, unsigned long& value, int base) {
    if (!token || *token == '\0') return false;
    char* end = nullptr;
    value = strtoul(token, &end, base);
    return end != token && *end == '\0';
}

bool parseWordValue(const char* token, uint16_t& value) {
    unsigned long v = 0;
    if (!parseUnsignedValue(token, v, 0) || v > 0xFFFFUL) return false;
    value = static_cast<uint16_t>(v); return true;
}

bool parseDWordValue(const char* token, uint32_t& value) {
    unsigned long v = 0;
    if (!parseUnsignedValue(token, v, 0)) return false;
    value = static_cast<uint32_t>(v); return true;
}

bool parseByteValue(const char* token, uint8_t& value) {
    unsigned long v = 0;
    if (!parseUnsignedValue(token, v, 0) || v > 0xFFUL) return false;
    value = static_cast<uint8_t>(v); return true;
}

bool parseUint16Value(const char* token, uint16_t& value) {
    unsigned long v = 0;
    if (!parseUnsignedValue(token, v, 0) || v > 0xFFFFUL) return false;
    value = static_cast<uint16_t>(v); return true;
}

bool parseListCount(const char* token, size_t max_value, uint8_t& count) {
    unsigned long v = 0;
    if (!parseUnsignedValue(token, v, 10) || v > max_value) return false;
    count = static_cast<uint8_t>(v); return true;
}

bool parsePointCount(const char* token, uint16_t& points, uint16_t max_pts = static_cast<uint16_t>(kMaxPoints)) {
    unsigned long v = 0;
    if (!parseUnsignedValue(token, v, 10) || v == 0 || v > max_pts) return false;
    points = static_cast<uint16_t>(v); return true;
}

bool parsePositiveCount(const char* token, uint16_t max_value, uint16_t& count) {
    unsigned long v = 0;
    if (!parseUnsignedValue(token, v, 10) || v == 0 || v > max_value) return false;
    count = static_cast<uint16_t>(v); return true;
}

bool parseBoolValue(char* token, bool& value) {
    if (!token) return false;
    uppercaseInPlace(token);
    if (strcmp(token, "1") == 0 || strcmp(token, "ON") == 0 || strcmp(token, "TRUE") == 0) {
        value = true; return true;
    }
    if (strcmp(token, "0") == 0 || strcmp(token, "OFF") == 0 || strcmp(token, "FALSE") == 0) {
        value = false; return true;
    }
    return false;
}

bool parseDeviceAddress(char* token, slmp::DeviceAddress& device) {
    if (!token || *token == '\0') return false;
    uppercaseInPlace(token);
    const DeviceSpec* spec = findDeviceSpecByName(token);
    if (!spec) return false;
    const size_t prefix_len = strlen(spec->name);
    unsigned long number = 0;
    if (!parseUnsignedValue(token + prefix_len, number, spec->hex_address ? 16 : 10)) return false;
    device.code = spec->code;
    device.number = static_cast<uint32_t>(number);
    return true;
}

void printDeviceAddress(const slmp::DeviceAddress& device, uint32_t offset = 0) {
    const DeviceSpec* spec = findDeviceSpecByCode(device.code);
    if (!spec) { Serial.print('?'); Serial.print(device.number + offset); return; }
    Serial.print(spec->name);
    if (spec->hex_address) Serial.print(device.number + offset, HEX);
    else                   Serial.print(device.number + offset);
}

int splitTokens(char* line, char* tokens[], int capacity) {
    int count = 0;
    char* cur = line;
    while (*cur != '\0' && count < capacity) {
        while (*cur != '\0' && isspace(static_cast<unsigned char>(*cur))) ++cur;
        if (*cur == '\0') break;
        tokens[count++] = cur;
        while (*cur != '\0' && !isspace(static_cast<unsigned char>(*cur))) ++cur;
        if (*cur == '\0') break;
        *cur++ = '\0';
    }
    return count;
}

void joinTokens(char* tokens[], int start, int token_count, char* out, size_t out_capacity) {
    if (!out || out_capacity == 0) return;
    out[0] = '\0';
    size_t used = 0;
    for (int i = start; i < token_count; ++i) {
        const char* tok = tokens[i];
        if (!tok || *tok == '\0') continue;
        if (used != 0) {
            if (used + 1 >= out_capacity) break;
            out[used++] = ' '; out[used] = '\0';
        }
        const size_t avail = out_capacity - used - 1;
        const size_t tlen  = strlen(tok);
        const size_t copy  = tlen < avail ? tlen : avail;
        memcpy(out + used, tok, copy);
        used += copy; out[used] = '\0';
        if (copy < tlen) break;
    }
}

void copyText(char* out, size_t capacity, const char* text) {
    if (!out || capacity == 0) return;
    size_t i = 0;
    if (text) for (; i + 1 < capacity && text[i] != '\0'; ++i) out[i] = text[i];
    out[i] = '\0';
}

uint16_t readFrameLe16(const uint8_t* b) {
    if (!b) return 0;
    return static_cast<uint16_t>(b[0]) | (static_cast<uint16_t>(b[1]) << 8U);
}

// ---------------------------------------------------------------------------
// Text helpers
// ---------------------------------------------------------------------------
const char* transportModeText(TransportMode m) {
    return m == TransportMode::Udp ? "udp" : "tcp";
}
const char* frameTypeText(slmp::FrameType f) {
    return f == slmp::FrameType::Frame3E ? "3e" : "4e";
}
const char* compatibilityModeText(slmp::CompatibilityMode m) {
    return m == slmp::CompatibilityMode::Legacy ? "legacy" : "iqr";
}

// ---------------------------------------------------------------------------
// Ethernet
// ---------------------------------------------------------------------------
void pulseEthernetReset() {
    pinMode(kEthernetResetPin, OUTPUT);
    digitalWrite(kEthernetResetPin, LOW);  delay(100);
    digitalWrite(kEthernetResetPin, HIGH); delay(100);
}

bool bringUpEthernet() {
    pulseEthernetReset();
    Ethernet.config(kLocalIp, kGateway, kSubnet, kDns);
    if (!Ethernet.begin()) {
        ethernet_ready = false;
        Serial.println(F("ethernet begin failed"));
        return false;
    }
    const IPAddress local_ip = Ethernet.localIP();
    ethernet_ready = (local_ip == kLocalIp);
    Serial.print(F("local_ip=")); Serial.println(local_ip);
    Serial.print(F("gateway="));  Serial.println(Ethernet.gatewayIP());
    Serial.print(F("subnet="));   Serial.println(Ethernet.subnetMask());
    Serial.print(F("ethernet="));
    Serial.println(Ethernet.connected() ? F("up") : F("down"));
    if (!ethernet_ready) Serial.println(F("static IP apply failed"));
    return ethernet_ready;
}

bool ensureEthernet() {
    return ethernet_ready ? true : bringUpEthernet();
}

// ---------------------------------------------------------------------------
// Connect / close
// ---------------------------------------------------------------------------
bool connectPlc(bool verbose) {
    if (!ensureEthernet()) return false;
    if (plc.connected()) {
        if (verbose) Serial.println(F("already connected"));
        return true;
    }
    plc.setFrameType(console_link.frame_type);
    plc.setCompatibilityMode(console_link.compatibility_mode);
    if (!plc.connect(kPlcHost, console_link.plc_port)) {
        if (verbose) {
            Serial.print(F("connect failed: "));
            Serial.println(slmp::errorString(plc.lastError()));
        }
        return false;
    }
    slmp::TypeNameInfo type_name = {};
    const bool type_name_ok = (plc.readTypeName(type_name) == slmp::Error::Ok);
    if (verbose) {
        Serial.print(F("transport=")); Serial.print(transportModeText(console_link.transport_mode));
        Serial.print(F(" frame="));   Serial.print(frameTypeText(console_link.frame_type));
        Serial.print(F(" compat="));  Serial.println(compatibilityModeText(console_link.compatibility_mode));
        if (type_name_ok) {
            Serial.print(F("model=")); Serial.print(type_name.model);
            if (type_name.has_model_code) { Serial.print(F(" code=0x")); Serial.print(type_name.model_code, HEX); }
            Serial.println();
        } else {
            Serial.println(F("model=unknown"));
        }
        Serial.println(F("connected"));
    }
    return true;
}

void closePlc() {
    plc.close();
    Serial.println(F("closed"));
}

void reinitializeEthernet() {
    plc.close();
    ethernet_ready = false;
    (void)bringUpEthernet();
}

// ---------------------------------------------------------------------------
// LED / board switch
// ---------------------------------------------------------------------------
void applyLedState() {
#ifdef LED_BUILTIN
    if (!led_ready) return;
    bool on = false;
    if (plc.connected()) {
        on = (console_link.transport_mode == TransportMode::Udp)
           ? ((millis() / 250U) % 2U) != 0U
           : true;
    }
    if (plc.isBusy()) on = ((millis() / 100U) % 2U) != 0U;
    digitalWrite(LED_BUILTIN, on ? HIGH : LOW);
#endif
}

bool boardSwitchPressed() { return static_cast<bool>(BOOTSEL); }

void handleBoardSwitchShortPress() {
    if (plc.connected()) closePlc(); else (void)connectPlc(true);
}
void handleBoardSwitchLongPress() {
    if (console_link.transport_mode == TransportMode::Tcp) {
#if SLMP_ENABLE_UDP_TRANSPORT
        console_link.transport_mode = TransportMode::Udp;
#endif
    } else {
        console_link.transport_mode = TransportMode::Tcp;
    }
    Serial.print(F("transport=")); Serial.println(transportModeText(console_link.transport_mode));
    if (plc.connected()) { plc.close(); (void)connectPlc(true); }
}
void handleBoardSwitchVeryLongPress() {
    const bool use3e = (console_link.frame_type == slmp::FrameType::Frame4E);
    console_link.frame_type = use3e ? slmp::FrameType::Frame3E : slmp::FrameType::Frame4E;
    plc.setFrameType(console_link.frame_type);
    Serial.print(F("frame=")); Serial.println(frameTypeText(console_link.frame_type));
}

void pollBoardSwitch() {
    const bool raw = boardSwitchPressed();
    if (raw != switch_raw_pressed) { switch_raw_pressed = raw; switch_last_change_ms = millis(); }
    if (millis() - switch_last_change_ms < kSwitchDebounceMs) return;
    if (switch_stable_pressed == switch_raw_pressed) return;
    switch_stable_pressed = switch_raw_pressed;
    if (switch_stable_pressed) { switch_press_started_ms = millis(); return; }
    const uint32_t held = millis() - switch_press_started_ms;
    if      (held < kSwitchShortPressMs)     handleBoardSwitchShortPress();
    else if (held < kSwitchLongPressMs)      handleBoardSwitchLongPress();
    else if (held < kSwitchLongPressMs * 2U) handleBoardSwitchVeryLongPress();
    switch_press_started_ms = 0;
}

// ---------------------------------------------------------------------------
// Diagnostic printers
// ---------------------------------------------------------------------------
void printLastFrames() {
    char hex[512] = {};
    if (plc.lastRequestFrameLength() == 0) {
        Serial.println(F("request: <none>"));
    } else {
        slmp::formatHexBytes(plc.lastRequestFrame(), plc.lastRequestFrameLength(), hex, sizeof(hex));
        Serial.print(F("request: ")); Serial.println(hex);
    }
    if (plc.lastResponseFrameLength() == 0) {
        Serial.println(F("response: <none>"));
    } else {
        slmp::formatHexBytes(plc.lastResponseFrame(), plc.lastResponseFrameLength(), hex, sizeof(hex));
        Serial.print(F("response: ")); Serial.println(hex);
    }
}

void printLastPlcError(const char* label) {
    Serial.print(label); Serial.print(F(" failed: "));
    Serial.print(slmp::errorString(plc.lastError()));
    Serial.print(F(" end=0x")); Serial.print(plc.lastEndCode(), HEX);
    Serial.print(F(" (")); Serial.print(slmp::endCodeString(plc.lastEndCode())); Serial.println(F(")"));
    printLastFrames();
}

void printTargetAddress() {
    const slmp::TargetAddress& t = plc.target();
    Serial.print(F("target_network="));  Serial.println(t.network);
    Serial.print(F("target_station="));  Serial.println(t.station);
    Serial.print(F("target_module_io=0x")); Serial.println(t.module_io, HEX);
    Serial.print(F("target_multidrop=")); Serial.println(t.multidrop);
}

void printEvidenceHeader() {
    Serial.println(F("--- Evidence Header ---"));
    Serial.println(F("board=W6300-EVB-Pico2 (RP2350)"));
    Serial.print(F("local_ip=")); Serial.println(Ethernet.localIP());
    Serial.print(F("plc_host=")); Serial.println(kPlcHost);
    Serial.print(F("plc_port=")); Serial.println(console_link.plc_port);
    Serial.print(F("transport=")); Serial.println(transportModeText(console_link.transport_mode));
    Serial.print(F("frame="));    Serial.println(frameTypeText(console_link.frame_type));
    Serial.print(F("compat="));   Serial.println(compatibilityModeText(console_link.compatibility_mode));
    if (plc.connected()) {
        slmp::TypeNameInfo ti = {};
        if (plc.readTypeName(ti) == slmp::Error::Ok) {
            Serial.print(F("plc_model=")); Serial.println(ti.model);
        } else {
            Serial.println(F("plc_model=unknown"));
        }
    } else {
        Serial.println(F("plc_model=disconnected"));
    }
    Serial.println(F("-----------------------"));
}

void printPrompt() { Serial.print(F("> ")); }

void printStatus() {
    Serial.print(F("local_ip="));      Serial.println(Ethernet.localIP());
    Serial.print(F("ethernet="));      Serial.println(Ethernet.connected() ? F("up") : F("down"));
    Serial.print(F("transport="));     Serial.println(transportModeText(console_link.transport_mode));
    Serial.print(F("frame="));         Serial.println(frameTypeText(console_link.frame_type));
    Serial.print(F("compat="));        Serial.println(compatibilityModeText(console_link.compatibility_mode));
    Serial.print(F("plc_port="));      Serial.println(console_link.plc_port);
    Serial.print(F("plc_connected=")); Serial.println(plc.connected() ? F("yes") : F("no"));
    Serial.print(F("tx_buffer_bytes="));Serial.println(sizeof(tx_buffer));
    Serial.print(F("rx_buffer_bytes="));Serial.println(sizeof(rx_buffer));
    Serial.print(F("timeout_ms="));    Serial.println(plc.timeoutMs());
    Serial.print(F("monitoring_timer="));Serial.println(plc.monitoringTimer());
    printTargetAddress();
    Serial.print(F("last_error="));    Serial.println(slmp::errorString(plc.lastError()));
    Serial.print(F("last_end_code=0x")); Serial.print(plc.lastEndCode(), HEX);
    Serial.print(F(" (")); Serial.print(slmp::endCodeString(plc.lastEndCode())); Serial.println(F(")"));
    Serial.print(F("watch_active="));  Serial.println(watch_session.active ? F("yes") : F("no"));
    Serial.print(F("endurance_active="));Serial.println(endurance.active ? F("yes") : F("no"));
    Serial.print(F("reconnect_active="));Serial.println(reconnect.active ? F("yes") : F("no"));
}

void printTypeName() {
    if (!connectPlc(false)) { Serial.println(F("type: not connected")); return; }
    slmp::TypeNameInfo ti = {};
    if (plc.readTypeName(ti) != slmp::Error::Ok) { printLastPlcError("type"); return; }
    Serial.print(F("model=")); Serial.print(ti.model);
    if (ti.has_model_code) { Serial.print(F(" code=0x")); Serial.print(ti.model_code, HEX); }
    Serial.println();
}

// ---------------------------------------------------------------------------
// Link setting commands
// ---------------------------------------------------------------------------
void setTransportMode(TransportMode mode, bool reconnect_after) {
    if (console_link.transport_mode == mode) {
        Serial.print(F("transport=")); Serial.println(transportModeText(mode)); return;
    }
    const bool was_connected = plc.connected();
    if (was_connected) plc.close();
    console_link.transport_mode = mode;
    Serial.print(F("transport=")); Serial.println(transportModeText(mode));
    if (reconnect_after && was_connected) (void)connectPlc(true);
}

void setFrameTypeMode(slmp::FrameType ft) {
    console_link.frame_type = ft;
    plc.setFrameType(ft);
    Serial.print(F("frame=")); Serial.println(frameTypeText(ft));
    if (plc.connected()) Serial.println(F("applies to next request"));
}

void setCompatibilityModeMode(slmp::CompatibilityMode cm) {
    console_link.compatibility_mode = cm;
    plc.setCompatibilityMode(cm);
    Serial.print(F("compat=")); Serial.println(compatibilityModeText(cm));
    if (plc.connected()) Serial.println(F("applies to next request"));
}

void transportCommand(char* tokens[], int token_count) {
    if (token_count == 1) { Serial.print(F("transport=")); Serial.println(transportModeText(console_link.transport_mode)); return; }
    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "TCP") == 0) { setTransportMode(TransportMode::Tcp, true); return; }
    if (strcmp(tokens[1], "UDP") == 0) {
#if SLMP_ENABLE_UDP_TRANSPORT
        setTransportMode(TransportMode::Udp, true);
#else
        Serial.println(F("UDP not compiled"));
#endif
        return;
    }
    if (strcmp(tokens[1], "LIST") == 0) { Serial.println(F("tcp | udp")); return; }
    Serial.println(F("transport usage: transport [tcp|udp|list]"));
}

void frameCommand(char* tokens[], int token_count) {
    if (token_count == 1) { Serial.print(F("frame=")); Serial.println(frameTypeText(console_link.frame_type)); return; }
    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "3E") == 0) { setFrameTypeMode(slmp::FrameType::Frame3E); return; }
    if (strcmp(tokens[1], "4E") == 0) { setFrameTypeMode(slmp::FrameType::Frame4E); return; }
    if (strcmp(tokens[1], "LIST") == 0) { Serial.println(F("3e | 4e")); return; }
    Serial.println(F("frame usage: frame [3e|4e|list]"));
}

void compatibilityCommand(char* tokens[], int token_count) {
    if (token_count == 1) { Serial.print(F("compat=")); Serial.println(compatibilityModeText(console_link.compatibility_mode)); return; }
    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "IQR") == 0 || strcmp(tokens[1], "MODERN") == 0) {
        setCompatibilityModeMode(slmp::CompatibilityMode::iQR); return;
    }
    if (strcmp(tokens[1], "LEGACY") == 0 || strcmp(tokens[1], "Q") == 0 || strcmp(tokens[1], "L") == 0) {
        setCompatibilityModeMode(slmp::CompatibilityMode::Legacy); return;
    }
    if (strcmp(tokens[1], "LIST") == 0) {
        Serial.println(F("iqr    - iQ-R/iQ-F (sub 0x0002/0x0003)"));
        Serial.println(F("legacy - Q/L series (sub 0x0000/0x0001)"));
        return;
    }
    Serial.println(F("compat usage: compat [iqr|legacy|list]"));
}

void portCommand(char* tokens[], int token_count) {
    if (token_count == 1) { Serial.print(F("plc_port=")); Serial.println(console_link.plc_port); return; }
    unsigned long v = 0;
    if (!parseUnsignedValue(tokens[1], v, 10) || v == 0 || v > 65535UL) {
        Serial.println(F("port usage: port <1..65535>")); return;
    }
    const bool was = plc.connected();
    if (was) plc.close();
    console_link.plc_port = static_cast<uint16_t>(v);
    Serial.print(F("plc_port=")); Serial.println(console_link.plc_port);
    if (was) (void)connectPlc(true);
}

void targetCommand(char* tokens[], int token_count) {
    if (token_count == 1) { printTargetAddress(); return; }
    if (token_count != 5) { Serial.println(F("target usage: target <network> <station> <module_io> <multidrop>")); return; }
    uint8_t net = 0, sta = 0, mdrop = 0; uint16_t mio = 0;
    if (!parseByteValue(tokens[1], net) || !parseByteValue(tokens[2], sta) ||
        !parseUint16Value(tokens[3], mio) || !parseByteValue(tokens[4], mdrop)) {
        Serial.println(F("target: values out of range")); return;
    }
    slmp::TargetAddress t = plc.target();
    t.network = net; t.station = sta; t.module_io = mio; t.multidrop = mdrop;
    plc.setTarget(t);
    printTargetAddress();
}

void monitorCommand(char* token) {
    if (!token) { Serial.print(F("monitoring_timer=")); Serial.println(plc.monitoringTimer()); return; }
    uint16_t v = 0;
    if (!parseUint16Value(token, v)) { Serial.println(F("monitor usage: monitor [value]")); return; }
    plc.setMonitoringTimer(v);
    Serial.print(F("monitoring_timer=")); Serial.println(plc.monitoringTimer());
}

void setTimeoutCommand(char* token) {
    if (!token) { Serial.print(F("timeout_ms=")); Serial.println(plc.timeoutMs()); return; }
    unsigned long v = 0;
    if (!parseUnsignedValue(token, v, 10)) { Serial.println(F("timeout usage: timeout <ms>")); return; }
    plc.setTimeoutMs(static_cast<uint32_t>(v));
    Serial.print(F("timeout_ms=")); Serial.println(plc.timeoutMs());
}
