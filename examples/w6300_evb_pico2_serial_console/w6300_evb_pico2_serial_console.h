/**
 * @example examples/w6300_evb_pico2_serial_console/w6300_evb_pico2_serial_console.ino
 * @brief SLMP Serial Console for W6300-EVB-Pico2.
 *
 * Commands are split across four sub-headers included at the bottom:
 *   console_hw.h       - Ethernet/connection/link-setting commands
 *   console_watch.h    - watch mode (continuous polling) + loopback
 *   console_commands.h - read/write/verify interactive commands
 *   console_tests.h    - funcheck / bench / endurance / reconnect / txlimit
 *
 * Target Hardware: W6300-EVB-Pico2 (RP2350)
 */
#pragma once

#include <Arduino.h>

#if __has_include(<W6300lwIP.h>)
#include <W6300lwIP.h>
#else
// cppcheck-suppress preprocessorErrorDirective
#error "This example requires Arduino-Pico with W6300lwIP support."
#endif

#if __has_include(<WiFiClient.h>)
#include <WiFiClient.h>
#endif
#if __has_include(<WiFiUdp.h>)
#include <WiFiUdp.h>
#endif

#include <ctype.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <slmp_arduino_transport.h>

namespace w6300_evb_pico2_serial_console {

// ---------------------------------------------------------------------------
// Network / hardware settings
// ---------------------------------------------------------------------------
constexpr uint16_t kPlcPort         = 1025;
const IPAddress    kLocalIp(192, 168, 250, 110);
const IPAddress    kDns(192, 168, 250, 1);
const IPAddress    kGateway(192, 168, 250, 1);
const IPAddress    kSubnet(255, 255, 255, 0);

constexpr uint8_t kEthernetCsPin    = 16;
constexpr uint8_t kEthernetResetPin = 22;

// ---------------------------------------------------------------------------
// Console limits  (memory is plentiful - usability first)
// ---------------------------------------------------------------------------
constexpr size_t kSerialLineCapacity   = 512;
constexpr size_t kMaxTokens            = 64;
constexpr size_t kMaxPoints            = 64;   ///< words / bits / dwords per command
constexpr size_t kMaxRandomWordDevices = 16;
constexpr size_t kMaxRandomDWordDevices= 16;
constexpr size_t kMaxRandomBitDevices  = 16;
constexpr size_t kMaxBlockCount        = 8;
constexpr size_t kMaxBlockPoints       = 64;   ///< total storage across all blocks
constexpr size_t kTxBufferSize         = 2048;
constexpr size_t kRxBufferSize         = 2048;
constexpr size_t kVerificationNoteCapacity = 96;

// ---------------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------------
constexpr uint32_t kEnduranceCycleGapMs        = 20;
constexpr uint32_t kEnduranceReportIntervalMs  = 5000;
constexpr uint32_t kReconnectCycleGapMs        = 250;
constexpr uint32_t kReconnectMaxCycleGapMs     = 5000;
constexpr uint32_t kReconnectReportIntervalMs  = 5000;
constexpr uint32_t kBenchReportIntervalMs      = 3000;
constexpr uint32_t kBenchDefaultCycles         = 1000;
constexpr uint32_t kSwitchDebounceMs           = 30;
constexpr uint32_t kSwitchShortPressMs         = 450;
constexpr uint32_t kSwitchLongPressMs          = 1200;
constexpr uint32_t kWatchDefaultIntervalMs     = 1000;
constexpr uint32_t kWatchMinIntervalMs         = 100;

// ---------------------------------------------------------------------------
// TX capacity calculations
// ---------------------------------------------------------------------------
constexpr size_t kSlmpRequestHeaderSize           = 19;
constexpr size_t kTxPayloadBudget                 =
    kTxBufferSize > kSlmpRequestHeaderSize ? (kTxBufferSize - kSlmpRequestHeaderSize) : 0;
constexpr size_t kTxLimitWriteWordsMaxCount       =
    kTxPayloadBudget >= 8U ? ((kTxPayloadBudget - 8U) / 2U) : 0U;
constexpr size_t kTxLimitWriteDWordsMaxCount      =
    kTxPayloadBudget >= 8U ? ((kTxPayloadBudget - 8U) / 4U) : 0U;
constexpr size_t kTxLimitWriteBitsMaxCount        =
    kTxPayloadBudget >= 8U ? ((kTxPayloadBudget - 8U) * 2U) : 0U;
constexpr size_t kTxLimitWriteRandomBitsMaxCount  =
    kTxPayloadBudget >= 1U ? ((kTxPayloadBudget - 1U) / 8U) : 0U;
constexpr size_t kTxLimitWriteRandomWordsMaxCount =
    kTxPayloadBudget >= 2U ? ((kTxPayloadBudget - 2U) / 8U) : 0U;
constexpr size_t kTxLimitWriteBlockWordMaxPoints  =
    kTxPayloadBudget >= 10U ? ((kTxPayloadBudget - 10U) / 2U) : 0U;
static_assert(kTxLimitWriteWordsMaxCount > 0,       "tx word max must be > 0");
static_assert(kTxLimitWriteBlockWordMaxPoints > 0,  "tx block max must be > 0");

// ---------------------------------------------------------------------------
// Bench / funcheck / endurance / txlimit fixed device addresses
// ---------------------------------------------------------------------------
constexpr size_t kBenchWordPoints  = 8;
constexpr size_t kBenchBlockPoints = 16;

constexpr slmp::DeviceAddress kBenchOneWordDevice   = {slmp::DeviceCode::D, 800};
constexpr slmp::DeviceAddress kBenchWordArrayDevice = {slmp::DeviceCode::D, 820};
constexpr slmp::DeviceAddress kBenchBlockWordDevice = {slmp::DeviceCode::D, 900};

constexpr slmp::DeviceAddress kFuncheckOneWordDevice    = {slmp::DeviceCode::D, 120};
constexpr slmp::DeviceAddress kFuncheckWordArrayDevice  = {slmp::DeviceCode::D, 130};
constexpr slmp::DeviceAddress kFuncheckOneBitDevice     = {slmp::DeviceCode::M, 120};
constexpr slmp::DeviceAddress kFuncheckBitArrayDevice   = {slmp::DeviceCode::M, 130};
constexpr slmp::DeviceAddress kFuncheckOneDWordDevice   = {slmp::DeviceCode::D, 220};
constexpr slmp::DeviceAddress kFuncheckDWordArrayDevice = {slmp::DeviceCode::D, 230};
constexpr slmp::DeviceAddress kFuncheckRandomWordDevices[]  = {{slmp::DeviceCode::D, 140}, {slmp::DeviceCode::D, 141}};
constexpr slmp::DeviceAddress kFuncheckRandomDWordDevices[] = {{slmp::DeviceCode::D, 240}};
constexpr slmp::DeviceAddress kFuncheckRandomBitDevices[]   = {{slmp::DeviceCode::M, 140}, {slmp::DeviceCode::M, 141}};
constexpr slmp::DeviceAddress kFuncheckBlockWordDevice  = {slmp::DeviceCode::D, 300};
constexpr slmp::DeviceAddress kFuncheckBlockBitDevice   = {slmp::DeviceCode::M, 200};

constexpr slmp::DeviceAddress kEnduranceOneWordDevice    = {slmp::DeviceCode::D, 500};
constexpr slmp::DeviceAddress kEnduranceWordArrayDevice  = {slmp::DeviceCode::D, 510};
constexpr slmp::DeviceAddress kEnduranceOneBitDevice     = {slmp::DeviceCode::M, 500};
constexpr slmp::DeviceAddress kEnduranceBitArrayDevice   = {slmp::DeviceCode::M, 510};
constexpr slmp::DeviceAddress kEnduranceOneDWordDevice   = {slmp::DeviceCode::D, 600};
constexpr slmp::DeviceAddress kEnduranceDWordArrayDevice = {slmp::DeviceCode::D, 610};
constexpr slmp::DeviceAddress kEnduranceRandomWordDevices[]  = {{slmp::DeviceCode::D, 520}, {slmp::DeviceCode::D, 521}};
constexpr slmp::DeviceAddress kEnduranceRandomDWordDevices[] = {{slmp::DeviceCode::D, 620}};
constexpr slmp::DeviceAddress kEnduranceRandomBitDevices[]   = {{slmp::DeviceCode::M, 520}, {slmp::DeviceCode::M, 521}};
constexpr slmp::DeviceAddress kEnduranceBlockWordDevice  = {slmp::DeviceCode::D, 540};
constexpr slmp::DeviceAddress kEnduranceBlockBitDevice   = {slmp::DeviceCode::M, 540};

constexpr slmp::DeviceAddress kTxLimitWordDevice      = {slmp::DeviceCode::D,  700};
constexpr slmp::DeviceAddress kTxLimitBlockWordDevice = {slmp::DeviceCode::D, 1100};

// ---------------------------------------------------------------------------
// Device spec table
// ---------------------------------------------------------------------------
struct DeviceSpec {
    const char*      name;
    slmp::DeviceCode code;
    bool             hex_address;
};

const DeviceSpec kDeviceSpecs[] = {
    {"SM",   slmp::DeviceCode::SM,   false},
    {"SD",   slmp::DeviceCode::SD,   false},
    {"X",    slmp::DeviceCode::X,    true},
    {"Y",    slmp::DeviceCode::Y,    true},
    {"M",    slmp::DeviceCode::M,    false},
    {"L",    slmp::DeviceCode::L,    false},
    {"F",    slmp::DeviceCode::F,    false},
    {"V",    slmp::DeviceCode::V,    false},
    {"B",    slmp::DeviceCode::B,    true},
    {"D",    slmp::DeviceCode::D,    false},
    {"W",    slmp::DeviceCode::W,    true},
    {"LSTS", slmp::DeviceCode::LSTS, false},
    {"LSTC", slmp::DeviceCode::LSTC, false},
    {"LSTN", slmp::DeviceCode::LSTN, false},
    {"LTS",  slmp::DeviceCode::LTS,  false},
    {"LTC",  slmp::DeviceCode::LTC,  false},
    {"LTN",  slmp::DeviceCode::LTN,  false},
    {"LCS",  slmp::DeviceCode::LCS,  false},
    {"LCC",  slmp::DeviceCode::LCC,  false},
    {"LCN",  slmp::DeviceCode::LCN,  false},
    {"LZ",   slmp::DeviceCode::LZ,   false},
    {"STS",  slmp::DeviceCode::STS,  false},
    {"STC",  slmp::DeviceCode::STC,  false},
    {"STN",  slmp::DeviceCode::STN,  false},
    {"SB",   slmp::DeviceCode::SB,   true},
    {"SW",   slmp::DeviceCode::SW,   true},
    {"TS",   slmp::DeviceCode::TS,   false},
    {"TC",   slmp::DeviceCode::TC,   false},
    {"TN",   slmp::DeviceCode::TN,   false},
    {"CS",   slmp::DeviceCode::CS,   false},
    {"CC",   slmp::DeviceCode::CC,   false},
    {"CN",   slmp::DeviceCode::CN,   false},
    {"DX",   slmp::DeviceCode::DX,   true},
    {"DY",   slmp::DeviceCode::DY,   true},
    {"ZR",   slmp::DeviceCode::ZR,   false},
    {"RD",   slmp::DeviceCode::RD,   false},
    {"HG",   slmp::DeviceCode::HG,   false},
    {"Z",    slmp::DeviceCode::Z,    false},
    {"R",    slmp::DeviceCode::R,    false},
    {"G",    slmp::DeviceCode::G,    false},
};

// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------
enum class VerificationKind : uint8_t { None = 0, WordWrite, BitWrite };
enum class BenchMode        : uint8_t { None = 0, Row, Wow, Pair, Rw, Ww, Block };
enum class WatchType        : uint8_t { Word = 0, Bit, DWord };
enum class TransportMode    : uint8_t { Tcp = 0, Udp };
enum class FuncheckResult   : uint8_t { Ok = 0, Fail };

// ---------------------------------------------------------------------------
// Structs
// ---------------------------------------------------------------------------
struct BenchSummary {
    uint32_t cycles          = 0;
    uint32_t requests        = 0;
    uint32_t fail            = 0;
    uint32_t last_cycle_ms   = 0;
    uint32_t min_cycle_ms    = 0;
    uint32_t max_cycle_ms    = 0;
    uint64_t total_cycle_ms  = 0;
    size_t   last_request_bytes  = 0;
    size_t   last_response_bytes = 0;
};

struct FuncheckSummary {
    uint32_t ok   = 0;
    uint32_t fail = 0;
};

struct VerificationRecord {
    bool             active         = false;
    bool             judged         = false;
    bool             pass           = false;
    bool             readback_match = false;
    VerificationKind kind           = VerificationKind::None;
    slmp::DeviceAddress device      = {slmp::DeviceCode::D, 0};
    uint16_t         points         = 0;
    uint16_t         before_words[kMaxPoints]   = {};
    uint16_t         written_words[kMaxPoints]  = {};
    uint16_t         readback_words[kMaxPoints] = {};
    bool             before_bit     = false;
    bool             written_bit    = false;
    bool             readback_bit   = false;
    uint32_t         started_ms     = 0;
    char             note[kVerificationNoteCapacity] = {};
};

struct EnduranceSession {
    bool         active            = false;
    uint32_t     started_ms        = 0;
    uint32_t     next_cycle_due_ms = 0;
    uint32_t     last_report_ms    = 0;
    uint32_t     last_cycle_ms     = 0;
    uint32_t     min_cycle_ms      = 0;
    uint32_t     max_cycle_ms      = 0;
    uint64_t     total_cycle_ms    = 0;
    uint32_t     cycle_limit       = 0;
    uint32_t     attempts          = 0;
    uint32_t     ok                = 0;
    uint32_t     fail              = 0;
    char         last_step[48]     = {};
    char         last_issue[64]    = {};
    slmp::Error  last_error        = slmp::Error::Ok;
    uint16_t     last_end_code     = 0;
};

struct ReconnectSession {
    bool         active                    = false;
    uint32_t     started_ms               = 0;
    uint32_t     next_cycle_due_ms        = 0;
    uint32_t     last_report_ms           = 0;
    uint32_t     last_cycle_ms            = 0;
    uint32_t     min_cycle_ms             = 0;
    uint32_t     max_cycle_ms             = 0;
    uint64_t     total_cycle_ms           = 0;
    uint32_t     cycle_limit              = 0;
    uint32_t     attempts                 = 0;
    uint32_t     ok                       = 0;
    uint32_t     fail                     = 0;
    uint32_t     recoveries               = 0;
    uint32_t     consecutive_failures     = 0;
    uint32_t     max_consecutive_failures = 0;
    char         last_step[48]            = {};
    char         last_issue[64]           = {};
    slmp::Error  last_error               = slmp::Error::Ok;
    uint16_t     last_end_code            = 0;
};

struct WatchSession {
    bool             active      = false;
    WatchType        type        = WatchType::Word;
    slmp::DeviceAddress device   = {slmp::DeviceCode::D, 0};
    uint16_t         points      = 1;
    uint32_t         interval_ms = kWatchDefaultIntervalMs;
    uint32_t         next_poll_ms= 0;
    bool             has_prev    = false;
    uint16_t         prev_words[kMaxPoints]  = {};
    uint32_t         prev_dwords[kMaxPoints] = {};
    bool             prev_bits[kMaxPoints]   = {};
};

struct ConsoleLinkState {
    TransportMode          transport_mode    = TransportMode::Tcp;
    slmp::FrameType        frame_type        = slmp::FrameType::Frame4E;
    slmp::CompatibilityMode compatibility_mode = slmp::CompatibilityMode::iQR;
    uint16_t               plc_port          = kPlcPort;
};

// ---------------------------------------------------------------------------
// Hardware globals
// ---------------------------------------------------------------------------
Wiznet6300lwIP Ethernet(kEthernetCsPin);
WiFiClient tcp_client;
slmp::ArduinoClientTransport tcp_transport(tcp_client);
#if SLMP_ENABLE_UDP_TRANSPORT
WiFiUDP udp_client;
slmp::ArduinoUdpTransport udp_transport(udp_client);
#endif
ConsoleLinkState console_link = {};

class ConsoleTransportRouter : public slmp::ITransport {
  public:
    ConsoleTransportRouter(
        slmp::ArduinoClientTransport& tcp,
#if SLMP_ENABLE_UDP_TRANSPORT
        slmp::ArduinoUdpTransport& udp,
#endif
        ConsoleLinkState& link)
        : tcp_(tcp)
#if SLMP_ENABLE_UDP_TRANSPORT
        , udp_(udp)
#endif
        , link_(link) {}

