///////////////////////////////////////////////////////////////////////////////
///  @file NuanceurCalculProg.cpp
///  @author  Frédéric Plourde 
///  @brief   Ce fichier définit la classe CNuanceurCalculProg, une classe utilitaire
///           contenant un shader et capable d'effectuer plusieurs fonctions
///           pratiques de gestion des nuanceurs.
///  @date    2007-12-12
///  @version 1.0
///
///////////////////////////////////////////////////////////////////////////////

#include "NuanceurCalculProg.h"
#include <iostream>


///////////////////////////////////////////////////////////////////////////////
///  public overloaded constructor  CNuanceurCalculProg \n
///
///  Constructeur par défaut utilisé pour créer le programme vide (fixed Functionality d'openGL)
///
///  @return Aucune
///
///  @author Frédéric Plourde 
///  @date   2007-12-12
///
///////////////////////////////////////////////////////////////////////////////
CNuanceurCalculProg::CNuanceurCalculProg(void)
{
   // indiquer que le programme en cours sera un programme vide (et déjà compilé)
   estVide_ = true;
   estCompileEtLie_ = true;

   // met prog_ à  0 parce que glUseProgram(0) va activer la fonctionalité fixe
   prog_ = 0;
}




///////////////////////////////////////////////////////////////////////////////
///  global public overloaded  CNuanceurCalculProg \n
///
///  Constructeur utilisé pour construire un programme de nuanceurs comportant 
///  un nuanceur de sommets et/ou un nuanceur de fragments. Peut produire 
///  un programme vide (pour fonctionnalité fixe d'openGL).
///
///  @param [in]  nuanceurSommetsStr char *    Nom de fichier du nuanceur de sommets  (si = 0 (ou NULL) -> pas de nuanceurs de sommets)
///  @param [in]  nuanceurFragmentStr char *   Nom de fichier du nuanceur de fragments(si = 0 -> pas de nuanceurs de fragments)
///  @param [in]  compilerMaintenant bool      Indique si l'on doit compiler les nuanceurs à la construction de l'objet
///
///  @return int 
///
///  @author Frédéric Plourde 
///  @date   2007-12-12
///
///////////////////////////////////////////////////////////////////////////////
CNuanceurCalculProg::CNuanceurCalculProg(char* nuanceurCalculsStr, bool compilerMaintenant) :
estCompileEtLie_(false)
{
   // s'assurer qu'au moins UN des deux nuanceurs est défini
   assert(!(nuanceurCalculsStr == NULL));
   
   // indiquer que le programme de nuanceurs n'est pas vide
   estVide_ = false;

   //  mémoriser les noms de fichiers des nuanceurs
   nuanceurCalculsStr_ = nuanceurCalculsStr;

   if (compilerMaintenant) {
	  compilerEtLierNuanceurs(nuanceurCalculsStr_);
   }
}




///////////////////////////////////////////////////////////////////////////////
///  public destructor  ~CNuanceurCalculProg \n
///
///  Destructeur par défaut (vide)
///  @author Frédéric Plourde 
///  @date   2007-12-12
///
///////////////////////////////////////////////////////////////////////////////
CNuanceurCalculProg::~CNuanceurCalculProg(void)
{
}




///////////////////////////////////////////////////////////////////////////////
///  global public  compilerEtLier \n
///
///  Compile et lie les nuanceurs (dans openGL).
///  Vérifie au préalable si les nuanceurs de sommets et de fragments furent 
///  définis, car un nuanceur vide (pour la "fixed functionality d'openGL") 
///  ne se compile pas.
///
///  @return Aucune
///
///  @author Frédéric Plourde 
///  @date   2007-12-12
///
///////////////////////////////////////////////////////////////////////////////
void CNuanceurCalculProg::compilerEtLier()
{
   // vérifie si le programme en cours est un programme de nuanceurs
   assert(!estVide_);
   compilerEtLierNuanceurs(nuanceurCalculsStr_);
}




void CNuanceurCalculProg::enregistrerUniformFloat(const char* nom, const float& val)
{
   floatUniform u(nom, &val);
   floatUniforms_.push_back(u);
}




void CNuanceurCalculProg::enregistrerUniformInteger(const char* nom, const int& val)
{
   integerUniform u(nom, &val);
   integerUniforms_.push_back(u);
}




///////////////////////////////////////////////////////////////////////////////
///  public  activer \n
///
///  Fonction publique qui active le programme de nuanceurs dans openGL
///  La fonction vérifie au préalable si le programme de nuanceurs fut compilé et lié
///
///  @return Aucune
///
///  @author Frédéric Plourde 
///  @date   2007-12-12
///
///////////////////////////////////////////////////////////////////////////////
void CNuanceurCalculProg::activer()
{
   // activer le programme de nuanceurs
   glUseProgram(prog_);

   // inscrire les uniforms float requis par les nuanceurs
   if (!floatUniforms_.empty()) {
	  for (unsigned int i=0; i<floatUniforms_.size(); i++) {
		 glUniform1f(glGetUniformLocation(prog_, floatUniforms_[i].nom), (float)*floatUniforms_[i].val);
	  }
   }

   // inscrire les uniforms int requis par les nuanceurs
   if (!integerUniforms_.empty()) {
	  for (unsigned int i=0; i<integerUniforms_.size(); i++) {
		 glUniform1i(glGetUniformLocation(prog_, integerUniforms_[i].nom), (int)*integerUniforms_[i].val);
	  }
   }
}



