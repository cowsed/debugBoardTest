#include "vdb/protocol.h"

namespace VDP {
static const Channel ControlChannel{0, nullptr};

class Registry {
public:
  using CallbackFn = std::function<void(const VDP::Channel &)>;
  Registry();

  ChannelID new_channel_id() {
    next_channel_id++;
    return next_channel_id;
  }
  /// @brief Call this if you are a device who has a packet for the protocol to
  /// decode
  /// @param pac the packet to take.
  void take_packet(const Packet &pac);

  PartPtr get_remote_schema(ChannelID id);

  void install_broadcast_callback(CallbackFn on_broadcast);
  void install_data_callback(CallbackFn on_data);

private:
  std::vector<Channel> my_channels = {ControlChannel};
  std::vector<Channel> remote_channels = {ControlChannel};
  ChannelID next_channel_id = 0;

  CallbackFn on_broadcast = [](VDP::Channel) {
    printf("No Broadcast Callback installed\n");
  };
  CallbackFn on_data = [](VDP::Channel) {
    printf("No Data Callback installed\n");
  };
};
} // namespace VDP