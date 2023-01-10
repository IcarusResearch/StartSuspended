#pragma once
#include <ntddk.h>

extern "C" NTSTATUS PsSuspendProcess(IN PEPROCESS Process);
