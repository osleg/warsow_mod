/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2013 Victor Luchits

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

// r_main.c

#include "r_local.h"

r_frontend_t rf;

mapconfig_t mapConfig;

refinst_t rn;

r_scene_t rsc;

/*
* R_TransformBounds
*/
void R_TransformBounds( const vec3_t origin, const mat3_t axis, vec3_t mins, vec3_t maxs, vec3_t bbox[8] )
{
	int i;
	vec3_t tmp;
	mat3_t axis_;

	Matrix3_Transpose( axis, axis_ );	// switch row-column order

	// rotate local bounding box and compute the full bounding box
	for( i = 0; i < 8; i++ )
	{
		vec_t *corner = bbox[i];

		corner[0] = ( ( i & 1 ) ? mins[0] : maxs[0] );
		corner[1] = ( ( i & 2 ) ? mins[1] : maxs[1] );
		corner[2] = ( ( i & 4 ) ? mins[2] : maxs[2] );

		Matrix3_TransformVector( axis_, corner, tmp );
		VectorAdd( tmp, origin, corner );
	}
}

/*
* R_ScissorForBounds
* 
* Returns the on-screen scissor box for given bounding box in 3D-space.
*/
qboolean R_ScissorForBounds( vec3_t bbox[8], int *x, int *y, int *w, int *h )
{
	int i;
	int ix1, iy1, ix2, iy2;
	float x1, y1, x2, y2;
	vec4_t corner = { 0, 0, 0, 1 }, proj = { 0, 0, 0, 1 }, v = { 0, 0, 0, 1 };

	x1 = y1 = 999999;
	x2 = y2 = -999999;
	for( i = 0; i < 8; i++ )
	{
		// compute and rotate the full bounding box
		VectorCopy( bbox[i], corner );

		Matrix4_Multiply_Vector( rn.cameraProjectionMatrix, corner, proj );

		if( proj[3] ) {
			v[0] = ( proj[0] / proj[3] + 1.0f ) * 0.5f * rn.refdef.width;
			v[1] = ( proj[1] / proj[3] + 1.0f ) * 0.5f * rn.refdef.height;
			v[2] = ( proj[2] / proj[3] + 1.0f ) * 0.5f; // [-1..1] -> [0..1]
		} else {
			v[0] = 999999.0f;
			v[1] = 999999.0f;
			v[2] = 999999.0f;
		}

		if( v[2] < 0 || v[2] > 1 )
		{
			// the test point is behind the nearclip or further than farclip
			if( PlaneDiff( corner, &rn.frustum[0] ) < PlaneDiff( corner, &rn.frustum[1] ) )
				v[0] = 0;
			else
				v[0] = rn.refdef.width;
			if( PlaneDiff( corner, &rn.frustum[2] ) < PlaneDiff( corner, &rn.frustum[3] ) )
				v[1] = 0;
			else
				v[1] = rn.refdef.height;
		}

		x1 = min( x1, v[0] ); y1 = min( y1, v[1] );
		x2 = max( x2, v[0] ); y2 = max( y2, v[1] );
	}

	ix1 = max( x1 - 1.0f, 0 ); ix2 = min( x2 + 1.0f, rn.refdef.width );
	if( ix1 >= ix2 )
		return qfalse; // FIXME

	iy1 = max( y1 - 1.0f, 0 ); iy2 = min( y2 + 1.0f, rn.refdef.height );
	if( iy1 >= iy2 )
		return qfalse; // FIXME

	*x = ix1;
	*y = rn.refdef.height - iy2;
	*w = ix2 - ix1;
	*h = iy2 - iy1;

	return qtrue;
}

/*
* R_ScissorForEntity
* 
* Returns the on-screen scissor box for given bounding box in 3D-space.
*/
qboolean R_ScissorForEntity( const entity_t *ent, vec3_t mins, vec3_t maxs, int *x, int *y, int *w, int *h )
{
	vec3_t bbox[8];

	R_TransformBounds( ent->origin, ent->axis, mins, maxs, bbox );

	return R_ScissorForBounds( bbox, x, y, w, h );
}

/*
* R_TransformForWorld
*/
void R_TransformForWorld( void )
{
	Matrix4_Identity( rn.objectMatrix );
	Matrix4_Copy( rn.cameraMatrix, rn.modelviewMatrix );

	RB_LoadObjectMatrix( rn.objectMatrix );
	RB_LoadModelviewMatrix( rn.modelviewMatrix );
}

/*
* R_TranslateForEntity
*/
static void R_TranslateForEntity( const entity_t *e )
{
	Matrix4_Identity( rn.objectMatrix );

	rn.objectMatrix[0] = e->scale;
	rn.objectMatrix[5] = e->scale;
	rn.objectMatrix[10] = e->scale;
	rn.objectMatrix[12] = e->origin[0];
	rn.objectMatrix[13] = e->origin[1];
	rn.objectMatrix[14] = e->origin[2];

	Matrix4_MultiplyFast( rn.cameraMatrix, rn.objectMatrix, rn.modelviewMatrix );

	RB_LoadObjectMatrix( rn.objectMatrix );
	RB_LoadModelviewMatrix( rn.modelviewMatrix );
}

/*
* R_TransformForEntity
*/
void R_TransformForEntity( const entity_t *e )
{
	if( e->rtype != RT_MODEL ) {
		R_TransformForWorld();
		return;
	}
	if( e == rsc.worldent ) {
		R_TransformForWorld();
		return;
	}

	if( e->scale != 1.0f ) {
		rn.objectMatrix[0] = e->axis[0] * e->scale;
		rn.objectMatrix[1] = e->axis[1] * e->scale;
		rn.objectMatrix[2] = e->axis[2] * e->scale;
		rn.objectMatrix[4] = e->axis[3] * e->scale;
		rn.objectMatrix[5] = e->axis[4] * e->scale;
		rn.objectMatrix[6] = e->axis[5] * e->scale;
		rn.objectMatrix[8] = e->axis[6] * e->scale;
		rn.objectMatrix[9] = e->axis[7] * e->scale;
		rn.objectMatrix[10] = e->axis[8] * e->scale;
	} else {
		rn.objectMatrix[0] = e->axis[0];
		rn.objectMatrix[1] = e->axis[1];
		rn.objectMatrix[2] = e->axis[2];
		rn.objectMatrix[4] = e->axis[3];
		rn.objectMatrix[5] = e->axis[4];
		rn.objectMatrix[6] = e->axis[5];
		rn.objectMatrix[8] = e->axis[6];
		rn.objectMatrix[9] = e->axis[7];
		rn.objectMatrix[10] = e->axis[8];
	}

	rn.objectMatrix[3] = 0;
	rn.objectMatrix[7] = 0;
	rn.objectMatrix[11] = 0;
	rn.objectMatrix[12] = e->origin[0];
	rn.objectMatrix[13] = e->origin[1];
	rn.objectMatrix[14] = e->origin[2];
	rn.objectMatrix[15] = 1.0;

	Matrix4_MultiplyFast( rn.cameraMatrix, rn.objectMatrix, rn.modelviewMatrix );

	RB_LoadObjectMatrix( rn.objectMatrix );
	RB_LoadModelviewMatrix( rn.modelviewMatrix );
}

/*
* R_LerpTag
*/
qboolean R_LerpTag( orientation_t *orient, const model_t *mod, int oldframe, int frame, float lerpfrac, const char *name )
{
	if( !orient )
		return qfalse;

	VectorClear( orient->origin );
	Matrix3_Identity( orient->axis );

	if( !name )
		return qfalse;

	if( mod->type == mod_alias )
		return R_AliasModelLerpTag( orient, (const maliasmodel_t *)mod->extradata, oldframe, frame, lerpfrac, name );

	return qfalse;
}

