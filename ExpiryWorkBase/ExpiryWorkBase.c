#include "../TheRequirements/TheRequirements.h"
struct ExpiryWorkBase {
    bool Cancelled;
    struct mutex Mutex;
    struct ExpiryWorkBase*Previous;
    void*Parent;
    void(*AutoDelete)(void*); 
    struct delayed_work Work;
};
static void StopRaceConditionExpiryWorkBase(struct ExpiryWorkBase*expiry_work_base);
static void StopRaceConditionExpiryWorkBase(struct ExpiryWorkBase*expiry_work_base){
    if(!expiry_work_base)return;
    mutex_lock(&expiry_work_base->Mutex);
    mutex_unlock(&expiry_work_base->Mutex);
    kfree(expiry_work_base->Parent);
}
static void ProcessExpiryWorkBaseToDo(struct work_struct*work);
static void ProcessExpiryWorkBaseToDo(struct work_struct*work){
    struct ExpiryWorkBase*expiry_work_base=container_of(work,struct ExpiryWorkBase,Work.work);
    if(!expiry_work_base)return;
    mutex_lock(&expiry_work_base->Mutex);
    if(expiry_work_base->Cancelled){
        mutex_unlock(&expiry_work_base->Mutex);
        return;
    }
    expiry_work_base->Cancelled=true;
    if(expiry_work_base->AutoDelete)
        expiry_work_base->AutoDelete(expiry_work_base->Parent);
    mutex_unlock(&expiry_work_base->Mutex);
    StopRaceConditionExpiryWorkBase(expiry_work_base);
}
void LockExpiryWorkBase(struct ExpiryWorkBase*expiry_work_base);
void LockExpiryWorkBase(struct ExpiryWorkBase*expiry_work_base){
    if(!expiry_work_base)return;
    mutex_lock(&expiry_work_base->Mutex);
}
EXPORT_SYMBOL(LockExpiryWorkBase);
void UnlockExpiryWorkBase(struct ExpiryWorkBase*expiry_work_base);
void UnlockExpiryWorkBase(struct ExpiryWorkBase*expiry_work_base){
    if(!expiry_work_base)return;
    mutex_unlock(&expiry_work_base->Mutex);
}
EXPORT_SYMBOL(UnlockExpiryWorkBase);
bool ResetExpiryWorkBase(struct ExpiryWorkBase*expiry_work_base);
bool ResetExpiryWorkBase(struct ExpiryWorkBase*expiry_work_base){
	if(!expiry_work_base||expiry_work_base->Cancelled||work_pending(&expiry_work_base->Work.work))return false;
	mutex_lock(&expiry_work_base->Mutex);
    if(expiry_work_base->Cancelled){
        mutex_unlock(&expiry_work_base->Mutex);
        return false;
    }
	bool is_pending=mod_delayed_work(system_wq,&expiry_work_base->Work,msecs_to_jiffies(600000));
	mutex_unlock(&expiry_work_base->Mutex);
	if(is_pending&&expiry_work_base->Previous)
        return ResetExpiryWorkBase(expiry_work_base->Previous);
	return is_pending;
}
EXPORT_SYMBOL(ResetExpiryWorkBase); 
void SetupExpiryWorkBase(struct ExpiryWorkBase*expiry_work_base,struct ExpiryWorkBase*previous,void*parent,void(*AutoDelete)(void*));
void SetupExpiryWorkBase(struct ExpiryWorkBase*expiry_work_base,struct ExpiryWorkBase*previous,void*parent,void(*AutoDelete)(void*)){
    expiry_work_base->Previous=previous;
    expiry_work_base->Parent=parent;
    expiry_work_base->AutoDelete=AutoDelete;
    expiry_work_base->Cancelled=false;
    mutex_init(&expiry_work_base->Mutex);
    INIT_DELAYED_WORK(&expiry_work_base->Work,ProcessExpiryWorkBaseToDo);
    ResetExpiryWorkBase(expiry_work_base);
}
EXPORT_SYMBOL(SetupExpiryWorkBase);
void CancelExpiryWorkBase(struct ExpiryWorkBase*expiry_work_base);
void CancelExpiryWorkBase(struct ExpiryWorkBase*expiry_work_base){
	if(!expiry_work_base)return;
	mutex_lock(&expiry_work_base->Mutex);
    if(expiry_work_base->Cancelled){
        mutex_unlock(&expiry_work_base->Mutex);
        return;
    }
    expiry_work_base->Cancelled=true;
    cancel_delayed_work_sync(&expiry_work_base->Work);
	if(expiry_work_base->AutoDelete)
		expiry_work_base->AutoDelete(expiry_work_base->Parent);
	mutex_unlock(&expiry_work_base->Mutex);
    StopRaceConditionExpiryWorkBase(expiry_work_base);
}
EXPORT_SYMBOL(CancelExpiryWorkBase);
struct BackgroundExpiryWorkBase{
    struct work_struct work;
    struct ExpiryWorkBase*expiry_work_base;
};
static void BackgroundProcessExpiryWorkBase(struct work_struct*work);
static void BackgroundProcessExpiryWorkBase(struct work_struct*work){
    struct BackgroundExpiryWorkBase*background_expiry_work_base=container_of(work,struct BackgroundExpiryWorkBase,work);
    if(!background_expiry_work_base)return;
    ResetExpiryWorkBase(background_expiry_work_base->expiry_work_base);
    kfree(background_expiry_work_base);
}
void BackgroundResetExpiryWorkBase(struct ExpiryWorkBase*expiry_work_base);
void BackgroundResetExpiryWorkBase(struct ExpiryWorkBase*expiry_work_base){
    struct BackgroundExpiryWorkBase*background_expiry_work_base=kmalloc(sizeof(struct BackgroundExpiryWorkBase),GFP_KERNEL);
    if(!background_expiry_work_base){
        ResetExpiryWorkBase(expiry_work_base);
        return;
    }
    background_expiry_work_base->expiry_work_base=expiry_work_base;
    INIT_WORK(&background_expiry_work_base->work,BackgroundProcessExpiryWorkBase);
    queue_work(system_wq,&background_expiry_work_base->work);
} 
EXPORT_SYMBOL(BackgroundResetExpiryWorkBase);  


Setup("Expiry Work Base",{},{})