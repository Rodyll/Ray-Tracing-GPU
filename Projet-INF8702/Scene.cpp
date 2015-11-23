#include "Scene.h"
#include <fstream>
#include "StringUtils.h"
#include "Triangle.h"
#include "Plan.h"
#include "Quadrique.h"

#include "Var.h"
#include "NuanceurCalculProg.h"
using namespace std;
using namespace Scene;


//Perhaps to change for constant values to share it with the GLSL file
#define MAX_NB_QUADRICS 10
#define MAX_NB_TRIANGLES 10
#define MAX_NB_PLANES 10
#define NB_SURFACETYPE 2

#define QUADRICTYPE_SIZE 4
#define TRIANGLETYPE_SIZE 2

enum { QUADRIC, TRIANGLES };



#define STRING_CHECKFIND( Buffer, String ) ( Buffer.find( String ) != string::npos )

const REAL CScene::DIM_FILM_CAM = 0.024;

///////////////////////////////////////////////////////////////////////////////
///  private overloaded constructor  CScene \n
///  Description : Constructeur par défaut
///
///  @return None
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
CScene::CScene(void)
	: m_ResLargeur(0)
	, m_ResHauteur(0)
	, m_InfoPixel(NULL)
	, m_TextureScene(0)
	, m_CouleurArrierePlan(CCouleur::NOIR)
	, m_NbRebondsMax(20)
	, m_EnergieMinRayon(RENDRE_REEL(0.01))
	, m_IndiceRefractionScene(RENDRE_REEL(1.0))
{
	m_Camera.Position = CVecteur3::ZERO;
	m_Camera.PointVise = CVecteur3::ZERO;
	m_Camera.Up = CVecteur3::UNIT_Y;
	m_Camera.Orientation = CMatrice4::IDENTITE;
	m_Camera.Focale = RENDRE_REEL(50.0);
	m_Camera.Angle = 0;
}

///////////////////////////////////////////////////////////////////////////////
///  private destructor  ~CScene \n
///  Description : Destructeur
///
///  @return None
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
CScene::~CScene(void)
{
	Liberer();
}

///////////////////////////////////////////////////////////////////////////////
///  public  AjouterSurface \n
///  Description : Ajoute une surface à la scène
///
///  @param [in]       Surface Scene::ISurface *const Un pointeur constant à la surface
///
///  @return None
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
void CScene::AjouterSurface(ISurface* const Surface)
{
	m_Surfaces.push_back(Surface);
}

///////////////////////////////////////////////////////////////////////////////
///  public  AjouterLumiere \n
///  Description : Ajoute une lumière à la scène
///
///  @param [in]       Lumiere Scene::CLumiere *const    Pointeur constant vers une lumière
///  
///  @return None
///
///  @author Olivier Dionne 
///  @date   13/08/2008   
///
///////////////////////////////////////////////////////////////////////////////
void CScene::AjouterLumiere(CLumiere* const Lumiere)
{
	m_Lumieres.push_back(Lumiere);
}

///////////////////////////////////////////////////////////////////////////////
///  private  Initialize \n
///  Description : Initialise la caméra de scène et les surfaces avant de commencer
///                le lancer de rayons
///
///  @return None
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
void CScene::Initialiser(void)
{
	InitialiserCamera();
	for (SurfaceIterator it = m_Surfaces.begin(); it != m_Surfaces.end(); it++)
		(*it)->Pretraitement();

	m_InfoPixel = new GLfloat[m_ResLargeur * m_ResHauteur * 3];
}

///////////////////////////////////////////////////////////////////////////////
///  public  AjusterResolution \n
///  Description : Ajuster la résolution de la texture
///
///  @param [in]       ResX const int Resolution en largeut
///  @param [in]       ResY const int Resolution en hauteur
///
///  @return None
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
void CScene::AjusterResolution(const int ResLargeur, const int ResHauteur)
{
	m_ResLargeur = ResLargeur;
	m_ResHauteur = ResHauteur;
}

///////////////////////////////////////////////////////////////////////////////
///  public  AjusterNbRebondsMax \n
///  Description : Set maximum ray bounce
///
///  @param [in]       NbRebondsMax const int      Le nombre maximal de rebonds
///
///  @return None
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
void CScene::AjusterNbRebondsMax(const int NbRebondsMax)
{
	m_NbRebondsMax = NbRebondsMax;
}

///////////////////////////////////////////////////////////////////////////////
///  public  AjusterEnergieMinimale \n
///  Description : Ajuste l'énergie minimale du rayon
///
///  @param [in]       EnergieMin const Math3D::REAL    L'énergie minimale du rayon
///
///  @return None
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
void CScene::AjusterEnergieMinimale(const REAL EnergieMin)
{
	m_EnergieMinRayon = EnergieMin;
}

///////////////////////////////////////////////////////////////////////////////
///  public  AjusterIndiceRefraction \n
///  Description : Ajuste l'indice de réfraction de la scène
///
///  @param [in]       IndiceRefraction const Math3D::REAL   Indice de réfraction
///
///  @return None
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
void CScene::AjusterIndiceRefraction(const REAL IndiceRefraction)
{
	m_IndiceRefractionScene = IndiceRefraction;
}

