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


#include <libproblem/workspace.h>
#include <libsampling/sampling.h>
#include "constlinearlocplan.h"
#include "linearlocplan.h"


namespace Kautham {

namespace IOC{




  ConstLinearLocalPlanner::ConstLinearLocalPlanner(SPACETYPE stype, Sample *init, Sample *goal, WorkSpace *ws, KthReal st )
    :LinearLocalPlanner(stype,init,goal,ws,st)
  {
		vanderMethod = true;
		constrained = true;
    _idName = "Const Linear";
	
		ox = 0.5;
		oy = 0.0;
		oz = 0.5;
		type = 1; //1= sphere - 2=cylinder
			_gen = new LCPRNG(3141592621, 1, 0, ((unsigned int)time(NULL) & 0xfffffffe) + 1);//LCPRNG(15485341);//15485341 is a big prime number
	}

/**
* descrition of canConnect in CPP
*/
	bool ConstLinearLocalPlanner::canConect()
	{
		if(initSamp() == NULL || goalSamp() == NULL) return false; //problem not set.
        if(initSamp()->getDim() != wkSpace()->getNumRobControls()) return false;  //sample is not for the workspace.
    
		KthReal dist = 0;
		Sample *tmpSample = NULL;

		dist = wkSpace()->distanceBetweenSamples(*initSamp(), *goalSamp(), Kautham::CONFIGSPACE);

		//compute step for coord 5
		
			_wkSpace->getRobot(0)->Kinematics(initSamp()->getMappedConf().at(0).getRn()); 
			std::vector<KthReal> tmpcoordTCP7; tmpcoordTCP7.resize(7); 
			mt::Transform ctransfini = _wkSpace->getRobot(0)->getLinkTransform(6);
			mt::Point3 ctransini = ctransfini.getTranslation();
			mt::Rotation crotini = ctransfini.getRotation();
			for(int k=0;k<3;k++) tmpcoordTCP7[k] =  ctransini[k];
			for(int k=0;k<4;k++) tmpcoordTCP7[3+k] =  crotini[k];
		KthReal thetaini = getTheta(tmpcoordTCP7);

			_wkSpace->getRobot(0)->Kinematics(goalSamp()->getMappedConf().at(0).getRn()); 
			mt::Transform ctransfgoal = _wkSpace->getRobot(0)->getLinkTransform(6);
			mt::Point3 ctransgoal = ctransfgoal.getTranslation();
			mt::Rotation crotgoal = ctransfgoal.getRotation();
			for(int k=0;k<3;k++) tmpcoordTCP7[k] =  ctransgoal[k];
			for(int k=0;k<4;k++) tmpcoordTCP7[3+k] =  crotgoal[k];
		KthReal thetagoal = getTheta(tmpcoordTCP7);

		KthReal stepsTheta = thetagoal - thetaini;
		int maxsteps = (dist/stepSize())+2; //the 2 is necessary to always reduce the distance...

		if( !vanderMethod )		
		{
			stepsTheta = stepsTheta/maxsteps;
			
			for(int i=0; i<maxsteps; i++)	
			{	
				tmpSample = initSamp()->interpolate(goalSamp(),(KthReal)i/(KthReal)maxsteps);
				if(constrained) //to correct robot coordinates in order to satisfy the constraints
				{
					KthReal theta = thetaini + stepsTheta*i;
					//if(correctorientation(tmpSample, theta, goalSamp())==false) return false; //not constraint satisfaction: cannot connect
					if(correctorientation(tmpSample, theta, tmpSample)==false) return false; //not constraint satisfaction: cannot connect
				}
				if(wkSpace()->collisionCheck(tmpSample)) return false; 
			}
		}
		else
		{ //method == VANDERCORPUT
			//find how many bits are needed to code the maxsteps
			int b= ceil(log10( (double) maxsteps) / log10( 2.0 ));
			int finalmaxsteps = (0x01<<b);
			//cout<<"maxsteps= "<<maxsteps<<" b= "<<b<<" finalmaxsteps = "<<finalmaxsteps<<endl;

			//index is the index of the Van der Corput sequence, using b bites the sequence has 2^b elements
			//dj is the bit j of the binary representation of the index
			//rj are the elements of the sequence
			//deltarj codes the jumps between successive elements of the sequence
			double rj=0;
			for(int index = 0; index < finalmaxsteps ; index++){
				int dj;
				double deltaj;
				double newrj=0;
				for(int j = 0; j < b ; j++)
				{
					dj = (index >> j) & 0x01;
					newrj += ((double)dj /  (double)(0x01<<(j+1)) );
				}
				deltaj = newrj - rj;
				
				rj = newrj;

				tmpSample = initSamp()->interpolate(goalSamp(),rj);
				
				if(constrained) //to correct robot coordinates in order to satisfy the constraints
				{
					KthReal theta = thetaini + rj*stepsTheta;
					//if(correctorientation(tmpSample, theta, goalSamp())==false) return false; //not constraint satisfaction: cannot connect
					if(correctorientation(tmpSample, theta, tmpSample)==false) return false; //not constraint satisfaction: cannot connect
				}
				
				if(wkSpace()->collisionCheck(tmpSample)) return false; //collision: cannot connect
			}
		}
		return true; //can connect
	}


