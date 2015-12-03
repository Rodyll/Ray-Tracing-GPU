#version 430 core


//Perhaps to change for constant values to share it with the app
#define MAX_NB_QUADRICS 10
#define MAX_NB_TRIANGLES 10
#define MAX_NB_PLANES 10
#define MAX_NB_LIGHTS 10

#define EPSILON 1.0e-6


#define QUADRIC 0
#define TRIANGLE 1
#define PLANE 2
///inputs





///Structures
struct Light {
	vec4 m_PositionLight;
	vec4 m_CouleurLight;
	float m_IntensiteLight;
};

struct Surface {
	vec4	m_Couleur;
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
	vec4 m_quadrique;
	vec4 m_lineaire;
	vec4 m_mixte;
	float m_cst;
}; 

struct Triangle {
	Surface m_surface;
	vec4 m_points[3];
	vec4 m_normal;
};

struct Plan {
	Surface m_surface;
	vec4 m_normal;
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
	vec4 m_normal;
};


///Uniforms blocks

//TODO : 
//	- reHauteur et resLargeur ?????? PAS SUR

///QUESTION : Or buffer block ??
layout(std140, binding = 2) uniform General 
{
	vec4 cameraPosition;
	mat4 cameraOrientation;
	

	//TODO : Not necessary, to remove (remove also from Scene.cpp)
	//int m_ResLargeur;
	//int m_ResHauteur;





	float halfWidth;
	float halfHeight;

	float invResWidth;
	float invResHeight;
	//

	vec4 couleurArrierePlan;
};

///AUTRE SOLUTION POSSIBLE
//layout (std...) uniform Quadrique {
//	vec3 m_quadrique;
//	vec3 surface.m_lineaire;
//	vec3 surface.m_mixte;
//	float surface.m_cst;
//}quadrics[]; 



//Not enough place if not in uniform block
//TODO : change to SSBO and std430 layout for more space and better layout
//TODO : set SSBO readonly for safety
layout(std140, binding = 0) uniform SceneData 
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


//output 
//TODO : (test writeonly)


layout(rgba32f, binding = 1) uniform restrict image2D renderedImage;


//buffer ssbo
//{
//	float[] infoPixel;
//};

//in/out
//	- infoPixel
//

//out	int infoPixel[]; //better with sampler2D ?

//shared Sampler2D result; // Shared??



///Method declaration
vec4 ObtenirCouleur(in Rayon rayon );
vec4 ObtenirCouleurSurIntersection( in Rayon rayon, in Intersection intersection );
vec4 ObtenirFiltreDeSurface( inout Rayon LumiereRayon );

Intersection intersectionQuadric(in Rayon rayon, in Quadrique surface, in int index );
Intersection intersectionTriangle(in Rayon rayon, in Triangle triangle, in int index );
Intersection intersectionPlane(in Rayon rayon, in Plan plan, in int index );

Surface getSurface(int surfaceType, int surfaceIndex);



/******************************************************/
///Methods definition

//two dimensional local workgroup


layout(local_size_x = 16, local_size_y = 16) in;
void main()
{
	//...

	///DEBUG

	//General variables
	//cameraPosition = vec4(10.0, 11.0, 12.0, 0.0);
	//cameraOrientation = mat4(cameraPosition, cameraPosition, cameraPosition, cameraPosition);



	//DEBUG : just for test
	//float test1 = quadrics[0].m_surface.m_CoeffAmbient;
	//vec4 test2 = triangles[0].m_normal;
	//vec4 pixelColor = vec4(1.0,0.0,0.0,0.0);//couleurArrierePlan;
	
	
	//imageStore(renderedImage, ivec2(gl_GlobalInvocationID.xy), pixelColor);
	//


	//first implementation
	Rayon ray;
	ray.m_origine = cameraPosition.xyz; //TODO : cameraPosition unset

	//TODO : to change
	

	ray.m_direction =  vec3( ( 2 * gl_GlobalInvocationID.x * invResWidth  - 1 ) * halfWidth,
		( 2 * gl_GlobalInvocationID.y * invResHeight - 1 ) * halfHeight,
		-1 );

	ray.m_direction = normalize(vec4(ray.m_direction, 0.0) *  cameraOrientation).xyz;
	ray.m_energie = 1;
	ray.m_nbRebonds = 0f;
	ray.m_indiceRefraction = 1f;

	vec4 pixelColor = ObtenirCouleur(ray);
	imageStore(renderedImage, ivec2(gl_GlobalInvocationID.xy), pixelColor);

}

