#VERTEX_SHADER
#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoords;

uniform mat4 MVP;

out vec2 TexCoords;

void main() {
	TexCoords = texCoords;
	//TexCoords.y = 1.0-texCoords.y;
	gl_Position = MVP*vec4(position.xy, 0.0, 1.0);
}

#FRAGMENT_SHADER
#version 460 core

layout(location = 0) out vec4 fragColor;

uniform vec2 iResolution;
uniform float iTime;
uniform int iFrame;
uniform sampler2D lastFrameTex;

//uniform sampler2D screenTexture;
//uniform int shouldTonemap;

in vec2 TexCoords;

uniform vec3 cameraPos;
uniform vec3 cameraRot;

struct Vertex {
	vec4 position;
    //vec2 uv;
    //vec4 normal;
};

layout (std430, binding=10) readonly buffer meshData
{ 
  Vertex vertices[];
};

// Pathtracing

// Common
#define PI 3.14159265
#define ACCUMULATE
#define EXPOSURE 0.5

uint wang_hash(inout uint seed)
{
    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}

uint pcg_hash(inout uint seed)
{
    seed = seed * 747796405u + 2891336453u;
    uint word = ((seed >> ((seed >> 28u) + 4u)) ^ seed) * 277803737u;
    return (word >> 22u) ^ word;
}
 
float RandomFloat01(inout uint state)
{
    return float(pcg_hash(state)) / 4294967296.0;
}
 
vec3 RandomUnitVector(inout uint state)
{
    float z = RandomFloat01(state) * 2.0f - 1.0f;
    float a = RandomFloat01(state) * 2.0*PI;
    float r = sqrt(1.0f - z * z);
    float x = r * cos(a);
    float y = r * sin(a);
    return vec3(x, y, z);
}

// Unity Random Range Implamentation
float randomRange(vec2 seed, float rMin, float rMax)
{
    float randomno =  fract(sin(dot(seed, vec2(12.9898, 78.233)))*43758.5453);
    return mix(rMin, rMax, randomno);
}

vec3 LessThan(vec3 f, float value)
{
    return vec3(
        (f.x < value) ? 1.0f : 0.0f,
        (f.y < value) ? 1.0f : 0.0f,
        (f.z < value) ? 1.0f : 0.0f);
}

vec3 LinearToSRGB(vec3 rgb)
{
    rgb = clamp(rgb, 0.0f, 1.0f);
    
    return mix(
        pow(rgb, vec3(1.0f / 2.4f)) * 1.055f - 0.055f,
        rgb * 12.92f,
        LessThan(rgb, 0.0031308f)
    );
}

vec3 SRGBToLinear(vec3 rgb)
{
    rgb = clamp(rgb, 0.0f, 1.0f);
    
    return mix(
        pow(((rgb + 0.055f) / 1.055f), vec3(2.4f)),
        rgb / 12.92f,
        LessThan(rgb, 0.04045f)
	);
}

// ACES tone mapping curve fit to go from HDR to LDR
//https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x + b)) / (x*(c*x + d) + e), 0.0f, 1.0f);
}

float FresnelReflectAmount(float n1, float n2, vec3 normal, vec3 incident, float f0, float f90)
{
        // Schlick aproximation
        float r0 = (n1-n2) / (n1+n2);
        r0 *= r0;
        float cosX = -dot(normal, incident);
        if (n1 > n2)
        {
            float n = n1/n2;
            float sinT2 = n*n*(1.0-cosX*cosX);
            // Total internal reflection
            if (sinT2 > 1.0)
                return f90;
            cosX = sqrt(1.0-sinT2);
        }
        float x = 1.0-cosX;
        float ret = r0+(1.0-r0)*x*x*x*x*x;
 
        // adjust reflect multiplier for object reflectivity
        return mix(f0, f90, ret);
}

// Buffer
struct Ray {
  vec3 origin;
  vec3 dir;
};

struct Material {
    vec4 albedo;
    float specular;
    float roughness;
    float metalicness;
    float IOR;
    float transmition;
    
    
    vec3 emissiveColor;
    float emissivePower;
};