	//!Given a sample tmpSample, corrects its tcp orientation in order to comply with the orientation constraints.
	//!The value of theta is the rotation along the line of view
	//!The configuration parameters of the arm must be the same as those of sample smp.
	bool ConstLinearLocalPlanner::correctorientation(Sample* tmpSample, KthReal theta, Sample *smp)
	{
			std::vector<KthReal> armcoords(6);
			std::vector<KthReal> coords = tmpSample->getCoords();

			//denormalize the joint values 
			KthReal low[6];
			KthReal high[6];
			for(int k = 0; k < 6; k++)
			{
				low[k] = *_wkSpace->getRobot(0)->getLink(k+1)->getLimits(true);
				high[k] = *_wkSpace->getRobot(0)->getLink(k+1)->getLimits(false);
			}
			/*
			for(int k = 0; k < 6; k++)
			{
				armcoords[k] = low[k] + tmpSample->getMappedConf().at(0).getRn().getCoordinate(k)*(high[k] - low[k]);
			}
			*/
			for(int k = 0; k < 6; k++)
			{
				armcoords[k] = tmpSample->getMappedConf().at(0).getRn().getCoordinate(k);
			}


			//compute the position of the tcp 
			RnConf armconf(6);
			armconf.setCoordinates(armcoords);
			_wkSpace->getRobot(0)->Kinematics(armconf); 
			mt::Transform ctransform = _wkSpace->getRobot(0)->getLinkTransform(6);
			mt::Point3 ctrans = ctransform.getTranslation();
			for(int k=0;k<3;k++) armcoords[k] =  ctrans[k];

			//compute the orientation that satisfies the constraints
			computeorientation(armcoords[0],armcoords[1],armcoords[2],&armcoords[3],&armcoords[4],&armcoords[5], theta);

			//CALL TO INVERSE KINEMATICS
			//armcoords sends se3 coordinates and is reloaded with joint values
			//coords stores the normalized values
			if(ArmInverseKinematics(armcoords, smp, true)==true) {
				for(int k = 0; k < 6; k++) coords[k]=(armcoords[k]-low[k])/(high[k]-low[k]);
			}
			else return false; //not constraint satisfaction: cannot connect

			//load corrected sample
			tmpSample->setCoords(coords);
			return true; //constraint satisfaction: can connect
	}

