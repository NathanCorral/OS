/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"

#include "errors.h"
#include "keyboard.h"
#include "keyboardirq.h"
#include "rtc.h"
#include "rtcirq.h"
 #include "paging.h"
 #include "interrupts.h"

/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))




/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void
entry (unsigned long magic, unsigned long addr)
{
//int x,y;
int i;
	multiboot_info_t *mbi;
	
	/* Clear the screen. */
	clear();


	/* Am I booted by a Multiboot-compliant boot loader? */
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		printf ("Invalid magic number: 0x%#x\n", (unsigned) magic);
		return;
	}

	/* Set MBI to the address of the Multiboot information structure. */
	mbi = (multiboot_info_t *) addr;

	/* Print out the flags. */
	printf ("flags = 0x%#x\n", (unsigned) mbi->flags);

	/* Are mem_* valid? */
	if (CHECK_FLAG (mbi->flags, 0))
		printf ("mem_lower = %uKB, mem_upper = %uKB\n",
				(unsigned) mbi->mem_lower, (unsigned) mbi->mem_upper);

	/* Is boot_device valid? */
	if (CHECK_FLAG (mbi->flags, 1))
		printf ("boot_device = 0x%#x\n", (unsigned) mbi->boot_device);

	/* Is the command line passed? */
	if (CHECK_FLAG (mbi->flags, 2))
		printf ("cmdline = %s\n", (char *) mbi->cmdline);

	if (CHECK_FLAG (mbi->flags, 3)) {
		int mod_count = 0;
		int i;
		module_t* mod = (module_t*)mbi->mods_addr;
		while(mod_count < mbi->mods_count) {
			printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start);
			printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
			printf("First few bytes of module:\n");
			for(i = 0; i<16; i++) {
				printf("0x%x ", *((char*)(mod->mod_start+i)));
			}
			printf("\n");
			mod_count++;
			mod++;
		}
	}
	/* Bits 4 and 5 are mutually exclusive! */
	if (CHECK_FLAG (mbi->flags, 4) && CHECK_FLAG (mbi->flags, 5))
	{
		printf ("Both bits 4 and 5 are set.\n");
		return;
	}

	/* Is the section header table of ELF valid? */
	if (CHECK_FLAG (mbi->flags, 5))
	{
		elf_section_header_table_t *elf_sec = &(mbi->elf_sec);

		printf ("elf_sec: num = %u, size = 0x%#x,"
				" addr = 0x%#x, shndx = 0x%#x\n",
				(unsigned) elf_sec->num, (unsigned) elf_sec->size,
				(unsigned) elf_sec->addr, (unsigned) elf_sec->shndx);
	}

	/* Are mmap_* valid? */
	if (CHECK_FLAG (mbi->flags, 6))
	{
		memory_map_t *mmap;

		printf ("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
				(unsigned) mbi->mmap_addr, (unsigned) mbi->mmap_length);
		for (mmap = (memory_map_t *) mbi->mmap_addr;
				(unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
				mmap = (memory_map_t *) ((unsigned long) mmap
					+ mmap->size + sizeof (mmap->size)))
			printf (" size = 0x%x,     base_addr = 0x%#x%#x\n"
					"     type = 0x%x,  length    = 0x%#x%#x\n",
					(unsigned) mmap->size,
					(unsigned) mmap->base_addr_high,
					(unsigned) mmap->base_addr_low,
					(unsigned) mmap->type,
					(unsigned) mmap->length_high,
					(unsigned) mmap->length_low);
	}

	/* Construct an LDT entry in the GDT */
	{
		seg_desc_t the_ldt_desc;
		the_ldt_desc.granularity    = 0;
		the_ldt_desc.opsize         = 1;
		the_ldt_desc.reserved       = 0;
		the_ldt_desc.avail          = 0;
		the_ldt_desc.present        = 1;
		the_ldt_desc.dpl            = 0x0;
		the_ldt_desc.sys            = 0;
		the_ldt_desc.type           = 0x2;

		SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
		ldt_desc_ptr = the_ldt_desc;
		lldt(KERNEL_LDT);
	}

	/* Construct a TSS entry in the GDT */
	{
		seg_desc_t the_tss_desc;
		the_tss_desc.granularity    = 0;
		the_tss_desc.opsize         = 0;
		the_tss_desc.reserved       = 0;
		the_tss_desc.avail          = 0;
		the_tss_desc.seg_lim_19_16  = TSS_SIZE & 0x000F0000;
		the_tss_desc.present        = 1;
		the_tss_desc.dpl            = 0x0;
		the_tss_desc.sys            = 0;
		the_tss_desc.type           = 0x9;
		the_tss_desc.seg_lim_15_00  = TSS_SIZE & 0x0000FFFF;

		SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

		tss_desc_ptr = the_tss_desc;

		tss.ldt_segment_selector = KERNEL_LDT;
		tss.ss0 = KERNEL_DS;
		tss.esp0 = 0x800000;
		ltr(KERNEL_TSS);
	}

	//exceptions
// for(i=0; i<0x20; i++){
	
// 	idt[i].present = 1;
//     idt[i].dpl = 0;
//     idt[i].reserved0 = 0;
//     idt[i].size = 1;
//     idt[i].reserved1 = 1;
//     idt[i].reserved2 = 1;
//     idt[i].reserved3 = 1;
//     idt[i].reserved4 = 0;
//     idt[i].seg_selector = KERNEL_CS;
// }

// //interrupts
// for(i=0x20; i<0x2F; i++){
// 	idt[i].present = 1;
//     idt[i].dpl = 0;
//     idt[i].reserved0 = 0;
//     idt[i].size = 1;
//     idt[i].reserved1 = 1;
//     idt[i].reserved2 = 1;
//     idt[i].reserved3 = 0;
//     idt[i].reserved4 = 0;
//     idt[i].seg_selector = KERNEL_CS;
// }


// 	SET_IDT_ENTRY(idt[0], dividebyzero);
// 	SET_IDT_ENTRY(idt[1], debugger);
// 	SET_IDT_ENTRY(idt[2], nmi);
// 	SET_IDT_ENTRY(idt[3], breakpoint);
// 	SET_IDT_ENTRY(idt[4], overflow);
// 	SET_IDT_ENTRY(idt[5], bounds);
// 	SET_IDT_ENTRY(idt[6], invalidopcode);
// 	SET_IDT_ENTRY(idt[7], coprocessornotavailable);
// 	SET_IDT_ENTRY(idt[8], doublefault);
// 	SET_IDT_ENTRY(idt[9], coprocessorsegoverrun);
// 	SET_IDT_ENTRY(idt[10], invalidtask);
// 	SET_IDT_ENTRY(idt[11], segnotpresent);
// 	SET_IDT_ENTRY(idt[12], stackfault);
// 	SET_IDT_ENTRY(idt[13], genprotection);
// 	SET_IDT_ENTRY(idt[14], pagefault);
// 	SET_IDT_ENTRY(idt[15], reserved);
// 	SET_IDT_ENTRY(idt[16], mathfault);
// 	SET_IDT_ENTRY(idt[17], aligncheck);
// 	SET_IDT_ENTRY(idt[18], machinecheck);
// 	SET_IDT_ENTRY(idt[19], simdfloat);

// 	//reserved
// 	for(i=20; i <= 31; i++) {
//         SET_IDT_ENTRY(idt[i], reserved);		
//     }
// 	i=0x80;
//     idt[i].present = 1;
//     idt[i].dpl = 3;
//     idt[i].reserved0 = 0;
//     idt[i].size = 1;
//     idt[i].reserved1 = 1;
//     idt[i].reserved2 = 1;
//     idt[i].reserved3 = 1;
//     idt[i].reserved4 = 0;
//     idt[i].seg_selector = KERNEL_CS; 
// 	SET_IDT_ENTRY(idt[0x80], syscall);

// SET_IDT_ENTRY(idt[33],keyboard_wrapper);
// 	SET_IDT_ENTRY(idt[40],rtc_wrapper);
interruptinit();

//	x=5/0;
	// i = 33;
	// idt[i].present = 1;
 //    idt[i].dpl = 0;
 //    idt[i].reserved0 = 0;
 //    idt[i].size = 1;
 //    idt[i].reserved1 = 1;
 //    idt[i].reserved2 = 1;
 //    idt[i].reserved3 = 0;
 //    idt[i].reserved4 = 0;
 //    idt[i].seg_selector = KERNEL_CS;
	lidt(idt_desc_ptr);
	clear();
	

pageinit();
pageenable();

int x;
x=5;
printf("%d \n", x);
x=10;
printf("%d \n", x);

// char *pointer;
// pointer=(char *) 0x1b8000;

// *pointer= 'c';
	/* Init the PIC */
	i8259_init();
	rtc_init();


	cli();
	enable_irq(8);
	enable_irq(1);
	enable_irq(2);
	sti();

	// SET_IDT_ENTRY(idt[0x28], reserved);
	// enable_irq(8);


	/* Initialize devices, memory, filesystem, enable device interrupts on the
	 * PIC, any other initialization stuff... */

	/* Enable interrupts */
	/* Do not enable the following until after you have set up your
	 * IDT correctly otherwise QEMU will triple fault and simple close
	 * without showing you any output */
	/*printf("Enabling Interrupts\n");
	sti();*/

	/* Execute the first program (`shell') ... */
	//while(1);

	/* Spin (nicely, so we don't chew up cycles) */
	asm volatile(".1: hlt; jmp .1;");
}



