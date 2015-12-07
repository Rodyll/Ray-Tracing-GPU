#version 430 core

//TODO: Perhaps to change for constant values to share it with the app
#define MAX_NB_QUADRICS 10
#define MAX_NB_TRIANGLES 10
#define MAX_NB_PLANES 10
#define MAX_NB_LIGHTS 10

#define EPSILON 1.0e-2

#define QUADRIC 0
#define TRIANGLE 1
#define PLANE 2


///Structures
struct Light {
	vec4 m_PositionLight;	// vec3 in C class
	vec4 m_CouleurLight;	// vec3 in C class
	float m_IntensiteLight;
};

struct Surface {
	vec4	m_Couleur;		//only vec3 in CCouleur
	float  m_CoeffAmbient;
	float  m_CoeffDiffus;
	float  m_CoeffSpeculaire;
	float  m_CoeffShininess;
	float  m_CoeffReflexion;
	float  m_CoeffRefraction;
	float  m_IndiceDeRefraction;
};

struct Quadrique {
	Surface m_surface;
	vec4 m_quadrique;	// vec3 in C class
	vec4 m_lineaire;	// vec3 in C class
	vec4 m_mixte;		// vec3 in C class
	float m_cst;
}; 

struct Triangle {
	Surface m_surface;
	vec4 m_points[3];	// vec3 in C class
	vec4 m_normal;		// vec3 in C class
};

struct Plan {
	Surface m_surface;
	vec4 m_normal;		// vec3 in C class
	float m_cst;
};

struct Rayon
{
	vec3 m_origine;
	vec3 m_direction;
	float m_indiceRefraction;
	float m_energie;
	float m_nbRebonds;
};

struct Intersection 
{
	int m_surfaceType;
	int m_surfaceIndex;
	float m_distance;
	vec3 m_normal;
};



//Outputs 
//TODO : (test writeonly)
layout(rgba32f, binding = 0) uniform restrict image2D renderedImage;


//Inputs
layout(std140, binding = 1) uniform General 
{
	vec4 cameraPosition;		// vec3 in C class
	layout (row_major) mat4 cameraOrientation;

	float halfWidth;
	float halfHeight;

	float invResWidth;
	float invResHeight;
	//
	vec4 couleurArrierePlan; // vec3 in C class
};


//TODO : change to SSBO and std430 layout for more space and better layout
//TODO : change to dynamic arrays (if possible with SSBO)?
//TODO : set SSBO readonly for safety
layout(std140, binding = 2) uniform SceneData 
{
	Quadrique quadrics[MAX_NB_QUADRICS];
	Triangle triangles[MAX_NB_TRIANGLES];
	Plan planes[MAX_NB_PLANES];
	Light lights[MAX_NB_LIGHTS];
	int nbQuadrics;
	int nbTriangles;
	int nbPlanes;
	int nbLights;
};



///Method declarations
vec3 ObtenirCouleur(in Rayon rayon );
vec3 ObtenirCouleurSurIntersection( in Rayon rayon, in Intersection intersection );
vec3 ObtenirFiltreDeSurface( inout Rayon LumiereRayon );

Intersection intersectionQuadric(in Rayon rayon, in Quadrique surface, in int index );
Intersection intersectionTriangle(in Rayon rayon, in Triangle triangle, in int index );
Intersection intersectionPlane(in Rayon rayon, in Plan plan, in int index );

Surface getSurface(int surfaceType, int surfaceIndex);



/******************************************************/
///Method definitions

//two dimensional local workgroup
layout(local_size_x = 16, local_size_y = 16) in;
void main()
{
	Rayon ray;
	ray.m_origine = cameraPosition.xyz;
	ray.m_direction =  vec3( ( 2 * gl_GlobalInvocationID.x * invResWidth  - 1 ) * halfWidth,
							 ( 2 * gl_GlobalInvocationID.y * invResHeight - 1 ) * halfHeight,
							  -1f );

	ray.m_direction = normalize(vec4(ray.m_direction, 0.0) * cameraOrientation).xyz;
	ray.m_energie = 1;
	ray.m_nbRebonds = 0f;
	ray.m_indiceRefraction = 1f;

	vec4 pixelColor = vec4(ObtenirCouleur(ray), 0.0);
	imageStore(renderedImage, ivec2(gl_GlobalInvocationID.xy), pixelColor);
	return;
}


