#include "../ExpiryWorkBase/ExpiryWorkBase.h"
#include <linux/netdevice.h>
#include <linux/if_packet.h>
#include <linux/net.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>
struct NetworkAdapterTable {
    SetupEWB;
    struct list_head routers,list;
    struct net_device*dev;
};
struct RouterTable{
    u8 MediaAccessControl[6];
    SetupEWB;
    struct NetworkAdapterTable*nat;
    struct list_head list;
};

static DEFINE_MUTEX(NATMutex);
static LIST_HEAD(NATList);
static struct NetworkAdapterTable*GetNetworkAdapter(struct net_device*dev);
static struct NetworkAdapterTable*GetNetworkAdapter(struct net_device*dev){
    struct NetworkAdapterTable*nat;
    if(list_empty(&NATList))return NULL;
    list_for_each_entry(nat,&NATList,list)
    if(GetExpiryWorkBaseParent(nat->ewb)||nat->dev==dev)
        return nat;
    return NULL;
}
static void AutoDeleteNetworkAdapter(void*adapter){
	struct NetworkAdapterTable*nat=(struct NetworkAdapterTable*)adapter;
	mutex_lock(&NATMutex);
	struct RouterTable*router,*tmp;
	list_for_each_entry_safe(router,tmp,&nat->routers,list)
        CancelExpiryWorkBase(&router->ewb);
	mutex_unlock(&NATMutex);
}
static struct NetworkAdapterTable*AddNetworkAdapter(struct net_device*dev);
static struct NetworkAdapterTable*AddNetworkAdapter(struct net_device*dev){
    struct NetworkAdapterTable*nat=GetNetworkAdapter(dev);
    if(nat)return nat;
    mutex_lock(&NATMutex);
    nat=GetNetworkAdapter(dev);
    if(nat){
        mutex_unlock(&NATMutex);
        return nat;
    }
	nat=kmalloc(sizeof(struct NetworkAdapterTable),GFP_KERNEL);
	if(!nat){
        mutex_unlock(&NATMutex);
        return NULL;
    }
	nat->dev=dev;
	INIT_LIST_HEAD(&nat->list);
    INIT_LIST_HEAD(&nat->routers);
	SetupExpiryWorkBase(&nat->ewb,NULL,nat,AutoDeleteNetworkAdapter);
	list_add(&nat->list,&NATList);
	mutex_unlock(&NATMutex);
	return nat;
}
struct PacketConversion{
    u8 reverse[(3*sizeof(void*))+2]; 
    u8*data;
    u16 SourcePort;
    bool IsTransmissionControlProtocol;
    bool IsVersion6;
    struct sk_buff *skb;
    struct net_device *dev;
    struct work_struct work;
};
///extern void TheMailConditionerPacketWorkHandler(struct NetworkAdapterTable*,struct PacketConversion*);
static void PacketConversionTask(struct PacketConversion*pw){
    struct NetworkAdapterTable*nat=AddNetworkAdapter(pw->dev);
    if(!nat||nat->ewb.Invalid)return;
   // TheMailConditionerPacketWorkHandler(nat,pw);
}
static void PacketWorkHandler(struct work_struct *work) {
    struct PacketConversion *packet_work = container_of(work, struct PacketConversion, work);
    PacketConversionTask(packet_work);
    kfree_skb(packet_work->skb);
    kfree(packet_work); 
}
static int ThePostOfficeReceivePacket(struct sk_buff*skb,struct net_device*dev,struct packet_type*pt,struct net_device*orig_dev){
	if(!strcmp(dev->name,"lo")||skb->len<14||skb->pkt_type==PACKET_OUTGOING)return 0;
	u8*data=skb_mac_header(skb);
	if((data[0]&2))return 0;
	u16 SourcePort;
	bool IsTransmissionControlProtocol;
	bool IsVersion6=false;
	u16 ethertype=ntohs(*(u16*)(data+12));
	switch(ethertype){
		case 2048:{
			if(data[14]!=69){
				kfree_skb(skb);
				return 1;
			}
			SourcePort=ntohs(*(u16*)(data+36));
			if((IsTransmissionControlProtocol=(data[23]==6))&&SourcePort==22)return 0;
			break;
		}
		case 34525:{
			IsVersion6=true;
			SourcePort=ntohs(*(u16*)(data+56));
			if((data[22]==254&&(data[23]&192)==128)||(data[38]==254&&(data[39]&192)==128))return 0;
			if((IsTransmissionControlProtocol=(data[20]==6))&&SourcePort==22)return 0;
			break;
		}
		default:return 0;
	}
	struct PacketConversion*pc=kmalloc(sizeof(struct PacketConversion),GFP_KERNEL);
	if(!pc){
		kfree_skb(skb);
		return 1;
	}
	pc->SourcePort=SourcePort;
	pc->IsTransmissionControlProtocol=IsTransmissionControlProtocol;
	pc->IsVersion6=IsVersion6;
	pc->skb=skb;
    pc->data=data+6;
	pc->dev=dev;
	INIT_WORK(&pc->work,PacketWorkHandler);
	queue_work(system_bh_highpri_wq ,&pc->work);
	return 1;
}
static void CancelAllNetworkAdapters(void){
    struct NetworkAdapterTable*nat, *tmp;
    list_for_each_entry_safe(nat, tmp, &NATList, list)
        CancelExpiryWorkBase(&nat->ewb);
}
static struct packet_type ThePostOfficePacketType;
static void BindNetworkAdapterToTheProject(void){
    ThePostOfficePacketType.type=htons(ETH_P_ALL);
    ThePostOfficePacketType.func=ThePostOfficeReceivePacket;
    dev_add_pack(&ThePostOfficePacketType);
}
static void UnbindNetworkAdapterToTheProject(void){
    dev_remove_pack(&ThePostOfficePacketType);
}
static void Layer0Start(void){
    BindNetworkAdapterToTheProject();
}
static void Layer0End(void){
    UnbindNetworkAdapterToTheProject();
    CancelAllNetworkAdapters();
}
Layer0Setup("ThePostOffice",0,0)


