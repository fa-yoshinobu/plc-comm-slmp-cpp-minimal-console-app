/**
 * @file console_watch.h
 * @brief Watch mode (continuous device polling) and self-test loopback command.
 *
 * Included by w6300_evb_pico2_serial_console.h inside its namespace after console_hw.h.
 *
 * Watch syntax:
 *   watch device [pts] [ms]      - watch words (default 1 pt, 1000 ms)
 *   watch b device [pts] [ms]    - watch bits
 *   watch d device [pts] [ms]    - watch dwords
 *   watch off | watch stop       - stop watching
 *   watch status                 - print current watch config
 */

// ---------------------------------------------------------------------------
// Watch helpers
// ---------------------------------------------------------------------------
void stopWatch() {
    if (!watch_session.active) return;
    watch_session.active   = false;
    watch_session.has_prev = false;
    Serial.println(F("watch=off"));
}

void printWatchStatus() {
    if (!watch_session.active) { Serial.println(F("watch=off")); return; }
    Serial.print(F("watch=on type="));
    switch (watch_session.type) {
        case WatchType::Word:  Serial.print(F("word"));  break;
        case WatchType::Bit:   Serial.print(F("bit"));   break;
        case WatchType::DWord: Serial.print(F("dword")); break;
    }
    Serial.print(F(" device="));
    printDeviceAddress(watch_session.device);
    Serial.print(F(" points=")); Serial.print(watch_session.points);
    Serial.print(F(" interval_ms=")); Serial.println(watch_session.interval_ms);
}

// ---------------------------------------------------------------------------
// Poll (called every loop iteration)
// ---------------------------------------------------------------------------
void pollWatch() {
    if (!watch_session.active) return;
    if (millis() < watch_session.next_poll_ms) return;
    watch_session.next_poll_ms = millis() + watch_session.interval_ms;

    if (!connectPlc(false)) return;   // silently skip if disconnected

    const uint16_t pts = watch_session.points;
    Serial.print(F("["));
    Serial.print(millis());
    Serial.print(F("ms] "));

    if (watch_session.type == WatchType::Word) {
        uint16_t values[kMaxPoints] = {};
        if (plc.readWords(watch_session.device, pts, values, kMaxPoints) != slmp::Error::Ok) {
            Serial.println(F("read error"));
            return;
        }
        for (uint16_t i = 0; i < pts; ++i) {
            if (i != 0) Serial.print(F("  "));
            printDeviceAddress(watch_session.device, i);
            Serial.print(F("="));
            Serial.print(values[i]);
            Serial.print(F("(0x")); Serial.print(values[i], HEX); Serial.print(F(")"));
            if (watch_session.has_prev && values[i] != watch_session.prev_words[i])
                Serial.print(F("*"));
            watch_session.prev_words[i] = values[i];
        }
        Serial.println();

    } else if (watch_session.type == WatchType::Bit) {
        bool values[kMaxPoints] = {};
        if (plc.readBits(watch_session.device, pts, values, kMaxPoints) != slmp::Error::Ok) {
            Serial.println(F("read error"));
            return;
        }
        for (uint16_t i = 0; i < pts; ++i) {
            if (i != 0) Serial.print(F(" "));
            printDeviceAddress(watch_session.device, i);
            Serial.print(F("="));
            Serial.print(values[i] ? 1 : 0);
            if (watch_session.has_prev && values[i] != watch_session.prev_bits[i])
                Serial.print(F("*"));
            watch_session.prev_bits[i] = values[i];
        }
        Serial.println();

    } else {  // DWord
        uint32_t values[kMaxPoints] = {};
        if (plc.readDWords(watch_session.device, pts, values, kMaxPoints) != slmp::Error::Ok) {
            Serial.println(F("read error"));
            return;
        }
        for (uint16_t i = 0; i < pts; ++i) {
            if (i != 0) Serial.print(F("  "));
            printDeviceAddress(watch_session.device, i * 2U);
            Serial.print(F("="));
            Serial.print(static_cast<unsigned long>(values[i]));
            Serial.print(F("(0x")); Serial.print(static_cast<unsigned long>(values[i]), HEX); Serial.print(F(")"));
            if (watch_session.has_prev && values[i] != watch_session.prev_dwords[i])
                Serial.print(F("*"));
            watch_session.prev_dwords[i] = values[i];
        }
        Serial.println();
    }

    watch_session.has_prev = true;
}

