struct Ray {
  vec3 origin;
  vec3 dir;
};

struct Material {
    vec4 albedo;
    float roughness;
    float metalicness;
    
    vec3 emissiveColor;
    float emissivePower;
};

struct Sphere {
    vec3 pos;
    float radius;
    Material mat;
};

struct Scene {
    float d; // HitPoint Distance
    Sphere closestHit;
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

bool miss(float hit) { return hit <= MIN_TRACE_DIST || hit > MAX_DIST; }

Scene world(Ray ray) {
    Scene scene;
    scene.d = MAX_DIST;
    
    Material m1 = Material(vec4(0.1, 0.7, 0.9, 1.0), 1.0, 0.0, vec3(0.0), 0.0); // sin(iTime)*0.5 + 0.5
    Material m2 = Material(vec4(0.9, 0.4, 0.1, 1.0), 1.0, 0.5, vec3(0.0), 0.0);
    Material refM = Material(vec4(0.8), 0.05, 0.0, vec3(0.0), 0.0);
    
    Material wlm = Material(vec4(0.0), 1.0, 0.0, vec3(1.0), 20.0);
    Material lm = Material(vec4(1.0), 1.0, 0.0, vec3(0.9, 0.5, 0.1), 20.0);
    
    Sphere[] sphs = Sphere[] (
        Sphere(vec3(0.0), 0.85, m1),
        Sphere(vec3(0.0, 0.9, 0.0), 0.05, m2),
        Sphere(lightPos, 2.0, lm) // lightSrc
    );
    
    for(int i = 0; i < 3; i++) {
        float sphereHit = iSphere(ray, sphs[i]);
        if (miss(sphereHit)) continue;
        scene.d = min(scene.d, sphereHit);
        if (scene.d == sphereHit) scene.closestHit = sphs[i];
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
    
    vec4 color = vec4(red, green, blue, 1.0) * 2.0; 
    color = vec4(0.0);
    return color;
}

uint seed = 0u;
vec2 fragCoords = vec2(0.0);
void genSeed() {
    seed = uint(uint(uint(fragCoords.x) * uint(1973) + uint(fragCoords.y) * uint(9277) + uint(iFrame) * uint(26699)) | uint(1));
}

RenderData worldRender(vec2 uv, Ray ray) {
    
    //worldLight();
    
    Scene scene;
    
    vec3 color = vec3(0.0);
    vec3 light = vec3(0.0);
    vec3 throughput = vec3(1.0);

    for(int i = 0; i < 2; i++) {
        scene = world(ray);
        if(scene.d >= MAX_DIST) { light += skyColor(uv).rgb * throughput; break; }
        vec3 closestHit = scene.closestHit.pos;
        
        //if(i == MAX_LIGHT_BOUNCES-1 ) { light *= 0.0; break; }

        //ray.origin -= closestHit;
        vec3 hitPoint = ray.origin - closestHit + (ray.dir*scene.d);
        
        vec3 normals = normalize(hitPoint);
        // Remap normals from [-1 1] to [0 1]
        

        vec4 hitColor = scene.closestHit.mat.albedo;
        
        throughput *= hitColor.rgb;
        light += scene.closestHit.mat.emissivePower * (throughput + scene.closestHit.mat.emissiveColor*0.25);
        
        //light += max(dot(normalize(lightPos-closestHit), normals) * scene.closestHit.mat.albedo.rgb, 0.0);

        
        color = normals;
        
        // Trace
        ray.origin = hitPoint + closestHit + normals*NORMAL_OFFSET;

        genSeed();

        vec3 randomSpread = RandomUnitVector(seed);
        vec3 normalShift = scene.closestHit.mat.roughness * randomSpread;
        ray.dir = normalize(ray.dir + normalShift); //normalize(reflect(ray.dir, normals + normalShift));
    }
        
    //light += throughput/float(MAX_LIGHT_BOUNCES/2);
    return RenderData(scene, vec4(light, 1.0));
}

mat2 rotate(float angle) {
    return mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
}

bool mouseUpdate(inout vec3 camera) {
    vec2 mouse = iMouse.xy / iResolution.xy * 2.0;
    
    //camera.yz *= rotate(mouse.y * 3.14);
    camera.xz *= rotate(mouse.x * 3.14 * 1.5);
    //camera.xz *= rotate(iMouse.x * 20.0 / iChannelResolution[0].x);
    //camera.y = iMouse.y * 5.0 / iChannelResolution[0].y - 1.0;

    return false;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    
    vec3 ro = vec3(0.0, 0.9*1.0, -0.1);
    vec3 rd = normalize(vec3(uv, 1.0));
    //rd.xz *= rotation(-sin(iTime)*3.14/2.0);//
    
    mouseUpdate(ro);
    mouseUpdate(rd);
    
    Ray ray = Ray(ro, rd);
    fragCoords = fragCoord;
    
    vec4 finalRender = worldRender(uv, ray).color;
    vec4 lastFrame = texture(iChannel0, fragCoord/iResolution.xy);
    //vec4 finalColor = mix(finalRender, lastFrame, 1.0f / float(iFrame+1));
    vec4 finalColor = vec4(finalRender.rgb, 1.0);
    
    #ifdef ACCUMULATE
    finalColor += vec4(lastFrame.rgb, 0.0);
    #endif
    
    fragColor = finalColor;
}