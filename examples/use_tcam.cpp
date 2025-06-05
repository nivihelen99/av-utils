#include "tcam.h"
#include <iostream>
#include <vector>

// Helper function to create a packet (vector<uint8_t>)
std::vector<uint8_t> make_example_packet(uint32_t src_ip, uint32_t dst_ip,
                                         uint16_t src_port, uint16_t dst_port,
                                         uint8_t proto, uint16_t eth_type) {
    std::vector<uint8_t> p(15);
    p[0] = (src_ip >> 24) & 0xFF; p[1] = (src_ip >> 16) & 0xFF; p[2] = (src_ip >> 8) & 0xFF; p[3] = src_ip & 0xFF;
    p[4] = (dst_ip >> 24) & 0xFF; p[5] = (dst_ip >> 16) & 0xFF; p[6] = (dst_ip >> 8) & 0xFF; p[7] = dst_ip & 0xFF;
    p[8] = (src_port >> 8) & 0xFF; p[9] = src_port & 0xFF;
    p[10] = (dst_port >> 8) & 0xFF; p[11] = dst_port & 0xFF;
    p[12] = proto;
    p[13] = (eth_type >> 8) & 0xFF; p[14] = eth_type & 0xFF;
    return p;
}

int main() {
    OptimizedTCAM my_tcam;

    OptimizedTCAM::WildcardFields fields1{};
    fields1.src_ip = 0x0A000001; fields1.src_ip_mask = 0xFFFFFFFF;
    fields1.dst_ip = 0xC0A80001; fields1.dst_ip_mask = 0xFFFFFFFF;
    fields1.src_port_min = 1024; fields1.src_port_max = 1024;
    fields1.dst_port_min = 80; fields1.dst_port_max = 80;
    fields1.protocol = 6; fields1.protocol_mask = 0xFF;
    fields1.eth_type = 0x0800; fields1.eth_type_mask = 0xFFFF;

    my_tcam.add_rule_with_ranges(fields1, 100, 1);

    auto packet = make_example_packet(0x0A000001, 0xC0A80001, 1024, 80, 6, 0x0800);
    int action = my_tcam.lookup_linear(packet);

    if (action != -1) {
        std::cout << "Packet matched action: " << action << std::endl;
    } else {
        std::cout << "Packet did not match any rule." << std::endl;
    }

    std::cout << "OptimizedTCAM example usage complete." << std::endl;
    return 0;
}
