///////////////////////////////////////////////////////////////////////////////
///  @file Main.cpp
///
///  @brief    Cette application implémente un engin simple de lancer de rayons a 
///            des fins éducationnelles du cours "Infographie avancée - INF8702".
///            L'implémentation et le design original du lancer de rayons de Polytechnique
///            fut écrite par Yves Martel (2003).
///            Cette version est plus modulaire et est plus facile à maintenir et à comprendre
///            Elle fut également conçue de telle façon à pouvoir être portée facilement sur GPU
///            en GLSL. Une démo technique de la version "temps réel" est déjà disponible.
///            Vous n'avez qu'à demander à votre chargé de Labo.
///
///  @remarks  Les vieux fichiers de donnnées (.dat) ne fonctionne plus avec cette version.
///            Vous devrez utiliser les fichiers .dat fournis avec le présent projet jusqu'à ce
///            qu'un meilleur format de fichier soit implémenté. Un format simple comme OBJ est 
///            présentement considéré à cet égard.
///
///  @author  Olivier Dionne 
///  @date    13/08/2008
///  @version 1.1.0
///
///////////////////////////////////////////////////////////////////////////////
#if _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <GL/glew.h>
#include <GL/glut.h>

#include "Cst.h"
#include "Var.h"
#include "NuanceurCalculProg.h"

using namespace std;

// Déclarations des méthodes
void Initialiser         ( void );
void Redimensionner      ( int w, int h );
void RafraichirProjection( void );
void Liberer             ( void );
void Dessiner            ( void );
string getSourceCode(const char *filename);

int main( int argc, char *argv[] )
{
	// Traiter les arguments d'entrée
	if( argc > 1 )
	{
		int XRes = CVar::g_LargeurFenetre;
		int YRes = CVar::g_HauteurFenetre;

		if( argc > 2 )
		{
			for( int i = 2; i < argc; i++ )
			{
				if( *argv[ i ] == '-' )
				{
					switch( argv[ i ][ 1 ] )
					{
						case 'x':
							XRes = atoi( argv[ ++i ] );
							break;
						case 'y':
							YRes = atoi( argv[ ++i ] );
							break;
					}
				}
			}
		}

		// S'assurer que la résolution fournie est une puissance de deux.
		if( ( ( XRes - 1 ) & XRes ) || ( ( YRes - 1 ) & YRes ) )
		{
			cerr<< "[ERREUR]: Resolution " << XRes << "x" << YRes << " n'est pas une puissance de deux" << endl;
			system( "pause" );
			exit( 1 );
		}

		// Construire la scène
		CVar::g_GestionnaireDeScene = CScene::ObtenirInstance();

		// Ajuster la résolution de la scène
		CVar::g_LargeurFenetre = XRes;
		CVar::g_HauteurFenetre = YRes;
		CVar::g_GestionnaireDeScene->AjusterResolution( CVar::g_LargeurFenetre, CVar::g_HauteurFenetre );

		// Traiter le fichier de données de la scène
		cout << "[ETAT]: Traitement du fichier de donnees de la scene..." << endl;
		CVar::g_GestionnaireDeScene->TraiterFichierDeScene( argv[ 1 ] );


		cout << "[ETAT]: Initialisation de glut...";
		// Initialiser GLUT
		glutInit( &argc, argv );
		// Définir le mode d'affichage de GLUT
		glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA );
		// Définir les dimensions de la fenêtre GLUT
		glutInitWindowSize( CVar::g_LargeurFenetre, CVar::g_HauteurFenetre );
		// Créer la fenêtre GLUT et enregistrer l'ID
		CVar::g_FenetreID = glutCreateWindow( "INF8702 - Lancer de rayon sur CPU/GPU" );
		// Positionner la fenêtre GLUT
		glutPositionWindow( CCst::g_FenetreX, CCst::g_FenetreY );

		// Dire à GLUT d'utiliser la méthode "Dessiner" comme principale fonction d'affichage
		glutDisplayFunc( Dessiner );
		// Dire à GLUT d'utiliser la méthode "Redimensionner" comme principale fonction de redimensionnement
		glutReshapeFunc( Redimensionner );
		
		cout << "OK" << endl;
	   
	}
	else
	{
		cerr << "[ERREUR]: Aucune fichier de scene ne fut passe en argument !" << endl;
		system( "pause" );
		exit( 1 );
	}


	//Initialiser GLEW
	cout << "[ETAT]: Initialisation de glew...";

	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		cout << "[ERREUR]: Initialisation de glew a retournee une erreur : " << glewGetErrorString(err) << endl;
		exit(EXIT_FAILURE);
	}
	else
	{
		cout << "OK" << endl;
	}

	// Initialiser OpenGL
	Initialiser();
	// Boucle principale
	glutMainLoop();

	// Libérer les données de l'application (jamais appelé à cause de l'appel à glutMainLoop())
	Liberer();

	return EXIT_SUCCESS;
}

