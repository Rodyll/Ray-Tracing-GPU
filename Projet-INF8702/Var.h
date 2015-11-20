#ifndef __VAR_H__
#define __VAR_H__

#include "Singleton.h"
#include "Scene.h"

using namespace Scene;

///////////////////////////////////////////////////////////////////////////////
///  CVar
///  Cette classe conserve toutes les variables globales du programme
///
///  Base class Singleton
///
///  @author Olivier Dionne 
///  @date   13/08/2008 
///
///////////////////////////////////////////////////////////////////////////////
class CVar : public Singleton<CVar>
{

    SINGLETON_DECLARATION_CLASSE(CVar);

   public:
   
        /// L'Identificateur de la fenêtre GLUT
        static int g_FenetreID;
        /// Largeur de la fenêtre GLUT
        static int g_LargeurFenetre;
        /// Hauteur de la fenêtre GLUT
        static int g_HauteurFenetre;

        /// Largeur du viewport
        static int g_LargeurViewport;
        /// Hauteur du viewport
        static int g_HauteurViewport;

        /// Gestionnaire de scène
        static CScene* g_GestionnaireDeScene;
};

#endif
