#include "vdb/protocol.h"

namespace VDP {
constexpr ChannelID control_channel_id = 0;
static const Channel ControlChannel{control_channel_id, nullptr};

class Registry {
public:
  using CallbackFn = std::function<void(const VDP::Channel &)>;
  Registry();
  Registry(AbstractDevice *device);
  /// @brief Call this if you are a device who has a packet for the protocol to
  /// decode
  /// @param pac the packet to take.
  void take_packet(const Packet &pac);

  PartPtr get_remote_schema(ChannelID id);

  void install_broadcast_callback(CallbackFn on_broadcast);
  void install_data_callback(CallbackFn on_data);

  // creates a new channel for the given VDP object, broadcasts it to the device
  // on the other end of the line this channel is open to be written too
  // immediately. however, it is not garaunteed to be sent to the other side
  // until broadcasting has been completed
  Channel open_channel(PartPtr for_data) {
    ChannelID id = new_channel_id();
    Channel chan = Channel{
        .id = id,
        .data = for_data,
    };

    PacketWriter writer;
    writer.write_channel_broadcast(chan);
    VDP::Packet pac = writer.get_packet();

    device->send_packet(pac);

    return chan;
  }

private:
  ChannelID new_channel_id() {
    next_channel_id++;
    return next_channel_id;
  }

  AbstractDevice *device;
  std::vector<Channel> my_channels = {ControlChannel};
  std::vector<Channel> remote_channels = {ControlChannel};
  ChannelID next_channel_id = 0;

  CallbackFn on_broadcast = [](VDP::Channel chan) {
    std::string schema_str = chan.data->pretty_print();
    printf("No Broadcast Callback installed: Received broadcast for channel id "
           "%d:\n%s\n",
           int(chan.id), schema_str.c_str());
  };
  CallbackFn on_data = [](VDP::Channel chan) {
    printf("No Data Callback installed: Received data for channel %d\n",
           int(chan.id));
  };
};
} // namespace VDP