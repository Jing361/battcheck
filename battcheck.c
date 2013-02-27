#include <linux/module.h>
#include <linux/kernel.h>
#include <acpi/acpi.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/i387.h>

struct task_struct* ts;
int const CHARGE_MAX = 3369;

int battery_acpi_call(int state, int *warn)
{
   acpi_handle handle;
   union acpi_object *result;
   struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL};
   int cur_charge_state, prv_charge_state, cur_charge_remaining, prv_charge_remaining;

   prv_charge_state = state;

   acpi_get_handle(NULL, (acpi_string)"\\_SB_.BAT0", &handle);
   acpi_evaluate_object(handle, "_BST", NULL, &buffer);
   result = buffer.pointer;

   if(result)
   {
      cur_charge_state = result->package.elements[0].integer.value;
      cur_charge_remaining = result->package.elements[2].integer.value;
      printk(KERN_EMERG "discharging = %d charge remaining = %d\n", cur_charge_state, cur_charge_remaining);
      kfree(result);
      if (prv_charge_state != cur_charge_state)
      {
         if (cur_charge_state == true)
         {
            printk(KERN_INFO "Battery now discharging!\n");
         }
         else if (cur_charge_state == false)
         {
            printk(KERN_INFO "Battery now charging!\n");
         }
      }
      if (cur_charge_remaining < 370  && *warn == 30)
      {
         if ( *warn == 30)
         {
            printk(KERN_INFO "Battery dangerously low!\n");
            warn = 0;
         }
         warn++;
      }
   }
   return cur_charge_state;
}

int *batt_thread(void* data)
{
   int prv_state;
   int warn_val;
   //Previous charging/discharging state to determine whether it has begun charging or discharging recently.
   while(!kthread_should_stop())
   {
      //Prevent spam by sleeping.
      msleep(1000);
      prv_state = battery_acpi_call(prv_state, &warn_val);
   }
   return 0;
}

int __init init_battery_checker(void)
{
   printk(KERN_INFO "Battery check init.\n");

   ts = kthread_run(batt_thread, NULL, "battchecker");

   return 0;
}

void __exit unload_battery_checker(void)
{
   printk(KERN_INFO "Battery check exit.\n");

   kthread_stop(ts);
}

module_init(init_battery_checker);
module_exit(unload_battery_checker);
