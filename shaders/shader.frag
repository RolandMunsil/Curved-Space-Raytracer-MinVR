#version 330

uniform vec3 iResolution; // viewport resolution (in pixels)

uniform vec4 userPos;
uniform vec4 userForwardDir;
uniform vec4 userUpDir;
uniform vec4 userRightDir;

in vec4 gl_FragCoord;
out vec4 fragColor;

/////////////////////////// IMPORTANT CONSTANTS ///////////////////////////

//Note: the containing 3-sphere always has a radius of 1

const float PI = 3.1415926535897932384626433832795;
const float TWO_PI = 2. * PI;
bool debug_overrideColor = false;

const float MIN_RAY_HIT_THRESHOLD = 0.001;


//////////////////////////// RAYTRACER PARAMS ////////////////////////////

float zoom = 500.0;
const int AA_AMOUNT = 1;
const int REFLECTION_COUNT = 4;
const float REFLECTANCE = 0.6;

const bool LIGHTING_ENABLED = true;
const float LIGHT_INTENSITY = 0.5;
const float AMBIENT_LIGHT = 0.1;
const vec4 LIGHT_POSITION = normalize(vec4(1.,0.,0., 0.25));

const vec3 BACKGROUND_COLOR = vec3(0);
const bool USER_SPHERE_VISIBLE = true;




///////////////////////////// UTILITY METHODS ////////////////////////////

float GeodesicDistance(vec4 p1, vec4 p2)
{
    float dotProd = dot(normalize(p1), normalize(p2));
    
    //clamp in case of float imprecision
    return acos(clamp(dotProd, -1.0, 1.0));
}

float AngleFromGeodesicDistance(float dist)
{
    return dist;
}

vec4 Reflect(vec4 normal, vec4 dir)
{
    vec4 n = normalize(normal);
    return dir - (2.*dot(dir, n))*n;
}

vec4 Project(vec4 toBeProjected, vec4 onto)
{
    vec4 normOnto = normalize(onto);
    return dot(toBeProjected, normOnto) * normOnto;
}

bool PointsAreEqualOrOpposite(vec4 pt1, vec4 pt2)
{
    return abs(dot(pt1, pt2)) == 1.0;
}


/////////////////////////////////// RAY ///////////////////////////////////

struct Ray
{
    vec4 origin;
    
    //For the purpose of this raytracer, the "direction" vector is the point in space that this
    //ray will reach at t=pi/2.
    
    //So the equation of this ray would be cos(t) * origin + sin(t) * direction
    vec4 direction;
};

vec4 PointAlongRay(Ray ray, float t)
{
    return normalize((cos(t) * ray.origin) + (sin(t) * ray.direction));
}

vec4 DirectionAtPointAlongRay(Ray ray, float t)
{
    return normalize((cos(t) * ray.direction) + (sin(t) * -ray.origin));
}

Ray RayFromAToB(vec4 from, vec4 to)
{    
    return Ray(from, normalize(to - Project(to, from)));
}

/////////////////////////////////// HIT ///////////////////////////////////

struct Hit
{
    bool isHit;
    float dist;
    vec4 normal;
    vec3 color;
    
    bool hasReflection;
    Ray reflectedRay;
};
Hit NO_HIT = Hit(false, -1., vec4(0.0), vec3(1.0, 0., 1.0), false, Ray(vec4(0), vec4(0)));
Hit HitWithoutReflection(bool isHit, float dist, vec4 normal, vec3 color)
{
    return Hit(isHit, dist, normal, color, false, Ray(vec4(0), vec4(0)));
}
    

////////////////////////////////// SPHERE /////////////////////////////////

struct Sphere
{
    vec4 center;
    float radius;
    vec3 color;
    
    bool hasCheckerboardPattern;
    bool isReflective;
    bool visibleFromInside;
};
   
