#version 430 core


//Perhaps to change for constant values to share it with the app
#define MAX_NB_QUADRICS 10
#define MAX_NB_TRIANGLES 10
#define MAX_NB_PLANES 10

///inputs

//TODO : 
//	- surfaces
//	- reHauteur et resLargeur ?????? PAS SUR

///QUESTION : Or buffer block ??
//uniform Uniforms {
//	//uint nbTriangles;
//	//uint nbQuadrics;
//	vec3 cameraPosition; // equal to 'm_Camera.Position'
//	vec3 cameraOrientation; // equal to 'm_Camera.Orientation'
//};

struct Quadrique {
	vec4 m_quadrique;
	vec4 m_lineaire;
	vec4 m_mixte;
	double m_cst;
}; 



///AUTRE SOLUTION POSSIBLE
//layout (std...) uniform Quadrique {
//	vec3 m_quadrique;
//	vec3 m_lineaire;
//	vec3 m_mixte;
//	float m_cst;
//}quadrics[]; 

struct Triangle {
	vec4 m_points[3];
	vec4 m_normal;
};

struct Plan {
	vec4 m_normal;
	double m_cst;
};

//struct Rayon
//{
//	vec3 m_origine;
//	vec3 m_direction;
//	double m_indiceRefraction;
//	double m_energie;
//	double m_nbRebonds;
//};

