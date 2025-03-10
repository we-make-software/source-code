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

typedef bool(*ThePostOfficeReceivePacketCallback)(u16 SourcePort,u16 DestinationPort,void*ConnectionIdentifier,u8**data);
static ThePostOfficeReceivePacketCallback ThePostOfficeReceivePacketCallbackFunction=NULL;
typedef bool (*ThePostOfficeRegisterPacketCallback)(
    struct net_device *dev, 
    bool IsVersion6, 
    const u8 *data, 
    u8 **ToNetworkAddress, 
    u8 **FromNetworkAddress,
    void**ConnectionIdentifier
);

static ThePostOfficeRegisterPacketCallback ThePostOfficeRegisterPacketCallbackFunction=NULL;
static int ThePostOfficeReceivePacket(struct sk_buff* skb, struct net_device* dev, struct packet_type* pt, struct net_device* orig_dev) {
    if(!ThePostOfficeRegisterPacketCallbackFunction||!ThePostOfficeReceivePacketCallbackFunction||strcmp(dev->name,"lo")==0||skb->len<14||skb->pkt_type==PACKET_OUTGOING)
    return 0; 
    u8*data;
    data=skb_mac_header(skb);
    if ((data[0]&2))return 0;

    u16 ethertype = ntohs(*(u16 *)(data + 12)),
        SourcePort = 0,
        DestinationPort = 0;
    bool IsVersion6 = false,
         IsTransmissionControlProtocol=false;
    u8 *ToNetworkAddress=NULL,
       *FromNetworkAddress=NULL; 
    switch (ethertype)
    {
        case 2048:{
            if(data[14]!=69)goto ForceExit;
            SourcePort = ntohs(*(u16 *)(data + 36)); 
            if((IsTransmissionControlProtocol=(data[23]==6))&&SourcePort==22)
                return 0;
                DestinationPort =  ntohs(*(u16 *)(data + 34));
            if(ToNetworkAddress=(u8*)kmalloc(4,GFP_KERNEL))
                memcpy(ToNetworkAddress,data+26,4);
            else goto ForceExit;  
            if(FromNetworkAddress=(u8*)kmalloc(4,GFP_KERNEL))
                memcpy(FromNetworkAddress,data+30,4);
            else goto ForceExit;  
            break;
        }
        case 34525:{
            if ((data[22] == 254 && (data[23] & 192) == 128) || 
                (data[38] == 254 && (data[39] & 192) == 128)) 
                return 0;
            IsVersion6 = true;
            if(ToNetworkAddress=(u8*)kmalloc(16,GFP_KERNEL))
                memcpy(ToNetworkAddress,data+22,16);
            else goto ForceExit;  
            if(FromNetworkAddress=(u8*)kmalloc(16,GFP_KERNEL))
                memcpy(FromNetworkAddress,data+38,16);
            else goto ForceExit;  
            SourcePort = ntohs(*(u16 *)(data + 56));
            if((IsTransmissionControlProtocol=(data[20]==6))&&SourcePort==22)
                return 0; 
            DestinationPort =ntohs(*(u16 *)(data + 54)); 
            break;   
        }
        default:
            return 0;
    }
    void*ConnectionIdentifier=NULL;
    if(!ThePostOfficeRegisterPacketCallbackFunction(dev,IsVersion6,data,&ToNetworkAddress,&FromNetworkAddress,&ConnectionIdentifier))
        goto ForceExit;
        u8* newdata = (u8*)kmalloc(skb->len - (14 + (IsVersion6 ? 37 : 16)), GFP_KERNEL);
        if (!newdata) goto ForceExit;
        if (IsVersion6) {
            memcpy(newdata, data + 14, 6);
            memcpy(newdata + 6, data + 21, 1); 
            memcpy(newdata + 7, data + 58, skb->len - 58); 
        } else {
            memcpy(newdata, data + 15, 8);
            memcpy(newdata + 8, data + 38, skb->len - 38);
        }
        
    kfree_skb(skb);
    if(ThePostOfficeReceivePacketCallbackFunction(&newdata)) return 1;
    ForceExit:
    if(ToNetworkAddress)kfree(ToNetworkAddress);
    if(FromNetworkAddress)kfree(FromNetworkAddress);
    if(ConnectionIdentifier)kfree(ConnectionIdentifier);
    kfree_skb(skb);
    return 1;
}


Setup("ThePostOffice", 
    ThePostOfficePacketType.type=htons(ETH_P_ALL);
    ThePostOfficePacketType.func=ThePostOfficeReceivePacket;
    dev_add_pack(&ThePostOfficePacketType);,
    dev_remove_pack(&ThePostOfficePacketType);
    )