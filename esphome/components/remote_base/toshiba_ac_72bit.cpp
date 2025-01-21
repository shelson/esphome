#include "toshiba_ac_72bit.h"
#include "esphome/core/log.h"
#include <cinttypes>

namespace esphome {
namespace remote_base {

static const char *const TAG = "remote.toshiba_ac_72bit";

// This is the same timings as the samsung protocol
// However the toshiba protocol adds a checksum after the 64 bits
// of data which breaks the samsung protocol defined in this component
static const uint32_t HEADER_HIGH_US = 4500;
static const uint32_t HEADER_LOW_US = 4500;
static const uint32_t BIT_HIGH_US = 560;
static const uint32_t BIT_ONE_LOW_US = 1690;
static const uint32_t BIT_ZERO_LOW_US = 560;
static const uint32_t FOOTER_HIGH_US = 560;
static const uint32_t FOOTER_LOW_US = 560;

uint8_t ToshibaAc72BitProtocol::get_xor8_checksum(uint32_t cmd_data) {
  // we need to make cmd_data into an array of bits
  uint8_t bytes_array[4];
  *(uint32_t*)&bytes_array = cmd_data;
  uint8_t checksum = 0;
  for (uint8_t i = 0; i < 4; i++) {
    checksum ^= bytes_array[i];
  }
  return checksum;
}

void ToshibaAc72BitProtocol::encode(RemoteTransmitData *dst, const ToshibaAc72BitData &data) {
  dst->set_carrier_frequency(38000);
  // reserve the extra 2 for the checksum
  dst->reserve(4 + N_BITS * 2u + N_CHECKSUM_BITS);

  dst->item(HEADER_HIGH_US, HEADER_LOW_US);

  for (uint8_t bit = N_BITS; bit > 0; bit--) {
    if ((data.data >> (bit - 1)) & 1) {
      dst->item(BIT_HIGH_US, BIT_ONE_LOW_US);
    } else {
      dst->item(BIT_HIGH_US, BIT_ZERO_LOW_US);
    }
  }
  // Send the checksum, 8 bits
  for (uint8_t bit = N_CHECKSUM_BITS; bit > 0; bit--) {
    if ((data.data >> (bit - 1)) & 1) {
      dst->item(BIT_HIGH_US, BIT_ONE_LOW_US);
    } else {
      dst->item(BIT_HIGH_US, BIT_ZERO_LOW_US);
    }
  }

  dst->item(FOOTER_HIGH_US, FOOTER_LOW_US);
  ESP_LOGI(TAG, "Sent ToshibaAc72Bit: data=0x%" PRIX64 ", Checksum: 0x%02X", data.data, data.checksum);
}
optional<ToshibaAc72BitData> ToshibaAc72BitProtocol::decode(RemoteReceiveData src) {
  ToshibaAc72BitData out{
      .data = 0,
      .nbits = 0,
      .checksum = 0,
  };
  if (!src.expect_item(HEADER_HIGH_US, HEADER_LOW_US))
    return {};

  for (out.nbits = 0; out.nbits < 64; out.nbits++) {
    if (src.expect_item(BIT_HIGH_US, BIT_ONE_LOW_US)) {
      out.data = (out.data << 1) | 1;
    } else if (src.expect_item(BIT_HIGH_US, BIT_ZERO_LOW_US)) {
      out.data = (out.data << 1) | 0;
    }
  }

  // look for checksum
  for(uint8_t i = 0; i < 8; i++) {
    if (src.expect_item(BIT_HIGH_US, BIT_ONE_LOW_US)) {
      out.checksum = (out.checksum << 1) | 1;
    } else if (src.expect_item(BIT_HIGH_US, BIT_ZERO_LOW_US)) {
      out.checksum = (out.checksum << 1) | 0;
    }
  }
  // finally look for footer
  if (!src.expect_mark(FOOTER_HIGH_US))
    return {};
  return out;
}
void ToshibaAc72BitProtocol::dump(const ToshibaAc72BitData &data) {
  ESP_LOGI(TAG, "Received ToshibaAc72Bit: data=0x%" PRIX64 ", nbits=%d, checksum=%02x", data.data, data.nbits, data.checksum);
}

}  // namespace remote_base
}  // namespace esphome