/*
* R_FogForBounds
*/
mfog_t *R_FogForBounds( const vec3_t mins, const vec3_t maxs )
{
	unsigned int i, j;
	mfog_t *fog;

	if( !rsh.worldModel || ( rn.refdef.rdflags & RDF_NOWORLDMODEL ) || !rsh.worldBrushModel->numfogs )
		return NULL;
	if( rn.renderFlags & RF_SHADOWMAPVIEW )
		return NULL;
	if( rsh.worldBrushModel->globalfog )
		return rsh.worldBrushModel->globalfog;

	fog = rsh.worldBrushModel->fogs;
	for( i = 0; i < rsh.worldBrushModel->numfogs; i++, fog++ )
	{
		if( !fog->shader )
			continue;

		for( j = 0; j < 3; j++ ) {
			if( mins[j] >= fog->maxs[j] ) {
				break;
			}
			if( maxs[j] <= fog->mins[j] ) {
				break;
			}
		}

		if( j == 3 ) {
			return fog;
		}
	}

	return NULL;
}

/*
* R_FogForSphere
*/
mfog_t *R_FogForSphere( const vec3_t centre, const float radius )
{
	int i;
	vec3_t mins, maxs;

	for( i = 0; i < 3; i++ ) {
		mins[i] = centre[i] - radius;
		maxs[i] = centre[i] + radius;
	}

	return R_FogForBounds( mins, maxs );
}

/*
* R_CompletelyFogged
*/
qboolean R_CompletelyFogged( const mfog_t *fog, vec3_t origin, float radius )
{
	// note that fog->distanceToEye < 0 is always true if
	// globalfog is not NULL and we're inside the world boundaries
	if( fog && fog->shader && fog == rn.fog_eye )
	{
		float vpnDist = ( ( rn.viewOrigin[0] - origin[0] ) * rn.viewAxis[AXIS_FORWARD+0] + 
			( rn.viewOrigin[1] - origin[1] ) * rn.viewAxis[AXIS_FORWARD+1] + 
			( rn.viewOrigin[2] - origin[2] ) * rn.viewAxis[AXIS_FORWARD+2] );
		return ( ( vpnDist + radius ) / fog->shader->fog_dist ) < -1;
	}

	return qfalse;
}

/*
* R_LODForSphere
*/
int R_LODForSphere( const vec3_t origin, float radius )
{
	float dist;
	int lod;

	dist = DistanceFast( origin, rn.lodOrigin );
	dist *= rn.lod_dist_scale_for_fov;

	lod = (int)( dist / radius );
	if( r_lodscale->integer )
		lod /= r_lodscale->integer;
	lod += r_lodbias->integer;

	if( lod < 1 )
		return 0;
	return lod;
}

/*
=============================================================

CUSTOM COLORS

=============================================================
*/

static byte_vec4_t r_customColors[NUM_CUSTOMCOLORS];

/*
* R_InitCustomColors
*/
void R_InitCustomColors( void )
{
	memset( r_customColors, 255, sizeof( r_customColors ) );
}

/*
* R_SetCustomColor
*/
void R_SetCustomColor( int num, int r, int g, int b )
{
	if( num < 0 || num >= NUM_CUSTOMCOLORS )
		return;
	Vector4Set( r_customColors[num], (qbyte)r, (qbyte)g, (qbyte)b, 255 );
}
/*
* R_GetCustomColor
*/
int R_GetCustomColor( int num )
{
	if( num < 0 || num >= NUM_CUSTOMCOLORS )
		return COLOR_RGBA( 255, 255, 255, 255 );
	return *(int *)r_customColors[num];
}

/*
* R_ShutdownCustomColors
*/
void R_ShutdownCustomColors( void )
{
	memset( r_customColors, 255, sizeof( r_customColors ) );
}

/*
=============================================================

SPRITE MODELS

=============================================================
*/

drawSurfaceType_t spriteDrawSurf = ST_SPRITE;

/*
* R_BeginSpriteSurf
*/
qboolean R_BeginSpriteSurf( const entity_t *e, const shader_t *shader, const mfog_t *fog, drawSurfaceType_t *drawSurf )
{
	RB_BindVBO( RB_VBO_STREAM_QUAD, GL_TRIANGLES );
	return qtrue;
}

/*
* R_BatchSpriteSurf
*/
void R_BatchSpriteSurf( const entity_t *e, const shader_t *shader, const mfog_t *fog, drawSurfaceType_t *drawSurf )
{
	int i;
	vec3_t point;
	vec3_t v_left, v_up;
	vec4_t xyz[4] = { {0,0,0,1}, {0,0,0,1}, {0,0,0,1}, {0,0,0,1} };
	vec4_t normals[4] = { {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} };
	byte_vec4_t colors[4];
	vec2_t texcoords[4] = { {0, 1}, {0, 0}, {1,0}, {1,1} };
	mesh_t mesh;
	float radius = e->radius * e->scale;
	float rotation = e->rotation;

	if( rotation )
	{
		RotatePointAroundVector( v_left, &rn.viewAxis[AXIS_FORWARD], &rn.viewAxis[AXIS_RIGHT], rotation );
		CrossProduct( &rn.viewAxis[AXIS_FORWARD], v_left, v_up );
	}
	else
	{
		VectorCopy( &rn.viewAxis[AXIS_RIGHT], v_left );
		VectorCopy( &rn.viewAxis[AXIS_UP], v_up );
	}

	if( rn.renderFlags & RF_MIRRORVIEW )
		VectorInverse( v_left );

	VectorMA( e->origin, -radius, v_up, point );
	VectorMA( point, radius, v_left, xyz[0] );
	VectorMA( point, -radius, v_left, xyz[3] );

	VectorMA( e->origin, radius, v_up, point );
	VectorMA( point, radius, v_left, xyz[1] );
	VectorMA( point, -radius, v_left, xyz[2] );

	for( i = 0; i < 4; i++ )
	{
		VectorNegate( &rn.viewAxis[AXIS_FORWARD], normals[i] );
		Vector4Copy( e->color, colors[i] );
	}

	// backend knows how to count elements for quads
	mesh.elems = NULL;
	mesh.numElems = 0;
	mesh.numVerts = 4;
	mesh.xyzArray = xyz;
	mesh.normalsArray = normals;
	mesh.lmstArray[0] = NULL;
	mesh.stArray = texcoords;
	mesh.colorsArray[0] = colors;
	mesh.colorsArray[1] = NULL;
	mesh.sVectorsArray = NULL;

	RB_BatchMesh( &mesh );
}

/*
* R_AddSpriteToDrawList
*/
static qboolean R_AddSpriteToDrawList( const entity_t *e )
{
	float dist;

	if( e->radius <= 0 || e->customShader == NULL || e->scale <= 0 ) {
		return qfalse;
	}

	dist =
		( e->origin[0] - rn.refdef.vieworg[0] ) * rn.viewAxis[AXIS_FORWARD+0] +
		( e->origin[1] - rn.refdef.vieworg[1] ) * rn.viewAxis[AXIS_FORWARD+1] +
		( e->origin[2] - rn.refdef.vieworg[2] ) * rn.viewAxis[AXIS_FORWARD+2];
	if( dist <= 0 )
		return qfalse; // cull it because we don't want to sort unneeded things

	if( !R_AddDSurfToDrawList( e, R_FogForSphere( e->origin, e->radius ), 
		e->customShader, dist, 0, NULL, &spriteDrawSurf ) ) {
		return qfalse;
	}

	return qtrue;
}

//==================================================================================

drawSurfaceType_t nullDrawSurf = ST_NULLMODEL;