// ---------------------------------------------------------------------------
// watch command
//   watch [b|d] device [pts] [ms]
//   watch off | status
// ---------------------------------------------------------------------------
void watchCommand(char* tokens[], int token_count) {
    if (token_count < 2) { printWatchStatus(); return; }

    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "OFF") == 0 || strcmp(tokens[1], "STOP") == 0) { stopWatch(); return; }
    if (strcmp(tokens[1], "STATUS") == 0) { printWatchStatus(); return; }

    // Determine type from optional first sub-token
    WatchType type = WatchType::Word;
    int dev_index  = 1;
    if (strcmp(tokens[1], "B") == 0) {
        type = WatchType::Bit; dev_index = 2;
    } else if (strcmp(tokens[1], "D") == 0) {
        type = WatchType::DWord; dev_index = 2;
    }

    if (token_count <= dev_index) {
        Serial.println(F("watch usage: watch [b|d] <device> [pts] [ms]"));
        return;
    }

    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(tokens[dev_index], device)) {
        Serial.println(F("watch: device parse failed"));
        return;
    }

    uint16_t points       = 1;
    uint32_t interval_ms  = kWatchDefaultIntervalMs;
    const int pts_index = dev_index + 1;
    const int ms_index  = dev_index + 2;

    if (token_count > pts_index && tokens[pts_index] != nullptr) {
        uint16_t p = 0;
        if (!parsePointCount(tokens[pts_index], p)) {
            Serial.print(F("watch: pts must be 1..")); Serial.println(kMaxPoints); return;
        }
        points = p;
    }
    if (token_count > ms_index && tokens[ms_index] != nullptr) {
        unsigned long ms = 0;
        if (!parseUnsignedValue(tokens[ms_index], ms, 10) || ms < kWatchMinIntervalMs) {
            Serial.print(F("watch: interval_ms must be >= ")); Serial.println(kWatchMinIntervalMs); return;
        }
        interval_ms = static_cast<uint32_t>(ms);
    }

    stopEndurance(false, false);
    stopReconnect(false);

    watch_session.active      = true;
    watch_session.type        = type;
    watch_session.device      = device;
    watch_session.points      = points;
    watch_session.interval_ms = interval_ms;
    watch_session.next_poll_ms= millis();
    watch_session.has_prev    = false;

    Serial.print(F("watch=on type="));
    switch (type) {
        case WatchType::Word:  Serial.print(F("word"));  break;
        case WatchType::Bit:   Serial.print(F("bit"));   break;
        case WatchType::DWord: Serial.print(F("dword")); break;
    }
    Serial.print(F(" device="));
    printDeviceAddress(device);
    Serial.print(F(" points=")); Serial.print(points);
    Serial.print(F(" interval_ms=")); Serial.println(interval_ms);
    Serial.println(F("(* = changed from previous poll)"));
    Serial.println(F("type \"watch off\" to stop"));
}

// ---------------------------------------------------------------------------
// loopback command - self-test (command 0619)
//   loopback [text]   default text: "SLMP"
// ---------------------------------------------------------------------------
void loopbackCommand(char* tokens[], int token_count) {
    if (!connectPlc(true)) return;

    // Build payload from optional text argument(s)
    char text[64] = "SLMP";
    if (token_count >= 2) {
        joinTokens(tokens, 1, token_count, text, sizeof(text));
    }

    const size_t req_len = strlen(text);
    uint8_t  resp_buf[128] = {};
    size_t   resp_len = 0;

    const slmp::Error err = plc.selfTestLoopback(
        reinterpret_cast<const uint8_t*>(text), req_len,
        resp_buf, sizeof(resp_buf), resp_len);

    if (err != slmp::Error::Ok) { printLastPlcError("loopback"); return; }

    resp_buf[resp_len < sizeof(resp_buf) ? resp_len : sizeof(resp_buf) - 1U] = '\0';
    Serial.print(F("loopback ok send=\"")); Serial.print(text);
    Serial.print(F("\" recv=\"")); Serial.print(reinterpret_cast<const char*>(resp_buf));
    Serial.println(F("\""));
}
