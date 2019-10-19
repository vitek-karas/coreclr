// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "common.h"
#include "assemblybinder.hpp"
#include "clrprivbindercoreclr.h"

using namespace BINDER_SPACE;

//=============================================================================
// Helper functions
//-----------------------------------------------------------------------------

HRESULT CLRPrivBinderCoreCLR::BindAssemblyByNameWorker(BINDER_SPACE::AssemblyName *pAssemblyName,
                                                       BINDER_SPACE::Assembly **ppCoreCLRFoundAssembly,
                                                       bool excludeAppPaths)
{
    VALIDATE_ARG_RET(pAssemblyName != nullptr && ppCoreCLRFoundAssembly != nullptr);
    HRESULT hr = S_OK;
    
#ifdef _DEBUG
    // MSCORLIB should be bound using BindToSystem
    _ASSERTE(!pAssemblyName->IsMscorlib());
#endif

    hr = AssemblyBinder::BindAssembly(&m_appContext,
                                      pAssemblyName,
                                      NULL,
                                      NULL,
                                      FALSE, //fNgenExplicitBind,
                                      FALSE, //fExplicitBindToNativeImage,
                                      excludeAppPaths,
                                      ppCoreCLRFoundAssembly);
    if (!FAILED(hr))
    {
        (*ppCoreCLRFoundAssembly)->SetBinder(this);
    }
    
    return hr;
}

// ============================================================================
// CLRPrivBinderCoreCLR implementation
// ============================================================================
HRESULT CLRPrivBinderCoreCLR::BindAssemblyByName(IAssemblyName     *pIAssemblyName,
                                                 ICLRPrivAssembly **ppAssembly)
{
    HRESULT hr = S_OK;
    VALIDATE_ARG_RET(pIAssemblyName != nullptr && ppAssembly != nullptr);
    
    EX_TRY
    {
        *ppAssembly = nullptr;

        ReleaseHolder<BINDER_SPACE::Assembly> pCoreCLRFoundAssembly;
        ReleaseHolder<AssemblyName> pAssemblyName;

        SAFE_NEW(pAssemblyName, AssemblyName);
        IF_FAIL_GO(pAssemblyName->Init(pIAssemblyName));
        
        hr = BindAssemblyByNameWorker(pAssemblyName, &pCoreCLRFoundAssembly, false /* excludeAppPaths */);

#if !defined(DACCESS_COMPILE) && !defined(CROSSGEN_COMPILE)
        if ((hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) ||
            (hr == FUSION_E_APP_DOMAIN_LOCKED) || (hr == FUSION_E_REF_DEF_MISMATCH))
        {
            // If we are here, one of the following is possible:
            //
            // 1) The assembly has not been found in the current binder's application context (i.e. it has not already been loaded), OR
            // 2) An assembly with the same simple name was already loaded in the context of the current binder but we ran into a Ref/Def
            //    mismatch (either due to version difference or strong-name difference).
            //
            // Thus, if default binder has been overridden, then invoke it in an attempt to perform the binding for it make the call
            // of what to do next. The host-overridden binder can either fail the bind or return reference to an existing assembly
            // that has been loaded.

            // Attempt to resolve the assembly via managed TPA ALC instance if one exists
            INT_PTR pManagedAssemblyLoadContext = GetManagedAssemblyLoadContext();
            if (pManagedAssemblyLoadContext != NULL)
            {
                hr = AssemblyBinder::BindUsingHostAssemblyResolver(pManagedAssemblyLoadContext, pAssemblyName, pIAssemblyName, 
                                                                   NULL, &pCoreCLRFoundAssembly);
                if (SUCCEEDED(hr))
                {
                    // We maybe returned an assembly that was bound to a different AssemblyLoadContext instance.
                    // In such a case, we will not overwrite the binding context (which would be wrong since it would not
                    // be present in the cache of the current binding context).
                    if (pCoreCLRFoundAssembly->GetBinder() == NULL)
                    {
                        pCoreCLRFoundAssembly->SetBinder(this);
                    }
                }
            }
        }
#endif // !defined(DACCESS_COMPILE) && !defined(CROSSGEN_COMPILE)
        
        IF_FAIL_GO(hr);

        *ppAssembly = pCoreCLRFoundAssembly.Extract();

Exit:;        
    }
    EX_CATCH_HRESULT(hr);

    return hr;
}

