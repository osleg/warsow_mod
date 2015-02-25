/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2002-2007 Victor Luchits

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifndef __R_GLIMP_H__
#define __R_GLIMP_H__

#ifdef __cplusplus
#define QGL_EXTERN extern "C"
#else
#define QGL_EXTERN extern
#endif

#ifdef _WIN32
#include <windows.h>

#define QGL_WGL( type, name, params ) QGL_EXTERN type( APIENTRY * q ## name ) params;
#define QGL_WGL_EXT( type, name, params ) QGL_EXTERN type( APIENTRY * q ## name ) params;
#define QGL_GLX( type, name, params )
#define QGL_GLX_EXT( type, name, params )
#endif

#if defined ( __linux__ ) || defined ( __FreeBSD__ )
#define QGL_WGL( type, name, params )
#define QGL_WGL_EXT( type, name, params )
#define QGL_GLX( type, name, params ) QGL_EXTERN type( APIENTRY * q ## name ) params;
#define QGL_GLX_EXT( type, name, params ) QGL_EXTERN type( APIENTRY * q ## name ) params;
#endif

#if defined ( __MACOSX__ )
#define QGL_WGL( type, name, params )
#define QGL_WGL_EXT( type, name, params )
#define QGL_GLX( type, name, params )
#define QGL_GLX_EXT( type, name, params )
#endif

#define QGL_FUNC( type, name, params ) QGL_EXTERN type( APIENTRY * q ## name ) params;
#define QGL_EXT( type, name, params ) QGL_EXTERN type( APIENTRY * q ## name ) params;

#include "qgl.h"

#undef QGL_GLX_EXT
#undef QGL_GLX
#undef QGL_WGL_EXT
#undef QGL_WGL
#undef QGL_EXT
#undef QGL_FUNC

//====================================================================

#define MAX_TEXTURE_UNITS				8

#define MAX_GLSL_UNIFORM_BONES			100
#define MAX_GLSL_UNIFORM_INSTANCES		40

#define GAMMARAMP_STRIDE				4096

extern cvar_t *r_stencilbits;
extern cvar_t *gl_drawbuffer;
extern cvar_t *gl_driver;

//====================================================================

enum
{
	GLSTATE_NONE = 0,

	//
	// glBlendFunc args
	//
	GLSTATE_SRCBLEND_ZERO					= 1,
	GLSTATE_SRCBLEND_ONE					= 2,
	GLSTATE_SRCBLEND_DST_COLOR				= 1|2,
	GLSTATE_SRCBLEND_ONE_MINUS_DST_COLOR	= 4,
	GLSTATE_SRCBLEND_SRC_ALPHA				= 1|4,
	GLSTATE_SRCBLEND_ONE_MINUS_SRC_ALPHA	= 2|4,
	GLSTATE_SRCBLEND_DST_ALPHA				= 1|2|4,
	GLSTATE_SRCBLEND_ONE_MINUS_DST_ALPHA	= 8,

	GLSTATE_DSTBLEND_ZERO					= 16,
	GLSTATE_DSTBLEND_ONE					= 32,
	GLSTATE_DSTBLEND_SRC_COLOR				= 16|32,
	GLSTATE_DSTBLEND_ONE_MINUS_SRC_COLOR	= 64,
	GLSTATE_DSTBLEND_SRC_ALPHA				= 16|64,
	GLSTATE_DSTBLEND_ONE_MINUS_SRC_ALPHA	= 32|64,
	GLSTATE_DSTBLEND_DST_ALPHA				= 16|32|64,
	GLSTATE_DSTBLEND_ONE_MINUS_DST_ALPHA	= 128,

	GLSTATE_AFUNC_GT0						= 0x100,
	GLSTATE_AFUNC_LT128						= 0x200,
	GLSTATE_AFUNC_GE128						= 0x100|0x200,

	GLSTATE_NO_COLORWRITE					= 0x400,

	GLSTATE_DEPTHWRITE						= 0x800,
	GLSTATE_DEPTHFUNC_EQ					= 0x1000,

