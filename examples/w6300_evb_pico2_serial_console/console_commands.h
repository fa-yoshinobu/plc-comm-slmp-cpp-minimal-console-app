/**
 * @file console_commands.h
 * @brief Interactive read/write/verify commands.
 *
 * Included by w6300_evb_pico2_serial_console.h inside its namespace after console_watch.h.
 * Depends on helpers defined in console_hw.h.
 */

// ---------------------------------------------------------------------------
// Array comparison helpers
// ---------------------------------------------------------------------------
bool wordArraysEqual(const uint16_t* a, const uint16_t* b, uint16_t n) {
    for (uint16_t i = 0; i < n; ++i) if (a[i] != b[i]) return false;
    return true;
}
bool dwordArraysEqual(const uint32_t* a, const uint32_t* b, size_t n) {
    for (size_t i = 0; i < n; ++i) if (a[i] != b[i]) return false;
    return true;
}
bool bitArraysEqual(const bool* a, const bool* b, size_t n) {
    for (size_t i = 0; i < n; ++i) if (a[i] != b[i]) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Silent clear helpers (used by verify and test suites)
// ---------------------------------------------------------------------------
void clearWordsSilently(const slmp::DeviceAddress& device, size_t count) {
    if (count == 0 || !connectPlc(false)) return;
    uint16_t zeros[kMaxPoints] = {};
    size_t offset = 0;
    while (offset < count) {
        const size_t chunk = (count - offset) > kMaxPoints ? kMaxPoints : (count - offset);
        (void)plc.writeWords({device.code, device.number + static_cast<uint32_t>(offset)}, zeros, chunk);
        offset += chunk;
    }
}
void clearBitsSilently(const slmp::DeviceAddress& device, size_t count) {
    if (count == 0 || !connectPlc(false)) return;
    if (count == 1) { (void)plc.writeOneBit(device, false); return; }
    bool zeros[kMaxPoints] = {};
    const size_t chunk = count > kMaxPoints ? kMaxPoints : count;
    (void)plc.writeBits(device, zeros, chunk);
}
void clearDWordsSilently(const slmp::DeviceAddress& device, size_t count) {
    if (count == 0 || !connectPlc(false)) return;
    if (count == 1) { (void)plc.writeOneDWord(device, 0U); return; }
    uint32_t zeros[kMaxPoints] = {};
    const size_t chunk = count > kMaxPoints ? kMaxPoints : count;
    (void)plc.writeDWords(device, zeros, chunk);
}
void clearRandomWordsSilently(
    const slmp::DeviceAddress* wdevs, size_t wn,
    const slmp::DeviceAddress* ddevs, size_t dn)
{
    if (!connectPlc(false)) return;
    uint16_t zw[kMaxRandomWordDevices]  = {};
    uint32_t zd[kMaxRandomDWordDevices] = {};
    (void)plc.writeRandomWords(wdevs, zw, wn, ddevs, zd, dn);
}
void clearRandomBitsSilently(const slmp::DeviceAddress* devs, size_t n) {
    if (!connectPlc(false)) return;
    bool zeros[kMaxRandomBitDevices] = {};
    (void)plc.writeRandomBits(devs, zeros, n);
}
void clearPackedBitWordsSilently(const slmp::DeviceAddress& device, uint16_t packed_word_count) {
    if (!connectPlc(false)) return;
    uint16_t zeros[kMaxBlockPoints] = {};
    if (packed_word_count <= kMaxBlockPoints) {
        const slmp::DeviceBlockWrite blk = {device, zeros, packed_word_count};
        if (plc.writeBlock(nullptr, 0, &blk, 1) == slmp::Error::Ok) return;
    }
    const uint32_t bit_count = static_cast<uint32_t>(packed_word_count) * 16U;
    for (uint32_t off = 0; off < bit_count; ++off)
        (void)plc.writeOneBit({device.code, device.number + off}, false);
}

// ---------------------------------------------------------------------------
// Verification record
// ---------------------------------------------------------------------------
void resetVerificationRecord() { verification = VerificationRecord(); }

void printVerificationSummary() {
    if (!verification.active) { Serial.println(F("verification: none")); return; }
    Serial.print(F("kind="));
    Serial.println(verification.kind == VerificationKind::WordWrite ? F("word_write") : F("bit_write"));
    Serial.print(F("device=")); printDeviceAddress(verification.device); Serial.println();

    if (verification.kind == VerificationKind::WordWrite) {
        for (uint16_t i = 0; i < verification.points; ++i) {
            printDeviceAddress(verification.device, i);
            Serial.print(F("  before=")); Serial.print(verification.before_words[i]);
            Serial.print(F("  written=")); Serial.print(verification.written_words[i]);
            Serial.print(F("  readback=")); Serial.println(verification.readback_words[i]);
        }
    } else {
        printDeviceAddress(verification.device);
        Serial.print(F("  before=")); Serial.print(verification.before_bit ? 1 : 0);
        Serial.print(F("  written=")); Serial.print(verification.written_bit ? 1 : 0);
        Serial.print(F("  readback=")); Serial.println(verification.readback_bit ? 1 : 0);
    }
    Serial.print(F("auto_readback_match=")); Serial.println(verification.readback_match ? F("yes") : F("no"));

    if (!verification.judged) {
        Serial.println(F("judgement=pending  -> judge ok [note]  or  judge ng [note]"));
        return;
    }
    Serial.print(F("judgement=")); Serial.println(verification.pass ? F("ok") : F("ng"));
    if (verification.note[0] != '\0') {
        Serial.print(F("note=")); Serial.println(verification.note);
    }
}

// ---------------------------------------------------------------------------
// Read words   r/rw <dev> [pts]
// ---------------------------------------------------------------------------
void readWordsCommand(char* device_token, char* points_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) { Serial.println(F("r usage: r <device> [points]")); return; }
    uint16_t points = 1;
    if (points_token && !parsePointCount(points_token, points)) {
        Serial.print(F("r: points must be 1..")); Serial.println(kMaxPoints); return;
    }
    if (!connectPlc(false)) { Serial.println(F("r: not connected")); return; }

    uint16_t values[kMaxPoints] = {};
    if (plc.readWords(device, points, values, kMaxPoints) != slmp::Error::Ok) {
        printLastPlcError("readWords"); return;
    }
    for (uint16_t i = 0; i < points; ++i) {
        printDeviceAddress(device, i);
        Serial.print(F("="));  Serial.print(values[i]);
        Serial.print(F(" (0x")); Serial.print(values[i], HEX); Serial.println(F(")"));
    }
}