struct SceneObject {
    vec3 pos;
    vec3 normal;
    Material mat;
};

struct Sphere {
    vec3 pos;
    float radius;
    Material mat;
};

struct Box {
    vec3 pos;
    vec3 size;
    Material mat;
};

struct Scene {
    float d; // HitPoint Distance
    SceneObject closestHit;
};

struct RenderData {
    Scene scene;
    vec4 color;
};

#define MAX_DIST 100.0
#define MAX_LIGHT_BOUNCES 16
#define MIN_TRACE_DIST 0.001
#define NORMAL_OFFSET 0.001

vec3 lightPos = (vec3(2.0, 3.0, 3.0))*1.0;



float iSphere(in Ray ray, in Sphere sph)
{
	//sphere at origin has equation |xyz| = r
	//sp |xyz|^2 = r^2.
	//Since |xyz| = ro + t*rd (where t is the parameter to move along the ray),
	//we have ro^2 + 2*ro*rd*t + t^2 - r2. This is a quadratic equation, so:
	vec3 oc = ray.origin - sph.pos; //distance ray origin - sphere center
	
	float b = dot(oc, ray.dir);
	float c = dot(oc, oc) - sph.radius * sph.radius; //sph.w is radius
	float h = b*b - c; //Commonly known as delta. The term a is 1 so is not included.
	
    const float missHit = -1.0;
	float t = (h < 0.0) ? missHit : -b - sqrt(h); // We use -sqrt because we want the CLOSEST distance
    
	return t;
}

// axis aligned box centered at the origin, with size boxSize
float boxIntersection( in vec3 ro, in vec3 rd, vec3 boxSize, out vec3 outNormal ) 
{
    vec3 m = vec3(1.0) / rd;
	vec3 n = m * ro;
	vec3 k = abs(m) * boxSize;
	vec3 t1 = -n - k;
	vec3 t2 = -n + k;
	float tN = max(max(t1.x, t1.y), t1.z);
	float tF = min(min(t2.x, t2.y), t2.z);
    
    vec3 yzx = vec3(t1.y, t1.z, t1.x);
	vec3 zxy = vec3(t1.z, t1.x, t1.y);
    outNormal = -sign(rd) * step(yzx, t1) * step(zxy, t1);
    
	return (tN > tF || tF < 0.0) ?
        -1.0 :        // no hit
        min(tN, tF);
}

vec3 triIntersect( in vec3 ro, in vec3 rd, in vec3 v0, in vec3 v1, in vec3 v2 )
{
    vec3 v1v0 = v1 - v0;
    vec3 v2v0 = v2 - v0;
    vec3 rov0 = ro - v0;
    vec3  n = cross( v1v0, v2v0 );
    vec3  q = cross( rov0, rd );
    float d = 1.0/dot( rd, n );
    float u = d*dot( -q, v2v0 );
    float v = d*dot(  q, v1v0 );
    float t = d*dot( -n, rov0 );
    if( u<0.0 || v<0.0 || (u+v)>1.0 ) t = -1.0;
    return vec3( t, u, v );
}

bool miss(float hit) { return hit <= MIN_TRACE_DIST || hit > MAX_DIST; }