Hit SphereHit(Sphere sphere, Ray ray)
{
    //A circle on the surface of a 3D sphere can be defined as the intersection
    //of a plane and a sphere (and conversely, the intersection of a plane and a
    //sphere will always generate a circle on the surface of the sphere)
    //
    //Similarly, intersecting a 3-dimensional "hyperplane" with a 4D sphere gets you
    //a sphere within the surface of the 4D sphere. We can use this to determine intersection.
    //Instead of trying to calculate the intersection of the ray and the sphere, we calculate
    //the intersection of the ray and the hyperplane that would generate the sphere if it 
    //intersected the 4D sphere.
    
    float angle = AngleFromGeodesicDistance(sphere.radius);
    vec4 volumeNormal = sphere.center;
    vec4 volumeNormalCenter = sphere.center * cos(angle);
    
    float A = dot(volumeNormal, ray.direction);
    float B = dot(volumeNormal, ray.origin);
    float C = dot(volumeNormal, volumeNormalCenter);

    float phaseShift = atan(B, A);
    float amplitude = sqrt((A*A)+(B*B));

    float asinInput = C / amplitude;
    if(abs(asinInput) > 1.)
    {
        return NO_HIT;
    }

    float asinVal = asin(asinInput);
    float asinAltVal = sign(asinVal) * (PI - abs(asinVal));
    
    float t1 = asinVal - phaseShift;
    float t2 = asinAltVal - phaseShift;
    
    while(t1 < 0.)      { t1 += TWO_PI; }
    while(t1 >= TWO_PI) { t1 -= TWO_PI; }
    while(t2 < 0.)      { t2 += TWO_PI; }
    while(t2 >= TWO_PI) { t2 -= TWO_PI; }
    
    //When we're inside a sphere, we can see through it.
    //(this is mainly to allow the user to have a sphere representing them.)
    bool rayIsComingFromWithinSphere = GeodesicDistance(ray.origin, sphere.center) <= sphere.radius;
    
    float t;
    float nearT = min(t1, t2);
    float farT = max(t1, t2);
    if(nearT < MIN_RAY_HIT_THRESHOLD && farT < MIN_RAY_HIT_THRESHOLD)
    {
        return NO_HIT;
    }
    else if(nearT < MIN_RAY_HIT_THRESHOLD)
    {
        t = farT;
    }
    else if(farT < MIN_RAY_HIT_THRESHOLD)
    {
        if(!sphere.visibleFromInside && rayIsComingFromWithinSphere)
        {
            return NO_HIT;
        }
        else
        {
            t = nearT;
        }
    }
    else
    {
        if(!sphere.visibleFromInside && rayIsComingFromWithinSphere)
        {
            t = farT;
        }
        else
        {       
            t = nearT;
        }
    }
    
    vec3 returnColor = sphere.color;
    
    vec4 hitPoint = PointAlongRay(ray, t); 
    
    //Draw a grid-like texture on the spheres to let you see how you rotate around them
    if(sphere.hasCheckerboardPattern)
    {  
        ivec4 alternating = ivec4(round(mod(vec4(floor(hitPoint / .06)), 2.)));
        if((alternating.x == 1) ^^ (alternating.y == 1) ^^ (alternating.z == 1) ^^ (alternating.w == 1))
        {
            returnColor = vec3(0);
        }
    }
    
    vec4 rayDirAtHitPoint = DirectionAtPointAlongRay(ray, t);
    vec4 vecToHitPoint = hitPoint - sphere.center;
    vec4 normal = normalize(vecToHitPoint - Project(vecToHitPoint, hitPoint));
    
    Ray reflectedRay = Ray(vec4(0), vec4(0));
    if(sphere.isReflective)
    {
        //Calculate reflection
        vec4 reflection = normalize(Reflect(normal, rayDirAtHitPoint));
        reflectedRay = Ray(hitPoint, reflection);
    }
    
    return Hit(true, t, normal, returnColor, sphere.isReflective, reflectedRay);  
}