	KthReal ConstLinearLocalPlanner::getTheta(vector<KthReal> c)
	{
		KthReal theta = 0.0;

			KthReal px = c[0];
			KthReal py = c[1];
			KthReal pz = c[2];

		//retrive the rotation R2*R1
		mt::Rotation R2R1;
		if(c.size()==7)
		{
			R2R1.setValue(c.at(3), c.at(4), c.at(5), c.at(6));
		}
		else
		{
			SE3Conf mp;
			mp.setCoordinates(c);
			std::vector<KthReal> tmp; tmp.resize(7);
			tmp = mp.getCoordinates();
			R2R1.setValue(tmp.at(3), tmp.at(4), tmp.at(5), tmp.at(6));
		}


		//change to camera
			mt::Rotation RC = _cameraTransform.getRotation();
			R2R1 = R2R1*RC;
		



		//recompute R1 as done in computeorientation
		KthReal x2x,x2y,x2z; //new x-axis
		x2x = ox - px;
		x2y = oy - py;
		x2z = oz - pz;
		KthReal modx2 = sqrt(x2x*x2x+x2y*x2y+x2z*x2z);
		x2x =x2x/modx2;
		x2y =x2y/modx2;
		x2z =x2z/modx2;

		KthReal nx,ny,nz; //rotation vector to align z=[0,0,1] with z2; n=z x z2
		nx = 0; //y
		ny = -x2z; //-x
		nz = x2y;
		KthReal modn = sqrt(nx*nx+ny*ny+nz*nz);
		nx =nx/modn;
		ny =ny/modn;
		nz =nz/modn;

		KthReal angle1 = mt::angle(mt::Vector3(1.0,0.0,0.0),mt::Vector3(x2x,x2y,x2z));
		mt::Rotation R1(mt::Unit3(nx,ny,nz),angle1);
		//find inverse of R1
		mt::Rotation invR1 = R1.inverse();

		//find R2 as R2R1*inv(R1)
		mt::Rotation R2 = R2R1*invR1;
		mt::Unit3 axis;
		R2.getAxisAngle(axis,theta);
		//the axis should be (x2x,x2y,x2z). If necessary change signs
		if(abs(x2x+axis[0])<0.0001 && abs(x2y+axis[1])<0.0001 && abs(x2z+axis[2])<0.0001)
		{
			theta = -theta;
			axis[0] = x2x;
			axis[1] = x2y;
			axis[2] = x2z;
		}
		mt::Rotation RR2(axis,theta);
		mt::Rotation tempRot = R2 * R1;
		mt::Rotation tempRot2 = RR2 * R1;

		//correct range if necessary
		if(theta>PI) theta = theta-2*PI;
		else if(theta<-PI) theta = theta+2*PI;
    
		return theta;
	}


