/**
 * @file console_tests.h
 * @brief Test commands: funcheck, bench, endurance, reconnect, txlimit, fullscan.
 *
 * Included by w6300_evb_pico2_serial_console.h inside its namespace after console_commands.h.
 * Depends on helpers defined in console_hw.h and console_commands.h.
 *
 * Defines the forward-declared functions:
 *   void stopEndurance(bool print_summary, bool failed);
 *   void stopReconnect(bool print_summary);
 */

// ---------------------------------------------------------------------------
// PRNG for funcheck (xorshift32)
// ---------------------------------------------------------------------------
uint32_t nextFuncheckRandom() {
    funcheck_prng_state ^= funcheck_prng_state << 13;
    funcheck_prng_state ^= funcheck_prng_state >> 17;
    funcheck_prng_state ^= funcheck_prng_state << 5;
    return funcheck_prng_state;
}
uint16_t nextFuncheckWordValue()  { return static_cast<uint16_t>(nextFuncheckRandom()); }
uint32_t nextFuncheckDWordValue() { return nextFuncheckRandom(); }

// ---------------------------------------------------------------------------
// Funcheck: print one case result
// ---------------------------------------------------------------------------
void funcheckReport(FuncheckSummary& s, const __FlashStringHelper* label, bool pass) {
    Serial.print(F("  "));
    Serial.print(label);
    Serial.println(pass ? F(" ... OK") : F(" ... FAIL"));
    if (pass) ++s.ok; else ++s.fail;
}

// ---------------------------------------------------------------------------
// Funcheck: direct suite - exercises primitive word/bit/dword API
// ---------------------------------------------------------------------------
FuncheckSummary runFuncheckDirectSuite() {
    FuncheckSummary s = {};

    // readOneWord / writeOneWord
    {
        const uint16_t val = nextFuncheckWordValue();
        uint16_t rb = 0;
        bool ok = (plc.writeOneWord(kFuncheckOneWordDevice, val) == slmp::Error::Ok) &&
                  (plc.readOneWord(kFuncheckOneWordDevice, rb)   == slmp::Error::Ok) &&
                  (rb == val);
        funcheckReport(s, F("readOneWord/writeOneWord"), ok);
        clearWordsSilently(kFuncheckOneWordDevice, 1);
    }

    // readWords / writeWords (4 words)
    {
        uint16_t wvals[4] = {nextFuncheckWordValue(), nextFuncheckWordValue(),
                             nextFuncheckWordValue(), nextFuncheckWordValue()};
        uint16_t rb[4] = {};
        bool ok = (plc.writeWords(kFuncheckWordArrayDevice, wvals, 4) == slmp::Error::Ok) &&
                  (plc.readWords(kFuncheckWordArrayDevice, 4, rb, 4)  == slmp::Error::Ok) &&
                  wordArraysEqual(wvals, rb, 4);
        funcheckReport(s, F("readWords/writeWords"), ok);
        clearWordsSilently(kFuncheckWordArrayDevice, 4);
    }

    // readOneBit / writeOneBit
    {
        const bool val = (nextFuncheckRandom() & 1U) != 0U;
        bool rb = false;
        bool ok = (plc.writeOneBit(kFuncheckOneBitDevice, val) == slmp::Error::Ok) &&
                  (plc.readOneBit(kFuncheckOneBitDevice, rb)   == slmp::Error::Ok) &&
                  (rb == val);
        funcheckReport(s, F("readOneBit/writeOneBit"), ok);
        clearBitsSilently(kFuncheckOneBitDevice, 1);
    }

    // readBits / writeBits (4 bits)
    {
        bool bvals[4] = {};
        for (int i = 0; i < 4; ++i) bvals[i] = (nextFuncheckRandom() & 1U) != 0U;
        bool rb[4] = {};
        bool ok = (plc.writeBits(kFuncheckBitArrayDevice, bvals, 4) == slmp::Error::Ok) &&
                  (plc.readBits(kFuncheckBitArrayDevice, 4, rb, 4)  == slmp::Error::Ok) &&
                  bitArraysEqual(bvals, rb, 4);
        funcheckReport(s, F("readBits/writeBits"), ok);
        clearBitsSilently(kFuncheckBitArrayDevice, 4);
    }

    // readOneDWord / writeOneDWord
    {
        const uint32_t val = nextFuncheckDWordValue();
        uint32_t rb = 0;
        bool ok = (plc.writeOneDWord(kFuncheckOneDWordDevice, val) == slmp::Error::Ok) &&
                  (plc.readOneDWord(kFuncheckOneDWordDevice, rb)   == slmp::Error::Ok) &&
                  (rb == val);
        funcheckReport(s, F("readOneDWord/writeOneDWord"), ok);
        clearDWordsSilently(kFuncheckOneDWordDevice, 1);
    }

    // readDWords / writeDWords (2 dwords)
    {
        uint32_t dvals[2] = {nextFuncheckDWordValue(), nextFuncheckDWordValue()};
        uint32_t rb[2] = {};
        bool ok = (plc.writeDWords(kFuncheckDWordArrayDevice, dvals, 2) == slmp::Error::Ok) &&
                  (plc.readDWords(kFuncheckDWordArrayDevice, 2, rb, 2)  == slmp::Error::Ok) &&
                  dwordArraysEqual(dvals, rb, 2);
        funcheckReport(s, F("readDWords/writeDWords"), ok);
        clearDWordsSilently(kFuncheckDWordArrayDevice, 2);
    }

    return s;
}