// ---------------------------------------------------------------------------
// Write words   w/ww <dev> <val...>
// ---------------------------------------------------------------------------
void writeWordsCommand(char* tokens[], int token_count) {
    if (token_count < 3) { Serial.println(F("w usage: w <device> <value...>")); return; }
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(tokens[1], device)) { Serial.println(F("w: device parse failed")); return; }
    const int vc = token_count - 2;
    if (vc <= 0 || vc > static_cast<int>(kMaxPoints)) {
        Serial.print(F("w: 1..")); Serial.print(kMaxPoints); Serial.println(F(" values")); return;
    }
    uint16_t values[kMaxPoints] = {};
    for (int i = 0; i < vc; ++i) {
        if (!parseWordValue(tokens[i + 2], values[i])) { Serial.println(F("w: value must fit 16 bits")); return; }
    }
    if (!connectPlc(false)) { Serial.println(F("w: not connected")); return; }
    if (plc.writeWords(device, values, static_cast<size_t>(vc)) != slmp::Error::Ok) {
        printLastPlcError("writeWords"); return;
    }
    Serial.println(F("ok"));
}

// ---------------------------------------------------------------------------
// Read one word   row <dev>
// ---------------------------------------------------------------------------
void readOneWordCommand(char* device_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) { Serial.println(F("row usage: row <device>")); return; }
    if (!connectPlc(false)) { Serial.println(F("row: not connected")); return; }
    uint16_t value = 0;
    if (plc.readOneWord(device, value) != slmp::Error::Ok) { printLastPlcError("readOneWord"); return; }
    printDeviceAddress(device);
    Serial.print(F("=")); Serial.print(value);
    Serial.print(F(" (0x")); Serial.print(value, HEX); Serial.println(F(")"));
}

// ---------------------------------------------------------------------------
// Write one word   wow <dev> <val>
// ---------------------------------------------------------------------------
void writeOneWordCommand(char* device_token, char* value_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) { Serial.println(F("wow usage: wow <device> <value>")); return; }
    uint16_t value = 0;
    if (!parseWordValue(value_token, value)) { Serial.println(F("wow: value must fit 16 bits")); return; }
    if (!connectPlc(false)) { Serial.println(F("wow: not connected")); return; }
    if (plc.writeOneWord(device, value) != slmp::Error::Ok) { printLastPlcError("writeOneWord"); return; }
    Serial.println(F("ok"));
}

