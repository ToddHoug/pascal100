#include "boards/compupro-selch.h"

#include "core/bus.h"
#include "core/statefile.h"
#include "core/value.h"

namespace altair {

namespace {

constexpr uint8_t kModeWrite = 0x80;  // D7 W/R*: 1 = write to the slave
constexpr uint8_t kModeIo    = 0x40;  // D6 IO/M*: 1 = I/O cycles
constexpr uint8_t kModeUp    = 0x20;  // D5 U/D*: 1 = count up

} // namespace

// The comparator U26 sees A7-A0 and the I/O status (logic sheet 1). Only an OUT is decoded:
// an IN clears the write counter and drives nothing (p. 33), so snoop() takes it.
bool SelectorChannelBoard::decodes(const BusCycle& c) const {
    return enabled_ && c.type == Cycle::IoWrite && c.port() == port_;
}

void SelectorChannelBoard::write(const BusCycle& c) {
    if (c.type != Cycle::IoWrite) return;
    // LD-STB shifts the four stages on every OUT (p. 33).
    addr_ = ((addr_ << 8) & 0xFFFF00u) | mode_;
    mode_ = c.data;
    if (writes_ < 4 && ++writes_ == 4) armed_ = true;
}

void SelectorChannelBoard::snoop(const BusCycle& c) {
    if (!enabled_ || c.type != Cycle::IoRead || c.port() != port_) return;
    writes_ = 0;
    armed_  = false;
}

// RESET* clears U10 (p. 33): disabled until the next four OUTs. The registers keep their
// contents; they are reloaded before use anyway.
void SelectorChannelBoard::reset(Reset r) {
    writes_ = 0;
    armed_  = false;
    if (r == Reset::PowerOn) {
        addr_ = 0;
        mode_ = 0;
    }
}

bool SelectorChannelBoard::serves(int priority) const {
    return enabled_ && armed_ && ((~mode_ & 0x0F) == (priority & 0x0F));
}

uint8_t SelectorChannelBoard::cycle(Bus& bus, uint8_t out) {
    const bool write = (mode_ & kModeWrite) != 0;
    uint8_t    in    = kFloating;
    if (mode_ & kModeIo) {
        // I/O addresses are not counted (p. 32). A0-A15 go out, as on any I/O cycle.
        if (write) bus.ioWrite((uint16_t)addr_, out);
        else       in = bus.ioRead((uint16_t)addr_);
        return in;
    }
    if (write) bus.memWriteX(addr_, out);
    else       in = bus.memReadX(addr_);
    addr_ = (mode_ & kModeUp) ? ((addr_ + 1) & 0xFFFFFFu) : ((addr_ - 1) & 0xFFFFFFu);
    return in;
}

std::vector<Property> SelectorChannelBoard::properties() {
    std::vector<Property> p;
    Property x;
    x.name  = "port";
    x.help  = "The board's one I/O port (SW1-3..10 = A7..A0; default F0, CompuPro's). "
              "OUT shifts in A23-A16, A15-A8, A7-A0, MODE; IN disables the board";
    x.kind  = Kind::Int;
    x.radix = 16;
    x.min   = 0;
    x.max   = 0xFF;
    x.get   = [this] { return Value::ofInt(port_); };
    x.set   = [this](const Value& v, std::string&) {
        port_ = (uint8_t)v.i();
        decodeChanged();
        return true;
    };
    p.push_back(std::move(x));
    return p;
}

std::vector<MapEntry> SelectorChannelBoard::ioMap() const {
    return {{(uint32_t)port_, (uint32_t)port_, "read/write",
             "A23-A16, A15-A8, A7-A0, MODE (out, four in a row); an IN disables the board, no data"}};
}

void SelectorChannelBoard::serialize(StateWriter& w) const {
    Board::serialize(w);
    w.u32(addr_);
    w.u8(mode_);
    w.u8((uint8_t)writes_);
    w.boolean(armed_);
}

void SelectorChannelBoard::deserialize(StateReader& r) {
    Board::deserialize(r);
    addr_   = r.u32() & 0xFFFFFFu;
    mode_   = r.u8();
    writes_ = r.u8() & 7;
    armed_  = r.boolean();
}

} // namespace altair
