#include "ThePostOffice.h"
#include <linux/netdevice.h>
#include <linux/if_packet.h>
#include <linux/net.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>
static struct packet_type ThePostOfficePacketType;
struct ExpiryWorkBase {
    struct mutex Mutex;
    struct delayed_work Work;
    struct workqueue_struct *Workqueue;
    void (*CallExpiryWorkFunction)(void*);
    bool Cancel,ForceClose;
    void*Parent;
    u64 Timestamp;
};
void SetExpiryWorkBase(struct ExpiryWorkBase *expiry) {
    if(!expiry||expiry->ForceClose)return;
    mutex_lock(&expiry->Mutex);
    expiry->Timestamp=jiffies+msecs_to_jiffies(600000);
    expiry->Cancel=false;
    mod_delayed_work(expiry->Workqueue, &expiry->Work,msecs_to_jiffies(600000));
    mutex_unlock(&expiry->Mutex);
}
void ProcessExpiryWorkBaseToDo(struct work_struct *work) {
    struct ExpiryWorkBase *expiry = container_of(work, struct ExpiryWorkBase, Work.work);
    mutex_lock(&expiry->Mutex);
    if (!expiry->ForceClose&&(expiry->Cancel||time_after(jiffies, expiry->Timestamp + msecs_to_jiffies(600000)))) {
        mutex_unlock(&expiry->Mutex);
        return;
    }
    expiry->ForceClose=true;
    if (expiry->CallExpiryWorkFunction)
        expiry->CallExpiryWorkFunction(expiry->Parent);  
    mutex_unlock(&expiry->Mutex);
    kfree(expiry);
}
void ForceCloseExpiryWorkBase(struct ExpiryWorkBase *expiry) {
    mutex_lock(&expiry->Mutex);
    if (expiry->ForceClose) {
        mutex_unlock(&expiry->Mutex);
        return;
    }
    expiry->ForceClose = true;
    if(cancel_delayed_work_sync(&expiry->Work)&&expiry->CallExpiryWorkFunction)
            expiry->CallExpiryWorkFunction(expiry->Parent);
    mutex_unlock(&expiry->Mutex);
}


struct NetworkAdapter {
    struct net_device *dev;
    struct NetworkAdapter *Next, *Prev;
    struct ExpiryWorkBase Expiry;
};

static struct NetworkAdapter *NetworkAdapters=NULL;
static DEFINE_MUTEX(NetworkAdaptersMutex);
void CleanupNetworkAdapters(void){
    struct NetworkAdapter *CurrentAdapter,*NextAdapter;
    mutex_lock(&NetworkAdaptersMutex);
    CurrentAdapter=NetworkAdapters;
    while(CurrentAdapter){
        NextAdapter=CurrentAdapter->Next;
        ForceCloseExpiryWorkBase(&CurrentAdapter->Expiry);
        kfree(CurrentAdapter);
        CurrentAdapter=NextAdapter;
    }
    NetworkAdapters=NULL;
    mutex_unlock(&NetworkAdaptersMutex);
}


static int ThePostOfficeReceivePacket(struct sk_buff*skb,struct net_device*dev,struct packet_type*pt,struct net_device*orig_dev){
    if(!strcmp(dev->name,"lo")==0||skb->len<14||skb->pkt_type==PACKET_OUTGOING)
    return 0;
    u8*data;
    data=skb_mac_header(skb);
    if((data[0]&2))return 0;
    struct NetworkAdapter *NetworkAdapter;
    //Prepare the Neworkcard and the router
    bool IsTransmissionControlProtocol=false;
    u16 ethertype=ntohs(*(u16*)(data+12)),SourcePort=0,
        DestinationPort=0;
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
                FromNetworkAddressVersion_0=ntohs(*(u16*)(data+30)),
                FromNetworkAddressVersion_1=ntohs(*(u16*)(data+32));
       
            DestinationPort=ntohs(*(u16*)(data+34));    
            break;
        }
        case 34525:{
            SourcePort=ntohs(*(u16*)(data+56));
            if((data[22]==254&&(data[23]&192)==128)||(data[38]==254&&(data[39]&192)==128))return 0;
            if((IsTransmissionControlProtocol=(data[20]==6))&&SourcePort==22)return 0;
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
static void BindNetworkAdapterToTheProject(){
    ThePostOfficePacketType.type=htons(ETH_P_ALL);
    ThePostOfficePacketType.func=ThePostOfficeReceivePacket;
    dev_add_pack(&ThePostOfficePacketType);
}
static void UnbindNetworkAdapterToTheProject(){
    dev_remove_pack(&ThePostOfficePacketType);
}
Setup("ThePostOffice", 
    BindNetworkAdapterToTheProject(),
    UnbindNetworkAdapterToTheProject();
    CleanupNetworkAdapters();
    )