// ---------------------------------------------------------------------------
// Funcheck: API suite - exercises random read/write and block read/write
// ---------------------------------------------------------------------------
FuncheckSummary runFuncheckApiSuite() {
    FuncheckSummary s = {};

    // writeRandomWords / readRandom (word + dword)
    {
        constexpr size_t nw = 2, nd = 1;
        uint16_t wvals[nw] = {nextFuncheckWordValue(), nextFuncheckWordValue()};
        uint32_t dvals[nd] = {nextFuncheckDWordValue()};
        bool wok = (plc.writeRandomWords(
                        kFuncheckRandomWordDevices,  wvals, nw,
                        kFuncheckRandomDWordDevices, dvals, nd) == slmp::Error::Ok);
        uint16_t rb_w[nw] = {};
        uint32_t rb_d[nd] = {};
        bool rok = (plc.readRandom(
                        kFuncheckRandomWordDevices,  nw, rb_w, nw,
                        kFuncheckRandomDWordDevices, nd, rb_d, nd) == slmp::Error::Ok);
        bool ok = wok && rok &&
                  wordArraysEqual(wvals, rb_w, nw) &&
                  dwordArraysEqual(dvals, rb_d, nd);
        funcheckReport(s, F("writeRandomWords/readRandom"), ok);
        clearRandomWordsSilently(kFuncheckRandomWordDevices, nw, kFuncheckRandomDWordDevices, nd);
    }

    // writeRandomBits / readOneBit (verify individual bits)
    {
        constexpr size_t nb = 2;
        bool bvals[nb] = {(nextFuncheckRandom() & 1U) != 0U, (nextFuncheckRandom() & 1U) != 0U};
        bool wok = (plc.writeRandomBits(kFuncheckRandomBitDevices, bvals, nb) == slmp::Error::Ok);
        bool rb0 = false, rb1 = false;
        bool rok = (plc.readOneBit(kFuncheckRandomBitDevices[0], rb0) == slmp::Error::Ok) &&
                   (plc.readOneBit(kFuncheckRandomBitDevices[1], rb1) == slmp::Error::Ok);
        bool ok = wok && rok && (rb0 == bvals[0]) && (rb1 == bvals[1]);
        funcheckReport(s, F("writeRandomBits/readOneBit"), ok);
        clearRandomBitsSilently(kFuncheckRandomBitDevices, nb);
    }

    // writeBlock / readBlock (word block, 4 points)
    {
        constexpr uint16_t blk_pts = 4;
        uint16_t wvals[blk_pts] = {nextFuncheckWordValue(), nextFuncheckWordValue(),
                                   nextFuncheckWordValue(), nextFuncheckWordValue()};
        const slmp::DeviceBlockWrite blk_w = {kFuncheckBlockWordDevice, wvals, blk_pts};
        bool wok = (plc.writeBlock(&blk_w, 1, nullptr, 0) == slmp::Error::Ok);
        uint16_t rb[blk_pts] = {};
        const slmp::DeviceBlockRead blk_r = {kFuncheckBlockWordDevice, blk_pts};
        bool rok = (plc.readBlock(&blk_r, 1, nullptr, 0, rb, blk_pts, nullptr, 0) == slmp::Error::Ok);
        bool ok = wok && rok && wordArraysEqual(wvals, rb, blk_pts);
        funcheckReport(s, F("writeBlock/readBlock(word)"), ok);
        clearWordsSilently(kFuncheckBlockWordDevice, blk_pts);
    }

    // writeBlock / readBlock (bit block, 1 packed word = 16 bits)
    {
        constexpr uint16_t blk_pts = 1;
        uint16_t packed = nextFuncheckWordValue();
        const slmp::DeviceBlockWrite blk_b = {kFuncheckBlockBitDevice, &packed, blk_pts};
        bool wok = (plc.writeBlock(nullptr, 0, &blk_b, 1) == slmp::Error::Ok);
        uint16_t rb_packed = 0;
        const slmp::DeviceBlockRead blk_r = {kFuncheckBlockBitDevice, blk_pts};
        bool rok = (plc.readBlock(nullptr, 0, &blk_r, 1, nullptr, 0, &rb_packed, blk_pts) == slmp::Error::Ok);
        bool ok = wok && rok && (rb_packed == packed);
        funcheckReport(s, F("writeBlock/readBlock(bit)"), ok);
        clearPackedBitWordsSilently(kFuncheckBlockBitDevice, blk_pts);
    }

    return s;
}

