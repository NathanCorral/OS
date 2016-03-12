/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"
 #define DEBUC_PIC 0

/* Interrupt masks to determine which interrupts
 * are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7 */
uint8_t slave_mask; /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void
i8259_init(void)
{
int i;
	//mask all
	master_mask=0xFF;
	slave_mask=0xFF;

	
	//initialize the master
	outb(ICW1, MASTER_8259_PORT);
for(i=0; i<100000;i++);
	outb(ICW1, SLAVE_8259_PORT);
for(i=0; i<100000;i++);


	outb(ICW2_MASTER, MASTER_IMR_PORT);
for(i=0; i<100000;i++);
	outb(ICW2_SLAVE, SLAVE_IMR_PORT);

for(i=0; i<100000;i++);

	outb(ICW3_MASTER, MASTER_IMR_PORT);
	for(i=0; i<100000;i++);
	outb(ICW3_SLAVE, SLAVE_IMR_PORT);
	for(i=0; i<100000;i++);

	outb(ICW4, MASTER_IMR_PORT);
	for(i=0; i<100000;i++);
	outb(ICW4, SLAVE_IMR_PORT);
	for(i=0; i<100000;i++);
	//initialize the slave
	
	

	
	

	
	

	


	outb(master_mask, MASTER_IMR_PORT);
	outb(slave_mask, SLAVE_IMR_PORT);

	//wait
	for(i=0; i<100000;i++);

	



}

/* Enable (unmask) the specified IRQ */
void
enable_irq(uint32_t irq_num)
{
	char mask;
	uint16_t port;
	#ifdef DEBUG_PIC
	int i;
#endif

	if(irq_num<8){
		port=MASTER_IMR_PORT;
		mask = inb(port);
		mask=mask & ~(1 << irq_num);

		outb(mask , port);
	}
	else{
		port=SLAVE_IMR_PORT;
		irq_num -=8;
		mask = inb(SLAVE_IMR_PORT) & ~(1 << irq_num);
		outb(mask, port);
		
	}
	// outb((inb(port) & ~(irq_num*2)), port);
		/*
			0xFF
			2 - 10
			0xF (1101)
		*/

	// char mask = inb(port);
	// mask=mask & ~(1 << irq_num);

	// 	outb(mask , port);
#ifdef DEBUG_PIC
		for(i=0; i<10000; i++); // wait a sec before we check
		i = inb(port);
		if(i & (irq_num << 1) != 0)
			printf("Failed to enable irq \n");
#endif
//mask= inb(port);
//printf("%x\n", mask);
	

}

/* Disable (mask) the specified IRQ */
void
disable_irq(uint32_t irq_num)
{
	uint16_t port;

	if(irq_num<8){
		port=MASTER_IMR_PORT;
	}
	else{
		port=SLAVE_IMR_PORT;
		irq_num -=8;
	}
	outb((inb(port) | (1<<irq_num)), port);
}

/* Send end-of-interrupt signal for the specified IRQ */
void
send_eoi(uint32_t irq_num)
{
	if(irq_num>=8){
		outb((EOI| (irq_num -8)), SLAVE_8259_PORT);
		send_eoi(2);
	}
		else
	outb((EOI | irq_num), MASTER_8259_PORT);

}

