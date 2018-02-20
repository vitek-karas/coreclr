// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
// precode.cpp
//

//
// Stub that runs before the actual native code
//


#include "common.h"

#ifdef FEATURE_PREJIT
#include "compile.h"
#endif

#ifdef FEATURE_PERFMAP
#include "perfmap.h"
#endif

//==========================================================================================
// class Precode
//==========================================================================================
BOOL Precode::IsValidType(PrecodeType t)
{
    LIMITED_METHOD_CONTRACT;
    SUPPORTS_DAC;

    switch (t) {
    case PRECODE_STUB:
#ifdef HAS_NDIRECT_IMPORT_PRECODE
    case PRECODE_NDIRECT_IMPORT:
#endif // HAS_NDIRECT_IMPORT_PRECODE
#ifdef HAS_REMOTING_PRECODE
    case PRECODE_REMOTING:
#endif // HAS_REMOTING_PRECODE
#ifdef HAS_FIXUP_PRECODE
    case PRECODE_FIXUP:
#endif // HAS_FIXUP_PRECODE
#ifdef HAS_THISPTR_RETBUF_PRECODE
    case PRECODE_THISPTR_RETBUF:
#endif // HAS_THISPTR_RETBUF_PRECODE
        return TRUE;
    default:
        return FALSE;
    }
}

SIZE_T Precode::SizeOf(PrecodeType t)
{
    LIMITED_METHOD_CONTRACT;
    SUPPORTS_DAC;

    switch (t)
    {
    case PRECODE_STUB:
        return sizeof(StubPrecode);
#ifdef HAS_NDIRECT_IMPORT_PRECODE
    case PRECODE_NDIRECT_IMPORT:
        return sizeof(NDirectImportPrecode);
#endif // HAS_NDIRECT_IMPORT_PRECODE
#ifdef HAS_REMOTING_PRECODE
    case PRECODE_REMOTING:
        return sizeof(RemotingPrecode);
#endif // HAS_REMOTING_PRECODE
#ifdef HAS_FIXUP_PRECODE
    case PRECODE_FIXUP:
        return sizeof(FixupPrecode);
#endif // HAS_FIXUP_PRECODE
#ifdef HAS_THISPTR_RETBUF_PRECODE
    case PRECODE_THISPTR_RETBUF:
        return sizeof(ThisPtrRetBufPrecode);
#endif // HAS_THISPTR_RETBUF_PRECODE

    default:
        UnexpectedPrecodeType("Precode::SizeOf", t);
        break;
    }
    return 0;
}

BOOL Precode::IsZapped()
{
    LIMITED_METHOD_CONTRACT;
    SUPPORTS_DAC;

#ifdef FEATURE_PREJIT
    PTR_MethodDesc pMD = (PTR_MethodDesc)GetMethodDesc();
    Module * pZapModule = pMD->GetZapModule();
    return (pZapModule != NULL) && pZapModule->IsZappedPrecode((PCODE)this);
#else
    return FALSE;
#endif
}

// Note: This is immediate target of the precode. It does not follow jump stub if there is one.
PCODE Precode::GetTarget()
{
    LIMITED_METHOD_CONTRACT;
    SUPPORTS_DAC;

    PCODE target = NULL;

    PrecodeType precodeType = GetType();
    switch (precodeType)
    {
    case PRECODE_STUB:
#ifdef FEATURE_PREJIT
        if (IsZapped())
        {
            target = GetMethodDesc()->GetMethodEntryPoint();
            if (target != this->GetEntryPoint())
            {
                break;
            }
        }
#endif
        target = AsStubPrecode()->GetTarget();
        break;
#ifdef HAS_REMOTING_PRECODE
    case PRECODE_REMOTING:
#ifdef FEATURE_PREJIT
        if (IsZapped())
        {
            target = GetMethodDesc()->GetMethodEntryPoint();
            if (target != this->GetEntryPoint())
            {
                break;
            }
        }
#endif
        target = AsRemotingPrecode()->GetTarget();
        break;
#endif // HAS_REMOTING_PRECODE
#ifdef HAS_FIXUP_PRECODE
    case PRECODE_FIXUP:
#ifdef FEATURE_PREJIT
        if (IsZapped())
        {
            target = GetMethodDesc()->GetMethodEntryPoint();
            if (target != this->GetEntryPoint())
            {
                break;
            }
        }
#endif
        target = AsFixupPrecode()->GetTarget();
        break;
#endif // HAS_FIXUP_PRECODE
#ifdef HAS_THISPTR_RETBUF_PRECODE
    case PRECODE_THISPTR_RETBUF:
        target = AsThisPtrRetBufPrecode()->GetTarget();
        break;
#endif // HAS_THISPTR_RETBUF_PRECODE

    default:
        UnexpectedPrecodeType("Precode::GetTarget", precodeType);
        break;
    }
    return target;
}

