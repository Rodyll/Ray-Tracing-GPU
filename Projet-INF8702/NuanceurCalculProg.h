///////////////////////////////////////////////////////////////////////////////
///  @file NuanceurCalculProg.h
///  @author  Frédéric Plourde 
///  @brief   Ce fichier encapsule la classe CNuanceurCalculProg, une classe utilitaire
///           contenant un shader et capable d'effectuer plusieurs fonctions
///           pratiques de gestion des nuanceurs.
///  @date    2007-12-12
///  @version 1.0
///
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include <GL/glew.h>
#include <stdio.h>
#include <vector>
#include "textfile.h"
#include "stdlib.h"
#include "assert.h"
#include "Cst.h"


///////////////////////////////////////////////////////////////////////////////
///  floatUniform
///  Structure encapsulant un uniform de type float
///
///  @remarks cette structure possède un contructeur assignant nom et val
///
///  @author Frédéric Plourde @date 2007-12-14
///////////////////////////////////////////////////////////////////////////////
struct floatUniform {
   const char*  nom;
   const float* val;

   floatUniform(const char* n, const float* v)
   {
      nom = n;
      val = v;
   }
};


///////////////////////////////////////////////////////////////////////////////
///  integerUniform
///  Structure encapsulant un uniform de type integer
///
///  @remarks cette structure possède un contructeur assignant nom et val
///
///  @author Frédéric Plourde @date 2007-12-14
///////////////////////////////////////////////////////////////////////////////
struct integerUniform {
   const char* nom;
   const int*  val;

   integerUniform(const char* n, const int* v)
   {
      nom = n;
      val = v;
   }
};


///////////////////////////////////////////////////////////////////////////////
///  @class CNuanceurCalculProg
///  @brief Classe qui encapsule un programme de nuanceurs (1 programme + 1 ou 2 nuanceurs)
///
///
///  @remarks le programme de nuanceur peut être vide (0 nuanceurs) pour simuler la fonctionalité fixe d'openGL
///
///  @author Frédéric Plourde 
///  @date   2007-12-12
///
///////////////////////////////////////////////////////////////////////////////
class CNuanceurCalculProg
{
public:
   /// constructeur par défaut : utilisé pour créer un PROGRAMME VKDE de nuanceur (0)
   CNuanceurCalculProg(void);
   /// constructeur spécifique : utilisé pour créer un PROGRAMME DE NUANCEURS.
   CNuanceurCalculProg(char* nuanceurCalculsStr, bool compilerImmediatement);
   /// destructeur par défaut.
   ~CNuanceurCalculProg(void);

   /// compile et lie dans openGL les nuanceurs du programme
   void compilerEtLier();

   /// Activer le nuanceur dans openGL
   void activer();

   /// Permet de lancer une valeur uniform (int) immédiatement
   void CNuanceurCalculProg::uniform1(char* nom, int v);

   /// Permet de lancer une valeur uniform (float) immédiatement
   void CNuanceurCalculProg::uniform1(char* nom, float v);

   /// Enregistre au sein du programme un uniform float qui sera inscrit à l'activation du nuanceur
   void enregistrerUniformFloat(const char* nom, const float& val);

   /// Enregistre au sein du programme un uniform entier qui sera inscrit à l'activation du nuanceur
   void enregistrerUniformInteger(const char* nom, const int& val);

   /// Retourne l'identificateur du programme de nuanceurs pour utiliser externe spécifique
   GLuint getProg();

private:
   /// Compile et lie les nuanceurs définis à la construction
   void compilerEtLierNuanceurs(char* ncStr);

   /// Affiche les informations de compilation d'un nuanceur
   void afficherShaderInfoLog(GLuint obj,  char* message);

   /// Affiche les informations de compilation et d'édition de liens (link) du programme de nuanceurs
   void afficherProgramInfoLog(GLuint obj, char* message);

   /// la chaîne de caractères du nom de fichier du nuanceur de calcul
   char* nuanceurCalculsStr_;


   /// la liste des uniforms float requis par les nuanceurs
   std::vector<floatUniform> floatUniforms_;

   /// la liste des uniforms integer requis par les nuanceurs
   std::vector<integerUniform> integerUniforms_;

   /// indique si le programme en cours est vide (fonctionalité fixe d'openGL)
   bool estVide_;

   /// Indique si les nuanceurs du programme ont été compilés et linkés
   bool estCompileEtLie_;

   /// l'identificateur du programme de nuanceurs
   GLuint prog_;
};
