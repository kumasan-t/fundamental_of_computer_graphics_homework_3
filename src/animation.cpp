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
				mesh->pos[i] += w * transform_point(bone_xform, mesh->skinning->rest_pos[i]);
				// update normal
				mesh->norm[i] += w * transform_normal(bone_xform, mesh->skinning->rest_norm[i]);
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
	for (auto mesh : scene->meshes) {
        // skip if no simulation
		if (mesh->simulation == nullptr) { continue; }
        // compute time per step
		auto time_per_step = scene->animation->dt / scene->animation->simsteps;
        // foreach simulation steps
		for (int i = 0; i < scene->animation->simsteps; i++) {
            // compute extenal forces (gravity)
			for (int j = 0; j < mesh->simulation->force.size(); j++) {
				mesh->simulation->force[j] = scene->animation->gravity * mesh->simulation->mass[j];
			}
            // for each spring, compute spring force on points
			for (auto spring : mesh->simulation->springs) {
				// compute spring distance and length
				auto spring_length = length(mesh->pos[spring.ids[1]] - mesh->pos[spring.ids[0]]);
				auto spring_direction = normalize(mesh->pos[spring.ids[1]] - mesh->pos[spring.ids[0]]);
				auto spring_relative_velocity = mesh->simulation->vel[spring.ids[1]] - mesh->simulation->vel[spring.ids[0]];
				// compute static force
				auto static_force = spring.ks * (spring_length - spring.restlength)*spring_direction;
				// compute dynamic force
				auto dynamic_force = spring.kd * dot(spring_relative_velocity,spring_direction) * spring_direction;
				// accumulate static force on points
				// accumulate dynamic force on points
				mesh->simulation->force[spring.ids[0]] += (static_force + dynamic_force);
				mesh->simulation->force[spring.ids[1]] += -1 * ( static_force + dynamic_force);
			}
            // newton laws
			for (int j = 0; j < mesh->pos.size(); j++) {
                // if pinned, skip
				if (mesh->simulation->pinned[j]) { continue; }
                // acceleration
				auto particle_acceleration = mesh->simulation->force[j] / mesh->simulation->mass[j];
                // update velocity and positions using Euler's method
				mesh->simulation->vel[j] += particle_acceleration * time_per_step;
				mesh->pos[j] += mesh->simulation->vel[j] * time_per_step + pow(time_per_step,2) * particle_acceleration / 2;
                // for each mesh, check for collision
				bool inside = false;
				vec3f surface_norm;
				for (auto collision_surface : scene->surfaces) {
                    // compute inside tests
                    // if quad
					if (collision_surface->isquad) {
                        // compute local poisition
						auto local_pos = transform_point_inverse(collision_surface->frame, mesh->pos[j]);
                        // perform inside test
						if (local_pos.z < 0 &&
							local_pos.x > -1 * collision_surface->radius &&
							local_pos.x < collision_surface->radius &&
							local_pos.y > -1 * collision_surface->radius &&
							local_pos.y < collision_surface->radius) {
                            // if inside, set position and normal
							mesh->pos[j] = transform_point(collision_surface->frame,vec3f(local_pos.x,local_pos.y,0));
							surface_norm = collision_surface->frame.z;
							inside = true;
						}
					} else {
                        // else sphere
						auto center_point_distance = length(mesh->pos[j] - collision_surface->frame.o);
                        // inside test
						if (center_point_distance < collision_surface->radius) {
                            // if inside, set position and normal
							mesh->pos[j] = collision_surface->radius * normalize(mesh->pos[j] - collision_surface->frame.o) + collision_surface->frame.o;
							surface_norm = normalize(mesh->pos[j] - collision_surface->frame.o);
							inside = true;
						}
					}
                    // if inside
					if (inside) {
						// update velocity
						mesh->simulation->vel[j] = (mesh->simulation->vel[j] - dot(surface_norm, mesh->simulation->vel[j])*surface_norm) *
							(1 - scene->animation->bounce_dump[0]) +
							(-1 * dot(surface_norm, mesh->simulation->vel[j]) *
							surface_norm)*(1 - scene->animation->bounce_dump[1]);
						inside = false;
					}
				}
			}
			if (mesh->quad.size() != 0 || mesh->triangle.size() != 0)
				smooth_normals(mesh);
		}
	}
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