MethodDesc* Precode::GetMethodDesc(BOOL fSpeculative /*= FALSE*/)
{
    CONTRACTL {
        NOTHROW;
        GC_NOTRIGGER;
        SO_TOLERANT;
        SUPPORTS_DAC;
    } CONTRACTL_END;

    TADDR pMD = NULL;

    PrecodeType precodeType = GetType();
    switch (precodeType)
    {
    case PRECODE_STUB:
        pMD = AsStubPrecode()->GetMethodDesc();
        break;
#ifdef HAS_NDIRECT_IMPORT_PRECODE
    case PRECODE_NDIRECT_IMPORT:
        pMD = AsNDirectImportPrecode()->GetMethodDesc();
        break;
#endif // HAS_NDIRECT_IMPORT_PRECODE
#ifdef HAS_REMOTING_PRECODE
    case PRECODE_REMOTING:
        pMD = AsRemotingPrecode()->GetMethodDesc();
        break;
#endif // HAS_REMOTING_PRECODE
#ifdef HAS_FIXUP_PRECODE
    case PRECODE_FIXUP:
        pMD = AsFixupPrecode()->GetMethodDesc();
        break;
#endif // HAS_FIXUP_PRECODE
#ifdef HAS_THISPTR_RETBUF_PRECODE
    case PRECODE_THISPTR_RETBUF:
        pMD = AsThisPtrRetBufPrecode()->GetMethodDesc();
        break;
#endif // HAS_THISPTR_RETBUF_PRECODE

    default:
        break;
    }

    if (pMD == NULL)
    {
        if (fSpeculative)
            return NULL;
        else
            UnexpectedPrecodeType("Precode::GetMethodDesc", precodeType);
    }

    // GetMethodDesc() on platform specific precode types returns TADDR. It should return 
    // PTR_MethodDesc instead. It is a workaround to resolve cyclic dependency between headers. 
    // Once we headers factoring of headers cleaned up, we should be able to get rid of it.

    // For speculative calls, pMD can be garbage that causes IBC logging to crash
    if (!fSpeculative)
        g_IBCLogger.LogMethodPrecodeAccess((PTR_MethodDesc)pMD);

    return (PTR_MethodDesc)pMD;
}

BOOL Precode::IsCorrectMethodDesc(MethodDesc *  pMD)
{
    CONTRACTL
    {
        NOTHROW;
        GC_NOTRIGGER;
        SO_TOLERANT;
        MODE_ANY;
    }
    CONTRACTL_END;
    MethodDesc * pMDfromPrecode = GetMethodDesc(TRUE);

    if (pMDfromPrecode == pMD)
        return TRUE;

#ifdef HAS_FIXUP_PRECODE_CHUNKS
    if (pMDfromPrecode == NULL)
    {
        PrecodeType precodeType = GetType();

#ifdef HAS_FIXUP_PRECODE_CHUNKS
        // We do not keep track of the MethodDesc in every kind of fixup precode
        if (precodeType == PRECODE_FIXUP)
            return TRUE;
#endif
    }
#endif // HAS_FIXUP_PRECODE_CHUNKS

    return FALSE;
}