/*

bool Send(struct PacketConfiguration*pc,u16 size,u8*data){

    if(size<34||!pc||!pc->rt||pc->rt->ewb.Invalid||!pc->server||!pc->client||pc->server->ewb.Invalid||pc->client->ewb.Invalid||pc->client->IsVersion6!=pc->server->IsVersion6||(pc->server->IsVersion6&&size<54))return false;

    struct sk_buff *skb;
    skb=netdev_alloc_skb(pc->rt->nat,size+NET_IP_ALIGN);
    if(!skb)return false;
    skb_reserve(skb,NET_IP_ALIGN);
    skb_put_data(skb,pc->rt->MediaAccessControl,6);
    skb_put_data(skb,pc->rt->nat->dev->dev_addr,6);
    skb_put_u16(skb,htons(pc->server->IsVersion6?34525:2048));
    skb_put_u16(skb, htons(pc->TrafficClass+(pc->server->IsVersion6?24576:16384)));
        //there some data here wee need to place before wee can send
    if(pc->server->IsVersion6){
     
        //there some data here wee need to place before wee can send
        skb_put_data(skb,pc->client->Address,16);
        skb_put_data(skb,pc->server->Address,16);
    }
    else{
     
        //there some data here wee need to place before wee can send
        skb_put_data(skb,pc->client->Address,4);
        skb_put_data(skb,pc->server->Address,4);
    }
    skb_put_data(skb,data,size); 
    skb->dev=pc->rt->nat->dev;
    skb->protocol=htons(ETH_P_IP);
    skb->priority=0;
    if(dev_queue_xmit(skb)<0){
        kfree_skb(skb);
        return false;
    }
    return true;
}
*/