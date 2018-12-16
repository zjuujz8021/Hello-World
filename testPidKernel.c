#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <asm/pgtable.h>
#include <asm/segment.h>
#include <linux/highmem.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("VERY NIUBILITY");
MODULE_VERSION("0.0.1");

static int pid = 0;
module_param(pid, int, 0644);
MODULE_PARM_DESC(pid_int, "the pid...\n");

static unsigned long addr = 0;
module_param(addr, ulong, 0644);
MODULE_PARM_DESC(addr_ulong, "the virtual address\n");

static inline void store_gdt(struct desc_ptr *ptr)
{
    asm("sgdt %x0":"=m" (*ptr));
}

static unsigned long get_pgd(pid_t pid)
{
    struct task_struct *task;
    for_each_process(task)
    {
        if (pid == task->pid)
        {
            return *(unsigned long*)(task->mm->pgd);
        }
    }
    return 0;
}

static unsigned long read_user(unsigned long user_addr, int is_va)
{
    int page_number, page_indent;
    struct page *pp;
    void *from;
    unsigned long temp = 0;
    unsigned long user_pa = 0;
    if (is_va)
    {
        user_pa = virt_to_phys(user_addr);
    }
    else
    {
        user_pa = user_addr;
    }
    //printk(KERN_INFO "[*] step debug: %d....",  __LINE__);

    page_number = user_pa / PAGE_SIZE;
    page_indent = user_pa % PAGE_SIZE;
    //printk(KERN_INFO "[*] step debug: %d....",  __LINE__);

    pp = pfn_to_page(page_number);
    //printk(KERN_INFO "[*] step debug: %d....",  __LINE__);
    from = kmap(pp) + page_indent;
    //printk(KERN_INFO "[*] step debug: %d....",  __LINE__);
    temp = /*(unsigned long *)*/ from;
    //printk(KERN_INFO "[*] step debug: %d....",  __LINE__);

    kunmap(pp);
    //printk(KERN_INFO "[*] step debug: %d....",  __LINE__);
    return temp;
}

#define GET_FIRST_INDEX(addr) ((addr>>39) & 0x1ffLL)
#define GET_SECOND_INDEX(addr) ((addr>>30) & 0x1ffLL)
#define GET_THIRD_INDEX(addr) ((addr>>21) & 0x1ffLL)
#define GET_FOURTH_INDEX(addr) ((addr>>12) & 0x1ffLL)

static int __init m_init(void)
{
    struct desc_ptr gdtr;
    unsigned long gdtr_entry, gdtr_entry_address;

    unsigned long gdtr_base = 0;
    unsigned long linear_add = 0;

    unsigned long pgd = 0;
    unsigned long second_pg = 0;
    unsigned long third_pg = 0;
    unsigned long fourth_pg = 0;

    unsigned long physical_frame = 0;
    unsigned long temp = 0;

    if (pid == 0)
    {
        printk(KERN_INFO "[*] please provide pid \n");
        return -1;
    }

    if (addr == 0)
    {
        printk(KERN_INFO "[*] please provide va \n");
        return -1;
    }

    store_gdt(&gdtr);
    gdtr_entry_address = gdtr.address + (__USER_DS -3);
    gdtr_entry = *(unsigned long *)(gdtr_entry_address);

    printk(KERN_INFO "[*] step 1: gdtr %lX gdt entry [%lX] : %lX\n", gdtr.address, gdtr_entry_address, gdtr_entry);

    gdtr_base = ((gdtr_entry>>16) & 0xFFFF) + ((gdtr_entry >> 32) &0xFF) + ((gdtr_entry>>56)&0xFF);
    linear_add = addr + gdtr_base;
    printk(KERN_INFO "[*] step 2: base addr: %lX linear addr %lX", gdtr_base, linear_add);

    pgd = get_pgd(pid);

    temp = pgd + GET_FIRST_INDEX(addr) * 8;
    printk(KERN_INFO "[*] step 3: pgd: [%lX] entry addr: [%lX]: %lX\n", pgd, temp, read_user(temp, 1));

    second_pg = read_user(temp, 1) & (0xffffffff000);
    temp = pgd + GET_SECOND_INDEX(addr) * 8;
    printk(KERN_INFO "[*] step 4: second level: [%lX] entry addr: [%lX]: %lX\n", second_pg, temp, read_user(temp, 0));

    third_pg = read_user(temp, 0) & (0xffffffff000);
    temp = third_pg + GET_THIRD_INDEX(addr) * 8;
    printk(KERN_INFO "[*] step 5: third level: [%lX] entry addr: [%lX]: %lX\n", third_pg, temp, read_user(temp, 0));

    fourth_pg = read_user(temp, 0) & (0xffffffff000);
    temp = fourth_pg + GET_FOURTH_INDEX(addr) * 8;
    printk(KERN_INFO "[*] step 6: fourth level: [%lX] entry addr: [%lX]: %lX\n", fourth_pg, temp, read_user(temp, 0));

    physical_frame = read_user(temp, 0) & (0xffffffff000);
    temp = physical_frame + (addr & 0xfffLL);
    printk(KERN_INFO "[*] step 7: physical frame: [%lX] physical addr: [%lX]: %lX\n ", physical_frame, temp, read_user(temp, 0));

    return 0;
}

static void __exit m_exit(void)
{
    printk(KERN_INFO "Goodbye, World\n");
}

module_init(m_init);    // 注册模块
module_exit(m_exit);    // 注销模块