// ---------------------------------------------------------------------------
// Read bits   b/rbits <dev> [pts]
// ---------------------------------------------------------------------------
void readBitsCommand(char* device_token, char* points_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) { Serial.println(F("b usage: b <device> [points]")); return; }
    uint16_t points = 1;
    if (points_token && !parsePointCount(points_token, points)) {
        Serial.print(F("b: points must be 1..")); Serial.println(kMaxPoints); return;
    }
    if (!connectPlc(false)) { Serial.println(F("b: not connected")); return; }
    bool values[kMaxPoints] = {};
    if (plc.readBits(device, points, values, kMaxPoints) != slmp::Error::Ok) {
        printLastPlcError("readBits"); return;
    }
    for (uint16_t i = 0; i < points; ++i) {
        printDeviceAddress(device, i);
        Serial.print(F("=")); Serial.println(values[i] ? 1 : 0);
    }
}

// ---------------------------------------------------------------------------
// Read one bit   rb <dev>
// ---------------------------------------------------------------------------
void readOneBitCommand(char* device_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) { Serial.println(F("rb usage: rb <device>")); return; }
    if (!connectPlc(false)) { Serial.println(F("rb: not connected")); return; }
    bool value = false;
    if (plc.readOneBit(device, value) != slmp::Error::Ok) { printLastPlcError("readOneBit"); return; }
    printDeviceAddress(device); Serial.print(F("=")); Serial.println(value ? 1 : 0);
}

// ---------------------------------------------------------------------------
// Write bits   wb/wbits <dev> <val...>   (0/1/on/off/true/false)
// ---------------------------------------------------------------------------
void writeBitsCommand(char* tokens[], int token_count) {
    if (token_count < 3) { Serial.println(F("wb usage: wb <device> <0|1...>")); return; }
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(tokens[1], device)) { Serial.println(F("wb: device parse failed")); return; }
    const int vc = token_count - 2;
    if (vc <= 0 || vc > static_cast<int>(kMaxPoints)) {
        Serial.print(F("wb: 1..")); Serial.print(kMaxPoints); Serial.println(F(" values")); return;
    }
    bool values[kMaxPoints] = {};
    for (int i = 0; i < vc; ++i) {
        if (!parseBoolValue(tokens[i + 2], values[i])) { Serial.println(F("wb: use 0/1/on/off/true/false")); return; }
    }
    if (!connectPlc(false)) { Serial.println(F("wb: not connected")); return; }
    if (vc == 1) {
        if (plc.writeOneBit(device, values[0]) != slmp::Error::Ok) { printLastPlcError("writeOneBit"); return; }
    } else {
        if (plc.writeBits(device, values, static_cast<size_t>(vc)) != slmp::Error::Ok) { printLastPlcError("writeBits"); return; }
    }
    Serial.println(F("ok"));
}

// ---------------------------------------------------------------------------
// Read dwords   rd/rdw <dev> [pts]
// ---------------------------------------------------------------------------
void readDWordsCommand(char* device_token, char* points_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) { Serial.println(F("rd usage: rd <device> [points]")); return; }
    uint16_t points = 1;
    if (points_token && !parsePositiveCount(points_token, static_cast<uint16_t>(kMaxPoints), points)) {
        Serial.print(F("rd: points must be 1..")); Serial.println(kMaxPoints); return;
    }
    if (!connectPlc(false)) { Serial.println(F("rd: not connected")); return; }
    uint32_t values[kMaxPoints] = {};
    if (plc.readDWords(device, points, values, kMaxPoints) != slmp::Error::Ok) {
        printLastPlcError("readDWords"); return;
    }
    for (uint16_t i = 0; i < points; ++i) {
        printDeviceAddress(device, i * 2U);
        Serial.print(F("=")); Serial.print(static_cast<unsigned long>(values[i]));
        Serial.print(F(" (0x")); Serial.print(static_cast<unsigned long>(values[i]), HEX); Serial.println(F(")"));
    }
}