//Not enough place if not in uniform block
//TODO : change to SSBO and std430 layout for more space and better layout
//TODO : set SSBO readonly for safety
layout(std140, binding = 0) uniform SurfacesData 
{
	Quadrique quadrics[MAX_NB_QUADRICS];
	Triangle triangles[MAX_NB_TRIANGLES];
	Plan planes[MAX_NB_PLANES];
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


//two dimensionnal local workgroup
layout(local_size_x = 16, local_size_y = 16) in;

void main()
{
	//just for test
	double test1 = quadrics[0].m_cst;
	vec4 test2 = triangles[0].m_normal;

	//int arrayIndex = gl_WorkGroupSize.x * glNumWorkGroups.x * gl_GlobalInvocation.y + gl_GlobalInvocation.x;
	//infoPixel[arrayIndex] = 1.0;
	//infoPixel[arrayIndex + 1] = 0.0;
	//infoPixel[arrayIndex + 2] = 0.0;

	imageStore(renderedImage, ivec2(gl_GlobalInvocationID.xy), vec4(1.0, 0.0, 0.0, 1.0));


	//
//	int[3] currentPixel = gl_GlobalInvocationIndex;


//	//old
//	Rayon ray;
//	ray.m_Origine = cameraPosition;

//	//TODO : to change
//	ray.m_Direction =  vec3( ( 2 * PixX * InvResWidth  - 1 ) * HalfW,
//		( 2 * PixY * InvResHeight - 1 ) * HalfH,
//		-1 );

//	ray.m_Direction = normalize(ray.m_Direction *  cameraOrientation);
//	ray.m_Energie = 1;
//	ray.m_nbRebonds = 0f;
//	ray.m_IndiceRefraction = 1f;

//	PixelColor = ObtenirCouleur( Rayon );

//	//PixelIdx = ( PixX + ( PixY * m_ResLargeur ) ) * 3;
//	//m_InfoPixel[ PixelIdx + 0 ] = PixelColor.r; 
//	//m_InfoPixel[ PixelIdx + 1 ] = PixelColor.g;
//	//m_InfoPixel[ PixelIdx + 2 ] = PixelColor.b;

//	currentPixel[ 0 ] = PixelColor.r; 
//	currentPixel[ 1 ] = PixelColor.g;
//	currentPixel[ 2 ] = PixelColor.b;
}

/////////////////////////////////////////////////////////////////////////////////
/////  private constant  ObtenirCouleur \n
/////  Description : Obtenir la couleur du pixel pour un rayon donné
/////
/////  @param [in]       Rayon const CRayon &    Le rayon à tester
/////
/////  @return const CCouleur La couleur du pixel
/////
/////  @author Olivier Dionne 
/////  @date   13/08/2008
/////
/////////////////////////////////////////////////////////////////////////////////
//const CCouleur ObtenirCouleur( const CRayon& Rayon ) const
//{
//	CIntersection Result;
//	CIntersection Tmp;

//	for( SurfaceIterator aSurface = m_Surfaces.begin(); aSurface != m_Surfaces.end(); aSurface++ )
//	{
//		Tmp = ( *aSurface )->Intersection( Rayon );
//		if( Tmp.ObtenirDistance() > EPSILON && ( Tmp.ObtenirDistance() < Result.ObtenirDistance() || Result.ObtenirDistance() < 0 ) )
//			Result = Tmp;
//	}

//	// S'il n'y aucune intersection, retourner la couleur de l'arrière-plan
//	// Sinon, retourner la couleur à l'intersection
//	return ( Result.ObtenirDistance() < 0 ) ? m_CouleurArrierePlan : ObtenirCouleurSurIntersection( Rayon, Result );
//}

/////////////////////////////////////////////////////////////////////////////////
/////  private constant  ObtenirCouleurSurIntersection \n
/////  Description : Obtient la couleur à un point d'intersection en particulier
/////                Calcule les contributions colorées de toutes les lumières, avec
/////                les modèles de Phong et de Gouraud. Aussi, dépendemment des
/////                propriétés de la surface en intersection, on réfléchi ou in 
/////                réfracte le rayon courant.
/////                current ray.
/////
/////  @param [in]       Rayon const CRayon &    Le rayon à tester
/////  @param [in]       Intersection const Scene::CIntersection &    L'ntersection spécifiée
/////
/////  @return const CCouleur La couleur à l'intersection donnée
/////
/////  @author Olivier Dionne 
/////  @date   13/08/2008
/////
/////////////////////////////////////////////////////////////////////////////////
//const CCouleur ObtenirCouleurSurIntersection( const CRayon& Rayon, const CIntersection& Intersection ) const
//{
//	CCouleur  Result = Intersection.ObtenirSurface()->ObtenirCouleur() * Intersection.ObtenirSurface()->ObtenirCoeffAmbiant();
//	CVecteur3 IntersectionPoint = Rayon.ObtenirOrigine()+ Intersection.ObtenirDistance() * Rayon.ObtenirDirection();

//	// Calculer les contribution colorées des toutes les lumières dans la scène
//	CCouleur LumiereContributions = CCouleur::NOIR;
//	CRayon   LumiereRayon;
//	for( LumiereIterator uneLumiere = m_Lumieres.begin(); uneLumiere != m_Lumieres.end(); uneLumiere++ )
//	{
//		// Initialise le rayon de lumière (ou rayon d'ombre)
//		LumiereRayon.AjusterOrigine( IntersectionPoint );
//		LumiereRayon.AjusterDirection( ( *uneLumiere )->GetPosition() - IntersectionPoint );
//		LumiereRayon.AjusterEnergie( 1 );
//		LumiereRayon.AjusterIndiceRefraction( 1 );

//		if( CVecteur3::ProdScal( LumiereRayon.ObtenirDirection(), Intersection.ObtenirNormale() ) > 0 )
//		{
//			// Obtenir la couleur à partir de la lumière
//			CCouleur Filter         = ObtenirFiltreDeSurface( LumiereRayon );
//			CCouleur LumiereCouleur = ( *uneLumiere )->ObtenirCouleur() * Filter;

//			// Ajouter la contribution de Gouraud
//			REAL GouraudFactor = ( *uneLumiere )->GetIntensity() * Intersection.ObtenirSurface()->ObtenirCoeffDiffus() *
//				CVecteur3::ProdScal( Intersection.ObtenirNormale(), LumiereRayon.ObtenirDirection() );
//			Result += Intersection.ObtenirSurface()->ObtenirCouleur() * GouraudFactor * LumiereCouleur;

//			// Ajouter la contribution de Phong
//			CVecteur3 ReflectedRayon = CVecteur3::Reflect( LumiereRayon.ObtenirDirection(), Intersection.ObtenirNormale() );
//			REAL ProdScal = CVecteur3::ProdScal( ReflectedRayon, Rayon.ObtenirDirection() );
//			if( ProdScal > 0 )
//			{
//				REAL PhongFactor = ( *uneLumiere )->GetIntensity() * Intersection.ObtenirSurface()->ObtenirCoeffSpeculaire() *
//					pow( ProdScal, Intersection.ObtenirSurface()->ObtenirCoeffBrillance() );
//				Result += ( PhongFactor * LumiereCouleur );
//			}
//		}
//	}

//	// Effectuer les réflexions de rayon
//	REAL ReflectedRayonEnergy = Intersection.ObtenirSurface()->ObtenirCoeffReflexion() * Rayon.ObtenirEnergie();
//	if(  ReflectedRayonEnergy > m_EnergieMinRayon && Rayon.ObtenirNbRebonds() < m_NbRebondsMax )
//	{
//		CRayon ReflectedRayon;
//		ReflectedRayon.AjusterDirection( CVecteur3::Reflect( Rayon.ObtenirDirection(), Intersection.ObtenirNormale() ) );
//		ReflectedRayon.AjusterOrigine( IntersectionPoint );
//		ReflectedRayon.AjusterEnergie( ReflectedRayonEnergy );
//		ReflectedRayon.AjusterNbRebonds( Rayon.ObtenirNbRebonds() + 1 );

//		Result += ObtenirCouleur( ReflectedRayon ) * Intersection.ObtenirSurface()->ObtenirCoeffReflexion();
//	}

//	// Effectuer les réfractions de rayon
//	REAL RefractedRayonEnergy = Intersection.ObtenirSurface()->ObtenirCoeffRefraction() * Rayon.ObtenirEnergie();
//	if( RefractedRayonEnergy > m_EnergieMinRayon && Rayon.ObtenirNbRebonds() < m_NbRebondsMax )
//	{
//		REAL      IndiceRefractionRatio;
//		CRayon    RefractedRayon;
//		CVecteur3 SurfaceNormal = Intersection.ObtenirNormale();

//		if( Rayon.ObtenirIndiceRefraction() == Intersection.ObtenirSurface()->ObtenirIndiceRefraction() )
//		{
//			// de l'intérieur, vers l'extérieur...
//			RefractedRayon.AjusterIndiceRefraction( m_IndiceRefractionScene );
//			IndiceRefractionRatio = Intersection.ObtenirSurface()->ObtenirIndiceRefraction() / m_IndiceRefractionScene;
//			SurfaceNormal = -SurfaceNormal;
//		}
//		else
//		{
//			// de l'extérieur, vers l'intérieur...
//			RefractedRayon.AjusterIndiceRefraction( Intersection.ObtenirSurface()->ObtenirIndiceRefraction() );
//			IndiceRefractionRatio = m_IndiceRefractionScene / Intersection.ObtenirSurface()->ObtenirIndiceRefraction();
//		}

//		//TODO: la direction du vecteur avec refraction
//		RefractedRayon.AjusterOrigine( IntersectionPoint );
//		RefractedRayon.AjusterEnergie( RefractedRayonEnergy );
//		RefractedRayon.AjusterNbRebonds( Rayon.ObtenirNbRebonds() + 1 );

//		RefractedRayon.AjusterDirection(CVecteur3::Refract(Rayon.ObtenirDirection(), SurfaceNormal, IndiceRefractionRatio));

//		//A decommenter apres ajustement de la direction!!
//		Result += ObtenirCouleur( RefractedRayon ) * Intersection.ObtenirSurface()->ObtenirCoeffRefraction();
//	}

//	return Result;
//}

/////////////////////////////////////////////////////////////////////////////////
/////  private constant  ObtenirFiltreDeSurface \n
/////  Description : Obtenir le filtre du matériau de la surface. Le coefficient de
/////                réfraction nous indique du même coup s'il y a transparence de la 
/////                surface.
/////
/////  @param [in, out]  LumiereRayon CRayon &     Le rayon de lumière (ou d'ombre) à tester
/////
/////  @return const CCouleur Le filtre de couleur
/////
/////  @author Olivier Dionne 
/////  @date   13/08/2008
/////
/////////////////////////////////////////////////////////////////////////////////
//const CCouleur ObtenirFiltreDeSurface( CRayon& LumiereRayon ) const
//{
//	CCouleur Filter = CCouleur::BLANC;
//	CIntersection LumiereIntersection;

//	REAL Distance = CVecteur3::Norme( LumiereRayon.ObtenirDirection() );
//	LumiereRayon.AjusterDirection( LumiereRayon.ObtenirDirection() / Distance );

//	for( SurfaceIterator aSurface = m_Surfaces.begin(); aSurface != m_Surfaces.end(); aSurface++ )
//	{
//		LumiereIntersection = ( *aSurface )->Intersection( LumiereRayon );

//		if( LumiereIntersection.ObtenirDistance() > EPSILON &&
//			LumiereIntersection.ObtenirDistance() < Distance )
//			Filter *=  LumiereIntersection.ObtenirSurface()->ObtenirCouleur() *
//			LumiereIntersection.ObtenirSurface()->ObtenirCoeffRefraction();
//	}

//	return Filter;
//}