#include "stdafx.h"
#include "vtsystem.h"
#include "vtasm.h"
#include "exithandler.h"
#include "common.h"

VMX_CPU g_VMXCPU[128];

KMUTEX g_GlobalMutex;

NTSTATUS AllocateVMXRegion()
{
	PVOID pVMXONRegion;
	PVOID pVMCSRegion;
	PVOID pHostEsp;
	ULONG64 uCPUID;

	uCPUID = KeGetCurrentProcessorNumber();
	pVMXONRegion = ExAllocatePoolWithTag(NonPagedPool,0x1000,'vmon'); //4KB
	if (!pVMXONRegion)
	{
		Log("ERROR:����VMXON�ڴ�����ʧ��!",0);
		return STATUS_MEMORY_NOT_ALLOCATED;
	}
	RtlZeroMemory(pVMXONRegion,0x1000);

	pVMCSRegion = ExAllocatePoolWithTag(NonPagedPool,0x1000,'vmcs');
	if (!pVMCSRegion)
	{
		Log("ERROR:����VMCS�ڴ�����ʧ��!",0);
		ExFreePoolWithTag(pVMXONRegion,0x1000);
		return STATUS_MEMORY_NOT_ALLOCATED;
	}
	RtlZeroMemory(pVMCSRegion,0x1000);

	pHostEsp = ExAllocatePoolWithTag(NonPagedPool,0x2000,'mini');
	if (!pHostEsp)
	{
		Log("ERROR:������������������ʧ��!",0);
		ExFreePoolWithTag(pVMXONRegion,0x1000);
		ExFreePoolWithTag(pVMCSRegion,0x1000);
		return STATUS_MEMORY_NOT_ALLOCATED;
	}
	RtlZeroMemory(pHostEsp,0x2000);

	Log("TIP:VMXON�ڴ������ַ",pVMXONRegion);
	Log("TIP:VMCS�ڴ������ַ",pVMCSRegion);
	Log("TIP:���������������ַ",pHostEsp);

	g_VMXCPU[uCPUID].pVMXONRegion = pVMXONRegion;
	g_VMXCPU[uCPUID].pVMXONRegion_PA = MmGetPhysicalAddress(pVMXONRegion);
	g_VMXCPU[uCPUID].pVMCSRegion = pVMCSRegion;
	g_VMXCPU[uCPUID].pVMCSRegion_PA = MmGetPhysicalAddress(pVMCSRegion);
	g_VMXCPU[uCPUID].pHostEsp = pHostEsp;
	return STATUS_SUCCESS;
}

void SetupVMXRegion()
{
	VMX_BASIC_MSR Msr;
	ULONG64 uRevId;
	_CR4 uCr4;
	_EFLAGS uEflags;
	ULONG64 uCPUID;

	uCPUID = KeGetCurrentProcessorNumber();

	RtlZeroMemory(&Msr,sizeof(Msr));

	*((PULONG64)&Msr) = Asm_ReadMsr(MSR_IA32_VMX_BASIC);
	uRevId = Msr.RevId;

	*((PULONG)g_VMXCPU[uCPUID].pVMXONRegion) = uRevId;
	*((PULONG)g_VMXCPU[uCPUID].pVMCSRegion) = uRevId;

	Log("TIP:VMX�汾����Ϣ",uRevId);

	*((PULONG64)&uCr4) = Asm_GetCr4();
	uCr4.VMXE = 1;
	Asm_SetCr4(*((PULONG64)&uCr4));

	Vmx_VmxOn(g_VMXCPU[uCPUID].pVMXONRegion_PA.QuadPart);
	*((PULONG64)&uEflags) = Asm_GetRflags();
	if (uEflags.CF != 0)
	{
		Log("ERROR:VMXONָ�����ʧ��!",0);
		return;
	}
	Log("SUCCESS:VMXONָ����óɹ�!",0);
}