////////////////////////////// SCENE OBJECTS //////////////////////////////

const Sphere lightObject = Sphere(LIGHT_POSITION, 0.05, vec3(1.0, 1.0, 1.0), false, false, false);

Sphere[] spheres = Sphere[](
    //bools are in this order: checkerboard, reflective, visible from inside.
    
    Sphere(vec4(0), 0.1, vec3(0.8, 0.5, 0.5), false, false, false) //This spot reserved for the player sphere
    
    ,Sphere(normalize(vec4(1., 0., 0., 0.)),  0.1, vec3(1.0, 1.0, 1.0), true, false, true)
    ,Sphere(normalize(vec4(1., 0.5, 0., 0.)), 0.1, vec3(0.0, 0.0, 0.0), false, true, true)
    ,Sphere(normalize(vec4(1., -0.5, 0., 0.)),  0.1, vec3(1.0, 0.0, 0.0), true, false, true)
    ,Sphere(normalize(vec4(1., 0., 0.5, 0.)), 0.1, vec3(1.0, 0.0, 1.0), true, false, true)
    ,Sphere(normalize(vec4(1., 0., -0.5, 0.)),  0.1, vec3(0.0, 1.0, 0.0), true, false, true)
    ,Sphere(normalize(vec4(1., 0., 0., 0.5)), 0.1, vec3(1.0, 1.0, 0.0), true, false, true)
    ,Sphere(normalize(vec4(1., 0., 0., -0.5)),  0.1, vec3(0.0, 0.0, 1.0), true, false, true)
    
    ,lightObject
    
    //almost-plane at the bottom
    ,Sphere(normalize(vec4(0.0, 0.0, 0.0, -1.)),  (PI/2.0) - 0.15, vec3(0.4, 0.2, 0.9), true, false, true)
);


////////////////////////// CORE RENDERING LOGIC ///////////////////////////

Hit FindClosestHit(Ray ray, out int hitObjectIndex)
{
    Hit nearest = HitWithoutReflection(false, 99999999999999., vec4(0), BACKGROUND_COLOR);
    hitObjectIndex = -1;

    //Iterate over spheres
    int startingPoint = USER_SPHERE_VISIBLE ? 0 : 1;
    for(int i = startingPoint; i < spheres.length(); i++)
    {            
        Hit sphereHit = SphereHit(spheres[i], ray);
        if(sphereHit.isHit && sphereHit.dist < nearest.dist)
        {
            nearest = sphereHit;
            hitObjectIndex = i;
        }
    }
    
    return nearest;
}

float CalculateDiffuseLightingAndShadows(vec4 hitPos, Hit nearest, int hitObjectIndex)
{
    float lightAmnt;
    if(spheres[hitObjectIndex] == lightObject)
    {
        lightAmnt = 1.0;
    }     
    else if(PointsAreEqualOrOpposite(hitPos, LIGHT_POSITION))
    {
        if(dot(hitPos, LIGHT_POSITION) == 1.0)
        {
            lightAmnt = 1.0;
        }
        else
        {
            //TODO
            
            //It's basically impossible to calclate this in any reasonable timeframe, so we'll just 
            //call it 1.0 since that's what it will most likely be.
            lightAmnt = 1.0;
        }
    }
    else
    {
        vec4 lightRayDirAtHitPoint = -normalize(LIGHT_POSITION - Project(LIGHT_POSITION, hitPos));
        float nearPathDotProduct = dot(-lightRayDirAtHitPoint, nearest.normal);

        Ray lightRayWithPossibilityOfHitting;
        float hitDotProduct;
        if(nearPathDotProduct > 0.0)
        {
            lightRayWithPossibilityOfHitting = RayFromAToB(LIGHT_POSITION, hitPos);
            hitDotProduct = nearPathDotProduct;
        }
        else if(nearPathDotProduct < 0.0)
        {
            Ray closeRay = RayFromAToB(LIGHT_POSITION, hitPos);
            closeRay.direction = -closeRay.direction;
            lightRayWithPossibilityOfHitting = closeRay;

            hitDotProduct = -nearPathDotProduct;
        }
        else
        {
            // angle is exactly 90deg so it's not lit at all
            lightAmnt = 0.0;
        }

        int lightHitObjectIndex;
        Hit firstHit = FindClosestHit(lightRayWithPossibilityOfHitting, lightHitObjectIndex);

        //TODO: this only works for convex objects - if concave objects are added this code will need to be updated 
        if(lightHitObjectIndex == hitObjectIndex)
        {
            //Nothing in between!

            //TODO: may want to have sanity check here, compare adjusted geodesic distance to firsthit dist
            //float dist = GeodesicDistance(lightPos, hitPos);
            float dist = firstHit.dist;
            lightAmnt = min(1.0, LIGHT_INTENSITY / (sin(dist) * sin(dist)));
            lightAmnt *= clamp(hitDotProduct, 0.0, 1.0);
        }
        else
        {
            lightAmnt = 0.0;
        }          
    }
    return lightAmnt;
}

