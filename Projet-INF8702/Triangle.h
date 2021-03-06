#ifndef __TRIANGLE_H__
#define __TRIANGLE_H__

#include "ISurface.h"
#include "Vecteur3.h"

namespace Scene
{
	///////////////////////////////////////////////////////////////////////////////
	///  CTriangle
	///  Cette classe impl�mente un triangle (ISurface)
	///
	///  Base class Scene::ISurface
	///
	///  @author Olivier Dionne 
	///  @date   13/08/2008
	///
	///////////////////////////////////////////////////////////////////////////////
	class CTriangle : public ISurface
	{
		private:
			/// Les points du triangle
			CVecteur3 m_Pts[ 3 ];

			float padding1;
			/// La normale du triangle
			CVecteur3 m_Normale;

			float padding2;
			/// Calculer la normale � partir des trois c�t�s
			void CalculerNormale( void );

		protected:
			/// Protected interface implementation
			virtual ostream& AfficherInfoDebug( ostream& Out ) const;

		public:
			/// Constructeurs
			CTriangle( void );
			CTriangle( const CTriangle& Triangle );
			/// Destructeur
			virtual ~CTriangle( void );
			/// Op�rateur copie
			CTriangle& operator = ( const CTriangle& Triangle );

			/// M�thodes d'ajustement
			inline void AjusterPoints( const CVecteur3& Pt0, const CVecteur3& Pt1, const CVecteur3& Pt2 )
			{
				 m_Pts[ 0 ] = Pt0;
				 m_Pts[ 1 ] = Pt1;
				 m_Pts[ 2 ] = Pt2;
			}

			inline void AjusterPoint( const int PointIdx, const CVecteur3& Pt )
			{
				assert( PointIdx > -1 && PointIdx < 3 );
				m_Pts[ PointIdx ] = Pt;
			}

			inline void AjusterNormale(const CVecteur3& Normal) { m_Normale = Normal; }


			/// M�thodes d'obtention
			inline CVecteur3 ObtenirPoint( const int PointIdx ) const
			{
				assert( PointIdx > -1 && PointIdx < 3 );
				return m_Pts[ PointIdx ];
			}

			inline CVecteur3 ObtenirNormale(void) const { return m_Normale; }

			/// Public interface implementation
			virtual void          Pretraitement( void );
			virtual CIntersection Intersection ( const CRayon& Rayon );
			virtual CTriangle*    Copier       ( void ) const;
	};
}

#endif
