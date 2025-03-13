#include "../TheRequirements/TheRequirements.h"
#include <linux/netdevice.h>
#include <linux/if_packet.h>
#include <linux/net.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>

//This is the expiry work base
struct ExpiryWorkBase {
    struct mutex Mutex;
    struct delayed_work Work;
    struct workqueue_struct *Workqueue;
    void (*CallExpiryWorkFunction)(void*);
    bool Cancel;
    void*Parent;
};
static void SetExpiryWorkBase(struct ExpiryWorkBase *expiry) {
    if(!expiry||expiry->Cancel)return;
    mutex_lock(&expiry->Mutex);
    if(expiry->Cancel) {
        mutex_unlock(&expiry->Mutex);
        return;
    }
    mod_delayed_work(expiry->Workqueue, &expiry->Work,msecs_to_jiffies(600000));
    mutex_unlock(&expiry->Mutex);
}
static void ProcessExpiryWorkBaseToDo(struct work_struct *work) {
    struct ExpiryWorkBase *expiry = container_of(work, struct ExpiryWorkBase, Work.work);
    mutex_lock(&expiry->Mutex);
    if(!expiry->Cancel) {
        mutex_unlock(&expiry->Mutex);
        return;
    }
    expiry->Cancel=true;
    mutex_unlock(&expiry->Mutex);
    mutex_lock(&expiry->Mutex);
    if(expiry->CallExpiryWorkFunction)
       expiry->CallExpiryWorkFunction(expiry->Parent);  
    if(expiry){
        destroy_workqueue(expiry->Workqueue);
        kfree(expiry->Parent);
    }    
}
static void SetupExpiryWorkBase(struct ExpiryWorkBase *expiry, void (*CallExpiryWorkFunction)(void*), void*Parent) {
    expiry->CallExpiryWorkFunction=CallExpiryWorkFunction;
    expiry->Parent=Parent;
    expiry->Cancel=false;
    expiry->Workqueue=create_singlethread_workqueue("ExpiryWorkBase");
    mutex_init(&expiry->Mutex);
    INIT_DELAYED_WORK(&expiry->Work, ProcessExpiryWorkBaseToDo);
}
// Start the ForceClose process the real start its in CleanupNetworkAdapters
static void ForceCloseExpiryWorkBase(struct ExpiryWorkBase *expiry) {
    mutex_lock(&expiry->Mutex);
    if (expiry->Cancel) {
        mutex_unlock(&expiry->Mutex);
        return;
    }
    expiry->Cancel = true;
    if(cancel_delayed_work_sync(&expiry->Work)&&expiry->CallExpiryWorkFunction){
        expiry->CallExpiryWorkFunction(expiry->Parent);
        kfree(expiry->Parent);
        return;
    }
    mutex_unlock(&expiry->Mutex);
}

struct Router;
struct NetworkAdapter {
    struct Router *Routers;
    struct net_device *dev;
    struct NetworkAdapter *Next, *Prev;
    struct ExpiryWorkBase Expiry;
};

struct VersionNetworkSegment;
struct Router{
    struct NetworkAdapter *NetworkInterfaces;
    u8 MediaAccessControl[6];
    struct VersionNetworkSegment *Segments;
    struct Router*Next,*Prev;
    struct ExpiryWorkBase Expiry;
};
static struct NetworkAdapter *NetworkAdapters=NULL;
static DEFINE_MUTEX(NetworkAdaptersMutex);
static void CleanupNetworkAdapters(void){
    mutex_lock(&NetworkAdaptersMutex);
    if(!NetworkAdapters){
        mutex_unlock(&NetworkAdaptersMutex);
        return;
    }
    struct NetworkAdapter*CurrentAdapter=NetworkAdapters;
    for(;CurrentAdapter;CurrentAdapter=CurrentAdapter->Prev)
        ForceCloseExpiryWorkBase(&CurrentAdapter->Expiry);
    mutex_unlock(&NetworkAdaptersMutex);
}


