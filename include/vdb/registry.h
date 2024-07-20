#include "vdb/protocol.h"

namespace VDP {
static const Channel ControlChannel{0, nullptr};

class Registry {
public:
  using CallbackFn = std::function<void(VDP::Channel)>;

  ChannelID new_channel_id() {
    next_channel_id++;
    return next_channel_id;
  }
  /// @brief Call this if you are a device who has a packet for the protocol to
  /// decode
  /// @param pac the packet to take.
  void take_packet(const Packet &pac);

  void install_broadcast_callback(CallbackFn on_broadcast);
  void install_data_callback(CallbackFn on_data);

private:
  std::vector<Channel> my_channels = {ControlChannel};
  std::vector<Channel> remote_channles = {ControlChannel};
  ChannelID next_channel_id = 0;

  CallbackFn on_broadcast = [](VDP::Channel c) {
    printf("No Broadcast Callback installed\n");
    fflush(stdout);
    vexDelay(500);
  };
  CallbackFn on_data = [](VDP::Channel c) {
    printf("No Data Callback installed\n");
    fflush(stdout);
    vexDelay(500);
  };
};
} // namespace VDP