// ---------------------------------------------------------------------------
// Write dwords   wd/wdw <dev> <val...>
// ---------------------------------------------------------------------------
void writeDWordsCommand(char* tokens[], int token_count) {
    if (token_count < 3) { Serial.println(F("wd usage: wd <device> <value...>")); return; }
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(tokens[1], device)) { Serial.println(F("wd: device parse failed")); return; }
    const int vc = token_count - 2;
    if (vc <= 0 || vc > static_cast<int>(kMaxPoints)) {
        Serial.print(F("wd: 1..")); Serial.print(kMaxPoints); Serial.println(F(" values")); return;
    }
    uint32_t values[kMaxPoints] = {};
    for (int i = 0; i < vc; ++i) {
        if (!parseDWordValue(tokens[i + 2], values[i])) { Serial.println(F("wd: value must fit 32 bits")); return; }
    }
    if (!connectPlc(false)) { Serial.println(F("wd: not connected")); return; }
    if (plc.writeDWords(device, values, static_cast<size_t>(vc)) != slmp::Error::Ok) {
        printLastPlcError("writeDWords"); return;
    }
    Serial.println(F("ok"));
}

// ---------------------------------------------------------------------------
// Read one dword   rod <dev>
// ---------------------------------------------------------------------------
void readOneDWordCommand(char* device_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) { Serial.println(F("rod usage: rod <device>")); return; }
    if (!connectPlc(false)) { Serial.println(F("rod: not connected")); return; }
    uint32_t value = 0;
    if (plc.readOneDWord(device, value) != slmp::Error::Ok) { printLastPlcError("readOneDWord"); return; }
    printDeviceAddress(device);
    Serial.print(F("=")); Serial.print(static_cast<unsigned long>(value));
    Serial.print(F(" (0x")); Serial.print(static_cast<unsigned long>(value), HEX); Serial.println(F(")"));
}

// ---------------------------------------------------------------------------
// Write one dword   wod <dev> <val>
// ---------------------------------------------------------------------------
void writeOneDWordCommand(char* device_token, char* value_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) { Serial.println(F("wod usage: wod <device> <value>")); return; }
    uint32_t value = 0;
    if (!parseDWordValue(value_token, value)) { Serial.println(F("wod: value must fit 32 bits")); return; }
    if (!connectPlc(false)) { Serial.println(F("wod: not connected")); return; }
    if (plc.writeOneDWord(device, value) != slmp::Error::Ok) { printLastPlcError("writeOneDWord"); return; }
    Serial.println(F("ok"));
}

// ---------------------------------------------------------------------------
// Read random   rr <wn> <devs..> <dn> <devs..>
// ---------------------------------------------------------------------------
void readRandomCommand(char* tokens[], int token_count) {
    if (token_count < 3) { Serial.println(F("rr usage: rr <wn> <devs..> <dn> <devs..>")); return; }
    int idx = 1;
    uint8_t wn = 0;
    if (!parseListCount(tokens[idx++], kMaxRandomWordDevices, wn)) {
        Serial.println(F("rr: word_count must be 0..16")); return;
    }
    if (token_count < idx + wn + 1) { Serial.println(F("rr: too few tokens")); return; }
    slmp::DeviceAddress wd[kMaxRandomWordDevices] = {};
    for (uint8_t i = 0; i < wn; ++i) {
        if (!parseDeviceAddress(tokens[idx++], wd[i])) { Serial.println(F("rr: word device parse failed")); return; }
    }
    uint8_t dn = 0;
    if (!parseListCount(tokens[idx++], kMaxRandomDWordDevices, dn)) {
        Serial.println(F("rr: dword_count must be 0..16")); return;
    }
    if (wn == 0 && dn == 0) { Serial.println(F("rr: need at least one device")); return; }
    if (token_count != idx + dn) { Serial.println(F("rr usage: rr <wn> <devs..> <dn> <devs..>")); return; }
    slmp::DeviceAddress dd[kMaxRandomDWordDevices] = {};
    for (uint8_t i = 0; i < dn; ++i) {
        if (!parseDeviceAddress(tokens[idx++], dd[i])) { Serial.println(F("rr: dword device parse failed")); return; }
    }
    if (!connectPlc(false)) { Serial.println(F("rr: not connected")); return; }

    uint16_t wv[kMaxRandomWordDevices]  = {};
    uint32_t dv[kMaxRandomDWordDevices] = {};
    if (plc.readRandom(wd, wn, wv, kMaxRandomWordDevices, dd, dn, dv, kMaxRandomDWordDevices) != slmp::Error::Ok) {
        printLastPlcError("readRandom"); return;
    }
    for (uint8_t i = 0; i < wn; ++i) {
        printDeviceAddress(wd[i]);
        Serial.print(F("="));  Serial.print(wv[i]);
        Serial.print(F(" (0x")); Serial.print(wv[i], HEX); Serial.println(F(")"));
    }
    for (uint8_t i = 0; i < dn; ++i) {
        printDeviceAddress(dd[i]);
        Serial.print(F("=")); Serial.print(static_cast<unsigned long>(dv[i]));
        Serial.print(F(" (0x")); Serial.print(static_cast<unsigned long>(dv[i]), HEX); Serial.println(F(")"));
    }
}