///////////////////////////////////////////////////////////////////////////////
///  public  TraiterFichierDeScene \n
///  Description : Traite le fichier de données de la scène
///
///  @param [in]       Fichier const char *    Fichier à traiter
///
///  @return None
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
void CScene::TraiterFichierDeScene(const char* Fichier)
{
	fstream FichierScene(Fichier, ios::in);
	size_t currSurfaceIdx = -1;
	if (FichierScene.is_open())
	{
		EtatTraitementScene EtatCourant = TRAITEMENT_SCENE;
		EtatTraitementScene EtatNouveau = TRAITEMENT_SCENE;

		char Line[NB_MAX_CAR_PAR_LIGNE];
		string Buffer;
		int count = 0;

		CLumiere* Lumiere = NULL;
		ISurface* Surface = NULL;

		float Val0, Val1, Val2;
		int  R, G, B;

		while (!FichierScene.eof())
		{
			FichierScene.getline(Line, NB_MAX_CAR_PAR_LIGNE);
			Buffer = Line;
			CStringUtils::Trim(Buffer, " ");

			// Passer les lignes vides et les commentaires
			if (Buffer.empty() || Buffer[0] == '*')
				continue;
			else
			{
				// Vérifier l'arrivée d'un nouvel état de traitement
				bool EstNouvelObjetScene = true;
				if (STRING_CHECKFIND(Buffer, "Lumiere:")) EtatNouveau = TRAITEMENT_LUMIERE;
				else if (STRING_CHECKFIND(Buffer, "Poly:")) EtatNouveau = TRAITEMENT_TRIANGLE;
				else if (STRING_CHECKFIND(Buffer, "Plane:")) EtatNouveau = TRAITEMENT_PLAN;
				else if (STRING_CHECKFIND(Buffer, "Quad:")) EtatNouveau = TRAITEMENT_QUADRIQUE;
				else
					EstNouvelObjetScene = false;

				if (EstNouvelObjetScene)
				{
					// Ajouter objet nouvellement traité à la scène
					if (EtatCourant != TRAITEMENT_SCENE)
					{
						if (EtatCourant == TRAITEMENT_LUMIERE)
						{
							AjouterLumiere(Lumiere);
						}
						else {
							AjouterSurface(Surface);
							currSurfaceIdx++;
						}
						Surface = NULL;
						Lumiere = NULL;
					}

					// Substituer le nouvel état pour l'ancien
					EtatCourant = EtatNouveau;

					// créer un nouvel objet de scène puis le remplir
					switch (EtatCourant)
					{
					case TRAITEMENT_LUMIERE:
						Lumiere = new CLumiere();
						cout << "[ACTION]: Creation d'une lumiere..." << endl;
						break;
					case TRAITEMENT_TRIANGLE:
						Surface = new CTriangle();
						m_trianglesIndexes.push_back(currSurfaceIdx);
						cout << "[ACTION]: Creation d'un triangle..." << endl;
						break;
					case TRAITEMENT_PLAN:
						Surface = new CPlan();
						m_planesIndexes.push_back(currSurfaceIdx);
						cout << "[ACTION]: Creation d'un plan..." << endl;
						break;
					case TRAITEMENT_QUADRIQUE:
						Surface = new CQuadrique();
						m_quadricsIndexes.push_back(currSurfaceIdx);
						cout << "[ACTION]: Creation d'une quadrique..." << endl;
						break;
					}
				}
				else
				{
					// Remplir les informations génériques de l'objet courant
					if (Surface != NULL)
					{
						bool IsGenericSurfaceInfo = true;

						if (STRING_CHECKFIND(Buffer, "color:"))
						{
							sscanf(Buffer.c_str(), "%s %i %i %i", Line, &R, &G, &B);
							Surface->AjusterCouleur(CCouleur(R, G, B));
						}
						else if (STRING_CHECKFIND(Buffer, "ambient:"))
						{
							sscanf(Buffer.c_str(), "%s %f", Line, &Val0);
							Surface->AjusterCoeffAmbiant(Val0);
						}
						else if (STRING_CHECKFIND(Buffer, "diffus:"))
						{
							sscanf(Buffer.c_str(), "%s %f", Line, &Val0);
							Surface->AjusterCoeffDiffus(Val0);
						}
						else if (STRING_CHECKFIND(Buffer, "specular:"))
						{
							sscanf(Buffer.c_str(), "%s %f %f", Line, &Val0, &Val1);
							Surface->AjusterCoeffSpeculaire(Val0);
							Surface->AjusterCoeffBrillance(Val1);
						}
						else if (STRING_CHECKFIND(Buffer, "reflect:"))
						{
							sscanf(Buffer.c_str(), "%s %f", Line, &Val0);
							Surface->AjusterCoeffReflexion(Val0);
						}
						else if (STRING_CHECKFIND(Buffer, "refract:"))
						{
							sscanf(Buffer.c_str(), "%s %f %f", Line, &Val0, &Val1);
							Surface->AjusterCoeffRefraction(Val0);
							Surface->AjusterIndiceRefraction(Val1);
						}
						else if (STRING_CHECKFIND(Buffer, "rotate:"))
						{
							sscanf(Buffer.c_str(), "%s %f %f %f", Line, &Val0, &Val1, &Val2);

							CMatrice4 Transform = Surface->ObtenirTransformation();
							Transform.RotationAutourDesX(Deg2Rad<REAL>(Val0));
							Transform.RotationAutourDesY(Deg2Rad<REAL>(Val1));
							Transform.RotationAutourDesZ(Deg2Rad<REAL>(Val2));
							Surface->AjusterTransformation(Transform);
						}
						else if (STRING_CHECKFIND(Buffer, "translate:"))
						{
							sscanf(Buffer.c_str(), "%s %f %f %f", Line, &Val0, &Val1, &Val2);
							CMatrice4 Transform = Surface->ObtenirTransformation();
							Transform.Translation(Val0, Val1, Val2);
							Surface->AjusterTransformation(Transform);
						}
						else if (STRING_CHECKFIND(Buffer, "scale:"))
						{
							sscanf(Buffer.c_str(), "%s %f %f %f", Line, &Val0, &Val1, &Val2);
							CMatrice4 Transform = Surface->ObtenirTransformation();
							Transform.MiseAEchelle(Val0, Val1, Val2);
							Surface->AjusterTransformation(Transform);
						}
						else
							IsGenericSurfaceInfo = false;

						if (IsGenericSurfaceInfo)
							continue;
					}

					// Remplir les infos spécifiques à l'objet
					switch (EtatCourant)
					{
					case TRAITEMENT_SCENE:

						if (STRING_CHECKFIND(Buffer, "background:"))
						{
							sscanf(Buffer.c_str(), "%s %i %i %i", Line, &R, &G, &B);
							AjusterCouleurArrierePlan(CCouleur(R, G, B));
						}
						else if (STRING_CHECKFIND(Buffer, "origin:"))
						{
							sscanf(Buffer.c_str(), "%s %f %f %f", Line, &Val0, &Val1, &Val2);
							AjusterPositionCamera(CVecteur3(Val0, Val1, Val2));
						}
						else if (STRING_CHECKFIND(Buffer, "eye:"))
						{
							sscanf(Buffer.c_str(), "%s %f %f %f", Line, &Val0, &Val1, &Val2);
							AjusterPointViseCamera(CVecteur3(Val0, Val1, Val2));
						}
						else if (STRING_CHECKFIND(Buffer, "up:"))
						{
							sscanf(Buffer.c_str(), "%s %f %f %f", Line, &Val0, &Val1, &Val2);
							AjusterVecteurUpCamera(CVecteur3(Val0, Val1, Val2));
						}


						break;

					case TRAITEMENT_LUMIERE:

						if (STRING_CHECKFIND(Buffer, "position:"))
						{
							sscanf(Buffer.c_str(), "%s %f %f %f", Line, &Val0, &Val1, &Val2);
							Lumiere->SetPosition(CVecteur3(Val0, Val1, Val2));
						}
						else if (STRING_CHECKFIND(Buffer, "intens:"))
						{
							sscanf(Buffer.c_str(), "%s %f", Line, &Val0);
							Lumiere->SetIntensity(Val0);
						}
						else if (STRING_CHECKFIND(Buffer, "color:"))
						{
							sscanf(Buffer.c_str(), "%s %i %i %i", Line, &R, &G, &B);
							Lumiere->AjusterCouleur(CCouleur(R, G, B));
						}

						break;

					case TRAITEMENT_TRIANGLE:

						if (STRING_CHECKFIND(Buffer, "point:"))
						{
							int PtIdx;
							sscanf(Buffer.c_str(), "%s %i %f %f %f", Line, &PtIdx, &Val0, &Val1, &Val2);
							((CTriangle*)Surface)->AjusterPoint(PtIdx, CVecteur3(Val0, Val1, Val2));
						}

						break;

					case TRAITEMENT_PLAN:

						if (STRING_CHECKFIND(Buffer, "v_linear:"))
						{
							sscanf(Buffer.c_str(), "%s %f %f %f", Line, &Val0, &Val1, &Val2);
							((CPlan*)Surface)->AjusterNormale(CVecteur3(Val0, Val1, Val2));
						}
						else if (STRING_CHECKFIND(Buffer, "v_const:"))
						{
							sscanf(Buffer.c_str(), "%s %f", Line, &Val0);
							((CPlan*)Surface)->AjusterConstante(Val0);
						}

						break;

					case TRAITEMENT_QUADRIQUE:

						if (STRING_CHECKFIND(Buffer, "v_quad:"))
						{
							sscanf(Buffer.c_str(), "%s %f %f %f", Line, &Val0, &Val1, &Val2);
							((CQuadrique*)Surface)->AjusterQuadratique(CVecteur3(Val0, Val1, Val2));
						}
						else if (STRING_CHECKFIND(Buffer, "v_mixte:"))
						{
							sscanf(Buffer.c_str(), "%s %f %f %f", Line, &Val0, &Val1, &Val2);
							((CQuadrique*)Surface)->AjusterMixte(CVecteur3(Val0, Val1, Val2));
						}
						else if (STRING_CHECKFIND(Buffer, "v_linear:"))
						{
							sscanf(Buffer.c_str(), "%s %f %f %f", Line, &Val0, &Val1, &Val2);
							((CQuadrique*)Surface)->AjusterLineaire(CVecteur3(Val0, Val1, Val2));
						}
						else if (STRING_CHECKFIND(Buffer, "v_const:"))
						{
							sscanf(Buffer.c_str(), "%s %f", Line, &Val0);
							((CQuadrique*)Surface)->AjusterConstante(Val0);
						}

						break;
					}
				}
			}
		}

		// Fermer le fichier de scène
		FichierScene.close();

		// Ajouter le dernier objet traité
		if (Surface != NULL) AjouterSurface(Surface);
		if (Lumiere != NULL) AjouterLumiere(Lumiere);
	}
	else
		cerr << "[CScene::TraiterFichierDeScene()] : Incapable d'ouvrir " << Fichier << "." << endl;

}