extern "C" void SetupVMCS()
{
	_EFLAGS uEflags;
	ULONG64 GdtBase,IdtBase;
	SEGMENT_SELECTOR SegmentSelector;
	ULONG64 uCPUBase,uExceptionBitmap;
	ULONG64 uCPUID;

	uCPUID = KeGetCurrentProcessorNumber();

	Vmx_VmClear(g_VMXCPU[uCPUID].pVMCSRegion_PA.QuadPart);
	*((PULONG64)&uEflags) = Asm_GetRflags();
	if (uEflags.CF != 0 || uEflags.ZF != 0)
	{
		Log("ERROR:VMCLEARָ�����ʧ��!",0);
		return;
	}
	Log("SUCCESS:VMCLEARָ����óɹ�!",0);
	Vmx_VmPtrld(g_VMXCPU[uCPUID].pVMCSRegion_PA.QuadPart);

	GdtBase = Asm_GetGdtBase();
	IdtBase = Asm_GetIdtBase();

	//
	// 1.Guest State Area
	//
	Vmx_VmWrite(GUEST_CR0,Asm_GetCr0());
	Vmx_VmWrite(GUEST_CR3,Asm_GetCr3());
	Vmx_VmWrite(GUEST_CR4,Asm_GetCr4());

	Vmx_VmWrite(GUEST_DR7,0x400);
	Vmx_VmWrite(GUEST_RFLAGS,Asm_GetRflags());

	FillGuestSelectorData(GdtBase,ES,Asm_GetEs());
	FillGuestSelectorData(GdtBase,FS,Asm_GetFs());
	FillGuestSelectorData(GdtBase,DS,Asm_GetDs());
	FillGuestSelectorData(GdtBase,CS,Asm_GetCs());
	FillGuestSelectorData(GdtBase,SS,Asm_GetSs());
	FillGuestSelectorData(GdtBase,GS,Asm_GetGs());
	FillGuestSelectorData(GdtBase,TR,Asm_GetTr());
	FillGuestSelectorData(GdtBase,LDTR,Asm_GetLdtr());

	Vmx_VmWrite(GUEST_CS_BASE,0);
	Vmx_VmWrite(GUEST_DS_BASE,0);
	Vmx_VmWrite(GUEST_ES_BASE,0);
	Vmx_VmWrite(GUEST_SS_BASE,0);
	Vmx_VmWrite(GUEST_FS_BASE,Asm_ReadMsr(MSR_FS_BASE));
	Vmx_VmWrite(GUEST_GS_BASE,Asm_ReadMsr(MSR_GS_BASE));
	Vmx_VmWrite(GUEST_GDTR_BASE,GdtBase);
	Vmx_VmWrite(GUEST_GDTR_LIMIT,Asm_GetGdtLimit());
	Vmx_VmWrite(GUEST_IDTR_BASE,IdtBase);
	Vmx_VmWrite(GUEST_IDTR_LIMIT,Asm_GetIdtLimit());

	Vmx_VmWrite(GUEST_IA32_DEBUGCTL,Asm_ReadMsr(MSR_IA32_DEBUGCTL));
	Vmx_VmWrite(GUEST_IA32_DEBUGCTL_HIGH,Asm_ReadMsr(MSR_IA32_DEBUGCTL)>>32);
	Vmx_VmWrite(GUEST_IA32_EFER,Asm_ReadMsr(MSR_EFER));

	Vmx_VmWrite(GUEST_SYSENTER_CS,Asm_ReadMsr(MSR_IA32_SYSENTER_CS));
	Vmx_VmWrite(GUEST_SYSENTER_ESP,Asm_ReadMsr(MSR_IA32_SYSENTER_ESP));
	Vmx_VmWrite(GUEST_SYSENTER_EIP,Asm_ReadMsr(MSR_IA32_SYSENTER_EIP)); // KiFastCallEntry

	Vmx_VmWrite(GUEST_RSP,Asm_GetGuestRSP());
	Vmx_VmWrite(GUEST_RIP,Asm_GetGuestReturn());// ָ��vmlaunch�ͻ�������ڵ� ���������ÿͻ�������ִ�м��������Ĵ���

	Vmx_VmWrite(GUEST_INTERRUPTIBILITY_INFO, 0);
	Vmx_VmWrite(GUEST_ACTIVITY_STATE, 0);
	Vmx_VmWrite(VMCS_LINK_POINTER, 0xffffffff);
	Vmx_VmWrite(VMCS_LINK_POINTER_HIGH, 0xffffffff);

	//
	// 2.Host State Area
	//
	Vmx_VmWrite(HOST_CR0,Asm_GetCr0());
	Vmx_VmWrite(HOST_CR3,Asm_GetCr3());
	Vmx_VmWrite(HOST_CR4,Asm_GetCr4());

	Vmx_VmWrite(HOST_ES_SELECTOR,KGDT64_R0_DATA);
	Vmx_VmWrite(HOST_CS_SELECTOR,KGDT64_R0_CODE);
	Vmx_VmWrite(HOST_SS_SELECTOR,KGDT64_R0_DATA);
	Vmx_VmWrite(HOST_DS_SELECTOR,KGDT64_R0_DATA);
	Vmx_VmWrite(HOST_FS_SELECTOR,(Asm_GetFs()&0xF8));
	Vmx_VmWrite(HOST_GS_SELECTOR,(Asm_GetGs()&0xF8));
	Vmx_VmWrite(HOST_TR_SELECTOR,(Asm_GetTr()&0xF8));

	Vmx_VmWrite(HOST_FS_BASE,Asm_ReadMsr(MSR_FS_BASE));
	Vmx_VmWrite(HOST_GS_BASE,Asm_ReadMsr(MSR_GS_BASE));
	InitializeSegmentSelector(&SegmentSelector,Asm_GetTr(),GdtBase);
	Vmx_VmWrite(HOST_TR_BASE,SegmentSelector.base);

	Vmx_VmWrite(HOST_GDTR_BASE,GdtBase);
	Vmx_VmWrite(HOST_IDTR_BASE,IdtBase);

	Vmx_VmWrite(HOST_IA32_EFER,Asm_ReadMsr(MSR_EFER));
	Vmx_VmWrite(HOST_IA32_SYSENTER_CS,Asm_ReadMsr(MSR_IA32_SYSENTER_CS));
	Vmx_VmWrite(HOST_IA32_SYSENTER_ESP,Asm_ReadMsr(MSR_IA32_SYSENTER_ESP));
	Vmx_VmWrite(HOST_IA32_SYSENTER_EIP,Asm_ReadMsr(MSR_IA32_SYSENTER_EIP)); // KiFastCallEntry

	Vmx_VmWrite(HOST_RSP,((ULONG64)g_VMXCPU[uCPUID].pHostEsp) + 0x1FFF);//8KB 0x2000
	Vmx_VmWrite(HOST_RIP,(ULONG64)&Asm_VMMEntryPoint);//���ﶨ�����ǵ�VMM����������

	//
	// 3.��������п�����
	//
	Vmx_VmWrite(PIN_BASED_VM_EXEC_CONTROL,VmxAdjustControls(0,MSR_IA32_VMX_PINBASED_CTLS));

	Vmx_VmWrite(PAGE_FAULT_ERROR_CODE_MASK,0);
	Vmx_VmWrite(PAGE_FAULT_ERROR_CODE_MATCH,0);
	Vmx_VmWrite(TSC_OFFSET,0);
	Vmx_VmWrite(TSC_OFFSET_HIGH,0);

	uCPUBase = VmxAdjustControls(0,MSR_IA32_VMX_PROCBASED_CTLS);

	//uCPUBase |= CPU_BASED_MOV_DR_EXITING; // ���ص��ԼĴ�������
	//uCPUBase |= CPU_BASED_USE_IO_BITMAPS; // ���ؼ��������Ϣ
	//uCPUBase |= CPU_BASED_ACTIVATE_MSR_BITMAP; // ����MSR����

	Vmx_VmWrite(CPU_BASED_VM_EXEC_CONTROL,uCPUBase);

	/*
	Vmx_VmWrite(IO_BITMAP_A,0);
	Vmx_VmWrite(IO_BITMAP_A_HIGH,0);
	Vmx_VmWrite(IO_BITMAP_B,0);
	Vmx_VmWrite(IO_BITMAP_B_HIGH,0);
	*/

	Vmx_VmWrite(CR3_TARGET_COUNT,0);
	Vmx_VmWrite(CR3_TARGET_VALUE0,0);
	Vmx_VmWrite(CR3_TARGET_VALUE1,0);
	Vmx_VmWrite(CR3_TARGET_VALUE2,0);
	Vmx_VmWrite(CR3_TARGET_VALUE3,0);

	//
	// 4.VMEntry���п�����
	//
	Vmx_VmWrite(VM_ENTRY_CONTROLS,VmxAdjustControls(VM_ENTRY_IA32E_MODE|VM_ENTRY_LOAD_IA32_EFER,MSR_IA32_VMX_ENTRY_CTLS));
	Vmx_VmWrite(VM_ENTRY_MSR_LOAD_COUNT,0);
	Vmx_VmWrite(VM_ENTRY_INTR_INFO_FIELD,0);


	//
	// 5.VMExit���п�����
	//
	Vmx_VmWrite(VM_EXIT_CONTROLS,VmxAdjustControls(VM_EXIT_IA32E_MODE|VM_EXIT_ACK_INTR_ON_EXIT,MSR_IA32_VMX_EXIT_CTLS));
	Vmx_VmWrite(VM_EXIT_MSR_LOAD_COUNT,0);
	Vmx_VmWrite(VM_EXIT_MSR_STORE_COUNT,0);

	Vmx_VmLaunch();

	g_VMXCPU[uCPUID].bVTStartSuccess = FALSE;

	Log("ERROR:VmLaunchָ�����ʧ��!",Vmx_VmRead(VM_INSTRUCTION_ERROR));
}

