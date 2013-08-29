/***************************************************************************
*               Generated by StarUML(tm) C++ Add-In                        *
***************************************************************************/
/***************************************************************************
*                                                                          *
*           Institute of Industrial and Control Engineering                *
*                 Technical University of Catalunya                        *
*                        Barcelona, Spain                                  *
*                                                                          *
*                Project Name:       Kautham Planner                       *
*                                                                          *
*     Copyright (C) 2007 - 2009 by Alexander Pérez and Jan Rosell          *
*            alexander.perez@upc.edu and jan.rosell@upc.edu                *
*                                                                          *
*             This is a motion planning tool to be used into               *
*             academic environment and it's provided without               *
*                     any warranty by the authors.                         *
*                                                                          *
*          Alexander Pérez is also with the Escuela Colombiana             *
*          de Ingeniería "Julio Garavito" placed in Bogotá D.C.            *
*             Colombia.  alexander.perez@escuelaing.edu.co                 *
*                                                                          *
***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#if !defined(_omplPRMPLANNER_H)
#define _omplPRMPLANNER_H

#if defined(KAUTHAM_USE_OMPL)

#include <ompl/base/SpaceInformation.h>
#include <ompl/geometric/planners/prm/PRM.h>
#include <ompl/geometric/SimpleSetup.h>
#include <ompl/config.h>
#include <ompl/datastructures/PDF.h>
#include <ompl/base/spaces/RealVectorStateSpace.h>

#include <libplanner/omplplanner.h>
#include <libproblem/workspace.h>
#include <libsampling/sampling.h>

namespace ob = ompl::base;
namespace og = ompl::geometric;


using namespace std;
using namespace libSampling;

namespace libPlanner {
  namespace omplplanner{


    class omplPRMPlanner:public omplPlanner {
	    public:
         omplPRMPlanner(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, Sampler *sampler,
          WorkSpace *ws, LocalPlanner *lcPlan, KthReal ssize);
         ~omplPRMPlanner();

         bool setParameters();

         KthReal _MaxNearestNeighbors;
         KthReal _distanceThreshold;
         KthReal _MinGrowTime;
         KthReal _MinExpandTime;
         int _BounceSteps;
	  };
  }
}

#endif // KAUTHAM_USE_OMPL
#endif  //_omplPRMPLANNER_H

