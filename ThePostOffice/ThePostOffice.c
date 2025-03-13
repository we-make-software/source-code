#include "../TheRequirements/TheRequirements.h"
#include <linux/netdevice.h>
#include <linux/if_packet.h>
#include <linux/net.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>
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
    if(!expiry)return;
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
    destroy_workqueue(expiry->Workqueue);
    kfree(expiry->Parent);
    expiry=NULL;  
}
static void SetupExpiryWorkBase(struct ExpiryWorkBase *expiry, void (*CallExpiryWorkFunction)(void*), void*Parent) {
    expiry->CallExpiryWorkFunction=CallExpiryWorkFunction;
    expiry->Parent=Parent;
    expiry->Cancel=false;
    expiry->Workqueue=create_singlethread_workqueue("ExpiryWorkBase");
    mutex_init(&expiry->Mutex);
    INIT_DELAYED_WORK(&expiry->Work, ProcessExpiryWorkBaseToDo);
    SetExpiryWorkBase(expiry);
}
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
    while(CurrentAdapter){
        struct NetworkAdapter*PrevAdapter=CurrentAdapter->Prev;
        ForceCloseExpiryWorkBase(&CurrentAdapter->Expiry);
        CurrentAdapter=PrevAdapter;
    }
    mutex_unlock(&NetworkAdaptersMutex);
}
static void DeleteNetworkAdapter(void*NetworkAdapter){
    struct NetworkAdapter*CurrentAdapter=(struct NetworkAdapter*)NetworkAdapter;
    mutex_lock(&CurrentAdapter->Expiry.Mutex);
    struct Router *CurrentRouter=CurrentAdapter->Routers;
    while (CurrentRouter) {
        struct Router*PrevRouter=CurrentRouter->Prev;
        ForceCloseExpiryWorkBase(&CurrentRouter->Expiry);
        CurrentRouter=PrevRouter;
    }
    if(CurrentAdapter->Next)CurrentAdapter->Next->Prev=CurrentAdapter->Prev;
    if(CurrentAdapter->Prev)CurrentAdapter->Prev->Next=CurrentAdapter->Next;
    if(NetworkAdapters==CurrentAdapter)NetworkAdapters=CurrentAdapter->Prev?CurrentAdapter->Prev:CurrentAdapter->Next;
    mutex_unlock(&CurrentAdapter->Expiry.Mutex);
}
static struct Router *Routers=NULL;


static struct VersionNetworkSegment *Segments=NULL;


//This is segment of network

enum VersionType { Version4, Version6 };
enum SegmentType { Server, Client };
struct VersionNetworkSegment{
    struct Router *router;
    enum VersionType Is; 
    struct ExpiryWorkBase Expiry;
    enum SegmentType Type; 
    struct VersionNetworkSegment *Next, *Prev;
    
};
struct Version4NetworkSegment {
    struct Router *router;
    enum VersionType Is; 
    struct ExpiryWorkBase Expiry;
    enum SegmentType Type; 
    struct Version4NetworkSegment *Next, *Prev; 
    u32 Address;
};
struct Version6NetworkSegment{
    struct Router *router;
    enum VersionType Is; 
    struct ExpiryWorkBase Expiry;
    enum SegmentType Type; 
    struct Version6NetworkSegment*Next,*Prev;
    u64 AddressHigh;
    u64 AddressLow;
};
void FreeNetworkSegment(struct VersionNetworkSegment *segment) {
    if(!segment)return;
    if(segment->Is==Version6) 
        kfree((struct Version6NetworkSegment*)segment);
    else 
        kfree((struct Version4NetworkSegment*)segment);
}

static void DeleteNetworkSegment(void*segment){
//for the client
}

static void DeleteRouterNetworkSegment(void*segment){
//for the router
}

