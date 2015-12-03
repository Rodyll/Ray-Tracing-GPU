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
#define MAX_NB_LIGHTS 10
#define NB_SURFACETYPE 3


#define SCENEDATATYPE_SIZE 8
#define SURFACETYPE_SIZE 8
#define QUADRICTYPE_SIZE 5
#define TRIANGLETYPE_SIZE 3
#define PLANETYPE_SIZE 3
#define LIGHTTYPE_SIZE 3

#define QUADRICTYPE_SIZE_ALL (QUADRICTYPE_SIZE-1 + SURFACETYPE_SIZE)
#define TRIANGLETYPE_SIZE_ALL (TRIANGLETYPE_SIZE + 1 + SURFACETYPE_SIZE)
#define PLANETYPE_SIZE_ALL (PLANETYPE_SIZE - 1 + SURFACETYPE_SIZE)


#define MAX_VARNAME_SIZE 64

#define U_GENERAL_SIZE 7

enum { QUADRIC, TRIANGLE, PLANE, LIGHT };



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
	, m_computedImage(0)
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
	size_t currSurfaceIdx = 0;
	size_t currLightIdx = 0;
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
							currLightIdx++;
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
						m_lightsIndexes.push_back(currLightIdx);
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


	// Créer une texture openGL (elle sera remplie selon la méthode choisie)
	glGenTextures(1, &m_TextureScene);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_TextureScene);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WIDTH, m_ResLargeur, )
	
	//DEBUG
	//std::cout << "CScene::DIM_FILM_CAM = " << CScene::DIM_FILM_CAM << std::endl;

	if (CVar::g_ComputerShadersON)
	{
		//1 - copy general data to first uniform block
		GLuint prog = CVar::g_ComputeShader->getProg();
		GLuint generalUniformIndex = glGetUniformBlockIndex(prog, "General");
		if (generalUniformIndex == GL_INVALID_INDEX)
		{
			std::cout << "[ERROR] : Can't find uniform block called 'General'" << std::endl;
			exit(EXIT_FAILURE);
		}
		GLint generalUniformSize = 0;
		glGetActiveUniformBlockiv(prog, generalUniformIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &generalUniformSize);

		//DEBUG
		cout << "generalUniformIndex = " << generalUniformIndex << endl;
		cout << "generalUniformSize = " << generalUniformSize << endl;
		//

		GLubyte* generalBuffer = (GLubyte*)malloc(generalUniformSize);
		if (generalBuffer == nullptr)
		{
			std::cout << "[ERROR] : Can't allocate enough space for uniform buffer data" << std::endl;
			exit(EXIT_FAILURE);
		}


		const char* generalNames[U_GENERAL_SIZE] = {
			"cameraPosition",
			"cameraOrientation",
			//"m_ResLargeur",
			//"m_ResHauteur",
			"halfWidth",
			"halfHeight",
			"invResWidth",
			"invResHeight",
			"couleurArrierePlan"
		};

		GLuint* generalIndices = new GLuint[U_GENERAL_SIZE];
		glGetUniformIndices(prog, U_GENERAL_SIZE, generalNames, generalIndices);


		//DEBUG
		for (size_t cpt = 0; cpt < U_GENERAL_SIZE; cpt++)
		{
			cout << "generalIndices[" << cpt << "] = " << generalIndices[cpt] << endl;
		}
		cout << endl;
		//

		//int test[10] = { 0 };
		//int value = 2;
		//memcpy(test + 0, &value, 1 * sizeof(int));

		GLint* generalSize = new GLint[U_GENERAL_SIZE];
		GLint* generalOffset = new GLint[U_GENERAL_SIZE];
		GLint* generalType = new GLint[U_GENERAL_SIZE];
		glGetActiveUniformsiv(prog, U_GENERAL_SIZE, generalIndices, GL_UNIFORM_OFFSET, generalOffset);
		glGetActiveUniformsiv(prog, U_GENERAL_SIZE, generalIndices, GL_UNIFORM_SIZE, generalSize);
		glGetActiveUniformsiv(prog, U_GENERAL_SIZE, generalIndices, GL_UNIFORM_TYPE, generalType);

		float camPosition[4] = { m_Camera.Position.x, m_Camera.Position.y, m_Camera.Position.z, 0.0f };
		memcpy(generalBuffer + generalOffset[0], &camPosition, generalSize[0] * TypeSize(generalType[0]));
		

		memcpy(generalBuffer + generalOffset[1], m_Camera.Orientation.m, generalSize[1] * TypeSize(generalType[1]));
		
		//memcpy(generalBuffer + generalOffset[2], &m_ResLargeur, generalSize[2] * TypeSize(generalType[2]));
		//memcpy(generalBuffer + generalOffset[3], &m_ResHauteur, generalSize[3] * TypeSize(generalType[3]));
		
		//Index changed because uniform variables commented
		memcpy(generalBuffer + generalOffset[2], &HalfW, generalSize[2] * TypeSize(generalType[2]));
		memcpy(generalBuffer + generalOffset[3], &HalfH, generalSize[3] * TypeSize(generalType[3]));
		
		memcpy(generalBuffer + generalOffset[4], &InvResWidth, generalSize[4] * TypeSize(generalType[4]));
		memcpy(generalBuffer + generalOffset[5], &InvResHeight, generalSize[5] * TypeSize(generalType[5]));
		
		float couleurArrierePlan[4] = { m_CouleurArrierePlan.r, m_CouleurArrierePlan.g, m_CouleurArrierePlan.b, 0.0 };
		memcpy(generalBuffer + generalOffset[6], couleurArrierePlan, generalSize[6] * TypeSize(generalType[6]));


		//Create buffer
		GLuint generalUbo;
		glGenBuffers(1, &generalUbo);
		glBindBuffer(GL_UNIFORM_BUFFER, generalUbo);
		glBufferData(GL_UNIFORM_BUFFER, generalUniformSize, generalBuffer, GL_STATIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, generalUniformIndex, generalUbo);



		//TODO : Clean data (needed to query buffer, find another place to put it)
		//delete[] generalIndices;
		//delete[] generalSize;
		//delete[] generalOffset;
		//delete[] generalType;
		GLenum err(glGetError());
		while (err != GL_NO_ERROR)
		{
			string error;
			switch (err) {
			case GL_INVALID_OPERATION:      error = "INVALID_OPERATION";      break;
			case GL_INVALID_ENUM:           error = "INVALID_ENUM";           break;
			case GL_INVALID_VALUE:          error = "INVALID_VALUE";          break;
			case GL_OUT_OF_MEMORY:          error = "OUT_OF_MEMORY";          break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "INVALID_FRAMEBUFFER_OPERATION";  break;
			}

			cerr << "GL_" << error.c_str() << endl;
			err = glGetError();
			cout << "An error happened after General" << endl;//Process/log the error.
		}


		/***********************************************/
		//2 - copy surfacesData to uniforms in shader
		prog = CVar::g_ComputeShader->getProg();
		GLuint uniformIndex = glGetUniformBlockIndex(prog, "SceneData");
		if (uniformIndex == GL_INVALID_INDEX)
		{
			std::cout << "[ERROR] : Can't find uniform block called 'SceneData'" << std::endl;
			exit(EXIT_FAILURE);
		}

		GLint uniformSize = 0;
		glGetActiveUniformBlockiv(prog, uniformIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &uniformSize);

		//DEBUG
		cout << "uniformIndex = " << uniformIndex << endl;
		cout << "uniformSize = " << uniformSize << endl;
		//

		GLubyte* SceneDataBuffer = (GLubyte*) malloc(uniformSize);
		if (SceneDataBuffer == nullptr)
		{
			std::cout << "[ERROR] : Can't allocate enough space for uniform buffer data" << std::endl;
			exit(EXIT_FAILURE);
		}


		//Binding output image to the right texture
		GLuint imageIndex = glGetUniformLocation(CVar::g_ComputeShader->getProg(), "renderedImage");
		//glUseProgram(prog);
		glActiveTexture(GL_TEXTURE0 + 1);
		//glUniform1i(imageIndex, 1);
		glProgramUniform1i(prog, imageIndex, 1);
		err = glGetError();
		while (err != GL_NO_ERROR)
		{
			string error;
			switch (err) {
			case GL_INVALID_OPERATION:      error = "INVALID_OPERATION";      break;
			case GL_INVALID_ENUM:           error = "INVALID_ENUM";           break;
			case GL_INVALID_VALUE:          error = "INVALID_VALUE";          break;
			case GL_OUT_OF_MEMORY:          error = "OUT_OF_MEMORY";          break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "INVALID_FRAMEBUFFER_OPERATION";  break;
			}

			cerr << "GL_" << error.c_str() << endl;
			err = glGetError();
			cout << "An error happened after texture setup" << endl;//Process/log the error.
		}
		glBindTexture(GL_TEXTURE_2D, m_TextureScene);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, m_ResLargeur, m_ResHauteur);
		glBindImageTexture(1, m_TextureScene, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		//Allocate texture
		/*glGenBuffers(1, &m_computedImage);
		glBindBuffer(GL_TEXTURE_BUFFER, m_computedImage);
		glBufferData(GL_TEXTURE_BUFFER, TypeSize(GL_FLOAT) * m_ResLargeur * m_ResHauteur * 3, NULL, GL_DYNAMIC_COPY);*/

		//GLuint glGenTextures()
		//GLuint imageIndex = glGetUniformLocation(prog, "renderedImage");

		/*glActiveTexture(GL_TEXTURE0);*/
		//glUniform1i(m_computedImage, 2);
		//glBindTexture(GL_TEXTURE_2D, m_TextureScene);
		

		//GLuint ssboBuf;
		//// Generate the buffer, bind it to create it and declare storage
		//GLuint ssboIndex = glGetProgramResourceIndex(prog, GL_SHADER_STORAGE_BLOCK, "ssbo");
		//GLuint ssboBindingIndex;
		//glShaderStorageBlockBinding(prog, ssboIndex, ssboBindingIndex);
		//glGenBuffers(1, &ssboBuf);
		//glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboBuf);
		//glBufferData(GL_SHADER_STORAGE_BUFFER, TypeSize(GL_FLOAT) * m_ResLargeur * m_ResHauteur * 3 , NULL, GL_DYNAMIC_COPY);
		//// Now bind the buffer to the zeroth GL_SHADER_STORAGE_BUFFER
		//// binding point
		//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboBuf);
		//glUniform1fv(infoPixelIndex, m_ResLargeur * m_ResHauteur * 3, )

		//// Créer une texture openGL
		//glGenTextures(1, &m_TextureScene);
		//glBindTexture(GL_TEXTURE_2D, m_TextureScene);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_ResLargeur, m_ResHauteur, 0, GL_RGB, GL_FLOAT, m_InfoPixel);


		size_t nbSurfaces = m_Surfaces.size();
		size_t nbQuadrics = m_quadricsIndexes.size();
		size_t nbTriangles = m_trianglesIndexes.size();
		size_t nbPlanes = m_planesIndexes.size();
		size_t nbLights = m_lightsIndexes.size();
		
		//DEBUG
		cout << "nbSurfaces = " << nbSurfaces << endl;
		cout << "nbQuadrics = " << nbQuadrics << endl;
		cout << "nbTriangles = " << nbTriangles << endl;
		cout << "nbPlanes = " << nbPlanes << endl;
		cout << "nbLights = " << nbLights << endl;

		//


		//Check if enough space in allocated surface buffers
		if (nbQuadrics > MAX_NB_QUADRICS || nbTriangles > MAX_NB_TRIANGLES || nbPlanes > MAX_NB_PLANES || nbLights > MAX_NB_LIGHTS)
		{
			std::cout << "[ERROR] : Too many surfaces in app to allocate it in the GLSL uniform struct";
			cout << "\tMAX_NB_QUADRICS = " << MAX_NB_QUADRICS << " | nb_quadrics = " << m_quadricsIndexes.size() << std::endl;
			cout << "\tMAX_NB_TRIANGLES =" << MAX_NB_TRIANGLES << " | nb_triangles = " << m_trianglesIndexes.size() << std::endl;
			cout << "\tMAX_NB_PLANES =" << MAX_NB_PLANES << " | nb_planes = " << m_planesIndexes.size() << std::endl;
			cout << "\tMAX_NB_LIGHTS =" << MAX_NB_LIGHTS << " | nb_lighs = " << m_lightsIndexes.size() << std::endl;
			exit(EXIT_FAILURE);
		}

		//Create structures similar to the GLSL ones (needed to generate uniform names)
		char* uniformNames[SCENEDATATYPE_SIZE] = {
			"quadrics",
			"triangles",
			"planes",
			"lights",
			"nbQuadrics",
			"nbTriangles",
			"nbPlanes",
			"nbLights"
		};

		const char* surfaceNames[SURFACETYPE_SIZE] = {
			"m_Couleur",
			"m_CoeffAmbient",
			"m_CoeffDiffus",
			"m_CoeffSpeculaire",
			"m_CoeffShininess",
			"m_CoeffReflexion",
			"m_CoeffRefraction",
			"m_IndiceDeRefraction"
		};


		const char* quadricNames[QUADRICTYPE_SIZE] = {
			"m_surface",
			"m_quadrique",
			"m_lineaire",
			"m_mixte",
			"m_cst"
		};
		const char* triangleNames[TRIANGLETYPE_SIZE] = {
			"m_surface",
			"m_points",
			"m_normal"
		};

		const char* planeNames[PLANETYPE_SIZE] = {
			"m_surface",
			"m_normal",
			"m_cst"
		};


		const char* lightNames[LIGHTTYPE_SIZE] = {
			"m_PositionLight",
			"m_CouleurLight",
			"m_IntensiteLight"
		};

		//size_t nbUniforms = nbQuadrics*QUADRICTYPE_SIZE + nbTriangles*TRIANGLETYPE_SIZE + nbPlanes*PLANETYPE_SIZE;

		//Init an array of names for each surface type
		int nbUnifQuadric = nbQuadrics*QUADRICTYPE_SIZE_ALL;
		int nbUnifTriangle = nbTriangles*TRIANGLETYPE_SIZE_ALL; // add '1' because there is an array of 3 points in "m_points", but also we do not count m_surface (because it's a struct. So "... + 2-1" = 1
		int nbUnifPlane = nbPlanes*PLANETYPE_SIZE_ALL;
		int nbUnifLights = nbLights*LIGHTTYPE_SIZE;


		char** quadricCompleteNames = new char*[nbUnifQuadric];
		for (size_t i = 0; i < nbUnifQuadric; i++)
		{
			quadricCompleteNames[i] = new char[MAX_VARNAME_SIZE];
		}

		char** triangleCompleteNames = new char*[nbUnifTriangle];
		for (size_t i = 0; i < nbUnifTriangle; i++)
		{
			triangleCompleteNames[i] = new char[MAX_VARNAME_SIZE];
		}

		char** planeCompleteNames = new char*[nbUnifPlane];
		for (size_t i = 0; i < nbUnifPlane; i++)
		{
			planeCompleteNames[i] = new char[MAX_VARNAME_SIZE];
		}

		char** lightCompleteNames = new char*[nbUnifLights];
		for (size_t i = 0; i < nbUnifLights; i++)
		{
			lightCompleteNames[i] = new char[MAX_VARNAME_SIZE];
		}

		//Fill char* array with all uniform name possible for each surface type
		
		//for quadrics
		for (size_t i = 0; i < nbQuadrics; i++)
		{
			int offset = i*QUADRICTYPE_SIZE_ALL;
			for (size_t j = 0; j < SURFACETYPE_SIZE; j++)
			{
				string s = std::to_string(i);
				strcpy(quadricCompleteNames[offset + j], uniformNames[QUADRIC]);
				strcat(quadricCompleteNames[offset + j], "[");
				strcat(quadricCompleteNames[offset + j], s.c_str());
				strcat(quadricCompleteNames[offset + j], "].m_surface.");
				strcat(quadricCompleteNames[offset + j], surfaceNames[j]);

			}
			
			offset += SURFACETYPE_SIZE;
			for (size_t j = 0; j < QUADRICTYPE_SIZE-1; j++)
			{
				string s = std::to_string(i);
				strcpy(quadricCompleteNames[offset + j], uniformNames[QUADRIC]);
				strcat(quadricCompleteNames[offset + j], "[");
				strcat(quadricCompleteNames[offset + j], s.c_str());
				strcat(quadricCompleteNames[offset + j], "].");
				strcat(quadricCompleteNames[offset + j], quadricNames[j+1]);
			}
		}

		//for triangles
		for (size_t i = 0; i < nbTriangles; i++)
		{
			int offset = i*TRIANGLETYPE_SIZE_ALL;
			for (size_t j = 0; j < SURFACETYPE_SIZE; j++)
			{
				string s = std::to_string(i);
				strcpy(triangleCompleteNames[offset + j], uniformNames[TRIANGLE]);
				strcat(triangleCompleteNames[offset + j], "[");
				strcat(triangleCompleteNames[offset + j], s.c_str());
				strcat(triangleCompleteNames[offset + j], "].m_surface.");
				strcat(triangleCompleteNames[offset + j], surfaceNames[j]);

			}

			offset += SURFACETYPE_SIZE;
			for (size_t j = 0; j < 3; j++)
			{
				string s = std::to_string(i);
				strcpy(triangleCompleteNames[offset + j], uniformNames[TRIANGLE]);
				strcat(triangleCompleteNames[offset + j], "[");
				strcat(triangleCompleteNames[offset + j], s.c_str());
				strcat(triangleCompleteNames[offset + j], "].m_points[");
				strcat(triangleCompleteNames[offset + j], std::to_string(j).c_str());
				strcat(triangleCompleteNames[offset + j], "]");
			}

			offset += 3;
			string s2 = std::to_string(i);
			strcpy(triangleCompleteNames[offset], uniformNames[TRIANGLE]);
			strcat(triangleCompleteNames[offset], "[");
			strcat(triangleCompleteNames[offset], s2.c_str());
			strcat(triangleCompleteNames[offset], "].m_normal");
		}

		//for planes
		for (size_t i = 0; i < nbPlanes; i++)
		{
			int offset = i*PLANETYPE_SIZE_ALL;
			for (size_t j = 0; j < SURFACETYPE_SIZE; j++)
			{
				string s = std::to_string(i);
				strcpy(planeCompleteNames[offset + j], uniformNames[PLANE]);
				strcat(planeCompleteNames[offset + j], "[");
				strcat(planeCompleteNames[offset + j], s.c_str());
				strcat(planeCompleteNames[offset + j], "].m_surface.");
				strcat(planeCompleteNames[offset + j], surfaceNames[j]);

			}

			offset += SURFACETYPE_SIZE;
			for (size_t j = 0; j < PLANETYPE_SIZE-1; j++)
			{
				string s = std::to_string(i);
				strcpy(planeCompleteNames[offset + j], uniformNames[PLANE]);
				strcat(planeCompleteNames[offset + j], "[");
				strcat(planeCompleteNames[offset + j], s.c_str());
				strcat(planeCompleteNames[offset + j], "].");
				strcat(planeCompleteNames[offset + j], planeNames[j+1]);
				//cout << "planes : " << offset + j << " | " << planeCompleteNames[offset + j] << endl;

			}
		}

		//for lights
		for (size_t i = 0; i < nbLights; i++)
		{
			int offset = i*LIGHTTYPE_SIZE;
			for (size_t j = 0; j < LIGHTTYPE_SIZE; j++)
			{
				string s = std::to_string(i);
				strcpy(lightCompleteNames[offset + j], uniformNames[LIGHT]);
				strcat(lightCompleteNames[offset + j], "[");
				strcat(lightCompleteNames[offset + j], s.c_str());
				strcat(lightCompleteNames[offset + j], "].");
				strcat(lightCompleteNames[offset + j], lightNames[j]);

			}
		}

		//****************************************************************************
		//DEBUG ZONE
		for (size_t i = 0; i < nbUnifQuadric; ++i) {
			cout << quadricCompleteNames[i] << endl;
		}
		cout << endl;
		

		for (size_t i = 0; i < nbUnifTriangle; ++i) {
			cout << triangleCompleteNames[i] << endl;
		}
		cout << endl;
		

		for (size_t i = 0; i < nbUnifPlane; ++i) {
			cout << planeCompleteNames[i] << endl;
		}
		cout << endl;

		for (size_t i = 0; i < nbUnifLights; ++i) {
			cout << lightCompleteNames[i] << endl;
		}
		cout << endl;

		//****************************************************************************

		//Based on the surface type and previous generated names for uniforms, get uniform index and update its value

		GLint** size = new GLint*[SCENEDATATYPE_SIZE];
		GLint** offset = new GLint*[SCENEDATATYPE_SIZE];
		GLint** type = new GLint*[SCENEDATATYPE_SIZE];

		//TODO : not good perf I think with this **pointer, should use ***pointer
		char** curNames;
		for (size_t i = 0; i < SCENEDATATYPE_SIZE; i++)
		{
			size_t curSize = 0;
			switch (i)
			{
			case QUADRIC :
				curSize = nbUnifQuadric;
				curNames = quadricCompleteNames;
				break;
			case TRIANGLE :
				curSize = nbUnifTriangle;
				curNames = triangleCompleteNames;
				break;
			case PLANE : 
				curSize = nbUnifPlane;
				curNames = planeCompleteNames;
				break;
			case LIGHT :
				curSize = nbUnifLights;
				curNames = lightCompleteNames;
				break;
			default:
				//cout << "[WARNING] : Not value matching for current size type :" << i << endl;
				curSize = 1;
				curNames = &uniformNames[i];
				break;
			}

			GLuint* indices = new GLuint[curSize];
			size[i] = new GLint[curSize];
			offset[i] = new GLint[curSize];
			type[i] = new GLint[curSize];


			glGetUniformIndices(prog, curSize, curNames, indices);


			//DEBUG
			for (size_t cpt = 0; cpt < curSize; cpt++)
			{
				cout << "indices[" << cpt << "] = " << indices[cpt] << endl;
			}
			cout << endl;
			//

			glGetActiveUniformsiv(prog, curSize, indices, GL_UNIFORM_OFFSET, offset[i]);
			glGetActiveUniformsiv(prog, curSize, indices, GL_UNIFORM_SIZE, size[i]);
			glGetActiveUniformsiv(prog, curSize, indices, GL_UNIFORM_TYPE, type[i]);

			//free memory
			delete[] indices;
		}

		err = glGetError();
		while (err != GL_NO_ERROR)
		{
			string error;
			switch (err) {
			case GL_INVALID_OPERATION:      error = "INVALID_OPERATION";      break;
			case GL_INVALID_ENUM:           error = "INVALID_ENUM";           break;
			case GL_INVALID_VALUE:          error = "INVALID_VALUE";          break;
			case GL_OUT_OF_MEMORY:          error = "OUT_OF_MEMORY";          break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "INVALID_FRAMEBUFFER_OPERATION";  break;
			}

			cerr << "GL_" << error.c_str() << endl;
			err = glGetError();
			cout << "An error happened before SceneData memcpy" << endl;//Process/log the error.
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

			size_t curIndex = i * QUADRICTYPE_SIZE_ALL;

			//For m_surface (struct)
			fillSurfaceStruct(SceneDataBuffer, offset[QUADRIC], size[QUADRIC], type[QUADRIC], curQuadric, curIndex);
			

			curIndex += SURFACETYPE_SIZE;

			//For m_quadrique (CVecteur3)
			CVecteur3 tmpVec = curQuadric->ObtenirQuadratique();
			float vecValues[4] = {tmpVec.x, tmpVec.y, tmpVec.z, 0.0};
			memcpy(SceneDataBuffer + offset[QUADRIC][curIndex], vecValues, size[QUADRIC][curIndex] * TypeSize(type[QUADRIC][curIndex]));

			//For m_lineaire (CVecteur3)
			tmpVec = curQuadric->ObtenirLineaire();
			float vecValues2[4] = { tmpVec.x, tmpVec.y, tmpVec.z, 0.0 };
			memcpy(SceneDataBuffer + offset[QUADRIC][curIndex + 1], vecValues2, size[QUADRIC][curIndex + 1] * TypeSize(type[QUADRIC][curIndex + 1]));

			//For m_mixte (CVecteur3)
			tmpVec = curQuadric->ObtenirQuadratique();
			float vecValues3[4] {tmpVec.x, tmpVec.y, tmpVec.z, 0.0};
			memcpy(SceneDataBuffer + offset[QUADRIC][curIndex + 2], vecValues3, size[QUADRIC][curIndex + 2] * TypeSize(type[QUADRIC][curIndex + 2]));

			//For m_cst (float)
			float value4 = curQuadric->ObtenirConstante();
			memcpy(SceneDataBuffer + offset[QUADRIC][curIndex + 3], &value4, size[QUADRIC][curIndex + 3] * TypeSize(type[QUADRIC][curIndex + 3]));
		}


		//fill data for triangles 
		for (size_t i = 0; i < nbTriangles; i++)
		{
			CTriangle* curTriangle = dynamic_cast<CTriangle*>(m_Surfaces[m_trianglesIndexes[i]]);

			if (curTriangle == nullptr)
			{
				cout << "[ERROR] : Value in m_trianglesIndexes is not an index to a CTriangle object" << endl;
				continue;
			}

			size_t curIndex = i * TRIANGLETYPE_SIZE_ALL;

			//For m_surface (struct)
			fillSurfaceStruct(SceneDataBuffer, offset[TRIANGLE], size[TRIANGLE], type[TRIANGLE], curTriangle, curIndex);

			curIndex += SURFACETYPE_SIZE;

			//For m_points[3] (3 * CVecteur3)
			CVecteur3 tmpVec;
			for (size_t cpt = 0; cpt < 3; cpt++)
			{
				tmpVec = curTriangle->ObtenirPoint(cpt);
				//TODO : Check if it does not cause an issue
				float vecValues[4] = { tmpVec.x, tmpVec.y, tmpVec.z, 0.0 }; 
				memcpy(SceneDataBuffer + offset[TRIANGLE][curIndex + cpt], vecValues, size[TRIANGLE][curIndex + cpt] * TypeSize(type[TRIANGLE][curIndex + cpt]));
			}
			
			//For m_normal (CVecteur3)
			tmpVec = curTriangle->ObtenirNormale();
			float vecValues2[4] = { tmpVec.x, tmpVec.y, tmpVec.z, 0.0 };
			memcpy(SceneDataBuffer + offset[TRIANGLE][curIndex + 3], vecValues2, size[TRIANGLE][curIndex + 3] * TypeSize(type[TRIANGLE][curIndex + 3]));
		}


		//TODO : Add for planes
		for (size_t i = 0; i < nbPlanes; i++)
		{
			CPlan* curPlane = dynamic_cast<CPlan*>(m_Surfaces[m_planesIndexes[i]]);

			if (curPlane == nullptr)
			{
				cout << "[ERROR] : Value in m_planesIndexes is not an index to a CPlan object" << endl;
				continue;
			}

			size_t curIndex = i * PLANETYPE_SIZE_ALL;

			//For m_surface (struct)
			fillSurfaceStruct(SceneDataBuffer, offset[PLANE], size[PLANE], type[PLANE], curPlane, curIndex);

			curIndex += SURFACETYPE_SIZE;

			//For m_normale (CVecteur3)
			CVecteur3 tmpVec = curPlane->ObtenirNormale();
			float vecValues[4] = { tmpVec.x, tmpVec.y, tmpVec.z, 0.0 };
			memcpy(SceneDataBuffer + offset[PLANE][curIndex], vecValues, size[PLANE][curIndex] * TypeSize(type[PLANE][curIndex]));

			//For m_cst (float)
			float value1 = curPlane->ObtenirConstante();
			memcpy(SceneDataBuffer + offset[PLANE][curIndex + 1], &value1, size[PLANE][curIndex + 1] * TypeSize(type[PLANE][curIndex + 1]));
		}


		//TODO : Add for lights
		for (size_t i = 0; i < nbLights; i++)
		{
			CLumiere* curLight = dynamic_cast<CLumiere*>(m_Lumieres[m_lightsIndexes[i]]);

			if (curLight == nullptr)
			{
				cout << "[ERROR] : Value in m_lightsIndexes is not an index to a CLumiere object" << endl;
				continue;
			}

			//For m_PositionLight (CVecteur3)
			CVecteur3 tmpVec = curLight->GetPosition();
			float vecValues[4] = { tmpVec.x, tmpVec.y, tmpVec.z, 0.0 };
			memcpy(SceneDataBuffer + offset[LIGHT][0], vecValues, size[LIGHT][0] * TypeSize(type[LIGHT][0]));

			//For m_CouleurLight (CVecteur3)
			CCouleur color = curLight->ObtenirCouleur();
			float vecValues2[4] = { color.r, color.g, color.b, 0.0 };
			memcpy(SceneDataBuffer + offset[LIGHT][1], vecValues2, size[LIGHT][1] * TypeSize(type[LIGHT][1]));

			//For m_IntensiteLight (float)
			float value3 = curLight->GetIntensity();
			memcpy(SceneDataBuffer + offset[LIGHT][2], &value3, size[LIGHT][2] * TypeSize(type[LIGHT][2]));
		}

		///Copying size attributes in the uniform block
		//nbQuadrics
		int dValue = nbQuadrics;
		memcpy(SceneDataBuffer + offset[4][0], &dValue, size[4][0] * TypeSize(type[4][0]));

		//nbTriangles
		dValue = nbTriangles;
		memcpy(SceneDataBuffer + offset[5][0], &dValue, size[5][0] * TypeSize(type[5][0]));

		//nbPlanes
		dValue = nbPlanes;
		memcpy(SceneDataBuffer + offset[6][0], &dValue, size[6][0] * TypeSize(type[6][0]));

		//nbLights
		dValue = nbLights;
		memcpy(SceneDataBuffer + offset[7][0], &dValue, size[7][0] * TypeSize(type[7][0]));

		err = glGetError();
		while (err != GL_NO_ERROR)
		{
			string error;
			switch (err) {
			case GL_INVALID_OPERATION:      error = "INVALID_OPERATION";      break;
			case GL_INVALID_ENUM:           error = "INVALID_ENUM";           break;
			case GL_INVALID_VALUE:          error = "INVALID_VALUE";          break;
			case GL_OUT_OF_MEMORY:          error = "OUT_OF_MEMORY";          break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "INVALID_FRAMEBUFFER_OPERATION";  break;
			}

			cerr << "GL_" << error.c_str() << endl;
			err = glGetError();
			cout << "An error happened in CSceneData memcpy" << endl;//Process/log the error.
		}

		//Create SceneDataBuffer
		GLuint SceneDataUBO;
		glGenBuffers(1, &SceneDataUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, SceneDataUBO);
		glBufferData(GL_UNIFORM_BUFFER, uniformSize, SceneDataBuffer, GL_STATIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, uniformIndex, SceneDataUBO);

		err = glGetError();
		while (err != GL_NO_ERROR)
		{
			string error;
			switch (err) {
			case GL_INVALID_OPERATION:      error = "INVALID_OPERATION";      break;
			case GL_INVALID_ENUM:           error = "INVALID_ENUM";           break;
			case GL_INVALID_VALUE:          error = "INVALID_VALUE";          break;
			case GL_OUT_OF_MEMORY:          error = "OUT_OF_MEMORY";          break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "INVALID_FRAMEBUFFER_OPERATION";  break;
			}

			cerr << "GL_" << error.c_str() << endl;
			err = glGetError();
			cout << "An error happened after CSceneData buffer creation" << endl;//Process/log the error.
		}
		//Clean memory
		for (size_t i = 0; i < nbQuadrics; i++)
		{
			delete quadricCompleteNames[i];
		}
		delete[] quadricCompleteNames;


		for (size_t i = 0; i < nbTriangles; i++)
		{
			delete triangleCompleteNames[i];
		}
		delete[] triangleCompleteNames;


		///TODO : commented just for DEBUG purpose
		//for (size_t i = 0; i < NB_SURFACETYPE; i++)
		//{
		//	//delete indices[i];
		//	delete size[i];
		//	delete offset[i];
		//	delete type[i];
		//}

		//delete[] indices;
		//delete[] size;
		//delete[] offset;
		//delete[] type;

		//Activate compute shader
		CVar::g_ComputeShader->activer();
		glDispatchCompute(m_ResLargeur / 16, m_ResHauteur/ 16, 1);
		
		
		//glDispatchCompute(CVar::g_LargeurViewport, CVar::g_HauteurViewport, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		///DEBUG
		//display uniforms variables results
		err = glGetError();

		while (err != GL_NO_ERROR)
		{
			string error;
			switch (err) {
			case GL_INVALID_OPERATION:      error = "INVALID_OPERATION";      break;
			case GL_INVALID_ENUM:           error = "INVALID_ENUM";           break;
			case GL_INVALID_VALUE:          error = "INVALID_VALUE";          break;
			case GL_OUT_OF_MEMORY:          error = "OUT_OF_MEMORY";          break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "INVALID_FRAMEBUFFER_OPERATION";  break;
			}

			cerr << "GL_" << error.c_str() << endl;
			err = glGetError();
			cout << "An error happened after CSceneData" << endl;//Process/log the error.
		}

		glBindBuffer(GL_UNIFORM_BUFFER, generalUbo);

		cout << "BEFORE get : ";
		cout << "\t cameraPosition = " << camPosition[0] << " | " << camPosition[1] << " | " << camPosition[2] << " | " << camPosition[3] << std::endl;

		float camPosition_OUT[4] = { 0.0, 0.0, 0.0, 0.0 };
		float camOrientation_OUT[4][4];
		float halfWidth_OUT = 0;
		float halfHeight_OUT = 0;
		float invResWidth_OUT = 0;
		float invResHeight_OUT = 0;
		float couleurArrierePlan_OUT[4] = { 0.0, 0.0, 0.0, 0.0 };


		glGetBufferSubData(GL_UNIFORM_BUFFER, generalOffset[0], generalSize[0] * TypeSize(generalType[0]), camPosition_OUT);
		glGetBufferSubData(GL_UNIFORM_BUFFER, generalOffset[1], generalSize[1] * TypeSize(generalType[1]), camOrientation_OUT);
		glGetBufferSubData(GL_UNIFORM_BUFFER, generalOffset[2], generalSize[2] * TypeSize(generalType[2]), &halfWidth_OUT);
		glGetBufferSubData(GL_UNIFORM_BUFFER, generalOffset[3], generalSize[3] * TypeSize(generalType[3]), &halfHeight_OUT);
		glGetBufferSubData(GL_UNIFORM_BUFFER, generalOffset[4], generalSize[4] * TypeSize(generalType[4]), &invResWidth_OUT);
		glGetBufferSubData(GL_UNIFORM_BUFFER, generalOffset[5], generalSize[5] * TypeSize(generalType[5]), &invResHeight_OUT);
		glGetBufferSubData(GL_UNIFORM_BUFFER, generalOffset[6], generalSize[6] * TypeSize(generalType[6]), couleurArrierePlan_OUT);


		///Debug GENERAL uniform
		glBindBuffer(GL_UNIFORM_BUFFER, generalUbo);

		cout << "AFTER get : ";
		cout << "\t cameraPosition = " << camPosition_OUT[0] << " | " << camPosition_OUT[1] << " | " << camPosition_OUT[2] << " | " << camPosition_OUT[3] << std::endl;
		cout << "\t camOrientation = " << camOrientation_OUT[0][0] << " | " << camOrientation_OUT[0][1] << " | " << camOrientation_OUT[0][2] << " | " << camOrientation_OUT[0][3] << std::endl;
		cout << "\t halfWidth_OUT = " << halfWidth_OUT << std::endl;
		cout << "\t halfHeight_OUT = " << halfHeight_OUT << std::endl;
		cout << "\t invResWidth_OUT = " << invResWidth_OUT << std::endl;
		cout << "\t invResHeight_OUT = " << invResHeight_OUT << std::endl;
		cout << "\t couleurArrierePlan_OUT = " << couleurArrierePlan_OUT[0] << " | " << couleurArrierePlan_OUT[1] << " | " << couleurArrierePlan_OUT[2] << " | " << couleurArrierePlan_OUT[3] << std::endl;

		///
		err = glGetError();

		while (err != GL_NO_ERROR)
		{
			string error;
			switch (err) {
			case GL_INVALID_OPERATION:      error = "INVALID_OPERATION";      break;
			case GL_INVALID_ENUM:           error = "INVALID_ENUM";           break;
			case GL_INVALID_VALUE:          error = "INVALID_VALUE";          break;
			case GL_OUT_OF_MEMORY:          error = "OUT_OF_MEMORY";          break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "INVALID_FRAMEBUFFER_OPERATION";  break;
			}

			cerr << "GL_" << error.c_str() << endl;
			err = glGetError();
			cout << "An error happened DEBUG test" << endl;//Process/log the error.
		}


		///DEBUG SceneData
		glBindBuffer(GL_UNIFORM_BUFFER, SceneDataUBO);

		cout << "AFTER get : ";
		
		for (size_t i = 0; i < nbQuadrics; i++)
		{
			CQuadrique* curQuadric = dynamic_cast<CQuadrique*>(m_Surfaces[m_quadricsIndexes[i]]);

			if (curQuadric == nullptr)
			{
				cout << "[ERROR] : Value in m_quadricsIndexes is not an index to a CQuadric object" << endl;
				continue;
			}

			size_t curIndex = i * QUADRICTYPE_SIZE_ALL;

			//For m_surface (struct)
			printSurfaceStruct(SceneDataBuffer, offset[QUADRIC], size[QUADRIC], type[QUADRIC], curQuadric, curIndex);


			curIndex += SURFACETYPE_SIZE;

			//For m_quadrique (CVecteur3)
			float vecValues4[4];
			glGetBufferSubData(GL_UNIFORM_BUFFER, offset[QUADRIC][curIndex], size[QUADRIC][curIndex] * TypeSize(type[QUADRIC][curIndex]), vecValues4);

			cout << "\t m_quadrique = " << vecValues4[0] << " | " << vecValues4[1] << " | " << vecValues4[2] << " | " << vecValues4[3] << std::endl;

			//For m_lineaire (CVecteur3)
			glGetBufferSubData(GL_UNIFORM_BUFFER, offset[QUADRIC][curIndex + 1], size[QUADRIC][curIndex + 1] * TypeSize(type[QUADRIC][curIndex + 1]), vecValues4);

			cout << "\t m_lineaire = " << vecValues4[0] << " | " << vecValues4[1] << " | " << vecValues4[2] << " | " << vecValues4[3] << std::endl;

			//For m_mixte (CVecteur3)
			glGetBufferSubData(GL_UNIFORM_BUFFER, offset[QUADRIC][curIndex + 2], size[QUADRIC][curIndex + 2] * TypeSize(type[QUADRIC][curIndex + 2]), vecValues4);

			cout << "\t m_mixte = " << vecValues4[0] << " | " << vecValues4[1] << " | " << vecValues4[2] << " | " << vecValues4[3] << std::endl;

			//For m_cst (float)
			float fValue;
			glGetBufferSubData(GL_UNIFORM_BUFFER, offset[QUADRIC][curIndex + 3], size[QUADRIC][curIndex + 3] * TypeSize(type[QUADRIC][curIndex + 3]), &fValue);

			cout << "\t m_cst = " << fValue << std::endl;

		}


		//fill data for triangles 
		for (size_t i = 0; i < nbTriangles; i++)
		{
			CTriangle* curTriangle = dynamic_cast<CTriangle*>(m_Surfaces[m_trianglesIndexes[i]]);

			if (curTriangle == nullptr)
			{
				cout << "[ERROR] : Value in m_trianglesIndexes is not an index to a CTriangle object" << endl;
				continue;
			}

			size_t curIndex = i * TRIANGLETYPE_SIZE_ALL;

			//For m_surface (struct)
			printSurfaceStruct(SceneDataBuffer, offset[TRIANGLE], size[TRIANGLE], type[TRIANGLE], curTriangle, curIndex);

			curIndex += SURFACETYPE_SIZE;

			//For m_points[3] (3 * CVecteur3)
			CVecteur3 tmpVec;
			for (size_t cpt = 0; cpt < 3; cpt++)
			{
				tmpVec = curTriangle->ObtenirPoint(cpt);
				//TODO : Check if it does not cause an issue
				float vecValues4[4];
				glGetBufferSubData(GL_UNIFORM_BUFFER, offset[TRIANGLE][curIndex + cpt], size[TRIANGLE][curIndex + cpt] * TypeSize(type[TRIANGLE][curIndex + cpt]), vecValues4);

				cout << "\t m_points = " << vecValues4[0] << " | " << vecValues4[1] << " | " << vecValues4[2] << " | " << vecValues4[3] << std::endl;
			}

			//For m_normal (CVecteur3)
			float vecValues4[4];
			glGetBufferSubData(GL_UNIFORM_BUFFER, offset[TRIANGLE][curIndex + 3], size[TRIANGLE][curIndex + 3] * TypeSize(type[TRIANGLE][curIndex + 3]), vecValues4);

			cout << "\t m_normal = " << vecValues4[0] << " | " << vecValues4[1] << " | " << vecValues4[2] << " | " << vecValues4[3] << std::endl;
		}


		//TODO : Add for planes
		for (size_t i = 0; i < nbPlanes; i++)
		{
			CPlan* curPlane = dynamic_cast<CPlan*>(m_Surfaces[m_planesIndexes[i]]);

			if (curPlane == nullptr)
			{
				cout << "[ERROR] : Value in m_planesIndexes is not an index to a CPlan object" << endl;
				continue;
			}

			size_t curIndex = i * PLANETYPE_SIZE_ALL;

			//For m_surface (struct)
			printSurfaceStruct(SceneDataBuffer, offset[PLANE], size[PLANE], type[PLANE], curPlane, curIndex);

			curIndex += SURFACETYPE_SIZE;

			//For m_normale (CVecteur3)
			float vecValues4[4];
			glGetBufferSubData(GL_UNIFORM_BUFFER, offset[PLANE][curIndex], size[PLANE][curIndex] * TypeSize(type[PLANE][curIndex]), vecValues4);

			cout << "\t m_normale = " << vecValues4[0] << " | " << vecValues4[1] << " | " << vecValues4[2] << " | " << vecValues4[3] << std::endl;

			//For m_cst (float)
			float fValue;
			glGetBufferSubData(GL_UNIFORM_BUFFER, offset[PLANE][curIndex + 1], size[PLANE][curIndex + 1] * TypeSize(type[PLANE][curIndex + 1]), &fValue);

			cout << "\t m_cst = " << fValue << std::endl;
		}


		//TODO : Add for lights
		for (size_t i = 0; i < nbLights; i++)
		{
			CLumiere* curLight = dynamic_cast<CLumiere*>(m_Lumieres[m_lightsIndexes[i]]);

			if (curLight == nullptr)
			{
				cout << "[ERROR] : Value in m_lightsIndexes is not an index to a CLumiere object" << endl;
				continue;
			}

			//For m_PositionLight (CVecteur3)
			float vecValues4[4];
			glGetBufferSubData(GL_UNIFORM_BUFFER, offset[LIGHT][0], size[LIGHT][0] * TypeSize(type[LIGHT][0]), vecValues4);

			cout << "\t m_PositionLight = " << vecValues4[0] << " | " << vecValues4[1] << " | " << vecValues4[2] << " | " << vecValues4[3] << std::endl;

			//For m_CouleurLight (CVecteur3)
			glGetBufferSubData(GL_UNIFORM_BUFFER, offset[LIGHT][1], size[LIGHT][1] * TypeSize(type[LIGHT][1]), vecValues4);

			cout << "\t m_PositionLight = " << vecValues4[0] << " | " << vecValues4[1] << " | " << vecValues4[2] << " | " << vecValues4[3] << std::endl;

			//For m_IntensiteLight (float)
			float fValue;
			glGetBufferSubData(GL_UNIFORM_BUFFER, offset[LIGHT][2], size[LIGHT][2] * TypeSize(type[LIGHT][2]), &fValue);

			cout << "\t m_IntensiteLight = " << fValue << std::endl;
		}

		///Copying size attributes in the uniform block
		//nbQuadrics
		int sizetValue;
		glGetBufferSubData(GL_UNIFORM_BUFFER, offset[4][0], size[4][0] * TypeSize(type[4][0]), &sizetValue);

		cout << "\t nbQuadrics = " << sizetValue << std::endl;

		//nbTriangles
		glGetBufferSubData(GL_UNIFORM_BUFFER, offset[5][0], size[5][0] * TypeSize(type[5][0]), &sizetValue);

		cout << "\t nbTriangles = " << sizetValue << std::endl;

		//nbPlanes
		glGetBufferSubData(GL_UNIFORM_BUFFER, offset[6][0], size[6][0] * TypeSize(type[6][0]), &sizetValue);

		cout << "\t nbPlanes = " << sizetValue << std::endl;

		//nbLights
		glGetBufferSubData(GL_UNIFORM_BUFFER, offset[7][0], size[7][0] * TypeSize(type[7][0]), &sizetValue);

		cout << "\t nbLights = " << sizetValue << std::endl;

		///
		err = glGetError();

		while (err != GL_NO_ERROR)
		{
			string error;
			switch (err) {
			case GL_INVALID_OPERATION:      error = "INVALID_OPERATION";      break;
			case GL_INVALID_ENUM:           error = "INVALID_ENUM";           break;
			case GL_INVALID_VALUE:          error = "INVALID_VALUE";          break;
			case GL_OUT_OF_MEMORY:          error = "OUT_OF_MEMORY";          break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "INVALID_FRAMEBUFFER_OPERATION";  break;
			}

			cerr << "GL_" << error.c_str() << endl;
			err = glGetError();
			cout << "An error happened DEBUG test" << endl;//Process/log the error.
		}

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
	//Specify texture image from data in m_InfoPixel 
	//TODO : Check this TEXTURE0 +1
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, m_TextureScene);

	if (CVar::g_ComputerShadersON)
	{
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_ResLargeur, m_ResHauteur, 0, GL_RGB, GL_FLOAT, NULL);
			
			
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_ResLargeur, m_ResHauteur, 0, GL_RGB, GL_FLOAT, m_InfoPixel);
	}
}