vec3 ObtenirCouleur(in Rayon rayon )
{
	Intersection result;
	result.m_surfaceType = -1;
	result.m_surfaceIndex = -1;

	result.m_distance = -1f;
	result.m_normal = vec3(0.0, 0.0, 0.0);

	//new (need 3 different loops)
	for(int i = 0; i < nbQuadrics; i++)
	{
		Intersection tmp = intersectionQuadric(rayon, quadrics[i], i);
		if( tmp.m_distance > EPSILON && ( tmp.m_distance < result.m_distance || result.m_distance < 0 ) )
		{
			result = tmp;
		}
	}

	for(int i = 0; i < nbTriangles; i++)
	{
		Intersection tmp = intersectionTriangle(rayon, triangles[i], i);
		if( tmp.m_distance > EPSILON && ( tmp.m_distance < result.m_distance || result.m_distance < 0 ) )
		{
			result = tmp;
		}
	}

	for(int i = 0; i < nbPlanes; i++)
	{
		Intersection tmp = intersectionPlane(rayon, planes[i], i);
		if( tmp.m_distance > EPSILON && ( tmp.m_distance < result.m_distance || result.m_distance < 0 ) )
		{
			result = tmp;
		}
	}

	return ( result.m_distance < 0 ) ? couleurArrierePlan.xyz : ObtenirCouleurSurIntersection( rayon, result );
}


