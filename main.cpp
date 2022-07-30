#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <cassert>
#include <string>
#include <memory>
#include <array>

static constexpr float INF = 1e6;
static constexpr int MAX_RAY_DEPTH = 10;
static constexpr int WIDTH = 200;
static constexpr int HEIGHT = 200;
static constexpr int resolution = WIDTH * HEIGHT;

using namespace std;

enum Material {
	Diffuse,
	Specular,
	Fresnel,
	Reflect,
	ReflectAndRefract
};

struct vec3 {
    float x, y, z;

    vec3() : x(0.f), y(0.f), z(0.f) {}
    vec3(float n) : x(n), y(n), z(n) {}
    vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    vec3 operator +(const vec3 &v) const { return vec3(x + v.x, y + v.y, z + v.z); }
    vec3& operator +=(const vec3 &v) { x += v.x, y += v.y, z += v.z; return *this; }
    vec3 operator -() const { return vec3(-x, -y, -z); }
    vec3 operator -(const vec3 &v) const { return vec3(x - v.x, y - v.y, z - v.z); }
    vec3 operator *(const float &f) const { return vec3(x * f, y * f, z * f); }
    vec3 operator *(const vec3 &v) const { return vec3(x * v.x, y * v.y, z * v.z); }
    vec3& operator *=(const vec3 &v) { x *= v.x, y *= v.y, z *= v.z; return *this; }

    float dot(const vec3 &v) const { return x * v.x + y * v.y + z * v.z; }
    vec3 cross(vec3 &v) { return vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }
    float magnitude() const { return sqrt((x * x) + (y * y) + (z * z)); }
    vec3& normalize() { double mag = magnitude(); x /= mag, y /= mag, z /= mag; return *this; }
};

struct Ray {
    vec3 orig;
    vec3 dir;

    Ray() : orig(vec3(0, 0, 0)), dir(vec3(1, 0, 0)) {}
    Ray(vec3 origin, vec3 direction) : orig(origin), dir(direction) {}
};

struct Light {
    vec3 position;
    vec3 color;
    float intensity;

    Light() : position(vec3(0, 0, 0)), color(vec3(1, 1, 1)), intensity(1.0) {}
    Light(vec3 pos, vec3 col, float i) : position(pos), color(col), intensity(i) {}
};

struct Intersection {
	pair<float, float> t = make_pair(INF, INF);
	vec3 point;
	vec3 normal;
	Material* material = nullptr;
};

struct Shape {
    virtual bool intersect(const Ray &ray, Intersection &inter) = 0;
};


struct Sphere : Shape {
    vec3 center;
    float radius;
	vec3 color;
	Material material;


    Sphere() : center(vec3(0, 0, 0)), radius(0.0), color(vec3(0.5, 0.5, 0.5)), material(Diffuse) {}

    Sphere(const vec3 &c, const float &r, const vec3 &col, Material mat) : 
           center(c), radius(r), color(col), material(mat) {}

    vec3 normalAt(vec3 point) {
        return (point + (-center)).normalize(); //check
    }

    bool intersect(const Ray &ray, Intersection &inter) {
        vec3 L = center - ray.orig;
        float tca = L.dot(ray.dir);
        if (tca < 0) return false;
        float d2 = L.dot(L) - tca * tca;
        if (d2 > radius * radius) return false;
        float thc = sqrt(radius * radius - d2);
        inter.t.first = tca - thc;
        inter.t.second = tca + thc;

		return true;
    }
};

struct Plane : Shape {
    vec3 normal;
    double dist;
    vec3 color;
	Material material;

    Plane() : normal(vec3(1, 0, 0)), dist(0.0), color(vec3(0.5, 0.5, 0.5)), material(Diffuse) {}
    Plane(vec3 norm, double d, vec3 col, Material mat) : normal(norm), dist(d), color(col), material(mat) {}

	bool intersect(const Ray &ray, Intersection &inter) {
        vec3 raydir = ray.dir;

        double a = raydir.dot(normal);

        if (a == 0) { return false; } //ray parallel to plane
        else {
            double b = normal.dot(-(ray.orig + ((normal * dist))));
            return true;
        }
    }
};