// ---------------------------------------------------------------------------
// Write random words   wrand <wn> <dev val..> <dn> <dev val..>
// ---------------------------------------------------------------------------
void writeRandomWordsCommand(char* tokens[], int token_count) {
    if (token_count < 2) { Serial.println(F("wrand usage: wrand <wn> <dev val..> <dn> <dev val..>")); return; }
    int idx = 1;
    uint8_t wn = 0;
    if (!parseListCount(tokens[idx++], kMaxRandomWordDevices, wn)) {
        Serial.println(F("wrand: word_count must be 0..16")); return;
    }
    if (token_count < idx + wn * 2 + 1) { Serial.println(F("wrand: too few tokens")); return; }
    slmp::DeviceAddress wd[kMaxRandomWordDevices] = {};
    uint16_t wv[kMaxRandomWordDevices] = {};
    for (uint8_t i = 0; i < wn; ++i) {
        if (!parseDeviceAddress(tokens[idx++], wd[i])) { Serial.println(F("wrand: device parse failed")); return; }
        if (!parseWordValue(tokens[idx++], wv[i]))     { Serial.println(F("wrand: word value must fit 16 bits")); return; }
    }
    uint8_t dn = 0;
    if (!parseListCount(tokens[idx++], kMaxRandomDWordDevices, dn)) {
        Serial.println(F("wrand: dword_count must be 0..16")); return;
    }
    if (wn == 0 && dn == 0) { Serial.println(F("wrand: need at least one device")); return; }
    if (token_count != idx + dn * 2) { Serial.println(F("wrand usage: wrand <wn> <dev val..> <dn> <dev val..>")); return; }
    slmp::DeviceAddress dd[kMaxRandomDWordDevices] = {};
    uint32_t dv[kMaxRandomDWordDevices] = {};
    for (uint8_t i = 0; i < dn; ++i) {
        if (!parseDeviceAddress(tokens[idx++], dd[i])) { Serial.println(F("wrand: dword device parse failed")); return; }
        if (!parseDWordValue(tokens[idx++], dv[i]))    { Serial.println(F("wrand: dword value must fit 32 bits")); return; }
    }
    if (!connectPlc(false)) { Serial.println(F("wrand: not connected")); return; }
    if (plc.writeRandomWords(wd, wv, wn, dd, dv, dn) != slmp::Error::Ok) {
        printLastPlcError("writeRandomWords"); return;
    }
    Serial.println(F("ok"));
}

// ---------------------------------------------------------------------------
// Write random bits   wrandb <n> <dev val..>
// ---------------------------------------------------------------------------
void writeRandomBitsCommand(char* tokens[], int token_count) {
    if (token_count < 2) { Serial.println(F("wrandb usage: wrandb <n> <dev val..>")); return; }
    int idx = 1;
    uint8_t n = 0;
    if (!parseListCount(tokens[idx++], kMaxRandomBitDevices, n) || n == 0) {
        Serial.println(F("wrandb: count must be 1..16")); return;
    }
    if (token_count != idx + n * 2) { Serial.println(F("wrandb usage: wrandb <n> <dev val..>")); return; }
    slmp::DeviceAddress devs[kMaxRandomBitDevices] = {};
    bool vals[kMaxRandomBitDevices] = {};
    for (uint8_t i = 0; i < n; ++i) {
        if (!parseDeviceAddress(tokens[idx++], devs[i])) { Serial.println(F("wrandb: device parse failed")); return; }
        if (!parseBoolValue(tokens[idx++], vals[i]))      { Serial.println(F("wrandb: use 0/1/on/off")); return; }
    }
    if (!connectPlc(false)) { Serial.println(F("wrandb: not connected")); return; }
    if (plc.writeRandomBits(devs, vals, n) != slmp::Error::Ok) {
        printLastPlcError("writeRandomBits"); return;
    }
    Serial.println(F("ok"));
}

