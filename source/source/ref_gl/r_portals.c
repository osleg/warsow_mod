/*
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

#include "r_local.h"

/*
* R_AddPortalSurface
*/
portalSurface_t *R_AddPortalSurface( const entity_t *ent, const mesh_t *mesh, 
	const vec3_t mins, const vec3_t maxs, const shader_t *shader )
{
	unsigned int i;
	float dist;
	cplane_t plane, untransformed_plane;
	vec3_t v[3];
	portalSurface_t *portalSurface;

	if( !mesh ) {
		return NULL;
	}

	if( R_FASTSKY() && !( shader->flags & (SHADER_PORTAL_CAPTURE|SHADER_PORTAL_CAPTURE2) ) ) {
		// r_fastsky doesn't affect portalmaps
		return NULL;
	}

	for( i = 0; i < 3; i++ ) {
		VectorCopy( mesh->xyzArray[mesh->elems[i]], v[i] );
	}

	PlaneFromPoints( v, &untransformed_plane );
	untransformed_plane.dist += DotProduct( ent->origin, untransformed_plane.normal );
	CategorizePlane( &untransformed_plane );

	if( shader->flags & SHADER_AUTOSPRITE )
	{
		vec3_t centre;

		// autosprites are quads, facing the viewer
		if( mesh->numVerts < 4 ) {
			return NULL;
		}

		// compute centre as average of 4 vertices
		VectorCopy( mesh->xyzArray[mesh->elems[3]], centre );
		for( i = 0; i < 3; i++ )
			VectorAdd( centre, v[i], centre );
		VectorMA( ent->origin, 0.25, centre, centre );

		VectorNegate( &rn.viewAxis[AXIS_FORWARD], plane.normal );
		plane.dist = DotProduct( plane.normal, centre );
		CategorizePlane( &plane );
	}
	else
	{
		vec3_t temp;
		mat3_t entity_rotation;

		// regular surfaces
		if( !Matrix3_Compare( ent->axis, axis_identity ) )
		{
			Matrix3_Transpose( ent->axis, entity_rotation );

			for( i = 0; i < 3; i++ ) {
				VectorCopy( v[i], temp );
				Matrix3_TransformVector( entity_rotation, temp, v[i] ); 
				VectorMA( ent->origin, ent->scale, v[i], v[i] );
			}

			PlaneFromPoints( v, &plane );
			CategorizePlane( &plane );
		}
		else
		{
			plane = untransformed_plane;
		}
	}

	if( ( dist = PlaneDiff( rn.viewOrigin, &plane ) ) <= BACKFACE_EPSILON )
	{
		// behind the portal plane
		if( !( shader->flags & SHADER_PORTAL_CAPTURE2 ) ) {
			return NULL;
		}

		// we need to render the backplane view
	}

	// check if portal view is opaque due to alphagen portal
	if( shader->portalDistance && dist > shader->portalDistance ) {
		return NULL;
	}

	// find the matching portal plane
	for( i = 0; i < rn.numPortalSurfaces; i++ ) {
		portalSurface = &rn.portalSurfaces[i];

		if( portalSurface->entity == ent &&
			portalSurface->shader == shader &&
			DotProduct( portalSurface->plane.normal, plane.normal ) > 0.99f &&
			fabs( portalSurface->plane.dist - plane.dist ) < 0.1f ) {
				goto addsurface;
		}
	}

	if( i == MAX_PORTAL_SURFACES ) {
		// not enough space
		return NULL;
	}

	portalSurface = &rn.portalSurfaces[rn.numPortalSurfaces++];
	portalSurface->entity = ent;
	portalSurface->plane = plane;
	portalSurface->shader = shader;
	portalSurface->untransformed_plane = untransformed_plane;
	ClearBounds( portalSurface->mins, portalSurface->maxs );
	memset( portalSurface->texures, 0, sizeof( portalSurface->texures ) );

addsurface:
	AddPointToBounds( mins, portalSurface->mins, portalSurface->maxs );
	AddPointToBounds( maxs, portalSurface->mins, portalSurface->maxs );
	VectorAdd( portalSurface->mins, portalSurface->maxs, portalSurface->centre );
	VectorScale( portalSurface->centre, 0.5, portalSurface->centre );

	return portalSurface;
}

