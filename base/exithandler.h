#pragma once

GUEST_REGS g_GuestRegs[128];

void HandleCPUID()
{
	ULONG64 uCPUID;
	uCPUID = KeGetCurrentProcessorNumber();
	if (g_GuestRegs[uCPUID].rax == 'Mini')
	{
		g_GuestRegs[uCPUID].rbx = 0x88888888;
		g_GuestRegs[uCPUID].rcx = 0x11111111;
		g_GuestRegs[uCPUID].rdx = 0x12345678;
	}
	else Asm_CPUID(g_GuestRegs[uCPUID].rax,&g_GuestRegs[uCPUID].rax,&g_GuestRegs[uCPUID].rbx,&g_GuestRegs[uCPUID].rcx,&g_GuestRegs[uCPUID].rdx);
}

void HandleInvd()
{
	Asm_Invd();
}

void HandleVmCall()
{
	ULONG64 JmpEIP;
	ULONG64 uCPUID;
	uCPUID = KeGetCurrentProcessorNumber();
	if (g_GuestRegs[uCPUID].rax == 'SVT')
	{
		JmpEIP = g_GuestRegs[uCPUID].rip + Vmx_VmRead(VM_EXIT_INSTRUCTION_LEN);
		Vmx_VmxOff();
		
		Asm_AfterVMXOff(g_GuestRegs[uCPUID].rsp,JmpEIP);
	}
}

void HandleMsrRead()
{
	ULONG64 uCPUID;
	uCPUID = KeGetCurrentProcessorNumber();
	switch(g_GuestRegs[uCPUID].rcx)
	{
	case MSR_IA32_SYSENTER_CS:
		{
			g_GuestRegs[uCPUID].rax = Vmx_VmRead(GUEST_SYSENTER_CS);
			g_GuestRegs[uCPUID].rdx = Vmx_VmRead(GUEST_SYSENTER_CS) >> 32;
			break;
		}
	case MSR_IA32_SYSENTER_ESP:
		{
			g_GuestRegs[uCPUID].rax = Vmx_VmRead(GUEST_SYSENTER_ESP);
			g_GuestRegs[uCPUID].rdx = Vmx_VmRead(GUEST_SYSENTER_ESP) >> 32;
			break;
		}
	case MSR_IA32_SYSENTER_EIP:	// KiFastCallEntry
		{
			g_GuestRegs[uCPUID].rax = Vmx_VmRead(GUEST_SYSENTER_EIP);
			g_GuestRegs[uCPUID].rdx = Vmx_VmRead(GUEST_SYSENTER_EIP) >> 32;
			break;
		}
	case MSR_FS_BASE:
		{
			g_GuestRegs[uCPUID].rax = Vmx_VmRead(GUEST_FS_BASE);
			g_GuestRegs[uCPUID].rdx = Vmx_VmRead(GUEST_FS_BASE) >> 32;
			break;
		}
	case MSR_GS_BASE:
		{
			g_GuestRegs[uCPUID].rax = Vmx_VmRead(GUEST_GS_BASE);
			g_GuestRegs[uCPUID].rdx = Vmx_VmRead(GUEST_GS_BASE) >> 32;
			break;
		}
	case MSR_EFER:
		{
			g_GuestRegs[uCPUID].rax = Asm_ReadMsr(MSR_EFER);
			g_GuestRegs[uCPUID].rdx = Asm_ReadMsr(MSR_EFER) >> 32;
			break;
		}
	default:
		g_GuestRegs[uCPUID].rax = Asm_ReadMsr(g_GuestRegs[uCPUID].rcx);
		g_GuestRegs[uCPUID].rdx = Asm_ReadMsr(g_GuestRegs[uCPUID].rcx) >> 32;
	}

}