uniform int vCount;
Scene world(Ray ray) {
    Scene scene;
    scene.d = MAX_DIST;
    
    Material m1 = Material(vec4(0.1, 0.7, 0.9, 1.0), 0.0, 1.0, 0.0, 0.0, 0.0, vec3(0.0), 0.0); // sin(iTime)*0.5 + 0.5
    Material m2 = Material(vec4(0.9, 0.4, 0.1, 1.0), 0.0, 1.0, 0.5, 0.0, 0.0, vec3(0.0), 0.0);
    Material refM = Material(vec4(0.2, 0.3, 0.8, 1.0), 0.7, 0.1, 0.0, 1.0, 0.0, vec3(0.0), 0.0);
    
    Material wlm = Material(vec4(0.0), 0.0, 1.0, 0.0, 0.0, 0.0, vec3(1.0), 20.0);
    Material lm = Material(vec4(0.0), 0.0, 1.0, 0.0, 0.0, 0.0, vec3(0.9, 0.5, 0.1), 20.0);
    
    Sphere[] sphs = Sphere[] (
        Sphere(vec3(0.0), 0.85, m1),
        //Sphere(vec3(0.0, 0.9, 0.0), 0.05, m2),
        Sphere(lightPos, 2.0, wlm) // lightSrc
    );
    
    for(int i = 0; i < 2; i++) { 
        float sphereHit = iSphere(ray, sphs[i]);
        if (miss(sphereHit)) continue;
        scene.d = min(scene.d, sphereHit);
        vec3 normal = normalize(ray.origin - sphs[i].pos + scene.d * ray.dir);
        SceneObject obj = SceneObject(sphs[i].pos, normal, sphs[i].mat);
        if (scene.d == sphereHit) scene.closestHit = obj;
    }

    Box[] boxes = Box[] (
        Box(vec3(0.0, 1.2, .0), vec3(0.5, 0.1, 0.1), m2)
    );

    for(int i = 0; i < 0; i++) {
        vec3 normal = vec3(0.0);
        float boxHit = boxIntersection(ray.origin - boxes[i].pos, ray.dir, boxes[i].size, normal);
        if (miss(boxHit)) continue;
        scene.d = min(scene.d, boxHit);
        SceneObject obj = SceneObject(boxes[i].pos, normalize(normal), boxes[i].mat);
        if (scene.d == boxHit) scene.closestHit = obj;
    }

    
    for (int i = 0; i < vCount; i+=3) {
        vec3 posOffset = vec3(0.0, 0.85, 0.0); //vec3(0.0, 0.9 + sin(iTime)+.0, -0.3);
        float size = 2.0;
        vec3 v1 = vertices[i].position.xyz * size;
        vec3 v2 = vertices[i+1].position.xyz * size;
        vec3 v3 = vertices[i+2].position.xyz * size;
        vec3 n1 = cross(v1, v2);//vertices[i].normal;
        float triHit = triIntersect(ray.origin - posOffset, ray.dir, v1, v2, v3).x;
        if (miss(triHit)) continue;
        scene.d = min(scene.d, triHit);
        SceneObject obj = SceneObject(posOffset, normalize(n1), m2);
        if (scene.d == triHit) scene.closestHit = obj;
    }

    /*for(int s = 0; s < vCount; s++) {
        vec3 posOffset = vec3(0.0, 0.8, 0.0); //vec3(0.0, 0.9 + sin(iTime)+.0, -0.3);
        float size = 1.0;
        Sphere sph = Sphere( vertices[s].position.xyz + posOffset, 0.02, m2);
        float sphereHit = iSphere(ray, sph);
        if (miss(sphereHit)) continue;
        scene.d = min(scene.d, sphereHit);
        vec3 normal = normalize(ray.origin - sph.pos + scene.d * ray.dir);
        SceneObject obj = SceneObject(sph.pos, normal, sph.mat);
        if (scene.d == sphereHit) scene.closestHit = obj;
    }*/
    

    return scene;
}

vec3 worldNormals(Ray ray, float world) {
    return normalize(ray.origin + ray.dir * world);
}


void worldLight() {
    lightPos = normalize(vec3( cos(iTime), 0.0, sin(iTime) ));
}

vec4 skyColor(vec2 uv) {
    
    float red = 0.1;
    float green = 0.25;
    float blue = 0.5;
    
    vec4 color = vec4(red, green, blue, 1.0) * 2.0; 
    //color = vec4(0.0);
    return color;
}

uint seed = 0u;
void genSeed() {
    seed = uint(uint(uint(gl_FragCoord.x) * uint(1973) + uint(gl_FragCoord.y) * uint(9277) + uint(iFrame) * uint(26699)) | uint(1));
}