BOOL Precode::IsPointingToPrestub(PCODE target)
{
    CONTRACTL
    {
        NOTHROW;
        GC_NOTRIGGER;
        SO_TOLERANT;
        MODE_ANY;
    }
    CONTRACTL_END;

    if (IsPointingTo(target, GetPreStubEntryPoint()))
        return TRUE;

#ifdef HAS_FIXUP_PRECODE
    if (IsPointingTo(target, GetEEFuncEntryPoint(PrecodeFixupThunk)))
        return TRUE;
#endif

#ifdef FEATURE_PREJIT
    Module *pZapModule = GetMethodDesc()->GetZapModule();
    if (pZapModule != NULL)
    {
        if (IsPointingTo(target, pZapModule->GetPrestubJumpStub()))
            return TRUE;

#ifdef HAS_FIXUP_PRECODE
        if (IsPointingTo(target, pZapModule->GetPrecodeFixupJumpStub()))
            return TRUE;
#endif
    }
#endif // FEATURE_PREJIT

    return FALSE;
}

// If addr is patched fixup precode, returns address that it points to. Otherwise returns NULL.
PCODE Precode::TryToSkipFixupPrecode(PCODE addr)
{
    CONTRACTL {
        NOTHROW;
        GC_NOTRIGGER;
        SO_TOLERANT;
    } CONTRACTL_END;

    PCODE pTarget = NULL;

#if defined(FEATURE_PREJIT) && defined(HAS_FIXUP_PRECODE)
    // Early out for common cases
    if (!FixupPrecode::IsFixupPrecodeByASM(addr))
        return NULL;

    // This optimization makes sense in NGened code only.
    Module * pModule = ExecutionManager::FindZapModule(addr);
    if (pModule == NULL)
        return NULL;

    // Verify that the address is in precode section
    if (!pModule->IsZappedPrecode(addr))
        return NULL;

    pTarget = GetPrecodeFromEntryPoint(addr)->GetTarget();

    // Verify that the target is in code section
    if (!pModule->IsZappedCode(pTarget))
        return NULL;

#if defined(_DEBUG)
    MethodDesc * pMD_orig   = MethodTable::GetMethodDescForSlotAddress(addr);
    MethodDesc * pMD_direct = MethodTable::GetMethodDescForSlotAddress(pTarget);

    // Both the original and direct entrypoint should map to same MethodDesc
    // Some FCalls are remapped to private methods (see System.String.CtorCharArrayStartLength)
    _ASSERTE((pMD_orig == pMD_direct) || pMD_orig->IsRuntimeSupplied());
#endif

#endif // defined(FEATURE_PREJIT) && defined(HAS_FIXUP_PRECODE)

    return pTarget;
}

Precode* Precode::GetPrecodeForTemporaryEntryPoint(TADDR temporaryEntryPoints, int index)
{
    WRAPPER_NO_CONTRACT;
    PrecodeType t = PTR_Precode(temporaryEntryPoints)->GetType();
#ifdef HAS_FIXUP_PRECODE_CHUNKS
    if (t == PRECODE_FIXUP)
    {
        return PTR_Precode(temporaryEntryPoints + index * sizeof(FixupPrecode));
    }
#endif
    SIZE_T oneSize = SizeOfTemporaryEntryPoint(t);
    return PTR_Precode(temporaryEntryPoints + index * oneSize);
}

SIZE_T Precode::SizeOfTemporaryEntryPoints(PrecodeType t, bool preallocateJumpStubs, int count)
{
    WRAPPER_NO_CONTRACT;
    SUPPORTS_DAC;

#ifdef HAS_FIXUP_PRECODE_CHUNKS
    if (t == PRECODE_FIXUP)
    {
        SIZE_T size = count * sizeof(FixupPrecode) + sizeof(PTR_MethodDesc);

#ifdef FIXUP_PRECODE_PREALLOCATE_DYNAMIC_METHOD_JUMP_STUBS
        if (preallocateJumpStubs)
        {
            // For dynamic methods, space for jump stubs is allocated along with the precodes as part of the temporary entry
            // points block. The first jump stub begins immediately after the PTR_MethodDesc.
            size += count * BACK_TO_BACK_JUMP_ALLOCATE_SIZE;
        }
#else // !FIXUP_PRECODE_PREALLOCATE_DYNAMIC_METHOD_JUMP_STUBS
        _ASSERTE(!preallocateJumpStubs);
#endif // FIXUP_PRECODE_PREALLOCATE_DYNAMIC_METHOD_JUMP_STUBS

        return size;
    }
    else
    {
        _ASSERTE(!preallocateJumpStubs);
    }
#endif
    SIZE_T oneSize = SizeOfTemporaryEntryPoint(t);
    return count * oneSize;
}

