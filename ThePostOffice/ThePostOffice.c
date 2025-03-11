#include "ThePostOffice.h"
#include <linux/netdevice.h>
#include <linux/if_packet.h>
#include <linux/net.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>
static struct packet_type ThePostOfficePacketType;

struct IEEE8021Router;
struct IEEE8021NetworkInterface{
    struct net_device *dev;
    u8 MediaAccessControl[6]; 
    struct IEEE8021NetworkInterface*Next,*Prev;
    struct IEEE8021Router*Router;
    struct delayed_work Work;
    struct workqueue_struct *Workqueue;
    struct mutex RouterMutex;
};


struct IEEE8021Router;


struct InternetProtocolAddressSegmentNext;
struct InternetProtocolAddressSegmentFirst{
    struct IEEE8021Router*Router;
    u8 Value;
    struct InternetProtocolAddressSegmentNext*List;
    struct delayed_work Work;
    struct workqueue_struct*Workqueue;
    struct mutex Mutex;
};
struct InternetProtocolAddressSegmentLast;
struct InternetProtocolAddressSegmentNext{
    struct InternetProtocolAddressSegmentFirst*First;
    u8 Value;
    union{
        struct InternetProtocolAddressSegmentNext*Next;
        struct InternetProtocolAddressSegmentLast*Last;
    }ToDo;
    struct delayed_work Work;
    struct workqueue_struct*Workqueue;
    struct mutex Mutex;
};
struct InternetProtocolAddressSegmentLast{
    struct InternetProtocolAddressSegmentNext*Previous;
    u8 Value;
    bool IsVersion6;
    struct IEEE8021Router*Router;
    struct delayed_work Work;
    struct workqueue_struct*Workqueue;
    struct mutex Mutex;
};


struct IEEE8021Router{
    struct IEEE8021NetworkInterface*NetworkInterfaces;
    u8 MediaAccessControl[6];
    bool IsVersion6;
    struct InternetProtocolAddressSegmentFirst*InternetProtocolAddressSegmentFirst;
    struct IEEE8021Router*Next,*Prev;
    struct delayed_work Work;
    struct workqueue_struct *Workqueue;
    struct mutex InternetProtocolAddressSegmentFirstMutex;
};
static void UpdateTimeIEEE8021Router(struct IEEE8021Router*router){
    if(!work_pending(&router->Work.work))
        mod_delayed_work(router->Workqueue, &router->Work, msecs_to_jiffies(600000));

    if(!work_pending(&router->NetworkInterfaces->Work.work))
        mod_delayed_work(router->NetworkInterfaces->Workqueue, &router->NetworkInterfaces->Work, msecs_to_jiffies(600000));

}

void ProcessInternetProtocolAddressSegmentLastInnerNetwork(struct work_struct *work){

}

static struct InternetProtocolAddressSegmentLast* SetInternetProtocolAddressSegmentLastInnerNetwork(struct IEEE8021Router* router, u8 **ToNetworkAddress) {
    u8 value = (*ToNetworkAddress)[1]; // First byte of the address segment
    struct InternetProtocolAddressSegmentFirst* Current;
    Current = router->InternetProtocolAddressSegmentFirst;

    if (!Current) {
        mutex_lock(&router->InternetProtocolAddressSegmentFirstMutex);
        Current = kmalloc(sizeof(struct InternetProtocolAddressSegmentFirst), GFP_KERNEL);
        if (!Current) {
            mutex_unlock(&router->InternetProtocolAddressSegmentFirstMutex);
            return NULL;
        }
        Current->Workqueue = create_singlethread_workqueue("InternetProtocolAddressSegmentFirstWorkQueue");
        if (!Current->Workqueue) {
            kfree(Current);
            mutex_unlock(&router->InternetProtocolAddressSegmentFirstMutex);
            return NULL;
        }
        mutex_init(&Current->Mutex);
        Current->Router = router;
        Current->Value = value;
        Current->List = NULL; // Initialize the next segment pointer
        INIT_DELAYED_WORK(&Current->Work, ProcessInternetProtocolAddressSegmentLastInnerNetwork);
        router->InternetProtocolAddressSegmentFirst = Current;
        mutex_unlock(&router->InternetProtocolAddressSegmentFirstMutex);
    } else {
        mutex_lock(&router->InternetProtocolAddressSegmentFirstMutex);

        if (value != Current->Value) {
            // If value is different, insert a new first-level segment
            struct InternetProtocolAddressSegmentFirst* newSegment;
            newSegment = kmalloc(sizeof(struct InternetProtocolAddressSegmentFirst), GFP_KERNEL);
            if (!newSegment) {
                mutex_unlock(&router->InternetProtocolAddressSegmentFirstMutex);
                return NULL;
            }
            newSegment->Workqueue = create_singlethread_workqueue("InternetProtocolAddressSegmentFirstWorkQueue");
            if (!newSegment->Workqueue) {
                kfree(newSegment);
                mutex_unlock(&router->InternetProtocolAddressSegmentFirstMutex);
                return NULL;
            }
            mutex_init(&newSegment->Mutex);
            newSegment->Router = router;
            newSegment->Value = value;
            newSegment->List = NULL; // No next segment yet
            INIT_DELAYED_WORK(&newSegment->Work, ProcessInternetProtocolAddressSegmentLastInnerNetwork);

            router->InternetProtocolAddressSegmentFirst = newSegment;
            mutex_unlock(&router->InternetProtocolAddressSegmentFirstMutex);
            Current = newSegment;
        } else {
            mutex_unlock(&router->InternetProtocolAddressSegmentFirstMutex);
        }
    }

