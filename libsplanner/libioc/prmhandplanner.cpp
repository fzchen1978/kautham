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
 

//FIXME: this planner is done for a single TREE robot (associtated to wkSpace->robots[0])


#include "prmhandplanner.h"
 
 namespace libPlanner {
  namespace IOC{
		
		PRMHandPlanner::PRMHandPlanner(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, Sampler *sampler, 
      WorkSpace *ws, LocalPlanner *lcPlan, KthReal ssize, int cloudSize, KthReal cloudRad)
      :PRMPlanner(stype, init, goal, samples, sampler, ws, lcPlan, ssize){
			
	    //_gen = new LCPRNG(15485341);//15485341 is a big prime number
		_gen = new LCPRNG(3141592621, 1, 0, ((unsigned int)time(NULL) & 0xfffffffe) + 1);//

        _guiName = "PRMHand Planner";
        _cloudSize = cloudSize;
        _cloudRadius = cloudRad;
        addParameter("Cloud Size", _cloudSize);
        addParameter("Cloud Radius", _cloudRadius);

		//set weights
        /*
		_weightPerJoint = new KthReal[_wkSpace->getDimension()];
		int i;
		for(i = 0; i < _wkSpace->getDimension()-_wkSpace->getRobot(0)->getTrunk(); i ++)
			_weightPerJoint[i]=1;//don't know how to play with that...
		for(; i < _wkSpace->getDimension(); i ++)
			_weightPerJoint[i]=10; //don't know how to play with that...try penalyzing arm motions
        */
		}

	PRMHandPlanner::~PRMHandPlanner(){
			
	}

    bool PRMHandPlanner::setParameters(){
      PRMPlanner::setParameters();
      try{
        HASH_S_K::iterator it = _parameters.find("Cloud Size");
        if(it != _parameters.end())
          _cloudSize = it->second;
        else
          return false;

        it = _parameters.find("Cloud Radius");
        if(it != _parameters.end())
          _cloudRadius = it->second;
        else
          return false;
      }catch(...){
        return false;
      }
      return true;
    }

//	void PRMHandPlanner::setIniGoal(){
//		  cout << "PRMHandPlanner::setIniGoal not implemented"<<endl<<flush;
//	}

    void PRMHandPlanner::saveData()
	{
		if(_solved) 
		{
			vector<KthReal> coordvector;
      RobConf* joints;

			cout << "Save PATH to FILE"<<endl;
			FILE *fpr,*fph;
			fpr = fopen ("robotconf.txt","wt");
			fph = fopen ("handconf.txt","wt");


			for(unsigned i = 0; i < _path.size(); i++){
				coordvector.clear();
				//convert from controls to real coordinates
				for(int k = 0; k < _wkSpace->getDimension(); k++)
					coordvector.push_back( _path[i]->getCoords()[k] ); 
								
				_wkSpace->getRobot(0)->control2Pose(coordvector);
				joints = _wkSpace->getRobot(0)->getCurrentPos();

				//arm coordinates
				int j;
				for(j =0; j < 6; j++)
              fprintf(fpr,"%.2f ",joints->getRn().getCoordinate(j)*180.0/PI);
				fprintf(fpr,"\n");

				//hand coordinates
        for(; j < joints->getRn().getDim(); j++)
				{
					if(j==6 || j==11 || j==15 || j==19 || j==23) continue;
          fprintf(fph,"%.2f ",joints->getRn().getCoordinate(j)*180.0/PI);
				}
				fprintf(fph,"\n");
			}
			fclose(fpr);
			fclose(fph);
		}
		else{
			cout << "Sorry: Path not yet found"<<endl;
		}
    }

	
   //!resample around the goal configuration
    bool PRMHandPlanner::getSampleInGoalRegion(){
      double radius = _cloudRadius / 2;
	    vector<KthReal> coordvector;
	    int trials, maxtrials;
      std::vector<KthReal> coord(_wkSpace->getDimension());
      std::vector<KthReal> coordarm(_wkSpace->getDimension());
      bool autocol;//flag to test autocollisions
      Sample *tmpSample;
		
                tmpSample = new Sample(_wkSpace->getDimension());

		//Set the coordinates of the robot joints 
		//Randomly set the coordinates of the robot joints at a autocollision-free conf
		trials=0;
		maxtrials=10;
		do{
			coordvector.clear();
			for(int k = 0; k < _wkSpace->getDimension()-_wkSpace->getRobot(0)->getTrunk(); k++)
			{
				coordvector.push_back(0.0); //dummy values - not used in call to autocollision with parameter 1
			}
			for(int k =_wkSpace->getDimension()-_wkSpace->getRobot(0)->getTrunk(); k < _wkSpace->getDimension(); k++)
			{
				coordarm[k] = goalSamp()->getCoords()[k] + radius*(2*(KthReal)_gen->d_rand()-1);
				coordvector.push_back(coordarm[k]);
			}
			//Set the new sample with the hand-arm coorinates and check for autocollision.
			_wkSpace->getRobot(0)->control2Pose(coordvector); 
			autocol = _wkSpace->getRobot(0)->autocollision(1);//only test for the trunk
			trials++;
		}while(autocol==true && trials<maxtrials);

		if(autocol==true) return false;
		
		//Randomly set the coordinates of the hand joints at a autocollision-free conf
		trials=0;
		maxtrials=10;
		do{
			coordvector.clear();
			//sample the hand coordinates
			for(int k = 0; k < _wkSpace->getDimension()-_wkSpace->getRobot(0)->getTrunk(); k++)
			{
				coord[k] = (KthReal)_gen->d_rand();
				coordvector.push_back(coord[k]);
			}
			//load the arm coordinates computed before
			for(int k =_wkSpace->getDimension()-_wkSpace->getRobot(0)->getTrunk(); k < _wkSpace->getDimension(); k++)
			{
				coord[k]=coordarm[k];
				coordvector.push_back(coord[k]);
			}
			//Set the new sample with the hand-arm coorinates and check for autocollision.				
			_wkSpace->getRobot(0)->control2Pose(coordvector); 
			autocol = _wkSpace->getRobot(0)->autocollision();
			trials++;
		}while(autocol==true && trials<maxtrials);

		if(autocol==true) return false;

		//Set the new sample with the hand-arm coorinates and collision-check.
		tmpSample->setCoords(coord);
		if( !_wkSpace->collisionCheck(tmpSample)){ 
			//Free sample. Add to the sampleset _samples.
			_samples->add(tmpSample);
			return true;
		 }		
		else return false;
	}




  }
};
