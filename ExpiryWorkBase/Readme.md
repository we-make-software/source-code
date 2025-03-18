# **Expiry Work Base - Memory Management for Time-Limited Data**  

This project is designed to **manage memory efficiently** by remembering data for **10 minutes** or allowing early cancellation when necessary.  

## **Background**  
The problem originates from **ARP in networking**, which has a **10-minute timeout** for remembering MAC-IP mappings. To **optimize memory usage**, this system ensures that:  
- Data is **automatically deleted** when no longer needed.  
- Each level in the **hierarchy deletes its lower levels first** before removing itself.  
- The **lower levels cannot delete higher levels**, ensuring proper data flow.  
- The system integrates well with **`list_head`** for efficient structuring.

## **Why is this needed in Linux?**  
Linux lacks an **automated, hierarchical deletion system** where deletion happens in an **orderly, structured way**. This project fills that gap.  

While this system does introduce **some workload**, Linux is **designed to handle it efficiently**. Since deletion is **less important than new data**, it runs in **`system_wq`**, ensuring that new data **takes priority** over old data cleanup.

## **Handling Mutex and Null Checks**  
- A **mutex** ensures thread safety when accessing or modifying data.  
- **Data retrieval** doesn’t need a mutex but benefits from **null checks**.  
- If data is **null**, the struct provides an **`Invalid`** flag, preventing unnecessary processing.  
- The **`Invalid`** flag should be checked **after** performing a null check.

---

## **Public Functions**  

### **1. Setup Expiry Work Base**
```c
extern void SetupExpiryWorkBase(struct ExpiryWorkBase*expiry_work_base, 
                                struct ExpiryWorkBase*previous, 
                                void*parent, 
                                void(*AutoDelete)(void*));
```
- **Initializes an `ExpiryWorkBase` instance.**  
- **Previous:** Links to a higher hierarchy level (or `NULL` if not needed).  
- **AutoDelete:** Custom function called when deletion occurs.

### **2. Cancel Expiry Work Base**
```c
extern void CancelExpiryWorkBase(struct ExpiryWorkBase*expiry_work_base);
```
- **Cancels an `ExpiryWorkBase` instance.**  
- Cancelling does **not** affect the **lower hierarchy**, but lower levels **can cancel higher levels**.

### **3. Reset Expiry Work Base**
```c
extern bool ResetExpiryWorkBase(struct ExpiryWorkBase*expiry_work_base);
```
- **Resets the **10-minute timeout** for **all linked instances**.

### **4. Lock Expiry Work Base**
```c
extern void LockExpiryWorkBase(struct ExpiryWorkBase*expiry_work_base);
```
- **Locks the instance to prevent modifications.**

### **5. Unlock Expiry Work Base**
```c
extern void UnlockExpiryWorkBase(struct ExpiryWorkBase*expiry_work_base);
```
- **Unlocks the instance to allow modifications.**

---

## **Performance Considerations**  
- This system **minimizes overhead** by running in **`system_wq`**, ensuring that **new data takes priority** over old data cleanup.  
- Mutex locking is only applied **where necessary** to maintain efficiency.  
- The `Invalid` flag provides a **lightweight mechanism** to avoid unnecessary operations.

---

## **Usage**
Just run the **KO file** and add `ExpiryWorkBase.h` to your project.