SIZE_T Precode::SizeOfTemporaryEntryPoints(TADDR temporaryEntryPoints, int count)
{
    WRAPPER_NO_CONTRACT;
    SUPPORTS_DAC;

    PrecodeType precodeType = PTR_Precode(temporaryEntryPoints)->GetType();
#ifdef FIXUP_PRECODE_PREALLOCATE_DYNAMIC_METHOD_JUMP_STUBS
    bool preallocateJumpStubs =
        precodeType == PRECODE_FIXUP &&
        ((PTR_MethodDesc)((PTR_FixupPrecode)temporaryEntryPoints)->GetMethodDesc())->IsLCGMethod();
#else // !FIXUP_PRECODE_PREALLOCATE_DYNAMIC_METHOD_JUMP_STUBS
    bool preallocateJumpStubs = false;
#endif // FIXUP_PRECODE_PREALLOCATE_DYNAMIC_METHOD_JUMP_STUBS
    return SizeOfTemporaryEntryPoints(precodeType, preallocateJumpStubs, count);
}

#ifndef DACCESS_COMPILE

Precode* Precode::Allocate(PrecodeType t, MethodDesc* pMD,
                           LoaderAllocator *  pLoaderAllocator, 
                           AllocMemTracker *  pamTracker)
{
    CONTRACTL
    {
        THROWS;
        GC_NOTRIGGER;
        MODE_ANY;
    }
    CONTRACTL_END;

    SIZE_T size;

#ifdef HAS_FIXUP_PRECODE_CHUNKS
    if (t == PRECODE_FIXUP)
    {
        size = sizeof(FixupPrecode) + sizeof(PTR_MethodDesc);
    }
    else
#endif
    {
        size = Precode::SizeOf(t);
    }

    Precode* pPrecode = (Precode*)pamTracker->Track(pLoaderAllocator->GetPrecodeHeap()->AllocAlignedMem(size, AlignOf(t)));
    pPrecode->Init(t, pMD, pLoaderAllocator);

#ifndef CROSSGEN_COMPILE
    ClrFlushInstructionCache(pPrecode, size);
#endif

    return pPrecode;
}

void Precode::Init(PrecodeType t, MethodDesc* pMD, LoaderAllocator *pLoaderAllocator)
{
    LIMITED_METHOD_CONTRACT;

    switch (t) {
    case PRECODE_STUB:
        ((StubPrecode*)this)->Init(pMD, pLoaderAllocator);
        break;
#ifdef HAS_NDIRECT_IMPORT_PRECODE
    case PRECODE_NDIRECT_IMPORT:
        ((NDirectImportPrecode*)this)->Init(pMD, pLoaderAllocator);
        break;
#endif // HAS_NDIRECT_IMPORT_PRECODE
#ifdef HAS_REMOTING_PRECODE
    case PRECODE_REMOTING:
        ((RemotingPrecode*)this)->Init(pMD, pLoaderAllocator);
        break;
#endif // HAS_REMOTING_PRECODE
#ifdef HAS_FIXUP_PRECODE
    case PRECODE_FIXUP:
        ((FixupPrecode*)this)->Init(pMD, pLoaderAllocator);
        break;
#endif // HAS_FIXUP_PRECODE
#ifdef HAS_THISPTR_RETBUF_PRECODE
    case PRECODE_THISPTR_RETBUF:
        ((ThisPtrRetBufPrecode*)this)->Init(pMD, pLoaderAllocator);
        break;
#endif // HAS_THISPTR_RETBUF_PRECODE
    default:
        UnexpectedPrecodeType("Precode::Init", t);
        break;
    }

    _ASSERTE(IsValidType(GetType()));
}

void Precode::ResetTargetInterlocked()
{
    WRAPPER_NO_CONTRACT;

    _ASSERTE(!IsZapped());

    PrecodeType precodeType = GetType();
    switch (precodeType)
    {
        case PRECODE_STUB:
            AsStubPrecode()->ResetTargetInterlocked();
            break;

#ifdef HAS_FIXUP_PRECODE
        case PRECODE_FIXUP:
            AsFixupPrecode()->ResetTargetInterlocked();
            break;
#endif // HAS_FIXUP_PRECODE

        default:
            UnexpectedPrecodeType("Precode::ResetTargetInterlocked", precodeType);
            break;
    }
}