///////////////////////////////////////////////////////////////////////////////
///  public  Liberer \n
///  Description : Désallouer toutes les données de scène
///
///  @return None
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
void CScene::Liberer(void)
{
	// Supprimer les surfaces
	for (SurfaceIterator it = m_Surfaces.begin(); it != m_Surfaces.end(); it++)
		delete (*it);
	m_Surfaces.clear();

	// Supprimer les lumières
	for (LumiereIterator it = m_Lumieres.begin(); it != m_Lumieres.end(); it++)
		delete (*it);
	m_Lumieres.clear();

	// Supprimer le rendu de scène
	glDeleteTextures(1, &m_TextureScene);
	delete m_InfoPixel;
}

///////////////////////////////////////////////////////////////////////////////
///  public  AjusterPositionCamera \n
///  Description : Ajuster la position de la caméra
///
///  @param [in]       Position const Math3D::CVecteur3 & Position
///
///  @return None
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
void CScene::AjusterPositionCamera(const CVecteur3& Position)
{
	m_Camera.Position = Position;
}

///////////////////////////////////////////////////////////////////////////////
///  public  AjusterPointViseCamera \n
///  Description : Ajuster le point cisé par la caméra
///
///  @param [in]       PointVise const Math3D::CVecteur3 &  PointVise
///
///  @return None
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
void CScene::AjusterPointViseCamera(const CVecteur3& PointVise)
{
	m_Camera.PointVise = PointVise;
}