vec3 ObtenirCouleurSurIntersection( in Rayon rayon, in Intersection intersection )
{ 
	//new
	Surface surface = getSurface(intersection.m_surfaceType, intersection.m_surfaceIndex);

	vec3  result = surface.m_Couleur.xyz * surface.m_CoeffAmbient;
	vec3 IntersectionPoint = rayon.m_origine + intersection.m_distance * rayon.m_direction;

	// Calculer les contribution colorées des toutes les lumières dans la scène
	vec3 LumiereContributions = vec3(0.0,0.0,0.0);
	Rayon LumiereRayon;
	
	for( int i = 0; i < nbLights; i++)
	{
		// Initialise le rayon de lumière (ou rayon d'ombre)
		LumiereRayon.m_origine = IntersectionPoint;
		LumiereRayon.m_direction = lights[i].m_PositionLight.xyz - IntersectionPoint;
		LumiereRayon.m_energie = 1;
		LumiereRayon.m_indiceRefraction = 1;

		if( dot( LumiereRayon.m_direction, intersection.m_normal ) > 0 )
		{
			// Obtenir la couleur à partir de la lumière
			vec3 Filter         = ObtenirFiltreDeSurface( LumiereRayon );
			vec3 LumiereCouleur = lights[i].m_CouleurLight.xyz * Filter;

			// Ajouter la contribution de Gouraud
			float GouraudFactor = lights[i].m_IntensiteLight * surface.m_CoeffDiffus *
				dot( intersection.m_normal, LumiereRayon.m_direction );
			result += surface.m_Couleur.xyz * GouraudFactor * LumiereCouleur;

			// Ajouter la contribution de Phong
			vec3 ReflectedRayon = reflect( LumiereRayon.m_direction, intersection.m_normal );
			float ProdScal = dot( ReflectedRayon, rayon.m_direction );
			if( ProdScal > 0 )
			{
				float PhongFactor = lights[i].m_IntensiteLight * surface.m_CoeffSpeculaire *
					pow( ProdScal, surface.m_CoeffShininess);
				result += ( PhongFactor * LumiereCouleur );
			}
		}
	}

	////TODO: integrate this feature to the rendering (need to add more uniform values and to break recursion)

	//// Effectuer les réflexions de rayon
	//float ReflectedRayonEnergy = surface.m_CoeffReflexion * rayon.m_energie;
	//if(  ReflectedRayonEnergy > m_EnergieMinRayon && rayon.m_nbRebonds < m_NbRebondsMax )
	//{
	//	Rayon ReflectedRayon;
	//	ReflectedRayon.m_direction = reflect( rayon.m_direction, intersection.m_normal );
	//	ReflectedRayon.m_origine = IntersectionPoint;
	//	ReflectedRayon.m_energie = ReflectedRayonEnergy;
	//	ReflectedRayon.m_nbRebonds = rayon.m_nbRebonds + 1;

	//	result += ObtenirCouleur( ReflectedRayon ) * surface.m_CoeffReflexion;
	//}

	//// Effectuer les réfractions de rayon
	//float RefractedRayonEnergy = surface.m_CoeffRefraction * rayon.m_energie;
	//if( RefractedRayonEnergy > m_EnergieMinRayon && rayon.m_nbRebonds < m_NbRebondsMax )
	//{
	//	//TODO : need to be a "float" for 'refract' function
	//	float IndiceRefractionRatio;
	//	Rayon RefractedRayon;
	//	vec3 SurfaceNormal = intersection.m_normal;

	//	if( rayon.m_indiceRefraction == surface.m_IndiceDeRefraction )
	//	{
	//		// de l'intérieur, vers l'extérieur...
	//		RefractedRayon.m_indiceRefraction = m_IndiceRefractionScene;
	//		IndiceRefractionRatio = surface.m_IndiceDeRefraction / m_IndiceRefractionScene;
	//		SurfaceNormal = -SurfaceNormal;
	//	}
	//	else
	//	{
	//		// de l'extérieur, vers l'intérieur...
	//		RefractedRayon.m_indiceRefraction = surface.m_IndiceDeRefraction;
	//		IndiceRefractionRatio = m_IndiceRefractionScene / surface.m_IndiceDeRefraction;
	//	}

	//	//TODO: la direction du vecteur avec refraction
	//	RefractedRayon.m_origine = IntersectionPoint;
	//	RefractedRayon.m_energie = RefractedRayonEnergy;
	//	RefractedRayon.m_nbRebonds = rayon.m_nbRebonds + 1;

	//	RefractedRayon.m_direction = refract(rayon.m_direction, SurfaceNormal, IndiceRefractionRatio);

	//	//A decommenter apres ajustement de la direction!!
	//	result += ObtenirCouleur( RefractedRayon ) * surface.m_CoeffRefraction;
	//}

	return result;
}


vec3 ObtenirFiltreDeSurface( inout Rayon LumiereRayon )
{
	vec3 Filter = vec3(1.0f, 1.0f, 1.0f);
	Intersection LumiereIntersection;

	float Distance = length( LumiereRayon.m_direction );
	LumiereRayon.m_direction = vec3(LumiereRayon.m_direction / Distance);


	for(int i = 0; i < nbQuadrics; i++)
	{
		LumiereIntersection = intersectionQuadric(LumiereRayon, quadrics[i], i);

		if (LumiereIntersection.m_distance > EPSILON &&
			LumiereIntersection.m_distance < Distance)
		{
			Filter *= quadrics[LumiereIntersection.m_surfaceIndex].m_surface.m_Couleur.xyz *
			quadrics[LumiereIntersection.m_surfaceIndex].m_surface.m_CoeffRefraction;
		}
	}

	for(int i = 0; i < nbTriangles; i++)
	{
		LumiereIntersection = intersectionTriangle(LumiereRayon, triangles[i], i);

		if (LumiereIntersection.m_distance > EPSILON &&
			LumiereIntersection.m_distance < Distance)
		{
			Filter *= triangles[LumiereIntersection.m_surfaceIndex].m_surface.m_Couleur.xyz *
			triangles[LumiereIntersection.m_surfaceIndex].m_surface.m_CoeffRefraction;
		}
	}

	for(int i = 0; i < nbPlanes; i++)
	{
		LumiereIntersection = intersectionPlane(LumiereRayon, planes[i], i);

		if (LumiereIntersection.m_distance > EPSILON &&
			LumiereIntersection.m_distance < Distance)
		{
			Filter *= planes[LumiereIntersection.m_surfaceIndex].m_surface.m_Couleur.xyz *
			planes[LumiereIntersection.m_surfaceIndex].m_surface.m_CoeffRefraction;
		}
	}

	return Filter;
}