BOOL Precode::SetTargetInterlocked(PCODE target, BOOL fOnlyRedirectFromPrestub)
{
    WRAPPER_NO_CONTRACT;

    _ASSERTE(!IsZapped());

    PCODE expected = GetTarget();
    BOOL ret = FALSE;

    if (fOnlyRedirectFromPrestub && !IsPointingToPrestub(expected))
        return FALSE;

    g_IBCLogger.LogMethodPrecodeWriteAccess(GetMethodDesc());

    PrecodeType precodeType = GetType();
    switch (precodeType)
    {
    case PRECODE_STUB:
        ret = AsStubPrecode()->SetTargetInterlocked(target, expected);
        break;

#ifdef HAS_REMOTING_PRECODE
    case PRECODE_REMOTING:
        ret = AsRemotingPrecode()->SetTargetInterlocked(target, expected);
        break;
#endif // HAS_REMOTING_PRECODE

#ifdef HAS_FIXUP_PRECODE
    case PRECODE_FIXUP:
        ret = AsFixupPrecode()->SetTargetInterlocked(target, expected);
        break;
#endif // HAS_FIXUP_PRECODE

#ifdef HAS_THISPTR_RETBUF_PRECODE
    case PRECODE_THISPTR_RETBUF:
        _ASSERTE(!IsZapped());
        ret = AsThisPtrRetBufPrecode()->SetTargetInterlocked(target, expected);
        break;
#endif // HAS_THISPTR_RETBUF_PRECODE

    default:
        UnexpectedPrecodeType("Precode::SetTargetInterlocked", precodeType);
        break;
    }

    //
    // SetTargetInterlocked does not modify code on ARM so the flush instruction cache is
    // not necessary.
    //
#if !defined(_TARGET_ARM_) && !defined(_TARGET_ARM64_)
    if (ret) {
        FlushInstructionCache(GetCurrentProcess(),this,SizeOf());
    }
#endif

    _ASSERTE(!IsPointingToPrestub());
    return ret;
}

void Precode::Reset()
{
    WRAPPER_NO_CONTRACT;

    _ASSERTE(!IsZapped());

    MethodDesc* pMD = GetMethodDesc();
    Init(GetType(), pMD, pMD->GetLoaderAllocatorForCode());
    FlushInstructionCache(GetCurrentProcess(),this,SizeOf());
}