    bool connect(const char* host, uint16_t port) override { return active().connect(host, port); }
    void close() override {
        tcp_.close();
#if SLMP_ENABLE_UDP_TRANSPORT
        udp_.close();
#endif
    }
    bool   connected()  const override { return active().connected(); }
    bool   writeAll(const uint8_t* d, size_t n) override { return active().writeAll(d, n); }
    bool   readExact(uint8_t* d, size_t n, uint32_t t) override { return active().readExact(d, n, t); }
    size_t write(const uint8_t* d, size_t n) override { return active().write(d, n); }
    size_t read(uint8_t* d, size_t n) override { return active().read(d, n); }
    size_t available() override { return active().available(); }

  private:
    slmp::ITransport& active() {
#if SLMP_ENABLE_UDP_TRANSPORT
        if (link_.transport_mode == TransportMode::Udp) return udp_;
#endif
        return tcp_;
    }
    const slmp::ITransport& active() const {
#if SLMP_ENABLE_UDP_TRANSPORT
        if (link_.transport_mode == TransportMode::Udp) return udp_;
#endif
        return tcp_;
    }
    slmp::ArduinoClientTransport& tcp_;
#if SLMP_ENABLE_UDP_TRANSPORT
    slmp::ArduinoUdpTransport& udp_;
#endif
    ConsoleLinkState& link_;
};

uint8_t tx_buffer[kTxBufferSize];
uint8_t rx_buffer[kRxBufferSize];
ConsoleTransportRouter transport_router(
    tcp_transport
#if SLMP_ENABLE_UDP_TRANSPORT
    , udp_transport
#endif
    , console_link
);
slmp::SlmpClient plc(transport_router,
    tx_buffer, sizeof(tx_buffer),
    rx_buffer, sizeof(rx_buffer));

// ---------------------------------------------------------------------------
// Runtime state globals
// ---------------------------------------------------------------------------
char   serial_line[kSerialLineCapacity] = {};
size_t serial_line_length               = 0;
char   plc_host[64]                     = "192.168.250.100";
char   cached_plc_model[32]             = "disconnected";
bool   ethernet_ready                   = false;
bool   led_ready                        = false;
bool   switch_raw_pressed               = false;
bool   switch_stable_pressed            = false;
uint32_t switch_last_change_ms          = 0;
uint32_t switch_press_started_ms        = 0;

VerificationRecord verification  = {};
EnduranceSession   endurance     = {};
ReconnectSession   reconnect     = {};
WatchSession       watch_session = {};
uint32_t           funcheck_prng_state = 0x13579BDFU;

// ---------------------------------------------------------------------------
// Large test buffers
// ---------------------------------------------------------------------------
uint16_t txlimit_word_values[kTxLimitWriteWordsMaxCount + 1U]        = {};
uint16_t txlimit_word_readback[kTxLimitWriteWordsMaxCount + 1U]      = {};
uint16_t txlimit_block_values[kTxLimitWriteBlockWordMaxPoints + 1U]  = {};
uint16_t txlimit_block_readback[kTxLimitWriteBlockWordMaxPoints + 1U]= {};
uint16_t bench_word_values[kBenchWordPoints]   = {};
uint16_t bench_word_readback[kBenchWordPoints] = {};
uint16_t bench_block_values[kBenchBlockPoints]  = {};
uint16_t bench_block_readback[kBenchBlockPoints]= {};

// ---------------------------------------------------------------------------
// Forward declarations (sub-files need to call across each other)
// ---------------------------------------------------------------------------
void stopEndurance(bool print_summary, bool failed);
void stopReconnect(bool print_summary);
void stopWatch();

// ---------------------------------------------------------------------------
// Sub-module implementations (order matters: hw → watch → commands → tests)
// ---------------------------------------------------------------------------
#include "console_hw.h"
#include "console_watch.h"
#include "console_commands.h"
#include "console_tests.h"
#include "console_tui.h"

// ---------------------------------------------------------------------------
// Help
// ---------------------------------------------------------------------------
void printHelp() {
    Serial.println(F("=== SLMP W6300 Demo Console ==="));
    Serial.println(F("[Demo TUI]"));
    Serial.println(F("  home | view <home|connect|read|write|watch|batch|stress|system|log>"));
    Serial.println(F("  1..9 menu shortcuts    refresh    ansi [on|off]"));
    Serial.println(F("  host <ip-or-name>"));
    Serial.println(F("  qr/qrb/qrd <dev> [pts]"));
    Serial.println(F("  qw/qwb/qwd <dev> <val...>"));
    Serial.println(F("  wa/wab/wad <dev> [pts]    wdel <index>    wclear"));
    Serial.println(F("  batch/batchb/batchd <dev> [pts]    brun"));
    Serial.println(F("  stressw/stressb/stressd <dev> [pts] [ms]"));
    Serial.println(F("  stress [start|stop|status]"));
    Serial.println(F("[Connection]"));
    Serial.println(F("  connect | close | reinit | status | type | dump"));
    Serial.println(F("  transport [tcp|udp]    frame [3e|4e]    compat [iqr|legacy]"));
    Serial.println(F("  port <n>    target [net sta io mdrop]    monitor [val]    timeout <ms>"));
    Serial.println(F("[Read]"));
    Serial.println(F("  r/rw  <dev> [pts]           words (1..64)"));
    Serial.println(F("  rd/rdw <dev> [pts]           dwords"));
    Serial.println(F("  b/rbits <dev> [pts]          bits"));
    Serial.println(F("  row <dev>  rod <dev>          single word / dword"));
    Serial.println(F("  rr <n> <devs..> <m> <devs..> random read"));
    Serial.println(F("[Write]"));
    Serial.println(F("  w/ww  <dev> <val..>          words"));
    Serial.println(F("  wd/wdw <dev> <val..>          dwords"));
    Serial.println(F("  wb/wbits <dev> <val..>        bits (0/1)"));
    Serial.println(F("  wow <dev> <val>  wod <dev> <val>  single word / dword"));
    Serial.println(F("  wrand <n> <dev val..> <m> <dev val..>"));
    Serial.println(F("  wrandb <n> <dev val..>"));
    Serial.println(F("  rblk <wb> <dev pts..> <bb> <dev pts..>"));
    Serial.println(F("  wblk <wb> <dev pts vals..> <bb> <dev pts packed..>"));
    Serial.println(F("  unlock/lock <password>"));
    Serial.println(F("[Watch]"));
    Serial.println(F("  watch <dev> [pts] [ms]       watch words (default 1pt/1000ms)"));
    Serial.println(F("  watch b <dev> [pts] [ms]     watch bits"));
    Serial.println(F("  watch d <dev> [pts] [ms]     watch dwords"));
    Serial.println(F("  watch off | watch status"));
    Serial.println(F("[Verification]"));
    Serial.println(F("  verifyw/vw <dev> <val..>     write + readback record"));
    Serial.println(F("  verifyb/vb <dev> <0|1>"));
    Serial.println(F("  judge <ok|ng> [note]    pending"));
    Serial.println(F("[Tests]"));
    Serial.println(F("  funcheck [all|direct|api|list]"));
    Serial.println(F("  fullscan / scan    loopback [text]"));
    Serial.println(F("  bench [row|wow|pair|rw|ww|block] [cycles]"));
    Serial.println(F("  endurance [cycles|status|stop]"));
    Serial.println(F("  reconnect [cycles|status|stop]"));
    Serial.println(F("  txlimit [calc|probe|sweep]"));
    Serial.println(F("hex-address devices: X Y B W SB SW DX DY"));
}

// ---------------------------------------------------------------------------
// Command dispatch
// ---------------------------------------------------------------------------
void handleCommand(char* line) {
    char* tokens[kMaxTokens] = {};
    const int token_count = splitTokens(line, tokens, static_cast<int>(kMaxTokens));
    if (token_count == 0) {
        if (!demo_ui.ansi_enabled) printPrompt();
        return;
    }

    uppercaseInPlace(tokens[0]);
    const char* cmd = tokens[0];

    if (handleDemoCommand(tokens, token_count)) {
        if (!demo_ui.ansi_enabled) printPrompt();
        return;
    }

    // --- Connection ---
    if (strcmp(cmd, "HELP") == 0 || strcmp(cmd, "?") == 0) {
        printHelp();
    } else if (strcmp(cmd, "STATUS") == 0) {
        printStatus();
    } else if (strcmp(cmd, "CONNECT") == 0) {
        (void)connectPlc(true);
    } else if (strcmp(cmd, "CLOSE") == 0) {
        closePlc();
    } else if (strcmp(cmd, "REINIT") == 0) {
        reinitializeEthernet();
    } else if (strcmp(cmd, "TYPE") == 0) {
        printTypeName();
    } else if (strcmp(cmd, "DUMP") == 0) {
        printLastFrames();
    } else if (strcmp(cmd, "TRANSPORT") == 0) {
        transportCommand(tokens, token_count);
    } else if (strcmp(cmd, "FRAME") == 0) {
        frameCommand(tokens, token_count);
    } else if (strcmp(cmd, "COMPAT") == 0 || strcmp(cmd, "COMPATIBILITY") == 0) {
        compatibilityCommand(tokens, token_count);
    } else if (strcmp(cmd, "PORT") == 0) {
        portCommand(tokens, token_count);
    } else if (strcmp(cmd, "TARGET") == 0) {
        targetCommand(tokens, token_count);
    } else if (strcmp(cmd, "MONITOR") == 0 || strcmp(cmd, "MON") == 0) {
        monitorCommand(token_count > 1 ? tokens[1] : nullptr);
    } else if (strcmp(cmd, "TIMEOUT") == 0) {
        setTimeoutCommand(token_count > 1 ? tokens[1] : nullptr);

    // --- Read words ---
    } else if (strcmp(cmd, "R") == 0 || strcmp(cmd, "RW") == 0) {
        readWordsCommand(token_count > 1 ? tokens[1] : nullptr,
                         token_count > 2 ? tokens[2] : nullptr);
    } else if (strcmp(cmd, "ROW") == 0) {
        readOneWordCommand(token_count > 1 ? tokens[1] : nullptr);

    // --- Write words ---
    } else if (strcmp(cmd, "W") == 0 || strcmp(cmd, "WW") == 0) {
        writeWordsCommand(tokens, token_count);
    } else if (strcmp(cmd, "WOW") == 0) {
        writeOneWordCommand(token_count > 1 ? tokens[1] : nullptr,
                            token_count > 2 ? tokens[2] : nullptr);

    // --- Read bits ---
    } else if (strcmp(cmd, "B") == 0 || strcmp(cmd, "RBITS") == 0 || strcmp(cmd, "RBS") == 0) {
        readBitsCommand(token_count > 1 ? tokens[1] : nullptr,
                        token_count > 2 ? tokens[2] : nullptr);
    } else if (strcmp(cmd, "RB") == 0) {
        readOneBitCommand(token_count > 1 ? tokens[1] : nullptr);

    // --- Write bits ---
    } else if (strcmp(cmd, "WB") == 0 || strcmp(cmd, "WBITS") == 0 || strcmp(cmd, "WBS") == 0) {
        writeBitsCommand(tokens, token_count);

    // --- Read dwords ---
    } else if (strcmp(cmd, "RD") == 0 || strcmp(cmd, "RDW") == 0) {
        readDWordsCommand(token_count > 1 ? tokens[1] : nullptr,
                          token_count > 2 ? tokens[2] : nullptr);
    } else if (strcmp(cmd, "ROD") == 0) {
        readOneDWordCommand(token_count > 1 ? tokens[1] : nullptr);

    // --- Write dwords ---
    } else if (strcmp(cmd, "WD") == 0 || strcmp(cmd, "WDW") == 0) {
        writeDWordsCommand(tokens, token_count);
    } else if (strcmp(cmd, "WOD") == 0) {
        writeOneDWordCommand(token_count > 1 ? tokens[1] : nullptr,
                             token_count > 2 ? tokens[2] : nullptr);

    // --- Random / block ---
    } else if (strcmp(cmd, "RR") == 0) {
        readRandomCommand(tokens, token_count);
    } else if (strcmp(cmd, "WRAND") == 0) {
        writeRandomWordsCommand(tokens, token_count);
    } else if (strcmp(cmd, "WRANDB") == 0) {
        writeRandomBitsCommand(tokens, token_count);
    } else if (strcmp(cmd, "RBLK") == 0) {
        readBlockCommand(tokens, token_count);
    } else if (strcmp(cmd, "WBLK") == 0) {
        writeBlockCommand(tokens, token_count);

    // --- Password ---
    } else if (strcmp(cmd, "UNLOCK") == 0) {
        remotePasswordUnlockCommand(token_count > 1 ? tokens[1] : nullptr);
    } else if (strcmp(cmd, "LOCK") == 0) {
        remotePasswordLockCommand(token_count > 1 ? tokens[1] : nullptr);

    // --- Verification ---
    } else if (strcmp(cmd, "VERIFYW") == 0 || strcmp(cmd, "VW") == 0) {
        verifyWordsCommand(tokens, token_count);
    } else if (strcmp(cmd, "VERIFYB") == 0 || strcmp(cmd, "VB") == 0) {
        verifyBitCommand(token_count > 1 ? tokens[1] : nullptr,
                         token_count > 2 ? tokens[2] : nullptr);
    } else if (strcmp(cmd, "PENDING") == 0) {
        printVerificationSummary();
    } else if (strcmp(cmd, "JUDGE") == 0) {
        judgeCommand(tokens, token_count);

    // --- Watch ---
    } else if (strcmp(cmd, "WATCH") == 0) {
        watchCommand(tokens, token_count);

    // --- Tests ---
    } else if (strcmp(cmd, "LOOPBACK") == 0 || strcmp(cmd, "SELFTEST") == 0) {
        loopbackCommand(tokens, token_count);
    } else if (strcmp(cmd, "FUNCHECK") == 0) {
        funcheckCommand(tokens, token_count);
    } else if (strcmp(cmd, "FULLSCAN") == 0 || strcmp(cmd, "SCAN") == 0) {
        fullscanCommand(tokens, token_count);
    } else if (strcmp(cmd, "BENCH") == 0 || strcmp(cmd, "PERF") == 0) {
        stopEndurance(false, false);
        stopReconnect(false);
        stopWatch();
        benchCommand(tokens, token_count);
    } else if (strcmp(cmd, "ENDURANCE") == 0 || strcmp(cmd, "SOAK") == 0) {
        enduranceCommand(tokens, token_count);
    } else if (strcmp(cmd, "RECONNECT") == 0 || strcmp(cmd, "RETRY") == 0) {
        reconnectCommand(tokens, token_count);
    } else if (strcmp(cmd, "TXLIMIT") == 0 || strcmp(cmd, "TXBUF") == 0) {
        txlimitCommand(tokens, token_count);
    } else {
        Serial.print(F("unknown: "));
        Serial.println(tokens[0]);
        printHelp();
    }

    if (!demo_ui.ansi_enabled) printPrompt();
}

// ---------------------------------------------------------------------------
// Arduino setup / loop
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    Serial.println(F("SLMP W6300-EVB-Pico2 demo console"));
    plc.setFrameType(console_link.frame_type);
    plc.setTimeoutMs(2000);

#ifdef LED_BUILTIN
    pinMode(LED_BUILTIN, OUTPUT);
    led_ready = true;
#endif