///////////////////////////////////////////////////////////////////////////////
///  private constant  ObtenirCouleur \n
///  Description : Obtenir la couleur du pixel pour un rayon donné
///
///  @param [in]       Rayon const CRayon &    Le rayon à tester
///
///  @return const CCouleur La couleur du pixel
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
vec4 ObtenirCouleur(in Rayon rayon )
{
	Intersection result;
	result.m_surfaceType = -1;
	result.m_surfaceIndex = -1;

	//result.m_intersect = false;
	result.m_distance = -1f;
	result.m_normal = vec4(0.0, 0.0, 0.0, 0.0);

	//new (need 3 different loops
	int i = 0;
	//if(nbTriangles == 0)
	//{
	//	return vec4(1.0,0.0,0.0,0.0);
	//}
	
	
	if( cameraPosition == vec4(0.0,0.0,0.0,0.0))
	{
		return vec4(0.0,1.0,0.0,0.0);
	}
	//else if(nbQuadrics == 8)
	//{
	//	return vec4(0.0,0.0,1.0,0.0);
	//}

	for(i = 0; i < nbQuadrics; i++)
	{
		//return vec4(1.0,0.0,0.0,0.0);

		//Quadric current = SceneData.quadrics[i];
		Intersection tmp = intersectionQuadric(rayon, quadrics[i], i);
		if( tmp.m_distance > EPSILON && ( tmp.m_distance < result.m_distance || result.m_distance < 0 ) )
		{
			result = tmp;

		}
	}
	//return vec4(0.0,1.0,0.0,0.0);



	for(int i = 0; i < nbTriangles; i++)
	{
		//Triangle current = triangles[i];
		Intersection tmp = intersectionTriangle(rayon, triangles[i], i);
		if( tmp.m_distance > EPSILON && ( tmp.m_distance < result.m_distance || result.m_distance < 0 ) )
			result = tmp;
	}

	for(int i = 0; i < nbPlanes; i++)
	{
		//Plane current = planes[i];
		Intersection tmp = intersectionPlane(rayon, planes[i], i);
		if( tmp.m_distance > EPSILON && ( tmp.m_distance < result.m_distance || result.m_distance < 0 ) )
			result = tmp;
	}

	//TODO : add "m_intersect == true"
	return ( result.m_distance < 0 ) ? couleurArrierePlan : ObtenirCouleurSurIntersection( rayon, result );
	//


	////old
	//CIntersection Result;
	//CIntersection Tmp;

	//for( SurfaceIterator aSurface = m_Surfaces.begin(); aSurface != m_Surfaces.end(); aSurface++ )
	//{
	//	Tmp = ( *aSurface )->Intersection( Rayon );
	//	if( Tmp.ObtenirDistance() > EPSILON && ( Tmp.ObtenirDistance() < Result.ObtenirDistance() || Result.ObtenirDistance() < 0 ) )
	//		Result = Tmp;
	//}

	//// S'il n'y aucune intersection, retourner la couleur de l'arrière-plan
	//// Sinon, retourner la couleur à l'intersection
	//return ( Result.ObtenirDistance() < 0 ) ? m_CouleurArrierePlan : ObtenirCouleurSurIntersection( Rayon, Result );
	//
}

