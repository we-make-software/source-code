#include "ThePostOffice.h"
#include <linux/netdevice.h>
#include <linux/if_packet.h>
#include <linux/net.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>
static struct packet_type ThePostOfficePacketType;
#define U48_To_U64(value) ((u64)(value[0])<<40 | (u64)(value[1])<<32 | (u64)(value[2])<<24 | (u64)(value[3])<<16 | (u64)(value[4])<<8 | (u64)(value[5]))


static int ThePostOfficeReceivePacket(struct sk_buff*skb,struct net_device*dev,struct packet_type*pt,struct net_device*orig_dev){
    if(!strcmp(dev->name,"lo")==0||skb->len<14||skb->pkt_type==PACKET_OUTGOING)
    return 0;
    u8*data;
    data=skb_mac_header(skb);
    if((data[0]&2))return 0;
    bool IsVersion6=false,IsTransmissionControlProtocol=false;
    u16 ethertype=ntohs(*(u16*)(data+12)),SourcePort=0,
        DestinationPort=0;
    //Prepare the Neworkcard and the router
    switch(ethertype){
        case 2048:{
            if(data[14]!=69){
                kfree_skb(skb);
                return 1;
            }
            SourcePort=ntohs(*(u16*)(data+36));
            if((IsTransmissionControlProtocol=(data[23]==6))&&SourcePort==22)return 0;
            u16 ToNetworkAddressVersion_0=ntohs(*(u16*)(data+26)),
                ToNetworkAddressVersion_1=ntohs(*(u16*)(data+28)),
                FromNetworkAddressVersion_0=ntohs(*(u16*)(data+ 30)),
                FromNetworkAddressVersion_1=ntohs(*(u16*)(data + 32));
       
            DestinationPort=ntohs(*(u16*)(data+34));    
            break;
        }
        case 34525:{
            SourcePort=ntohs(*(u16*)(data+56));
            if((data[22]==254&&(data[23]&192)==128)||(data[38]==254&&(data[39]&192)==128))return 0;
            if((IsTransmissionControlProtocol=(data[20]==6))&&SourcePort==22)return 0;
            IsVersion6=true;
            u64 ToNetworkAddressVersion_0=be64_to_cpu(*(u64*)(data+22)),
                ToNetworkAddressVersion_1=be64_to_cpu(*(u64*)(data+30)),
                FromNetworkAddressVersion_0=be64_to_cpu(*(u64*)(data+38)),
                FromNetworkAddressVersion_1=be64_to_cpu(*(u64*)(data+46));
        
            
            DestinationPort=ntohs(*(u16*)(data+54));
            break;
        }
        default:return 0;
    }
    

    kfree_skb(skb);
    return 1;
}
/*
bool ThePostOfficeSendPacket(struct IEEE8021Router* router){
    struct sk_buff *skb;
    skb=netdev_alloc_skb(router->NetworkInterfaces->dev,14+NET_IP_ALIGN);
    if(!skb)return false;
    skb_reserve(skb,NET_IP_ALIGN);
    skb_put_data(skb,router->MediaAccessControl,6);
    skb_put_data(skb,router->NetworkInterfaces->dev->dev_addr,6); 
    u16 Ethertype=htons(router->IsVersion6?34525:2048); 
    skb_put_data(skb,&Ethertype,2); 
    skb->dev=router->NetworkInterfaces->dev;
    skb->protocol=htons(ETH_P_IP);
    skb->priority=0;
    if(dev_queue_xmit(skb)<0){
        kfree_skb(skb);
        return false;
    }
    return true;
}
*/
Setup("ThePostOffice", 
    ThePostOfficePacketType.type=htons(ETH_P_ALL);
    ThePostOfficePacketType.func=ThePostOfficeReceivePacket;
    dev_add_pack(&ThePostOfficePacketType);,
    dev_remove_pack(&ThePostOfficePacketType);
    )