/*
* R_InitNullModelVBO
*/
mesh_vbo_t *R_InitNullModelVBO( void )
{
	float scale = 15;
	vec4_t xyz[6] = { {0,0,0,1}, {0,0,0,1}, {0,0,0,1}, {0,0,0,1}, {0,0,0,1}, {0,0,0,1} };
	vec4_t normals[6] = { {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} };
	byte_vec4_t colors[6];
	vec2_t texcoords[6] = { {0,0}, {0,1}, {0,0}, {0,1}, {0,0}, {0,1} };
	elem_t elems[6] = { 0, 1, 2, 3, 4, 5 };
	mesh_t mesh;
	vattribmask_t vattribs = VATTRIB_POSITION_BIT | VATTRIB_TEXCOORDS_BIT | VATTRIB_COLOR0_BIT;
	mesh_vbo_t *vbo;
	
	vbo = R_CreateMeshVBO( &rf, 6, 6, 0, vattribs, VBO_TAG_NONE, vattribs );
	if( !vbo ) {
		return NULL;
	}

	VectorSet( xyz[0], 0, 0, 0 );
	VectorSet( xyz[1], scale, 0, 0 );
	Vector4Set( colors[0], 255, 0, 0, 127 );
	Vector4Set( colors[1], 255, 0, 0, 127 );

	VectorSet( xyz[2], 0, 0, 0 );
	VectorSet( xyz[3], 0, scale, 0 );
	Vector4Set( colors[2], 0, 255, 0, 127 );
	Vector4Set( colors[3], 0, 255, 0, 127 );

	VectorSet( xyz[4], 0, 0, 0 );
	VectorSet( xyz[5], 0, 0, scale );
	Vector4Set( colors[4], 0, 0, 255, 127 );
	Vector4Set( colors[5], 0, 0, 255, 127 );

	memset( &mesh, 0, sizeof( mesh ) );
	mesh.numVerts = 6;
	mesh.xyzArray = xyz;
	mesh.normalsArray = normals;
	mesh.stArray = texcoords;
	mesh.colorsArray[0] = colors;
	mesh.numElems = 6;
	mesh.elems = elems;

	R_UploadVBOVertexData( vbo, 0, vattribs, &mesh, VBO_HINT_NONE );
	R_UploadVBOElemData( vbo, 0, 0, &mesh, VBO_HINT_NONE );

	return vbo;
}

/*
* R_DrawNullSurf
*/
qboolean R_DrawNullSurf( const entity_t *e, const shader_t *shader, const mfog_t *fog, drawSurfaceType_t *drawSurf )
{
	assert( rsh.nullVBO != NULL );
	if( !rsh.nullVBO ) {
		return qfalse;
	}

	RB_BindVBO( rsh.nullVBO->index, GL_LINES );

	RB_DrawElements( 0, 6, 0, 6 );

	return qfalse;
}

/*
* R_AddNullSurfToDrawList
*/
static qboolean R_AddNullSurfToDrawList( const entity_t *e )
{
	if( !R_AddDSurfToDrawList( e, R_FogForSphere( e->origin, 0.1f ), 
		rsh.whiteShader, 0, 0, NULL, &nullDrawSurf ) ) {
		return qfalse;
	}

	return qtrue;
}

//==================================================================================

static vec4_t pic_xyz[4] = { {0,0,0,1}, {0,0,0,1}, {0,0,0,1}, {0,0,0,1} };
static vec4_t pic_normals[4] = { {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} };
static vec2_t pic_st[4];
static byte_vec4_t pic_colors[4];
static mesh_t pic_mesh = { 4, pic_xyz, pic_normals, NULL, pic_st, { 0, 0, 0, 0 }, { pic_colors, pic_colors, pic_colors, pic_colors }, 0, NULL };
static const shader_t *pic_mbuffer_shader;
static float pic_x_offset, pic_y_offset;

/*
* R_ResetStretchPic
*/
static void R_ResetStretchPic( void )
{
	pic_mbuffer_shader = NULL;
	pic_x_offset = pic_y_offset = 0;
}

/*
* R_BeginStretchBatch
*/
void R_BeginStretchBatch( const shader_t *shader, float x_offset, float y_offset )
{
	if( pic_mbuffer_shader != shader
		|| x_offset != pic_x_offset || y_offset != pic_y_offset ) {
		R_EndStretchBatch();

		pic_mbuffer_shader = shader;
		pic_x_offset = x_offset;
		pic_y_offset = y_offset;

		if( pic_x_offset != 0 || pic_y_offset != 0 ) {
			mat4_t translation;

			Matrix4_Identity( translation );
			Matrix4_Translate2D( translation, pic_x_offset, pic_y_offset );

			RB_LoadModelviewMatrix( translation );
		}

		RB_BindShader( NULL, shader, NULL );

		RB_BindVBO( RB_VBO_STREAM_QUAD, GL_TRIANGLES );

		RB_BeginBatch();
	}
}

/*
* R_EndStretchBatch
*/
void R_EndStretchBatch( void )
{
	if( !pic_mbuffer_shader ) {
		return;
	}

	// upload video right before rendering
	if( pic_mbuffer_shader->cin ) {
		R_UploadCinematicShader( pic_mbuffer_shader );
	}

	RB_EndBatch();

	// reset matrix
	if( pic_x_offset != 0 || pic_y_offset != 0 ) {
		RB_LoadModelviewMatrix( mat4x4_identity );
	}

	R_ResetStretchPic();
}

/*
* R_Set2DMode
*
* Note that this sets the viewport to size of the active framebuffer.
*/
void R_Set2DMode( qboolean enable )
{
	int width, height;

	width = rf.frameBufferWidth;
	height = rf.frameBufferHeight;

	if( rf.in2D == qtrue && enable == qtrue && width == rf.width2D && height == rf.height2D ) {
		return;
	} else if( rf.in2D == qfalse && enable == qfalse ) {
		return;
	}

	rf.in2D = enable;

	if( enable )
	{
		rf.width2D = width;
		rf.height2D = height;

		// reset 2D batching
		R_ResetStretchPic();

		Matrix4_OrthogonalProjection( 0, width, height, 0, -99999, 99999, rn.projectionMatrix );
		Matrix4_Copy( mat4x4_identity, rn.modelviewMatrix );
		Matrix4_Copy( rn.projectionMatrix, rn.cameraProjectionMatrix );

		// set 2D virtual screen size
		RB_Scissor( 0, 0, width, height );
		RB_Viewport( 0, 0, width, height );

		RB_LoadProjectionMatrix( rn.projectionMatrix );
		RB_LoadModelviewMatrix( rn.modelviewMatrix );

		RB_SetShaderStateMask( ~0, GLSTATE_NO_DEPTH_TEST );
	}
	else
	{
		// render previously batched 2D geometry, if any
		R_EndStretchBatch();

		RB_SetShaderStateMask( ~0, 0 );
	}
}

/*
* R_DrawRotatedStretchPic
*/
void R_DrawRotatedStretchPic( int x, int y, int w, int h, float s1, float t1, float s2, float t2, float angle, 
	const vec4_t color, const shader_t *shader )
{
	int bcolor;

	if( !shader ) {
		return;
	}

	R_BeginStretchBatch( shader, 0, 0 );

	// lower-left
	Vector2Set( pic_xyz[0], x, y );
	Vector2Set( pic_st[0], s1, t1 );
	Vector4Set( pic_colors[0], 
		fast_ftol( bound( 0, color[0] * 255, 255 ) ), 
		fast_ftol( bound( 0, color[1] * 255, 255 ) ),
		fast_ftol( bound( 0, color[2] * 255, 255 ) ), 
		fast_ftol( bound( 0, color[3] * 255, 255 ) ) );
	bcolor = *(int *)pic_colors[0];

	// lower-right
	Vector2Set( pic_xyz[1], x+w, y );
	Vector2Set( pic_st[1], s2, t1 );
	*(int *)pic_colors[1] = bcolor;

	// upper-right
	Vector2Set( pic_xyz[2], x+w, y+h );
	Vector2Set( pic_st[2], s2, t2 );
	*(int *)pic_colors[2] = bcolor;

	// upper-left
	Vector2Set( pic_xyz[3], x, y+h );
	Vector2Set( pic_st[3], s1, t2 );
	*(int *)pic_colors[3] = bcolor;

	// rotated image
	angle = anglemod( angle );
	if( angle ) {
		int j;
		float sint, cost;

		angle = angle / 360.0f;
		sint = sin( angle );
		cost = cos( angle );

		for( j = 0; j < 4; j++ )
		{
			t1 = pic_st[j][0];
			t2 = pic_st[j][1];
			pic_st[j][0] = cost * (t1 - 0.5f) - sint * (t2 - 0.5f) + 0.5f;
			pic_st[j][1] = cost * (t2 - 0.5f) + sint * (t1 - 0.5f) + 0.5f;
		}
	}

	RB_BatchMesh( &pic_mesh );
}