// ---------------------------------------------------------------------------
// Funcheck: run selected suites and print combined result
// ---------------------------------------------------------------------------
void runFuncheckAll(bool direct, bool api) {
    if (!connectPlc(true)) return;
    Serial.println(F("--- funcheck start ---"));
    FuncheckSummary total = {};
    if (direct) {
        Serial.println(F("[direct suite]"));
        const FuncheckSummary r = runFuncheckDirectSuite();
        total.ok += r.ok; total.fail += r.fail;
    }
    if (api) {
        Serial.println(F("[api suite]"));
        const FuncheckSummary r = runFuncheckApiSuite();
        total.ok += r.ok; total.fail += r.fail;
    }
    Serial.print(F("funcheck result: "));
    Serial.print(total.ok); Serial.print(F(" ok, "));
    Serial.print(total.fail);
    Serial.println(total.fail == 0 ? F(" fail => PASS") : F(" fail => FAIL"));
}

// ---------------------------------------------------------------------------
// funcheck command
//   funcheck [all|direct|api|list]
// ---------------------------------------------------------------------------
void funcheckCommand(char* tokens[], int token_count) {
    if (token_count < 2) { runFuncheckAll(true, true); return; }

    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "LIST") == 0) {
        Serial.println(F("funcheck suites: all (default), direct, api"));
        return;
    }
    const bool do_direct = strcmp(tokens[1], "DIRECT") == 0 || strcmp(tokens[1], "ALL") == 0;
    const bool do_api    = strcmp(tokens[1], "API")    == 0 || strcmp(tokens[1], "ALL") == 0;
    if (!do_direct && !do_api) {
        Serial.println(F("funcheck: unknown suite. use: all, direct, api, list"));
        return;
    }
    runFuncheckAll(do_direct, do_api);
}

// ---------------------------------------------------------------------------
// Bench helpers
// ---------------------------------------------------------------------------
BenchMode parseBenchModeToken(const char* tok) {
    if (!tok) return BenchMode::None;
    char buf[8] = {};
    copyText(buf, sizeof(buf), tok);
    for (char* p = buf; *p; ++p) *p = static_cast<char>(toupper(static_cast<unsigned char>(*p)));
    if (strcmp(buf, "ROW")   == 0) return BenchMode::Row;
    if (strcmp(buf, "WOW")   == 0) return BenchMode::Wow;
    if (strcmp(buf, "PAIR")  == 0) return BenchMode::Pair;
    if (strcmp(buf, "RW")    == 0) return BenchMode::Rw;
    if (strcmp(buf, "WW")    == 0) return BenchMode::Ww;
    if (strcmp(buf, "BLOCK") == 0) return BenchMode::Block;
    return BenchMode::None;
}

void printBenchSummary(const BenchSummary& bs, BenchMode mode) {
    const char* mode_str = "?";
    switch (mode) {
        case BenchMode::Row:   mode_str = "row";   break;
        case BenchMode::Wow:   mode_str = "wow";   break;
        case BenchMode::Pair:  mode_str = "pair";  break;
        case BenchMode::Rw:    mode_str = "rw";    break;
        case BenchMode::Ww:    mode_str = "ww";    break;
        case BenchMode::Block: mode_str = "block"; break;
        default: break;
    }
    Serial.print(F("bench mode=")); Serial.print(mode_str);
    Serial.print(F(" cycles="));   Serial.print(bs.cycles);
    Serial.print(F(" req="));      Serial.print(bs.requests);
    Serial.print(F(" fail="));     Serial.print(bs.fail);
    Serial.print(F(" last_ms="));  Serial.print(bs.last_cycle_ms);
    Serial.print(F(" min_ms="));   Serial.print(bs.min_cycle_ms);
    Serial.print(F(" max_ms="));   Serial.print(bs.max_cycle_ms);
    if (bs.cycles > 0) {
        Serial.print(F(" avg_ms="));
        Serial.print(static_cast<uint32_t>(bs.total_cycle_ms / static_cast<uint64_t>(bs.cycles)));
    }
    Serial.print(F(" tx="));  Serial.print(static_cast<unsigned long>(bs.last_request_bytes));
    Serial.print(F(" rx="));  Serial.println(static_cast<unsigned long>(bs.last_response_bytes));
}