static void DeleteRouter(void*router){

}
//This is the packet Receive function
static int ThePostOfficeReceivePacket(struct sk_buff*skb,struct net_device*dev,struct packet_type*pt,struct net_device*orig_dev){
    if(!strcmp(dev->name,"lo")==0||skb->len<14||skb->pkt_type==PACKET_OUTGOING)
    return 0;
    u8*data;
    data=skb_mac_header(skb);
    if((data[0]&2))return 0;
    struct NetworkAdapter *NetworkAdapter=NetworkAdapters;
    mutex_lock(&NetworkAdaptersMutex);
    for(;NetworkAdapter&&NetworkAdapter->dev!=dev;NetworkAdapter=NetworkAdapter->Next);
    mutex_unlock(&NetworkAdaptersMutex);
    if(!NetworkAdapter){
        NetworkAdapter=kmalloc(sizeof(struct NetworkAdapter),GFP_KERNEL);
        if(!NetworkAdapter){
            kfree_skb(skb);
            return 0;
        }
        NetworkAdapter->dev=dev;
        SetupExpiryWorkBase(&NetworkAdapter->Expiry,DeleteNetworkAdapter,NetworkAdapter);
        NetworkAdapter->Next=NULL;
        NetworkAdapter->Routers=NULL;
        mutex_lock(&NetworkAdaptersMutex);
        if(NetworkAdapters)NetworkAdapters->Prev=NetworkAdapter;
        NetworkAdapter->Next=NetworkAdapters;
        NetworkAdapters=NetworkAdapter;
        mutex_unlock(&NetworkAdaptersMutex);
    } 
    struct Router*router=NetworkAdapter->Routers;
    for(;router&&memcmp(router->MediaAccessControl,data+6,6)!=0;router=router->Next);
      
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
        router->Segments=NULL;
        SetupExpiryWorkBase(&router->Expiry,DeleteRouter,router);
        router->Prev=NetworkAdapter->Routers;
        if(NetworkAdapter->Routers)NetworkAdapter->Routers->Next=router;
        NetworkAdapter->Routers=router;
        mutex_unlock(&NetworkAdapter->Expiry.Mutex);
    }
    bool IsTransmissionControlProtocol=false;
    u16 ethertype=ntohs(*(u16*)(data+12)),SourcePort=0,
        DestinationPort=0;
    struct VersionNetworkSegment *server=NULL,*client=NULL;
    switch(ethertype){
        case 2048:{
            if(data[14]!=69){
                kfree_skb(skb);
                return 1;
            }
            SourcePort=ntohs(*(u16*)(data+36));
            if((IsTransmissionControlProtocol=(data[23]==6))&&SourcePort==22)return 0;
            u32 SourceIP=*(u32*)(data+26);
            if(!router->Segments){
                router->Segments=kmalloc(sizeof(struct Version4NetworkSegment),GFP_KERNEL);
                if(!router->Segments){
                    kfree_skb(skb);
                    return 1;
                }
                struct Version4NetworkSegment*segment=(struct Version4NetworkSegment*)router->Segments;
                segment->router=router;
                segment->Is=Version4;
                segment->Type=Server;
                segment->Address=SourceIP;
                segment->Prev=segment->Next=NULL;
                SetupExpiryWorkBase(&segment->Expiry,DeleteRouterNetworkSegment,segment);
                server=segment;
            }else{
                struct Version4NetworkSegment*segment=(struct Version4NetworkSegment*)router->Segments;
                struct Version4NetworkSegment*last=NULL;
                for(;segment&&(segment->Is==Version6||(segment->Next&&segment->Next->Address<=SourceIP));segment=segment->Next){
                    if(segment->Is==Version4) last=segment;
                    if(segment->Is==Version4&&segment->Address==SourceIP){
                        server=segment;
                        break;
                    }
                }
                if(!server){
                    struct Version4NetworkSegment*new=kmalloc(sizeof(struct Version4NetworkSegment),GFP_KERNEL);
                    if(!new){
                        kfree_skb(skb);
                        return 1;
                    }
                    new->router=router;
                    new->Is=Version4;
                    new->Type=Server;
                    new->Address=SourceIP;
                    new->Prev=new->Next=NULL;
                    SetupExpiryWorkBase(&new->Expiry,DeleteRouterNetworkSegment,new);
                    if(!last){
                        new->Next=(struct Version4NetworkSegment*)router->Segments;
                        if(router->Segments)router->Segments->Prev=new;
                        router->Segments=(struct VersionNetworkSegment*)new;
                    }else{
                        new->Next=last->Next;
                        new->Prev=last;
                        if(last->Next)last->Next->Prev=new;
                        last->Next=new;
                    }
                    server=new;    
                }
            }
            u32 DestinationIP=*(u32*)(data+30);
            if(!Segments){
                Segments=kmalloc(sizeof(struct Version4NetworkSegment),GFP_KERNEL);
                if(!Segments){
                    kfree_skb(skb);
                    return 1;
                }
                struct Version4NetworkSegment*segment=(struct Version4NetworkSegment*)Segments;
                segment->router=NULL;
                segment->Is=Version4;
                segment->Type=Client;
                segment->Address=DestinationIP;
                segment->Prev=segment->Next=NULL;
                SetupExpiryWorkBase(&segment->Expiry,DeleteRouterNetworkSegment,segment);
                client=segment;
            }else{
                struct Version4NetworkSegment*segment=(struct Version4NetworkSegment*)Segments;
                struct Version4NetworkSegment*last=NULL;
                for(;segment&&(segment->Is==Version6||(segment->Next&&segment->Next->Address<=DestinationIP));segment=segment->Next){
                    if(segment->Is==Version4) last=segment;
                    if(segment->Is==Version4&&segment->Address==DestinationIP){
                        client=segment;
                        break;
                    }
                }
                if(!client){
                    struct Version4NetworkSegment*new=kmalloc(sizeof(struct Version4NetworkSegment),GFP_KERNEL);
                    if(!new){
                        kfree_skb(skb);
                        return 1;
                    }
                    new->router=NULL;
                    new->Is=Version4;
                    new->Type=Client;
                    new->Address=DestinationIP;
                    new->Prev=new->Next=NULL;
                    SetupExpiryWorkBase(&new->Expiry,DeleteRouterNetworkSegment,new);
                    if(!last){
                        new->Next=(struct Version4NetworkSegment*)Segments;
                        if(Segments)Segments->Prev=new;
                        Segments=(struct VersionNetworkSegment*)new;
                    }else{
                        new->Next=last->Next;
                        new->Prev=last;
                        if(last->Next)last->Next->Prev=new;
                        last->Next=new;
                    }
                    client=new;
                }
            }
            
            
            SetExpiryWorkBase(&server->Expiry);// when its this need to do thats the point
            SetExpiryWorkBase(&router->Expiry);// when its this need to do thats the point
            SetExpiryWorkBase(&NetworkAdapter->Expiry);// when its this need to do thats the point

            DestinationPort=ntohs(*(u16*)(data+34));    
            break;
        }
        case 34525:{
            SourcePort=ntohs(*(u16*)(data+56));
            if((data[22]==254&&(data[23]&192)==128)||(data[38]==254&&(data[39]&192)==128))return 0;
            if((IsTransmissionControlProtocol=(data[20]==6))&&SourcePort==22)return 0;
            u64 SourceIPHigh=*(u64*)(data+22);
            u64 SourceIPLow=*(u64*)(data+30);
            u64 DestinationIP_High=*(u64*)(data+38);
            u64 DestinationIP_Low=*(u64*)(data+46);

           
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