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
    struct IEEE8021NetworkInterface*Next,*Prev;
    struct IEEE8021Router*Router;
    struct mutex Mutex;
    struct delayed_work Work;
    struct workqueue_struct *Workqueue;
};
struct IEEE8021Router;
struct InternetProtocolAddressSegmentNext;
struct InternetProtocolAddressSegmentFirst{
    struct IEEE8021Router*Router;
    u8 Value;
    struct InternetProtocolAddressSegmentNext*List;
    struct mutex Mutex;
    struct InternetProtocolAddressSegmentFirst*Next,*Prev;  
    struct delayed_work Work;
    struct workqueue_struct*Workqueue;
};
struct InternetProtocolAddressSegmentLast;
struct InternetProtocolAddressSegmentNext{
    u8 Value;
    union{
        struct InternetProtocolAddressSegmentNext*Next;
        struct InternetProtocolAddressSegmentLast*Last;
    }ToDo;
    struct mutex Mutex;
    struct InternetProtocolAddressSegmentNext*Next,*Prev;
    struct delayed_work Work;
    struct workqueue_struct*Workqueue;
};
struct InternetProtocolAddressSegmentLast{
    u8 Value;
    struct mutex Mutex;
    struct InternetProtocolAddressSegmentLast*Next,*Prev;
    struct delayed_work Work;
    struct workqueue_struct*Workqueue;
};
struct IEEE8021Router{
    struct IEEE8021NetworkInterface*NetworkInterfaces;
    u8 MediaAccessControl[6];
    bool IsVersion6;
    struct InternetProtocolAddressSegmentLast*Server,*Client;
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
static void UpdateInternetProtocolAddressSegmentFirst(struct InternetProtocolAddressSegmentFirst*segment){
    if(!work_pending(&segment->Work.work))
        mod_delayed_work(segment->Workqueue, &segment->Work, msecs_to_jiffies(600000));
    UpdateTimeIEEE8021Router(segment->Router);    
}



void ProcessInternetProtocolAddressSegmentLastInnerNetwork(struct work_struct *work){

}
void ProcessInternetProtocolAddressSegmentNext(struct work_struct *work){

}
void ProcessInternetProtocolAddressSegmentLast(struct work_struct *work){

}
static struct InternetProtocolAddressSegmentFirst*CreateInternetProtocolAddressSegmentFirst(struct IEEE8021Router* router, u8 value){
    struct InternetProtocolAddressSegmentFirst* segment;
    segment=kmalloc(sizeof(struct InternetProtocolAddressSegmentFirst),GFP_KERNEL);
    if(!segment)return NULL;
    segment->Workqueue = create_singlethread_workqueue("InternetProtocolAddressSegmentFirstWorkQueue");
    if(!segment->Workqueue) {
        kfree(segment);
        return NULL;
    }
    mutex_init(&segment->Mutex);
    segment->Router=router;
    segment->Value=value;
    segment->List=NULL;
    segment->Prev=segment->Next=NULL;
    INIT_DELAYED_WORK(&segment->Work, ProcessInternetProtocolAddressSegmentLastInnerNetwork);
    return segment;
}
static struct InternetProtocolAddressSegmentNext*CreateInternetProtocolAddressSegmentNext(u8 value){
    struct InternetProtocolAddressSegmentNext*segment=kmalloc(sizeof(struct InternetProtocolAddressSegmentNext),GFP_KERNEL);
    if(!segment)return NULL;
    segment->Workqueue=create_singlethread_workqueue("InternetProtocolAddressSegmentNextWorkQueue");
    if(!segment->Workqueue){
        kfree(segment);
        return NULL;
    }
    mutex_init(&segment->Mutex);
    segment->Value=value;
    segment->Prev=segment->Next=NULL;
    segment->ToDo.Next=NULL;
    INIT_DELAYED_WORK(&segment->Work,ProcessInternetProtocolAddressSegmentNext);
    return segment;
}
static struct InternetProtocolAddressSegmentLast*CreateInternetProtocolAddressSegmentLast(u8 value){
    struct InternetProtocolAddressSegmentLast*segment=kmalloc(sizeof(struct InternetProtocolAddressSegmentLast),GFP_KERNEL);
    if(!segment)return NULL;
    segment->Workqueue=create_singlethread_workqueue("InternetProtocolAddressSegmentLastWorkQueue");
    if(!segment->Workqueue){
        kfree(segment);
        return NULL;
    }
    mutex_init(&segment->Mutex);
    segment->Value=value;
    segment->Next=segment->Prev=NULL;
    INIT_DELAYED_WORK(&segment->Work,ProcessInternetProtocolAddressSegmentLast);
    return segment;
}


static struct InternetProtocolAddressSegmentLast* SetInternetProtocolAddressSegmentLastNetwork(struct IEEE8021Router*router,struct InternetProtocolAddressSegmentFirst** FirstCurrentPtr,u8**NetworkAddress){
    u8 value=(*NetworkAddress)[1];
    mutex_lock(&router->InternetProtocolAddressSegmentFirstMutex);
    if (!(*FirstCurrentPtr)) { 
        *FirstCurrentPtr = CreateInternetProtocolAddressSegmentFirst(router, value);
        if (!(*FirstCurrentPtr)){
            mutex_unlock(&router->InternetProtocolAddressSegmentFirstMutex);
            kfree(*NetworkAddress);
            return NULL;
        }
    } else if((*FirstCurrentPtr)->Value!=value) {
        if((*FirstCurrentPtr)->Value>value){
            for(struct InternetProtocolAddressSegmentFirst*now=(*FirstCurrentPtr);now&&now->Prev&&now->Prev->Value>value;now=now->Prev);
            if((*FirstCurrentPtr)->Prev){
                struct InternetProtocolAddressSegmentFirst*New=CreateInternetProtocolAddressSegmentFirst(router,value);
                if(!New){
                    mutex_unlock(&router->InternetProtocolAddressSegmentFirstMutex);
                    kfree(*NetworkAddress);
                    return NULL;
                }
                New->Next=(*FirstCurrentPtr);
                New->Prev=(*FirstCurrentPtr)->Prev;
                (*FirstCurrentPtr)->Prev->Next=New;
                (*FirstCurrentPtr)->Prev=New;
                *FirstCurrentPtr=New;
            }else{
                struct InternetProtocolAddressSegmentFirst*New=CreateInternetProtocolAddressSegmentFirst(router,value);
                if(!New){
                    mutex_unlock(&router->InternetProtocolAddressSegmentFirstMutex);
                    kfree(*NetworkAddress);
                    return NULL;
                }
                New->Next=(*FirstCurrentPtr);
                (*FirstCurrentPtr)->Prev=New;
                *FirstCurrentPtr=New;
            }
        }else{
            for(struct InternetProtocolAddressSegmentFirst*now=(*FirstCurrentPtr);now&&now->Next&&now->Next->Value<value;now=now->Next);
            if((*FirstCurrentPtr)->Next){
                struct InternetProtocolAddressSegmentFirst*New=CreateInternetProtocolAddressSegmentFirst(router,value);
                if(!New){
                    mutex_unlock(&router->InternetProtocolAddressSegmentFirstMutex);
                    kfree(*NetworkAddress);
                    return NULL;
                }
                New->Prev=(*FirstCurrentPtr);
                New->Next=(*FirstCurrentPtr)->Next;
                (*FirstCurrentPtr)->Next->Prev=New;
                (*FirstCurrentPtr)->Next=New;
            }else{
                struct InternetProtocolAddressSegmentFirst*New=CreateInternetProtocolAddressSegmentFirst(router,value);
                if(!New){
                    mutex_unlock(&router->InternetProtocolAddressSegmentFirstMutex);
                    kfree(*NetworkAddress);
                    return NULL;
                }
                New->Prev=(*FirstCurrentPtr);
                (*FirstCurrentPtr)->Next=New;
            }
        }
    } 
    struct InternetProtocolAddressSegmentFirst*LockFirstCurrent=*FirstCurrentPtr;  
    mutex_unlock(&router->InternetProtocolAddressSegmentFirstMutex);
    struct InternetProtocolAddressSegmentNext*BetweenCurrent;
    BetweenCurrent=LockFirstCurrent->List;
    u8 BetweenLookUpLimit=router->IsVersion6?14:2;
    mutex_lock(&LockFirstCurrent->Mutex);
    for(u8 i=0;i<BetweenLookUpLimit;i++){
        value=(*NetworkAddress)[i+1];
        if(BetweenCurrent){
            if(BetweenCurrent->Value==value){
                mod_delayed_work(BetweenCurrent->Workqueue,&BetweenCurrent->Work,msecs_to_jiffies(600000));
                continue;
            }
            if(BetweenCurrent->Value>value){
                for(;BetweenCurrent->Value!=value&&BetweenCurrent->Prev&&BetweenCurrent->Prev->Value>value;BetweenCurrent=BetweenCurrent->Prev);
                if(BetweenCurrent->Value==value){
                    mod_delayed_work(BetweenCurrent->Workqueue,&BetweenCurrent->Work,msecs_to_jiffies(600000));
                    continue;
                }
                mutex_lock(&BetweenCurrent->Mutex);
                struct InternetProtocolAddressSegmentNext*New=CreateInternetProtocolAddressSegmentNext(value);
                if(!New){
                    mutex_unlock(&BetweenCurrent->Mutex);
                    mutex_unlock(&LockFirstCurrent->Mutex);
                    kfree(*NetworkAddress);
                    return NULL;
                }
                New->Next=BetweenCurrent;
                if((New->Prev=BetweenCurrent->Prev))
                    BetweenCurrent->Prev->Next=New;
                BetweenCurrent->Prev=New;
                BetweenCurrent=New;
                mutex_unlock(&BetweenCurrent->Mutex);
                mod_delayed_work(BetweenCurrent->Workqueue,&BetweenCurrent->Work,msecs_to_jiffies(600000));
                continue;
            }
            for(;BetweenCurrent->Value!=value&&BetweenCurrent->Next&&BetweenCurrent->Next->Value<value;BetweenCurrent=BetweenCurrent->Next);
            if(BetweenCurrent->Value==value){
                mod_delayed_work(BetweenCurrent->Workqueue,&BetweenCurrent->Work,msecs_to_jiffies(600000));
                continue;
            }
            mutex_lock(&BetweenCurrent->Mutex);
            struct InternetProtocolAddressSegmentNext*New=CreateInternetProtocolAddressSegmentNext(value);
            if(!New){
                mutex_unlock(&BetweenCurrent->Mutex);
                mutex_unlock(&LockFirstCurrent->Mutex);
                kfree(*NetworkAddress);
                return NULL;
            }
            New->Prev=BetweenCurrent;
            if((New->Next=BetweenCurrent->Next))
                BetweenCurrent->Next->Prev=New;
            BetweenCurrent->Next=New;
            BetweenCurrent=New;
            mutex_unlock(&BetweenCurrent->Mutex);
            mod_delayed_work(BetweenCurrent->Workqueue,&BetweenCurrent->Work,msecs_to_jiffies(600000));
            continue;
        }
        BetweenCurrent=CreateInternetProtocolAddressSegmentNext(value);
        if(!BetweenCurrent){
            mutex_unlock(&LockFirstCurrent->Mutex);
            kfree(*NetworkAddress);
            return NULL;
        }
        if(i==0)LockFirstCurrent->List=BetweenCurrent;
        mod_delayed_work(BetweenCurrent->Workqueue,&BetweenCurrent->Work,msecs_to_jiffies(600000));
    }
    mutex_unlock(&LockFirstCurrent->Mutex);
    BetweenLookUpLimit=router->IsVersion6?15:3;
    value=(*NetworkAddress)[BetweenLookUpLimit];
    mutex_lock(&BetweenCurrent->Mutex);
    if(BetweenCurrent->ToDo.Last){
        if(BetweenCurrent->ToDo.Last->Value==value){
            mutex_unlock(&BetweenCurrent->Mutex);
            mod_delayed_work(BetweenCurrent->ToDo.Last->Workqueue,&BetweenCurrent->ToDo.Last->Work,msecs_to_jiffies(600000));
            UpdateInternetProtocolAddressSegmentFirst(LockFirstCurrent);
            kfree(*NetworkAddress);
            return BetweenCurrent->ToDo.Last;
        }
        if(BetweenCurrent->ToDo.Last->Value>value){
            for(;BetweenCurrent->ToDo.Last->Value!=value&&BetweenCurrent->ToDo.Last->Prev&&BetweenCurrent->ToDo.Last->Prev->Value>value;BetweenCurrent->ToDo.Last=BetweenCurrent->ToDo.Last->Prev);
            if(BetweenCurrent->ToDo.Last->Value==value){
                mutex_unlock(&BetweenCurrent->Mutex);
                mod_delayed_work(BetweenCurrent->ToDo.Last->Workqueue,&BetweenCurrent->ToDo.Last->Work,msecs_to_jiffies(600000));
                UpdateInternetProtocolAddressSegmentFirst(LockFirstCurrent);
                kfree(*NetworkAddress);
                return BetweenCurrent->ToDo.Last;
            }
            struct InternetProtocolAddressSegmentLast*New=CreateInternetProtocolAddressSegmentLast(value);
            if(!New){
                mutex_unlock(&BetweenCurrent->Mutex);
                kfree(*NetworkAddress);
                return NULL;
            }
            New->Next=BetweenCurrent->ToDo.Last;
            if((New->Prev=BetweenCurrent->ToDo.Last->Prev))
                BetweenCurrent->ToDo.Last->Prev->Next=New;
            BetweenCurrent->ToDo.Last->Prev=New;
            BetweenCurrent->ToDo.Last=New;
            mutex_unlock(&BetweenCurrent->Mutex);
            mod_delayed_work(BetweenCurrent->ToDo.Last->Workqueue,&BetweenCurrent->ToDo.Last->Work,msecs_to_jiffies(600000));
            UpdateInternetProtocolAddressSegmentFirst(LockFirstCurrent);
            kfree(*NetworkAddress);
            return BetweenCurrent->ToDo.Last;
        }
        for(;BetweenCurrent->ToDo.Last->Value!=value&&BetweenCurrent->ToDo.Last->Next&&BetweenCurrent->ToDo.Last->Next->Value<value;BetweenCurrent->ToDo.Last=BetweenCurrent->ToDo.Last->Next);
        if(BetweenCurrent->ToDo.Last->Value==value){
            mutex_unlock(&BetweenCurrent->Mutex);
            mod_delayed_work(BetweenCurrent->ToDo.Last->Workqueue,&BetweenCurrent->ToDo.Last->Work,msecs_to_jiffies(600000));
            UpdateInternetProtocolAddressSegmentFirst(LockFirstCurrent);
            kfree(*NetworkAddress);
            return BetweenCurrent->ToDo.Last;
        }
        struct InternetProtocolAddressSegmentLast*New=CreateInternetProtocolAddressSegmentLast(value);
        if(!New){
            mutex_unlock(&BetweenCurrent->Mutex);
            kfree(*NetworkAddress);
            return NULL;
        }
        New->Prev=BetweenCurrent->ToDo.Last;
        if((New->Next=BetweenCurrent->ToDo.Last->Next))
            BetweenCurrent->ToDo.Last->Next->Prev=New;
        BetweenCurrent->ToDo.Last->Next=New;
        BetweenCurrent->ToDo.Last=New;
        mutex_unlock(&BetweenCurrent->Mutex);
        mod_delayed_work(BetweenCurrent->ToDo.Last->Workqueue,&BetweenCurrent->ToDo.Last->Work,msecs_to_jiffies(600000));
        UpdateInternetProtocolAddressSegmentFirst(LockFirstCurrent);
        kfree(*NetworkAddress);
        return BetweenCurrent->ToDo.Last;
    }
    if(!(BetweenCurrent->ToDo.Last=CreateInternetProtocolAddressSegmentLast(value))){
        mutex_unlock(&BetweenCurrent->Mutex);
        kfree(*NetworkAddress);
        return NULL;
    }
    mutex_unlock(&BetweenCurrent->Mutex);
    mod_delayed_work(BetweenCurrent->ToDo.Last->Workqueue,&BetweenCurrent->ToDo.Last->Work,msecs_to_jiffies(600000));
    UpdateInternetProtocolAddressSegmentFirst(LockFirstCurrent);
    kfree(*NetworkAddress);
    return BetweenCurrent->ToDo.Last;
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
    if(router->Server){
      /*
        mutex_lock(&router->InternetProtocolAddressSegmentFirstMutex);
        struct InternetProtocolAddressSegmentFirst*Current;
        Current=router->Server;
        if(Current){
            for(struct InternetProtocolAddressSegmentFirst*now=Current;now;now=now->Prev)
                if(!work_pending(&now->Work.work))
                    mod_delayed_work(now->Workqueue, &now->Work, msecs_to_jiffies(0));
            for(struct InternetProtocolAddressSegmentFirst*now=Current->Next;now;now=now->Next)
                if(!work_pending(&now->Work.work))
                    mod_delayed_work(now->Workqueue, &now->Work, msecs_to_jiffies(0));
            if(!work_pending(&Current->Work.work))
                mod_delayed_work(Current->Workqueue, &Current->Work, msecs_to_jiffies(0));
        }
        mutex_unlock(&router->InternetProtocolAddressSegmentFirstMutex);
        mod_delayed_work(router->Workqueue, &router->Work, msecs_to_jiffies(1000));
        return;
    */
    }
    if(router->Client){
        /*
        mutex_lock(&router->InternetProtocolAddressSegmentFirstMutex);
        struct InternetProtocolAddressSegmentFirst*Current;
        Current=router->Client;
        if(Current){
            for(struct InternetProtocolAddressSegmentFirst*now=Current;now;now=now->Prev)
                if(!work_pending(&now->Work.work))
                    mod_delayed_work(now->Workqueue, &now->Work, msecs_to_jiffies(0));
            for(struct InternetProtocolAddressSegmentFirst*now=Current->Next;now;now=now->Next)
                if(!work_pending(&now->Work.work))
                    mod_delayed_work(now->Workqueue, &now->Work, msecs_to_jiffies(0));
            if(!work_pending(&Current->Work.work))
                mod_delayed_work(Current->Workqueue, &Current->Work, msecs_to_jiffies(0));
        }
        mutex_unlock(&router->InternetProtocolAddressSegmentFirstMutex);
        mod_delayed_work(router->Workqueue, &router->Work, msecs_to_jiffies(1000));
        return;
        */
    }
    mutex_lock(&router->NetworkInterfaces->Mutex);    
    if(router->Prev)
        router->Prev->Next=router->Next;
    if(router->Next)
        router->Next->Prev=router->Prev;
    if(router->NetworkInterfaces)
        router->NetworkInterfaces->Router=NULL;
    mutex_unlock(&router->NetworkInterfaces->Mutex);
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
    for(struct IEEE8021Router*Current=interface->Router;Current;Current=Current->Prev)
        if(memcmp(Current->MediaAccessControl,Value,6)==0&&Current->IsVersion6==IsVersion6){
            if(work_pending(&Current->Work.work)){
                mutex_unlock(&interface->Mutex);
                return NULL;
            }
            UpdateTimeIEEE8021Router(Current);
            return Current;
        }
    return NULL;
}
static struct IEEE8021Router*SetIEEE8021Router(struct IEEE8021NetworkInterface*interface,const u8 Value[6],bool IsVersion6){
    struct IEEE8021Router*Current;
    if((Current=GetIEEE8021Router(interface,Value,IsVersion6))){
        mutex_unlock(&interface->Mutex);
        return Current;
    }
    if(!(Current=(struct IEEE8021Router*)kmalloc(sizeof(struct IEEE8021Router),GFP_KERNEL))){
        mutex_unlock(&interface->Mutex);
        return NULL;
    }
    if(!(Current->Workqueue=create_singlethread_workqueue("IEEE8021RouterWorkQueue"))){
        kfree(Current);
        mutex_unlock(&interface->Mutex);
        return NULL;
    }
    INIT_DELAYED_WORK(&Current->Work, ProcessIEEE8021RouterClose);
    mutex_init(&Current->InternetProtocolAddressSegmentFirstMutex);
    memcpy(Current->MediaAccessControl,Value,6);
    Current->IsVersion6=IsVersion6;
    Current->NetworkInterfaces=interface;
    Current->Next=NULL;
    Current->Server=Current->Client=NULL;
    Current->Prev=interface->Router;
    if(interface->Router)
        interface->Router->Next=Current;
    interface->Router=Current;
    mutex_unlock(&interface->Mutex);
    UpdateTimeIEEE8021Router(Current);
    return Current;
}
void ProcessIEEE8021NetworkInterfaceClose(struct work_struct *work){
    mutex_lock(&IEEE8021NetworkInterfaceMutex);
    struct IEEE8021NetworkInterface *interface=container_of(work,struct IEEE8021NetworkInterface,Work.work);
    if(interface->Router){
        mutex_lock(&interface->Mutex);
        for(struct IEEE8021Router *Current=interface->Router;Current;Current=Current->Prev)
            if(!work_pending(&Current->Work.work))
                mod_delayed_work(Current->Workqueue,&Current->Work,msecs_to_jiffies(0));
        mutex_unlock(&interface->Mutex);
        mod_delayed_work(interface->Workqueue,&interface->Work,msecs_to_jiffies(1000));
        return;
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
static struct IEEE8021NetworkInterface* GetIEEE8021NetworkInterface(const u8 Value[6]){
    for(struct IEEE8021NetworkInterface*Current=ThePostOfficeNetworkInterfaces;Current;Current=Current->Prev){
        if(!memcmp(Current->dev->dev_addr,Value,6)){
            if(work_pending(&Current->Work.work)){
                mutex_unlock(&IEEE8021NetworkInterfaceMutex);
                return NULL;
            }
            mutex_unlock(&IEEE8021NetworkInterfaceMutex);
            mod_delayed_work(Current->Workqueue,&Current->Work,msecs_to_jiffies(600000));
            return Current;
        }
    }
    return NULL;
}
static struct IEEE8021NetworkInterface* SetIEEE8021NetworkInterface(struct net_device *dev){
    struct IEEE8021NetworkInterface* Current;
    mutex_lock(&IEEE8021NetworkInterfaceMutex);
    if((Current=GetIEEE8021NetworkInterface(dev->dev_addr))){
        mutex_unlock(&IEEE8021NetworkInterfaceMutex);
        return Current;
    }
    if(!(Current=(struct IEEE8021NetworkInterface*)kmalloc(sizeof(struct IEEE8021NetworkInterface),GFP_KERNEL))){
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
    mutex_init(&Current->Mutex); 
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
    struct IEEE8021Router*router=SetIEEE8021Router(SetIEEE8021NetworkInterface(dev),data+6,IsVersion6);
    struct InternetProtocolAddressSegmentLast*Server=SetInternetProtocolAddressSegmentLastNetwork(router,&router->Server,ToNetworkAddress),
                                             *Client=SetInternetProtocolAddressSegmentLastNetwork(router,&router->Client,FromNetworkAddress);
    
    if(!Client)return false;
    if(!Server){
        // **Do something**
        // **You'll need to implement this**
        // block clients ip address or something they have call a protol thast not part of the system
        return false;
    }
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