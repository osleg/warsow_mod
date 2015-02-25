#include "../gameshared/q_arch.h"
#include "steamlib_local.h"

namespace WSWSTEAM {

static steamlib_import_t si;

/*
* GetSteamImport
*/
steamlib_import_t *GetSteamImport( void )
{
	return &si;
}

}

/*
* GetSteamLibAPI
* 
* Returns a pointer to the structure with all entry points
*/
extern "C" QF_DLL_EXPORT steamlib_export_t *GetSteamLibAPI( steamlib_import_t *import )
{
	static steamlib_export_t globals;

	WSWSTEAM::si = *import;

	globals.API = &WSWSTEAM::SteamLib_API;

	globals.Init = &WSWSTEAM::SteamLib_Init;
	globals.RunFrame = &WSWSTEAM::SteamLib_RunFrame;
	globals.Shutdown = &WSWSTEAM::SteamLib_Shutdown;

	return &globals;
}

#if defined ( HAVE_DLLMAIN ) && !defined ( FTLIB_HARD_LINKED )
int _stdcall DLLMain( void *hinstDll, unsigned long dwReason, void *reserved )
{
	return 1;
}
#endif
