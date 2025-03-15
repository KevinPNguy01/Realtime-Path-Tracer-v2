#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <random>
#include <vector>
#include <omp.h>
#define PI 3.1415926535897932384626433832795

const double epsilon = pow(10, -12);

/*
 * Thread-safe random number generator
 */

struct RNG {
    RNG() : distrb(0.0, 1.0), engines() {}

    void init(int nworkers) {
        std::random_device rd;
        engines.resize(nworkers);
        for (int i = 0; i < nworkers; ++i)
            engines[i].seed(rd());
    }

    double operator()() {
        int id = omp_get_thread_num();
        return distrb(engines[id]);
    }

    std::uniform_real_distribution<double> distrb;
    std::vector<std::mt19937> engines;
} rng;


/*
 * Basic data types
 */

struct Vec {
    double x, y, z;

    Vec(double x_ = 0, double y_ = 0, double z_ = 0) { x = x_; y = y_; z = z_; }

    Vec operator+ (const Vec& b) const { return Vec(x + b.x, y + b.y, z + b.z); }
    Vec operator- (const Vec& b) const { return Vec(x - b.x, y - b.y, z - b.z); }
    Vec operator* (double b) const { return Vec(x * b, y * b, z * b); }
    bool operator== (const Vec& b) const { return abs(x - b.x) < epsilon && abs(y - b.y) < epsilon && abs(z - b.z) < epsilon; }

    Vec mult(const Vec& b) const { return Vec(x * b.x, y * b.y, z * b.z); }
    Vec& normalize() { return *this = *this * (1.0 / std::sqrt(x * x + y * y + z * z)); }
    double dot(const Vec& b) const { return x * b.x + y * b.y + z * b.z; }
    Vec cross(const Vec& b) const { return Vec(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x); }
    double length() const { return sqrt(x * x + y * y + z * z); }
};

struct Ray {
    Vec o, d;
    Ray(Vec o_, Vec d_) : o(o_), d(d_) {}
};

struct BRDF {
    virtual Vec eval(const Vec& n, const Vec& o, const Vec& i) const = 0;
    virtual void sample(const Vec& n, const Vec& o, Vec& i, double& pdf) const = 0;
    virtual bool isSpecular() const = 0;
};


/*
 * Utility functions
 */

inline double clamp(double x) {
    return x < 0 ? 0 : x > 1 ? 1 : x;
}

inline int toInt(double x) {
    return static_cast<int>(std::pow(clamp(x), 1.0 / 2.2) * 255 + .5);
}


/*
 * Shapes
 */

struct Sphere {
    Vec p, e;           // position, emitted radiance
    double rad;         // radius
    const BRDF& brdf;   // BRDF

    Sphere(double rad_, Vec p_, Vec e_, const BRDF& brdf_) :
        rad(rad_), p(p_), e(e_), brdf(brdf_) {}

    double intersect(const Ray& r) const { // returns distance, 0 if nohit
        Vec op = p - r.o; // Solve t^2*d.d + 2*t*(o-p).d + (o-p).(o-p)-R^2 = 0
        double t, eps = 1e-4, b = op.dot(r.d), det = b * b - op.dot(op) + rad * rad;
        if (det < 0) return 0; else det = sqrt(det);
        return (t = b - det) > eps ? t : ((t = b + det) > eps ? t : 0);
    }

    /**
     * Sample a random point on the sphere
     */
    void sample(Vec& point, double& pdf) const {
        double xi1 = rng();
        double xi2 = rng();
        double z = 2 * xi1 - 1;
        double x = sqrt(1 - z * z) * cos(2 * PI * xi2);
        double y = sqrt(1 - z * z) * sin(2 * PI * xi2);
        point = p + Vec(x, y, z) * rad;
        pdf = 1.0 / (4 * PI * rad * rad);
    }
};


/*
 * Sampling functions
 */

inline void createLocalCoord(const Vec& n, Vec& u, Vec& v, Vec& w) {
    w = n;
    u = ((std::abs(w.x) > .1 ? Vec(0, 1) : Vec(1)).cross(w)).normalize();
    v = w.cross(u);
}


/*
 * BRDFs
 */

 // Ideal diffuse BRDF
struct DiffuseBRDF : public BRDF {
    DiffuseBRDF(Vec kd_) : kd(kd_) {}

    Vec eval(const Vec& n, const Vec& o, const Vec& i) const {
        return kd * (1.0 / PI);
    }

