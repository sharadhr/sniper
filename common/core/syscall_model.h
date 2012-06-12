#ifndef SYSCALL_MODEL_H
#define SYSCALL_MODEL_H

#include "fixed_types.h"
#include "subsecond_time.h"

#include <iostream>

class Thread;

class SyscallMdl
{
   public:
      struct syscall_args_t
      {
          IntPtr arg0;
          IntPtr arg1;
          IntPtr arg2;
          IntPtr arg3;
          IntPtr arg4;
          IntPtr arg5;
      };

      SyscallMdl(Thread *thread);
      ~SyscallMdl();

      void runEnter(IntPtr syscall_number, syscall_args_t &args);
      IntPtr runExit(IntPtr old_return);
      bool isEmulated() { return m_emulated; }

   private:
      static const char *futex_names[];

      struct futex_counters_t
      {
         uint64_t count[16];
         SubsecondTime delay[16];
      } *futex_counters;

      Thread *m_thread;
      IntPtr m_syscall_number;
      bool m_emulated;
      IntPtr m_ret_val;

      // ------------------------------------------------------

      IntPtr handleFutexCall(syscall_args_t &args);

      // Helper functions
      void futexCount(uint32_t function, SubsecondTime delay);
};

#endif