    // Now process next segment inside Current->List
    // (You'll need a separate function for this, similar to SetInternetProtocolAddressSegmentNext)
    
    kfree(*ToNetworkAddress);
    return NULL;
}

static struct IEEE8021NetworkInterface*ThePostOfficeNetworkInterfaces=NULL;
static DEFINE_MUTEX(IEEE8021NetworkInterfaceMutex);
void ProcessIEEE8021RouterClose(struct work_struct *work){
    mutex_lock(&IEEE8021NetworkInterfaceMutex);
    struct IEEE8021Router*router=container_of(work,struct IEEE8021Router,Work.work);
    if(!router){
        mutex_unlock(&IEEE8021NetworkInterfaceMutex);
        return;
    }
    if(router->InternetProtocolAddressSegmentFirst){
        mutex_lock(&router->InternetProtocolAddressSegmentFirstMutex);
        struct InternetProtocolAddressSegmentFirst*Current;
        Current=router->InternetProtocolAddressSegmentFirst;
        while(Current){
            if(!work_pending(&Current->Work.work))
                mod_delayed_work(Current->Workqueue, &Current->Work, msecs_to_jiffies(0));
         
            Current=Current->List;// this is wrong to do.
        }
        mutex_unlock(&router->InternetProtocolAddressSegmentFirstMutex);
        mod_delayed_work(router->Workqueue, &router->Work, msecs_to_jiffies(1000));
    }
    if(router->Prev)
        router->Prev->Next=router->Next;
    if(router->Next)
        router->Next->Prev=router->Prev;
    if(router->NetworkInterfaces)
        router->NetworkInterfaces->Router=NULL;
    mutex_unlock(&router->NetworkInterfaces->RouterMutex);
    if(ThePostOfficeNetworkInterfaces==router->NetworkInterfaces)
        ThePostOfficeNetworkInterfaces=router->NetworkInterfaces->Prev?router->NetworkInterfaces->Prev:router->NetworkInterfaces->Next;
    mutex_unlock(&IEEE8021NetworkInterfaceMutex);
    if(router->Workqueue){
        flush_workqueue(router->Workqueue);
        destroy_workqueue(router->Workqueue);
    }
    kfree(router);
}
static struct IEEE8021Router*GetIEEE8021Router(const struct IEEE8021NetworkInterface*interface,const u8 Value[6],bool IsVersion6){
    struct IEEE8021Router*Current;
    Current=interface->Router;
    while(Current){
        if(memcmp(Current->MediaAccessControl,Value,6)==0&&Current->IsVersion6==IsVersion6){
            if(work_pending(&Current->Work.work)){
                mutex_unlock(&interface->RouterMutex);
                return NULL;
            }
            UpdateTimeIEEE8021Router(Current);
            return Current;
        }
        Current=Current->Prev;
    }
    return NULL;
}
static struct IEEE8021Router*SetIEEE8021Router(const struct IEEE8021NetworkInterface*interface,const u8 Value[6],bool IsVersion6){
    struct IEEE8021Router*Current;
    Current=GetIEEE8021Router(interface,Value,IsVersion6);
    if(Current){
        mutex_unlock(&interface->RouterMutex);
        return Current;
    }
    Current=(struct IEEE8021Router*)kmalloc(sizeof(struct IEEE8021Router),GFP_KERNEL);
    if(!Current){
        mutex_unlock(&interface->RouterMutex);
        return NULL;
    }
    Current->Workqueue=create_singlethread_workqueue("IEEE8021RouterWorkQueue");
    if(!Current->Workqueue){
        kfree(Current);
        mutex_unlock(&interface->RouterMutex);
        return NULL;
    }
    INIT_DELAYED_WORK(&Current->Work, ProcessIEEE8021RouterClose);
    mutex_init(&Current->InternetProtocolAddressSegmentFirstMutex);
    memcpy(Current->MediaAccessControl,Value,6);
    Current->IsVersion6=IsVersion6;
    Current->NetworkInterfaces=interface;
    Current->Next=NULL;
    Current->InternetProtocolAddressSegmentFirst=NULL;
    Current->Prev=interface->Router;
    if(interface->Router)
        interface->Router->Next=Current;
    Current->NetworkInterfaces->Router=Current;
    mutex_unlock(&interface->RouterMutex);
    UpdateTimeIEEE8021Router(Current);
    return Current;
}
void ProcessIEEE8021NetworkInterfaceClose(struct work_struct *work) {
    mutex_lock(&IEEE8021NetworkInterfaceMutex);
    struct IEEE8021NetworkInterface *interface = container_of(work, struct IEEE8021NetworkInterface, Work.work);
    if(!interface){
        mutex_unlock(&IEEE8021NetworkInterfaceMutex);
        return;
    }
    if(interface->Router){
        mutex_lock(&interface->RouterMutex);
        struct IEEE8021Router*Current;
        Current=interface->Router;
        while (Current)
        {
            if(!work_pending(&Current->Work.work))
                mod_delayed_work(Current->Workqueue, &Current->Work, msecs_to_jiffies(0));
            Current=Current->Prev;
        }
        mutex_unlock(&interface->RouterMutex);
        mod_delayed_work(interface->Workqueue, &interface->Work, msecs_to_jiffies(1000));
    }
    if(interface->Prev)
        interface->Prev->Next=interface->Next;
    if(interface->Next)
        interface->Next->Prev=interface->Prev;
    if(ThePostOfficeNetworkInterfaces==interface) 
        ThePostOfficeNetworkInterfaces=interface->Prev?interface->Prev:interface->Next;
    mutex_unlock(&IEEE8021NetworkInterfaceMutex);
    if(interface->Workqueue){
        flush_workqueue(interface->Workqueue);
        destroy_workqueue(interface->Workqueue);
    }
    kfree(interface);
}
static struct IEEE8021NetworkInterface* GetIEEE8021NetworkInterface(const u8 Value[6]) {
    struct IEEE8021NetworkInterface* Current;
    Current = ThePostOfficeNetworkInterfaces;
    while(Current) {
        if (memcmp(Current->MediaAccessControl, Value, 6) == 0) {
            if (work_pending(&Current->Work.work)) { 
                mutex_unlock(&IEEE8021NetworkInterfaceMutex);
                return NULL; 
            }
            mutex_unlock(&IEEE8021NetworkInterfaceMutex);
            mod_delayed_work(Current->Workqueue, &Current->Work, msecs_to_jiffies(600000));
            return Current; 
        }
        Current = Current->Prev;
    }
    return NULL;
}
static struct IEEE8021NetworkInterface* SetIEEE8021NetworkInterface(const u8 Value[6],struct net_device *dev){
    struct IEEE8021NetworkInterface* Current;
    mutex_lock(&IEEE8021NetworkInterfaceMutex);
    Current=GetIEEE8021NetworkInterface(Value);
    if(Current){
        mutex_unlock(&IEEE8021NetworkInterfaceMutex);
        return Current;
    }
    Current=(struct IEEE8021NetworkInterface*)kmalloc(sizeof(struct IEEE8021NetworkInterface),GFP_KERNEL);
    if(!Current){
        mutex_unlock(&IEEE8021NetworkInterfaceMutex);
        return NULL;
    }
    Current->Workqueue=create_singlethread_workqueue("IEEE8021WorkQueue");
    if(!Current->Workqueue){
        kfree(Current);
        mutex_unlock(&IEEE8021NetworkInterfaceMutex);
        return NULL;
    }
    INIT_DELAYED_WORK(&Current->Work, ProcessIEEE8021NetworkInterfaceClose);
    mutex_init(&Current->RouterMutex); 
    memcpy(Current->MediaAccessControl,Value,6);
    Current->Router=NULL;
    Current->dev=dev;
    Current->Next=NULL;
    Current->Prev=ThePostOfficeNetworkInterfaces;
    if(ThePostOfficeNetworkInterfaces)
        ThePostOfficeNetworkInterfaces->Next=Current;
    ThePostOfficeNetworkInterfaces=Current;
    mutex_unlock(&IEEE8021NetworkInterfaceMutex);
    mod_delayed_work(Current->Workqueue, &Current->Work, msecs_to_jiffies(600000));
    return Current;
}

