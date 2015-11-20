#ifndef __SCENE_H__
#define __SCENE_H__

#include <vector>
#include <GL/glew.h>
#include "Math.h"
#include "Vecteur3.h"
#include "Matrice4.h"
#include "Singleton.h"
#include "ISurface.h"
#include "Rayon.h"
#include "Lumiere.h"
#include "Couleur.h"

using namespace Math3D;

namespace Scene 
{
    ///////////////////////////////////////////////////////////////////////////////
    ///  CScene
    ///  Cette classe conserve et g�re toutes les informations d�crivant la sc�ne
    ///  Elle impl�mente l'interface Singleton
    ///
    ///  Base class Singleton
    ///
    ///  @author Olivier Dionne 
    ///  @date   13/08/2008 
    ///
    ///////////////////////////////////////////////////////////////////////////////
    class CScene : public Singleton<CScene>
    {
        SINGLETON_DECLARATION_CLASSE_SANS_CONSTRUCTEUR( CScene );
        
        /// Constructor
        CScene( void );
        /// Destructeur
        ~CScene( void );

        public:
            /// Ajoute une surface � la sc�ne
            void AjouterSurface( ISurface* const Surface );
            /// Ajoute une lumi�re � la sc�ne
            void AjouterLumiere( CLumiere* const Lumiere );

            /// Effectue le lancer de rayons de la sc�ne
            void LancerRayons( void );

            /// M�thodes d'ajustement de la cam�ra
            void AjusterPositionCamera ( const CVecteur3& Position );
            void AjusterPointViseCamera( const CVecteur3& PointVise );
            void AjusterVecteurUpCamera( const CVecteur3& Up );

            /// M�thodes d'ajustement des propri�t�s de la sc�ne
            void AjusterResolution        ( const int ResX, const int ResY );
            void AjusterCouleurArrierePlan( const CCouleur& Couleur );
            void AjusterNbRebondsMax      ( const int NbRebondsMax );
            void AjusterEnergieMinimale   ( const REAL EnergieMinimale );
            void AjusterIndiceRefraction  ( const REAL IndiceRefraction );
            
            /// Obtenir la texture openGL du rendu de la sc�ne
            GLuint ObtenirTextureGL( void );

            /// Traiter le ficheir de donn�es de la sc�ne
            void TraiterFichierDeScene( const char* Fichier );

            /// Release all scene data
            void Liberer( void );

        private:

            /// Parser state enum
            enum EtatTraitementScene
            { 
                TRAITEMENT_SCENE,
                TRAITEMENT_LUMIERE,
                TRAITEMENT_TRIANGLE,
                TRAITEMENT_PLAN,
                TRAITEMENT_QUADRIQUE
            };

            /// Nombre maximum de caract�res pour le tampon de traitement
            const static int NB_MAX_CAR_PAR_LIGNE = 80;
            /// Les dimensions du film de la cam�ra
            const static REAL DIM_FILM_CAM;

            ///////////////////////////////////////////////////////////////////////////////
            ///  CameraDeScene
            ///  Petite structure encapsulant une cam�ra de sc�ne
            ///
            ///  @author Olivier Dionne @date 13/08/2008
            ///////////////////////////////////////////////////////////////////////////////
            struct CameraDeScene
            {
                /// Position de la cam�ra
                CVecteur3 Position;
                /// Point vis� de la cam�ra
                CVecteur3 PointVise;
                /// Le vecteur "up" de la cam�ra
                CVecteur3 Up;
                /// L'orientation de la cam�ra
                CMatrice4 Orientation;
                /// La focale de la cam�ra
                REAL      Focale;
                /// L'angle d'ouverture de la cam�ra
                REAL      Angle;
            };

            /// Useful std typedefs
            typedef std::vector< ISurface* >       SurfaceVecteur;
            typedef SurfaceVecteur::const_iterator SurfaceIterator;
            typedef std::vector< CLumiere* >       LumiereVecteur;
            typedef LumiereVecteur::const_iterator LumiereIterator;

            /// Initialise la sc�ne
            void Initialiser( void );
            /// Initialise la camera
            void InitialiserCamera( void );
            /// Obtenir la couleur du rayon
            const CCouleur ObtenirCouleur( const CRayon& Rayon ) const;
            /// Obtenir la couleur au point d'intersection
            const CCouleur ObtenirCouleurSurIntersection( const CRayon& Rayon, const CIntersection& Intersection ) const;
            /// Obtenir le filtre du mat�riau de surface
            const CCouleur ObtenirFiltreDeSurface( CRayon& LumiereRayon ) const;
            
            /// Resolution en largeur
            int      m_ResLargeur;
            /// Resolution en hauteur
            int      m_ResHauteur;
            /// Donn�es ( r, g, b ) du pixel
            GLfloat* m_InfoPixel;
            /// Texture de rendu openGL
            GLuint   m_TextureScene;

            /// Cam�ra de sc�ne
            CameraDeScene   m_Camera;
            /// La couleur de l'arri�re-plan
            CCouleur        m_CouleurArrierePlan;
            /// Le conteneur des surfaces de la sc�ne
            SurfaceVecteur  m_Surfaces;
            /// Le conteneur des lumi�res de la sc�ne
            LumiereVecteur  m_Lumieres;
            /// L'indice de r�fraction de la sc�ne
            REAL            m_IndiceRefractionScene;

            /// Le nombre maximum de rebonds permis
            int  m_NbRebondsMax;
            /// L'�nergie minimale d'un rayon
            REAL m_EnergieMinRayon;
    };
}

#endif