#if !defined(DACCESS_COMPILE) && !defined(CROSSGEN_COMPILE)
HRESULT CLRPrivBinderCoreCLR::BindUsingPEImage(/* in */ PEImage *pPEImage, 
                                               /* in */ BOOL fIsNativeImage, 
                                               /* [retval][out] */ ICLRPrivAssembly **ppAssembly)
{
    HRESULT hr = S_OK;

    EX_TRY
    {
        ReleaseHolder<BINDER_SPACE::Assembly> pCoreCLRFoundAssembly;
        ReleaseHolder<BINDER_SPACE::AssemblyName> pAssemblyName;        
        ReleaseHolder<IMDInternalImport> pIMetaDataAssemblyImport;
        
        PEKIND PeKind = peNone;
        
        IF_FAIL_GO(AssemblyBinder::PrepareBindUsingPEImage(pPEImage, fIsNativeImage, &pAssemblyName, &pIMetaDataAssemblyImport, &PeKind));

        {
            // Ensure we are not being asked to bind to a TPA assembly
            //
            // This effectively implements a relaxed behavior for assemblies from TPA
            // Other assemblies have a very strict behavior that LoadFromAssemblyPath will only succeed if:
            //   - The assembly doesn't exist in the context yet
            //   - Or if it does exist, it has the exact same MVID (basically must be the exact same build - same file)
            //     in this case it returns the already loaded assembly.
            // This code below relaxes this behavior for assemblies in TPA where calling LoadFromAssemblyPath
            // on an assembly which already exists in the TPA will allow to load any assembly with lower or same version
            // in which case it returns the already loaded assembly. So the difference is that it allows:
            //   - Loading of assembly of the same version but different build (different MVID)
            //   - Loading of assembly of a lower version
            //
            // Not 100% this was intentional, but it has a relatively nice side effect where:
            //   - App uses some assembly System.Feature.dll from NuGet, but loads it dynamically into Default context.
            //   - In later version we now include System.Feature.dll (higher or same version) in the framework itself.
            //   - After the upgrade the app automatically gets the framework version of the assembly (from TPA).
            // This matches the host behavior if the app include the System.Featire.dll in its deps.json (the one from
            // framework will be used if equal or higher).
            //
            SString& simpleName = pAssemblyName->GetSimpleName();
            SimpleNameToFileNameMap* tpaMap = GetAppContext()->GetTpaList();
            if (tpaMap->LookupPtr(simpleName.GetUnicode()) != NULL)
            {
                // The simple name of the assembly being requested to be bound was found in the TPA list.
                // Now, perform the actual bind to see if the assembly was really in the TPA assembly list or not.
                hr = BindAssemblyByNameWorker(pAssemblyName, &pCoreCLRFoundAssembly, true /* excludeAppPaths */);
                if (SUCCEEDED(hr))
                {
                    if (pCoreCLRFoundAssembly->GetIsInGAC())
                    {
                        *ppAssembly = pCoreCLRFoundAssembly.Extract();
                        goto Exit;
                    }
                }
            }
        }
            
        hr = AssemblyBinder::BindUsingPEImage(&m_appContext, pAssemblyName, pPEImage, PeKind, pIMetaDataAssemblyImport, &pCoreCLRFoundAssembly);
        if (hr == S_OK)
        {
            _ASSERTE(pCoreCLRFoundAssembly != NULL);
            pCoreCLRFoundAssembly->SetBinder(this);
            *ppAssembly = pCoreCLRFoundAssembly.Extract();
        }
Exit:;        
    }
    EX_CATCH_HRESULT(hr);

    return hr;
}
#endif // !defined(DACCESS_COMPILE) && !defined(CROSSGEN_COMPILE)

HRESULT CLRPrivBinderCoreCLR::GetBinderID( 
        UINT_PTR *pBinderId)
{
    *pBinderId = reinterpret_cast<UINT_PTR>(this); 
    return S_OK;
}
         
HRESULT CLRPrivBinderCoreCLR::SetupBindingPaths(SString  &sTrustedPlatformAssemblies,
                                                SString  &sPlatformResourceRoots,
                                                SString  &sAppPaths,
                                                SString  &sAppNiPaths)
{
    HRESULT hr = S_OK;
    
    EX_TRY
    {
        hr = m_appContext.SetupBindingPaths(sTrustedPlatformAssemblies, sPlatformResourceRoots, sAppPaths, sAppNiPaths, TRUE /* fAcquireLock */);
    }
    EX_CATCH_HRESULT(hr);
    return hr;
}

// See code:BINDER_SPACE::AssemblyBinder::GetAssembly for info on fNgenExplicitBind
// and fExplicitBindToNativeImage, and see code:CEECompileInfo::LoadAssemblyByPath
// for an example of how they're used.
HRESULT CLRPrivBinderCoreCLR::Bind(SString           &assemblyDisplayName,
                                   LPCWSTR            wszCodeBase,
                                   PEAssembly        *pParentAssembly,
                                   BOOL               fNgenExplicitBind,
                                   BOOL               fExplicitBindToNativeImage,
                                   ICLRPrivAssembly **ppAssembly)
{
    HRESULT hr = S_OK;
    VALIDATE_ARG_RET(ppAssembly != NULL);

    AssemblyName assemblyName;
    
    ReleaseHolder<AssemblyName> pAssemblyName;
    
    if (!assemblyDisplayName.IsEmpty())
    {
        // AssemblyDisplayName can be empty if wszCodeBase is specified.
        SAFE_NEW(pAssemblyName, AssemblyName);
        IF_FAIL_GO(pAssemblyName->Init(assemblyDisplayName));
    }
    
    EX_TRY
    {
        ReleaseHolder<BINDER_SPACE::Assembly> pAsm;
        hr = AssemblyBinder::BindAssembly(&m_appContext,
                                          pAssemblyName,
                                          wszCodeBase,
                                          pParentAssembly,
                                          fNgenExplicitBind,
                                          fExplicitBindToNativeImage,
                                          false, // excludeAppPaths
                                          &pAsm);
        if(SUCCEEDED(hr))
        {
            _ASSERTE(pAsm != NULL);
            pAsm->SetBinder(this);
            *ppAssembly = pAsm.Extract();
        }
    }
    EX_CATCH_HRESULT(hr);
    
Exit:    
    return hr;
}

HRESULT CLRPrivBinderCoreCLR::GetLoaderAllocator(LPVOID* pLoaderAllocator)
{
    // Not supported by this binder
    return E_FAIL;
}
