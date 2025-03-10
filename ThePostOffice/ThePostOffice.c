#include "ThePostOffice.h"
#include <linux/netdevice.h>
#include <linux/if_packet.h>
#include <linux/net.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>
static struct packet_type ThePostOfficePacketType;
bool ThePostOfficeSendPacket(struct net_device *dev, u8 *packet, u16 packet_len) {
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
EXPORT_SYMBOL(ThePostOfficeSendPacket);

typedef void (*ThePostOfficeReceivePacketCallback)(u16 NetworkID,u8*data,u16 data_len);
static ThePostOfficeReceivePacketCallback ThePostOfficeReceivePacketCallbackFunction=NULL;
typedef u16(*ThePostOfficeRegisterPacketCallback)(bool IsVersion6,struct sk_buff*skb,struct net_device*dev);
static ThePostOfficeRegisterPacketCallback ThePostOfficeRegisterPacketCallbackFunction=NULL;
static int ThePostOfficeReceivePacket(struct sk_buff* skb, struct net_device* dev, struct packet_type* pt, struct net_device* orig_dev) {
    if(
        //!ThePostOfficeRegisterPacketCallbackFunction||!ThePostOfficeReceivePacketCallbackFunction||
        strcmp(dev->name,"lo")==0||skb->len<14)
        return 0; 
    u8*data;
    data=skb_mac_header(skb);
    if(data[0]&2)return 0;
    u16 ethertype = ntohs(*(u16 *)(data + 12)),
        SourcePort=0,
        DestinationPort=0;
    bool IsVersion6=false,
        DropPacket=false,
        IsTransmissionControlProtocol=false;
    switch (ethertype)
    {
        case 2048:{
            if(data[15]!=69){
                kfree_skb(skb);
                return 1;
            }
            SourcePort = ntohs(*(u16 *)(data + 34));  
            if((IsTransmissionControlProtocol=(data[23]==6))&&SourcePort==22)return 0;
            DestinationPort = ntohs(*(u16 *)(data + 36));  
            break;
        }
        case 34525:{
            IsVersion6=true;
            SourcePort = ntohs(*(u16 *)(data + 54));
            if((IsTransmissionControlProtocol=(data[20]==6))&&SourcePort==22)return 0; 
            DestinationPort = ntohs(*(u16 *)(data + 56));
            break;   
        }
        default:
            return 0;
    }
    if(DropPacket){
        kfree_skb(skb);
        return 1;
    }
    //ThePostOfficeReceivePacketCallbackFunction(ThePostOfficeRegisterPacketCallbackFunction(IsVersion6,skb,dev),data,skb->len);
    return 0;
}


Setup("ThePostOffice", 
    ThePostOfficePacketType.type=htons(ETH_P_ALL);
    ThePostOfficePacketType.func=ThePostOfficeReceivePacket;
    dev_add_pack(&ThePostOfficePacketType);,
    dev_remove_pack(&ThePostOfficePacketType);
    )