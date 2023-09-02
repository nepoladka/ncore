#pragma once
#include <windows.h>
#include "includes/ntos.h"

#define NtCurrentTeb() ((TEB*)(__readgsqword(FIELD_OFFSET(NT_TIB, Self))))

#define __thread_environment NtCurrentTeb()
#define __process_environment (__thread_environment->ProcessEnvironmentBlock)
#define __process_id DWORD32(__thread_environment->ClientId.UniqueProcess)
#define __thread_id DWORD32(__thread_environment->ClientId.UniqueThread)

#define __current_process HANDLE(-1)
#define __current_thread HANDLE(-2)

#define GetCurrentProcess() __current_process
#define GetCurrentProcessId() __process_id

#define GetCurrentThread() __current_thread
#define GetCurrentThreadId() __thread_id

#define GetCommandLineW() (__process_environment->ProcessParameters->CommandLine.Buffer)

#define GetLastError() DWORD32(__thread_environment->LastErrorValue)
#define SetLastError(VALUE) (__thread_environment->LastErrorValue = VALUE)

#define GetLastStatus() NTSTATUS(__thread_environment->LastStatusValue)
#define SetLastStatus(VALUE) (__thread_environment->LastStatusValue = VALUE)
