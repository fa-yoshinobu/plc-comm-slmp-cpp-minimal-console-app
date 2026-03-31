/**
 * @file atom_matrix_serial_console.h
 * @brief Minimal SLMP serial console for M5Stack Atom Matrix.
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <slmp_arduino_transport.h>

#include "config.h"

namespace atom_matrix_serial_console {

constexpr size_t kSerialLineCapacity = 192;
constexpr size_t kMaxTokens = 32;
constexpr size_t kMaxWordPoints = 8;

struct DeviceSpec {
    const char* name;
    slmp::DeviceCode code;
    bool hex_address;
};

const DeviceSpec kDeviceSpecs[] = {
    {"SM", slmp::DeviceCode::SM, false},
    {"SD", slmp::DeviceCode::SD, false},
    {"X", slmp::DeviceCode::X, true},
    {"Y", slmp::DeviceCode::Y, true},
    {"M", slmp::DeviceCode::M, false},
    {"L", slmp::DeviceCode::L, false},
    {"F", slmp::DeviceCode::F, false},
    {"V", slmp::DeviceCode::V, false},
    {"B", slmp::DeviceCode::B, true},
    {"D", slmp::DeviceCode::D, false},
    {"W", slmp::DeviceCode::W, true},
    {"TS", slmp::DeviceCode::TS, false},
    {"TC", slmp::DeviceCode::TC, false},
    {"TN", slmp::DeviceCode::TN, false},
    {"LTS", slmp::DeviceCode::LTS, false},
    {"LTC", slmp::DeviceCode::LTC, false},
    {"LTN", slmp::DeviceCode::LTN, false},
    {"STS", slmp::DeviceCode::STS, false},
    {"STC", slmp::DeviceCode::STC, false},
    {"STN", slmp::DeviceCode::STN, false},
    {"CS", slmp::DeviceCode::CS, false},
    {"CC", slmp::DeviceCode::CC, false},
    {"CN", slmp::DeviceCode::CN, false},
    {"SB", slmp::DeviceCode::SB, true},
    {"SW", slmp::DeviceCode::SW, true},
    {"DX", slmp::DeviceCode::DX, true},
    {"DY", slmp::DeviceCode::DY, true},
    {"Z", slmp::DeviceCode::Z, false},
    {"LZ", slmp::DeviceCode::LZ, false},
    {"R", slmp::DeviceCode::R, false},
    {"ZR", slmp::DeviceCode::ZR, false},
    {"RD", slmp::DeviceCode::RD, false},
    {"G", slmp::DeviceCode::G, false},
    {"HG", slmp::DeviceCode::HG, false},
};

WiFiClient tcp_client;
slmp::ArduinoClientTransport transport(tcp_client);
uint8_t tx_buffer[example_config::kTxBufferSize];
uint8_t rx_buffer[example_config::kRxBufferSize];
slmp::SlmpClient plc(transport, tx_buffer, sizeof(tx_buffer), rx_buffer, sizeof(rx_buffer));

char serial_line[kSerialLineCapacity] = {};
size_t serial_line_length = 0;
bool serial_last_was_cr = false;
bool wifi_ready = false;
char plc_host[64] = {};
uint16_t plc_port = example_config::kPlcPort;

const DeviceSpec* findDeviceSpecByName(const char* token) {
    const DeviceSpec* match = nullptr;
    size_t match_length = 0;
    for (size_t i = 0; i < sizeof(kDeviceSpecs) / sizeof(kDeviceSpecs[0]); ++i) {
        const size_t name_length = strlen(kDeviceSpecs[i].name);
        if (name_length < match_length) continue;
        if (strncmp(token, kDeviceSpecs[i].name, name_length) == 0) {
            match = &kDeviceSpecs[i];
            match_length = name_length;
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

void copyText(char* out, size_t capacity, const char* text) {
    if (!out || capacity == 0) return;
    size_t i = 0;
    if (text) {
        for (; i + 1 < capacity && text[i] != '\0'; ++i) out[i] = text[i];
    }
    out[i] = '\0';
}

void uppercaseInPlace(char* text) {
    if (!text) return;
    for (; *text != '\0'; ++text) {
        *text = static_cast<char>(toupper(static_cast<unsigned char>(*text)));
    }
}

bool parseUnsignedValue(const char* token, unsigned long& value, int base) {
    if (!token || *token == '\0') return false;
    char* end = nullptr;
    value = strtoul(token, &end, base);
    return end != token && *end == '\0';
}

bool parseWordValue(const char* token, uint16_t& value) {
    unsigned long parsed = 0;
    if (!parseUnsignedValue(token, parsed, 0) || parsed > 0xFFFFUL) return false;
    value = static_cast<uint16_t>(parsed);
    return true;
}

bool parseDWordValue(const char* token, uint32_t& value) {
    unsigned long parsed = 0;
    if (!parseUnsignedValue(token, parsed, 0)) return false;
    value = static_cast<uint32_t>(parsed);
    return true;
}

bool parseByteValue(const char* token, uint8_t& value) {
    unsigned long parsed = 0;
    if (!parseUnsignedValue(token, parsed, 0) || parsed > 0xFFUL) return false;
    value = static_cast<uint8_t>(parsed);
    return true;
}

bool parseUint16Value(const char* token, uint16_t& value) {
    unsigned long parsed = 0;
    if (!parseUnsignedValue(token, parsed, 0) || parsed > 0xFFFFUL) return false;
    value = static_cast<uint16_t>(parsed);
    return true;
}

bool parsePointCount(const char* token, uint16_t& points) {
    unsigned long parsed = 0;
    if (!parseUnsignedValue(token, parsed, 10) || parsed == 0 || parsed > kMaxWordPoints) return false;
    points = static_cast<uint16_t>(parsed);
    return true;
}

bool parseBoolValue(char* token, bool& value) {
    if (!token) return false;
    uppercaseInPlace(token);
    if (strcmp(token, "1") == 0 || strcmp(token, "ON") == 0 || strcmp(token, "TRUE") == 0) {
        value = true;
        return true;
    }
    if (strcmp(token, "0") == 0 || strcmp(token, "OFF") == 0 || strcmp(token, "FALSE") == 0) {
        value = false;
        return true;
    }
    return false;
}

bool parseDeviceAddress(char* token, slmp::DeviceAddress& device) {
    if (!token || *token == '\0') return false;
    uppercaseInPlace(token);
    const DeviceSpec* spec = findDeviceSpecByName(token);
    if (!spec) return false;
    const size_t prefix_length = strlen(spec->name);
    unsigned long number = 0;
    if (!parseUnsignedValue(token + prefix_length, number, spec->hex_address ? 16 : 10)) return false;
    device.code = spec->code;
    device.number = static_cast<uint32_t>(number);
    return true;
}

void printDeviceAddress(const slmp::DeviceAddress& device, uint32_t offset = 0) {
    const DeviceSpec* spec = findDeviceSpecByCode(device.code);
    if (!spec) {
        Serial.print('?');
        Serial.print(device.number + offset);
        return;
    }
    Serial.print(spec->name);
    if (spec->hex_address) Serial.print(device.number + offset, HEX);
    else Serial.print(device.number + offset);
}

int splitTokens(char* line, char* tokens[], int token_capacity) {
    int count = 0;
    char* cursor = line;
    while (*cursor != '\0' && count < token_capacity) {
        while (*cursor != '\0' && isspace(static_cast<unsigned char>(*cursor)) != 0) ++cursor;
        if (*cursor == '\0') break;
        tokens[count++] = cursor;
        while (*cursor != '\0' && isspace(static_cast<unsigned char>(*cursor)) == 0) ++cursor;
        if (*cursor == '\0') break;
        *cursor++ = '\0';
    }
    return count;
}

void printPrompt() {
    Serial.print(F("> "));
}

void printLastError(const char* label) {
    Serial.print(label);
    Serial.print(F(" failed: "));
    Serial.print(slmp::errorString(plc.lastError()));
    Serial.print(F(" end=0x"));
    Serial.print(plc.lastEndCode(), HEX);
    Serial.print(F(" ("));
    Serial.print(slmp::endCodeString(plc.lastEndCode()));
    Serial.println(F(")"));
}

bool wifiConfigLooksUnset() {
    return strcmp(example_config::kWifiSsid, "YOUR_WIFI_SSID") == 0 || strcmp(example_config::kWifiPassword, "YOUR_WIFI_PASSWORD") == 0;
}

bool bringUpWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        wifi_ready = true;
        return true;
    }
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    if (wifiConfigLooksUnset()) {
        Serial.println(F("wifi config looks unset; edit config.h"));
    }
    WiFi.begin(example_config::kWifiSsid, example_config::kWifiPassword);
    const uint32_t started_ms = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - started_ms >= example_config::kWifiConnectTimeoutMs) {
            wifi_ready = false;
            Serial.println(F("wifi connect timeout"));
            return false;
        }
        delay(120);
    }
    wifi_ready = true;
    Serial.print(F("wifi_ip="));
    Serial.println(WiFi.localIP());
    return true;
}

bool connectPlc(bool verbose) {
    if (!bringUpWiFi()) return false;
    if (plc.connected()) {
        if (verbose) Serial.println(F("already connected"));
        return true;
    }
    if (!plc.connect(plc_host, plc_port)) {
        if (verbose) printLastError("connect");
        return false;
    }
    if (verbose) {
        Serial.print(F("connected host="));
        Serial.print(plc_host);
        Serial.print(F(" port="));
        Serial.println(plc_port);
    }
    return true;
}

void printTypeName() {
    if (!connectPlc(false)) {
        Serial.println(F("type: not connected"));
        return;
    }
    slmp::TypeNameInfo info = {};
    if (plc.readTypeName(info) != slmp::Error::Ok) {
        printLastError("type");
        return;
    }
    Serial.print(F("model="));
    Serial.print(info.model);
    if (info.has_model_code) {
        Serial.print(F(" code=0x"));
        Serial.print(info.model_code, HEX);
    }
    Serial.println();
}

void printStatus() {
    Serial.print(F("wifi="));
    Serial.println(WiFi.status() == WL_CONNECTED ? F("connected") : F("disconnected"));
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print(F("wifi_ip="));
        Serial.println(WiFi.localIP());
    }
    Serial.print(F("plc_host="));
    Serial.println(plc_host);
    Serial.print(F("plc_port="));
    Serial.println(plc_port);
    Serial.print(F("plc_connected="));
    Serial.println(plc.connected() ? F("yes") : F("no"));
    Serial.print(F("timeout_ms="));
    Serial.println(plc.timeoutMs());
    Serial.print(F("monitoring_timer="));
    Serial.println(plc.monitoringTimer());
    const slmp::TargetAddress& target = plc.target();
    Serial.print(F("target="));
    Serial.print(target.network);
    Serial.print(F("/"));
    Serial.print(target.station);
    Serial.print(F("/0x"));
    Serial.print(target.module_io, HEX);
    Serial.print(F("/"));
    Serial.println(target.multidrop);
}

void printHelp() {
    Serial.println(F("=== SLMP Atom Minimal Console ==="));
    Serial.println(F("connect | close | status | type"));
    Serial.println(F("host <ip> | port <n> | timeout <ms> | monitor [value]"));
    Serial.println(F("target <network> <station> <module_io> <multidrop>"));
    Serial.println(F("row <device> | rw <device> [points] | rb <device>"));
    Serial.println(F("rod <device> | rdw <device> [points]"));
    Serial.println(F("wow <device> <value> | ww <device> <values...>"));
    Serial.println(F("wb <device> <0|1...> | wod <device> <value> | wdw <device> <values...>"));
    Serial.println(F("help"));
}

void readWordsCommand(char* device_token, char* points_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) {
        Serial.println(F("rw usage: rw <device> [points]"));
        return;
    }
    uint16_t points = 1;
    if (points_token && !parsePointCount(points_token, points)) {
        Serial.print(F("rw points must be 1.."));
        Serial.println(kMaxWordPoints);
        return;
    }
    if (!connectPlc(false)) {
        Serial.println(F("rw: not connected"));
        return;
    }
    uint16_t values[kMaxWordPoints] = {};
    if (plc.readWords(device, points, values, kMaxWordPoints) != slmp::Error::Ok) {
        printLastError("readWords");
        return;
    }
    for (uint16_t i = 0; i < points; ++i) {
        printDeviceAddress(device, i);
        Serial.print(F("="));
        Serial.print(values[i]);
        Serial.print(F(" (0x"));
        Serial.print(values[i], HEX);
        Serial.println(F(")"));
    }
}

void readOneWordCommand(char* device_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) {
        Serial.println(F("row usage: row <device>"));
        return;
    }
    if (!connectPlc(false)) {
        Serial.println(F("row: not connected"));
        return;
    }
    uint16_t value = 0;
    if (plc.readOneWord(device, value) != slmp::Error::Ok) {
        printLastError("readOneWord");
        return;
    }
    printDeviceAddress(device);
    Serial.print(F("="));
    Serial.println(value);
}

void readOneBitCommand(char* device_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) {
        Serial.println(F("rb usage: rb <device>"));
        return;
    }
    if (!connectPlc(false)) {
        Serial.println(F("rb: not connected"));
        return;
    }
    bool value = false;
    if (plc.readOneBit(device, value) != slmp::Error::Ok) {
        printLastError("readOneBit");
        return;
    }
    printDeviceAddress(device);
    Serial.print(F("="));
    Serial.println(value ? 1 : 0);
}

void readDWordsCommand(char* device_token, char* points_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) {
        Serial.println(F("rdw usage: rdw <device> [points]"));
        return;
    }
    uint16_t points = 1;
    if (points_token && !parsePointCount(points_token, points)) {
        Serial.print(F("rdw points must be 1.."));
        Serial.println(kMaxWordPoints);
        return;
    }
    if (!connectPlc(false)) {
        Serial.println(F("rdw: not connected"));
        return;
    }
    uint32_t values[kMaxWordPoints] = {};
    if (plc.readDWords(device, points, values, kMaxWordPoints) != slmp::Error::Ok) {
        printLastError("readDWords");
        return;
    }
    for (uint16_t i = 0; i < points; ++i) {
        printDeviceAddress(device, i * 2U);
        Serial.print(F("="));
        Serial.println(static_cast<unsigned long>(values[i]));
    }
}

void writeWordsCommand(char* tokens[], int token_count) {
    if (token_count < 3) {
        Serial.println(F("ww usage: ww <device> <value...>"));
        return;
    }
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(tokens[1], device)) {
        Serial.println(F("ww: device parse failed"));
        return;
    }
    const int value_count = token_count - 2;
    if (value_count <= 0 || value_count > static_cast<int>(kMaxWordPoints)) {
        Serial.print(F("ww values must be 1.."));
        Serial.println(kMaxWordPoints);
        return;
    }
    uint16_t values[kMaxWordPoints] = {};
    for (int i = 0; i < value_count; ++i) {
        if (!parseWordValue(tokens[i + 2], values[i])) {
            Serial.println(F("ww values must fit in 16 bits"));
            return;
        }
    }
    if (!connectPlc(false)) {
        Serial.println(F("ww: not connected"));
        return;
    }
    const slmp::Error err = (value_count == 1) ? plc.writeOneWord(device, values[0]) : plc.writeWords(device, values, static_cast<size_t>(value_count));
    if (err != slmp::Error::Ok) {
        printLastError("writeWords");
        return;
    }
    Serial.println(F("ok"));
}

void writeBitsCommand(char* tokens[], int token_count) {
    if (token_count < 3) {
        Serial.println(F("wb usage: wb <device> <0|1...>"));
        return;
    }
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(tokens[1], device)) {
        Serial.println(F("wb: device parse failed"));
        return;
    }
    const int value_count = token_count - 2;
    if (value_count <= 0 || value_count > static_cast<int>(kMaxWordPoints)) {
        Serial.print(F("wb values must be 1.."));
        Serial.println(kMaxWordPoints);
        return;
    }
    bool values[kMaxWordPoints] = {};
    for (int i = 0; i < value_count; ++i) {
        if (!parseBoolValue(tokens[i + 2], values[i])) {
            Serial.println(F("wb values must be 0/1, on/off, true/false"));
            return;
        }
    }
    if (!connectPlc(false)) {
        Serial.println(F("wb: not connected"));
        return;
    }
    const slmp::Error err = (value_count == 1) ? plc.writeOneBit(device, values[0]) : plc.writeBits(device, values, static_cast<size_t>(value_count));
    if (err != slmp::Error::Ok) {
        printLastError("writeBits");
        return;
    }
    Serial.println(F("ok"));
}

void writeDWordsCommand(char* tokens[], int token_count) {
    if (token_count < 3) {
        Serial.println(F("wdw usage: wdw <device> <value...>"));
        return;
    }
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(tokens[1], device)) {
        Serial.println(F("wdw: device parse failed"));
        return;
    }
    const int value_count = token_count - 2;
    if (value_count <= 0 || value_count > static_cast<int>(kMaxWordPoints)) {
        Serial.print(F("wdw values must be 1.."));
        Serial.println(kMaxWordPoints);
        return;
    }
    uint32_t values[kMaxWordPoints] = {};
    for (int i = 0; i < value_count; ++i) {
        if (!parseDWordValue(tokens[i + 2], values[i])) {
            Serial.println(F("wdw values must fit in 32 bits"));
            return;
        }
    }
    if (!connectPlc(false)) {
        Serial.println(F("wdw: not connected"));
        return;
    }
    const slmp::Error err = (value_count == 1) ? plc.writeOneDWord(device, values[0]) : plc.writeDWords(device, values, static_cast<size_t>(value_count));
    if (err != slmp::Error::Ok) {
        printLastError("writeDWords");
        return;
    }
    Serial.println(F("ok"));
}

void setTimeoutCommand(char* token) {
    unsigned long timeout_ms = 0;
    if (!parseUnsignedValue(token, timeout_ms, 10)) {
        Serial.println(F("timeout usage: timeout <ms>"));
        return;
    }
    plc.setTimeoutMs(static_cast<uint32_t>(timeout_ms));
    Serial.print(F("timeout_ms="));
    Serial.println(plc.timeoutMs());
}

void hostCommand(char* token) {
    if (!token || *token == '\0') {
        Serial.print(F("host="));
        Serial.println(plc_host);
        return;
    }
    copyText(plc_host, sizeof(plc_host), token);
    Serial.print(F("host="));
    Serial.println(plc_host);
}

void portCommand(char* token) {
    unsigned long parsed = 0;
    if (!parseUnsignedValue(token, parsed, 10) || parsed == 0 || parsed > 65535UL) {
        Serial.println(F("port usage: port <1..65535>"));
        return;
    }
    plc_port = static_cast<uint16_t>(parsed);
    Serial.print(F("port="));
    Serial.println(plc_port);
}

void targetCommand(char* tokens[], int token_count) {
    if (token_count != 5) {
        Serial.println(F("target usage: target <network> <station> <module_io> <multidrop>"));
        return;
    }
    uint8_t network = 0;
    uint8_t station = 0;
    uint8_t multidrop = 0;
    uint16_t module_io = 0;
    if (!parseByteValue(tokens[1], network) || !parseByteValue(tokens[2], station) || !parseUint16Value(tokens[3], module_io) || !parseByteValue(tokens[4], multidrop)) {
        Serial.println(F("target values out of range"));
        return;
    }
    slmp::TargetAddress target = plc.target();
    target.network = network;
    target.station = station;
    target.module_io = module_io;
    target.multidrop = multidrop;
    plc.setTarget(target);
    printStatus();
}

void monitorCommand(char* token) {
    if (!token) {
        Serial.print(F("monitoring_timer="));
        Serial.println(plc.monitoringTimer());
        return;
    }
    uint16_t value = 0;
    if (!parseUint16Value(token, value)) {
        Serial.println(F("monitor usage: monitor [value]"));
        return;
    }
    plc.setMonitoringTimer(value);
    Serial.print(F("monitoring_timer="));
    Serial.println(plc.monitoringTimer());
}

void handleCommand(char* line) {
    char* tokens[kMaxTokens] = {};
    const int token_count = splitTokens(line, tokens, static_cast<int>(kMaxTokens));
    if (token_count == 0) {
        printPrompt();
        return;
    }
    uppercaseInPlace(tokens[0]);
    const char* cmd = tokens[0];

    if (strcmp(cmd, "HELP") == 0 || strcmp(cmd, "?") == 0) {
        printHelp();
    } else if (strcmp(cmd, "STATUS") == 0) {
        printStatus();
    } else if (strcmp(cmd, "CONNECT") == 0) {
        (void)connectPlc(true);
    } else if (strcmp(cmd, "CLOSE") == 0) {
        plc.close();
        Serial.println(F("closed"));
    } else if (strcmp(cmd, "TYPE") == 0) {
        printTypeName();
    } else if (strcmp(cmd, "HOST") == 0) {
        hostCommand(token_count > 1 ? tokens[1] : nullptr);
    } else if (strcmp(cmd, "PORT") == 0) {
        portCommand(token_count > 1 ? tokens[1] : nullptr);
    } else if (strcmp(cmd, "TIMEOUT") == 0) {
        setTimeoutCommand(token_count > 1 ? tokens[1] : nullptr);
    } else if (strcmp(cmd, "TARGET") == 0) {
        targetCommand(tokens, token_count);
    } else if (strcmp(cmd, "MONITOR") == 0 || strcmp(cmd, "MON") == 0) {
        monitorCommand(token_count > 1 ? tokens[1] : nullptr);
    } else if (strcmp(cmd, "RW") == 0 || strcmp(cmd, "R") == 0) {
        readWordsCommand(token_count > 1 ? tokens[1] : nullptr, token_count > 2 ? tokens[2] : nullptr);
    } else if (strcmp(cmd, "ROW") == 0) {
        readOneWordCommand(token_count > 1 ? tokens[1] : nullptr);
    } else if (strcmp(cmd, "RB") == 0) {
        readOneBitCommand(token_count > 1 ? tokens[1] : nullptr);
    } else if (strcmp(cmd, "RDW") == 0 || strcmp(cmd, "ROD") == 0) {
        readDWordsCommand(token_count > 1 ? tokens[1] : nullptr, token_count > 2 ? tokens[2] : nullptr);
    } else if (strcmp(cmd, "WW") == 0 || strcmp(cmd, "WOW") == 0) {
        writeWordsCommand(tokens, token_count);
    } else if (strcmp(cmd, "WB") == 0) {
        writeBitsCommand(tokens, token_count);
    } else if (strcmp(cmd, "WDW") == 0 || strcmp(cmd, "WOD") == 0) {
        writeDWordsCommand(tokens, token_count);
    } else {
        Serial.print(F("unknown: "));
        Serial.println(tokens[0]);
        printHelp();
    }
    printPrompt();
}

void setupConsole() {
    Serial.begin(115200);
    const uint32_t started_ms = millis();
    while (!Serial && millis() - started_ms < 500U) {
        delay(10);
    }
    copyText(plc_host, sizeof(plc_host), example_config::kPlcHost);
    Serial.println(F("SLMP Atom Matrix minimal console"));
    plc.setFrameType(slmp::FrameType::Frame4E);
    plc.setCompatibilityMode(slmp::CompatibilityMode::iQR);
    plc.setTimeoutMs(2000);
    (void)bringUpWiFi();
    printHelp();
    if (connectPlc(false)) {
        printTypeName();
        readWordsCommand(const_cast<char*>("D100"), const_cast<char*>("1"));
    }
    printPrompt();
}

void setupButtonIncrement() {
    Serial.println(F("atom mode=minimal"));
}

void submitSerialLine() {
    serial_line[serial_line_length] = '\0';
    handleCommand(serial_line);
    serial_line_length = 0;
    serial_line[0] = '\0';
}

void loop() {
    while (Serial.available() > 0) {
        const int raw = Serial.read();
        if (raw < 0) break;
        const char ch = static_cast<char>(raw);
        if (ch == '\r' || ch == '\n') {
            if (ch == '\n' && serial_last_was_cr) {
                serial_last_was_cr = false;
                continue;
            }
            serial_last_was_cr = (ch == '\r');
            Serial.println();
            submitSerialLine();
            continue;
        }
        serial_last_was_cr = false;
        if (ch == '\b' || ch == 127) {
            if (serial_line_length > 0) {
                --serial_line_length;
                serial_line[serial_line_length] = '\0';
                Serial.print("\b \b");
            }
            continue;
        }
        if (!isprint(static_cast<unsigned char>(ch)) && ch != '\t') continue;
        if (serial_line_length + 1 >= sizeof(serial_line)) {
            Serial.println();
            Serial.println(F("command too long"));
            serial_line_length = 0;
            serial_line[0] = '\0';
            printPrompt();
            continue;
        }
        Serial.print(ch);
        serial_line[serial_line_length++] = ch;
    }
}

void pollButtonIncrement() {}
void pollDemoDisplay() {}
void pollEnduranceTest() {}
void pollReconnectTest() {}
void pollBenchmark() {}

}  // namespace atom_matrix_serial_console
