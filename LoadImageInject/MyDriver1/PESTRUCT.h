#pragma once
#ifndef _PESTRUCT_H
#define _PESTRUCT_H

#include <ntimage.h>
#include "SSDT.h"

BOOLEAN IsPEFile(ULONG_PTR ImageBase)
{
	IMAGE_DOS_HEADER * DosHeader = (IMAGE_DOS_HEADER *)ImageBase;
	if (DosHeader->e_magic == IMAGE_DOS_SIGNATURE)
	{
		IMAGE_NT_HEADERS64 *NtHeader = (IMAGE_NT_HEADERS64 *)(ImageBase + DosHeader->e_lfanew);
		if (NtHeader->Signature == IMAGE_NT_SIGNATURE)
			return TRUE;
	}

	return FALSE;
}

/*��ģ��ص��У����˽�����ģ����ص�ʱ��������ģ���ʱ����̻���Eprocess��������֮���ٲ����ý����ڴ棨�����ȡд�룩��ʱ��Ҳ���������˻�����������*/
VOID InjectDll(ULONG_PTR ImageBase)
{
	IMAGE_DOS_HEADER * DosHeader = (IMAGE_DOS_HEADER *)ImageBase;
	IMAGE_NT_HEADERS64 *NtHeader = (IMAGE_NT_HEADERS64 *)(ImageBase + DosHeader->e_lfanew);
	NTSTATUS status;
	PVOID AllocateBase = NULL;						//ImageBase + NtHeader->OptionalHeader.SizeOfImage
	ULONG_PTR RegionSize = NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);		//�Ȳ鿴��ǰ�ж��ٸ�IMAGE_IMPORT_DESCRIPTOR
	RegionSize = (RegionSize + 1) * sizeof(IMAGE_IMPORT_DESCRIPTOR);																		//����һ������������ڴ�
	RegionSize = RegionSize + 0x82c;																										//��������·�����ַ���������

	for (ULONG i = 0; i < 1000; ++i)
	{
		AllocateBase = (PVOID)(ImageBase + (i << 12));

		status = ZwAllocateVirtualMemory(NtCurrentProcess(), &AllocateBase, 0, &RegionSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (NT_SUCCESS(status))
			break;
	}

	RtlZeroMemory(AllocateBase, RegionSize);													//����ڴ�

	RtlCopyMemory((PVOID)((ULONG_PTR)AllocateBase + sizeof(IMAGE_IMPORT_DESCRIPTOR)),
		(PVOID)(NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + ImageBase),
		NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size);				//�����ڴ�

	IMAGE_THUNK_DATA64 *ThunkData = (IMAGE_THUNK_DATA64 *)((ULONG_PTR)AllocateBase +
		NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size + 0x830);		//0x830���Ƿ����ַ��ĩβ��ȥ����IMAGE_THUNK_DATA��λ��
	ThunkData->u1.Function = 0x8000000000000001;												//����һ��ΪNULL��ʾ��β�������ڶ���Ϊ0x8000000000000001

	strcpy(((char *)(ULONG_PTR)AllocateBase + sizeof(IMAGE_IMPORT_DESCRIPTOR) + NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size), "C:\\Users\\t.dll");

	//��������ʼ�����������IMAGE_IMPORT_DESCRIPTOR
	IMAGE_IMPORT_DESCRIPTOR *MyImportDescriptor = (IMAGE_IMPORT_DESCRIPTOR*)AllocateBase;
	MyImportDescriptor->FirstThunk = (ULONG)((ULONG_PTR)ThunkData - ImageBase);
	MyImportDescriptor->OriginalFirstThunk = (ULONG)((ULONG_PTR)ThunkData - ImageBase);
	MyImportDescriptor->Name = (ULONG)((ULONG_PTR)AllocateBase + sizeof(IMAGE_IMPORT_DESCRIPTOR) + NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size - ImageBase);

	//���ﲻ����ʹ��ֱ���޸�CR0���޸��ڴ�
	//��Ϊ���ֱ���޸�CR0�������޸ĵ�������ҳ������ݣ�������ӳ������������ַ
	//֮��ļ��ض������
	if (Local_ProtectVirtualMemory(&NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT], 8, PAGE_EXECUTE_READWRITE))
	{
		NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size += sizeof(IMAGE_IMPORT_DESCRIPTOR);
		NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = (ULONG)((ULONG_PTR)MyImportDescriptor - ImageBase);
	}
	if (Local_ProtectVirtualMemory(&NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT], 8, PAGE_EXECUTE_READWRITE))
	{
		NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = 0;
		NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0;
	}
}

#endif