///////////////////////////////////////////////////////////////////////////////
///  private constant  ObtenirCouleurSurIntersection \n
///  Description : Obtient la couleur à un point d'intersection en particulier
///                Calcule les contributions colorées de toutes les lumières, avec
///                les modèles de Phong et de Gouraud. Aussi, dépendemment des
///                propriétés de la surface en intersection, on réfléchi ou in 
///                réfracte le rayon courant.
///                current ray.
///
///  @param [in]       Rayon const CRayon &    Le rayon à tester
///  @param [in]       Intersection const Scene::CIntersection &    L'ntersection spécifiée
///
///  @return const CCouleur La couleur à l'intersection donnée
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
vec4 ObtenirCouleurSurIntersection( in Rayon rayon, in Intersection intersection )
{
	//new
	Surface surface = getSurface(intersection.m_surfaceType, intersection.m_surfaceIndex);
	
	vec4  result = surface.m_Couleur * surface.m_CoeffAmbient;
	vec3 IntersectionPoint = rayon.m_origine + intersection.m_distance * rayon.m_direction;

	// Calculer les contribution colorées des toutes les lumières dans la scène
	vec3 LumiereContributions = vec3(0.0,0.0,0.0);
	Rayon LumiereRayon;
	
	for( uint i = 0; i < nbLights; i++)
	{
		// Initialise le rayon de lumière (ou rayon d'ombre)
		LumiereRayon.m_origine = IntersectionPoint;
		LumiereRayon.m_direction = lights[i].m_PositionLight.xyz - IntersectionPoint;
		LumiereRayon.m_energie = 1;
		LumiereRayon.m_indiceRefraction = 1;

		if( dot( LumiereRayon.m_direction, intersection.m_normal.xyz ) > 0 )
		{
			// Obtenir la couleur à partir de la lumière
			vec4 Filter         = ObtenirFiltreDeSurface( LumiereRayon );
			vec4 LumiereCouleur = lights[i].m_CouleurLight * Filter;

			// Ajouter la contribution de Gouraud
			float GouraudFactor = lights[i].m_IntensiteLight * surface.m_CoeffDiffus *
				dot( intersection.m_normal.xyz, LumiereRayon.m_direction );
			result += surface.m_Couleur * GouraudFactor * LumiereCouleur;

			// Ajouter la contribution de Phong
			vec3 ReflectedRayon = reflect( LumiereRayon.m_direction, intersection.m_normal.xyz );
			float ProdScal = dot( ReflectedRayon, rayon.m_direction );
			if( ProdScal > 0 )
			{
				float PhongFactor = lights[i].m_IntensiteLight * surface.m_CoeffSpeculaire *
					pow( ProdScal, surface.m_CoeffShininess);
				result += ( PhongFactor * LumiereCouleur );
			}
		}
	}

	//// Effectuer les réflexions de rayon
	//float ReflectedRayonEnergy = surface.m_CoeffReflexion * rayon.m_energie;
	//if(  ReflectedRayonEnergy > m_EnergieMinRayon && rayon.m_nbRebonds < m_NbRebondsMax )
	//{
	//	Rayon ReflectedRayon;
	//	ReflectedRayon.m_direction = reflect( rayon.m_direction, intersection.m_normal.xyz );
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
	//	vec3 SurfaceNormal = intersection.m_normal.xyz;

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
	//







	//old
	//CCouleur  Result = Intersection.ObtenirSurface()->ObtenirCouleur() * Intersection.ObtenirSurface()->ObtenirCoeffAmbiant();
	//CVecteur3 IntersectionPoint = Rayon.ObtenirOrigine()+ Intersection.ObtenirDistance() * Rayon.ObtenirDirection();

	//// Calculer les contribution colorées des toutes les lumières dans la scène
	//CCouleur LumiereContributions = CCouleur::NOIR;
	//CRayon   LumiereRayon;
	//for( LumiereIterator uneLumiere = m_Lumieres.begin(); uneLumiere != m_Lumieres.end(); uneLumiere++ )
	//{
	//	// Initialise le rayon de lumière (ou rayon d'ombre)
	//	LumiereRayon.AjusterOrigine( IntersectionPoint );
	//	LumiereRayon.AjusterDirection( ( *uneLumiere )->GetPosition() - IntersectionPoint );
	//	LumiereRayon.AjusterEnergie( 1 );
	//	LumiereRayon.AjusterIndiceRefraction( 1 );

	//	if( CVecteur3::ProdScal( LumiereRayon.ObtenirDirection(), Intersection.ObtenirNormale() ) > 0 )
	//	{
	//		// Obtenir la couleur à partir de la lumière
	//		CCouleur Filter         = ObtenirFiltreDeSurface( LumiereRayon );
	//		CCouleur LumiereCouleur = ( *uneLumiere )->ObtenirCouleur() * Filter;

	//		// Ajouter la contribution de Gouraud
	//		REAL GouraudFactor = ( *uneLumiere )->GetIntensity() * Intersection.ObtenirSurface()->ObtenirCoeffDiffus() *
	//			CVecteur3::ProdScal( Intersection.ObtenirNormale(), LumiereRayon.ObtenirDirection() );
	//		Result += Intersection.ObtenirSurface()->ObtenirCouleur() * GouraudFactor * LumiereCouleur;

	//		// Ajouter la contribution de Phong
	//		CVecteur3 ReflectedRayon = CVecteur3::Reflect( LumiereRayon.ObtenirDirection(), Intersection.ObtenirNormale() );
	//		REAL ProdScal = CVecteur3::ProdScal( ReflectedRayon, Rayon.ObtenirDirection() );
	//		if( ProdScal > 0 )
	//		{
	//			REAL PhongFactor = ( *uneLumiere )->GetIntensity() * Intersection.ObtenirSurface()->ObtenirCoeffSpeculaire() *
	//				pow( ProdScal, Intersection.ObtenirSurface()->ObtenirCoeffBrillance() );
	//			Result += ( PhongFactor * LumiereCouleur );
	//		}
	//	}
	//}

	//// Effectuer les réflexions de rayon
	//REAL ReflectedRayonEnergy = Intersection.ObtenirSurface()->ObtenirCoeffReflexion() * Rayon.ObtenirEnergie();
	//if(  ReflectedRayonEnergy > m_EnergieMinRayon && Rayon.ObtenirNbRebonds() < m_NbRebondsMax )
	//{
	//	CRayon ReflectedRayon;
	//	ReflectedRayon.AjusterDirection( CVecteur3::Reflect( Rayon.ObtenirDirection(), Intersection.ObtenirNormale() ) );
	//	ReflectedRayon.AjusterOrigine( IntersectionPoint );
	//	ReflectedRayon.AjusterEnergie( ReflectedRayonEnergy );
	//	ReflectedRayon.AjusterNbRebonds( Rayon.ObtenirNbRebonds() + 1 );

	//	Result += ObtenirCouleur( ReflectedRayon ) * Intersection.ObtenirSurface()->ObtenirCoeffReflexion();
	//}

	//// Effectuer les réfractions de rayon
	//REAL RefractedRayonEnergy = Intersection.ObtenirSurface()->ObtenirCoeffRefraction() * Rayon.ObtenirEnergie();
	//if( RefractedRayonEnergy > m_EnergieMinRayon && Rayon.ObtenirNbRebonds() < m_NbRebondsMax )
	//{
	//	REAL      IndiceRefractionRatio;
	//	CRayon    RefractedRayon;
	//	CVecteur3 SurfaceNormal = Intersection.ObtenirNormale();

	//	if( Rayon.ObtenirIndiceRefraction() == Intersection.ObtenirSurface()->ObtenirIndiceRefraction() )
	//	{
	//		// de l'intérieur, vers l'extérieur...
	//		RefractedRayon.AjusterIndiceRefraction( m_IndiceRefractionScene );
	//		IndiceRefractionRatio = Intersection.ObtenirSurface()->ObtenirIndiceRefraction() / m_IndiceRefractionScene;
	//		SurfaceNormal = -SurfaceNormal;
	//	}
	//	else
	//	{
	//		// de l'extérieur, vers l'intérieur...
	//		RefractedRayon.AjusterIndiceRefraction( Intersection.ObtenirSurface()->ObtenirIndiceRefraction() );
	//		IndiceRefractionRatio = m_IndiceRefractionScene / Intersection.ObtenirSurface()->ObtenirIndiceRefraction();
	//	}

	//	//TODO: la direction du vecteur avec refraction
	//	RefractedRayon.AjusterOrigine( IntersectionPoint );
	//	RefractedRayon.AjusterEnergie( RefractedRayonEnergy );
	//	RefractedRayon.AjusterNbRebonds( Rayon.ObtenirNbRebonds() + 1 );

	//	RefractedRayon.AjusterDirection(CVecteur3::Refract(Rayon.ObtenirDirection(), SurfaceNormal, IndiceRefractionRatio));

	//	//A decommenter apres ajustement de la direction!!
	//	Result += ObtenirCouleur( RefractedRayon ) * Intersection.ObtenirSurface()->ObtenirCoeffRefraction();
	//}

	//return Result;
}

