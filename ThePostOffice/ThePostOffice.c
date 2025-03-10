#include "ThePostOffice.h"
#include <linux/netdevice.h>
#include <linux/if_packet.h>
#include <linux/net.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>
static struct packet_type ThePostOfficePacketType;

#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>

static bool ThePostOfficeSendPacket(struct net_device *dev, u8 *packet, u16 packet_len) {
    struct sk_buff*skb;
    skb=netdev_alloc_skb(dev,packet_len+NET_IP_ALIGN);
    if(!skb)return false; 
    skb_reserve(skb,NET_IP_ALIGN); 
    skb_put_data(skb,packet,packet_len); 
    skb->dev=dev;
    skb->protocol=htons(ETH_P_IP); 
    skb->priority=0; 
    if(dev_queue_xmit(skb)<0){
        kfree_skb(skb); 
        return false;
    }
    return true; 
}


static void ThePostOfficeSendPacket(struct net_device *dev, u8 *packet, u16 packet_len) {
    struct sk_buff *skb;

    // Allocate sk_buff (metadata + linear buffer)
    skb = netdev_alloc_skb(dev, packet_len + NET_IP_ALIGN);
    if (!skb) return;

    skb_reserve(skb, NET_IP_ALIGN); // Ensure proper alignment
    skb_put_data(skb, packet, packet_len); // Copy packet into sk_buff

    skb->dev = dev;  // Assign network device
    skb->protocol = htons(ETH_P_IP);  // Set protocol
    skb->priority = 0;  // Normal priority
    dev_queue_xmit(skb);
}

static int ThePostOfficeReceivePacket(struct sk_buff*skb,struct net_device*dev,struct packet_type*pt,struct net_device*orig_dev){
    struct iphdr*ip; 
    struct ipv6hdr*ip6; 
    struct tcphdr*tcp;
    if(strcmp(dev->name,"lo")==0)return 0;
    if(skb->protocol==htons(ETH_P_IP)){
        ip=ip_hdr(skb);
        if(ip->protocol==IPPROTO_TCP){
            tcp=(struct tcphdr *)((__u32 *)ip+ip->ihl);
            if(ntohs(tcp->dest)==22)return 0;
        }
    }else if(skb->protocol==htons(ETH_P_IPV6)){
        ip6=ipv6_hdr(skb);
        if(ip6->nexthdr==IPPROTO_TCP){
            tcp=(struct tcphdr*)(skb_transport_header(skb));
            if(ntohs(tcp->dest)==22)return 0;
        }
    }
    return 0;
}


Setup("ThePostOffice", 
    ThePostOfficePacketType.type=htons(ETH_P_ALL);
    ThePostOfficePacketType.func=ThePostOfficeReceivePacket;
    dev_add_pack(&ThePostOfficePacketType);,
    dev_remove_pack(&ThePostOfficePacketType);
    )