#include <Ntifs.h>
#include <ntddk.h>
#include <ntstrsafe.h>
#include <wdm.h>

#include "Undocumented.h"

WCHAR pSzTargetProcess[1024];

VOID cbProcessCreated(PEPROCESS PEProcess, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateNotifyInfo);

PCREATE_PROCESS_NOTIFY_ROUTINE_EX pProcessNotifyRoutine = cbProcessCreated;

VOID Unload(PDRIVER_OBJECT DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[StartSuspended] Unloading driver\n"));
	PsSetCreateProcessNotifyRoutineEx(pProcessNotifyRoutine, TRUE);
}

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);
	NTSTATUS status;
	HANDLE hKey;
	OBJECT_ATTRIBUTES objAttrs;
	UNICODE_STRING szValueName;
	ULONG uSize = 0;
	PKEY_VALUE_PARTIAL_INFORMATION pValuePartialInfo;
	ULONG uLenValue;

	DriverObject->DriverUnload = Unload;
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[StartSuspended] Initializing kernel driver!\n"));

	InitializeObjectAttributes(&objAttrs, RegistryPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwOpenKey(&hKey, KEY_READ, &objAttrs);
	if (!NT_SUCCESS(status)) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[StartSuspended] Failed to open registry key\n"));
		return status;
	}
	RtlInitUnicodeString(&szValueName, L"Target");
	status = ZwQueryValueKey(hKey, &szValueName, KeyValuePartialInformation, NULL, 0, &uSize);
	if (!uSize || (status != STATUS_BUFFER_TOO_SMALL && status != STATUS_BUFFER_OVERFLOW)) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[StartSuspended] Failed to query value information for registry key\n"));
		ZwClose(hKey);
		return NT_SUCCESS(status) ? STATUS_BUFFER_TOO_SMALL : status;
	}
	pValuePartialInfo = reinterpret_cast<PKEY_VALUE_PARTIAL_INFORMATION>(ExAllocatePool2(POOL_FLAG_PAGED, uSize, 'kvpi'));
	if (pValuePartialInfo == NULL) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[StartSuspended] Failed to allocate memory for partial information\n"));
		ZwClose(hKey);
		return STATUS_NO_MEMORY;
	}
	status = ZwQueryValueKey(hKey, &szValueName, KeyValuePartialInformation, pValuePartialInfo, uSize, &uLenValue);
	if (!NT_SUCCESS(status)) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[StartSuspended] Failed to query value for registry key (Maybe there is no Target key)\n"));
		ExFreePoolWithTag(pValuePartialInfo, 'kvpi');
		ZwClose(hKey);
		return status;
	}
	if (pValuePartialInfo->Type != REG_SZ) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[StartSuspended] Target has wrong value type\n"));
		ExFreePoolWithTag(pValuePartialInfo, 'kvpi');
		ZwClose(hKey);
		return STATUS_INVALID_PARAMETER;
	}
	RtlStringCchCopyNW(pSzTargetProcess, pValuePartialInfo->DataLength / sizeof(WCHAR), reinterpret_cast<PWSTR>(pValuePartialInfo->Data), 1024);
	if (!wcslen(pSzTargetProcess)) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[StartSuspended] Target is invalid\n"));
		ExFreePoolWithTag(pValuePartialInfo, 'kvpi');
		ZwClose(hKey);
		return STATUS_INVALID_PARAMETER;
	}
	ExFreePoolWithTag(pValuePartialInfo, 'kvpi');
	ZwClose(hKey);
	status = PsSetCreateProcessNotifyRoutineEx(pProcessNotifyRoutine, FALSE);
	if (!NT_SUCCESS(status)) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[StartSuspended] Failed to register process notify routine: %08X\n", status));
		return status;
	}
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[StartSuspended] Successfully registered process notify routine\n"));
	return 0;
}

VOID cbProcessCreated(PEPROCESS PEProcess, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateNotifyInfo) {
	UNREFERENCED_PARAMETER(PEProcess);
	UNREFERENCED_PARAMETER(ProcessId);
	if (CreateNotifyInfo != NULL && wcsstr(CreateNotifyInfo->CommandLine->Buffer, pSzTargetProcess) != NULL) {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[StartSuspended] Suspending %S\n", pSzTargetProcess));
		PsSuspendProcess(PEProcess);
	}
}