///////////////////////////////////////////////////////////////////////////////
///  private constant  ObtenirFiltreDeSurface \n
///  Description : Obtenir le filtre du matériau de la surface. Le coefficient de
///                réfraction nous indique du même coup s'il y a transparence de la 
///                surface.
///
///  @param [in, out]  LumiereRayon CRayon &     Le rayon de lumière (ou d'ombre) à tester
///
///  @return const CCouleur Le filtre de couleur
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
vec4 ObtenirFiltreDeSurface( inout Rayon LumiereRayon )
{
	vec4 Filter = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	Intersection LumiereIntersection;

	//TODO : NEED to be a float
	float Distance = length( LumiereRayon.m_direction );
	//TODO : workaround, need to check if there is no drawbacks
	LumiereRayon.m_direction = vec3(LumiereRayon.m_direction / Distance);


	for(int i = 0; i < nbQuadrics; i++)
	{
		LumiereIntersection = intersectionQuadric(LumiereRayon, quadrics[i], i);

		if (LumiereIntersection.m_distance > EPSILON &&
			LumiereIntersection.m_distance < Distance)
			Filter *= quadrics[LumiereIntersection.m_surfaceIndex].m_surface.m_Couleur *
			quadrics[LumiereIntersection.m_surfaceIndex].m_surface.m_CoeffRefraction;
	}

	for(int i = 0; i < nbTriangles; i++)
	{
		LumiereIntersection = intersectionTriangle(LumiereRayon, triangles[i], i);

		if (LumiereIntersection.m_distance > EPSILON &&
			LumiereIntersection.m_distance < Distance)
			Filter *= triangles[LumiereIntersection.m_surfaceIndex].m_surface.m_Couleur *
			triangles[LumiereIntersection.m_surfaceIndex].m_surface.m_CoeffRefraction;
	}

	for(int i = 0; i < nbPlanes; i++)
	{
		LumiereIntersection = intersectionPlane(LumiereRayon, planes[i], i);

		if (LumiereIntersection.m_distance > EPSILON &&
			LumiereIntersection.m_distance < Distance)
			Filter *= planes[LumiereIntersection.m_surfaceIndex].m_surface.m_Couleur *
			planes[LumiereIntersection.m_surfaceIndex].m_surface.m_CoeffRefraction;
	}

	return Filter;
}