static void DeleteNetworkAdapter(void*NetworkAdapter){
    struct NetworkAdapter *CurrentAdapter=(struct NetworkAdapter*)NetworkAdapter;
    mutex_lock(&CurrentAdapter->Expiry.Mutex);
    struct Router *CurrentRouter=CurrentAdapter->Routers;
    for(;CurrentRouter;CurrentRouter=CurrentRouter->Prev)
        ForceCloseExpiryWorkBase(&CurrentRouter->Expiry);
    if(CurrentAdapter->Next)CurrentAdapter->Next->Prev=CurrentAdapter->Prev;
    if(CurrentAdapter->Prev)CurrentAdapter->Prev->Next=CurrentAdapter->Next;
    if(NetworkAdapters==CurrentAdapter)NetworkAdapters=CurrentAdapter->Prev?CurrentAdapter->Prev:CurrentAdapter->Next;
    mutex_unlock(&CurrentAdapter->Expiry.Mutex);
}

static struct Router *Routers=NULL;
static void HelpFunctionForGetRouterSetExpiryWorkBase(struct Router*router){
    SetExpiryWorkBase(&router->Expiry);
    SetExpiryWorkBase(&router->NetworkInterfaces->Expiry);
}

//i made this if other want to do something inside the router i dont know. mabe something.

static void DeleteRouter(void*router){

}

//This is segment of network
static struct VersionNetworkSegment* HelpFunctionForNetworkSegmentByRouter(struct VersionNetworkSegment*CurrentSegment,struct Router*router);
struct VersionNetworkSegment{
    struct Router *router;
    u16*Gateway;
    bool IsVersion6;
    struct ExpiryWorkBase Expiry;
};

//This is the segment of network for version 4
struct Version4NetworkSegmentLast {
    struct Router *router;
    u16*Gateway;
    bool IsVersion6;
    struct ExpiryWorkBase Expiry;
    u16 Address;
    struct Version4NetworkSegmentLast *Next, *Prev; 
};
struct Version4NetworkSegmentFirst {
    u16 Address;
    struct Version4NetworkSegmentLast *Last;
    struct Version4NetworkSegmentFirst *Next,*Prev;
    struct ExpiryWorkBase Expiry;
};
static struct Version4NetworkSegmentFirst *Version4NetworkSegmentFirst=NULL;
static DEFINE_MUTEX(Version4NetworkSegmentMutex);

//This is the segment of network for version 6
struct Version6NetworkSegmentLast
{   struct Router *router;
    u16*Gateway;
    bool IsVersion6;
    struct ExpiryWorkBase Expiry;
    u64 Address;
    struct Version6NetworkSegmentLast *Next,*Prev;
};
struct Version6NetworkSegmentFirst {
    u64 Address;
    struct Version6NetworkSegmentLast *Last;
    struct Version6NetworkSegmentFirst *Next,*Prev;
    struct ExpiryWorkBase Expiry;
};
static struct Version6NetworkSegmentFirst *Version6NetworkSegmentFirst=NULL;
static DEFINE_MUTEX(Version6NetworkSegmentMutex);

// Mix function for the segment of network
static void HelpFunctionForNetworkSegmentByRouterSetExpiryWorkBase(struct VersionNetworkSegment*CurrentSegment){
    if(!CurrentSegment)return;
    SetExpiryWorkBase(&CurrentSegment->Expiry);
    HelpFunctionForGetRouterSetExpiryWorkBase(CurrentSegment->router);
}


;
//Server
static struct VersionNetworkSegment*Server=NULL,*Client=NULL;