/*
* R_DrawStretchPic
*/
void R_DrawStretchPic( int x, int y, int w, int h, float s1, float t1, float s2, float t2, 
	const vec4_t color, const shader_t *shader )
{
	R_DrawRotatedStretchPic( x, y, w, h, s1, t1, s2, t2, 0, color, shader );
}

/*
* R_DrawStretchRaw
*
* Passing NULL for data redraws last uploaded frame
*/
void R_DrawStretchRaw( int x, int y, int w, int h, int cols, int rows, 
	float s1, float t1, float s2, float t2, qbyte *data )
{
	float h_scale, v_scale;

	if( !rows || !cols ) {
		return;
	}

	if( data ) {
		if( rsh.rawTexture->width != cols || rsh.rawTexture->height != rows ) {
			qbyte *nodata[1] = { NULL };
			R_ReplaceImage( rsh.rawTexture, nodata, cols, rows, rsh.rawTexture->flags, 3 );
		}
		R_ReplaceSubImage( rsh.rawTexture, &data, cols, rows );
	}

	h_scale = (float)rsh.rawTexture->width / rsh.rawTexture->upload_width;
	v_scale = (float)rsh.rawTexture->height / rsh.rawTexture->upload_height;
	s1 *= h_scale;
	s2 *= h_scale;
	t1 *= v_scale;
	t2 *= v_scale;

	R_DrawStretchQuick( x, y, w, h, s1, t1, s2, t2, colorWhite, GLSL_PROGRAM_TYPE_NONE, rsh.rawTexture, 0 );
}

/*
* R_DrawStretchRawYUVBuiltin
*
* Set bit 0 in 'flip' to flip the image horizontally
* Set bit 1 in 'flip' to flip the image vertically
*/
void R_DrawStretchRawYUVBuiltin( int x, int y, int w, int h, 
	float s1, float t1, float s2, float t2, 
	ref_img_plane_t *yuv, image_t **yuvTextures, int flip )
{
	static char *s_name = "$builtinyuv";
	static shaderpass_t p;
	static shader_t s;
	float h_scale, v_scale;
	float s2_, t2_;
	float h_ofs, v_ofs;

	s.vattribs = VATTRIB_POSITION_BIT|VATTRIB_TEXCOORDS_BIT;
	s.sort = SHADER_SORT_NEAREST;
	s.numpasses = 1;
	s.name = s_name;
	s.passes = &p;

	p.rgbgen.type = RGB_GEN_IDENTITY;
	p.alphagen.type = ALPHA_GEN_IDENTITY;
	p.tcgen = TC_GEN_BASE;
	p.images[0] = yuvTextures[0];
	p.images[1] = yuvTextures[1];
	p.images[2] = yuvTextures[2];
	p.flags = 0;
	p.program_type = GLSL_PROGRAM_TYPE_YUV;

	if( yuv ) {
		int i;

		for( i = 0; i < 3; i++ ) {
			qbyte *data = yuv[i].data;
			int flags = yuvTextures[i]->flags;
			int stride = yuv[i].stride;
			int height = yuv[i].height;

			if( stride < 0 ) {
				// negative stride flips the image vertically
				data = data + stride * height;
				flags = (flags & ~(IT_FLIPX|IT_FLIPY|IT_FLIPDIAGONAL)) | IT_FLIPY;
				stride = -stride;
			}

			if( yuvTextures[i]->width != stride || yuvTextures[i]->height != height ) {
				qbyte *nodata[1] = { NULL };
				R_ReplaceImage( yuvTextures[i], nodata, stride, height, flags, 1 );
			}
			R_ReplaceSubImage( yuvTextures[i], &data, stride, height );
		}
	}

	h_scale = (float)yuvTextures[0]->width / yuvTextures[0]->upload_width;
	v_scale = (float)yuvTextures[0]->height / yuvTextures[0]->upload_height;
	h_ofs = 1.0f / yuvTextures[0]->upload_width;
	v_ofs = 1.0f / yuvTextures[0]->upload_height;

	s1 *= h_scale;
	s2 *= h_scale;
	t1 *= v_scale;
	t2 *= v_scale;

	s2_ = s2;
	t2_ = t2;
	if( flip & 1 ) {
		s1 = s2_ - s1, s2 = s2_ - s2;
	}
	if( flip & 2 ) {
		t1 = t2_ - t1, t2 = t2_ - t2;
	}

	// avoid lerp seams
	if( s1 > s2 ) {
		s1 -= h_ofs, s2 += h_ofs;
	} else {
		s1 += h_ofs, s2 -= h_ofs;
	}
	if( t1 > t2 ) {
		t1 -= v_ofs, t2 += v_ofs;
	} else {
		t1 += v_ofs, t2 -= v_ofs;
	}

	R_DrawRotatedStretchPic( x, y, w, h, s1, t1, s2, t2, 0, colorWhite, &s );

	R_EndStretchBatch();
}

/*
* R_DrawStretchRawYUV
*
* Passing NULL for data redraws last uploaded frame
*/
void R_DrawStretchRawYUV( int x, int y, int w, int h, 
	float s1, float t1, float s2, float t2, ref_img_plane_t *yuv )
{
	R_DrawStretchRawYUVBuiltin( x, y, w, h, s1, t1, s2, t2, yuv, rsh.rawYUVTextures, 0 );
}

/*
* R_DrawStretchImage
*/
void R_DrawStretchQuick( int x, int y, int w, int h, float s1, float t1, float s2, float t2, 
	const vec4_t color, int program_type, image_t *image, int blendMask )
{
	static char *s_name = "$builtinimage";
	static shaderpass_t p;
	static shader_t s;
	static float rgba[4];

	s.vattribs = VATTRIB_POSITION_BIT|VATTRIB_TEXCOORDS_BIT;
	s.sort = SHADER_SORT_NEAREST;
	s.numpasses = 1;
	s.name = s_name;
	s.passes = &p;

	Vector4Copy( color, rgba );
	p.rgbgen.type = RGB_GEN_CONST;
	p.rgbgen.args = rgba;
	p.alphagen.type = ALPHA_GEN_CONST;
	p.alphagen.args = &rgba[3];
	p.tcgen = TC_GEN_BASE;
	p.images[0] = image;
	p.flags = blendMask;
	p.program_type = program_type;

	R_DrawRotatedStretchPic( x, y, w, h, s1, t1, s2, t2, 0, color, &s );

	R_EndStretchBatch();
}

/*
* R_BindFrameBufferObject
*/
void R_BindFrameBufferObject( int object )
{
	int width, height;

	RFB_GetObjectSize( object, &width, &height );

	rf.frameBufferWidth = width;
	rf.frameBufferHeight = height;

	RB_BindFrameBufferObject( object );

	RB_Viewport( rn.viewport[0], rn.viewport[1], rn.viewport[2], rn.viewport[3] );
	RB_Scissor( rn.scissor[0], rn.scissor[1], rn.scissor[2], rn.scissor[3] );
}