void runBench(BenchMode mode, uint32_t max_cycles) {
    if (!connectPlc(true)) return;

    BenchSummary bs = {};
    uint32_t next_report_ms = millis() + kBenchReportIntervalMs;

    for (size_t i = 0; i < kBenchWordPoints;  ++i) bench_word_values[i]  = static_cast<uint16_t>(i + 1U);
    for (size_t i = 0; i < kBenchBlockPoints; ++i) bench_block_values[i] = static_cast<uint16_t>(i + 1U);

    Serial.print(F("bench mode="));
    switch (mode) {
        case BenchMode::Row:   Serial.print(F("row"));   break;
        case BenchMode::Wow:   Serial.print(F("wow"));   break;
        case BenchMode::Pair:  Serial.print(F("pair"));  break;
        case BenchMode::Rw:    Serial.print(F("rw"));    break;
        case BenchMode::Ww:    Serial.print(F("ww"));    break;
        case BenchMode::Block: Serial.print(F("block")); break;
        default: break;
    }
    Serial.print(F(" cycles=")); Serial.println(max_cycles);

    while (bs.cycles < max_cycles) {
        if (!connectPlc(false)) { ++bs.fail; delay(100); continue; }

        const uint32_t t0 = millis();

        switch (mode) {
            case BenchMode::Row: {
                uint16_t v = 0;
                const slmp::Error e = plc.readOneWord(kBenchOneWordDevice, v);
                ++bs.requests;
                if (e != slmp::Error::Ok) ++bs.fail;
                bs.last_request_bytes  = plc.lastRequestFrameLength();
                bs.last_response_bytes = plc.lastResponseFrameLength();
                break;
            }
            case BenchMode::Wow: {
                const slmp::Error e = plc.writeOneWord(kBenchOneWordDevice,
                                                        static_cast<uint16_t>(bs.cycles));
                ++bs.requests;
                if (e != slmp::Error::Ok) ++bs.fail;
                bs.last_request_bytes  = plc.lastRequestFrameLength();
                bs.last_response_bytes = plc.lastResponseFrameLength();
                break;
            }
            case BenchMode::Pair: {
                slmp::Error e = plc.writeOneWord(kBenchOneWordDevice,
                                                  static_cast<uint16_t>(bs.cycles));
                ++bs.requests;
                if (e == slmp::Error::Ok) {
                    uint16_t rv = 0;
                    e = plc.readOneWord(kBenchOneWordDevice, rv);
                    ++bs.requests;
                }
                if (e != slmp::Error::Ok) ++bs.fail;
                bs.last_request_bytes  = plc.lastRequestFrameLength();
                bs.last_response_bytes = plc.lastResponseFrameLength();
                break;
            }
            case BenchMode::Rw: {
                const slmp::Error e = plc.readWords(kBenchWordArrayDevice, kBenchWordPoints,
                                                    bench_word_readback, kBenchWordPoints);
                ++bs.requests;
                if (e != slmp::Error::Ok) ++bs.fail;
                bs.last_request_bytes  = plc.lastRequestFrameLength();
                bs.last_response_bytes = plc.lastResponseFrameLength();
                break;
            }
            case BenchMode::Ww: {
                bench_word_values[0] = static_cast<uint16_t>(bs.cycles);
                const slmp::Error e = plc.writeWords(kBenchWordArrayDevice,
                                                      bench_word_values, kBenchWordPoints);
                ++bs.requests;
                if (e != slmp::Error::Ok) ++bs.fail;
                bs.last_request_bytes  = plc.lastRequestFrameLength();
                bs.last_response_bytes = plc.lastResponseFrameLength();
                break;
            }
            case BenchMode::Block: {
                const slmp::DeviceBlockRead blk_r = {kBenchBlockWordDevice,
                                                      static_cast<uint16_t>(kBenchBlockPoints)};
                slmp::Error e = plc.readBlock(&blk_r, 1, nullptr, 0,
                                               bench_block_readback, kBenchBlockPoints, nullptr, 0);
                ++bs.requests;
                if (e == slmp::Error::Ok) {
                    bench_block_values[0] = static_cast<uint16_t>(bs.cycles);
                    const slmp::DeviceBlockWrite blk_w = {kBenchBlockWordDevice,
                                                           bench_block_values,
                                                           static_cast<uint16_t>(kBenchBlockPoints)};
                    e = plc.writeBlock(nullptr, 0, &blk_w, 1);
                    ++bs.requests;
                }
                if (e != slmp::Error::Ok) ++bs.fail;
                bs.last_request_bytes  = plc.lastRequestFrameLength();
                bs.last_response_bytes = plc.lastResponseFrameLength();
                break;
            }
            default: break;
        }

        ++bs.cycles;
        const uint32_t elapsed = millis() - t0;
        bs.last_cycle_ms  = elapsed;
        bs.total_cycle_ms += elapsed;
        if (bs.cycles == 1 || elapsed < bs.min_cycle_ms) bs.min_cycle_ms = elapsed;
        if (elapsed > bs.max_cycle_ms) bs.max_cycle_ms = elapsed;

        if (millis() >= next_report_ms) {
            printBenchSummary(bs, mode);
            next_report_ms = millis() + kBenchReportIntervalMs;
        }
    }

    printBenchSummary(bs, mode);
    Serial.println(F("bench done"));
}

// ---------------------------------------------------------------------------
// bench command
//   bench [row|wow|pair|rw|ww|block] [cycles]
// ---------------------------------------------------------------------------
void benchCommand(char* tokens[], int token_count) {
    BenchMode mode    = BenchMode::Pair;
    uint32_t  cycles  = kBenchDefaultCycles;

    if (token_count >= 2) {
        const BenchMode m = parseBenchModeToken(tokens[1]);
        if (m != BenchMode::None) {
            mode = m;
        } else {
            unsigned long v = 0;
            if (parseUnsignedValue(tokens[1], v, 10)) {
                cycles = static_cast<uint32_t>(v);
            } else {
                Serial.println(F("bench modes: row wow pair rw ww block"));
                return;
            }
        }
    }
    if (token_count >= 3) {
        unsigned long v = 0;
        if (parseUnsignedValue(tokens[2], v, 10)) cycles = static_cast<uint32_t>(v);
    }

    runBench(mode, cycles);
}

