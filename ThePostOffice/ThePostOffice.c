#include "ThePostOffice.h"
#include <linux/netdevice.h>
#include <linux/if_packet.h>
#include <linux/net.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>
static struct packet_type ThePostOfficePacketType;
static int ThePostOfficeReceivePacket(struct sk_buff*skb, struct net_device*dev, struct packet_type*pt, struct net_device*orig_dev){
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
            tcp=(struct tcphdr *)(skb_transport_header(skb));
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