void SetupVT()
{
	ULONG64 uCPUID;

	uCPUID = KeGetCurrentProcessorNumber();
	NTSTATUS status = STATUS_SUCCESS;
	if (!IsVTEnabled())
		return;

	status = AllocateVMXRegion();
	if (!NT_SUCCESS(status))
	{
		Log("ERROR:VMX�ڴ���������ʧ��",0);
		return;
	}
	Log("SUCCESS:VMX�ڴ���������ɹ�!",0);

	SetupVMXRegion();
	g_VMXCPU[uCPUID].bVTStartSuccess = TRUE;

	Asm_SetupVMCS();

	if (g_VMXCPU[uCPUID].bVTStartSuccess)
	{
		Log("SUCCESS:����VT�ɹ�!",uCPUID);
		Log("SUCCESS:�������CPU������VMXģʽ.",0);
		return;
	}
	else Log("ERROR:����VTʧ��!",0);
}

void UnsetupVT()
{
	_CR4 uCr4;
	ULONG64 uCPUID;
	uCPUID = KeGetCurrentProcessorNumber();
	if(g_VMXCPU[uCPUID].bVTStartSuccess)
	{
		Vmx_VmCall('SVT');

		*((PULONG64)&uCr4) = Asm_GetCr4();
		uCr4.VMXE = 0;
		Asm_SetCr4(*((PULONG64)&uCr4));

		ExFreePoolWithTag(g_VMXCPU[uCPUID].pVMXONRegion,'vmon');
		ExFreePoolWithTag(g_VMXCPU[uCPUID].pVMCSRegion,'vmcs');
		ExFreePoolWithTag(g_VMXCPU[uCPUID].pHostEsp,'mini');

		Log("SUCCESS:�ر�VT�ɹ�!",uCPUID);
		Log("SUCCESS:�������CPU�˳���VMXģʽ.",0);
	}
}