Intersection intersectionQuadric(in Rayon rayon, in Quadrique surface, in int index )
{
	// algorithme d'intersection tiré de ... 
	// Eric Haines, Paul Heckbert "An Introduction to Rayon Tracing",
	// Academic Press, Edited by Andrw S. Glassner, pp.68-73 & 288-293
	Intersection result;
	result.m_surfaceType = -1;
	result.m_surfaceIndex = -1;
	result.m_distance = -1.0f;
	result.m_normal = vec3(0.0, 0.0, 0.0);


	vec3 rayonDirection = rayon.m_direction.xyz;
	vec3 rayonOrigin    = rayon.m_origine.xyz;

	float ACoeff = rayonDirection.x * ( surface.m_quadrique.x * rayonDirection.x   +
											 surface.m_mixte.z       * rayonDirection.y   +
											 surface.m_mixte.y       * rayonDirection.z ) +
						rayonDirection.y * ( surface.m_quadrique.y * rayonDirection.y   +
											 surface.m_mixte.x       * rayonDirection.z ) +
						rayonDirection.z * ( surface.m_quadrique.z * rayonDirection.z );

	float BCoeff = rayonDirection.x * ( surface.m_quadrique.x * rayonOrigin.x + 
		0.5	*   ( surface.m_mixte.z * rayonOrigin.y + surface.m_mixte.y * rayonOrigin.z + surface.m_lineaire.x ) ) +
						rayonDirection.y * ( surface.m_quadrique.y * rayonOrigin.y + 
		0.5  *  ( surface.m_mixte.z * rayonOrigin.x + surface.m_mixte.x * rayonOrigin.z + surface.m_lineaire.y ) ) +
						rayonDirection.z * ( surface.m_quadrique.z * rayonOrigin.z + 
		0.5 *  ( surface.m_mixte.y * rayonOrigin.x + surface.m_mixte.x * rayonOrigin.y + surface.m_lineaire.z ) );

	float CCoeff = rayonOrigin.x * ( surface.m_quadrique.x * rayonOrigin.x   +
										  surface.m_mixte.z       * rayonOrigin.y   +
										  surface.m_mixte.y       * rayonOrigin.z   +
										  surface.m_lineaire.x                    ) +
						rayonOrigin.y * ( surface.m_quadrique.y * rayonOrigin.y   +
										  surface.m_mixte.x       * rayonOrigin.z   +
										  surface.m_lineaire.y                    ) +
						rayonOrigin.z * ( surface.m_quadrique.z * rayonOrigin.z   +
										  surface.m_lineaire.z                    ) +
						surface.m_cst;

	
	if( ACoeff != 0.0 )
	{
		float Ka    = -BCoeff / ACoeff;
		float Kb    =  CCoeff / ACoeff;
		float Delta = Ka * Ka - Kb;

		if( Delta > 0 )
		{
			Delta   = sqrt( Delta );
			float T0 = Ka - Delta;
			float T1 = Ka + Delta;

			float Distance = min( T0, T1 );
			if( Distance < EPSILON )
				Distance = max( T0, T1 );
			
			if( !( Distance < 0 ) )
			{
				result.m_distance = Distance;

				//ADDED
				result.m_surfaceType = QUADRIC;
				result.m_surfaceIndex = index;
				//

				// Calcule la normale de surface
				vec3 HitPt = rayonOrigin + Distance * rayonDirection;
				
				vec3 Normal;
				Normal.x = 2.0f * surface.m_quadrique.x * HitPt.x +
						   surface.m_mixte.y * HitPt.z                            +
						   surface.m_mixte.z * HitPt.y                            +
						   surface.m_lineaire.x;

				Normal.y = 2.0f * surface.m_quadrique.y * HitPt.y +
						   surface.m_mixte.x * HitPt.z                            +
						   surface.m_mixte.z * HitPt.x                            +
						   surface.m_lineaire.y;

				Normal.z = 2.0f * surface.m_quadrique.z * HitPt.z +
						   surface.m_mixte.x * HitPt.y                            +
						   surface.m_mixte.y * HitPt.x                            +
						   surface.m_lineaire.z;

				result.m_normal = normalize( Normal );
			}
		}
	}
	else
	{
		result.m_surfaceType = QUADRIC;
		result.m_surfaceIndex = index;

		result.m_distance = -0.5f * ( CCoeff / BCoeff );
		result.m_normal = normalize(surface.m_lineaire.xyz);
	}

	return result;
}