// ---------------------------------------------------------------------------
// Read block   rblk <wb> <dev pts..> <bb> <dev pts..>
// ---------------------------------------------------------------------------
void readBlockCommand(char* tokens[], int token_count) {
    if (token_count < 2) {
        Serial.println(F("rblk usage: rblk <wblocks> <dev pts..> <bblocks> <dev pts..>")); return;
    }
    int idx = 1;
    uint8_t wbc = 0;
    if (!parseListCount(tokens[idx++], kMaxBlockCount, wbc)) {
        Serial.println(F("rblk: word_blocks must be 0..8")); return;
    }
    slmp::DeviceBlockRead wb[kMaxBlockCount] = {};
    uint16_t total_words = 0;
    for (uint8_t i = 0; i < wbc; ++i) {
        if (token_count <= idx + 1) { Serial.println(F("rblk: word block definition incomplete")); return; }
        slmp::DeviceAddress dev = {};
        uint16_t pts = 0;
        if (!parseDeviceAddress(tokens[idx++], dev)) { Serial.println(F("rblk: word device parse failed")); return; }
        if (!parsePositiveCount(tokens[idx++], static_cast<uint16_t>(kMaxBlockPoints), pts) || total_words + pts > kMaxBlockPoints) {
            Serial.println(F("rblk: total word points must be <= 64")); return;
        }
        wb[i] = {dev, pts};
        total_words = static_cast<uint16_t>(total_words + pts);
    }
    if (token_count <= idx) { Serial.println(F("rblk: bit_blocks count missing")); return; }
    uint8_t bbc = 0;
    if (!parseListCount(tokens[idx++], kMaxBlockCount, bbc)) {
        Serial.println(F("rblk: bit_blocks must be 0..8")); return;
    }
    if (wbc == 0 && bbc == 0) { Serial.println(F("rblk: need at least one block")); return; }
    slmp::DeviceBlockRead bb[kMaxBlockCount] = {};
    uint16_t total_bits = 0;
    for (uint8_t i = 0; i < bbc; ++i) {
        if (token_count <= idx + 1) { Serial.println(F("rblk: bit block definition incomplete")); return; }
        slmp::DeviceAddress dev = {};
        uint16_t pts = 0;
        if (!parseDeviceAddress(tokens[idx++], dev)) { Serial.println(F("rblk: bit device parse failed")); return; }
        if (!parsePositiveCount(tokens[idx++], static_cast<uint16_t>(kMaxBlockPoints), pts) || total_bits + pts > kMaxBlockPoints) {
            Serial.println(F("rblk: total bit points must be <= 64")); return;
        }
        bb[i] = {dev, pts};
        total_bits = static_cast<uint16_t>(total_bits + pts);
    }
    if (token_count != idx) { Serial.println(F("rblk usage: rblk <wb> <dev pts..> <bb> <dev pts..>")); return; }
    if (!connectPlc(false)) { Serial.println(F("rblk: not connected")); return; }

    uint16_t word_buf[kMaxBlockPoints] = {};
    uint16_t bit_buf[kMaxBlockPoints]  = {};
    if (plc.readBlock(wb, wbc, bb, bbc, word_buf, kMaxBlockPoints, bit_buf, kMaxBlockPoints) != slmp::Error::Ok) {
        printLastPlcError("readBlock"); return;
    }
    uint16_t wo = 0;
    for (uint8_t i = 0; i < wbc; ++i) {
        for (uint16_t j = 0; j < wb[i].points; ++j) {
            printDeviceAddress(wb[i].device, j);
            Serial.print(F("=")); Serial.print(word_buf[wo + j]);
            Serial.print(F(" (0x")); Serial.print(word_buf[wo + j], HEX); Serial.println(F(")"));
        }
        wo = static_cast<uint16_t>(wo + wb[i].points);
    }
    uint16_t bo = 0;
    for (uint8_t i = 0; i < bbc; ++i) {
        for (uint16_t j = 0; j < bb[i].points; ++j) {
            Serial.print(F("bit_block[")); Serial.print(i); Serial.print(F("] packed_word[")); Serial.print(j);
            Serial.print(F("]=0x")); Serial.println(bit_buf[bo + j], HEX);
        }
        bo = static_cast<uint16_t>(bo + bb[i].points);
    }
}

