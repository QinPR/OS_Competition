#define __module_name__ 	"pagetable"

#include "printf.h"
#include "panic.h"
#include "types.h"
#include "string.h"
<<<<<<< HEAD
#include "pagetable.h"
=======
#include "mm/pagetable.h"
>>>>>>> fcd8328ce79e4ad0f6f868be8502f290ca99895f
#include "mm/framealloc.h"


// 从vpn中提取出二级PPN
uint64 get_ppn_2(uint64 ppn){
    return (ppn >> 17) & ((1 << 26) - 1);
}

// 从vpn中提取出一级PPN
uint64 get_ppn_1(uint64 ppn){
    return (ppn >> 8) & ((1 << 9) - 1);
}

// 从vpn中提取出零级PPN
uint64 get_ppn_0(uint64 ppn){
    return ppn & ((1 << 9) - 1);
}

uint64 to_physical_addr(uint64 ppn){
    return ppn << 12;
}

struct PageTable new_pagetable(){
    struct PageTable new_pt;
    new_pt.root_ppn = get_frame();

    return new_pt;
}

uint64 get_PTE(uint64 ppn, uint64 index){

    uint64 phys_addr = to_physical_addr(ppn) + 0x80200000;
    return (uint64*)(phys_addr+index*8);

}

uint64 check_PTE_valid(uint64 PTE){
    return PTE & 0x1;
}

void set_permission(uint64 PTE_addr, uint64 set_bit){
    *(uint64*)PTE_addr = *(uint64*)PTE_addr | set_bit;  
}

void set_PTE_ppn(uint64 PTE_addr, uint64 ppn){
    *(uint64*)PTE_addr |= (ppn << 10);
}

void free_PTE(uint64 PTE_addr){
    *(uint64*)PTE_addr = 0;
}


uint64 get_PTE_ppn(uint64 PTE_addr){   // 从PTE中取出其指向的物理页号
    uint64 PTE = *(uint64*)PTE_addr;
    return (PTE >> 10) & ((1 << 44) - 1);
}


void map(struct PageTable pg, uint64 vpn, uint64 ppn, uint64 flags){
    uint64 root_ppn = pg.root_ppn;

    uint64 ppn_2 = get_ppn_2(vpn);
    uint64 ppn_1 = get_ppn_1(vpn);
    uint64 ppn_0 = get_ppn_0(vpn);


    uint64 PTE_addr = get_PTE(root_ppn, ppn_2);

    if (!check_PTE_valid(*(uint64*)PTE_addr)){    // 证明下面的两级页表还没有建好。开始建表
        set_permission(PTE_addr, 1); // 设置Valid bit
        uint64 next_page_directory = get_frame();       // 为一级页表申请了一个frame
        set_PTE_ppn(PTE_addr, next_page_directory);     // 二级页表 -> 一级页表
        
        // 第2级页表建好了，现在建立第1级页表
        PTE_addr = get_PTE(next_page_directory, ppn_1);
        if (!check_PTE_valid(*(uint64*)PTE_addr)){
            set_permission(PTE_addr, 1); // 设置Valid bit
            next_page_directory = get_frame();    // 为零级页表申请了一个frame
            set_PTE_ppn(PTE_addr, next_page_directory);  // 一级页表 -> 零级页表

            // 第1级页表建好了，现在建立第0级页表
            PTE_addr = get_PTE(next_page_directory, ppn_0);
            if (!check_PTE_valid(*(uint64*)PTE_addr)){
                set_permission(PTE_addr, flags); 
                set_PTE_ppn(PTE_addr, ppn);  // 一级页表 -> 零级页表
            }else{
                panic("Wrong in mapping!\n");
            }
        }else{
            panic("Wrong in mapping!\n");
        }
    }else{
        panic("Wrong in mapping!\n");
    }
}


void unmap(struct PageTable pg, uint64 vpn){
    uint64 root_ppn = pg.root_ppn;

    uint64 ppn_2 = get_ppn_2(vpn);
    uint64 ppn_1 = get_ppn_1(vpn);
    uint64 ppn_0 = get_ppn_0(vpn);

    uint64 first_PTE_addr = get_PTE(root_ppn, ppn_2);

    if (check_PTE_valid(*(uint64*)first_PTE_addr)){    
     
        uint64 next_page_directory = get_PTE_ppn(first_PTE_addr);            
        uint64 second_PTE_addr = get_PTE(next_page_directory, ppn_1);
        free_PTE(first_PTE_addr);  

        if (check_PTE_valid(*(uint64*)second_PTE_addr)){
            
            next_page_directory = get_PTE_ppn(second_PTE_addr);            
            uint64 third_PTE_addr = get_PTE(next_page_directory, ppn_0);
            free_PTE(second_PTE_addr);

            if (check_PTE_valid(*(uint64*)third_PTE_addr)){
                uint64 corresponding_frame = get_PTE_ppn(third_PTE_addr);
                free_PTE(third_PTE_addr); 
                free_frame(corresponding_frame);
            }else{
                panic("Wrong in unmapping!\n");
            } 
        }else{
            panic("Wrong in unmapping!\n");
        }  
    }else{
        panic("Wrong in unmapping!\n");
    }
}

void test_page_table(){
    struct PageTable test_page_table = new_pagetable();

    uint64 root_ppn = test_page_table.root_ppn;

    map(test_page_table, 1, 11, 0b1111);

    unmap(test_page_table, 1);
}
