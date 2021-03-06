#pragma once

#include "intersections.h"

// CHECKITOUT
/**
 * Computes a cosine-weighted random direction in a hemisphere.
 * Used for diffuse lighting.
 */
__host__ __device__
glm::vec3 calculateRandomDirectionInHemisphere(
        glm::vec3 normal, thrust::default_random_engine &rng) {
    thrust::uniform_real_distribution<float> u01(0, 1);

    float up = sqrt(u01(rng)); // cos(theta)
    float over = sqrt(1 - up * up); // sin(theta)
    float around = u01(rng) * TWO_PI;

    // Find a direction that is not the normal based off of whether or not the
    // normal's components are all equal to sqrt(1/3) or whether or not at
    // least one component is less than sqrt(1/3). Learned this trick from
    // Peter Kutz.

    glm::vec3 directionNotNormal;
    if (abs(normal.x) < SQRT_OF_ONE_THIRD) {
        directionNotNormal = glm::vec3(1, 0, 0);
    } else if (abs(normal.y) < SQRT_OF_ONE_THIRD) {
        directionNotNormal = glm::vec3(0, 1, 0);
    } else {
        directionNotNormal = glm::vec3(0, 0, 1);
    }

    // Use not-normal direction to generate two perpendicular directions
    glm::vec3 perpendicularDirection1 =
        glm::normalize(glm::cross(normal, directionNotNormal));
    glm::vec3 perpendicularDirection2 =
        glm::normalize(glm::cross(normal, perpendicularDirection1));

    return up * normal
        + cos(around) * over * perpendicularDirection1
        + sin(around) * over * perpendicularDirection2;
}

/**
* Computes a cosine-weighted random direction in a hemisphere.
* Used for non-perfect specular lighting.
*/
__host__ __device__
glm::vec3 calculateRandomSpecDirection(
glm::vec3 normal, float n, thrust::default_random_engine &rng) {
	thrust::uniform_real_distribution<float> u01(0, 1);

	// Weighted cosine by specular exponent
	float up = pow(u01(rng), 1/(n+1)); // cos(theta)
	float over = sqrt(1 - up * up); // sin(theta)
	float around = u01(rng) * TWO_PI;	// phi

	// Find a direction that is not the normal based off of whether or not the
	// normal's components are all equal to sqrt(1/3) or whether or not at
	// least one component is less than sqrt(1/3). Learned this trick from
	// Peter Kutz.

	glm::vec3 directionNotNormal;
	if (abs(normal.x) < SQRT_OF_ONE_THIRD) {
		directionNotNormal = glm::vec3(1, 0, 0);
	}
	else if (abs(normal.y) < SQRT_OF_ONE_THIRD) {
		directionNotNormal = glm::vec3(0, 1, 0);
	}
	else {
		directionNotNormal = glm::vec3(0, 0, 1);
	}

	// Use not-normal direction to generate two perpendicular directions
	glm::vec3 perpendicularDirection1 =
		glm::normalize(glm::cross(normal, directionNotNormal));
	glm::vec3 perpendicularDirection2 =
		glm::normalize(glm::cross(normal, perpendicularDirection1));

	return up * normal
		+ cos(around) * over * perpendicularDirection1
		+ sin(around) * over * perpendicularDirection2;
}

/**
* Computes a cosine-weighted random direction and a guessed new origin in a hemisphere for SSS
* Used for SSS.
* Really a simplified version of Dipole approximation
*/
__host__ __device__
void calculateSSSOut(Ray &r, glm::vec3 &intersect, glm::vec3 &inDirection, glm::vec3 &normal, thrust::default_random_engine &rng) {
	// Punch in a "pinhole" along negative normal direction
	// Use endpoint as origin, and shoot a random ray toward surface from inside
	glm::vec3 origin = intersect + normalize(inDirection)*0.0001f - normal;
	glm::vec3 scatterDirection = calculateRandomDirectionInHemisphere(normal, rng);
	// Shoot the ray from inside
	// Approximate scattered origin with normal length
	glm::vec3 scatterOrigin = origin + normalize(scatterDirection) * glm::length(normal);
	// Does not necessarily put the ray outside geometry; if not it will be catched at the next intersection test
	// Passing geometry all the way to this function is REALLY slow; therefore the hacky way
	r.origin = scatterOrigin + normalize(scatterDirection)*0.0005f;
	r.direction = scatterDirection;
}

/**
* Computes refraction direction
* Used for refraction lighting.
*/
__host__ __device__
glm::vec3 calculateRefractDirection(bool outside, glm::vec3 inDirection, glm::vec3 normal, float n1, float n2) {
	float mu;
	glm::vec3 n = normalize(normal);
	glm::vec3 i = normalize(inDirection);
	if (outside){
		// Enter medium, leave air
		mu = n1/n2;
	}
	else {
		// Enter air, leave medium
		mu = n2/n1;
	}
	float cosI = -dot(n, i);
	float cosTsq = 1 - mu*mu*(1 - cosI*cosI);
	if (cosTsq < 0){
		return glm::vec3(0.0f);
	}
	else {
		return (mu * i) + (mu * cosI - sqrt(cosTsq)) * n;
	}
}

/**
* Computes the perfect reflection direction on a given normal
* Used for perfect specular lighting. (not used for non-perfect specular lighting)
*/
__host__ __device__
glm::vec3 calculatePerfectSpecDirection(glm::vec3 inDirection, glm::vec3 normal) {
	return inDirection - 2.0f * glm::dot(inDirection, normal) * normal;
}

