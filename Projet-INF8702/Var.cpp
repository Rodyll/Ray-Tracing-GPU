#include "Var.h"

int CVar::g_FenetreID       = 0;
int CVar::g_LargeurFenetre  = 512;
int CVar::g_HauteurFenetre  = 256;
int CVar::g_LargeurViewport = 0;
int CVar::g_HauteurViewport = 0;

CScene* CVar::g_GestionnaireDeScene = NULL;

bool CVar::g_ComputerShadersON = true;

char* CVar::g_ComputeShaderPath = "shaders/rayTracing.glsl";
CNuanceurCalculProg* CVar::g_ComputeShader = nullptr;

//CNuanceurCalculProg variables are initialized in main.cpp/initialiser

