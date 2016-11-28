#include "animation.h"
#include "tesselation.h"

// compute the frame from an animation
frame3f animate_compute_frame(FrameAnimation* animation, int time) {
    // grab keyframe interval
    auto interval = 0;
    for(auto t : animation->keytimes) if(time < t) break; else interval++;
    interval--;
    // get translation and rotation matrices
    auto t = float(time-animation->keytimes[interval])/float(animation->keytimes[interval+1]-animation->keytimes[interval]);
    auto m_t = translation_matrix(animation->translation[interval]*(1-t)+animation->translation[interval+1]*t);
    auto m_rz = rotation_matrix(animation->rotation[interval].z*(1-t)+animation->rotation[interval+1].z*t,z3f);
    auto m_ry = rotation_matrix(animation->rotation[interval].y*(1-t)+animation->rotation[interval+1].y*t,y3f);
    auto m_rx = rotation_matrix(animation->rotation[interval].x*(1-t)+animation->rotation[interval+1].x*t,x3f);
    // compute combined xform matrix
    auto m = m_t * m_rz * m_ry * m_rx;
    // return the transformed frame
    return transform_frame(m, animation->rest_frame);
}

// update mesh frames for animation
void animate_frame(Scene* scene) {
    // YOUR CODE GOES HERE ---------------------
    // foreach mesh
	for (auto mesh : scene->meshes) {
		// if not animation, continue
		if (mesh->animation == nullptr) { continue; }
		// update frame
		frame3f updated_frame = animate_compute_frame(mesh->animation, scene->animation->time);
		mesh->frame = updated_frame;
	}
    // foreach surface
	for (auto surface : scene->surfaces) {
		// if not animation, continue
		if (surface->animation == nullptr) { continue; }
		// update frame
		frame3f updated_frame = animate_compute_frame(surface->animation, scene->animation->time);
		surface->frame = updated_frame;
		// update the _display_mesh
		surface->_display_mesh = make_surface_mesh(surface->frame, surface->radius, surface->isquad, surface->mat);
	}
}

// skinning scene
void animate_skin(Scene* scene) {
    // YOUR CODE GOES HERE ---------------------
    // foreach mesh
	for (auto mesh : scene->meshes) {
        // if no skinning, continue
		if (mesh->skinning == nullptr) { continue; }
        // foreach vertex index
		for (int i = 0; i < mesh->pos.size(); i++) {
            // set pos/norm to zero
			mesh->pos[i] = zero3f;
			mesh->norm[i] = zero3f;
            // for each bone slot (0..3)
			for (int j = 0; j < 4; j++) {
				// get bone weight and index
				float w = mesh->skinning->bone_weights.at(i)[j];
				int index = mesh->skinning->bone_ids.at(i)[j];
				// if index < 0, continue
				if (index < 0) { continue; }
				// grab bone xform
				mat4f bone_xform = mesh->skinning->bone_xforms.at(scene->animation->time).at(index);
				// update position
				vec4f rest_pos_vec4 = vec4f(
					mesh->skinning->rest_pos[i].x,
					mesh->skinning->rest_pos[i].y,
					mesh->skinning->rest_pos[i].z,
					1);
				vec4f weighted_deformation_pos = w * bone_xform * rest_pos_vec4;
				mesh->pos[i] += vec3f(weighted_deformation_pos.x, weighted_deformation_pos.y, weighted_deformation_pos.z);
				// update normal
				vec4f rest_norm_vec4f = vec4f(
					mesh->skinning->rest_norm[i].x,
					mesh->skinning->rest_norm[i].y,
					mesh->skinning->rest_norm[i].z,
					1);
				vec4f weighted_deformation_norm = w * bone_xform * rest_norm_vec4f;
				mesh->norm[i] += vec3f(weighted_deformation_norm.x,weighted_deformation_norm.y,weighted_deformation_norm.z);
			}
            // normalize normal
			mesh->norm[i] = normalize(mesh->norm[i]);
		}
	}
}

// particle simulation
void simulate(Scene* scene) {
    // YOUR CODE GOES HERE ---------------------
    // for each mesh
        // skip if no simulation
        // compute time per step
        // foreach simulation steps
            // compute extenal forces (gravity)
            // for each spring, compute spring force on points
                // compute spring distance and length
                // compute static force
                // accumulate static force on points
                // compute dynamic force
                // accumulate dynamic force on points
            // newton laws
                // if pinned, skip
                // acceleration
                // update velocity and positions using Euler's method
                // for each mesh, check for collision
                    // compute inside tests
                    // if quad
                        // compute local poisition
                        // perform inside test
                            // if inside, set position and normal
                        // else sphere
                        // inside test
                            // if inside, set position and normal
                    // if inside
                        // set particle position
                        // update velocity
        // smooth normals if it has triangles or quads
}

// scene reset
void animate_reset(Scene* scene) {
    scene->animation->time = 0;
    for(auto mesh : scene->meshes) {
        if(mesh->animation) {
            mesh->frame = mesh->animation->rest_frame;
        }
        if(mesh->skinning) {
            mesh->pos = mesh->skinning->rest_pos;
            mesh->norm = mesh->skinning->rest_norm;
        }
        if(mesh->simulation) {
            mesh->pos = mesh->simulation->init_pos;
            mesh->simulation->vel = mesh->simulation->init_vel;
            mesh->simulation->force.resize(mesh->simulation->init_pos.size());
        }
    }
}

// scene update
void animate_update(Scene* scene) {
    if(scene->animation->time >= scene->animation->length-1) {
        if(scene->animation->loop) animate_reset(scene);
        else return;
    } else scene->animation->time ++;
    animate_frame(scene);
    if(not scene->animation->gpu_skinning) animate_skin(scene);
    simulate(scene);
}