void Initialiser( void )
{
	// Activer le test de profondeur d'OpenGL
	glEnable( GL_DEPTH_TEST );
	
	// Définir la fonction du test de profondeur
	glDepthFunc( GL_LEQUAL );
	
	//Creating a compute shader (if GPU mode selected)
	if (CVar::g_ComputerShadersON)
	{	
		CVar::g_ComputeShader = new CNuanceurCalculProg(CVar::g_ComputeShaderPath, true);
	}

	cout << "[ETAT]: Lancer de rayons..." << endl;

	// Obtenir le temps courant
#if _WIN32
	double Time;
	LARGE_INTEGER lTime, lFrequency;
	QueryPerformanceCounter( &lTime );
	QueryPerformanceFrequency( &lFrequency );
	Time = static_cast<double>( lTime.QuadPart ) / static_cast<double>( lFrequency.QuadPart );
#else
	timeval Time;
	gettimeofday( &Time, 0 );
#endif

	// Générer la scène par lancers de rayons
	CVar::g_GestionnaireDeScene->LancerRayons();

	// Calculer le temps pris
#if _WIN32
	QueryPerformanceCounter( &lTime );
	QueryPerformanceFrequency( &lFrequency );
	Time = static_cast<double>( lTime.QuadPart ) / static_cast<double>( lFrequency.QuadPart ) - Time;
#else
	timeval CurrentTime;
	gettimeofday( &CurrentTime, 0 );

	const double T1 = static_cast<double>( Time.tv_sec ) + static_cast<double>( Time.tv_usec / ( 1000 * 1000 ) );
	const double T2 = static_cast<double>( CurrentTime.tv_sec ) + static_cast<double>( CurrentTime.tv_usec / ( 1000 * 1000 ) );

	Time = T2 - T1;
#endif

   cout << "[ETAT]: Termine! --> Temps total de rendu : " << Time << " secondes" << endl;
}


void Redimensionner( int w, int h )
{
	CVar::g_LargeurViewport = w;
	CVar::g_HauteurViewport = h;
	glViewport( 0, 0, w, h );

	RafraichirProjection();
}

void RafraichirProjection( void )
{
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluOrtho2D( 0, 1, 0, 1 );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
}

void Liberer( void )
{
	//free shader object
	delete CVar::g_ComputeShader;

	CVar::g_GestionnaireDeScene->LibererInstance();
	CVar::g_GestionnaireDeScene = NULL;
}

void Dessiner( void )
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glEnable( GL_TEXTURE_2D );
	
	// Lier la texture de scène au quad OpenGL
	glBindTexture( GL_TEXTURE_2D, CVar::g_GestionnaireDeScene->ObtenirTextureGL() );
	
	glBegin( GL_QUADS );
	{
		glTexCoord2i( 0, 0 ); glVertex2i( 0, 0 );
		glTexCoord2i( 1, 0 ); glVertex2i( 1, 0 );
		glTexCoord2i( 1, 1 ); glVertex2i( 1, 1 );
		glTexCoord2i( 0, 1 ); glVertex2i( 0, 1 );
	}
	glEnd();

	glDisable( GL_TEXTURE_2D );

	glutSwapBuffers();
}