/**
 * Scatter a ray with some probabilities according to the material properties.
 * For example, a diffuse surface scatters in a cosine-weighted hemisphere.
 * A perfect specular surface scatters in the reflected ray direction.
 * In order to apply multiple effects to one surface, probabilistically choose
 * between them.
 * 
 * The visual effect you want is to straight-up add the diffuse and specular
 * components. You can do this in a few ways:
 * - Always take a 50/50 split between a diffuse bounce and a specular bounce,
 *   but multiply the result of either one by 1/0.5 to cancel the 0.5 chance
 *   of it happening.
 * - Pick the split based on the intensity of each color, and multiply each
 *   branch result by the inverse of that branch's probability (same as above).
 *
 * This method applies its changes to the Ray parameter `ray` in place.
 * It also modifies the color `color` of the ray in place.
 *
 * You may need to change the parameter list for your purposes!
 */

__host__ __device__
void scatterRay(
		PathRay &path,
        const Material &m,
        thrust::default_random_engine &rng) {

	thrust::uniform_real_distribution<float> range(0, 1);
	// Diffuse is for sure to happen
	glm::vec3 diffuseDirection = calculateRandomDirectionInHemisphere(path.normal, rng);
	glm::vec3 diffuseColor = path.color * m.color;

	if (m.hasRefractive == 1.0f){
		float n1 = 1;
		float n2 = m.indexOfRefraction;
		float r0 = pow((n1-n2)/(n1+n2), 2);

		//glm::vec3 perfectSpecDirection = calculatePerfectSpecDirection(inDirection, normal);

		// Jitter intersection normal to get jittered specular reflections
		glm::vec3 glossNormal = calculateRandomSpecDirection(path.normal, m.specular.exponent, rng);
		glm::vec3 glossDirection = calculatePerfectSpecDirection(path.ray.direction, glossNormal);
		glm::vec3 glossColor = path.color * m.specular.color;

		glm::vec3 refractDirection = calculateRefractDirection(path.outside, path.ray.direction, path.normal, n1, n2);

		// Schlick coefficient
		float r = r0 + (1 - r0)*pow((1 - dot(normalize(glossDirection), normalize(path.normal))), 5);

		if (refractDirection == glm::vec3(0.0f)){
			// Total internal reflection
			path.color = glossColor * r;
			path.ray.direction = glossDirection;
			path.ray.origin = path.intersect;
		}
		else {
			glossColor = glossColor * r;
			float dI = glm::length(diffuseColor);
			float split = dI / (dI + glm::length(glossColor));
			float pick = range(rng);
			if (pick < split){
				path.color = diffuseColor / split;
				// Move on the original incident direction
				path.ray.origin = path.intersect + normalize(path.ray.direction)*0.0005f;
				// Update to outbound direction
				path.ray.direction = refractDirection;
			}
			else {
				path.color = glossColor / split;
				path.ray.direction = glossDirection;
				path.ray.origin = path.intersect;
			}
		}
	}
	else if (m.hasReflective == 1.0f){
		//glm::vec3 perfectSpecDirection = calculatePerfectSpecDirection(inDirection, normal);
		glm::vec3 glossNormal = calculateRandomSpecDirection(path.normal, m.specular.exponent, rng);
		glm::vec3 glossDirection = calculatePerfectSpecDirection(path.ray.direction, glossNormal);
		glm::vec3 glossColor = path.color * m.specular.color;

		float dI = glm::length(diffuseColor);
		float split = dI / (dI + glm::length(glossColor));
		float pick = range(rng);

		if (pick < split){
			/*
			color = diffuseColor / split;
			ray.direction = diffuseDirection;
			*/
			if (m.hasSSS == 1.0f){
				if (path.outside){
					calculateSSSOut(path.ray, path.intersect, path.ray.direction, path.normal, rng);
				}
				else {
					float pick = range(rng);
					if (pick < 0.5){
						path.ray.origin = path.intersect;
						path.ray.direction = diffuseDirection;
					}
					else {
						path.ray.origin = path.intersect + normalize(path.ray.direction)*0.0005f;
						path.ray.direction = path.ray.direction;
					}
					path.color = diffuseColor / split / 2.0f;
				}
				path.color = diffuseColor / split;
			}
			else {
				path.color = diffuseColor / split;
				path.ray.origin = path.intersect;
				path.ray.direction = diffuseDirection;
			}
		}
		else {
			path.color = glossColor / split;
			path.ray.origin = path.intersect;
			path.ray.direction = glossDirection;
		}
	}
	else if (m.hasSSS == 1.0f){
		if (path.outside){
			// Only scatter under surface if it's an incoming light
			calculateSSSOut(path.ray, path.intersect, path.ray.direction, path.normal, rng);
		}
		else {
			// Otherwise light is going out; should be the offset diffuse reflection
			float pick = range(rng);
			if (pick < 0.5){
				path.ray.origin = path.intersect;
				path.ray.direction = diffuseDirection;
			}
			else {
				path.ray.origin = path.intersect + normalize(path.ray.direction)*0.0005f;
				path.ray.direction = path.ray.direction;
			}
		}
		path.color = diffuseColor * 2.0f;
	}
	else {
		path.color = diffuseColor;
		path.ray.origin = path.intersect;
		path.ray.direction = diffuseDirection;
	}
}