//This is the packet Receive function
static int ThePostOfficeReceivePacket(struct sk_buff*skb,struct net_device*dev,struct packet_type*pt,struct net_device*orig_dev){
    if(!strcmp(dev->name,"lo")==0||skb->len<14||skb->pkt_type==PACKET_OUTGOING)
    return 0;
    u8*data;
    data=skb_mac_header(skb);
    if((data[0]&2))return 0;
    struct NetworkAdapter *NetworkAdapter=NULL;
    mutex_lock(&NetworkAdaptersMutex);
    for(NetworkAdapter=NetworkAdapters;NetworkAdapter;NetworkAdapter=NetworkAdapter->Next)
        if(NetworkAdapter->dev==dev)break;
    mutex_unlock(&NetworkAdaptersMutex);
    if(!NetworkAdapter){
        NetworkAdapter=kmalloc(sizeof(struct NetworkAdapter),GFP_KERNEL);
        if(!NetworkAdapter){
            kfree_skb(skb);
            return 0;
        }
        NetworkAdapter->dev=dev;
        mutex_init(&NetworkAdapter->Expiry.Mutex);
        NetworkAdapter->Next=NULL;
        NetworkAdapter->Routers=NULL;
        mutex_lock(&NetworkAdaptersMutex);
        if(NetworkAdapters)NetworkAdapters->Prev=NetworkAdapter;
        NetworkAdapter->Next=NetworkAdapters;
        NetworkAdapters=NetworkAdapter;
        mutex_unlock(&NetworkAdaptersMutex);
        SetupExpiryWorkBase(&NetworkAdapter->Expiry,DeleteNetworkAdapter,NetworkAdapter);
    }else 
        SetExpiryWorkBase(&NetworkAdapter->Expiry);
    struct Router*router=NULL;
    for(router=NetworkAdapter->Routers;router;router=router->Next)
        if(!memcmp(router->MediaAccessControl,data+6,6))break;
    if(!router){
        mutex_lock(&NetworkAdapter->Expiry.Mutex);
        router=kmalloc(sizeof(struct Router),GFP_KERNEL);
        if(!router){
            mutex_unlock(&NetworkAdapter->Expiry.Mutex);
            kfree_skb(skb);
            return 1;
        }
        memcpy(router->MediaAccessControl,data+6,6);
        router->NetworkInterfaces=NetworkAdapter;
        router->Next=NULL;
        router->Prev=NetworkAdapter->Routers;
        if(NetworkAdapter->Routers)NetworkAdapter->Routers->Next=router;
        NetworkAdapter->Routers=router;
        SetupExpiryWorkBase(&router->Expiry,DeleteRouter,router);
        mutex_unlock(&NetworkAdapter->Expiry.Mutex);
    }else 
        SetExpiryWorkBase(&router->Expiry);
    bool IsTransmissionControlProtocol=false;
    u16 ethertype=ntohs(*(u16*)(data+12)),SourcePort=0,
        DestinationPort=0;
    struct VersionNetworkSegment *Server=NULL,*Client=NULL;
    switch(ethertype){
        case 2048:{
            if(data[14]!=69){
                kfree_skb(skb);
                return 1;
            }
            SourcePort=ntohs(*(u16*)(data+36));
            const u16* address = (const u16*)(data + 26);
            if((IsTransmissionControlProtocol=(data[23]==6))&&SourcePort==22)return 0;
            if(!(Server=GetVersion4NetworkSegmentByRouter((const u16*)(data + 26),router))){
                //here wee create it this is server ip addres
            }
            if(!(Client=GetVersion4NetworkSegmentByRouter((const u16*)(data+30),router))){
                //here wee create it this is client ip addres
            }
            DestinationPort=ntohs(*(u16*)(data+34));    
            break;
        }
        case 34525:{
            SourcePort=ntohs(*(u16*)(data+56));
            if((data[22]==254&&(data[23]&192)==128)||(data[38]==254&&(data[39]&192)==128))return 0;
            if((IsTransmissionControlProtocol=(data[20]==6))&&SourcePort==22)return 0;
            if(!(Server=GetVersion6NetworkSegmentByRouter((const u16*)(data+22),router))){
                //here wee create it this is server ip addres
            }
            if(!(Client=GetVersion6NetworkSegmentByRouter((const u16*)(data+38),router))){
                //here wee create it this is client ip addres
            }           
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




//This is the bind and unbind of the network adapter to the project
static struct packet_type ThePostOfficePacketType;
static void BindNetworkAdapterToTheProject(void){
    ThePostOfficePacketType.type=htons(ETH_P_ALL);
    ThePostOfficePacketType.func=ThePostOfficeReceivePacket;
    dev_add_pack(&ThePostOfficePacketType);
}
static void UnbindNetworkAdapterToTheProject(void){
    dev_remove_pack(&ThePostOfficePacketType);
}
//This is the basic setup of the project
Setup("ThePostOffice", 
    BindNetworkAdapterToTheProject(),
    UnbindNetworkAdapterToTheProject();
    CleanupNetworkAdapters();
    )