/* static */ 
TADDR Precode::AllocateTemporaryEntryPoints(MethodDescChunk *  pChunk,
                                            LoaderAllocator *  pLoaderAllocator, 
                                            AllocMemTracker *  pamTracker)
{
    WRAPPER_NO_CONTRACT;

    MethodDesc* pFirstMD = pChunk->GetFirstMethodDesc();

    int count = pChunk->GetCount();

    PrecodeType t = PRECODE_STUB;
    bool preallocateJumpStubs = false;

#ifdef HAS_FIXUP_PRECODE
    // Default to faster fixup precode if possible
    if (!pFirstMD->RequiresMethodDescCallingConvention(count > 1))
    {
        t = PRECODE_FIXUP;

#ifdef FIXUP_PRECODE_PREALLOCATE_DYNAMIC_METHOD_JUMP_STUBS
        if (pFirstMD->IsLCGMethod())
        {
            preallocateJumpStubs = true;
        }
#endif // FIXUP_PRECODE_PREALLOCATE_DYNAMIC_METHOD_JUMP_STUBS
    }
    else
    {
        _ASSERTE(!pFirstMD->IsLCGMethod());
    }
#endif // HAS_FIXUP_PRECODE

    SIZE_T totalSize = SizeOfTemporaryEntryPoints(t, preallocateJumpStubs, count);

#ifdef HAS_COMPACT_ENTRYPOINTS
    // Note that these are just best guesses to save memory. If we guessed wrong,
    // we will allocate a new exact type of precode in GetOrCreatePrecode.
    BOOL fForcedPrecode = pFirstMD->RequiresStableEntryPoint(count > 1);

#ifdef _TARGET_ARM_
    if (pFirstMD->RequiresMethodDescCallingConvention(count > 1)
        || count >= MethodDescChunk::GetCompactEntryPointMaxCount ())
    {
        // We do not pass method desc on scratch register
        fForcedPrecode = TRUE;
    }
#endif // _TARGET_ARM_

    if (!fForcedPrecode && (totalSize > MethodDescChunk::SizeOfCompactEntryPoints(count)))
        return NULL;
#endif

    TADDR temporaryEntryPoints = (TADDR)pamTracker->Track(pLoaderAllocator->GetPrecodeHeap()->AllocAlignedMem(totalSize, AlignOf(t)));

#ifdef HAS_FIXUP_PRECODE_CHUNKS
    if (t == PRECODE_FIXUP)
    {
        TADDR entryPoint = temporaryEntryPoints;
        MethodDesc * pMD = pChunk->GetFirstMethodDesc();
        for (int i = 0; i < count; i++)
        {
            ((FixupPrecode *)entryPoint)->Init(pMD, pLoaderAllocator, pMD->GetMethodDescIndex(), (count - 1) - i);

            _ASSERTE((Precode *)entryPoint == GetPrecodeForTemporaryEntryPoint(temporaryEntryPoints, i));
            entryPoint += sizeof(FixupPrecode);
    
            pMD = (MethodDesc *)(dac_cast<TADDR>(pMD) + pMD->SizeOf());
        }

#ifdef FEATURE_PERFMAP
        PerfMap::LogStubs(__FUNCTION__, "PRECODE_FIXUP", (PCODE)temporaryEntryPoints, count * sizeof(FixupPrecode));
#endif
        ClrFlushInstructionCache((LPVOID)temporaryEntryPoints, count * sizeof(FixupPrecode));

        return temporaryEntryPoints;
    }
#endif

    SIZE_T oneSize = SizeOfTemporaryEntryPoint(t);
    TADDR entryPoint = temporaryEntryPoints;
    MethodDesc * pMD = pChunk->GetFirstMethodDesc();
    for (int i = 0; i < count; i++)
    {
        ((Precode *)entryPoint)->Init(t, pMD, pLoaderAllocator);

        _ASSERTE((Precode *)entryPoint == GetPrecodeForTemporaryEntryPoint(temporaryEntryPoints, i));
        entryPoint += oneSize;

        pMD = (MethodDesc *)(dac_cast<TADDR>(pMD) + pMD->SizeOf());
    }

#ifdef FEATURE_PERFMAP
    PerfMap::LogStubs(__FUNCTION__, "PRECODE_STUB", (PCODE)temporaryEntryPoints, count * oneSize);
#endif

    ClrFlushInstructionCache((LPVOID)temporaryEntryPoints, count * oneSize);

    return temporaryEntryPoints;
}

#ifdef FEATURE_NATIVE_IMAGE_GENERATION

static DataImage::ItemKind GetPrecodeItemKind(DataImage * image, MethodDesc * pMD, BOOL fIsPrebound = FALSE)
{
    STANDARD_VM_CONTRACT;

    DataImage::ItemKind kind = DataImage::ITEM_METHOD_PRECODE_COLD_WRITEABLE;

    DWORD flags = image->GetMethodProfilingFlags(pMD);

    if (flags & (1 << WriteMethodPrecode))
    {
        kind = fIsPrebound ? DataImage::ITEM_METHOD_PRECODE_HOT : DataImage::ITEM_METHOD_PRECODE_HOT_WRITEABLE;
    }
    else
    if (flags & (1 << ReadMethodPrecode))
    {
        kind = DataImage::ITEM_METHOD_PRECODE_HOT;
    }
    else
    if (
        fIsPrebound ||
        // The generic method definitions get precode to make GetMethodDescForSlot work.
        // This precode should not be ever written to.
        pMD->ContainsGenericVariables() ||
        // Interface MDs are run only for remoting and cominterop which is pretty rare. Make them cold.
        pMD->IsInterface()
        )
    {
        kind = DataImage::ITEM_METHOD_PRECODE_COLD;
    }

    return kind;
}