// ---------------------------------------------------------------------------
// Endurance: one cycle (writeOneWord/Bit/DWord → readback + compare)
// ---------------------------------------------------------------------------
bool runEnduranceCycle() {
    const uint16_t wv = static_cast<uint16_t>(endurance.attempts ^ 0xA5A5U);
    const bool     bv = (endurance.attempts & 1U) != 0U;
    const uint32_t dv = static_cast<uint32_t>(endurance.attempts) * 0x10001UL;

    // Word
    strncpy(endurance.last_step, "writeOneWord", sizeof(endurance.last_step) - 1);
    if (plc.writeOneWord(kEnduranceOneWordDevice, wv) != slmp::Error::Ok) {
        snprintf(endurance.last_issue, sizeof(endurance.last_issue),
                 "writeOneWord err=%d ec=0x%04X",
                 static_cast<int>(plc.lastError()), static_cast<unsigned>(plc.lastEndCode()));
        return false;
    }
    uint16_t rb_w = 0;
    strncpy(endurance.last_step, "readOneWord", sizeof(endurance.last_step) - 1);
    if (plc.readOneWord(kEnduranceOneWordDevice, rb_w) != slmp::Error::Ok || rb_w != wv) {
        snprintf(endurance.last_issue, sizeof(endurance.last_issue),
                 "readOneWord rb=0x%04X exp=0x%04X", rb_w, wv);
        return false;
    }

    // Bit
    strncpy(endurance.last_step, "writeOneBit", sizeof(endurance.last_step) - 1);
    if (plc.writeOneBit(kEnduranceOneBitDevice, bv) != slmp::Error::Ok) {
        snprintf(endurance.last_issue, sizeof(endurance.last_issue),
                 "writeOneBit err=%d", static_cast<int>(plc.lastError()));
        return false;
    }
    bool rb_b = false;
    strncpy(endurance.last_step, "readOneBit", sizeof(endurance.last_step) - 1);
    if (plc.readOneBit(kEnduranceOneBitDevice, rb_b) != slmp::Error::Ok || rb_b != bv) {
        snprintf(endurance.last_issue, sizeof(endurance.last_issue),
                 "readOneBit rb=%d exp=%d", rb_b ? 1 : 0, bv ? 1 : 0);
        return false;
    }

    // DWord
    strncpy(endurance.last_step, "writeOneDWord", sizeof(endurance.last_step) - 1);
    if (plc.writeOneDWord(kEnduranceOneDWordDevice, dv) != slmp::Error::Ok) {
        snprintf(endurance.last_issue, sizeof(endurance.last_issue),
                 "writeOneDWord err=%d", static_cast<int>(plc.lastError()));
        return false;
    }
    uint32_t rb_d = 0;
    strncpy(endurance.last_step, "readOneDWord", sizeof(endurance.last_step) - 1);
    if (plc.readOneDWord(kEnduranceOneDWordDevice, rb_d) != slmp::Error::Ok || rb_d != dv) {
        snprintf(endurance.last_issue, sizeof(endurance.last_issue),
                 "readOneDWord rb=0x%08lX exp=0x%08lX",
                 static_cast<unsigned long>(rb_d), static_cast<unsigned long>(dv));
        return false;
    }

    return true;
}

void printEnduranceStatus() {
    if (!endurance.active) { Serial.println(F("endurance=inactive")); return; }
    const uint32_t elapsed_s = (millis() - endurance.started_ms) / 1000UL;
    Serial.print(F("endurance=active attempts=")); Serial.print(endurance.attempts);
    Serial.print(F(" ok="));         Serial.print(endurance.ok);
    Serial.print(F(" fail="));       Serial.print(endurance.fail);
    Serial.print(F(" elapsed_s="));  Serial.print(elapsed_s);
    Serial.print(F(" last_ms="));    Serial.print(endurance.last_cycle_ms);
    Serial.print(F(" min_ms="));     Serial.print(endurance.min_cycle_ms);
    Serial.print(F(" max_ms="));     Serial.println(endurance.max_cycle_ms);
    if (endurance.last_issue[0]) {
        Serial.print(F("  last_issue: ")); Serial.println(endurance.last_issue);
    }
}

void stopEndurance(bool print_summary, bool failed) {
    if (!endurance.active) return;
    endurance.active = false;
    if (print_summary) {
        Serial.print(F("endurance stopped"));
        if (failed) Serial.print(F(" (failed)"));
        Serial.print(F(": attempts=")); Serial.print(endurance.attempts);
        Serial.print(F(" ok="));       Serial.print(endurance.ok);
        Serial.print(F(" fail="));     Serial.println(endurance.fail);
    }
}

void startEndurance(uint32_t cycle_limit) {
    stopWatch();
    stopReconnect(false);
    if (!connectPlc(true)) return;
    endurance             = {};
    endurance.active      = true;
    endurance.cycle_limit = cycle_limit;
    endurance.started_ms  = millis();
    endurance.last_report_ms = millis();
    Serial.print(F("endurance started"));
    if (cycle_limit > 0) { Serial.print(F(" limit=")); Serial.print(cycle_limit); }
    Serial.println();
}

