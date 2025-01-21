#pragma once

#include "esphome/core/component.h"
#include "remote_base.h"

namespace esphome {
namespace remote_base {

// Toshiba AC 72 bit protocol
static const uint8_t N_BITS = 64;
static const uint8_t N_CHECKSUM_BITS = 8;

struct ToshibaAc72BitData {
  uint64_t data;
  uint8_t nbits;
  uint8_t checksum; 

  bool operator==(const ToshibaAc72BitData &rhs) const { return data == rhs.data && nbits == rhs.nbits; }
};

class ToshibaAc72BitProtocol : public RemoteProtocol<ToshibaAc72BitData> {
 public:
  void encode(RemoteTransmitData *dst, const ToshibaAc72BitData &data) override;
  optional<ToshibaAc72BitData> decode(RemoteReceiveData src) override;
  void dump(const ToshibaAc72BitData &data) override;
  uint8_t get_xor8_checksum(uint32_t cmd_data);
};

DECLARE_REMOTE_PROTOCOL(ToshibaAc72Bit)

template<typename... Ts> class ToshibaAc72BitAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(uint64_t, data)

  void encode(RemoteTransmitData *dst, Ts... x) override {
    ToshibaAc72BitData data{};
    data.data = this->data_.value(x...);
    data.nbits = N_BITS;
    data.checksum = ToshibaAc72BitProtocol().get_xor8_checksum((uint64_t)data.data & 0xFFFFFFFF);
    ToshibaAc72BitProtocol().encode(dst, data);
  }
};

}  // namespace remote_base
}  // namespace esphome