/*
* R_Scissor
*
* Set scissor region for 2D drawing. Passing a negative value
* for any of the variables sets the scissor region to full screen.
* x and y represent the bottom left corner of the region/rectangle.
*/
void R_Scissor( int x, int y, int w, int h )
{
	// flush batched 2D geometry
	R_EndStretchBatch();

	if( x < 0 || y < 0 || w < 0 || h < 0 ) {
		// reset
		RB_Scissor( 0, 0, rf.frameBufferWidth, rf.frameBufferHeight );
	}
	else {
		RB_Scissor( x, y, w, h );
	}
}

/*
* R_GetScissor
*/
void R_GetScissor( int *x, int *y, int *w, int *h )
{
	RB_GetScissor( x, y, w, h );
}

/*
* R_EnableScissor
*/
void R_EnableScissor( qboolean enable )
{
	RB_EnableScissor( enable );
}

/*
* R_PolyBlend
*/
static void R_PolyBlend( void )
{
	if( !r_polyblend->integer )
		return;
	if( rsc.refdef.blend[3] < 0.01f )
		return;

	R_Set2DMode( qtrue );
	R_DrawStretchPic( 0, 0, rf.frameBufferWidth, rf.frameBufferHeight, 0, 0, 1, 1, rsc.refdef.blend, rsh.whiteShader );
	R_EndStretchBatch();
}

/*
* R_ApplyBrightness
*/
static void R_ApplyBrightness( void )
{
	float c;
	vec4_t color;

	c = r_brightness->value;
	if( c < 0.005 )
		return;
	else if( c > 1.0 )
		c = 1.0;

	color[0] = color[1] = color[2] = c, color[3] = 1;

	R_Set2DMode( qtrue );
	R_DrawStretchQuick( 0, 0, rf.frameBufferWidth, rf.frameBufferHeight, 0, 0, 1, 1,
		color, GLSL_PROGRAM_TYPE_NONE, rsh.whiteTexture, GLSTATE_SRCBLEND_ONE|GLSTATE_DSTBLEND_ONE );
	R_EndStretchBatch();
}

//=======================================================================

/*
* R_DefaultFarClip
*/
float R_DefaultFarClip( void )
{
	float farclip_dist;

	if( rn.renderFlags & RF_SHADOWMAPVIEW ) {
		return rn.shadowGroup->projDist;
	} else if( rn.refdef.rdflags & RDF_NOWORLDMODEL ) {
		farclip_dist = 1024;
	} else if( rsh.worldModel && rsh.worldBrushModel->globalfog ) {
		farclip_dist = rsh.worldBrushModel->globalfog->shader->fog_dist;
	} else {
		farclip_dist = Z_NEAR;
	}

	return max( Z_NEAR, farclip_dist ) + Z_BIAS;
}

/*
* R_SetVisFarClip
*/
static void R_SetVisFarClip( void )
{
	int i;
	float dist;
	vec3_t tmp;
	float farclip_dist;

	if( !rsh.worldModel || ( rn.refdef.rdflags & RDF_NOWORLDMODEL ) ) {
		return;
	}

	farclip_dist = 0;
	for( i = 0; i < 8; i++ )
	{
		tmp[0] = ( ( i & 1 ) ? rn.visMins[0] : rn.visMaxs[0] );
		tmp[1] = ( ( i & 2 ) ? rn.visMins[1] : rn.visMaxs[1] );
		tmp[2] = ( ( i & 4 ) ? rn.visMins[2] : rn.visMaxs[2] );

		dist = DistanceSquared( tmp, rn.viewOrigin );
		farclip_dist = max( farclip_dist, dist );
	}

	farclip_dist = sqrt( farclip_dist );

	if( rsh.worldBrushModel->globalfog )
	{
		float fogdist = rsh.worldBrushModel->globalfog->shader->fog_dist;
		if( farclip_dist > fogdist )
			farclip_dist = fogdist;
		else
			rn.clipFlags &= ~16;
	}

	rn.farClip = max( Z_NEAR, farclip_dist ) + Z_BIAS;
}

