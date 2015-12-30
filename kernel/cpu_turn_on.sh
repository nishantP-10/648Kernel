#Turn off
echo 0 > /sys/module/cpu_tegra3/parameters/auto_hotplug
# Disable freq scaling for all cpus (recommended to do it in these two steps in this order):
CPU_PATH=/sys/devices/system/cpu
for cpu in 0 1 2 3; do echo 1 > $CPU_PATH/cpu$cpu/online; sleep 1; done
for cpu in 0 1 2 3; do echo performance > $CPU_PATH/cpu$cpu/cpufreq/scaling_governor; done