void pollEnduranceTest() {
    if (!endurance.active) return;
    if (millis() < endurance.next_cycle_due_ms) return;
    endurance.next_cycle_due_ms = millis() + kEnduranceCycleGapMs;

    if (!connectPlc(false)) { ++endurance.fail; return; }

    const uint32_t t0 = millis();
    ++endurance.attempts;
    const bool ok = runEnduranceCycle();
    const uint32_t elapsed = millis() - t0;

    endurance.last_cycle_ms  = elapsed;
    endurance.total_cycle_ms += elapsed;
    if (endurance.attempts == 1 || elapsed < endurance.min_cycle_ms) endurance.min_cycle_ms = elapsed;
    if (elapsed > endurance.max_cycle_ms) endurance.max_cycle_ms = elapsed;

    if (ok) {
        ++endurance.ok;
        endurance.last_issue[0] = '\0';
    } else {
        ++endurance.fail;
    }

    if (millis() - endurance.last_report_ms >= kEnduranceReportIntervalMs) {
        printEnduranceStatus();
        endurance.last_report_ms = millis();
    }

    if (endurance.cycle_limit > 0 && endurance.attempts >= endurance.cycle_limit) {
        stopEndurance(true, endurance.fail > 0);
    }
}

// ---------------------------------------------------------------------------
// endurance command
//   endurance [cycles|status|stop]
// ---------------------------------------------------------------------------
void enduranceCommand(char* tokens[], int token_count) {
    if (token_count < 2) {
        if (!endurance.active) startEndurance(0);
        else printEnduranceStatus();
        return;
    }
    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "STOP")   == 0) { stopEndurance(true, false); return; }
    if (strcmp(tokens[1], "STATUS") == 0) { printEnduranceStatus(); return; }

    unsigned long v = 0;
    if (parseUnsignedValue(tokens[1], v, 10)) {
        startEndurance(static_cast<uint32_t>(v));
    } else {
        Serial.println(F("endurance usage: endurance [cycles|status|stop]"));
    }
}

// ---------------------------------------------------------------------------
// Reconnect: one cycle (close → connect → readOneWord verify)
// ---------------------------------------------------------------------------
bool runReconnectCycle() {
    plc.close();
    strncpy(reconnect.last_step, "connect", sizeof(reconnect.last_step) - 1);
    if (!connectPlc(false)) {
        strncpy(reconnect.last_issue, "connect failed", sizeof(reconnect.last_issue) - 1);
        return false;
    }
    uint16_t v = 0;
    strncpy(reconnect.last_step, "readOneWord", sizeof(reconnect.last_step) - 1);
    const slmp::Error e = plc.readOneWord(kEnduranceOneWordDevice, v);
    if (e != slmp::Error::Ok) {
        snprintf(reconnect.last_issue, sizeof(reconnect.last_issue),
                 "readOneWord err=%d ec=0x%04X",
                 static_cast<int>(e), static_cast<unsigned>(plc.lastEndCode()));
        return false;
    }
    return true;
}

void printReconnectStatus() {
    if (!reconnect.active) { Serial.println(F("reconnect=inactive")); return; }
    const uint32_t elapsed_s = (millis() - reconnect.started_ms) / 1000UL;
    Serial.print(F("reconnect=active attempts=")); Serial.print(reconnect.attempts);
    Serial.print(F(" ok="));          Serial.print(reconnect.ok);
    Serial.print(F(" fail="));        Serial.print(reconnect.fail);
    Serial.print(F(" recoveries="));  Serial.print(reconnect.recoveries);
    Serial.print(F(" elapsed_s="));   Serial.print(elapsed_s);
    Serial.print(F(" consec_fail=")); Serial.print(reconnect.consecutive_failures);
    Serial.print(F(" max_consec="));  Serial.println(reconnect.max_consecutive_failures);
    if (reconnect.last_issue[0]) {
        Serial.print(F("  last_issue: ")); Serial.println(reconnect.last_issue);
    }
}

void stopReconnect(bool print_summary) {
    if (!reconnect.active) return;
    reconnect.active = false;
    if (print_summary) {
        Serial.print(F("reconnect stopped: attempts=")); Serial.print(reconnect.attempts);
        Serial.print(F(" ok="));   Serial.print(reconnect.ok);
        Serial.print(F(" fail=")); Serial.println(reconnect.fail);
    }
}

void startReconnect(uint32_t cycle_limit) {
    stopWatch();
    stopEndurance(false, false);
    reconnect             = {};
    reconnect.active      = true;
    reconnect.cycle_limit = cycle_limit;
    reconnect.started_ms  = millis();
    reconnect.last_report_ms = millis();
    Serial.print(F("reconnect started"));
    if (cycle_limit > 0) { Serial.print(F(" limit=")); Serial.print(cycle_limit); }
    Serial.println();
}