// ---------------------------------------------------------------------------
// Write block   wblk <wb> <dev pts vals..> <bb> <dev pts packed..>
// ---------------------------------------------------------------------------
void writeBlockCommand(char* tokens[], int token_count) {
    if (token_count < 2) {
        Serial.println(F("wblk usage: wblk <wb> <dev pts vals..> <bb> <dev pts packed..>")); return;
    }
    int idx = 1;
    uint8_t wbc = 0;
    if (!parseListCount(tokens[idx++], kMaxBlockCount, wbc)) {
        Serial.println(F("wblk: word_blocks must be 0..8")); return;
    }
    slmp::DeviceBlockWrite wb[kMaxBlockCount] = {};
    uint16_t word_storage[kMaxBlockPoints] = {};
    uint16_t word_off = 0;
    for (uint8_t i = 0; i < wbc; ++i) {
        if (token_count <= idx + 1) { Serial.println(F("wblk: word block incomplete")); return; }
        slmp::DeviceAddress dev = {};
        uint16_t pts = 0;
        if (!parseDeviceAddress(tokens[idx++], dev)) { Serial.println(F("wblk: word device parse failed")); return; }
        if (!parsePositiveCount(tokens[idx++], static_cast<uint16_t>(kMaxBlockPoints), pts) ||
            word_off + pts > kMaxBlockPoints) {
            Serial.println(F("wblk: total word points <= 64")); return;
        }
        if (token_count < idx + pts) { Serial.println(F("wblk: word values missing")); return; }
        wb[i] = {dev, word_storage + word_off, pts};
        for (uint16_t j = 0; j < pts; ++j) {
            if (!parseWordValue(tokens[idx++], word_storage[word_off + j])) {
                Serial.println(F("wblk: word value must fit 16 bits")); return;
            }
        }
        word_off = static_cast<uint16_t>(word_off + pts);
    }
    if (token_count <= idx) { Serial.println(F("wblk: bit_blocks count missing")); return; }
    uint8_t bbc = 0;
    if (!parseListCount(tokens[idx++], kMaxBlockCount, bbc)) {
        Serial.println(F("wblk: bit_blocks must be 0..8")); return;
    }
    if (wbc == 0 && bbc == 0) { Serial.println(F("wblk: need at least one block")); return; }
    slmp::DeviceBlockWrite bb[kMaxBlockCount] = {};
    uint16_t bit_storage[kMaxBlockPoints] = {};
    uint16_t bit_off = 0;
    for (uint8_t i = 0; i < bbc; ++i) {
        if (token_count <= idx + 1) { Serial.println(F("wblk: bit block incomplete")); return; }
        slmp::DeviceAddress dev = {};
        uint16_t pts = 0;
        if (!parseDeviceAddress(tokens[idx++], dev)) { Serial.println(F("wblk: bit device parse failed")); return; }
        if (!parsePositiveCount(tokens[idx++], static_cast<uint16_t>(kMaxBlockPoints), pts) ||
            bit_off + pts > kMaxBlockPoints) {
            Serial.println(F("wblk: total bit points <= 64")); return;
        }
        if (token_count < idx + pts) { Serial.println(F("wblk: bit packed values missing")); return; }
        bb[i] = {dev, bit_storage + bit_off, pts};
        for (uint16_t j = 0; j < pts; ++j) {
            if (!parseWordValue(tokens[idx++], bit_storage[bit_off + j])) {
                Serial.println(F("wblk: packed bit value must fit 16 bits")); return;
            }
        }
        bit_off = static_cast<uint16_t>(bit_off + pts);
    }
    if (token_count != idx) { Serial.println(F("wblk usage: wblk <wb> <dev pts vals..> <bb> <dev pts packed..>")); return; }
    if (!connectPlc(false)) { Serial.println(F("wblk: not connected")); return; }
    if (plc.writeBlock(wb, wbc, bb, bbc) != slmp::Error::Ok) { printLastPlcError("writeBlock"); return; }
    Serial.println(F("ok"));
}

// ---------------------------------------------------------------------------
// Remote password
// ---------------------------------------------------------------------------
void remotePasswordUnlockCommand(char* pw) {
    if (!pw) { Serial.println(F("unlock usage: unlock <password>")); return; }
    if (!connectPlc(false)) { Serial.println(F("unlock: not connected")); return; }
    if (plc.remotePasswordUnlock(pw) != slmp::Error::Ok) { printLastPlcError("remotePasswordUnlock"); return; }
    Serial.println(F("unlock ok"));
}
void remotePasswordLockCommand(char* pw) {
    if (!pw) { Serial.println(F("lock usage: lock <password>")); return; }
    if (!connectPlc(false)) { Serial.println(F("lock: not connected")); return; }
    if (plc.remotePasswordLock(pw) != slmp::Error::Ok) { printLastPlcError("remotePasswordLock"); return; }
    Serial.println(F("lock ok"));
}