///////////////////////////////////////////////////////////////////////////////
///  public   Intersection \n
///  Description : Effectue l'intersection Rayon/Quadrique
///
///  @param [in]       Rayon const CRayon &    Le rayon à tester
///
///  @return Scene::CIntersection Le résultat de l'ntersection
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
Intersection intersectionQuadric(in Rayon rayon, in Quadrique surface, in int index )
{
	

	// algorithme d'intersection tiré de ... 
	// Eric Haines, Paul Heckbert "An Introduction to Rayon Tracing",
	// Academic Press, Edited by Andrw S. Glassner, pp.68-73 & 288-293
	Intersection result;
	result.m_surfaceType = -1;
	result.m_surfaceIndex = -1;
	result.m_distance = -1.0f;
	result.m_normal = vec4(0.0, 0.0, 0.0, 0.0);


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
				//old
				//Result.AjusterSurface( this );
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

				// TODO : Ceck if it gives the right behaviour, or if we have to change Intersection.m_normal to vec3
				result.m_normal = normalize( vec4(Normal,0.0) );
			}
		}
	}
	else
	{
		result.m_surfaceType = QUADRIC;
		result.m_surfaceIndex = index;
		//Result.AjusterSurface ( this );

		result.m_distance = -0.5f * ( CCoeff / BCoeff );
		result.m_normal = normalize(surface.m_lineaire);
		//Result.AjusterDistance( -RENDRE_REEL( 0.5 ) * ( CCoeff / BCoeff ) );
		//Result.AjusterNormale ( CVecteur3::Normaliser( surface.m_lineaire ) );
	}

	return result;
}