void Precode::Fixup(DataImage *image, MethodDesc * pMD)
{
    STANDARD_VM_CONTRACT;

    PrecodeType precodeType = GetType();

#if defined(_TARGET_X86_) || defined(_TARGET_AMD64_)
#if defined(HAS_FIXUP_PRECODE)
    if (precodeType == PRECODE_FIXUP)
    {
        AsFixupPrecode()->Fixup(image, pMD);
    }
#endif
#else // _TARGET_X86_ || _TARGET_AMD64_
    ZapNode * pCodeNode = NULL;

    if (IsPrebound(image))
    {
        pCodeNode = image->GetCodeAddress(pMD);
    }

    switch (precodeType) {
    case PRECODE_STUB:
        AsStubPrecode()->Fixup(image);
        break;
#ifdef HAS_NDIRECT_IMPORT_PRECODE
    case PRECODE_NDIRECT_IMPORT:
        AsNDirectImportPrecode()->Fixup(image);
        break;
#endif // HAS_NDIRECT_IMPORT_PRECODE
#ifdef HAS_REMOTING_PRECODE
    case PRECODE_REMOTING:
        AsRemotingPrecode()->Fixup(image, pCodeNode);
        break;
#endif // HAS_REMOTING_PRECODE
#ifdef HAS_FIXUP_PRECODE
    case PRECODE_FIXUP:
        AsFixupPrecode()->Fixup(image, pMD);
        break;
#endif // HAS_FIXUP_PRECODE
    default:
        UnexpectedPrecodeType("Precode::Save", precodeType);
        break;
    }
#endif // _TARGET_X86_ || _TARGET_AMD64_
}

BOOL Precode::IsPrebound(DataImage *image)
{
    WRAPPER_NO_CONTRACT;

#ifdef HAS_REMOTING_PRECODE
    // This will make sure that when IBC logging is on, the precode goes thru prestub.
    if (GetAppDomain()->ToCompilationDomain()->m_fForceInstrument)
        return FALSE;

    if (GetType() != PRECODE_REMOTING)
        return FALSE;

    // Prebind the remoting precode if possible
    return image->CanDirectCall(GetMethodDesc(), CORINFO_ACCESS_THIS);
#else
    return FALSE;
#endif
}

void Precode::SaveChunk::AddPrecodeForMethod(MethodDesc * pMD, BOOL registerSurrogate)
{
    STANDARD_VM_CONTRACT;

    m_rgMethods.Append(pMD);
    m_rgRegisterSurrogateFlags.Append(registerSurrogate);
}

#ifdef HAS_FIXUP_PRECODE_CHUNKS
static PVOID SaveFixupPrecodeChunk(
    DataImage * image,
    MethodDesc ** rgMD,
    BOOL * rgRegisterSurrogate,
    COUNT_T count,
    DataImage::ItemKind kind)
{
    STANDARD_VM_CONTRACT;

    ULONG size = sizeof(FixupPrecode) * count + sizeof(PTR_MethodDesc);
    FixupPrecode * pBase = (FixupPrecode *)new (image->GetHeap()) BYTE[size];

    ZapStoredStructure * pNode = image->StoreStructure(NULL, size, kind,
        Precode::AlignOf(PRECODE_FIXUP));

    for (COUNT_T i = 0; i < count; i++)
    {
        MethodDesc * pMD = rgMD[i];
        FixupPrecode * pPrecode = pBase + i;

        pPrecode->InitForSave((count - 1) - i);
        _ASSERTE((Precode *)pPrecode == Precode::GetPrecodeForTemporaryEntryPoint((TADDR)pBase, i));

        image->BindPointer(pPrecode, pNode, i * sizeof(FixupPrecode));

        // Alias the temporary entrypoint to the "new" MD - the one saved in the image.
        image->RegisterSurrogate((MethodDesc *)image->GetImagePointer(pMD), pPrecode);

        // If asked for, register alias the precode to the "old" MD (the one created from metadata) as well.
        if (rgRegisterSurrogate[i])
            image->RegisterSurrogate(pMD, pPrecode);
    }

    image->CopyData(pNode, pBase, size);

    return pBase;
}
#endif // HAS_FIXUP_PRECODE_CHUNKS