/*
* R_DrawPortalSurface
* 
* Renders the portal view and captures the results from framebuffer if
* we need to do a $portalmap stage. Note that for $portalmaps we must
* use a different viewport.
*/
static void R_DrawPortalSurface( portalSurface_t *portalSurface )
{
	unsigned int i;
	int x, y, w, h;
	float dist, d, best_d;
	vec3_t viewerOrigin;
	vec3_t origin;
	mat3_t axis;
	entity_t *ent, *best;
	const entity_t *portal_ent = portalSurface->entity;
	cplane_t *portal_plane = &portalSurface->plane, *untransformed_plane = &portalSurface->untransformed_plane;
	const shader_t *shader = portalSurface->shader;
	vec_t *portal_mins = portalSurface->mins, *portal_maxs = portalSurface->maxs;
	vec_t *portal_centre = portalSurface->centre;
	qboolean mirror, refraction = qfalse;
	image_t *captureTexture;
	int captureTextureId = -1;
	int prevRenderFlags = 0;
	qboolean doReflection, doRefraction;
	image_t *portalTexures[2] = { NULL, NULL };

	doReflection = doRefraction = qtrue;
	if( shader->flags & SHADER_PORTAL_CAPTURE )
	{
		shaderpass_t *pass;

		captureTexture = NULL;
		captureTextureId = 0;

		for( i = 0, pass = shader->passes; i < shader->numpasses; i++, pass++ )
		{
			if( pass->program_type == GLSL_PROGRAM_TYPE_DISTORTION )
			{
				if( ( pass->alphagen.type == ALPHA_GEN_CONST && pass->alphagen.args[0] == 1 ) )
					doRefraction = qfalse;
				else if( ( pass->alphagen.type == ALPHA_GEN_CONST && pass->alphagen.args[0] == 0 ) )
					doReflection = qfalse;
				break;
			}
		}
	}
	else
	{
		captureTexture = NULL;
		captureTextureId = -1;
	}

	x = y = 0;
	w = rn.refdef.width;
	h = rn.refdef.height;

	dist = PlaneDiff( rn.viewOrigin, portal_plane );
	if( dist <= BACKFACE_EPSILON || !doReflection )
	{
		if( !( shader->flags & SHADER_PORTAL_CAPTURE2 ) || !doRefraction )
			return;

		// even if we're behind the portal, we still need to capture
		// the second portal image for refraction
		refraction = qtrue;
		captureTexture = NULL;
		captureTextureId = 1;
		if( dist < 0 )
		{
			VectorInverse( portal_plane->normal );
			portal_plane->dist = -portal_plane->dist;
		}
	}

	if( !(rn.renderFlags & RF_NOVIS) && !R_ScissorForEntity( portal_ent, portal_mins, portal_maxs, &x, &y, &w, &h ) )
		return;

	mirror = qtrue; // default to mirror view
	// it is stupid IMO that mirrors require a RT_PORTALSURFACE entity

	best = NULL;
	best_d = 100000000;
	for( i = 1; i < rsc.numEntities; i++ )
	{
		ent = R_NUM2ENT(i);
		if( ent->rtype != RT_PORTALSURFACE )
			continue;

		d = PlaneDiff( ent->origin, untransformed_plane );
		if( ( d >= -64 ) && ( d <= 64 ) )
		{
			d = Distance( ent->origin, portal_centre );
			if( d < best_d )
			{
				best = ent;
				best_d = d;
			}
		}
	}

	if( best == NULL )
	{
		if( captureTextureId < 0 )
			return;
	}
	else
	{
		if( !VectorCompare( best->origin, best->origin2 ) )	// portal
			mirror = qfalse;
		best->rtype = NUM_RTYPES;
	}

	prevRenderFlags = rn.renderFlags;
	if( !R_PushRefInst() ) {
		return;
	}

	VectorCopy( rn.viewOrigin, viewerOrigin );

setup_and_render:

	if( refraction )
	{
		VectorInverse( portal_plane->normal );
		portal_plane->dist = -portal_plane->dist - 1;
		CategorizePlane( portal_plane );
		VectorCopy( rn.viewOrigin, origin );
		Matrix3_Copy( rn.refdef.viewaxis, axis );
		VectorCopy( viewerOrigin, rn.pvsOrigin );

		rn.renderFlags = RF_PORTALVIEW;
		if( !mirror )
			rn.renderFlags |= RF_PVSCULL;
	}
	else if( mirror )
	{
		VectorReflect( rn.viewOrigin, portal_plane->normal, portal_plane->dist, origin );

		VectorReflect( &rn.viewAxis[AXIS_FORWARD], portal_plane->normal, 0, &axis[AXIS_FORWARD] );
		VectorReflect( &rn.viewAxis[AXIS_RIGHT], portal_plane->normal, 0, &axis[AXIS_RIGHT] );
		VectorReflect( &rn.viewAxis[AXIS_UP], portal_plane->normal, 0, &axis[AXIS_UP] );

		Matrix3_Normalize( axis );

		VectorCopy( viewerOrigin, rn.pvsOrigin );

		rn.renderFlags = RF_MIRRORVIEW|RF_FLIPFRONTFACE;
	}
	else
	{
		vec3_t tvec;
		mat3_t A, B, C, rot;

		// build world-to-portal rotation matrix
		VectorNegate( portal_plane->normal, tvec );
		NormalVectorToAxis( tvec, A );

		// build portal_dest-to-world rotation matrix
		ByteToDir( best->frame, tvec );
		NormalVectorToAxis( tvec, B );
		Matrix3_Transpose( B, C );

		// multiply to get world-to-world rotation matrix
		Matrix3_Multiply( C, A, rot );

		// translate view origin
		VectorSubtract( rn.viewOrigin, best->origin, tvec );
		Matrix3_TransformVector( rot, tvec, origin );
		VectorAdd( origin, best->origin2, origin );

		Matrix3_Transpose( A, B );
		Matrix3_Multiply( rn.viewAxis, B, rot );
		Matrix3_Multiply( best->axis, rot, B );
		Matrix3_Transpose( C, A );
		Matrix3_Multiply( B, A, axis );

		// set up portal_plane
		VectorCopy( &axis[AXIS_FORWARD], portal_plane->normal );
		portal_plane->dist = DotProduct( best->origin2, portal_plane->normal );
		CategorizePlane( portal_plane );

		// for portals, vis data is taken from portal origin, not
		// view origin, because the view point moves around and
		// might fly into (or behind) a wall
		rn.renderFlags = RF_PORTALVIEW|RF_PVSCULL;
		VectorCopy( best->origin2, rn.pvsOrigin );
		VectorCopy( best->origin2, rn.lodOrigin );

		// ignore entities, if asked politely
		if( best->renderfx & RF_NOPORTALENTS )
			rn.renderFlags |= RF_NOENTS;
	}

	rn.renderFlags |= (prevRenderFlags & RF_SOFT_PARTICLES);
	rn.refdef.rdflags &= ~( RDF_UNDERWATER|RDF_CROSSINGWATER );

	rn.shadowGroup = NULL;
	rn.meshlist = &r_portallist;

	rn.renderFlags |= RF_CLIPPLANE;
	rn.clipPlane = *portal_plane;

	rn.farClip = R_DefaultFarClip();

	rn.clipFlags |= ( 1<<5 );
	rn.frustum[5] = *portal_plane;
	CategorizePlane( &rn.frustum[5] );

	// if we want to render to a texture, initialize texture
	// but do not try to render to it more than once
	if( captureTextureId >= 0 )
	{
		int texFlags = shader->flags & SHADER_NO_TEX_FILTERING ? IT_NOFILTERING : 0;

		captureTexture = R_GetPortalTexture( rsc.refdef.width, rsc.refdef.height, texFlags,
			rsc.frameCount );
		portalTexures[captureTextureId] = captureTexture;

		if( !captureTexture ) {
			// couldn't register a slot for this plane
			goto done;
		}

		x = y = 0;
		w = captureTexture->upload_width;
		h = captureTexture->upload_height;
		rn.refdef.width = w;
		rn.refdef.height = h;
		rn.refdef.x = 0;
		rn.refdef.y = 0;
		rn.fbColorAttachment = captureTexture;
		// no point in capturing the depth buffer due to oblique frustum messing up
		// the far plane and depth values
		rn.fbDepthAttachment = NULL;
		Vector4Set( rn.viewport, rn.refdef.x + x, rn.refdef.y + y, w, h );
		Vector4Set( rn.scissor, rn.refdef.x + x, rn.refdef.y + y, w, h );
	}
	else {
		// no point in capturing the depth buffer due to oblique frustum messing up
		// the far plane and depth values
		rn.fbDepthAttachment = NULL;
		Vector4Set( rn.scissor, rn.refdef.x + x, rn.refdef.y + y, w, h );
	}

	VectorCopy( origin, rn.refdef.vieworg );
	Matrix3_Copy( axis, rn.refdef.viewaxis );

	R_RenderView( &rn.refdef );

	if( doRefraction && !refraction && ( shader->flags & SHADER_PORTAL_CAPTURE2 ) )
	{
		refraction = qtrue;
		captureTexture = NULL;
		captureTextureId = 1;
		goto setup_and_render;
	}

done:
	portalSurface->texures[0] = portalTexures[0];
	portalSurface->texures[1] = portalTexures[1];

	R_PopRefInst( rn.fbDepthAttachment != NULL ? 0 : GL_DEPTH_BUFFER_BIT );
}