vec3 RayColor(Ray ray)
{
    vec3[REFLECTION_COUNT+1] colors;
    
    for(int i = 0; i < colors.length(); i++)
    {
        //So we can detect non-reflective surfaces.
        colors[i] = vec3(-1);
    }
    
    for(int reflections = 0; reflections <= REFLECTION_COUNT; reflections++)
    {
        int hitObjectIndex;
        Hit nearest = FindClosestHit(ray, hitObjectIndex);
        
        if(!nearest.isHit)
        {
            colors[reflections] = BACKGROUND_COLOR;
            break;
        }

        float lightAmnt = 1.0;
        if(LIGHTING_ENABLED)
        {
            vec4 hitPos = PointAlongRay(ray, nearest.dist);
            lightAmnt = CalculateDiffuseLightingAndShadows(hitPos, nearest, hitObjectIndex);
            lightAmnt = min(1.0, lightAmnt + AMBIENT_LIGHT);
        }
             
        colors[reflections] = nearest.color * lightAmnt;

        if(nearest.hasReflection)
        {
            ray = nearest.reflectedRay;           
        }
        else
        {
            break;
        }
    }
    
    
    vec3 curColor = colors[REFLECTION_COUNT];
    for(int j = REFLECTION_COUNT - 1; j >= 0; j--)
    {
        if(curColor == vec3(-1))
        {
            curColor = colors[j];
        }
        else
        {
            curColor = mix(colors[j], curColor, REFLECTANCE);
        }
    }
    
    return curColor;
}

vec4 ColorAt(vec2 pixelCoord)
{
    //Generate ray
    vec2 adjPixel = pixelCoord - iResolution.xy/2.;
    
    //TODO: do this more accurately (or check that it's accurate)
    vec4 rayDir = normalize(userForwardDir + (adjPixel.x/zoom * userRightDir) + (adjPixel.y/zoom * userUpDir));

    Ray ray = Ray(normalize(userPos), normalize(rayDir));
    
    if(USER_SPHERE_VISIBLE)
    {
        spheres[0].center = ray.origin;
    }

    return vec4(RayColor(ray), 1.0);
}

void main()
{
    zoom *= iResolution.x / 1000.;
    
    fragColor = vec4(0.0);
    
    //antialiasing
    for(float x = 0.; x < 1.; x += 1. / float(AA_AMOUNT))
    {
        for(float y = 0.; y < 1.; y += 1. / float(AA_AMOUNT))
        {    
            fragColor += ColorAt(gl_FragCoord.xy + vec2(x, y));    
        }
    }
    fragColor /= float(AA_AMOUNT * AA_AMOUNT);
    
    if(debug_overrideColor)
    {
        fragColor = vec4(1., 0., 1., 1.);
    }
}