typedef bool(*ThePostOfficeReceivePacketCallback)(u16 DestinationPort,void*ConnectionIdentifier,u8**data);
static bool ThePostOfficeRegisterPacketCallbackFunction(
    struct net_device *dev, 
    bool IsTransmissionControlProtocol, 
    u16 SourcePort,
    bool IsVersion6, 
    const u8 *data, 
    u8 **ToNetworkAddress, 
    u8 **FromNetworkAddress,
    void**ConnectionIdentifier,
    ThePostOfficeReceivePacketCallback**ThePostOfficeReceivePacketCallbackFunction
){
  
  struct InternetProtocolAddressSegmentLast* innerNetwork;
   
    innerNetwork = SetInternetProtocolAddressSegmentLastInnerNetwork(SetIEEE8021Router(SetIEEE8021NetworkInterface(data, dev), data+6, IsVersion6), ToNetworkAddress);
   

    return false;  
}
static int ThePostOfficeReceivePacket(struct sk_buff*skb,struct net_device*dev,struct packet_type*pt,struct net_device*orig_dev){
    if(!strcmp(dev->name,"lo")==0||skb->len<14||skb->pkt_type==PACKET_OUTGOING)
    return 0;
    u8*data;
    data=skb_mac_header(skb);
    if((data[0]&2))return 0;
    u16 ethertype=ntohs(*(u16*)(data+12)),SourcePort=0,DestinationPort=0;
    bool IsVersion6=false,IsTransmissionControlProtocol=false;
    u8*ToNetworkAddress=NULL,*FromNetworkAddress=NULL;
    switch(ethertype){
        case 2048:{
            if(data[14]!=69)goto ForceExit;
            SourcePort=ntohs(*(u16*)(data+36));
            if((IsTransmissionControlProtocol=(data[23]==6))&&SourcePort==22)return 0;
            DestinationPort=ntohs(*(u16*)(data+34));
            ToNetworkAddress=data+26;
            FromNetworkAddress=data+30;
            break;
        }
        case 34525:{
            SourcePort=ntohs(*(u16*)(data+56));
            if((data[22]==254&&(data[23]&192)==128)||(data[38]==254&&(data[39]&192)==128))return 0;
            if((IsTransmissionControlProtocol=(data[20]==6))&&SourcePort==22)return 0;
            IsVersion6=true;
            ToNetworkAddress=data+22;
            FromNetworkAddress=data+38;
            DestinationPort=ntohs(*(u16*)(data+54));
            break;
        }
        default:return 0;
    }
    void*ConnectionIdentifier=NULL;
    ThePostOfficeReceivePacketCallback* ThePostOfficeReceivePacketCallbackFunction=NULL;
    if(!ThePostOfficeRegisterPacketCallbackFunction(dev,IsTransmissionControlProtocol,SourcePort,IsVersion6,data,&ToNetworkAddress,&FromNetworkAddress,&ConnectionIdentifier,&ThePostOfficeReceivePacketCallbackFunction))
       goto ForceExit;
    u8*newdata=(u8*)kmalloc(skb->len-(14+(IsVersion6?37:16)),GFP_KERNEL);
    if(!newdata)goto ForceExit;
    if(IsVersion6){
        memcpy(newdata,data+14,6);
        memcpy(newdata+6,data+21,1);
        memcpy(newdata+7,data+58,skb->len-58);
    }else{
        memcpy(newdata,data+15,8);
        memcpy(newdata+8,data+38,skb->len-38);
    }
    kfree_skb(skb);
    if((*ThePostOfficeReceivePacketCallbackFunction)(DestinationPort,ConnectionIdentifier,&newdata))return 1;
    ForceExit:
    if(skb)kfree_skb(skb);
    return 1;
}
bool ThePostOfficeSendPacket(struct IEEE8021Router* router){
    struct sk_buff *skb;
    skb=netdev_alloc_skb(router->NetworkInterfaces->dev,14+NET_IP_ALIGN);
    if(!skb)return false;
    skb_reserve(skb,NET_IP_ALIGN);
    skb_put_data(skb,router->MediaAccessControl,6);
    skb_put_data(skb,router->NetworkInterfaces->MediaAccessControl,6); 
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

Setup("ThePostOffice", 
    ThePostOfficePacketType.type=htons(ETH_P_ALL);
    ThePostOfficePacketType.func=ThePostOfficeReceivePacket;
    dev_add_pack(&ThePostOfficePacketType);,
    dev_remove_pack(&ThePostOfficePacketType);
    mutex_lock(&IEEE8021NetworkInterfaceMutex);
    for(struct IEEE8021NetworkInterface*Current=ThePostOfficeNetworkInterfaces;Current;Current=Current->Prev)
        if(!work_pending(&Current->Work.work))
            mod_delayed_work(Current->Workqueue,&Current->Work,msecs_to_jiffies(0));
    mutex_unlock(&IEEE8021NetworkInterfaceMutex);
    for(;ThePostOfficeNetworkInterfaces;msleep(100));
    )