void HandleMsrWrite()
{
	ULONG64 uCPUID;
	uCPUID = KeGetCurrentProcessorNumber();
	switch(g_GuestRegs[uCPUID].rcx)
	{
	case MSR_IA32_SYSENTER_CS:
		{
			Vmx_VmWrite(GUEST_SYSENTER_CS,g_GuestRegs[uCPUID].rax|(g_GuestRegs[uCPUID].rdx<<32));
			break;
		}
	case MSR_IA32_SYSENTER_ESP:
		{
			Vmx_VmWrite(GUEST_SYSENTER_ESP,g_GuestRegs[uCPUID].rax|(g_GuestRegs[uCPUID].rdx<<32));
			break;
		}
	case MSR_IA32_SYSENTER_EIP:	// KiFastCallEntry
		{
			Vmx_VmWrite(GUEST_SYSENTER_EIP,g_GuestRegs[uCPUID].rax|(g_GuestRegs[uCPUID].rdx<<32));
			break;
		}
	case MSR_FS_BASE:
		{
			Vmx_VmWrite(GUEST_FS_BASE,(g_GuestRegs[uCPUID].rax)|(g_GuestRegs[uCPUID].rdx<<32));
			break;
		}
	case MSR_GS_BASE:
		{
			Vmx_VmWrite(GUEST_GS_BASE,(g_GuestRegs[uCPUID].rax)|(g_GuestRegs[uCPUID].rdx<<32));
			break;
		}
	case MSR_EFER:
		{
			Asm_WriteMsr(MSR_EFER,(g_GuestRegs[uCPUID].rax)|(g_GuestRegs[uCPUID].rdx<<32));
			break;
		}
	default:
		Asm_WriteMsr(g_GuestRegs[uCPUID].rcx,(g_GuestRegs[uCPUID].rax)|(g_GuestRegs[uCPUID].rdx<<32));
	}
}

void HandleCrAccess()
{
	ULONG64		movcrControlRegister;
	ULONG64		movcrAccessType;
	ULONG64		movcrOperandType;
	ULONG64		movcrGeneralPurposeRegister;
	ULONG64		movcrLMSWSourceData;
	ULONG64		ExitQualification;
	ULONG64 uCPUID;
	uCPUID = KeGetCurrentProcessorNumber();

	ExitQualification = Vmx_VmRead(EXIT_QUALIFICATION) ;
	movcrControlRegister = ( ExitQualification & 0x0000000F );
	movcrAccessType = ( ( ExitQualification & 0x00000030 ) >> 4 );
	movcrOperandType = ( ( ExitQualification & 0x00000040 ) >> 6 );
	movcrGeneralPurposeRegister = ( ( ExitQualification & 0x00000F00 ) >> 8 );

	//	Control Register Access (CR3 <-- reg32)
	//
	if( movcrControlRegister == 3 && movcrAccessType == 0 && movcrOperandType == 0 && movcrGeneralPurposeRegister == 0 )
	{
		Vmx_VmWrite( GUEST_CR3, g_GuestRegs[uCPUID].rax );
	}
	if( movcrControlRegister == 3 && movcrAccessType == 0 && movcrOperandType == 0 && movcrGeneralPurposeRegister == 1 )
	{
		Vmx_VmWrite( GUEST_CR3, g_GuestRegs[uCPUID].rcx );
	}
	if( movcrControlRegister == 3 && movcrAccessType == 0 && movcrOperandType == 0 && movcrGeneralPurposeRegister == 2 )
	{
		Vmx_VmWrite( GUEST_CR3, g_GuestRegs[uCPUID].rdx );
	}
	if( movcrControlRegister == 3 && movcrAccessType == 0 && movcrOperandType == 0 && movcrGeneralPurposeRegister == 3 )
	{
		Vmx_VmWrite( GUEST_CR3, g_GuestRegs[uCPUID].rbx );
	}
	if( movcrControlRegister == 3 && movcrAccessType == 0 && movcrOperandType == 0 && movcrGeneralPurposeRegister == 4 )
	{
		Vmx_VmWrite( GUEST_CR3, g_GuestRegs[uCPUID].rsp );
	}
	if( movcrControlRegister == 3 && movcrAccessType == 0 && movcrOperandType == 0 && movcrGeneralPurposeRegister == 5 )
	{
		Vmx_VmWrite( GUEST_CR3, g_GuestRegs[uCPUID].rbp );
	}
	if( movcrControlRegister == 3 && movcrAccessType == 0 && movcrOperandType == 0 && movcrGeneralPurposeRegister == 6 )
	{
		Vmx_VmWrite( GUEST_CR3, g_GuestRegs[uCPUID].rsi );
	}
	if( movcrControlRegister == 3 && movcrAccessType == 0 && movcrOperandType == 0 && movcrGeneralPurposeRegister == 7 )
	{
		Vmx_VmWrite( GUEST_CR3, g_GuestRegs[uCPUID].rdi );
	}
	//	Control Register Access (reg32 <-- CR3)
	//
	if( movcrControlRegister == 3 && movcrAccessType == 1 && movcrOperandType == 0 && movcrGeneralPurposeRegister == 0 )
	{
		g_GuestRegs[uCPUID].rax = g_GuestRegs[uCPUID].cr3;
	}
	if( movcrControlRegister == 3 && movcrAccessType == 1 && movcrOperandType == 0 && movcrGeneralPurposeRegister == 1 )
	{
		g_GuestRegs[uCPUID].rcx = g_GuestRegs[uCPUID].cr3;
	}
	if( movcrControlRegister == 3 && movcrAccessType == 1 && movcrOperandType == 0 && movcrGeneralPurposeRegister == 2 )
	{
		g_GuestRegs[uCPUID].rdx = g_GuestRegs[uCPUID].cr3;
	}
	if( movcrControlRegister == 3 && movcrAccessType == 1 && movcrOperandType == 0 && movcrGeneralPurposeRegister == 3 )
	{
		g_GuestRegs[uCPUID].rbx = g_GuestRegs[uCPUID].cr3;
	}
	if( movcrControlRegister == 3 && movcrAccessType == 1 && movcrOperandType == 0 && movcrGeneralPurposeRegister == 4 )
	{
		g_GuestRegs[uCPUID].rsp = g_GuestRegs[uCPUID].cr3;
	}
	if( movcrControlRegister == 3 && movcrAccessType == 1 && movcrOperandType == 0 && movcrGeneralPurposeRegister == 5 )
	{
		g_GuestRegs[uCPUID].rbp = g_GuestRegs[uCPUID].cr3;
	}
	if( movcrControlRegister == 3 && movcrAccessType == 1 && movcrOperandType == 0 && movcrGeneralPurposeRegister == 6 )
	{
		g_GuestRegs[uCPUID].rsi = g_GuestRegs[uCPUID].cr3;
	}
	if( movcrControlRegister == 3 && movcrAccessType == 1 && movcrOperandType == 0 && movcrGeneralPurposeRegister == 7 )
	{
		g_GuestRegs[uCPUID].rdi = g_GuestRegs[uCPUID].cr3;
	}  
}