	GLSTATE_OFFSET_FILL						= 0x2000,
	GLSTATE_NO_DEPTH_TEST					= 0x4000,

	GLSTATE_STENCIL_TEST					= 0x8000,

	GLSTATE_MARK_END						= 0x10000 // SHADERPASS_MARK_BEGIN
};

#define GLSTATE_MASK		( GLSTATE_MARK_END-1 )

// #define SHADERPASS_SRCBLEND_MASK (((GLSTATE_SRCBLEND_DST_ALPHA)<<1)-GLSTATE_SRCBLEND_ZERO)
#define GLSTATE_SRCBLEND_MASK	0xF

// #define SHADERPASS_DSTBLEND_MASK (((GLSTATE_DSTBLEND_DST_ALPHA)<<1)-GLSTATE_DSTBLEND_ZERO)
#define GLSTATE_DSTBLEND_MASK	0xF0

#define GLSTATE_BLEND_ADD		( GLSTATE_SRCBLEND_ONE|GLSTATE_DSTBLEND_ONE )

#define GLSTATE_ALPHAFUNC		( GLSTATE_AFUNC_GT0|GLSTATE_AFUNC_LT128|GLSTATE_AFUNC_GE128 )

//====================================================================

typedef struct
{
	int			_extMarker;

	//
	// only qbytes must follow the extensionsBoolMarker
	//

	char		draw_range_elements
				,multitexture
				,texture_cube_map
				,texture_edge_clamp
				,texture_filter_anisotropic
				,texture_non_power_of_two
				,texture_compression
				,vertex_buffer_object
				,GLSL
				,GLSL130
				,depth_texture
				,shadow
				,framebuffer_object
				,vertex_shader
				,fragment_shader
				,shader_objects
				,shading_language_100
				,shading_language_130
				,bgra
				,gamma_control
				,swap_control
				,draw_instanced
				,instanced_arrays
				,gpu_memory_info
				,meminfo
				,framebuffer_blit
				,half_float_vertex
				,get_program_binary
				;
} glextinfo_t;

typedef struct
{
	const char		*rendererString;
	const char		*vendorString;
	const char		*versionString;
	const char		*extensionsString;
	const char		*glwExtensionsString;
	const char		*shadingLanguageVersionString;
	unsigned		versionHash;

	int				shadingLanguageVersion;

	int				width, height;
	qboolean		fullScreen;
	qboolean		wideScreen;

	qboolean		stereoEnabled;
	qboolean		stencilEnabled;

	qboolean		hwGamma;
	unsigned short	gammaRampSize;
	unsigned short	originalGammaRamp[3*GAMMARAMP_STRIDE];

	int				maxTextureSize
					,maxTextureUnits
					,maxTextureCubemapSize
					,maxTextureSize3D
					,maxTextureFilterAnisotropic
					,maxVaryingFloats
					,maxVertexUniformComponents
					,maxVertexAttribs
					,maxFragmentUniformComponents
					;

	unsigned int	maxGLSLBones;	// the maximum amount of bones we can handle in a vertex shader

	glextinfo_t		ext;
} glconfig_t;

extern glconfig_t	glConfig;

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void		GLimp_BeginFrame( void );
void		GLimp_EndFrame( void );
int			GLimp_Init( const char *applicationName, void *hinstance, void *wndproc, void *parenthWnd );
void	    GLimp_Shutdown( void );
rserr_t		GLimp_SetMode( int x, int y, int width, int height, int displayFrequency,
				qboolean fullscreen, qboolean wideScreen );
void	    GLimp_AppActivate( qboolean active, qboolean destroy );
qboolean	GLimp_GetGammaRamp( size_t stride, unsigned short *psize, unsigned short *ramp );
void		GLimp_SetGammaRamp( size_t stride, unsigned short   size, unsigned short *ramp );

void		*GLimp_SharedContext_Create( void );
qboolean	GLimp_SharedContext_MakeCurrent( void *ctx );
void		GLimp_SharedContext_Destroy( void *ctx );

#endif /*__R_GLIMP_H__*/