    (void)bringUpEthernet();
    initializeDemoTui();
    printHelp();
    (void)connectPlc(false);
    refreshDemoDashboardData(true);
    requestDemoRender();
    if (!demo_ui.ansi_enabled) printPrompt();
}

void loop() {
    const uint32_t loop_started_us = micros();
    pollBoardSwitch();
    applyLedState();
    pollEnduranceTest();
    pollReconnectTest();
    pollWatch();
    pollDemoWatchList();
    pollDemoBatch();
    pollDemoStress();
    pollDemoSystemMetrics();

    while (Serial.available() > 0) {
        const int raw = Serial.read();
        if (raw < 0) break;
        const char ch = static_cast<char>(raw);
        if (ch == '\r') continue;
        if (ch == '\n') {
            serial_line[serial_line_length] = '\0';
            handleCommand(serial_line);
            serial_line_length = 0;
            serial_line[0] = '\0';
            requestDemoRender();
            continue;
        }
        if (serial_line_length + 1 >= sizeof(serial_line)) {
            Serial.println();
            Serial.println(F("line too long"));
            serial_line_length = 0;
            serial_line[0] = '\0';
            if (!demo_ui.ansi_enabled) printPrompt();
            continue;
        }
        serial_line[serial_line_length++] = ch;
    }

    noteDemoLoopSample(micros() - loop_started_us);
    renderDemoScreenIfNeeded();
}

}  // namespace w6300_evb_pico2_serial_console
