#ifndef CPUFREQ_H
#define CPUFREQ_H
void cpufreq_init();
void cpufreq_setTarget(uint32_t);
void cpufreq_change(uint32_t);
void cpufreq_boost();
void cpufreq_task();
#endif
