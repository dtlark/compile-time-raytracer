#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <cassert>
#include <string>
#include <memory>
#include <array>
#include <optional>

constexpr float INF = 1e6;
constexpr int MAX_RAY_DEPTH = 10;
constexpr int WIDTH = 200;
constexpr int HEIGHT = 200;
constexpr int resolution = WIDTH * HEIGHT;

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

    constexpr vec3() : x(0.f), y(0.f), z(0.f) {}
    constexpr vec3(float n) : x(n), y(n), z(n) {}
    constexpr vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    constexpr vec3 operator +(const vec3 &v) const { return vec3(x + v.x, y + v.y, z + v.z); }
    constexpr vec3& operator +=(const vec3 &v) { x += v.x, y += v.y, z += v.z; return *this; }
    constexpr vec3 operator -() const { return vec3(-x, -y, -z); }
    constexpr vec3 operator -(const vec3 &v) const { return vec3(x - v.x, y - v.y, z - v.z); }
    constexpr vec3 operator *(const float &f) const { return vec3(x * f, y * f, z * f); }
    constexpr vec3 operator *(const vec3 &v) const { return vec3(x * v.x, y * v.y, z * v.z); }
    constexpr vec3& operator *=(const vec3 &v) { x *= v.x, y *= v.y, z *= v.z; return *this; }

    constexpr float dot(const vec3 &v) const { return x * v.x + y * v.y + z * v.z; }
    constexpr vec3 cross(vec3 &v) { return vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }
    constexpr float magnitude() const { return sqrt((x * x) + (y * y) + (z * z)); }
    constexpr vec3& normalize() { double mag = magnitude(); x /= mag, y /= mag, z /= mag; return *this; }
};

struct Ray {
    vec3 orig;
    vec3 dir;

    constexpr Ray() : orig(vec3(0, 0, 0)), dir(vec3(1, 0, 0)) {}
    constexpr Ray(vec3 origin, vec3 direction) : orig(origin), dir(direction) {}
};

struct Light {
    vec3 position;
    vec3 color;
    float intensity;

    constexpr Light() : position(vec3(0, 0, 0)), color(vec3(1, 1, 1)), intensity(1.0) {}
    constexpr Light(vec3 pos, vec3 col, float i) : position(pos), color(col), intensity(i) {}
};

struct Intersection {
	pair<float, float> t;
	vec3 point;
	vec3 normal;
	//Material* material = nullptr;

    constexpr Intersection () {}

    constexpr Intersection (pair<float, float> p) {
        t = p;
    }
};


struct Sphere {
    vec3 center;
    float radius;
	vec3 color;
	Material material;

    constexpr Sphere() : center(vec3(0, 0, 0)), radius(0.0), color(vec3(0.5, 0.5, 0.5)), material(Diffuse) {}

    constexpr Sphere(const vec3 &c, const float &r, const vec3 &col, const Material &mat) : 
              center(c), radius(r), color(col), material(mat) {}

    constexpr vec3 normalAt(const vec3 point) const {
        return (point + (-center)).normalize(); //check
    }

    constexpr optional<Intersection> intersect(const Ray &ray) const {
        vec3 L = center - ray.orig;
        float tca = L.dot(ray.dir);

        if (tca < 0) return {};
        
        float d2 = L.dot(L) - tca * tca;
        
        if (d2 > radius * radius) return {};
        
        float thc = sqrt(radius * radius - d2);

        float t0 = tca - thc;
        float t1 = tca + thc;

		return Intersection(make_pair(t0, t1));
    }
};

constexpr vec3 trace(const Ray &ray, const array<Sphere, 4> &spheres, const array<Light, 1> &lights, const vec3 &background, const int depth) {
    
    float tnear = INF;
    const Sphere* sphere = nullptr;

    for (unsigned i = 0; i < spheres.size(); ++i) {
        if (auto inter = spheres[i].intersect(ray); inter) {
            
            if (inter->t.first < 0) inter->t.first = inter->t.second;
            if (inter->t.first < tnear) {
                tnear = inter->t.first;
                sphere = &spheres[i];
            }
        }
    }

    //if sphere is null, return bg
    if (!sphere) {
        return background; //black backgorund
    }

    vec3 finalColor = 0;
    vec3 pointHit = ray.orig + ray.dir * tnear; // point of intersection 
    vec3 normalHit = pointHit - sphere->center; // normal at intersection point
    normalHit.normalize(); // normalize normal direction


    bool inside = false;
    if (ray.dir.dot(normalHit) > 0) {
        normalHit = -normalHit;
        inside = true;
    }

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
					if (auto shadow_inter = spheres[j].intersect(shadowRay); shadow_inter) {
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

void save(string &&fileName, const std::array<std::array<vec3, WIDTH>, HEIGHT> image) {

    ofstream outfile(fileName, ios::out | ios::binary);
    
    outfile << "P3\n" << WIDTH << " " << HEIGHT << "\n" << 255 << "\n";

    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j < HEIGHT; j++) {
            vec3 color = image[i][j];

            int red = min(color.x * 255.0, 255.0);
            int green = min(color.y * 255.0, 255.0);
            int blue = min(color.z * 255.0, 255.0);

            outfile << red << " " << green << " " << blue << " ";
        }
    }

    outfile <<  "\n"; //footer
    outfile.close();
}

template <typename Scene, typename Canvas>
constexpr void render(const Scene& scene, Canvas& canvas, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const auto point = get_point(width, height, x, y, scene.get_camera());
            const auto color = trace_ray({ scene.get_camera().pos, point }, scene, 0);
            canvas.set_pixel(x, y, color);
        }
    }
}

int main() {

                                                //center, radius, color, material
    static constexpr array<Sphere, 4> spheres = {Sphere(vec3(0.0, -10004, -20), 10000, vec3(0.20, 0.20, 0.25), Diffuse), 
                                                 Sphere(vec3(2.0, -2.5, -25), 1.5, vec3(1.0, 0.75, 0.45), Diffuse),
                                                 Sphere(vec3(-5, -1, -35), 3, vec3(0.75, 0.45, 0.45), Diffuse),
                                                 Sphere(vec3(5.0, 1, -45), 5, vec3(0.45, 0.45, 0.75), Diffuse)
    };
                                                //pos, color, intensity
    static constexpr array<Light, 1> lights = {Light(vec3(-10.0, 20, -10), vec3(1, 1, 1), 1.0)};

    static constexpr vec3 background = { 0, 0, 0 }; //black background

    static constexpr float invWidth = 1.f / float(WIDTH), invHeight = 1.f / float(HEIGHT);
    static constexpr float fov = 30;
    static constexpr float aspectratio = float(WIDTH) / float(HEIGHT);
    static constexpr float angle = tan(M_PI * 0.5 * fov / 180.);

    static constexpr std::array<std::array<vec3, WIDTH>, HEIGHT> image = []{
        array<std::array<vec3, WIDTH>, HEIGHT> canvas{};

        for (unsigned y = 0; y < HEIGHT; ++y) {
            for (unsigned x = 0; x < WIDTH; ++x) {
                float xx = (2 * ((x + 0.5) * invWidth) - 1) * angle * aspectratio;
                float yy = (1 - 2 * ((y + 0.5) * invHeight)) * angle;

                Ray ray = Ray(vec3(0), vec3(xx, yy, -1).normalize());
                canvas[y][x] = trace(ray, spheres, lights, background, 0);
            }
        }
        return canvas;
    }();
    
    save("Picture.ppm", image);

    return 0;
}