Intersection intersectionTriangle(in Rayon rayon, in Triangle triangle, in int index )
{
	Intersection result;
	result.m_surfaceType = -1;
	result.m_surfaceIndex = -1;
	result.m_distance = -1.0f;
	result.m_normal = vec3(0.0, 0.0, 0.0);

	vec3 edge1 = triangle.m_points[ 1 ].xyz - triangle.m_points[ 0 ].xyz;
	vec3 edge2 = triangle.m_points[ 2 ].xyz - triangle.m_points[ 0 ].xyz;

	// Commencer par le calcul du déterminant - aussi utilisé pour calculer U
	vec3 VecP = cross(rayon.m_direction, edge2 );
	// Si le déterminant est près de 0, le rayon se trouve dans le PLAN du triangle
	float Det  = dot( edge1, VecP );

	if( abs( Det ) < EPSILON )
		return result;
	else
	{
		float InvDet = 1.0f / Det;
		
		// Calculer la distance entre point0 et l'origine du rayon
		vec3 VecS = rayon.m_origine - triangle.m_points[ 0 ].xyz;
		// Calculer le paramètre U pour tester les frontières
		float u = dot( VecS, VecP ) * InvDet;
		if ( u < 0 || u > 1 )
			return result;
		else
		{
			vec3 VecQ = cross( VecS, edge1 );

			// Calculer le paramètre V pour tester les frontières
			float v = dot( rayon.m_direction, VecQ ) * InvDet;
			if( v < 0 || u + v > 1 )
				return result;
			else
			{
				//result.m_surfaceIntersect = triangle;
				result.m_surfaceType = TRIANGLE;
				result.m_surfaceIndex = index;
				result.m_distance = dot( edge2, VecQ ) * InvDet;
				result.m_normal = triangle.m_normal.xyz;
			}
		}
	}

	return result;
}


Intersection intersectionPlane(in Rayon rayon, in Plan plan, in int index )
{
	//new
	Intersection result;
	result.m_distance = -1.0f;
	result.m_normal = vec3(0.0, 0.0, 0.0);
	result.m_surfaceType = -1;
	result.m_surfaceIndex = -1;

	// From http://www.siggraph.org/education/materials/HyperGraph/raytrace/rayplane_intersection.htm

	float Vd = dot( plan.m_normal.xyz, rayon.m_direction );

	if( abs( Vd ) > EPSILON )
	{
		result.m_surfaceType = PLANE;
		result.m_surfaceIndex = index;
		result.m_distance = -( dot( plan.m_normal.xyz, rayon.m_origine ) + plan.m_cst ) / Vd;
		result.m_normal = plan.m_normal.xyz;
	}
	
	return result;
}


Surface getSurface(int surfaceType, int surfaceIndex)
{
	Surface result; 
	switch(surfaceType)
	{
		case QUADRIC :
			result = quadrics[surfaceIndex].m_surface;
		break;
		case TRIANGLE :
			result = triangles[surfaceIndex].m_surface;
		break;
		case PLANE :
			result = planes[surfaceIndex].m_surface;
		break;
	}
	return result;
}