    /**
     * Sample using uniformRandomPSA
     */
    void sample(const Vec& n, const Vec& o, Vec& i, double& pdf) const {
        double z = sqrt(rng());
        double r = sqrt(1.0 - z * z);
        double phi = 2.0 * PI * rng();
        double x = r * cos(phi);
        double y = r * sin(phi);
        Vec u, v, w;
        createLocalCoord(n, u, v, w);
        i = u * x + v * y + w * z;
        pdf = clamp(i.dot(n)) / PI;
    }

    bool isSpecular() const { return false; }

    Vec kd;
};

// Ideal specular BRDF
struct SpecularBRDF : public BRDF {
    SpecularBRDF(Vec ks_) : ks(ks_) {}

    static Vec mirroredDirection(const Vec& n, const Vec& o) {
        return n * n.dot(o) * 2.0 - o;
    }

    Vec eval(const Vec& n, const Vec& o, const Vec& i) const {
        if (i == mirroredDirection(n, o)) {
            return ks * (1.0 / n.dot(i));
        }
        return Vec();
    }

    void sample(const Vec& n, const Vec& o, Vec& i, double& pdf) const {
        i = mirroredDirection(n, o);
        pdf = 1.0;
    }

    bool isSpecular() const { return true; }

    Vec ks;
};

/*
 * Scene configuration
 */

 // Pre-defined BRDFs
const DiffuseBRDF leftWall(Vec(.75, .25, .25)),
rightWall(Vec(.25, .25, .75)),
otherWall(Vec(.75, .75, .75)),
blackSurf(Vec(0.0, 0.0, 0.0)),
brightSurf(Vec(0.9, 0.9, 0.9));
const SpecularBRDF shinySurf(Vec(0.999, 0.999, 0.999));

// Scene: list of spheres
const Sphere spheres[] = {
    Sphere(1e5,  Vec(1e5 + 1,40.8,81.6),   Vec(),         leftWall),   // Left
    Sphere(1e5,  Vec(-1e5 + 99,40.8,81.6), Vec(),         rightWall),  // Right
    Sphere(1e5,  Vec(50,40.8, 1e5),      Vec(),         otherWall),  // Back
    Sphere(1e5,  Vec(50, 1e5, 81.6),     Vec(),         otherWall),  // Bottom
    Sphere(1e5,  Vec(50,-1e5 + 81.6,81.6), Vec(),         otherWall),  // Top
    Sphere(16.5, Vec(27,16.5,47),        Vec(),         brightSurf), // Ball 1
    Sphere(16.5, Vec(73,16.5,78),        Vec(),         shinySurf), // Ball 2
    Sphere(5.0,  Vec(50,70.0,81.6),      Vec(50,50,50), blackSurf)   // Light
};

// Camera position & direction
const Ray cam(Vec(50, 52, 295.6), Vec(0, -0.042612, -1).normalize());


/*
 * Global functions
 */

bool intersect(const Ray& r, double& t, int& id) {
    double n = sizeof(spheres) / sizeof(Sphere), d, inf = t = 1e20;
    for (int i = int(n); i--;) if ((d = spheres[i].intersect(r)) && d < t) { t = d; id = i; }
    return t < inf;
}

Vec reflectedRadiance(const Ray& r, int depth) {
    double t;                                   // Distance to intersection
    int id = 0;                                 // id of intersected sphere

    if (!intersect(r, t, id)) return Vec();   // if miss, return black
    const Sphere& obj = spheres[id];            // the hit object

    Vec x = r.o + r.d * t;                        // The intersection point
    Vec o = (Vec() - r.d).normalize();          // The outgoing direction (= -r.d)

    Vec n = (x - obj.p).normalize();            // The normal direction
    if (n.dot(o) < 0) n = n * -1.0;

    /*
    Tips

    1. Other useful quantities/variables:
    Vec Le = obj.e;                             // Emitted radiance
    const BRDF &brdf = obj.brdf;                // Surface BRDF at x

    2. Call brdf.sample() to sample an incoming direction and continue the recursion
    */

    // Direct radiance
    const Sphere& light = spheres[7];

    // Sample random point on the light source
    Vec y1;
    double pdf;
    light.sample(y1, pdf);

    // Some calculations we need for radiance
    Vec xToY = (y1 - x);
    Vec w1 = (Vec(xToY)).normalize();
    Vec w1_neg = Vec(-w1.x, -w1.y, -w1.z);
    double r_sq = (xToY).dot(xToY);
    Vec ny = (y1 - light.p).normalize();

    // Mutually visible if rays from each object intersect each other
    int id2;
    int visibility = intersect(Ray(x, w1), t, id2) && id2 == 7 && intersect(Ray(y1, w1_neg), t, id2) && id2 == id ? 1 : 0;

    // Final calculation for direct radiance
    Vec reflRad = light.e.mult(obj.brdf.eval(n, w1, o)) * visibility * clamp(n.dot(w1)) * clamp(ny.dot(w1_neg)) * (1.0 / (r_sq * pdf));

    // Russian roulette
    double p = depth <= 5 ? 1 : 0.9;
    if (rng() < p) {
        // Sample new direction
        Vec w2;
        double pdf2;
        obj.brdf.sample(n, o, w2, pdf2);

        // Add radiance from new sampled direction
        Ray y2(x, w2);
        reflRad = reflRad + reflectedRadiance(y2, depth + 1).mult(obj.brdf.eval(n, w2, o)) * clamp(n.dot(w2)) * (1.0 / (pdf2 * p));
    }

    return reflRad;
}