void CScene::fillSurfaceStruct(GLubyte*& buffer, GLint* offset, GLint* size, GLint* type, ISurface* surface, int index)
{

	//couleur
	CCouleur tmpCol = surface->ObtenirCouleur();
	float valCol[4] = { tmpCol.r, tmpCol.g, tmpCol.b, 0.0 };
	memcpy(buffer + offset[index], valCol, size[index] * TypeSize(type[index]));

	//m_coefAmbient
	float tmpCoef1 = surface->ObtenirCoeffAmbiant();
	memcpy(buffer + offset[index + 1], &tmpCoef1, size[index + 1] * TypeSize(type[index + 1]));

	//Coef diffus
	float tmpCoef2 = surface->ObtenirCoeffDiffus();
	memcpy(buffer + offset[index + 2], &tmpCoef2, size[index + 2] * TypeSize(type[index + 2]));

	//coef specular
	float tmpCoef3 = surface->ObtenirCoeffSpeculaire();
	memcpy(buffer + offset[index + 3], &tmpCoef3, size[index + 3] * TypeSize(type[index + 3]));

	// coef shininess
	float tmpCoef4 = surface->ObtenirCoeffBrillance();
	memcpy(buffer + offset[index + 4], &tmpCoef4, size[index + 4] * TypeSize(type[index + 4]));

	//coef reflexion
	float tmpCoef5 = surface->ObtenirCoeffReflexion();
	memcpy(buffer + offset[index + 5], &tmpCoef5, size[index + 5] * TypeSize(type[index + 5]));

	// coef refraction
	float tmpCoef6 = surface->ObtenirCoeffRefraction();
	memcpy(buffer + offset[index + 6], &tmpCoef6, size[index + 6] * TypeSize(type[index + 6]));

	// indice refraction
	float tmpCoef7 = surface->ObtenirIndiceRefraction();
	memcpy(buffer + offset[index + 7], &tmpCoef7, size[index + 7] * TypeSize(type[index + 7]));
	
	return;
}


