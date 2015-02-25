#ifndef _STEAMLIB_LOCAL_H_
#define _STEAMLIB_LOCAL_H_

#include "steamlib_public.h"
#include "steamlib_syscalls.h"

namespace WSWSTEAM {

int SteamLib_API( void );
int SteamLib_Init( void );
void SteamLib_RunFrame( void );
void SteamLib_Shutdown( void );
uint64_t SteamLib_GetSteamID( void );
int SteamLib_GetAuthSessionTicket( void (*callback)( void *, size_t ) );

}

#endif