void pollReconnectTest() {
    if (!reconnect.active) return;
    if (millis() < reconnect.next_cycle_due_ms) return;

    const uint32_t t0 = millis();
    ++reconnect.attempts;
    const bool ok = runReconnectCycle();
    const uint32_t elapsed = millis() - t0;

    reconnect.last_cycle_ms  = elapsed;
    reconnect.total_cycle_ms += elapsed;
    if (reconnect.attempts == 1 || elapsed < reconnect.min_cycle_ms) reconnect.min_cycle_ms = elapsed;
    if (elapsed > reconnect.max_cycle_ms) reconnect.max_cycle_ms = elapsed;

    if (ok) {
        if (reconnect.consecutive_failures > 0) ++reconnect.recoveries;
        ++reconnect.ok;
        reconnect.consecutive_failures = 0;
        reconnect.last_issue[0] = '\0';
        reconnect.next_cycle_due_ms = millis() + kReconnectCycleGapMs;
    } else {
        ++reconnect.fail;
        ++reconnect.consecutive_failures;
        if (reconnect.consecutive_failures > reconnect.max_consecutive_failures)
            reconnect.max_consecutive_failures = reconnect.consecutive_failures;
        const uint32_t gap = kReconnectCycleGapMs * reconnect.consecutive_failures;
        reconnect.next_cycle_due_ms = millis() +
            (gap > kReconnectMaxCycleGapMs ? kReconnectMaxCycleGapMs : gap);
    }

    if (millis() - reconnect.last_report_ms >= kReconnectReportIntervalMs) {
        printReconnectStatus();
        reconnect.last_report_ms = millis();
    }

    if (reconnect.cycle_limit > 0 && reconnect.attempts >= reconnect.cycle_limit) {
        stopReconnect(true);
    }
}

// ---------------------------------------------------------------------------
// reconnect command
//   reconnect [cycles|status|stop]
// ---------------------------------------------------------------------------
void reconnectCommand(char* tokens[], int token_count) {
    if (token_count < 2) {
        if (!reconnect.active) startReconnect(0);
        else printReconnectStatus();
        return;
    }
    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "STOP")   == 0) { stopReconnect(true); return; }
    if (strcmp(tokens[1], "STATUS") == 0) { printReconnectStatus(); return; }

    unsigned long v = 0;
    if (parseUnsignedValue(tokens[1], v, 10)) {
        startReconnect(static_cast<uint32_t>(v));
    } else {
        Serial.println(F("reconnect usage: reconnect [cycles|status|stop]"));
    }
}

// ---------------------------------------------------------------------------
// TX limit: calculation report
// ---------------------------------------------------------------------------
void printTxLimitCalc() {
    Serial.println(F("--- TX Limit Calculations ---"));
    Serial.print(F("TxBufferSize="));         Serial.println(static_cast<unsigned long>(kTxBufferSize));
    Serial.print(F("TxPayloadBudget="));      Serial.println(static_cast<unsigned long>(kTxPayloadBudget));
    Serial.print(F("WriteWords  max="));      Serial.println(static_cast<unsigned long>(kTxLimitWriteWordsMaxCount));
    Serial.print(F("WriteDWords max="));      Serial.println(static_cast<unsigned long>(kTxLimitWriteDWordsMaxCount));
    Serial.print(F("WriteBits   max="));      Serial.println(static_cast<unsigned long>(kTxLimitWriteBitsMaxCount));
    Serial.print(F("WriteRandBits  max="));   Serial.println(static_cast<unsigned long>(kTxLimitWriteRandomBitsMaxCount));
    Serial.print(F("WriteRandWords max="));   Serial.println(static_cast<unsigned long>(kTxLimitWriteRandomWordsMaxCount));
    Serial.print(F("WriteBlockWord max="));   Serial.println(static_cast<unsigned long>(kTxLimitWriteBlockWordMaxPoints));
}

// ---------------------------------------------------------------------------
// TX limit: probe helpers
// ---------------------------------------------------------------------------
bool runTxlimitProbeWords(size_t count) {
    for (size_t i = 0; i < count; ++i)
        txlimit_word_values[i] = static_cast<uint16_t>(i + 1U);
    if (plc.writeWords(kTxLimitWordDevice, txlimit_word_values, count) != slmp::Error::Ok)
        return false;
    for (size_t i = 0; i < count; ++i) txlimit_word_readback[i] = 0;
    if (plc.readWords(kTxLimitWordDevice, static_cast<uint16_t>(count),
                      txlimit_word_readback, count + 1U) != slmp::Error::Ok)
        return false;
    return wordArraysEqual(txlimit_word_values, txlimit_word_readback, static_cast<uint16_t>(count));
}

bool runTxlimitProbeBlock(size_t pts) {
    for (size_t i = 0; i < pts; ++i)
        txlimit_block_values[i] = static_cast<uint16_t>(i + 1U);
    const slmp::DeviceBlockWrite blk_w = {kTxLimitBlockWordDevice,
                                           txlimit_block_values,
                                           static_cast<uint16_t>(pts)};
    if (plc.writeBlock(nullptr, 0, &blk_w, 1) != slmp::Error::Ok) return false;
    for (size_t i = 0; i < pts; ++i) txlimit_block_readback[i] = 0;
    const slmp::DeviceBlockRead blk_r = {kTxLimitBlockWordDevice, static_cast<uint16_t>(pts)};
    if (plc.readBlock(&blk_r, 1, nullptr, 0,
                      txlimit_block_readback, pts, nullptr, 0) != slmp::Error::Ok)
        return false;
    return wordArraysEqual(txlimit_block_values, txlimit_block_readback, static_cast<uint16_t>(pts));
}

