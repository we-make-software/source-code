#ifndef ThePostOffice_h
#define ThePostOffice_h
#include "../TheRequirements/TheRequirements.h"
struct VersionNetworkSegment{
    struct Router *router;
    u16*Address;
    bool IsVersion6;
    struct VersionNetworkSegment *Next,*Prev;
};
struct Router {
    struct NetworkAdapter *NetworkInterfaces;
    u8 MediaAccessControl[6];
    struct VersionNetworkSegment *Segments;
};
struct NetworkAdapter {
    struct Router *Routers;
};
extern void GetNetworkAdapter(const u64 MediaAccessControl, void (*Callback)(struct NetworkAdapter*));
extern void GetAllNetworkAdapters(void(*Callback)(struct NetworkAdapter*));
extern struct Router *GetRouter(const u8*MediaAccessControl, const void(*Callback)(struct Router*));
extern void GetAllRouters(const struct NetworkAdapter *NetworkInterfaces, void(*Callback)(struct Router*));
extern struct VersionNetworkSegment *GetVersionNetworkSegment(const u16*Address, const bool IsVersion6);
extern struct VersionNetworkSegment *GetVersion6NetworkSegment(const u16*Address);
extern struct VersionNetworkSegment *GetVersion4NetworkSegment(const u16*Address);
extern struct VersionNetworkSegment *GetVersion6NetworkSegmentByRouter(const u16*Address, struct Router*router);
extern struct VersionNetworkSegment *GetVersionNetworkSegmentByRouter(const u16*Address, struct Router*router, const bool IsVersion6);

#endif