NTSTATUS StartVirtualTechnology()
{
	NTSTATUS status = STATUS_SUCCESS;
	KIRQL OldIrql;

	KeInitializeMutex(&g_GlobalMutex,0);
	KeWaitForMutexObject(&g_GlobalMutex,Executive,KernelMode,FALSE,0);

	for (int i = 0;i<KeNumberProcessors;i++)
	{
		KeSetSystemAffinityThread((KAFFINITY)(1 << i));
		Log("��������������Ͽ���VT",KeGetCurrentProcessorNumber());

		OldIrql = KeRaiseIrqlToDpcLevel();
		
		SetupVT();

		KeLowerIrql(0);

		KeRevertToUserAffinityThread();
	}

	KeReleaseMutex(&g_GlobalMutex,FALSE);

	return STATUS_UNSUCCESSFUL;
}

NTSTATUS StopVirtualTechnology()
{
	KIRQL OldIrql;

	KeInitializeMutex(&g_GlobalMutex,0);
	KeWaitForMutexObject(&g_GlobalMutex,Executive,KernelMode,FALSE,0);

	for (int i = 0;i<KeNumberProcessors;i++)
	{
		KeSetSystemAffinityThread((KAFFINITY)(1 << i));
		Log("��������������Ϲر�VT",KeGetCurrentProcessorNumber());

		OldIrql = KeRaiseIrqlToDpcLevel();

		UnsetupVT();

		KeLowerIrql(0);

		KeRevertToUserAffinityThread();
	}

	KeReleaseMutex(&g_GlobalMutex,FALSE);

	return STATUS_SUCCESS;
}

BOOLEAN IsVTEnabled()
{
	ULONG64 uRet_EAX,uRet_ECX,uRet_EDX,uRet_EBX;
	_CPUID_ECX uCPUID;
	_CR0 uCr0;
	_CR4 uCr4;
	IA32_FEATURE_CONTROL_MSR msr;
	//1. CPUID
	Asm_CPUID(1,&uRet_EAX,&uRet_EBX,&uRet_ECX,&uRet_EDX);
	*((PULONG64)&uCPUID) = uRet_ECX;

	if (uCPUID.VMX != 1)
	{
		Log("ERROR:���CPU��֧��VT!",0);
		return FALSE;
	}

	// 2. CR0 CR4
	*((PULONG64)&uCr0) = Asm_GetCr0();
	*((PULONG64)&uCr4) = Asm_GetCr4();

	if (uCr0.PE != 1||uCr0.PG!=1||uCr0.NE!=1)
	{
		Log("ERROR:���CPUû�п���VT!",0);
		return FALSE;
	}

	if (uCr4.VMXE == 1)
	{
		Log("ERROR:���CPU�Ѿ�������VT!",0);
		Log("�����Ǳ�������Ѿ�ռ����VT�������ر�������ܿ�����",0);
		return FALSE;
	}

	// 3. MSR
	*((PULONG64)&msr) = Asm_ReadMsr(MSR_IA32_FEATURE_CONTROL);
	if (msr.Lock!=1)
	{
		Log("ERROR:VTָ��δ������!",0);
		return FALSE;
	}
	Log("SUCCESS:���CPU֧��VT!",0);
	return TRUE;
}