#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "config.h"
#include "log.h"
#include "inst_mode.h"


class _Thread;
class SyscallServer;
class SyncServer;
class MagicServer;
class ClockSkewMinimizationServer;
class StatsManager;
class Transport;
class CoreManager;
class Thread;
class ThreadManager;
class SimThreadManager;
class HooksManager;
class ClockSkewMinimizationManager;
class TraceManager;
class DvfsManager;
namespace config { class Config; }

class Simulator
{
public:
   Simulator();
   ~Simulator();

   void start();

   static Simulator* getSingleton() { return m_singleton; }
   static void setConfig(config::Config * cfg, Config::SimulationMode mode);
   static void allocate();
   static void release();

   SyscallServer* getSyscallServer() { return m_syscall_server; }
   SyncServer* getSyncServer() { return m_sync_server; }
   MagicServer* getMagicServer() { return m_magic_server; }
   ClockSkewMinimizationServer* getClockSkewMinimizationServer() { return m_clock_skew_minimization_server; }
   CoreManager *getCoreManager() { return m_core_manager; }
   SimThreadManager *getSimThreadManager() { return m_sim_thread_manager; }
   ThreadManager *getThreadManager() { return m_thread_manager; }
   ClockSkewMinimizationManager *getClockSkewMinimizationManager() { return m_clock_skew_minimization_manager; }
   Config *getConfig() { return &m_config; }
   config::Config *getCfg() {
      //if (! m_config_file_allowed)
      //   LOG_PRINT_ERROR("getCfg() called after init, this is not nice\n");
      return m_config_file;
   }
   void hideCfg() { m_config_file_allowed = false; }
   StatsManager *getStatsManager() { return m_stats_manager; }
   DvfsManager *getDvfsManager() { return m_dvfs_manager; }
   HooksManager *getHooksManager() { return m_hooks_manager; }
   TraceManager *getTraceManager() { return m_trace_manager; }

   static void enablePerformanceModels();
   static void disablePerformanceModels();

   void setInstrumentationMode(InstMode::inst_mode_t new_mode);
   InstMode::inst_mode_t getInstrumentationMode() { return InstMode::inst_mode; }

   void startTimer();
   void stopTimer();
   bool finished();

private:
   Config m_config;
   Log m_log;
   SyscallServer *m_syscall_server;
   SyncServer *m_sync_server;
   MagicServer *m_magic_server;
   ClockSkewMinimizationServer *m_clock_skew_minimization_server;
   StatsManager *m_stats_manager;
   Transport *m_transport;
   CoreManager *m_core_manager;
   ThreadManager *m_thread_manager;
   SimThreadManager *m_sim_thread_manager;
   ClockSkewMinimizationManager *m_clock_skew_minimization_manager;
   TraceManager *m_trace_manager;
   DvfsManager *m_dvfs_manager;
   HooksManager *m_hooks_manager;

   static Simulator *m_singleton;

   std::map<IntPtr, Byte> m_memory;
   Lock m_memory_lock;

   UInt64 m_boot_time;
   UInt64 m_start_time;
   UInt64 m_stop_time;
   UInt64 m_shutdown_time;

   static config::Config *m_config_file;
   static bool m_config_file_allowed;
   static Config::SimulationMode m_mode;
};

__attribute__((unused)) static Simulator *Sim()
{
   return Simulator::getSingleton();
}

#endif // SIMULATOR_H