void CNuanceurCalculProg::uniform1(char* nom, int v)
{
   glUniform1i(glGetUniformLocation(prog_, nom), v);
}



void CNuanceurCalculProg::uniform1(char* nom, float v)
{
   glUniform1f(glGetUniformLocation(prog_, nom), v);
}



///////////////////////////////////////////////////////////////////////////////
///  private  afficherShaderInfoLog \n
///
///  Cette fonction sert à afficher les informations de compilation 
///  d'un nuanceur. Elle utilise la fonction glGetShaderInfoLog()".
///
///  @param [in]       obj GLuint   Le programme nuanceurs qui vient d'être créé
///
///  @return Aucune
///
///  @author Frédéric Plourde 
///  @date   2007-12-12
///
///////////////////////////////////////////////////////////////////////////////
void CNuanceurCalculProg::afficherShaderInfoLog(GLuint obj, char* message)
{
	int infologLength = 0;
	int charsWritten  = 0;
	char *infoLog;

   // afficher le message d'en-tête
   printf("%s\n", message);

   // afficher le message d'erreur, le cas échéant
	glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

	if (infologLength > 1)
	{
		infoLog = (char *)malloc(infologLength);
		glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
		printf("%s\n",infoLog);
		free(infoLog);
	} 
   else 
   {
	  printf("Aucune erreur :-)\n\n");
   }
}



///////////////////////////////////////////////////////////////////////////////
///  private  afficherProgramInfoLog \n
///
///  Cette fonction sert à afficher les informations de compilation et d'édition
///  de liens (link) du programme de nuanceurs. Elle utilise la fonction 
///  "glGetProgramInfoLog()".
///
///  @param [in]       obj GLuint   Le programme nuanceurs qui vient d'être créé
///
///  @return Aucune
///
///  @author Frédéric Plourde 
///  @date   2007-12-12
///
///////////////////////////////////////////////////////////////////////////////
void CNuanceurCalculProg::afficherProgramInfoLog(GLuint obj, char* message)
{
	int infologLength = 0;
	int charsWritten  = 0;
	char *infoLog;

   // afficher le message d'en-tête
   printf("%s\n", message);

   // afficher le message d'erreur, le cas échéant
	glGetProgramiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

	if (infologLength > 0)
	{
		infoLog = (char *)malloc(infologLength);
		glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
		printf("%s\n",infoLog);
		free(infoLog);
	} 
   else 
   {
	  printf("Aucune erreur :-)\n\n");
   }
}




///////////////////////////////////////////////////////////////////////////////
///  private  compilerEtLierNuanceurs \n
///
///  Lance la compilation et l'édition des liens (link) des différents nuanceurs
///  spécifiés (!= NULL). Crée aussi le programme de nuanceurs.
///
///  @param [in, out]  ncStr char *    le nom de fichier du nuanceur de calculs
///
///  @return Aucune
///
///  @author Frédéric Plourde 
///  @date   2007-12-12
///
///////////////////////////////////////////////////////////////////////////////
void CNuanceurCalculProg::compilerEtLierNuanceurs(char* ncStr) 
{	
	GLuint nuanceurCalculs;

	char *nc = NULL;

	// création du NUANCEUR DE SOMMETS (si spécifié)
   if (ncStr) {
	  // indiquer la progression...
	  printf("Creation du nuanceur de calculs   : %s \n", ncStr);

	  // créer le nuanceur en GLSL
	  nuanceurCalculs = glCreateShader(GL_COMPUTE_SHADER);

	  // lecture du code du nuanceur
	  nc = textFileRead(ncStr);

	  if (nc == nullptr)
	  {
		  std::cout << "[ERROR] : Bad path for shader file (" << ncStr << ")" << std::endl;
		  exit(EXIT_FAILURE);
	  }
	  // créer un pointeur sur le texte du code du nuanceur
	  const char * nc_ptr = nc;

	  // sourcer le nuanceur
	  glShaderSource(nuanceurCalculs, 1, &nc_ptr, NULL);

	  // libérer la mémoire des codes source
	  free(nc);

	  printf("Compilation du nuanceur de calculs   : %s \n", ncStr);
	  glCompileShader(nuanceurCalculs);
	  afficherShaderInfoLog(nuanceurCalculs, "ERREURS DE COMPILATION DU NUANCEUR DE CALCULS : ");
   }
   else
   {
	   std::cout << "[WARNING] : An empty shader object has been created" << std::endl;
   }
   // créer le programme des nuanceurs et lier
   prog_ = glCreateProgram();
   glAttachShader(prog_,nuanceurCalculs);
   glLinkProgram(prog_);

   // afficher les erreurs de compilation et de linkage
   afficherProgramInfoLog(prog_, "ERREURS DE L'EDITION DES LIENS : ");    

   // marquer les nuanceurs compilés
   estCompileEtLie_ = true;
}




/// À CHANGER !
GLuint CNuanceurCalculProg::getProg()
{
   return prog_;
}