	//!Given the position of the tcp of the arm, returns the six joint angles of the arm
	//! computed using the same configuration solution as the sample smp.
	//! the six first values of the vector carm are used for the input/output
	bool ConstLinearLocalPlanner::ArmInverseKinematics(vector<KthReal> &carm, Sample *smp, bool maintainSameWrist)
	{
		//CALL TO INVERSE KINEMATICS
		RobConf rc;
		SE3Conf c;
		std::vector<KthReal> coordarmGoal; coordarmGoal.resize(6); 
		std::vector<KthReal> tmpcoordTCP; tmpcoordTCP.resize(6); 

		//load the six joint values of the goal
		for(int k=0;k<6;k++) coordarmGoal[k] = smp->getMappedConf().at(0).getRn().getCoordinate(k);
			
		//trial: set the goal configuration as objective to test the kinematics
		/*
			  //obtain the tcp transform of the goal using the direct kinematics
			  _wkSpace->getRobot(0)->Kinematics(goalSamp()->getMappedConf().at(0).getRn()); 
			  //write the tcp transform as a 7-vector
			  std::vector<KthReal> tmpcoordTCP7; tmpcoordTCP7.resize(7); 
			  mt::Transform goaltransf = _wkSpace->getRobot(0)->getLinkTransform(6);
			  mt::Point3 goaltrans = goaltransf.getTranslation();
			  mt::Rotation goalrot = goaltransf.getRotation();
			  for(int k=0;k<3;k++) tmpcoordTCP7[k] =  goaltrans[k];
			  for(int k=0;k<4;k++) tmpcoordTCP7[3+k] =  goalrot[k];
			  //load the Robconf to be passed to the InvKinematics
			  c.setCoordinates(tmpcoordTCP7);
		*/
		//end trial

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






//!computes a orientation that satisfies the constraints, given the position px,py,pz
	KthReal ConstLinearLocalPlanner::computeorientation(KthReal px, KthReal py, KthReal pz, KthReal* X1, KthReal* X2, KthReal* X3, KthReal theta)
	{
		//change from TCP to camera
			mt::Rotation RC = _cameraTransform.getRotation();
		


			KthReal x2x,x2y,x2z; //new x-axis
			x2x = ox - px;
			x2y = oy - py;
			x2z = oz - pz;
			KthReal modx2 = sqrt(x2x*x2x+x2y*x2y+x2z*x2z);
			if(modx2==0)
			{
				cout<<"modx2 should not be 0! in ConstLinearLocalPlanner::computeorientation"<<endl;
//				exit(0);
                                return -1;
			}
			x2x =x2x/modx2;
			x2y =x2y/modx2;
			x2z =x2z/modx2;

			KthReal nx,ny,nz; //rotation vector to align x=[1,0,0] with x2; n=x \times x2
			nx = 0; //y
			ny = -x2z; //-x
			nz = x2y;
			KthReal modn = sqrt(nx*nx+ny*ny+nz*nz);
			nx =nx/modn;
			ny =ny/modn;
			nz =nz/modn;


			KthReal angle1 = mt::angle(mt::Vector3(1.0,0.0,0.0),mt::Vector3(x2x,x2y,x2z));
			mt::Rotation tempRot1(mt::Unit3(nx,ny,nz),angle1);

			KthReal angle2;
			if(theta == 1000) angle2 =  -PI + (KthReal)_gen->d_rand()*2*PI;
			else angle2 = theta; 

			mt::Rotation tempRot2(mt::Unit3(x2x,x2y,x2z),angle2);
			
			mt::Rotation tempRot = tempRot2 * tempRot1;

			//Translate from camera to TCP
			tempRot = tempRot*RC.inverse();


			mt::Unit3 axis;
			KthReal angle;
			tempRot.getAxisAngle(axis,angle);
			
      SE3Conf mp;
      vector<KthReal> tmpMp(7);
      tmpMp.at(3) = axis[0];
      tmpMp.at(4) = axis[1];
      tmpMp.at(5) = axis[2];
      tmpMp.at(6) = angle;
      mp.fromAxisToQuaternion(tmpMp);
      mp.setCoordinates(tmpMp);

      //mapping::MyParameters mp(axis[0],axis[1],axis[2],angle);
      tmpMp = mp.getParams();
			*X1 = tmpMp.at(0);
			*X2 = tmpMp.at(1);
			*X3 = tmpMp.at(2);
      

    return 0;
	}


	//FIXME: distance shoulds be mneasured taking propoerly into account the oreintation 
	//and the satisfaction of the constraints in orientation when going from one to the other,
	KthReal ConstLinearLocalPlanner::distance(Sample* from, Sample* to)
	{
		
    return _wkSpace->distanceBetweenSamples(*from, *to, CONFIGSPACE);
		/*
		KthReal dist = 0.0;
		KthReal dd=0.0;
		if( weights != NULL )
		{
            for(int k=0; k < _wkSpace->getNumRobControls() ; k++)
			{
				dd = from->getCoords()[k] - to->getCoords()[k];
				dist += (dd*dd*weights[k]);
			}
		}
		else
		{
            for(int k=0; k < _wkSpace->getNumRobControls() ; k++)
			{
				dd = from->getCoords()[k] - to->getCoords()[k];
				dist += (dd*dd);
			}
		}
		return sqrt(dist);
		*/
  }
}
}