///////////////////////////////////////////////////////////////////////////////
///  public  AjusterVecteurUpCamera \n
///  Description : Ajuster le vecteur up de la caméra
///
///  @param [in]       Up const Math3D::CVecteur3 &  Up
///
///  @return None
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
void CScene::AjusterVecteurUpCamera(const CVecteur3& Up)
{
	m_Camera.Up = Up;
}

///////////////////////////////////////////////////////////////////////////////
///  public  AjusterCouleurArrierePlan \n
///  Description : Ajuster la couleur de l'arrière-plan de la scène
///
///  @param [in]       Color const CCouleur &     Couleur ArrierePlan
///
///  @return None
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
void CScene::AjusterCouleurArrierePlan(const CCouleur& Couleur)
{
	m_CouleurArrierePlan = Couleur;
}

///////////////////////////////////////////////////////////////////////////////
///  public  ObtenirTextureGL \n
///  Description : Obtenir le rendu de la scène dans une texture openGL
///
///  @return GLuint Texture Id
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
GLuint CScene::ObtenirTextureGL(void)
{
	return m_TextureScene;
}

///////////////////////////////////////////////////////////////////////////////
///  private  InitialiserCamera \n
///  Description : Initialiser l'angle et la matrice "modelView" de la caméra
///
///  @return None
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
void CScene::InitialiserCamera(void)
{
	// Calculer l'angle de la caméra
	//
	//
	//                  lentille                  |
	//                    |                       |
	// écran/             |                       y2
	// film               |                       |
	// -+---------+-------+-------+---------------+
	//  |         |<- f ->|<- f ->|
	//  y1                |
	//  |                 |
	//  |
	//
	//  <------ d1 ------>|<--------- d2 --------->
	//
	// From: 1 / f   = 1 / d1 + 1 / d2
	//  and: y1 / y2 = d1 / d2
	//  and: d1      = DIM_FILM
	//       y2      = ( d2 / f - 1 ) * DIM_FILM
	//
	// Nous avons besoin de trouver atan( y2 / d2 ) pour l'ouverture du champ
	REAL d2 = CVecteur3::Distance(m_Camera.Position, m_Camera.PointVise);
	REAL y2 = (d2 / (m_Camera.Focale * RENDRE_REEL(0.001)) - 1) * DIM_FILM_CAM;
	m_Camera.Angle = (360 * atan2(y2 * RENDRE_REEL(0.5), d2)) / RENDRE_REEL(PI);

	// Calculer la matrice modelview de la caméra
	CVecteur3 N = CVecteur3::Normaliser(m_Camera.Position - m_Camera.PointVise);
	CVecteur3 V = CVecteur3::Normaliser(m_Camera.Up - N * CVecteur3::ProdScal(m_Camera.Up, N));
	CVecteur3 U = CVecteur3::ProdVect(V, N);

	m_Camera.Orientation = CMatrice4(U.x, U.y, U.z, 0.0f,
		V.x, V.y, V.z, 0.0f,
		N.x, N.y, N.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}

///////////////////////////////////////////////////////////////////////////////
///  public  LancerRayons \n
///  Description : Lancement des rayons (raytrace) dans la scène courante
///
///  @return None
///
///  @author Olivier Dionne 
///  @date   13/08/2008
///
///////////////////////////////////////////////////////////////////////////////
void CScene::LancerRayons(void)
{
	Initialiser();


	REAL HalfH = tan(Deg2Rad<REAL>(m_Camera.Angle * RENDRE_REEL(0.5)));
	REAL HalfW = (RENDRE_REEL(m_ResLargeur) / m_ResHauteur) * HalfH;
	REAL InvResWidth = RENDRE_REEL(1.0) / m_ResLargeur;
	REAL InvResHeight = RENDRE_REEL(1.0) / m_ResHauteur;

	CRayon   Rayon;
	CCouleur PixelColor;
	int      PixelIdx = 0;


	if (CVar::g_ComputerShadersON)
	{
		//copie surfacesData to uniforms in shader
		GLuint prog = CVar::g_ComputeShader->getProg();
		GLuint uniformIndex = glGetUniformBlockIndex(prog, "SurfacesData");
		GLint uniformSize = 0;
		glGetActiveUniformBlockiv(prog, uniformIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &uniformSize);

		GLubyte* buffer = (GLubyte*) malloc(uniformSize);
		if (buffer == nullptr)
		{
			std::cout << "[ERROR] : Can't allocate enough space for uniform buffer data" << std::endl;
			exit(EXIT_FAILURE);
		}

		size_t nbSurfaces = m_Surfaces.size();
		size_t nbQuadrics = m_quadricsIndexes.size();
		size_t nbTriangles = m_trianglesIndexes.size();
		size_t nbPlanes = m_planesIndexes.size();

		//CHeck if enough space in allocated surface buffers
		if (nbQuadrics > MAX_NB_QUADRICS || nbTriangles > MAX_NB_TRIANGLES || nbPlanes > MAX_NB_PLANES)
		{
			std::cout << "[ERROR] : Too many surfaces in app to allocate it in the GLSL uniform struct";
			cout << "\tMAX_NB_QUADRICS = " << MAX_NB_QUADRICS << " | nb_quadrics = " << m_quadricsIndexes.size() << std::endl;
			cout << "\tMAX_NB_TRIANGLES =" << MAX_NB_TRIANGLES << " | nb_triangles = " << m_trianglesIndexes.size() << std::endl;
			cout << "\tMAX_NB_PLANES =" << MAX_NB_PLANES << " | nb_planes = " << m_planesIndexes.size() << std::endl;
			exit(EXIT_FAILURE);
		}

		//Create structures similar to the GLSL ones (needed to generate uniform names)
		const char* uniformNames[NB_SURFACETYPE] = {
			"quadrics",
			"triangles"
		};

		const char* quadricNames[QUADRICTYPE_SIZE] = {
			"m_quadrique",
			"m_lineaire",
			"m_mixte",
			"m_cst"
		};
		const char* triangleNames[TRIANGLETYPE_SIZE] = {
			"m_points",
			"m_normal"
		};


		size_t nbUniforms = nbQuadrics*QUADRICTYPE_SIZE + nbTriangles*TRIANGLETYPE_SIZE;

		//Init an array of names for each surface type
		char** QuadricCompleteNames = new char*[nbQuadrics];
		for (size_t i = 0; i < nbUniforms; i++)
		{
			QuadricCompleteNames[i] = new char[64];
		}

		char** TriangleCompleteNames = new char*[nbTriangles];
		for (size_t i = 0; i < nbUniforms; i++)
		{
			TriangleCompleteNames[i] = new char[64];
		}


		//Fill char* array with all uniform name possible for each surface type
		//for quadrics
		for (size_t i = 0; i < nbQuadrics; i++)
		{
			int offset = i * QUADRICTYPE_SIZE;
			for (size_t j = 0; j < QUADRICTYPE_SIZE; j++)
			{
				string s = std::to_string(i);
				strcpy(QuadricCompleteNames[offset + j], uniformNames[0]);
				strcat(QuadricCompleteNames[offset + j], "[");
				strcat(QuadricCompleteNames[offset + j], s.c_str());
				strcat(QuadricCompleteNames[offset + j], "].");
				strcat(QuadricCompleteNames[offset + j], quadricNames[j]);
				cout << "quadric : " << offset + j << " | " << QuadricCompleteNames[offset + j] << endl;
			}
		}

		for (size_t i = 0; i < nbTriangles; i++)
		{
			int offset = nbQuadrics + i * TRIANGLETYPE_SIZE;
			for (size_t j = 0; j < TRIANGLETYPE_SIZE; j++)
			{
				string s = std::to_string(i);
				strcpy(TriangleCompleteNames[offset + j], uniformNames[1]);
				strcat(TriangleCompleteNames[offset + j], "[");
				strcat(TriangleCompleteNames[offset + j], s.c_str());
				strcat(TriangleCompleteNames[offset + j], "].");
				strcat(TriangleCompleteNames[offset + j], triangleNames[j]);
				cout << "triangles : " << offset + j << " | " << TriangleCompleteNames[offset + j] << endl;

			}
		}

		cout << endl;


		//Based on the surface type and previous generated names for uniforms, get uniform index and update its value

		GLuint** indices = new GLuint*[NB_SURFACETYPE];
		GLint** size = new GLint*[NB_SURFACETYPE];
		GLint** offset = new GLint*[NB_SURFACETYPE];
		GLint** type = new GLint*[NB_SURFACETYPE];

		//not good perf I think with this **pointer, should use ***pointer
		const char** curNames;
		for (size_t i = 0; i < NB_SURFACETYPE; i++)
		{
			size_t curSize = 0;
			switch (i)
			{
			case 0:
				curSize = QUADRICTYPE_SIZE;
				curNames = quadricNames;
				break;
			case 1:
				curSize = TRIANGLETYPE_SIZE;
				curNames = triangleNames;
				break;
			default:
				cout << "[WARNING] : Not value matching for current size type" << endl;
				break;
			}

			indices[i] = new GLuint(curSize);
			size[i] = new GLint(curSize);
			offset[i] = new GLint(curSize);
			type[i] = new GLint(curSize);


			glGetUniformIndices(prog, curSize, curNames, indices[i]);

			glGetActiveUniformsiv(prog, curSize, indices[i], GL_UNIFORM_OFFSET, offset[i]);
			glGetActiveUniformsiv(prog, curSize, indices[i], GL_UNIFORM_SIZE, size[i]);
			glGetActiveUniformsiv(prog, curSize, indices[i], GL_UNIFORM_TYPE, type[i]);

		}


		///Fill data for all surfaces per type
		//fill data for quadrics
		for (size_t i = 0; i < nbQuadrics; i++)
		{
			CQuadrique* curQuadric = dynamic_cast<CQuadrique*>(m_Surfaces[m_quadricsIndexes[i]]);

			if (curQuadric == nullptr)
			{
				cout << "[ERROR] : Value in m_quadricsIndexes is not an index to a CQuadric object" << endl;
				continue;
			}

			//For m_quadrique (CVecteur3)
			CVecteur3 tmpVec = curQuadric->ObtenirQuadratique();
			double vecValues[4] = {tmpVec.x, tmpVec.y, tmpVec.z, 0.0};
			memcpy(buffer + offset[QUADRIC][0], vecValues, size[QUADRIC][0] * TypeSize(type[QUADRIC][0]));

			//For m_lineaire (CVecteur3)
			tmpVec = curQuadric->ObtenirLineaire();
			double vecValues2[4] = { tmpVec.x, tmpVec.y, tmpVec.z, 0.0 };
			memcpy(buffer + offset[QUADRIC][1], vecValues2, size[QUADRIC][1] * TypeSize(type[QUADRIC][1]));

			//For m_mixte (CVecteur3)
			tmpVec = curQuadric->ObtenirQuadratique();
			double vecValues3[4] {tmpVec.x, tmpVec.y, tmpVec.z, 0.0};
			memcpy(buffer + offset[QUADRIC][2], vecValues3, size[QUADRIC][2] * TypeSize(type[QUADRIC][2]));

			//For m_cst (double)
			double value4 = curQuadric->ObtenirConstante();
			memcpy(buffer + offset[QUADRIC][3], &value4, size[QUADRIC][3] * TypeSize(type[QUADRIC][3]));

		}


		//fill data for triangles 


		//TODO : add les memcpy
		//for (size_t j = 0; j < curSize; j++)
		//{
		//	memcpy(buffer + offset[j], &scale,
		//		size[Scale] * TypeSize(type[Scale]));


		//}



		//Create buffer
		GLuint ubo;
		glGenBuffers(1, &ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		glBufferData(GL_UNIFORM_BUFFER, uniformSize, buffer, GL_STATIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, uniformIndex, ubo);


		//Clean memory
		for (size_t i = 0; i < nbQuadrics; i++)
		{
			delete QuadricCompleteNames[i];
		}
		delete[] QuadricCompleteNames;


		for (size_t i = 0; i < nbTriangles; i++)
		{
			delete TriangleCompleteNames[i];
		}
		delete[] TriangleCompleteNames;

		for (size_t i = 0; i < NB_SURFACETYPE; i++)
		{
			delete indices[i];
			delete size[i];
			delete offset[i];
			delete type[i];
		}

		delete[] indices;
		delete[] size;
		delete[] offset;
		delete[] type;

		//Activate compute shader
		CVar::g_ComputeShader->activer();
		glDispatchCompute(CVar::g_LargeurViewport / 16, CVar::g_HauteurViewport / 16, 1);
		return;
	}
	else
	{
		//RayTracing CPU
		for (int PixY = 0; PixY < m_ResHauteur; PixY++)
		{
			for (int PixX = 0; PixX < m_ResLargeur; PixX++)
			{
				// Prépare le rayon
				Rayon.AjusterOrigine(m_Camera.Position);

				Rayon.AjusterDirection(CVecteur3((2 * PixX * InvResWidth - 1) * HalfW,
					(2 * PixY * InvResHeight - 1) * HalfH,
					-1));

				Rayon.AjusterDirection(CVecteur3::Normaliser(Rayon.ObtenirDirection() * m_Camera.Orientation));
				Rayon.AjusterEnergie(1);
				Rayon.AjusterNbRebonds(0);
				Rayon.AjusterIndiceRefraction(1);

				PixelColor = ObtenirCouleur(Rayon);

				PixelIdx = (PixX + (PixY * m_ResLargeur)) * 3;
				m_InfoPixel[PixelIdx + 0] = PixelColor.r;
				m_InfoPixel[PixelIdx + 1] = PixelColor.g;
				m_InfoPixel[PixelIdx + 2] = PixelColor.b;
			}
		}
	}

	// Créer une texture openGL
	glGenTextures(1, &m_TextureScene);
	glBindTexture(GL_TEXTURE_2D, m_TextureScene);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_ResLargeur, m_ResHauteur, 0, GL_RGB, GL_FLOAT, m_InfoPixel);
}

/* Helper function to convert GLSL types to storage sizes */
size_t CScene::TypeSize(GLenum type)
{
	size_t size;
#define CASE(Enum, Count, Type) \
case Enum: size = Count * sizeof(Type); break
	switch (type) {
		CASE(GL_FLOAT, 1, GLfloat);
		CASE(GL_FLOAT_VEC2, 2, GLfloat);
		CASE(GL_FLOAT_VEC3, 3, GLfloat);
		CASE(GL_FLOAT_VEC4, 4, GLfloat);
		CASE(GL_INT, 1, GLint);
		CASE(GL_INT_VEC2, 2, GLint);
		CASE(GL_INT_VEC3, 3, GLint);
		CASE(GL_INT_VEC4, 4, GLint);
		CASE(GL_UNSIGNED_INT, 1, GLuint);
		CASE(GL_UNSIGNED_INT_VEC2, 2, GLuint);
		CASE(GL_UNSIGNED_INT_VEC3, 3, GLuint);
		CASE(GL_UNSIGNED_INT_VEC4, 4, GLuint);
		CASE(GL_BOOL, 1, GLboolean);
		CASE(GL_BOOL_VEC2, 2, GLboolean);
		CASE(GL_BOOL_VEC3, 3, GLboolean);
		CASE(GL_BOOL_VEC4, 4, GLboolean);
		CASE(GL_FLOAT_MAT2, 4, GLfloat);
		CASE(GL_FLOAT_MAT2x3, 6, GLfloat);
		CASE(GL_FLOAT_MAT2x4, 8, GLfloat);
		CASE(GL_FLOAT_MAT3, 9, GLfloat);
		CASE(GL_FLOAT_MAT3x2, 6, GLfloat);
		CASE(GL_FLOAT_MAT3x4, 12, GLfloat);
		CASE(GL_FLOAT_MAT4, 16, GLfloat);
		CASE(GL_FLOAT_MAT4x2, 8, GLfloat);
		CASE(GL_FLOAT_MAT4x3, 12, GLfloat);
#undef CASE
	default:
		fprintf(stderr, "Unknown type: 0x%x\n", type);
		exit(EXIT_FAILURE);
		break;
	}
	return size;
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
const CCouleur CScene::ObtenirCouleur(const CRayon& Rayon) const
{
	CIntersection Result;
	CIntersection Tmp;

	for (SurfaceIterator aSurface = m_Surfaces.begin(); aSurface != m_Surfaces.end(); aSurface++)
	{
		Tmp = (*aSurface)->Intersection(Rayon);
		if (Tmp.ObtenirDistance() > EPSILON && (Tmp.ObtenirDistance() < Result.ObtenirDistance() || Result.ObtenirDistance() < 0))
			Result = Tmp;
	}

	// S'il n'y aucune intersection, retourner la couleur de l'arrière-plan
	// Sinon, retourner la couleur à l'intersection
	return (Result.ObtenirDistance() < 0) ? m_CouleurArrierePlan : ObtenirCouleurSurIntersection(Rayon, Result);
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
const CCouleur CScene::ObtenirCouleurSurIntersection(const CRayon& Rayon, const CIntersection& Intersection) const
{
	CCouleur  Result = Intersection.ObtenirSurface()->ObtenirCouleur() * Intersection.ObtenirSurface()->ObtenirCoeffAmbiant();
	CVecteur3 IntersectionPoint = Rayon.ObtenirOrigine() + Intersection.ObtenirDistance() * Rayon.ObtenirDirection();

	// Calculer les contribution colorées des toutes les lumières dans la scène
	CCouleur LumiereContributions = CCouleur::NOIR;
	CRayon   LumiereRayon;
	for (LumiereIterator uneLumiere = m_Lumieres.begin(); uneLumiere != m_Lumieres.end(); uneLumiere++)
	{
		// Initialise le rayon de lumière (ou rayon d'ombre)
		LumiereRayon.AjusterOrigine(IntersectionPoint);
		LumiereRayon.AjusterDirection((*uneLumiere)->GetPosition() - IntersectionPoint);
		LumiereRayon.AjusterEnergie(1);
		LumiereRayon.AjusterIndiceRefraction(1);

		if (CVecteur3::ProdScal(LumiereRayon.ObtenirDirection(), Intersection.ObtenirNormale()) > 0)
		{
			// Obtenir la couleur à partir de la lumière
			CCouleur Filter = ObtenirFiltreDeSurface(LumiereRayon);
			CCouleur LumiereCouleur = (*uneLumiere)->ObtenirCouleur() * Filter;

			// Ajouter la contribution de Gouraud
			REAL GouraudFactor = (*uneLumiere)->GetIntensity() * Intersection.ObtenirSurface()->ObtenirCoeffDiffus() *
				CVecteur3::ProdScal(Intersection.ObtenirNormale(), LumiereRayon.ObtenirDirection());
			Result += Intersection.ObtenirSurface()->ObtenirCouleur() * GouraudFactor * LumiereCouleur;

			// Ajouter la contribution de Phong
			CVecteur3 ReflectedRayon = CVecteur3::Reflect(LumiereRayon.ObtenirDirection(), Intersection.ObtenirNormale());
			REAL ProdScal = CVecteur3::ProdScal(ReflectedRayon, Rayon.ObtenirDirection());
			if (ProdScal > 0)
			{
				REAL PhongFactor = (*uneLumiere)->GetIntensity() * Intersection.ObtenirSurface()->ObtenirCoeffSpeculaire() *
					pow(ProdScal, Intersection.ObtenirSurface()->ObtenirCoeffBrillance());
				Result += (PhongFactor * LumiereCouleur);
			}
		}
	}

	// Effectuer les réflexions de rayon
	REAL ReflectedRayonEnergy = Intersection.ObtenirSurface()->ObtenirCoeffReflexion() * Rayon.ObtenirEnergie();
	if (ReflectedRayonEnergy > m_EnergieMinRayon && Rayon.ObtenirNbRebonds() < m_NbRebondsMax)
	{
		CRayon ReflectedRayon;
		ReflectedRayon.AjusterDirection(CVecteur3::Reflect(Rayon.ObtenirDirection(), Intersection.ObtenirNormale()));
		ReflectedRayon.AjusterOrigine(IntersectionPoint);
		ReflectedRayon.AjusterEnergie(ReflectedRayonEnergy);
		ReflectedRayon.AjusterNbRebonds(Rayon.ObtenirNbRebonds() + 1);

		Result += ObtenirCouleur(ReflectedRayon) * Intersection.ObtenirSurface()->ObtenirCoeffReflexion();
	}

	// Effectuer les réfractions de rayon
	REAL RefractedRayonEnergy = Intersection.ObtenirSurface()->ObtenirCoeffRefraction() * Rayon.ObtenirEnergie();
	if (RefractedRayonEnergy > m_EnergieMinRayon && Rayon.ObtenirNbRebonds() < m_NbRebondsMax)
	{
		REAL      IndiceRefractionRatio;
		CRayon    RefractedRayon;
		CVecteur3 SurfaceNormal = Intersection.ObtenirNormale();

		if (Rayon.ObtenirIndiceRefraction() == Intersection.ObtenirSurface()->ObtenirIndiceRefraction())
		{
			// de l'intérieur, vers l'extérieur...
			RefractedRayon.AjusterIndiceRefraction(m_IndiceRefractionScene);
			IndiceRefractionRatio = Intersection.ObtenirSurface()->ObtenirIndiceRefraction() / m_IndiceRefractionScene;
			SurfaceNormal = -SurfaceNormal;
		}
		else
		{
			// de l'extérieur, vers l'intérieur...
			RefractedRayon.AjusterIndiceRefraction(Intersection.ObtenirSurface()->ObtenirIndiceRefraction());
			IndiceRefractionRatio = m_IndiceRefractionScene / Intersection.ObtenirSurface()->ObtenirIndiceRefraction();
		}

		//TODO: la direction du vecteur avec refraction
		RefractedRayon.AjusterOrigine(IntersectionPoint);
		RefractedRayon.AjusterEnergie(RefractedRayonEnergy);
		RefractedRayon.AjusterNbRebonds(Rayon.ObtenirNbRebonds() + 1);

		RefractedRayon.AjusterDirection(CVecteur3::Refract(Rayon.ObtenirDirection(), SurfaceNormal, IndiceRefractionRatio));

		//A decommenter apres ajustement de la direction!!
		Result += ObtenirCouleur(RefractedRayon) * Intersection.ObtenirSurface()->ObtenirCoeffRefraction();
	}

	return Result;
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
const CCouleur CScene::ObtenirFiltreDeSurface(CRayon& LumiereRayon) const
{
	CCouleur Filter = CCouleur::BLANC;
	CIntersection LumiereIntersection;

	REAL Distance = CVecteur3::Norme(LumiereRayon.ObtenirDirection());
	LumiereRayon.AjusterDirection(LumiereRayon.ObtenirDirection() / Distance);

	for (SurfaceIterator aSurface = m_Surfaces.begin(); aSurface != m_Surfaces.end(); aSurface++)
	{
		LumiereIntersection = (*aSurface)->Intersection(LumiereRayon);

		if (LumiereIntersection.ObtenirDistance() > EPSILON &&
			LumiereIntersection.ObtenirDistance() < Distance)
			Filter *= LumiereIntersection.ObtenirSurface()->ObtenirCouleur() *
			LumiereIntersection.ObtenirSurface()->ObtenirCoeffRefraction();
	}

	return Filter;
}
