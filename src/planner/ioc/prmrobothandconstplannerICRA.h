/*************************************************************************\
   Copyright 2014 Institute of Industrial and Control Engineering (IOC)
                 Universitat Politecnica de Catalunya
                 BarcelonaTech
    All Rights Reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the
    Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 \*************************************************************************/

/* Author: Alexander Perez, Jan Rosell, Nestor Garcia Hidalgo */


#include "prmhandplanner.h"

#if !defined(_PRMROBOTHANDCONSTPLANNERICRA_H)
#define _PRMROBOTHANDCONSTPLANNERICRA_H
 namespace Kautham {
 /** \addtogroup Planner
  *  @{
  */
  namespace IOC{
  class PRMRobotHandConstPlannerICRA:public PRMHandPlanner{
		public:
      PRMRobotHandConstPlannerICRA(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, Sampler *sampler,
                WorkSpace *ws,  int cloundSize, KthReal cloudRad);
	  
      ~PRMRobotHandConstPlannerICRA();
      
			bool  setParameters();
			bool  trySolve();
			void  saveData();

			inline void  setNumberSamples(int n){_numberSamples = n;}
			inline int   numberSamples(){return _numberSamples;}  
			void moveAlongPath(unsigned int step); //reimplemented
			bool solveAndInherit();//reimplemented
//		    void setIniGoal();//reimplemented
			//bool getSampleInInitRegion(double R=0);	
			//bool getSampleInGoalRegion(double R=0);//reimplemented
			bool getSampleInRegion(Sample *smp, double R=0);

			void computeStaringPoint(KthReal px,KthReal py, KthReal pz, KthReal q1, KthReal q2, KthReal q3, KthReal q4);
			void setConstrianed(int c){_constrained=c;};
			
			void writeFiles(FILE *fpr, FILE *fph, RobConf* joints);

	 
		private:
			int _numberSamples;
			int _constrained;
			double _sampletimeok;
			double _sampletimeko;
			double _connecttime;
      KthReal _planeHeight; //height of the plane parallel to the x-z plane, where the staring point is to be located

      mt::Transform _cameraTransform;
	};	
  }
  /** @}   end of Doxygen module "Planner */
};
 
#endif  //_PRMPLANNERICRA_H