/*
* R_SetupFrame
*/
static void R_SetupFrame( void )
{
	int viewcluster;
	int viewarea;

	// build the transformation matrix for the given view angles
	VectorCopy( rn.refdef.vieworg, rn.viewOrigin );
	Matrix3_Copy( rn.refdef.viewaxis, rn.viewAxis );

	rn.lod_dist_scale_for_fov = tan( rn.refdef.fov_x * ( M_PI/180 ) * 0.5f );

	// current viewcluster
	if( !( rn.refdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		mleaf_t *leaf;

		VectorCopy( rsh.worldModel->mins, rn.visMins );
		VectorCopy( rsh.worldModel->maxs, rn.visMaxs );

		leaf = Mod_PointInLeaf( rn.pvsOrigin, rsh.worldModel );
		viewcluster = leaf->cluster;
		viewarea = leaf->area;

		if( rf.worldModelSequence != rsh.worldModelSequence ) {
			rf.frameCount = 0;
			rf.viewcluster = -1; // force R_MarkLeaves
			rf.worldModelSequence = rsh.worldModelSequence;

			// load all world images if not yet
			R_FinishLoadingImages();
		}
	}
	else
	{
		viewcluster = -1;
		viewarea = -1;
	}

	rf.oldviewcluster = rf.viewcluster;
	rf.viewcluster = viewcluster;
	rf.viewarea = viewarea;

	rf.frameCount++;
}

/*
* R_SetupViewMatrices
*/
static void R_SetupViewMatrices( void )
{
	refdef_t *rd = &rn.refdef;

	Matrix4_Modelview( rd->vieworg, rd->viewaxis, rn.cameraMatrix );

	if( rd->rdflags & RDF_USEORTHO ) {
		Matrix4_OrthogonalProjection( -rd->ortho_x, rd->ortho_x, -rd->ortho_y, rd->ortho_y, 
			-rn.farClip, rn.farClip, rn.projectionMatrix );
	}
	else {
		Matrix4_PerspectiveProjection( rd->fov_x, rd->fov_y, 
			Z_NEAR, rn.farClip, rf.cameraSeparation, rn.projectionMatrix );
	}

	Matrix4_Multiply( rn.projectionMatrix, rn.cameraMatrix, rn.cameraProjectionMatrix );
}

/*
* R_Clear
*/
static void R_Clear( int bitMask )
{
	int bits;
	qbyte *envColor = rsh.worldModel && !( rn.refdef.rdflags & RDF_NOWORLDMODEL ) && rsh.worldBrushModel->globalfog ?
		rsh.worldBrushModel->globalfog->shader->fog_color : mapConfig.environmentColor;
	qboolean rgbShadow = ( rn.renderFlags & RF_SHADOWMAPVIEW ) && rn.fbColorAttachment != NULL ? qtrue : qfalse;

	bits = GL_DEPTH_BUFFER_BIT;

	if( ( !( rn.refdef.rdflags & RDF_NOWORLDMODEL ) && R_FASTSKY() ) || rgbShadow )
		bits |= GL_COLOR_BUFFER_BIT;
	if( glConfig.stencilEnabled )
		bits |= GL_STENCIL_BUFFER_BIT;

	bits &= bitMask;

	if( !( rn.renderFlags & RF_SHADOWMAPVIEW ) ) {
		RB_Clear( bits, envColor[0] / 255.0, envColor[1] / 255.0, envColor[2] / 255.0, 1 );
	}
	else {
		RB_Clear( bits, 1, 1, 1, 1 );
	}
}

/*
* R_SetupGL
*/
static void R_SetupGL( int clearBitMask )
{
	RB_Scissor( rn.scissor[0], rn.scissor[1], rn.scissor[2], rn.scissor[3] );
	RB_Viewport( rn.viewport[0], rn.viewport[1], rn.viewport[2], rn.viewport[3] );

	if( rn.renderFlags & RF_CLIPPLANE )
	{
		cplane_t *p = &rn.clipPlane;
		Matrix4_ObliqueNearClipping( p->normal, -p->dist, rn.cameraMatrix, rn.projectionMatrix );
	}

	RB_SetZClip( Z_NEAR, rn.farClip );

	RB_SetCamera( rn.viewOrigin, rn.viewAxis );

	RB_SetMinLight( rn.refdef.minLight );

	RB_SetRenderFlags( rn.renderFlags );

	RB_LoadProjectionMatrix( rn.projectionMatrix );

	RB_LoadModelviewMatrix( rn.cameraMatrix );

	if( rn.renderFlags & RF_FLIPFRONTFACE )
		RB_FlipFrontFace();

	if( ( rn.renderFlags & RF_SHADOWMAPVIEW ) && glConfig.ext.shadow )
		RB_SetShaderStateMask( ~0, GLSTATE_NO_COLORWRITE );

	R_Clear( clearBitMask );
}

/*
* R_EndGL
*/
static void R_EndGL( void )
{
	if( ( rn.renderFlags & RF_SHADOWMAPVIEW ) && glConfig.ext.shadow )
		RB_SetShaderStateMask( ~0, 0 );

	if( rn.renderFlags & RF_FLIPFRONTFACE )
		RB_FlipFrontFace();
}

/*
* R_DrawEntities
*/
static void R_DrawEntities( void )
{
	unsigned int i;
	entity_t *e;
	qboolean shadowmap = ( ( rn.renderFlags & RF_SHADOWMAPVIEW ) != 0 );
	qboolean culled = qtrue;

	if( rn.renderFlags & RF_NOENTS )
		return;

	if( rn.renderFlags & RF_ENVVIEW )
	{
		for( i = 0; i < rsc.numBmodelEntities; i++ )
		{
			e = rsc.bmodelEntities[i];
			if( !r_lerpmodels->integer )
				e->backlerp = 0;
			e->outlineHeight = rsc.worldent->outlineHeight;
			Vector4Copy( rsc.worldent->outlineRGBA, e->outlineColor );
			R_AddBrushModelToDrawList( e );
		}
		return;
	}

	for( i = 1; i < rsc.numEntities; i++ )
	{
		e = R_NUM2ENT(i);
		culled = qtrue;

		if( !r_lerpmodels->integer )
			e->backlerp = 0;

		switch( e->rtype )
		{
		case RT_MODEL:
			if( !e->model ) {
				R_AddNullSurfToDrawList( e );
				continue;
			}

			switch( e->model->type )
			{
			case mod_alias:
				culled = ! R_AddAliasModelToDrawList( e );
				break;
			case mod_skeletal:
				culled = ! R_AddSkeletalModelToDrawList( e );
				break;
			case mod_brush:
				e->outlineHeight = rsc.worldent->outlineHeight;
				Vector4Copy( rsc.worldent->outlineRGBA, e->outlineColor );
				culled = ! R_AddBrushModelToDrawList( e );
			default:
				break;
			}
			break;
		case RT_SPRITE:
			culled = ! R_AddSpriteToDrawList( e );
			break;
		default:
			break;
		}

		if( shadowmap && !culled ) {
			if( rsc.entShadowGroups[i] != rn.shadowGroup->id ||
				r_shadows_self_shadow->integer ) {
				// not from the casting group, mark as shadowed
				rsc.entShadowBits[i] |= rn.shadowGroup->bit;
			}
		}
	}
}

//=======================================================================

/*
* R_BindRefInstFBO
*/
static void R_BindRefInstFBO( void )
{
	int fbo;

	if( rn.fbColorAttachment ) {
		fbo = rn.fbColorAttachment->fbo;
	}
	else if( rn.fbDepthAttachment ) {
		fbo = rn.fbDepthAttachment->fbo;
	}
	else {
		fbo = 0;
	}

	R_BindFrameBufferObject( fbo );

	if( fbo && !rn.fbColorAttachment ) {
		// inform the driver we do not wish to render to the color buffer
		RFB_DisableObjectDrawBuffer();
	}
}

/*
* R_RenderView
*/
void R_RenderView( const refdef_t *fd )
{
	int msec = 0;
	qboolean shadowMap = rn.renderFlags & RF_SHADOWMAPVIEW ? qtrue : qfalse;

	rn.refdef = *fd;
	rn.numVisSurfaces = 0;

	// load view matrices with default far clip value
	R_SetupViewMatrices();

	rn.fog_eye = NULL;

	rn.shadowBits = 0;
	rn.dlightBits = 0;
	
	rn.numPortalSurfaces = 0;

	ClearBounds( rn.visMins, rn.visMaxs );

	R_ClearSky();

	// enable PVS culling for some rendering instances
	if( rn.refdef.rdflags & RDF_PORTALINVIEW
		|| ((rn.refdef.rdflags & RDF_SKYPORTALINVIEW) && !rn.refdef.skyportal.noEnts) ) {
		rn.renderFlags |= RF_PVSCULL;
	}

	if( r_novis->integer ) {
		rn.renderFlags |= RF_NOVIS;
	}

	if( r_lightmap->integer ) {
		rn.renderFlags |= RF_LIGHTMAP;
	}

	if( r_drawflat->integer ) {
		rn.renderFlags |= RF_DRAWFLAT;
	}

	R_ClearDrawList();

	if( !rsh.worldModel && !( rn.refdef.rdflags & RDF_NOWORLDMODEL ) )
		ri.Com_Error( ERR_DROP, "R_RenderView: NULL worldmodel" );

	R_SetupFrame();

	R_SetupFrustum( &rn.refdef, rn.farClip, rn.frustum );

	// we know the initial farclip at this point after determining visible world leafs
	// R_DrawEntities can make adjustments as well

	if( !shadowMap ) {
		if( r_speeds->integer )
			msec = ri.Sys_Milliseconds();
		R_MarkLeaves();
		if( r_speeds->integer )
			rf.stats.t_mark_leaves += ( ri.Sys_Milliseconds() - msec );

		if( ! ( rn.refdef.rdflags & RDF_NOWORLDMODEL ) ) {
			R_DrawWorld();

			if( !rn.numVisSurfaces ) {
				// no world surfaces visible
				return;
			}

			rn.fog_eye = R_FogForSphere( rn.viewOrigin, 0.5 );
		}

		R_DrawCoronas();

		if( r_speeds->integer )
			msec = ri.Sys_Milliseconds();
		R_DrawPolys();
		if( r_speeds->integer )
			rf.stats.t_add_polys += ( ri.Sys_Milliseconds() - msec );
	}

	if( r_speeds->integer )
		msec = ri.Sys_Milliseconds();
	R_DrawEntities();
	if( r_speeds->integer )
		rf.stats.t_add_entities += ( ri.Sys_Milliseconds() - msec );

	if( !shadowMap ) {
		// now set  the real far clip value and reload view matrices
		R_SetVisFarClip();

		R_SetupViewMatrices();

		// render to depth textures, mark shadowed entities and surfaces
		R_DrawShadowmaps();
	}

	R_SortDrawList();

	R_BindRefInstFBO();

	R_SetupGL( ~0 );

	R_DrawPortals();

	if( r_portalonly->integer && !( rn.renderFlags & ( RF_MIRRORVIEW|RF_PORTALVIEW ) ) )
		return;

	if( r_speeds->integer )
		msec = ri.Sys_Milliseconds();
	R_DrawSurfaces();
	if( r_speeds->integer )
		rf.stats.t_draw_meshes += ( ri.Sys_Milliseconds() - msec );

	if( r_showtris->integer )
		R_DrawOutlinedSurfaces();

	R_TransformForWorld();

	R_EndGL();
}

#define REFINST_STACK_SIZE	64
static refinst_t riStack[REFINST_STACK_SIZE];
static unsigned int riStackSize;

/*
* R_ClearRefInstStack
*/
void R_ClearRefInstStack( void )
{
	riStackSize = 0;
}

/*
* R_PushRefInst
*/
qboolean R_PushRefInst( void )
{
	if( riStackSize == REFINST_STACK_SIZE ) {
		return qfalse;
	}
	riStack[riStackSize++] = rn;
	R_EndGL();
	return qtrue;
}

/*
* R_PopRefInst
*/
void R_PopRefInst( int clearBitMask )
{
	if( !riStackSize ) {
		return;
	}

	rn = riStack[--riStackSize];
	R_BindRefInstFBO();

	R_SetupGL( clearBitMask );
}

//=======================================================================

/*
* R_SwapInterval
*/
static void R_SwapInterval( qboolean swapInterval )
{
	if( !glConfig.stereoEnabled )
	{
		if( qglSwapInterval )
			qglSwapInterval( swapInterval );
	}
}

/*
* R_UpdateSwapInterval
*/
static void R_UpdateSwapInterval( void )
{
	if( r_swapinterval->modified )
	{
		r_swapinterval->modified = qfalse;
		R_SwapInterval( r_swapinterval->integer ? qtrue : qfalse );
	}
}

/*
* R_UpdateHWGamma
*/
static void R_UpdateHWGamma( void )
{
	int i, v;
	double invGamma, div;
	unsigned short gammaRamp[3*GAMMARAMP_STRIDE];

	if( !glConfig.hwGamma )
		return;

	invGamma = 1.0 / bound( 0.5, r_gamma->value, 3.0 );
	div = (double)( 1 << 0 ) / (glConfig.gammaRampSize - 0.5);

	for( i = 0; i < glConfig.gammaRampSize; i++ )
	{
		v = ( int )( 65535.0 * pow( ( (double)i + 0.5 ) * div, invGamma ) + 0.5 );
		gammaRamp[i] = gammaRamp[i + GAMMARAMP_STRIDE] = gammaRamp[i + 2*GAMMARAMP_STRIDE] = ( ( unsigned short )bound( 0, v, 65535 ) );
	}

	GLimp_SetGammaRamp( GAMMARAMP_STRIDE, glConfig.gammaRampSize, gammaRamp );
}

/*
* R_BeginFrame
*/
void R_BeginFrame( float cameraSeparation, qboolean forceClear, qboolean forceVsync )
{
	GLimp_BeginFrame();

	RB_BeginFrame();

	rf.cameraSeparation = cameraSeparation;
	if( cameraSeparation < 0 && glConfig.stereoEnabled )
	{
		qglDrawBuffer( GL_BACK_LEFT );
	}
	else if( cameraSeparation > 0 && glConfig.stereoEnabled )
	{
		qglDrawBuffer( GL_BACK_RIGHT );
	}
	else
	{
		qglDrawBuffer( GL_BACK );
	}

	if( mapConfig.forceClear )
		forceClear = qtrue;

	if( r_clear->integer || forceClear )
	{
		const qbyte *color = mapConfig.environmentColor;
		RB_Clear( GL_COLOR_BUFFER_BIT, 
			color[0]*( 1.0/255.0 ), color[1]*( 1.0/255.0 ), color[2]*( 1.0/255.0 ), 1 );
	}

	// update gamma
	if( r_gamma->modified )
	{
		r_gamma->modified = qfalse;
		R_UpdateHWGamma();
	}

	if( r_wallcolor->modified || r_floorcolor->modified ) {
		int i;

		// parse and clamp colors for walls and floors we will copy into our texture
		sscanf( r_wallcolor->string,  "%3f %3f %3f", &rsh.wallColor[0], &rsh.wallColor[1], &rsh.wallColor[2] );
		sscanf( r_floorcolor->string, "%3f %3f %3f", &rsh.floorColor[0], &rsh.floorColor[1], &rsh.floorColor[2] );
		for( i = 0; i < 3; i++ ) {
			rsh.wallColor[i] = bound( 0, floor(rsh.wallColor[i]) / 255.0, 1.0 );
			rsh.floorColor[i] = bound( 0, floor(rsh.floorColor[i]) / 255.0, 1.0 );
		}

		r_wallcolor->modified = r_floorcolor->modified = qfalse;
	}

	// run cinematic passes on shaders
	R_RunAllCinematics();

	// draw buffer stuff
	if( gl_drawbuffer->modified )
	{
		gl_drawbuffer->modified = qfalse;

		if( cameraSeparation == 0 || !glConfig.stereoEnabled )
		{
			if( Q_stricmp( gl_drawbuffer->string, "GL_FRONT" ) == 0 )
				qglDrawBuffer( GL_FRONT );
			else
				qglDrawBuffer( GL_BACK );
		}
	}

	// texturemode stuff
	if( r_texturemode->modified )
	{
		R_TextureMode( r_texturemode->string );
		r_texturemode->modified = qfalse;
	}

	if( r_texturefilter->modified )
	{
		R_AnisotropicFilter( r_texturefilter->integer );
		r_texturefilter->modified = qfalse;
	}

	// keep r_outlines_cutoff value in sane bounds to prevent wallhacking
	if( r_outlines_scale->modified ) {
		if( r_outlines_scale->value < 0 ) {
			ri.Cvar_ForceSet( r_outlines_scale->name, "0" );
		}
		else if( r_outlines_scale->value > 3 ) {
			ri.Cvar_ForceSet( r_outlines_scale->name, "3" );
		}
		r_outlines_scale->modified = qfalse;
	}

	// swapinterval stuff (vertical synchronization)
	if( forceVsync ) {
		R_SwapInterval( qtrue );
	}
	else {
		r_swapinterval->modified = qtrue;
		R_UpdateSwapInterval();
	}

	memset( &rf.stats, 0, sizeof( rf.stats ) );

	R_Set2DMode( qtrue );
}

/*
* R_EndFrame
*/
void R_EndFrame( void )
{
	// render previously batched 2D geometry, if any
	R_EndStretchBatch();

	R_PolyBlend();
	
	R_ApplyBrightness();

	// reset the 2D state so that the mode will be 
	// properly set back again in R_BeginFrame
	R_Set2DMode( qfalse );

	RB_EndFrame();

	GLimp_EndFrame();
}

/*
* R_AppActivate
*/
void R_AppActivate( qboolean active, qboolean destroy )
{
	qglFlush();

	GLimp_AppActivate( active, destroy );
}

/*
* R_SpeedsMessage
*/
const char *R_SpeedsMessage( char *out, size_t size )
{
	char backend_msg[1024];

	if( !out || !size ) {
		return out;
	}

	out[0] = '\0';
	if( r_speeds->integer && !( rn.refdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		switch( r_speeds->integer )
		{
		case 1:
		default:
			RB_StatsMessage( backend_msg, sizeof( backend_msg ) );

			Q_snprintfz( out, size,
				"%4i wpoly %4i leafs\n"
				"%s",
				rf.stats.c_brush_polys, rf.stats.c_world_leafs,
				backend_msg
			);
			break;
		case 2:
		case 3:
			Q_snprintfz( out, size,
				"lvs: %5i  node: %5i\n"
				"polys\\ents: %5i\\%5i  draw: %5i",
				rf.stats.t_mark_leaves, rf.stats.t_world_node,
				rf.stats.t_add_polys, rf.stats.t_add_entities, rf.stats.t_draw_meshes
			);
			break;
		case 4:
		case 5:
			if( rsc.debugSurface )
			{
				int numVerts = 0, numTris = 0;

				Q_snprintfz( out, size,
					"%s type:%i sort:%i", 
					rsc.debugSurface->shader->name, rsc.debugSurface->facetype, rsc.debugSurface->shader->sort );

				Q_strncatz( out, "\n", size );

				if( r_speeds->integer == 5 && rsc.debugSurface->drawSurf->vbo ) {
					numVerts = rsc.debugSurface->drawSurf->vbo->numVerts;
					numTris = rsc.debugSurface->drawSurf->vbo->numElems / 3;
				}
				else if( rsc.debugSurface->mesh ) {
					numVerts = rsc.debugSurface->mesh->numVerts;
					numTris = rsc.debugSurface->mesh->numElems;
				}

				if( numVerts ) {
					Q_snprintfz( out + strlen( out ), size - strlen( out ),
						"verts: %5i tris: %5i", numVerts, numTris );
				}

				Q_strncatz( out, "\n", size );

				if( rsc.debugSurface->fog && rsc.debugSurface->fog->shader
					&& rsc.debugSurface->fog->shader != rsc.debugSurface->shader )
					Q_strncatz( out, rsc.debugSurface->fog->shader->name, size );
			}
			break;
		case 6:
			Q_snprintfz( out, size,
				"%.1f %.1f %.1f",
				rn.refdef.vieworg[0], rn.refdef.vieworg[1], rn.refdef.vieworg[2]
				);
			break;
		}
	}

	out[size-1] = '\0';
	return out;
}

//==================================================================================

/*
* R_BeginAviDemo
*/
void R_BeginAviDemo( void )
{
}

/*
* R_WriteAviFrame
*/
void R_WriteAviFrame( int frame, qboolean scissor )
{
	int x, y, w, h;
	int checkname_size;
	int quality;
	char *checkname;
	const char *extension;

	if( scissor )
	{
		x = rsc.refdef.x;
		y = glConfig.height - rsc.refdef.height - rsc.refdef.y;
		w = rsc.refdef.width;
		h = rsc.refdef.height;
	}
	else
	{
		x = 0;
		y = 0;
		w = glConfig.width;
		h = glConfig.height;
	}

	if( r_screenshot_jpeg->integer ) {
		extension = ".jpg";
		quality = r_screenshot_jpeg_quality->integer;
	}
	else {
		extension = ".tga";
		quality = 100;
	}

	checkname_size = sizeof( char ) * ( strlen( "avi/avi" ) + 6 + strlen( extension ) + 1 );
	checkname = malloc( checkname_size );
	Q_snprintfz( checkname, checkname_size, "avi/avi%06i", frame );
	COM_DefaultExtension( checkname, ".jpg", checkname_size );

	R_ScreenShot( checkname, x, y, w, h, quality, qfalse, qfalse, qfalse, qtrue );

	free( checkname );
}

/*
* R_StopAviDemo
*/
void R_StopAviDemo( void )
{
}

/*
* R_TransformVectorToScreen
*/
void R_TransformVectorToScreen( const refdef_t *rd, const vec3_t in, vec2_t out )
{
	refdef_t trd;
	mat4_t p, m;
	vec4_t temp, temp2;

	if( !rd || !in || !out )
		return;

	trd = *rd;
	if( glConfig.wideScreen && !( trd.rdflags & RDF_NOFOVADJUSTMENT ) ) {
		AdjustFov( &trd.fov_x, &trd.fov_y, glConfig.width, glConfig.height, qfalse );
	}

	temp[0] = in[0];
	temp[1] = in[1];
	temp[2] = in[2];
	temp[3] = 1.0f;
	
	if( trd.rdflags & RDF_USEORTHO ) {
		Matrix4_OrthogonalProjection( trd.ortho_x, trd.ortho_x, trd.ortho_y, trd.ortho_y, 
			-4096.0f, 4096.0f, p );
	}
	else {
		Matrix4_PerspectiveProjection( trd.fov_x, trd.fov_y, Z_NEAR, rn.farClip, 
			rf.cameraSeparation, p );
	}

	Matrix4_Modelview( trd.vieworg, trd.viewaxis, m );

	Matrix4_Multiply_Vector( m, temp, temp2 );
	Matrix4_Multiply_Vector( p, temp2, temp );

	if( !temp[3] )
		return;

	out[0] = trd.x + ( temp[0] / temp[3] + 1.0f ) * trd.width * 0.5f;
	out[1] = glConfig.height - (trd.y + ( temp[1] / temp[3] + 1.0f ) * trd.height * 0.5f);
}

/*
* R_GetShaderForOrigin
*
* Trace 64 units in all axial directions to find the closest surface
*/
shader_t *R_GetShaderForOrigin( const vec3_t origin )
{
	int i, j;
	vec3_t dir, end;
	rtrace_t tr;
	shader_t *best = NULL;
	float best_frac = 1000.0f;

	for( i = 0; i < 3; i++ ) {
		VectorClear( dir );

		for( j = -1; j <= 1; j += 2 ) {
			dir[i] = j;
			VectorMA( origin, 64, dir, end );

			R_TraceLine( &tr, origin, end, 0 );
			if( !tr.shader ) {
				continue;
			}

			if( tr.fraction < best_frac ) {
				best = tr.shader;
				best_frac = tr.fraction;
			}
		}
	}

	return best;
}

/*
* R_GetShaderCinematic
*/
struct cinematics_s *R_GetShaderCinematic( shader_t *shader )
{
	if( !shader ) {
		return NULL;
	}
	return R_GetCinematicById( shader->cin );
}

//===================================================================

/*
* R_NormToLatLong
*/
void R_NormToLatLong( const vec_t *normal, qbyte latlong[2] )
{
	float flatlong[2];

	NormToLatLong( normal, flatlong );
	latlong[0] = (int)( flatlong[0] * 255.0 / M_TWOPI ) & 255;
	latlong[1] = (int)( flatlong[1] * 255.0 / M_TWOPI ) & 255;
}

/*
* R_LatLongToNorm4
*/
void R_LatLongToNorm4( const qbyte latlong[2], vec4_t out )
{
	static float * const sinTable = rsh.sinTableByte;
	float sin_a, sin_b, cos_a, cos_b;

	cos_a = sinTable[( latlong[0] + 64 ) & 255];
	sin_a = sinTable[latlong[0]];
	cos_b = sinTable[( latlong[1] + 64 ) & 255];
	sin_b = sinTable[latlong[1]];

	Vector4Set( out, cos_b * sin_a, sin_b * sin_a, cos_a, 0 );
}

/*
* R_LatLongToNorm
*/
void R_LatLongToNorm( const qbyte latlong[2], vec3_t out )
{
	vec4_t t;
	R_LatLongToNorm4( latlong, t );
	VectorCopy( t, out );
}

/*
* R_CopyString
*/
char *R_CopyString_( const char *in, const char *filename, int fileline )
{
	char *out;

	out = ri.Mem_AllocExt( r_mempool, ( strlen( in ) + 1 ), 0, 1, filename, fileline );
	strcpy( out, in );

	return out;
}

/*
* R_LoadFile
*/
int R_LoadFile_( const char *path, void **buffer, const char *filename, int fileline )
{
	qbyte *buf;
	unsigned int len;
	int fhandle;

	buf = NULL; // quiet compiler warning

	// look for it in the filesystem or pack files
	len = ri.FS_FOpenFile( path, &fhandle, FS_READ );
	if( !fhandle )
	{
		if( buffer )
			*buffer = NULL;
		return -1;
	}

	if( !buffer )
	{
		ri.FS_FCloseFile( fhandle );
		return len;
	}

	buf = ( qbyte *)ri.Mem_AllocExt( r_mempool, len + 1, 16, 0, filename, fileline );
	buf[len] = 0;
	*buffer = buf;

	ri.FS_Read( buf, len, fhandle );
	ri.FS_FCloseFile( fhandle );

	return len;
}

/*
* R_FreeFile
*/
void R_FreeFile_( void *buffer, const char *filename, int fileline )
{
	ri.Mem_Free( buffer, filename, fileline );
}