/*
 * KEY FUNCTION: radiance estimator
 */

Vec receivedRadiance(const Ray& r, int depth, bool flag) {
    double t;                                   // Distance to intersection
    int id = 0;                                 // id of intersected sphere

    if (!intersect(r, t, id)) return Vec();   // if miss, return black
    const Sphere& obj = spheres[id];            // the hit object

    Vec x = r.o + r.d * t;                        // The intersection point
    Vec o = (Vec() - r.d).normalize();          // The outgoing direction (= -r.d)

    Vec n = (x - obj.p).normalize();            // The normal direction
    if (n.dot(o) < 0) n = n * -1.0;

    // If specular, use the radiance calculation from task 2
    if (obj.brdf.isSpecular()) {
        // Emitted radiance
        Vec rad = obj.e;

        // Russian roulette
        double p = depth <= 5 ? 1 : 0.9;
        if (rng() < p) {
            // Sample new direction
            Vec i;
            double pdf;
            obj.brdf.sample(n, o, i, pdf);
            Ray Y(x, i);

            // Add radiance from new sampled direction
            rad = rad + receivedRadiance(Y, depth + 1, false).mult(obj.brdf.eval(n, o, i)) * (clamp(n.dot(i)) / (pdf * p));
        }
        return rad;
    }

    // Otherwise, use our next event estimation
    return obj.e + reflectedRadiance(r, depth);
}


/*
 * Main function (do not modify)
 */

int main(int argc, char* argv[]) {
    int nworkers = omp_get_num_procs();
    omp_set_num_threads(nworkers);
    rng.init(nworkers);

    int w = 480, h = 360, samps = argc == 2 ? atoi(argv[1]) / 4 : 16; // # samples
    Vec cx = Vec(w * .5135 / h), cy = (cx.cross(cam.d)).normalize() * .5135;
    std::vector<Vec> c(w * h);

    int tot = 0;
    omp_set_num_threads(16);
    #pragma omp parallel for
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            const int i = (h - y - 1) * w + x;
            for (int sy = 0; sy < 2; ++sy) {
                for (int sx = 0; sx < 2; ++sx) {
                    Vec r;
                    for (int s = 0; s < samps; s++) {
                        double r1 = 2 * rng(), dx = r1 < 1 ? sqrt(r1) - 1 : 1 - sqrt(2 - r1);
                        double r2 = 2 * rng(), dy = r2 < 1 ? sqrt(r2) - 1 : 1 - sqrt(2 - r2);
                        Vec d = cx * (((sx + .5 + dx) / 2 + x) / w - .5) +
                            cy * (((sy + .5 + dy) / 2 + y) / h - .5) + cam.d;
                        r = r + receivedRadiance(Ray(cam.o, d.normalize()), 1, true) * (1. / samps);
                    }
                    c[i] = c[i] + Vec(clamp(r.x), clamp(r.y), clamp(r.z)) * .25;
                }
            }
        }
    #pragma omp critical
        fprintf(stderr, "\rRendering (%d spp) %6.2f%%", samps * 4, 100. * (++tot) / h);
    }
    fprintf(stderr, "\n");

    // Write resulting image to a PPM file
    FILE* f = fopen("image.ppm", "w");
    fprintf(f, "P3\n%d %d\n%d\n", w, h, 255);
    for (int i = 0; i < w * h; i++)
        fprintf(f, "%d %d %d ", toInt(c[i].x), toInt(c[i].y), toInt(c[i].z));
    fclose(f);

    return 0;
}
