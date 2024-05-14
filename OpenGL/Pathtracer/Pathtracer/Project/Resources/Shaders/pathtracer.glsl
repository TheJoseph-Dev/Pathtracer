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

in vec2 TexCoords;

uniform vec3 cameraPos;
uniform vec3 cameraRot;

struct MeshInfo {
    vec4 vCount; // vec4 because gotta satisfy std430
    vec4 gPos;
};

struct Vertex {
	vec4 position;
    vec4 uv;
    vec4 normal;
};

struct ObjectInfo {
    vec4 pos;
    float type;
    float matIndex;
    float size;
    float w;
};

layout (std430, binding=10) readonly buffer meshInfoData {
   MeshInfo mInfo[];
};

layout (std430, binding=11) readonly buffer meshData
{ 
  Vertex vertices[];
};

layout (std430, binding=12) readonly buffer objsData {
    ObjectInfo objsInfo[];
};

// Pathtracing

// Common
#define PI 3.14159265
//#define ACCUMULATE
#define EXPOSURE 0.5

uniform int accumulate;

mat2 rotation(float angle) {
    return mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
}

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
    float transmission;
    
    
    vec3 emissiveColor;
    float emissivePower;
};

struct SceneObject {
    vec3 pos;
    vec3 normal;
    vec3 normal2;
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
    float d2; // Second HitPoint (if exists)
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



float iSphere(in Ray ray, in Sphere sph, out float d2)
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
    d2 = (h < 0.0) ? missHit : -b + sqrt(h);

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

uniform sampler2D meshTexture;
//uniform int mCount; // The meshes count coming from SSBO
#define SPHERE_TYPE 0
#define BOX_TYPE 1

Scene world(Ray ray) {
    Scene scene;
    scene.d = MAX_DIST;
    scene.d2 = MAX_DIST;
    
    Material m1 = Material(vec4(0.1, 0.7, 0.9, 1.0), 0.0, 1.0, 0.0, 0.0, 0.0, vec3(0.0), 0.0);
    Material m2 = Material(vec4(0.9, 0.4, 0.1, 1.0), 0.0, 1.0, 0.5, 0.0, 0.0, vec3(0.0), 0.0);
    Material tranM = Material(vec4(1.0, 0.7, 0.9, 1.0), 0.7, 0.2, 0.0, 1.0, 0.4, vec3(0.0), 0.0);
    Material wRefM = Material(vec4(1.0), 0.02, 0.5, 0.0, 2.0, 0.0, vec3(0.0), 0.0);
    
    Material wlm = Material(vec4(0.0), 0.0, 1.0, 0.0, 0.0, 0.0, vec3(1.0), 20.0);
    Material lm = Material(vec4(0.0), 0.0, 1.0, 0.0, 0.0, 0.0, vec3(0.9, 0.5, 0.1), 10.0);
    
    Material[] mats = Material[] ( m1, m2, tranM, wRefM, wlm, lm );

    for(int i = 0; i < objsInfo.length(); i++) {
        ObjectInfo objInfo = objsInfo[i];
        if(objInfo.type == SPHERE_TYPE) {
            Sphere sph = Sphere(objInfo.pos.xyz, objInfo.size, mats[int(objInfo.matIndex)]);
            float d2;
            float sphereHit = iSphere(ray, sph, d2);
            if (miss(sphereHit)) continue;
            scene.d = min(scene.d, sphereHit);
            vec3 normal = normalize(ray.origin - objInfo.pos.xyz + scene.d * ray.dir);
            vec3 normal2 = normalize(ray.origin - objInfo.pos.xyz + d2 * ray.dir);
            SceneObject obj = SceneObject(objInfo.pos.xyz, normal, normal2, sph.mat);
            if (scene.d == sphereHit) { 
                scene.closestHit = obj;
                scene.d2 = d2;
            }
        }
        else if(objInfo.type == BOX_TYPE) {
            Box box = Box(objInfo.pos.xyz, vec3(objInfo.size), mats[int(objInfo.matIndex)]);
            vec3 normal = vec3(0.0);
            float boxHit = boxIntersection(ray.origin - objInfo.pos.xyz, ray.dir, box.size, normal);
            if (miss(boxHit)) continue;
            scene.d = min(scene.d, boxHit);
            SceneObject obj = SceneObject(objInfo.pos.xyz, normalize(normal), vec3(0.0), box.mat);
            if (scene.d == boxHit) scene.closestHit = obj;
        }
    }

    Material texM = Material(vec4(0.0), 0.0, 1.0, 0.5, 0.0, 0.0, vec3(0.0), 0.0);
    for(int m = 0; m < mInfo.length(); m++) {
        for (int i = int(mInfo[m-1].vCount.x); i < mInfo[m].vCount.x; i+=3) {
            vec3 posOffset = mInfo[m].gPos.xyz;
            float size = 2.0;
            vec3 v1 = vertices[i].position.xyz * size;
            vec3 v2 = vertices[i+1].position.xyz * size;
            vec3 v3 = vertices[i+2].position.xyz * size;
            //v1.xz *= rotation(3.14/4.0); v2.xz *= rotation(3.14/4.0); v3.xz *= rotation(3.14/4.0);
            vec3 n = normalize(vertices[i].normal).xyz;
            //n.xz *=  rotation(3.14/4.0);
            float triHit = triIntersect(ray.origin - posOffset, ray.dir, v1, v2, v3).x;
            if (miss(triHit)) continue;
            scene.d = min(scene.d, triHit);

            vec2 tC = vertices[i].uv.xy;
            vec4 texColor = texture(meshTexture, tC);
            texM.albedo = texColor;
            SceneObject obj = SceneObject(posOffset, n, vec3(0.0), texM);
            if (scene.d == triHit) scene.closestHit = obj;
        }
    }

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
    
    vec4 color = vec4(red, green, blue, 1.0) * 1.0; 
    //color = vec4(0.5);
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
        vec3 normals2 = scene.closestHit.normal2;
        
        if(i == MAX_LIGHT_BOUNCES-1 ) { light *= 0.0; break; }

        vec3 hitPoint = ray.origin - closestHit + (ray.dir*scene.d);
        vec3 hitPoint2 = ray.origin - closestHit + (ray.dir*scene.d2);

        vec4 hitColor = scene.closestHit.mat.albedo;
        
        throughput *= hitColor.rgb + scene.closestHit.mat.emissiveColor;
        light += scene.closestHit.mat.emissivePower * (throughput + scene.closestHit.mat.emissiveColor*0.25);
        
        color = normals;

        // do absorption if we are hitting from inside the object
        // if (hitInfo.fromInside)
        //    throughput *= exp(-hitInfo.material.refractionColor * hitInfo.dist);
        
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

        float refractChance = scene.closestHit.mat.transmission;
        
        // calculate whether we are going to do a diffuse or specular reflection ray 
        float doSpecular = (RandomFloat01(seed) < specularChance) ? 1.0f : 0.0f;
        float doRefraction = (RandomFloat01(seed) < refractChance) ? 1.0f : 0.0f;

        if(doRefraction == 1.0f) {
            ray.origin -= 2*NORMAL_OFFSET*normals;
            ray.dir = normalize(refract(ray.dir, normals, 1.01/scene.closestHit.mat.IOR));

            //Scene preRefTrace = world(ray);
            ///normals2 = preRefTrace.closestHit.normal2;
            //closestHit = preRefTrace.closestHit.pos;
            //hitPoint2 = ray.origin - closestHit + (ray.dir*preRefTrace.d2);
            //ray.origin = hitPoint2 + closestHit + normals2*NORMAL_OFFSET;
            //ray.dir = normalize(refract(ray.dir, -normals2, preRefTrace.closestHit.mat.IOR/1.01));
            //hitColor = preRefTrace.closestHit.mat.albedo;
        
            //throughput *= hitColor.rgb + preRefTrace.closestHit.mat.emissiveColor;
            //light += preRefTrace.closestHit.mat.emissivePower * (throughput + preRefTrace.closestHit.mat.emissiveColor*0.25);

            /*
            vec3 randomSpread = RandomUnitVector(seed);
            vec3 diffuseRayDir = normalize(-normals2 + randomSpread);
            vec3 specularRayDir = reflect(ray.dir, -normals2);
            float sqrRoughness = preRefTrace.closestHit.mat.roughness * preRefTrace.closestHit.mat.roughness;
            vec3 glossyDir = mix(specularRayDir, diffuseRayDir, sqrRoughness);
            specularRayDir = normalize(glossyDir);
            ray.dir = mix(ray.dir, mix(diffuseRayDir, specularRayDir, doSpecular), 0.5);*/

        }
        else {
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
        }
        //vec3 normalShift = scene.closestHit.mat.roughness * randomSpread;
        //ray.dir = normalize(ray.dir + normalShift); //normalize(reflect(ray.dir, normals + normalShift));

        // Russian Roulette
        // As the throughput gets smaller, the ray is more likely to get terminated early.
        // Survivors have their value boosted to make up for fewer samples being in the average.
        {
        	float p = max(throughput.r, max(throughput.g, throughput.b));
        	if (RandomFloat01(seed) > p)
            	break;

        	// Add the energy we 'lose' by randomly terminating paths
        	throughput *= 1.0f / p;            
        }
    }
        
    return RenderData(scene, vec4(light, 1.0));
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
    
    if(accumulate == 1) finalColor += lastFrame;
    
    fragColor = vec4(finalColor, 1.0);
}