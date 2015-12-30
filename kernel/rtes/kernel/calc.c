/*
 * sys_call for the calculator
 * author : Mengye Gong
 */
#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm/div64.h>

asmlinkage int sys_calc(char *str1, char *str2, char operation, char *result) {
	printk("begin calc\n");

	long *operand1 = (long *)kmalloc(sizeof(long), GFP_KERNEL);
	long *operand2 = (long *)kmalloc(sizeof(long), GFP_KERNEL);
//	int op1 = kstrtol(str1, 10, operand1);
//	int op2 = kstrtol(str2, 10, operand2);

	printk("op1: %d\n", (int)*operand1);
	printk("op2: %d\n", (int)*operand2);

	int resultInt;
	switch(operation) {
		case '+':
			resultInt = (int)*operand1 + (int)*operand2;
			printk("Finish calc add\n");
			break;
		case '-':
			resultInt = (int)*operand1 - (int)*operand2;
			printk("Finish calc sub\n");
			break;
		case '*':
			resultInt = (int)(((long long)(*operand1) * (long long)(*operand2)) >> 12);
			printk("Finish calc multiple: %d\n", resultInt);
			break;
		case '/':
			printk("begin devide\n");
			if(*operand2 == 0) {
				return 0;
			}
			else {
				long long operand1long = ((long long)(*operand1)) << 12;
				long long operand2long = (long long)(*operand2);
				printk("ope1: %lld\n", operand1long);
				printk("ope2: %lld\n", operand2long);
				do_div(operand1long, operand2long);
				resultInt = (int)operand1long;
				printk("Finish calc divide: %lld\n", operand1long);
			}
			break;
		default:
			resultInt = 0;
			break;
	}
	sprintf(result, "%d", resultInt);	

	return 1;
}
