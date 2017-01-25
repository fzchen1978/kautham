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

//FIXME: this planner is done for a single TREE robot (associtated to wkSpace->robots[0])

#include <stdio.h>
#include <time.h>

#include <kautham/planner/ioc/prmAUROhandarmplanner.h>
#include <iostream>
#include <fstream>

#define RAD2GRAD 180.0/M_PI
 namespace Kautham {
  namespace IOC{
	
	PRMAUROHandArmPlanner::PRMAUROHandArmPlanner(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, Sampler *sampler, 
           WorkSpace *ws, int cloudSize, KthReal cloudRad, int numHC)
      :PRMHandPlanner(stype, init, goal, samples, sampler, ws, cloudSize,  cloudRad)
	{
		_idName = "PRMAURO HandArm";
        _guiName = "PRMAURO HandArm";
		_numberHandConf = numHC;
		addParameter("Ratio Hand/Arm Conf", _numberHandConf);

    _incrementalPMDs = 1;
    addParameter("Incremental PMDs", _incrementalPMDs);
		   _cloudRadiusMax = cloudRad;
           addParameter("Cloud Radius Max", _cloudRadiusMax);

    removeParameter("Neigh Thresshold");

	_exhaustiveCloud=1;
    addParameter("Cloud Exhaustive", _exhaustiveCloud);

	}


	PRMAUROHandArmPlanner::~PRMAUROHandArmPlanner(){
			
	}

    bool PRMAUROHandArmPlanner::setParameters()
	{
      //PRMHandPlanner::setParameters(); //why it is not called?
      try{
        HASH_S_K::iterator it = _parameters.find("Step Size");
        if(it != _parameters.end())
            _locPlanner->setStepSize(it->second);
        else
          return false;

        it = _parameters.find("Speed Factor");
        if(it != _parameters.end())
          _speedFactor = it->second;
        else
          return false;

        it = _parameters.find("Max. Samples");
		if(it != _parameters.end()){
          _maxNumSamples = it->second;
		  _samples->setANNdatastructures(_kNeighs, _maxNumSamples);
	    }
        else
          return false;

        //it = _parameters.find("Neigh Thresshold");
        //if(it != _parameters.end())
        //  _neighThress = it->second;
        //else
        //  return false;

        it = _parameters.find("Max. Neighs");
		if(it != _parameters.end()){
          _kNeighs = (unsigned)it->second;
		  _samples->setANNdatastructures(_kNeighs, _maxNumSamples);
	  }
        else
          return false;

		it = _parameters.find("Cloud Size");
        if(it != _parameters.end())
          _cloudSize = it->second;
        else
          return false;

        it = _parameters.find("Cloud Radius");
        if(it != _parameters.end())
          _cloudRadius = it->second;
        else
          return false;

        it = _parameters.find("Cloud Exhaustive");
        if(it != _parameters.end())
          _exhaustiveCloud = it->second;
        else
          return false;

        it = _parameters.find("Cloud Radius Max");
		if(it != _parameters.end()){
          _cloudRadiusMax = it->second;
		  if(_cloudRadiusMax < _cloudRadius) {
			  _cloudRadiusMax = _cloudRadius;
			  cout <<"WARNING: _cloudRadiusMax must be greater or equal to _cloudRadius\n";
		  }
		}
        else
          return false;

        it = _parameters.find("Ratio Hand/Arm Conf");
        if(it != _parameters.end())
          _numberHandConf = it->second;
        else
          return false;

		
        it = _parameters.find("Incremental PMDs");
        if(it != _parameters.end())
          _incrementalPMDs = it->second;
        else
          return false;

      }catch(...){
        return false;
      }
      return true;
    }


	//! finds a random hand configuration for a given arm configuration
	//! returns a range for the thumb joint for which there is no collisions
	//! If randhand is false, the coord vector passed as a parameter is unchanged,
	//! only the thumb limits are computed; then the numPMDs parameter is not used.
	//! if randhand is true, it computes the random conf using a number numPMDs of
	//! PMDs. The value numPMDs = -1 means all of them.
  bool PRMAUROHandArmPlanner::getHandConfig(vector<KthReal>& coord,  bool randhand, int numPMDs)
	{
		vector<KthReal> coordvector;

		//If random - first find a autocollisionfree sample up to a max of maxtrials
		if(randhand)
		{
			bool autocol;
			int trials=0;
			int maxtrials=10;
			do{
				coordvector.clear();
				//sample the hand coordinates
                if(numPMDs==-1 || numPMDs>int(_wkSpace->getNumRobControls()-_wkSpace->getRobot(0)->getTrunk()))
                        numPMDs = _wkSpace->getNumRobControls()-_wkSpace->getRobot(0)->getTrunk();
                unsigned k;
                unsigned kini = _wkSpace->getRobot(0)->getTrunk();
                for(k = kini; int(k-kini) < numPMDs; k++)
				{
					//hand coords not set - sample the whole range
					if(coord[k]==-1) coord[k] = (KthReal)_gen->d_rand();
					//hand coords already set - saple around the known values
					else{
						coord[k] = coord[k]+ 0.3*(2*(KthReal)_gen->d_rand()-1);
						if(coord[k]<0) coord[k]=0;
						else if(coord[k]>1) coord[k]=1;
					}
					coordvector.push_back(coord[k]);
				}
                for(; k < _wkSpace->getNumRobControls(); k++)
				{
					coord[k] = 0.5;
					coordvector.push_back(coord[k]);
				}

				//load the arm coordinates passed as a parameter
                for(unsigned k =0; k < _wkSpace->getRobot(0)->getTrunk(); k++)
				{
					coordvector.push_back(coord[k]);
				}
				//Set the new sample with the hand-arm coorinates and check for autocollision.				
				_wkSpace->getRobot(0)->control2Pose(coordvector); 
				autocol = _wkSpace->getRobot(0)->autocollision();
				trials++;
			}while(autocol==true && trials<maxtrials);
			if(autocol==true) return false;
		}
		else
		{
			//load the hand-arm coordinates passed as a parameter
            for(unsigned k = 0; k < _wkSpace->getNumRobControls(); k++)
			{
				coordvector.push_back(coord[k]);
			}
		}
		return true;
	}




	
 	void PRMAUROHandArmPlanner::computeMaxSteps(KthReal radius, int *bits, int *steps)
	{
		//compute the number of steps to interpolate the straight segment of the arm connecting cini and cgoal
	  //we assume each arm configuration will be associated with "_numberHandConf" hand configurations
	  /*
	  double dist=0; //distance from cini to cgoal (in the arm cspace)
      for(int i =_wkSpace->getNumRobControls()-_wkSpace->getRobot(0)->getTrunk(); i < _wkSpace->getNumRobControls(); i++)
	  {
			double s = goalSamp()->getCoords()[i] - initSamp()->getCoords()[i];
			dist += (s*s);
	  }
	  dist = sqrt(dist);
      int maxsteps = (int)(dist/(double)(radius)) + 2; 
	  */
	  
	  double dist=0; //distance from cini to cgoal (in the arm cspace)
	  vector<KthReal> pi = _inise3.getPos();
	  vector<KthReal> pg = _goalse3.getPos();
	  KthReal d;
	  for(int i=0;i<3;i++)
	  {
		  d = pg[i]-pi[i];
		  dist += d*d;
	  }
	  dist = sqrt(dist);
      int maxsteps = (int)(dist/(double)(radius)) + 2;

	  //find how many bits are needed to code the maxsteps
	  int b= ceil(log10( (double) maxsteps) / log10( 2.0 ));
	  *steps = (0x01<<b);
	  *bits = b;

	  cout << "MAXSTEPS = " << *steps << " DIST = " << dist << " cloudRadius = " << radius << endl;
	  cout << flush;
	  

	}

//!Given the position of the tcp of the arm, returns the six joint angles of the arm
	//! computed using the same configuration solution as the sample smp.
	//! the six first values of the vector carm are used for the input/output
	bool PRMAUROHandArmPlanner::ArmInverseKinematics(vector<KthReal> &carm, Sample *smp, bool maintainSameWrist)
	{
		//CALL TO INVERSE KINEMATICS
		RobConf rc;
		SE3Conf c;
		std::vector<KthReal> coordarmGoal; coordarmGoal.resize(6); 
		std::vector<KthReal> tmpcoordTCP; tmpcoordTCP.resize(6); 

		//load the six joint values of the goal
		for(int k=0;k<6;k++) coordarmGoal[k] = smp->getMappedConf().at(0).getRn().getCoordinate(k);

	    //load the transform of the tcp (coordarm) and convert it to a RobConf
		for(int k=0;k<6;k++) tmpcoordTCP[k] = carm[k];
		c.setCoordinates(tmpcoordTCP);

		try{
			//cal inverse kinematics - want to find a solution similar to the goal, in termns of configuration
			//parameters (l/r,ep,en,wp,wn)
				rc = _wkSpace->getRobot(0)->InverseKinematics(c.getCoordinates(), coordarmGoal, maintainSameWrist);
		}catch(InvKinEx &ex){
			std::cout << ex.what() << std::endl;
			return false;
		}
		//load the six joint values of the arm
        for(int k=0;k<6;k++) carm[k] = rc.getRn().getCoordinate(k);
		return true;
	}


 	void PRMAUROHandArmPlanner::setIniGoalSe3()
	{
		std::vector<RobConf> r = initSamp()->getMappedConf();
		RnConf q = initSamp()->getMappedConf().at(0).getRn();
		_wkSpace->getRobot(0)->Kinematics(q);
		std::vector<KthReal> tmpcoordTCPpos; 
		tmpcoordTCPpos.resize(3);  
		std::vector<KthReal> tmpcoordTCPori; 
		tmpcoordTCPori.resize(4);  
		mt::Transform ctransfini = _wkSpace->getRobot(0)->getLinkTransform(6);
		mt::Point3 ctransini = ctransfini.getTranslation();
		mt::Rotation crotini = ctransfini.getRotation();
		for(int k=0;k<3;k++) tmpcoordTCPpos[k] =  ctransini[k];
		for(int k=0;k<4;k++) tmpcoordTCPori[k] =  crotini[k];
		_inise3.setPos(tmpcoordTCPpos);
		_inise3.setOrient(tmpcoordTCPori);

		_wkSpace->getRobot(0)->Kinematics(goalSamp()->getMappedConf().at(0).getRn()); 
		mt::Transform ctransfgoal = _wkSpace->getRobot(0)->getLinkTransform(6);
		mt::Point3 ctransgoal = ctransfgoal.getTranslation();
		mt::Rotation crotgoal = ctransfgoal.getRotation();
		for(int k=0;k<3;k++) tmpcoordTCPpos[k] =  ctransgoal[k];
		for(int k=0;k<4;k++) tmpcoordTCPori[k] =  crotgoal[k];
		_goalse3.setPos(tmpcoordTCPpos);
		_goalse3.setOrient(tmpcoordTCPori);
	}



 	bool PRMAUROHandArmPlanner::trySolve()
	{
        _wkSpace->collisionCheck(_init.at(0));
        _wkSpace->collisionCheck(_goal.at(0));

		_neighThress = goalSamp()->getDistance(initSamp(), CONFIGSPACE) *4;//  / 4;
//			cout<<"...._neighThress = "<<_neighThress<<endl<<flush;

		int numPMDs;
		int twoPMDs=0;//for statistic results of number of PMDs used
		int threePMDs=0;
		int fourPMDs=0;
		int fivePMDs=0;

		if(_samples->changed())
		{
			PRMPlanner::clearGraph(); //also puts _solved to false;
		}

		if(_solved) {
			cout << "PATH ALREADY SOLVED"<<endl;
			return true;
		}
		
		cout<<"ENTERING TRYSOLVE!!!"<<endl;
	    clock_t entertime = clock();
		
		double radius;

    std::vector<KthReal> coord(wkSpace()->getNumRobControls());
      Sample *tmpSample;
	  vector<KthReal> coordvector;
	  int trials, maxtrials;

	
	  //Create graph with initial and goal sample
	  if( !_isGraphSet )
	  {
		  //to try to connect ini and goal set threshold to 4.1*_neighThress
		  //Take into account that _neighThress is set to dist_ini_to_goal/4.
		_samples->findBFNeighs(_neighThress*4.1, _kNeighs);

		vector<KthReal>& coordinit = initSamp()->getCoords();
		getHandConfig(coordinit, false, 0);
		vector<KthReal>& coordgoal = goalSamp()->getCoords();
		getHandConfig(coordgoal, false, 0);

        connectSamples();
		PRMPlanner::loadGraph();
		if(PRMPlanner::findPath()) return true;
	  }

	  //load ini and goal transforms of the robot TCP as se3conf objects, to interpolate between them.
	  setIniGoalSe3();
	  //sample around goal
	  int ig=0;
	  trials=0;
	  maxtrials=100;
	  int maxsamplesingoal=10;//50;
	  
	  for(ig=0;trials<maxtrials && ig<maxsamplesingoal;trials++) {
		  /*
		  if(getSampleInGoalRegion(0.0, 0.0)) 
		  {
			  ig++;
			  PRMPlanner::connectLastSample( goalSamp() );
		  }
		  
		  if(getSampleInGoalRegion(_cloudRadius / 2, 0.005)) 
		  {
			  ig++;
			  PRMPlanner::connectLastSample( goalSamp() );
		  }
		  */
		  if(getSampleInGoalRegion(_cloudRadius , 0.01)) 
		  {
			  ig++;
			  PRMPlanner::connectLastSample( goalSamp() );
		  }
		  /*
		  if(getSampleInGoalRegion(_cloudRadius*2, 0.02)) 
		  {
			  ig++;
			  PRMPlanner::connectLastSample( goalSamp() );
		  }
		  */
		  
	  }
	  
	  


	  //return false;




	  //maxsteps is sweeped following the Van Der Corput sequence
	  //index is the index of the Van der Corput sequence, using b bites the sequence has 2^b elements
	  //dj is the bit j of the binary representation of the index
	  //rj are the elements of the sequence
	  int maxsteps;
	  int b;
	  int index;
	  double rj;
	  int dj;
      
	
	  //Sampling the clouds...
	  //Iterate through all the steps of the straighth path, centering a cloud of robot configurations
	  //around each. Loop by setting a new sample per step until the clouds are done.
      tmpSample = new Sample(_wkSpace->getNumRobControls());

	  int found = 0;
      unsigned int n=0;
	  int p=0;//counter of times the arm has swept all its maxsteps steps.
	  double minRJ;
	  double maxRJ;
	  int maxjh=0;//max counter of trials to obtaine a valid hand-arm config per step i
	  do{
			vector<Sample*>::iterator itSam;
			bool autocol;//flag to test autocollisions
	
			minRJ=0.0;
			maxRJ=1.0;

			p++;
			//set the radius of the sphere where to sample randomly
			//the radius increases as new passes are required
			radius = _cloudRadius * p;
			if(radius>_cloudRadiusMax) radius = _cloudRadiusMax;
			computeMaxSteps(radius, &b, &maxsteps);
			//cout << "Pass number = "<< p << " cloudradius = " << radius<< endl<<flush;

			cout << "maxsteps*nA*nH = "<<maxsteps*_cloudSize*_numberHandConf<< "maxSamples = " <<_maxNumSamples<<endl;

			//loop for the arm configurations, swept using the vanderCorput sequence
			for(int i=0; i <= maxsteps; i++){

				//Set the coordinates of the robot joints 
				//Randomly set the coordinates of the robot joints at a autocollision-free conf

				//maxsteps is sweeped following the Van Der Corput sequence
			    //dj is the bit j of the binary representation of the index
			    //rj are the elements of the sequence
				//rj = 1, 0, 0.5, 0.25, 0.75, ...
				if(i==0) rj=1;
				else{
					index = i-1;
					rj=0;
					for(int jj = 0; jj < b ; jj++){
						dj = (index >> jj) & 0x01;
						rj += ((double)dj /  (double)(0x01<<(jj+1)) );
					}
				}
				//end computing the vandercorput step index


				//filter
				//cout << "------------ rj = " << rj << " minRJ = "<<minRJ <<" maxRJ = "<<maxRJ<<endl << flush;
				if(rj<minRJ) continue;
				else if(rj>maxRJ) continue;
				//printConnectedComponents();
				//endfilter

// To test evolution of samples set 
//	rj = (double)i/maxsteps; // instead of vandercorput;

				//start loop of arm	configurations
				int trialsj=0;
				int maxtrialsj=10;
				for(int j=0; j< _cloudSize && trialsj<maxtrialsj; trialsj++)
				{
					
					//load tmpS with the cartesian coords of the interpolated point between ini and goal configs
					//_inise3 and _goalse3 are set in setIniGoalSe3()
					SE3Conf tmpS = _inise3.interpolate(_goalse3, rj );
					vector<KthReal> tmpSpos; tmpSpos.resize(3); 
					vector<KthReal> tmpSrot; tmpSrot.resize(3); 
					vector<KthReal> coordrobot; coordrobot.resize(6); 
					tmpSpos	= tmpS.getPos();
					tmpSrot	= tmpS.getParams();

					//START oversample near goal
					/*
					if(rj>=0.75 && rj!=1){
						int gtrials = 0;
						int gmaxtrials=100;
						for(int g=0;gtrials<gmaxtrials && g<5; gtrials++){//10
							if(_samples->getSize()<_maxNumSamples)
							{
								if(getSampleInRegion(&tmpS, _cloudRadius*2, 0.02 )){
									g++;
									PRMPlanner::connectLastSample( goalSamp() );
								}
							}
							else gtrials=gmaxtrials;
						}
					}
					if(_samples->getSize()>= _maxNumSamples)
					{	
						i = maxsteps;		  //break for i
						break;		  //break for j
					}
					*/
					//END oversample near goal



					//compute the joint values for the arm
					trials=0;
					maxtrials=10;
					do{
						coordvector.clear();

	//////////////////////////////interpolate in cartesian space//////////////////////////////////////
							
						//Add random noise to tmpS (only translational part)
						for(int k =0; k < 3; k++)
						{
							coordrobot[k] = tmpSpos[k] + radius*(2*(KthReal)_gen->d_rand()-1);
						}
						for(int k =0; k < 3; k++)
						{
							coordrobot[k+3] = tmpSrot[k]+ 0.005*(2*(KthReal)_gen->d_rand()-1);
							if(coordrobot[k+3]<0) coordrobot[k+3]=0;
							else if(coordrobot[k+3]>1) coordrobot[k+3]=1;
						}

						//compute inverse kinematics. solution is reloaded in same vector coord
						bool maintainSameWrist=true;
						bool invKinSolved=ArmInverseKinematics(coordrobot, goalSamp(), maintainSameWrist);
						
					
						if(invKinSolved==true) 
						{
							//load arm coordinates (normalized)
							KthReal low[6];
							KthReal high[6];
                            for(unsigned k=0; k < 6; k++)
							{
								//normalize
								low[k] = *_wkSpace->getRobot(0)->getLink(k+1)->getLimits(true);
								high[k] = *_wkSpace->getRobot(0)->getLink(k+1)->getLimits(false);
								coord[k]=(coordrobot[k]-low[k])/(high[k]-low[k]);
							}

							//Set the new sample with the arm coorinates and check for autocollision.	
                            for(unsigned k=6; k < _wkSpace->getNumRobControls(); k++)	coord[k]=-1.0;//dummmy fixed to -1 for later call to getHandConfig
							_wkSpace->getRobot(0)->control2Pose(coord); 
							autocol = _wkSpace->getRobot(0)->autocollision(1);//test for the trunk
						}
						else{
							autocol = true;//invkinematics failed, considered as an autocolision to continue looping
						}
						trials++;
	//////////////////////////////END interpolate in cartesian space//////////////////////////////////////

					}while(autocol==true && trials<maxtrials);

					if(autocol == true) continue;
					

					//a valid arm configuration
					j++; 
					//proeceed to find a set of associated hand confiogurations


					//loop for several hand conf per arm conf in the cloud
					int trialsh=0;
					int maxtrialsh=10;	

					if(_incrementalPMDs==0) numPMDs = -1; //all
					else numPMDs=2; //starts with two PMDs (the thumb and the first PMD obtained by PCA)

					for(int h=0; h< _numberHandConf && trialsh<maxtrialsh; trialsh++)
					{						

						n++;
						//Set the coordinates of the hand joints at a autocollision-free conf
						bool flag;
						if(h==0) {
							//use the last hand cooridnates (h==0 garantees different arm configs)
							if(_gen->d_rand()<0.5)
							{
								flag = true; //sample randomly around them
								Sample *last = _samples->getSampleAt(_samples->getSize()-1);
                                for(unsigned k=6; k < _wkSpace->getNumRobControls(); k++)	coord[k]=last->getCoords()[k];
							}
							else flag = false; //use exactly the same hand coords as the last sample
						}
						else flag = true; //random sample (completely) the hand 

						if(_samples->getSize()<_maxNumSamples && getHandConfig(coord, flag, numPMDs))
						{
							//autocollisionfree samples found

							tmpSample->setCoords(coord);

							//collisioon-ckeck with obstacles
							if( !_wkSpace->collisionCheck(tmpSample))
							{

								//a valid hand-arm configuration
								h++;

								_samples->add(tmpSample);
                                tmpSample = new Sample(_wkSpace->getNumRobControls());
							
								//add to graph
								if(rj<0.25) PRMPlanner::connectLastSample( initSamp() );
								else if (rj>0.75) PRMPlanner::connectLastSample( goalSamp() );
								else PRMPlanner::connectLastSample( );

								int lastComponent = _samples->getSampleAt(_samples->getSize() - 1)->getConnectedComponent();

								//already found hand-arm conf for step i
								if(j!=0 || h!=0){//for infomration only
									int m=(j+1)*_numberHandConf + (h+1);
									if(m>maxjh) maxjh=m;
								}

								if(_exhaustiveCloud==0){
									//if not necessary to complete the cloud, exit the loop as soon the first sample
									//of the cloud is found
									h =  _numberHandConf; //break for h
									j = _cloudSize;		  //break for j
								}
								
								if(goalSamp()->getConnectedComponent() == initSamp()->getConnectedComponent()) 
								{
									i = maxsteps;		  //break for i
									found = 1;			//end while loop
								}
							
								else if(lastComponent == initSamp()->getConnectedComponent()) 
								{
									minRJ = rj;
								}
								else if(lastComponent == goalSamp()->getConnectedComponent()) 
								{
									maxRJ = rj;
								}							
								//to store results:
								if(numPMDs==2) twoPMDs++;
								else if(numPMDs==3) threePMDs++;
								else if(numPMDs==4) fourPMDs++;
								else if(numPMDs>4) fivePMDs++;
							}
							//else try with more PMDs
							else numPMDs++;
						}

                        //if(_samples->getSize()>= _maxNumSamples)
                        if(n>= _maxNumSamples)
						{	
							h =  _numberHandConf; //break for h
							j = _cloudSize;		  //break for j
							i = maxsteps;		  //break for i
						}
					}
				}
			}

			if(PRMPlanner::findPath())
			{
				printConnectedComponents();
				cout << "PRM Nodes = " << _samples->getSize() << endl;
				cout << "Number sampled configurations = " << n << endl;
				
				clock_t finaltime = clock();
				cout<<"TIME TO COMPUTE THE PATH = "<<(double)(finaltime-entertime)/CLOCKS_PER_SEC<<endl;
				PRMPlanner::smoothPath();

				clock_t finalsmoothtime = clock();
				cout<<"TIME TO SMOOTH THE PATH = "<<(double)(finalsmoothtime - finaltime)/CLOCKS_PER_SEC<<endl;

				
				cout << "Number of passes = " << p << " radius = " << radius<< endl;
				cout << "maxjh = " <<maxjh<< " minRJ = "<<minRJ <<" maxRJ = "<<maxRJ<<endl << flush;
				cout << "2PMDs = " <<twoPMDs<< " 3PMDs = "<<threePMDs <<" 4PMDs = "<<fourPMDs<<" 5PMDs = "<<fivePMDs<<endl<<flush;
				
        _solved = true;
        _triedSamples = n;
        _generatedEdges = weights.size();
        _totalTime = (KthReal)(finaltime - entertime)/CLOCKS_PER_SEC ;
        _smoothTime = (KthReal)(finalsmoothtime - finaltime)/CLOCKS_PER_SEC ;
 
        return _solved;
			}
          }while(n < _maxNumSamples && found==0);
          //}while(_samples->getSize() < _maxNumSamples && found==0);

		  /////////////////////////////////////////////////////////////////////////////
			//Last chance - sample more at the goal...
			//TO BE FIXED:
			//BE CAREFUL - the number of samples mya exceed the maximum and then the search withthe ANN that
			//has a fixed arry gives segmentation fault
			/*
			maxtrials = 1000; //_numberHandConf * _cloudSize;
			trials = 0;

			while(trials<maxtrials && goalSamp()->getConnectedComponent() != initSamp()->getConnectedComponent())
			{
				trials++;
			
				if(getSampleInGoalRegion(_cloudRadius, 0.005) == false) continue;
			
				//add to graph
				connectLastSample( goalSamp() );

				if(goalSamp()->getConnectedComponent() != initSamp()->getConnectedComponent()) continue;

				if(PRMPlanner::findPath())
				{
					printConnectedComponents();
					cout << "PRM Nodes = " << _samples->getSize() << endl;
					cout << "Number sampled configurations = " << n << endl;
				
					clock_t finaltime = clock();
					cout<<"TIME TO COMPUTE THE PATH = "<<(double)(finaltime-entertime)/CLOCKS_PER_SEC<<endl;
					PRMPlanner::smoothPath();

					clock_t finalsmoothtime = clock();
					cout<<"TIME TO SMOOTH THE PATH = "<<(double)(finalsmoothtime - finaltime)/CLOCKS_PER_SEC<<endl;

				
					cout << "Number of passes = " << p << " radius = " << radius<< endl;
					cout << "maxjh = " <<maxjh<< " minRJ = "<<minRJ <<" maxRJ = "<<maxRJ<<endl << flush;
					cout << "2PMDs = " <<twoPMDs<< " 3PMDs = "<<threePMDs <<" 4PMDs = "<<fourPMDs<<" 5PMDs = "<<fivePMDs<<endl<<flush;
				
					return true;
				}
			}
			*/
		  /////////////////////////////////////////////////////////////////////////////
	  

	  cout << "PRM Free Nodes = " << _samples->getSize() << endl;
	  cout<<"PATH NOT POUND"<<endl;
	  printConnectedComponents();
	  
	  clock_t finaltime = clock();
	  cout<<"ELAPSED TIME = "<<(double)(finaltime-entertime)/CLOCKS_PER_SEC<<endl;

	  cout << "Number sampled configurations = " << n << endl;
	  cout << "Number of passes = " << p << " radius = " << radius<< endl;
	  cout << "maxjh = " <<maxjh<< " minRJ = "<<minRJ <<" maxRJ = "<<maxRJ<<endl << flush;
	  cout << "2PMDs = " <<twoPMDs<< " 3PMDs = "<<threePMDs <<" 4PMDs = "<<fourPMDs<<" 5PMDs = "<<fivePMDs<<endl<<flush;

    _solved = false;
    _triedSamples = n;
    _totalTime = (KthReal)(finaltime - entertime)/CLOCKS_PER_SEC ;
    _smoothTime = 0. ;
    return _solved;

    }


   //!resample around the goal configuration
    bool PRMAUROHandArmPlanner::getSampleInRegion(SE3Conf  *smpse3, double tradius, double rradius){
	    int trials, maxtrials;
        vector<KthReal> coord(_wkSpace->getNumRobControls());
		bool autocol;//flag to test autocollisions
		Sample *tmpSample;
        tmpSample = new Sample(_wkSpace->getNumRobControls());

		//Set the coordinates of the robot joints 
		//Randomly set the coordinates of the robot joints at a autocollision-free conf
		trials=0;
		maxtrials=10;
		do{
			vector<KthReal> tmpSpos; tmpSpos.resize(3); 
			vector<KthReal> tmpSrot; tmpSrot.resize(3); 
			vector<KthReal> coordrobot; coordrobot.resize(6); 
			tmpSpos	= smpse3->getPos();
			tmpSrot	= smpse3->getParams();
						
			//Add random noise to tmpS (only translational part)
			for(int k =0; k < 3; k++)
			{
				coordrobot[k] = tmpSpos[k] + tradius*(2*(KthReal)_gen->d_rand()-1);
			}
			for(int k =0; k < 3; k++)
			{
				coordrobot[k+3] = tmpSrot[k]+ rradius*(2*(KthReal)_gen->d_rand()-1);
				if(coordrobot[k+3]<0) coordrobot[k+3]=0;
				else if(coordrobot[k+3]>1) coordrobot[k+3]=1;
			}

			//compute inverse kinematics. solution is reloaded in same vector coord
			bool maintainSameWrist=true;
			bool invKinSolved=ArmInverseKinematics(coordrobot, goalSamp(), maintainSameWrist);
						
					
			if(invKinSolved==true) 
			{
				//load arm coordinates (normalized)
				KthReal low[6];
				KthReal high[6];
				for(int k=0; k < 6; k++)
				{
					//normalize
					low[k] = *_wkSpace->getRobot(0)->getLink(k+1)->getLimits(true);
					high[k] = *_wkSpace->getRobot(0)->getLink(k+1)->getLimits(false);
					coord[k]=(coordrobot[k]-low[k])/(high[k]-low[k]);
				}

				//Set the new sample with the arm coorinates and check for autocollision.	
                for(unsigned k=6; k < _wkSpace->getNumRobControls(); k++)	coord[k]=goalSamp()->getCoords()[k]  ;//dummmy -  set to goal values for later call to getHandConfig
				_wkSpace->getRobot(0)->control2Pose(coord); 
				autocol = _wkSpace->getRobot(0)->autocollision(1);//test for the trunk
			}
			else{
				autocol = true;//invkinematics failed, considered as an autocolision to continue looping
			}

			trials++;
		}while(autocol==true && trials<maxtrials);

		if(autocol==true) return false;
		
		//Set the coordinates of the hand joints at a autocollision-free conf
		bool flag = false; //random sample the hand 
			
		if(getHandConfig(coord, flag, -1))	
		{
			//autocollisionfree sample found
			tmpSample->setCoords(coord);
			if( !_wkSpace->collisionCheck(tmpSample))
			{ 	
				_samples->add(tmpSample);
				return true;
			}
		}
		return false;
	}

	 //!resample around the goal configuration
	//!reimplemented
    bool PRMAUROHandArmPlanner::getSampleInGoalRegion(double tradius, double rradius){
	    int trials, maxtrials;
        vector<KthReal> coord(_wkSpace->getNumRobControls());
		bool autocol;//flag to test autocollisions
		Sample *tmpSample;
        tmpSample = new Sample(_wkSpace->getNumRobControls());

		//Set the coordinates of the robot joints 
		//Randomly set the coordinates of the robot joints at a autocollision-free conf
		trials=0;
		maxtrials=10;
		do{
			vector<KthReal> tmpSpos; tmpSpos.resize(3); 
			vector<KthReal> tmpSrot; tmpSrot.resize(3); 
			vector<KthReal> coordrobot; coordrobot.resize(6); 
			tmpSpos	= _goalse3.getPos();
			tmpSrot	= _goalse3.getParams();
						
			//Add random noise to tmpS (only translational part)
			for(int k =0; k < 3; k++)
			{
				coordrobot[k] = tmpSpos[k] + tradius*(2*(KthReal)_gen->d_rand()-1);
			}
			for(int k =0; k < 3; k++)
			{
				coordrobot[k+3] = tmpSrot[k]+ rradius*(2*(KthReal)_gen->d_rand()-1);
				if(coordrobot[k+3]<0) coordrobot[k+3]=0;
				else if(coordrobot[k+3]>1) coordrobot[k+3]=1;
			}

			//compute inverse kinematics. solution is reloaded in same vector coord
			bool maintainSameWrist=true;
			bool invKinSolved=ArmInverseKinematics(coordrobot, goalSamp(), maintainSameWrist);
						
					
			if(invKinSolved==true) 
			{
				//load arm coordinates (normalized)
				KthReal low[6];
				KthReal high[6];
				for(int k=0; k < 6; k++)
				{
					//normalize
					low[k] = *_wkSpace->getRobot(0)->getLink(k+1)->getLimits(true);
					high[k] = *_wkSpace->getRobot(0)->getLink(k+1)->getLimits(false);
					coord[k]=(coordrobot[k]-low[k])/(high[k]-low[k]);
				}

				//Set the new sample with the arm coorinates and check for autocollision.	
                for(unsigned k=6; k < _wkSpace->getNumRobControls(); k++)	coord[k]=goalSamp()->getCoords()[k]  ;//dummmy -  set to goal values for later call to getHandConfig
				_wkSpace->getRobot(0)->control2Pose(coord); 
				autocol = _wkSpace->getRobot(0)->autocollision(1);//test for the trunk
			}
			else{
				autocol = true;//invkinematics failed, considered as an autocolision to continue looping
			}

			trials++;
		}while(autocol==true && trials<maxtrials);

		if(autocol==true) return false;
		
		//Set the coordinates of the hand joints at a autocollision-free conf
		bool flag;
		if(_gen->d_rand()<0.5) flag = true; //use as hand coords those of the goal sample
		else flag = false; //random sample the hand 
			
		if(getHandConfig(coord, flag, -1))	
		{
			//autocollisionfree sample found
			tmpSample->setCoords(coord);
			if( !_wkSpace->collisionCheck(tmpSample))
			{ 	
				_samples->add(tmpSample);
				return true;
			}
		}
		return false;
	}




    void PRMAUROHandArmPlanner::writeFiles(FILE *fpr, FILE *fph, RobConf* joints)
	{
		//write arm coordinates
        unsigned j;
		for(j =0; j < 6; j++)
      fprintf(fpr,"%.2f ",joints->getRn().getCoordinate(j)*180.0/PI);
		fprintf(fpr,"\n");

		//write hand coordinates
    for(; j < joints->getRn().getDim(); j++)
		{
			if(j==6 || j==11 || j==15 || j==19 || j==23) continue;
      fprintf(fph,"%.2f ",joints->getRn().getCoordinate(j)*180.0/PI);
		}
		fprintf(fph,"\n");
	}

////	void PRMAUROHandArmPlanner::setIniGoal()
////	{
////		  if(_wkSpace->getNumRobControls()==11)
////		  {
////		    cout << "Setting initial and goal configurations (5 PMDs assumed)"<<endl<<flush;
//			
//      std::vector<KthReal> c(11);
//		/*	c[0] = 0.366;	
//			c[1] = 0.588;	
//			c[2] = 0.0;	
//			c[3] = 0.392;	
//			c[4] = 0.773;	
//			c[5] = 0.258;	
//			c[6] = 0.515;	
//			c[7] = 0.856;	
//			c[8] = 0.5;	
//			c[9] = 0.392;	
//			c[10] = 0.737;*/
//	  /*
//        c[0] = (KthReal)0.805;//0.806;//0.303;
//        c[1] = (KthReal)0.395;//0.399833333;
//        c[2] = (KthReal)0.262;//0.260138889;
//        c[3] = (KthReal)0.5;//0.502944444;
//        c[4] = (KthReal)0.595;//0.594;
//        c[5] = (KthReal)0.236;//0.237166667; 
//        c[6] = (KthReal)0.559;
//        c[7] = (KthReal)0.523;
//        c[8] = (KthReal)0.728;
//        c[9] = (KthReal)0.821;
//        c[10] = (KthReal)0.928;//0.169;
//	*/
//      // sceneRevistaICRA09-exhaustive.iv
//			/*c[0] = 0.221;
//			c[1] = 0.492;
//			c[2] = 0.0;	
//			c[3] = 0.682;
//			c[4] = 0.513;
//			c[5] = 0.291;
//			c[6] = 0.574;
//			c[7] = 0.756;
//			c[8] = 0.621;
//			c[9] = 0.427;
//			c[10] = 0.618;*/
//	  // sceneRevistaICRA09-exhaustive2.iv
//	  /* too cluttered!
//			c[0] = 0.291;
//			c[1] = 0.574;
//			c[2] = 0.756;
//			c[3] = 0.621;
//			c[4] = 0.427;
//			c[5] = 0.618;
//			c[6] = 0.221;
//			c[7] = 0.492;
//			c[8] = 0.0;	
//			c[9] = 0.682;
//			c[10] = 0.513;
//*/
//	  /* ok
//			c[0] = 0.313;
//			c[1] = 0.605;
//			c[2] = 0.754;
//			c[3] = 0.641;
//			c[4] = 0.277;
//			c[5] = 0.621;
//			c[6] = 0.354;
//			c[7] = 1.0;
//			c[8] = 0.0;	
//			c[9] = 1.0;
//			c[10] = 0.754;
//			*/
//	  /*too cluttered!*/
//			c[0] = 0.288;
//			c[1] = 0.579;
//			c[2] = 0.738;
//			c[3] = 0.595;
//			c[4] = 0.426;
//			c[5] = 0.646;
//			c[6] = 0.231;
//			c[7] = 0.61;
//			c[8] = 0.205;	
//			c[9] = 0.754;
//			c[10] = 0.518;
//			/**/
//
//
//      // sceneRevistaICRA09-other2.iv
//	  /*
//			c[0] = 0.272;
//			c[1] = 0.544;
//			c[2] = 0.718;
//			c[3] = 0.467;
//			c[4] = 0.676;
//			c[5] = 0.754;//0.708;
//			c[6] = 0.369;
//			c[7] = 0.226;
//			c[8] = 0.041;	
//			c[9] = 1.0;
//			c[10] = 1.0;
//*/
//
////			_goal = new Sample(_wkSpace->getNumRobControls());
////			goalSamp()->setCoords(c);
////			_samples->add(_goal);
//
//			if( _wkSpace->collisionCheck(goalSamp()))
//			{
//				cout<<"BE CAREFUL GOAL SAMPLE NOT FREE"<<endl;
//			}
//
//
////ADD SAMPLE NEAR GOAL
//			/*
//			c[0] = 0.313;
//			c[1] = 0.605;
//			c[2] = 0.754;
//			c[3] = 0.641;
//			c[4] = 0.277;
//			c[5] = 0.621;
//			c[6] = 0.354;
//			c[7] = 1.0;
//			c[8] = 0.0;	
//			c[9] = 1.0;
//			c[10] = 0.754;
//			Sample *nearGoal = new Sample(_wkSpace->getNumRobControls());
//			nearGoal->setCoords(c);
//			_samples->add(nearGoal);
//			if( _wkSpace->collisionCheck(nearGoal))
//			{
//				cout<<"BE CAREFUL NEARGOAL SAMPLE NOT FREE"<<endl;
//			}
//			*/
////END ADD SAMPLE NEAR GOAL
//
//
//
//			/*
//			vector<KthReal> parametersCGOAL;
//			vector<KthReal> controlsCGOAL;
//			for(int i=0; i<11;i++) controlsCGOAL.push_back(c[i]);
//			_wkSpace->getRobot(0)->control2Parameters(controlsCGOAL,parametersCGOAL);
//			cout<<"CONTROLS CGOAL";
//			for(int i=0; i<controlsCGOAL.size();i++) 
//			{
//				cout << ", "<< controlsCGOAL[i];
//			}
//			cout<<flush<<endl;
//			cout<<"PARAMETERS CGOAL";
//			for(int i=0; i<parametersCGOAL.size();i++) 
//			{
//				cout << ", "<< parametersCGOAL[i];
//			}
//			cout<<flush<<endl;
//			*/
///*
//			c[0] = 1.0;
//			c[1] = 0.5;
//			c[2] = 0.0;
//			c[3] = 0.5;
//			c[4] = 0.5;
//			c[5] = 0.191;
//			c[6] = 0.515;
//			c[7] = 0.84;
//			c[8] = 0.5;
//			c[9] = 0.392;
//			c[10] = 0.737;*/
///*
//		c[0] = 0.71;//0.762;//0.71;//0.21;
//        c[1] = 0.506;
//        c[2] = 0.150;//0.178;//0.262;
//        c[3] = 0.5;//0.418;//0.5;
//        c[4] = 0.595;//0.584;//0.208;//0.508;
//        c[5] = 0.236;//0.177;
//        c[6] = 0.251;
//        c[7] = 0.036;
//        c[8] = 0.226;
//        c[9] = 0.518;
//        c[10] = 0.21;
//		*/
//
//			
//      // sceneRevistaICRA09-exhaustive.iv
//      /*c[0] = 0.221;
//			c[1] = 0.492;
//			c[2] = 0.0;	
//			c[3] = 0.5;
//			c[4] = 0.513;
//			c[5] = 0.5;
//			c[6] = 0.6;
//			c[7] = 0.569;
//			c[8] = 0.646;
//			c[9] = 0.436;
//			c[10] = 0.344;*/
//	// sceneRevistaICRA09-exhaustive2.iv
//			/* compte dona problemes - en colisio
//c[0] = 0.225468;
//c[1] = 0.493986;
//c[2] = 0.882932;
//c[3] = 0.540882;
//c[4] = 0.371212;
//c[5] = 0.709392;
//c[6] = 0.626274;
//c[7] = 0.791468;
//c[8] = 0.205205;
//c[9] = 0.873384;
//c[10] = 0.957097;
//*/
//
///* ok
//c[0] = 0.374;
//			c[1] = 0.574;
//			c[2] = 0.756;
//			c[3] = 0.621;
//			c[4] = 0.427;
//			c[5] = 0.618;
//c[6] = 1.0;
//c[7] = 0.5;
//c[8] = 0.0;
//c[9] = 0.5;
//c[10] = 0.5;
//*/
//
//c[0] = 0.256;
//c[1] = 0.431;
//c[2] = 0.897;
//c[3] = 0.595;
//c[4] = 0.427;
//c[5] = 0.618;
//c[6] = 1.0;
//c[7] = 0.5;
//c[8] = 0.0;
//c[9] = 0.5;
//c[10] = 0.5;
//
//		// sceneRevistaICRA09-other2.iv
//			/*
//			c[0] = 0.146;//0.246;
//			c[1] = 0.467;
//			c[2] = 0.810;
//			c[3] = 0.477;
//			c[4] = 0.478;//0.528;
//			c[5] = 0.708;
//			c[6] = 0.887;
//			c[7] = 0.682;
//			c[8] = 0.405;	
//			c[9] = 0.338;
//			c[10] = 0.323;
//			*/
//
////			_init = new Sample(_wkSpace->getNumRobControls());
////			initSamp()->setCoords(c);
////			_samples->add(_init);
//	
////			if( _wkSpace->collisionCheck(initSamp()))
////			{
////				cout<<"BE CAREFUL INIT SAMPLE NOT FREE"<<endl;
////			}
////			/*
////			<KthReal> parametersCINI;
////			vector<KthReal> controlsCINI;
////			for(int i=0; i<11;i++) controlsCINI.push_back(c[i]);
////			_wkSpace->getRobot(0)->control2Parameters(controlsCINI,parametersCINI);
////			cout<<"CONTROLS CINI";
////			for(int i=0; i<controlsCINI.size();i++)
////			{
////				cout << ", "<< controlsCINI[i];
////			}
////			cout<<flush<<endl;
////			cout<<"PARAMETERS CINI";
////			for(int i=0; i<parametersCINI.size();i++)
////			{
////				cout << ", "<< parametersCINI[i];
////			}
////			cout<<flush<<endl;
////			*/
//
//			//set neightrheshold equal to distance between init and goal configs
//			/*
//			double dist=0; 
//			for(int i =0; i < _wkSpace->getNumRobControls(); i++)
//			{
//				double s = goalSamp()->getCoords()[i] - initSamp()->getCoords()[i];
//				dist += (s*s);
//			}
//			dist = sqrt(dist);
//			*/
//			_neighThress = goalSamp()->getDistance(initSamp(), CONFIGSPACE) *4;//  / 4;
//			cout<<"...._neighThress = "<<_neighThress<<endl<<flush;
//			
//
////		  }
////		  else if(_wkSpace->getNumRobControls()==19)
////		  {
////			    cout << "Setting initial and goal configurations (all joints assumed)"<<endl<<flush;
//				
////        std::vector<KthReal> c(19);
////				int h=0;
////				//PARAMETERS CGOAL,
////				c[h++] = 0.1715;
////				c[h++] = 0.18605;
////				c[h++] = 0.736189;
////				c[h++] = 0.84121;
////				c[h++] = 0.351502;
////				c[h++] = 0.607788;
////				c[h++] = 0.921575;
////				c[h++] = 0.5;
////				c[h++] = 0.69032;
////				c[h++] =  0.726542;
////				c[h++] = 0.624597;
////				c[h++] = 0.641927;
////				c[h++] = 0.780143;
////				c[h++] =  0.258;
////				c[h++] =  0.515;
////				c[h++] = 0.856;
////				c[h++] =  0.5;
////				c[h++] = 0.392;
////				c[h++] = 0.737;
////        _goal = new Sample(_wkSpace->getNumRobControls());
////				goalSamp()->setCoords(c);
////				_samples->add(_goal);
//
////				//PARAMETERS CINI,
////				h=0;
////				c[h++] =  0.8055;
////				c[h++] =  0.315836;
////				c[h++] =  0.725758;
////				c[h++] =  0.681895;
////				c[h++] =  0.40439;
////				c[h++] =  0.685201;
////				c[h++] =  0.869964;
////				c[h++] =  0.5;
////				c[h++] =  0.755927;
////				c[h++] =  0.748784;
////				c[h++] =  0.581193;
////				c[h++] =  0.728463;
////				c[h++] =  0.807463;
////				c[h++] =  0.191;
////				c[h++] =  0.515;
////				c[h++] =  0.84;
////				c[h++] =  0.5;
////				c[h++] =  0.392;
////				c[h++] =  0.737;
////        _init = new Sample(_wkSpace->getNumRobControls());
////				initSamp()->setCoords(c);
////				_samples->add(_init);
//
//			
////				//set neightrheshold equal to distance between init and goal configs
////				double dist=0;
////				for(int i =0; i < _wkSpace->getNumRobControls(); i++)
////				{
////					double s = goalSamp()->getCoords()[i] - initSamp()->getCoords()[i];
////					dist += (s*s);
////				}
////				dist = sqrt(dist);
////				_neighThress = dist;
////				cout<<"...._neighThress = "<<_neighThress<<endl<<flush;
////		  }
////		  else
////			  cout << "ERROR: not init/goal not set - dimensions do not fit"<<endl<<flush;
////			  cout << "dimensionWorkspace = "<<_wkSpace->getNumRobControls() <<endl<<flush;
//
//
////	  }

    void PRMAUROHandArmPlanner::saveData()
	{
		/*
		setIniGoalSe3();
		for(int i=0;i<=10;i++)
		{
			SE3Conf tmpS = _inise3.interpolate(_goalse3, (KthReal)i/10.0 );
			vector<KthReal> tmpSpos; tmpSpos.resize(3); 
			vector<KthReal> tmpSrot; tmpSrot.resize(3); 
			vector<KthReal> coordrobot; coordrobot.resize(6); 
			tmpSpos	= tmpS.getPos();
			tmpSrot	= tmpS.getParams();

			cout << "pos: "<< tmpSpos[0] << ", " << tmpSpos[1] << ", " << tmpSpos[2];
			cout << " ori: "<< tmpSrot[0] << ", " << tmpSrot[1] << ", " << tmpSrot[2]<<endl; 
		}
		for(int i=0;i<=10;i++)
		{
			SE3Conf tmpS = _inise3.interpolate(_goalse3, (KthReal)i/10.0 );
			vector<KthReal> tmpSpos; tmpSpos.resize(3); 
			vector<KthReal> tmpSrot; tmpSrot.resize(3); 
			vector<KthReal> c; c.resize(6); 
			tmpSpos	= tmpS.getPos();
			tmpSrot	= tmpS.getParams();
			for(int k =0; k < 3; k++) c[k] = tmpSpos[k];
			for(int k =0; k < 3; k++) c[k+3] = tmpSrot[k];
			ArmInverseKinematics(c, goalSamp(), true);
			KthReal low[6];
			KthReal high[6];
			for(int k=0; k < 6; k++)
			{
				//normalize
				low[k] = *_wkSpace->getRobot(0)->getLink(k+1)->getLimits(true);
				high[k] = *_wkSpace->getRobot(0)->getLink(k+1)->getLimits(false);
				c[k]=(c[k]-low[k])/(high[k]-low[k]);
			}
			cout << "joints: "<< c[0] << ", " << c[1] << ", " << c[2] << ", " << c[3] << ", " << c[4] << ", " << c[5]<<endl;
		}
		*/


/*
		if(_solved) 
		{
			vector<KthReal> coordvector;
			RobConf* joints;
			Sample *init, *goal, *init2, *goal2;
			KthReal initthumb;

			cout << "Save PATH to FILE"<<endl;
			FILE *fpr,*fph;
			fpr = fopen ("robotconf.txt","wt");
			if(fpr==NULL)
			{
				cout<<"Cannot open robotconf.txt for writting..."<<endl;
			}
			fph = fopen ("handconf.txt","wt");
			if(fph==NULL)
			{
				cout<<"Cannot open handconf.txt for writting..."<<endl;
			}


			for(unsigned i = 0; i < _path.size()-1; i++){
			//compute intermediate points
				init = _path[i];
				goal = _path[i+1];
				if(i==0) initthumb = _path[0]->getCoords()[0];
				else initthumb = goal2->getCoords()[0];

				_locPlanner->setInitSamp(_path[i]);
				_locPlanner->setGoalSamp(_path[i+1]);
				((ManhattanLikeLocalPlanner*) _locPlanner)->setIntermediateConfs();
				init2 = ((ManhattanLikeLocalPlanner*) _locPlanner)->initSamp2();
				goal2 = ((ManhattanLikeLocalPlanner*) _locPlanner)->goalSamp2();

			//Save Init configuration 
				coordvector.clear();
				coordvector.push_back( initthumb ); 
                for(int k = 1; k < _wkSpace->getNumRobControls(); k++)
					coordvector.push_back( init->getCoords()[k] ); 

				//convert from controls to real coordinates
				_wkSpace->getRobot(0)->control2Pose(coordvector);
				joints = _wkSpace->getRobot(0)->getCurrentPos();
				
				//write to file
				writeFiles(fpr,fph,joints);
				
			//Save Init2 configuration 
				coordvector.clear();
                for(int k = 0; k < _wkSpace->getNumRobControls(); k++)
					coordvector.push_back( init2->getCoords()[k] );

				//convert from controls to real coordinates
				_wkSpace->getRobot(0)->control2Pose(coordvector);
				joints = _wkSpace->getRobot(0)->getCurrentPos();
				
				//write to file
				writeFiles(fpr,fph,joints);

				
			//Save Goal2 configuration 
				coordvector.clear();
                for(int k = 0; k < _wkSpace->getNumRobControls(); k++)
					coordvector.push_back( goal2->getCoords()[k] );

				//convert from controls to real coordinates
				_wkSpace->getRobot(0)->control2Pose(coordvector);
				joints = _wkSpace->getRobot(0)->getCurrentPos();
				
				//write to file
				writeFiles(fpr,fph,joints);

			//Last move towards GOAL
				if(i == _path.size()-2)
				{
					coordvector.clear();
                    for(int k = 0; k < _wkSpace->getNumRobControls(); k++)
						coordvector.push_back( goal->getCoords()[k] );

					//convert from controls to real coordinates
					_wkSpace->getRobot(0)->control2Pose(coordvector);
					joints = _wkSpace->getRobot(0)->getCurrentPos();
				
					//write to file
					writeFiles(fpr,fph,joints);
				}
			}
			fclose(fpr);
			fclose(fph);
		}
		else{
			cout << "Sorry: Path not yet found"<<endl;
		}
		*/
    }


  }
};