void CScene::printSurfaceStruct(GLubyte*& buffer, GLint* offset, GLint* size, GLint* type, ISurface* surface, int index)
{
	//couleur
	float vecValues4[4];
	glGetBufferSubData(GL_UNIFORM_BUFFER, offset[index], size[index] * TypeSize(type[index]), vecValues4);

	cout << "\t m_couleur = " << vecValues4[0] << " | " << vecValues4[1] << " | " << vecValues4[2] << " | " << vecValues4[3] << std::endl;

	//m_coefAmbient
	float fValue;
	glGetBufferSubData(GL_UNIFORM_BUFFER, offset[index + 1], size[index + 1] * TypeSize(type[index + 1]), &fValue);

	cout << "\t m_coefAmbient = " << fValue << std::endl;

	//Coef diffus
	glGetBufferSubData(GL_UNIFORM_BUFFER, offset[index + 2], size[index + 2] * TypeSize(type[index + 2]), &fValue);
	cout << "\t m_coefDiffus = " << fValue << std::endl;

	//coef specular
	glGetBufferSubData(GL_UNIFORM_BUFFER, offset[index + 3], size[index + 3] * TypeSize(type[index + 3]), &fValue);
	cout << "\t m_coefSpecular = " << fValue << std::endl;

	// coef shininess
	glGetBufferSubData(GL_UNIFORM_BUFFER, offset[index + 4], size[index + 4] * TypeSize(type[index + 4]), &fValue);
	cout << "\t m_coefShiness = " << fValue << std::endl;

	//coef reflexion
	glGetBufferSubData(GL_UNIFORM_BUFFER, offset[index + 5], size[index + 5] * TypeSize(type[index + 5]), &fValue);
	cout << "\t m_coefReflexion = " << fValue << std::endl;

	// coef refraction
	glGetBufferSubData(GL_UNIFORM_BUFFER, offset[index + 6], size[index + 6] * TypeSize(type[index + 6]), &fValue);
	cout << "\t m_coefrefraction = " << fValue << std::endl;

	// indice refraction
	glGetBufferSubData(GL_UNIFORM_BUFFER, offset[index + 7], size[index + 7] * TypeSize(type[index + 7]), &fValue);
	cout << "\t m_indice refraction = " << fValue << std::endl;

	return;
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
		//WARNING : double size added ! Should be the right value (see https://www.opengl.org/wiki/OpenGL_Type)
		CASE(GL_DOUBLE, 1, GLdouble);
		//
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