// ---------------------------------------------------------------------------
// Verification commands
// ---------------------------------------------------------------------------
void verifyWordsCommand(char* tokens[], int token_count) {
    if (token_count < 3) { Serial.println(F("verifyw usage: verifyw <device> <value...>")); return; }
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(tokens[1], device)) { Serial.println(F("verifyw: device parse failed")); return; }
    const int vc = token_count - 2;
    if (vc <= 0 || vc > static_cast<int>(kMaxPoints)) {
        Serial.print(F("verifyw: 1..")); Serial.print(kMaxPoints); Serial.println(F(" values")); return;
    }
    uint16_t values[kMaxPoints] = {};
    for (int i = 0; i < vc; ++i) {
        if (!parseWordValue(tokens[i + 2], values[i])) { Serial.println(F("verifyw: value must fit 16 bits")); return; }
    }
    if (!connectPlc(false)) { Serial.println(F("verifyw: not connected")); return; }

    uint16_t before[kMaxPoints]   = {};
    uint16_t readback[kMaxPoints] = {};
    if (plc.readWords(device, static_cast<uint16_t>(vc), before, kMaxPoints) != slmp::Error::Ok) {
        printLastPlcError("verifyw before read"); return;
    }
    if (plc.writeWords(device, values, static_cast<size_t>(vc)) != slmp::Error::Ok) {
        printLastPlcError("verifyw write"); return;
    }
    if (plc.readWords(device, static_cast<uint16_t>(vc), readback, kMaxPoints) != slmp::Error::Ok) {
        printLastPlcError("verifyw readback"); return;
    }

    resetVerificationRecord();
    verification.active = true;
    verification.kind   = VerificationKind::WordWrite;
    verification.device = device;
    verification.points = static_cast<uint16_t>(vc);
    verification.started_ms = millis();
    memcpy(verification.before_words,   before,   sizeof(uint16_t) * static_cast<size_t>(vc));
    memcpy(verification.written_words,  values,   sizeof(uint16_t) * static_cast<size_t>(vc));
    memcpy(verification.readback_words, readback, sizeof(uint16_t) * static_cast<size_t>(vc));
    verification.readback_match = wordArraysEqual(values, readback, static_cast<uint16_t>(vc));
    printVerificationSummary();
}

void verifyBitCommand(char* device_token, char* value_token) {
    slmp::DeviceAddress device = {};
    if (!parseDeviceAddress(device_token, device)) { Serial.println(F("verifyb usage: verifyb <device> <0|1>")); return; }
    bool value = false;
    if (!parseBoolValue(value_token, value)) { Serial.println(F("verifyb: use 0/1/on/off")); return; }
    if (!connectPlc(false)) { Serial.println(F("verifyb: not connected")); return; }

    bool before = false, readback = false;
    if (plc.readOneBit(device, before) != slmp::Error::Ok) { printLastPlcError("verifyb before read"); return; }
    if (plc.writeOneBit(device, value) != slmp::Error::Ok) { printLastPlcError("verifyb write"); return; }
    if (plc.readOneBit(device, readback) != slmp::Error::Ok) { printLastPlcError("verifyb readback"); return; }

    resetVerificationRecord();
    verification.active       = true;
    verification.kind         = VerificationKind::BitWrite;
    verification.device       = device;
    verification.points       = 1;
    verification.started_ms   = millis();
    verification.before_bit   = before;
    verification.written_bit  = value;
    verification.readback_bit = readback;
    verification.readback_match = (value == readback);
    printVerificationSummary();
}

void judgeCommand(char* tokens[], int token_count) {
    if (!verification.active) { Serial.println(F("judge: no active verification")); return; }
    if (token_count < 2) { Serial.println(F("judge usage: judge <ok|ng> [note]")); return; }
    uppercaseInPlace(tokens[1]);
    if (strcmp(tokens[1], "OK") == 0 || strcmp(tokens[1], "PASS") == 0) {
        verification.pass = true;
    } else if (strcmp(tokens[1], "NG") == 0 || strcmp(tokens[1], "FAIL") == 0) {
        verification.pass = false;
    } else {
        Serial.println(F("judge usage: judge <ok|ng> [note]")); return;
    }
    verification.judged = true;
    joinTokens(tokens, 2, token_count, verification.note, sizeof(verification.note));
    printVerificationSummary();
}