Intersection intersectionTriangle(in Rayon rayon, in Triangle triangle, in int index )
{

	//new
	Intersection result;
	//result.m_surfaceIntersect = surface;
	result.m_surfaceType = -1;
	result.m_surfaceIndex = -1;
	result.m_distance = -1.0f;
	result.m_normal = vec4(0.0, 0.0, 0.0, 0.0);

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
				result.m_normal = triangle.m_normal;
			}
		}
	}

	return result;
	//


	////old
	//CIntersection Result;

	//// Tomas Akenine-Moller and Eric Haines "Real-Time Rendering 2nd Ed." 2002, p.581
	//// http://jgt.akpeters.com/papers/MollerTrumbore97/code.html

	//CVecteur3 Edge1 = m_Pts[ 1 ] - m_Pts[ 0 ];
	//CVecteur3 Edge2 = m_Pts[ 2 ] - m_Pts[ 0 ];

	//// Commencer par le calcul du déterminant - aussi utilisé pour calculer U
	//CVecteur3 VecP = CVecteur3::ProdVect( Rayon.ObtenirDirection(), Edge2 );
	//// Si le déterminant est près de 0, le rayon se trouve dans le PLAN du triangle
	//REAL Det  = CVecteur3::ProdScal( Edge1, VecP );

	//if( Abs<REAL>( Det ) < EPSILON )
	//	return Result;
	//else
	//{
	//	REAL InvDet = RENDRE_REEL( 1.0 ) / Det;
		
	//	// Calculer la distance entre point0 et l'origine du rayon
	//	CVecteur3 VecS = Rayon.ObtenirOrigine() - m_Pts[ 0 ];
	//	// Calculer le paramètre U pour tester les frontières
	//	REAL u = CVecteur3::ProdScal( VecS, VecP ) * InvDet;
	//	if ( u < 0 || u > 1 )
	//		return Result;
	//	else
	//	{
	//		CVecteur3 VecQ = CVecteur3::ProdVect( VecS, Edge1 );

	//		// Calculer le paramètre V pour tester les frontières
	//		REAL v = CVecteur3::ProdScal( Rayon.ObtenirDirection(), VecQ ) * InvDet;
	//		if( v < 0 || u + v > 1 )
	//			return Result;
	//		else
	//		{
	//			Result.AjusterSurface( this );
	//			Result.AjusterDistance( CVecteur3::ProdScal( Edge2, VecQ ) * InvDet );
	//			Result.AjusterNormale( m_Normale );
	//		}
	//	}
	//}

	//return Result;
	////
}


Intersection intersectionPlane(in Rayon rayon, in Plan plan, in int index )
{
	//new
	Intersection result;
	result.m_distance = -1.0f;
	result.m_normal = vec4(0.0, 0.0, 0.0, 0.0);
	result.m_surfaceType = -1;
	result.m_surfaceIndex = -1;

	// From http://www.siggraph.org/education/materials/HyperGraph/raytrace/rayplane_intersection.htm

	float Vd = dot( plan.m_normal.xyz, rayon.m_direction );

	if( abs( Vd ) > EPSILON )
	{
		result.m_surfaceType = PLANE;
		result.m_surfaceIndex = index;
		result.m_distance = -( dot( plan.m_normal.xyz, rayon.m_origine ) + plan.m_cst ) / Vd;
		result.m_normal = plan.m_normal;
	}
	
	return result;
	//


	////old
	//CIntersection Result;

 //   // From http://www.siggraph.org/education/materials/HyperGraph/raytrace/rayplane_intersection.htm

 //   REAL Vd = CVecteur3::ProdScal( m_Normale, Rayon.ObtenirDirection() );

 //   if( Abs<REAL>( Vd ) > EPSILON )
 //   {
 //       Result.AjusterSurface( this );
 //       Result.AjusterDistance( -( CVecteur3::ProdScal( m_Normale, Rayon.ObtenirOrigine() ) + m_Cst ) / Vd );
 //       Result.AjusterNormale( m_Normale );
 //   }
	
 //   return Result;

	////
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