extern "C" ULONG64 GetGuestRegsAddress()
{
	ULONG64 uCPUID;
	uCPUID = KeGetCurrentProcessorNumber();
	return (ULONG64)&g_GuestRegs[uCPUID];
}

extern "C" void VMMEntryPoint()
{
	ULONG64 ExitReason;
	ULONG64 ExitInstructionLength;
	ULONG64 GuestResumeEIP;
	ULONG64 uCPUID;
	static BOOLEAN bWatch = FALSE;
	uCPUID = KeGetCurrentProcessorNumber();
	ExitReason = Vmx_VmRead(VM_EXIT_REASON);
	ExitInstructionLength = Vmx_VmRead(VM_EXIT_INSTRUCTION_LEN);

	g_GuestRegs[uCPUID].rsp = Vmx_VmRead(GUEST_RSP);
	g_GuestRegs[uCPUID].rip = Vmx_VmRead(GUEST_RIP);
	g_GuestRegs[uCPUID].cr3 = Vmx_VmRead(GUEST_CR3);

	if (bWatch)
		__debugbreak();
	switch(ExitReason)
	{
	case EXIT_REASON_CPUID:
		{
			HandleCPUID();
			break;
		}
	case EXIT_REASON_INVD:
		{
			HandleInvd();
			break;
		}
	case EXIT_REASON_VMCALL:
		{
			HandleVmCall();
			break;
		}
	case EXIT_REASON_MSR_READ:
		{
			HandleMsrRead();
			break;
		}
	case EXIT_REASON_MSR_WRITE:
		{
			HandleMsrWrite();
			break;
		}
	case EXIT_REASON_CR_ACCESS:
		{
			HandleCrAccess();
			break;
		}
	default:
		break;
	}

Resume:
	GuestResumeEIP = g_GuestRegs[uCPUID].rip+ExitInstructionLength;
	Vmx_VmWrite(GUEST_RIP,GuestResumeEIP);
	Vmx_VmWrite(GUEST_RSP,g_GuestRegs[uCPUID].rsp);
}