/*
* R_DrawPortals
*/
void R_DrawPortals( void )
{
	unsigned int i;

	if( rf.viewcluster == -1 ) {
		return;
	}

	if( !( rn.renderFlags & ( RF_MIRRORVIEW|RF_PORTALVIEW|RF_SHADOWMAPVIEW ) ) )
	{
		for( i = 0; i < rn.numPortalSurfaces; i++ ) {
			portalSurface_t portalSurface = rn.portalSurfaces[i]; 
			R_DrawPortalSurface( &portalSurface );
			rn.portalSurfaces[i] = portalSurface;
		}
	}
}

/*
* R_DrawSkyPortal
*/
void R_DrawSkyPortal( const entity_t *e, skyportal_t *skyportal, vec3_t mins, vec3_t maxs )
{
	int x, y, w, h;

	if( !R_ScissorForEntity( e, mins, maxs, &x, &y, &w, &h ) ) {
		return;
	}
	if( !R_PushRefInst() ) {
		return;
	}

	rn.renderFlags = ( rn.renderFlags|RF_SKYPORTALVIEW|RF_SOFT_PARTICLES );
	VectorCopy( skyportal->vieworg, rn.pvsOrigin );

	rn.farClip = R_DefaultFarClip();

	rn.clipFlags = 15;
	rn.shadowGroup = NULL;
	rn.meshlist = &r_skyportallist;
	//Vector4Set( rn.scissor, rn.refdef.x + x, rn.refdef.y + y, w, h );

	if( skyportal->noEnts ) {
		rn.renderFlags |= RF_NOENTS;
	}

	if( skyportal->scale )
	{
		vec3_t centre, diff;

		VectorAdd( rsh.worldModel->mins, rsh.worldModel->maxs, centre );
		VectorScale( centre, 0.5f, centre );
		VectorSubtract( centre, rn.viewOrigin, diff );
		VectorMA( skyportal->vieworg, -skyportal->scale, diff, rn.refdef.vieworg );
	}
	else
	{
		VectorCopy( skyportal->vieworg, rn.refdef.vieworg );
	}

	// FIXME
	if( !VectorCompare( skyportal->viewanglesOffset, vec3_origin ) )
	{
		vec3_t angles;
		mat3_t axis;

		Matrix3_Copy( rn.refdef.viewaxis, axis );
		VectorInverse( &axis[AXIS_RIGHT] );
		Matrix3_ToAngles( axis, angles );

		VectorAdd( angles, skyportal->viewanglesOffset, angles );
		AnglesToAxis( angles, axis );
		Matrix3_Copy( axis, rn.refdef.viewaxis );
	}

	rn.refdef.rdflags &= ~( RDF_UNDERWATER|RDF_CROSSINGWATER|RDF_SKYPORTALINVIEW );
	if( skyportal->fov )
	{
		rn.refdef.fov_x = skyportal->fov;
		rn.refdef.fov_y = CalcFov( rn.refdef.fov_x, rn.refdef.width, rn.refdef.height );
		if( glConfig.wideScreen && !( rn.refdef.rdflags & RDF_NOFOVADJUSTMENT ) )
			AdjustFov( &rn.refdef.fov_x, &rn.refdef.fov_y, glConfig.width, glConfig.height, qfalse );
	}

	R_RenderView( &rn.refdef );

	// restore modelview and projection matrices, scissoring, etc for the main view
	R_PopRefInst( ~GL_COLOR_BUFFER_BIT );
}