void runTxlimitBinarySearch() {
    if (!connectPlc(true)) return;

    Serial.println(F("--- TX Limit Probe: writeWords ---"));
    {
        size_t lo = 1, hi = kTxLimitWriteWordsMaxCount, last_ok = 0;
        while (lo <= hi) {
            const size_t mid = (lo + hi) / 2U;
            const bool ok = runTxlimitProbeWords(mid);
            Serial.print(F("  words=")); Serial.print(static_cast<unsigned long>(mid));
            Serial.println(ok ? F(" OK") : F(" FAIL"));
            if (ok) { last_ok = mid; lo = mid + 1U; }
            else    { if (mid == 0) break; hi = mid - 1U; }
        }
        Serial.print(F("writeWords probe max=")); Serial.println(static_cast<unsigned long>(last_ok));
    }

    Serial.println(F("--- TX Limit Probe: writeBlock(word) ---"));
    {
        size_t lo = 1, hi = kTxLimitWriteBlockWordMaxPoints, last_ok = 0;
        while (lo <= hi) {
            const size_t mid = (lo + hi) / 2U;
            const bool ok = runTxlimitProbeBlock(mid);
            Serial.print(F("  blk_pts=")); Serial.print(static_cast<unsigned long>(mid));
            Serial.println(ok ? F(" OK") : F(" FAIL"));
            if (ok) { last_ok = mid; lo = mid + 1U; }
            else    { if (mid == 0) break; hi = mid - 1U; }
        }
        Serial.print(F("writeBlock probe max=")); Serial.println(static_cast<unsigned long>(last_ok));
    }
}

void runTxlimitSweep() {
    if (!connectPlc(true)) return;
    Serial.println(F("--- TX Limit Sweep: writeWords 1..max ---"));
    size_t last_ok = 0;
    for (size_t n = 1; n <= kTxLimitWriteWordsMaxCount; ++n) {
        const bool ok = runTxlimitProbeWords(n);
        Serial.print(static_cast<unsigned long>(n));
        Serial.print(ok ? F(":OK ") : F(":FAIL "));
        if (ok) { last_ok = n; } else break;
        if (n % 16U == 0U) Serial.println();
    }
    Serial.println();
    Serial.print(F("sweep max=")); Serial.println(static_cast<unsigned long>(last_ok));
}

// ---------------------------------------------------------------------------
// txlimit command
//   txlimit [calc|probe|sweep]
// ---------------------------------------------------------------------------
void txlimitCommand(char* tokens[], int token_count) {
    if (token_count < 2) { printTxLimitCalc(); return; }

    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "CALC")  == 0) { printTxLimitCalc(); }
    else if (strcmp(tokens[1], "PROBE") == 0) { runTxlimitBinarySearch(); }
    else if (strcmp(tokens[1], "SWEEP") == 0) { runTxlimitSweep(); }
    else { Serial.println(F("txlimit usage: txlimit [calc|probe|sweep]")); }
}

// ---------------------------------------------------------------------------
// Full device scan: attempt readOneWord on every device type
// ---------------------------------------------------------------------------
void runFullScan(char* tokens[], int token_count) {
    if (!connectPlc(true)) return;

    uint32_t start_addr = 0;
    if (token_count >= 2) {
        unsigned long v = 0;
        if (parseUnsignedValue(tokens[1], v, 10)) start_addr = static_cast<uint32_t>(v);
    }

    constexpr size_t n_specs = sizeof(kDeviceSpecs) / sizeof(kDeviceSpecs[0]);
    Serial.print(F("--- Fullscan: ")); Serial.print(static_cast<unsigned long>(n_specs));
    Serial.print(F(" devices @ addr="));
    Serial.println(static_cast<unsigned long>(start_addr));

    uint32_t ok_count = 0, fail_count = 0;
    for (size_t i = 0; i < n_specs; ++i) {
        const DeviceSpec& spec = kDeviceSpecs[i];
        uint16_t v = 0;
        const slmp::Error e = plc.readOneWord({spec.code, start_addr}, v);

        Serial.print(spec.name); Serial.print(F(":"));
        if (e == slmp::Error::Ok) {
            if (spec.hex_address) Serial.print(start_addr, HEX);
            else                  Serial.print(static_cast<unsigned long>(start_addr));
            Serial.print(F("="));
            Serial.print(v);
            Serial.print(F("(0x")); Serial.print(v, HEX); Serial.print(F(")"));
            ++ok_count;
        } else if (e == slmp::Error::PlcError) {
            Serial.print(F("EC=0x")); Serial.print(plc.lastEndCode(), HEX);
            ++fail_count;
        } else {
            Serial.print(F("err=")); Serial.print(static_cast<int>(e));
            ++fail_count;
        }

        if ((i + 1U) % 4U == 0U) Serial.println();
        else                     Serial.print(F("  "));
    }
    Serial.println();
    Serial.print(F("scan done: ok=")); Serial.print(ok_count);
    Serial.print(F(" fail="));        Serial.println(fail_count);
}

void fullscanCommand(char* tokens[], int token_count) {
    runFullScan(tokens, token_count);
}