RenderData worldRender(vec2 uv, Ray ray) {
    
    //worldLight();
    
    Scene scene;
    
    vec3 color = vec3(0.0);
    vec3 light = vec3(0.0);
    vec3 throughput = vec3(1.0);

    for(int i = 0; i < MAX_LIGHT_BOUNCES; i++) {
        scene = world(ray);
        if(scene.d >= MAX_DIST) { light += (skyColor(uv).rgb) * throughput; break; }
        vec3 closestHit = scene.closestHit.pos;
        vec3 normals = scene.closestHit.normal;
        
        if(i == MAX_LIGHT_BOUNCES-1 ) { light *= 0.0; break; }

        vec3 hitPoint = ray.origin - closestHit + (ray.dir*scene.d);
        
        //vec3 normals = normalize(hitPoint);
        // Remap normals from [-1 1] to [0 1]
        

        vec4 hitColor = scene.closestHit.mat.albedo;
        
        throughput *= hitColor.rgb + scene.closestHit.mat.emissiveColor;
        light += scene.closestHit.mat.emissivePower * (throughput + scene.closestHit.mat.emissiveColor*0.25);
        
        color = normals;
        
        // Trace
        ray.origin = hitPoint + closestHit + normals*NORMAL_OFFSET;

        genSeed();
        
        // apply fresnel
        float specularChance = scene.closestHit.mat.specular;
        if (specularChance > 0.0f)
        {
            specularChance = FresnelReflectAmount(
                1.0,
                scene.closestHit.mat.IOR,
                ray.dir, normals, specularChance, 1.0f);  
        }
        
        // calculate whether we are going to do a diffuse or specular reflection ray 
        float doSpecular = (RandomFloat01(seed) < specularChance) ? 1.0f : 0.0f;
 
        // Calculate a new ray direction.
        // Diffuse uses a normal oriented cosine weighted hemisphere sample.
        // Perfectly smooth specular uses the reflection ray.
        // Rough (glossy) specular lerps from the smooth specular to the rough diffuse by the material roughness squared
        // Squaring the roughness is just a convention to make roughness feel more linear perceptually.
        vec3 randomSpread = RandomUnitVector(seed);
        vec3 diffuseRayDir = normalize(normals + randomSpread);
        vec3 specularRayDir = reflect(ray.dir, normals);
        float sqrRoughness = scene.closestHit.mat.roughness * scene.closestHit.mat.roughness;
        vec3 glossyDir = mix(specularRayDir, diffuseRayDir, sqrRoughness);
        specularRayDir = normalize(glossyDir);
        ray.dir = mix(diffuseRayDir, specularRayDir, doSpecular);
        
        //vec3 normalShift = scene.closestHit.mat.roughness * randomSpread;
        //ray.dir = normalize(ray.dir + normalShift); //normalize(reflect(ray.dir, normals + normalShift));
    }
        
    //light += throughput/float(MAX_LIGHT_BOUNCES/2);
    return RenderData(scene, vec4(light, 1.0));
}

mat2 rotation(float angle) {
    return mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
}

void main()
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = gl_FragCoord.xy/iResolution.xy * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    lightPos = vec3(sin(3.14/2.0), 1.0, cos(3.14/2.0)) * 4.0;
    genSeed();
    
    // calculate subpixel camera jitter for anti aliasing
    vec2 jitter = vec2(RandomFloat01(seed), RandomFloat01(seed)) - 0.5f;
    vec2 jitteredUV = (gl_FragCoord.xy+jitter)/iResolution.xy * 2.0 - 1.0;
    jitteredUV.x *= aspect;
    
    vec3 ro = cameraPos;
    vec3 rd = normalize(vec3(jitteredUV, 1.0));
    rd.yz *= rotation(cameraRot.x);
    rd.xz *= rotation(cameraRot.y);
    rd.xy *= rotation(cameraRot.z);

    // Orbit
    //ro.xz *= rotation(iTime);
    //rd.xz *= rotation(iTime);

    Ray ray = Ray(ro, rd);
    
    vec4 finalRender = worldRender(uv, ray).color;
    vec3 lastFrame = texture(lastFrameTex, gl_FragCoord.xy/iResolution.xy).rgb;
    vec3 finalColor = finalRender.rgb;
    
    #ifdef ACCUMULATE
        finalColor += lastFrame;
    #endif
    
    fragColor = vec4(finalColor, 1.0);
}