#ifndef _STEAMLIB_PUBLIC_H_
#define _STEAMLIB_PUBLIC_H_

#include <stdint.h>

// steamlib_public.h - steam integration subsystem

#define	STEAMLIB_API_VERSION 1

//===============================================================

//
// functions provided by the main engine
//
typedef struct
{
	// halts the application or drops to console
	void ( *Com_Error )( int code, const char *format, ... );

	// console messages
	void ( *Com_Printf )( const char *format, ... );
	void ( *Com_DPrintf )( const char *format, ... );
} steamlib_import_t;

//
// functions exported by the steam integration subsystem
//
typedef struct
{
	// if API is different, the dll cannot be used
	int ( *API )( void );

	int ( *Init )( void );
	void ( *RunFrame )( void );
	void ( *Shutdown )( void );

	uint64_t ( *GetSteamID )( void );
	int ( *GetAuthSessionTicket )( void (*callback)( void *, size_t ) );
} steamlib_export_t;

#endif