vec3 trace(const Ray &ray, array<Sphere, 4> &spheres, const array<Light, 1> &lights, vec3 &background, const int depth) {
    
    float tnear = INF;
    const Sphere* sphere = nullptr;
    Intersection inter;

    for (unsigned i = 0; i < spheres.size(); ++i) {
        
        if (spheres[i].intersect(ray, inter)) {
            if (inter.t.first < 0) inter.t.first = inter.t.second;
            if (inter.t.first < tnear) {
                tnear = inter.t.first;
                sphere = &spheres[i];
            }
        }
    }

    //if sphere is null, return bg
    if (!sphere) return background; //black backgorund

    vec3 finalColor = 0;
    vec3 pointHit = ray.orig + ray.dir * tnear; // point of intersection 
    vec3 normalHit = pointHit - sphere->center; // normal at intersection point
    normalHit.normalize(); // normalize normal direction


    bool inside = false;
    if (ray.dir.dot(normalHit) > 0) normalHit = -normalHit, inside = true;



	switch (sphere->material) {

		case Diffuse: // DIFFUSE LIGHTING
		{
			for (int i = 0; i < lights.size(); ++i) {
				// this is a light
				vec3 transmission = 1;
				vec3 lightDirection = lights[i].position - pointHit;
				lightDirection.normalize();
				for (unsigned j = 0; j < spheres.size(); ++j) {

					Ray shadowRay = Ray(pointHit + normalHit, lightDirection);
					if (spheres[j].intersect(shadowRay, inter)) {
						transmission = 0; //shadowed
						break;
					}
				}
				finalColor += sphere->color * transmission * max(float(0), normalHit.dot(lightDirection)) * lights[i].color;
			}
			break;
		}
		default:
			break;
	}

    return finalColor;
}

void save(string &&fileName, const vec3 *image) {

    ofstream outfile(fileName, ios::out | ios::binary);
    
    outfile << "P3\n" << WIDTH << " " << HEIGHT << "\n" << 255 << "\n";

    for (int i = 0; i < resolution; i++) {

        vec3 color = image[i];

        int red = min(color.x * 255.0, 255.0);
        int green = min(color.y * 255.0, 255.0);
        int blue = min(color.z * 255.0, 255.0);

        outfile << red << " " << green << " " << blue << " ";
    }

    outfile <<  "\n"; //footer
    outfile.close();
}

void makeScene(array<Sphere, 4> &spheres, array<Light, 1> &lights) {

    //center, radius, color, material
    spheres[0] = Sphere(vec3(0.0, -10004, -20), 10000, vec3(0.20, 0.20, 0.25), Diffuse); //floor
    spheres[1] = Sphere(vec3(2.0, -2.5, -25), 1.5, vec3(1.0, 0.75, 0.45), Diffuse);
    spheres[2] = Sphere(vec3(-5, -1, -35), 3, vec3(0.75, 0.45, 0.45), Diffuse);
    spheres[3] = Sphere(vec3(5.0, 1, -45), 5, vec3(0.45, 0.45, 0.75), Diffuse);
    
	//Plane pl = Plane(vec3(0, 1, 0), 30.0, vec3(1, 1, 1), Diffuse);

    //pos, color, intensity
    lights[0] = Light(vec3(-10.0, 20, -10), vec3(1, 1, 1), 1.0);
}

int main() {

    array<Sphere, 4> spheres;
    array<Light, 1> lights;
    makeScene(spheres, lights);

    vec3 background = { 0, 0, 0 }; //black background

    vec3 *image = new vec3[resolution]; //array of vec3
    vec3 *pixels = image;
    static constexpr float invWidth = 1.f / float(WIDTH), invHeight = 1.f / float(HEIGHT);
    static constexpr float fov = 30;
    static constexpr float aspectratio = float(WIDTH) / float(HEIGHT);
    static constexpr float angle = tan(M_PI * 0.5 * fov / 180.);

    for (unsigned y = 0; y < HEIGHT; ++y) {
         for (unsigned x = 0; x < WIDTH; ++x, ++pixels) {
            float xx = (2 * ((x + 0.5) * invWidth) - 1) * angle * aspectratio;
            float yy = (1 - 2 * ((y + 0.5) * invHeight)) * angle;

            Ray ray = Ray(vec3(0), vec3(xx, yy, -1).normalize());
            *pixels = trace(ray, spheres, lights, background, 0);
         }
    }
    
    save("Picture.ppm", image);
    delete[] image;

    return 0;
}