static PVOID SaveStubPrecodeChunk(
    DataImage * image,
    MethodDesc ** rgMD,
    BOOL * rgRegisterSurrogate,
    COUNT_T count,
    DataImage::ItemKind kind)
{
    STANDARD_VM_CONTRACT;

    ULONG sizeOfOne = Precode::SizeOfTemporaryEntryPoint(PRECODE_STUB);
    ULONG size = sizeOfOne * count;
    TADDR pBase = (TADDR)new (image->GetHeap()) BYTE[size];

    for (COUNT_T i = 0; i < count; i++)
    {
        MethodDesc * pMD = rgMD[i];
        StubPrecode * pPrecode = (StubPrecode *)(pBase + (sizeOfOne * i));

        pPrecode->Init(pMD, NULL);
        _ASSERTE((Precode *)pPrecode == Precode::GetPrecodeForTemporaryEntryPoint(pBase, i));

        // Alias the temporary entrypoint to the "new" MD - the one saved in the image.
        image->RegisterSurrogate((MethodDesc *)image->GetImagePointer(pMD), pPrecode);

        // If asked for, register alias the precode to the "old" MD (the one created from metadata) as well.
        if (rgRegisterSurrogate[i])
            image->RegisterSurrogate(pMD, pPrecode);
    }

#if defined(_TARGET_X86_) || defined(_TARGET_AMD64_)
    image->SaveStubPrecodeChunk(pBase, sizeOfOne, rgMD, count, kind);
#else
    ZapStoredStructure * pNode = image->StoreStructure(NULL, size, kind, PRECODE_ALIGNMENT);
    for (COUNT_T i = 0; i < count; i++)
    {
        image->BindPointer((void *)(pBase + (sizeOfOne * i)), pNode, sizeOfOne * i);
    }

    image->CopyData(pNode, (void *)pBase, size);
#endif // defined(_TARGET_X86_) || defined(_TARGET_AMD64_)

    return (PVOID)pBase;
}

PVOID Precode::SaveChunk::Save(DataImage * image)
{
    STANDARD_VM_CONTRACT;

    // TODO: Since we have to save precodes in chunks defined by the MethodDescChunk
    // and not by us here, the hot/cold splitting of precodes is not possible anymore
    // Review the changes here and decide how to handle this - maybe we could apply the
    // precode hot/cold split to method desc chunk ordering, or we will have to completely ignore it.
    // For now we will save everything as cold - but that should probably change
    // we should possibly apply the same hot/cold split as for methods themselves.
    if (m_rgMethods.GetCount() == 0)
        return NULL;

    // The type of the precode used is simple.
    // If we can, we will use Fixup precodes as those are more space efficient.
    // If not we fallback to Stub precodes. We use the same precode type for all precodes in one chunk.
    // There's no need to use precode which would be most suitable for the given method
    // as the precodes saved here act as temporary entry points which will never be patched.
    // Nobody should use the precode as a stable/multi-callable entry point for the method.
    // We need the precode as the most simple way to get to the DoPrestub which will figure out
    // what the method needs and potentially create a runtime allocated (patchable) precode of the right type.
#ifdef HAS_FIXUP_PRECODE_CHUNKS
    return SaveFixupPrecodeChunk(
        image,
        &m_rgMethods[0],
        &m_rgRegisterSurrogateFlags[0],
        m_rgMethods.GetCount(),
        DataImage::ItemKind::ITEM_METHOD_PRECODE_COLD);
#else
    return SaveStubPrecodeChunk(
        image,
        &m_rgMethods[0],
        &m_rgRegisterSurrogateFlags[0],
        m_rgMethods.GetCount(),
        DataImage::ItemKind::ITEM_METHOD_PRECODE_COLD);
#endif // HAS_FIXUP_PRECODE_CHUNKS
}

#endif // FEATURE_NATIVE_IMAGE_GENERATION

#endif // !DACCESS_COMPILE


#ifdef DACCESS_COMPILE
void Precode::EnumMemoryRegions(CLRDataEnumMemoryFlags flags)
{   
    SUPPORTS_DAC;
    PrecodeType t = GetType();

#ifdef HAS_FIXUP_PRECODE_CHUNKS
    if (t == PRECODE_FIXUP)
    {
        AsFixupPrecode()->EnumMemoryRegions(flags);
        return;
    }
#endif

    DacEnumMemoryRegion(GetStart(), SizeOf(t));
}
#endif

