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


  
 /*	This file contains some useful definitions to be used into Kautham planning
		system.
*/


/**
* \mainpage THE KAUTHAM PROJECT
*  A robot simulation toolkit for motion planning and teleoperation guiding
* \section Features
*  - Multiplatform and open-source
*  -# Programming language: C++
*  -# 3D rendering: coin3D
*  -# GUI: Qt
*  -# Collision detection: PQP
*  -# Graph management: Boost Graph
*  -# Neighborhood: MPNN
*  -# Input data: XML files
*
*  - Modular and extensible
*  -# Geometric library: freeflying robots, kinematic chains and trees
*  -# Sampling library: random and deterministic sampling strategies (Halton, sdk)
*  -# Planning library: gridbased planners (NF1,HF), roadmaps and all the suite of planners provided by the OMPL library (ompl.kavrakilab.org)
*  -# GUI library: graphic rendering and flexible management to include new planners
* \section Credits
* Alexander Perez and Jan Rosell \n
* alexander.perez@upc.edu - jan.rosell@upc.edu\n
*
* Institute of Industrial and Control Engineering\n
* Technical University of Catalunya\n
* Barcelona, Spain\n
* <A HREF="http://www.ioc.upc.edu"> www.ioc.upc.edu</A>\n
*
* Alexander Perez is also with the Escuela Colombiana
* de Ingenieria "Julio Garavito" placed in Bogota D.C.
* Colombia (alexander.perez@escuelaing.edu.co)
*
*
*/



#if !defined(_KAUTHAMDEFS_H)
#define _KAUTHAMDEFS_H

#include <iostream> 
#include <string>
#include <vector>
#include <map>



using namespace std;

namespace Kautham{
  #define KthReal float
  #define MAJOR_VERSION "2"
  #define MINOR_VERSION "1"

	enum ROBOTTYPE{
		FREEFLY,
		CHAIN,
		TREE,
		CLOSEDCHAIN
	};

    enum APPROACH{
		DHSTANDARD,
        DHMODIFIED,
        URDF
  };
	
	enum NEIGHSEARCHTYPE {
		BRUTEFORCE,
		ANNMETHOD,
	};

	enum CONFIGTYPE {
		SE2,
		SE3,
		Rn
	};

  enum LIBUSED{
    INVENTOR,
    IVPQP,
    IVSOLID
  };

  enum SPACETYPE{
    SAMPLEDSPACE,
    CONTROLSPACE,
    CONFIGSPACE
  };

  enum PROBLEMSTATE{
	  INITIAL,
	  PROBLEMLOADED,
	  CSPACECREATED,
	  PRMCALCULATED
  };

  enum CONSTRAINEDKINEMATICS{
    UNCONSTRAINED,
    BRONCHOSCOPY
  };

  //! This enumeration has the relationship of all Inverse Kinematic models
  //! available to be used as solver of the robot inverser kinematics.
  enum INVKINECLASSES{
    NOINVKIN,
    UNIMPLEMENTED,
    RR2D,
    TX90,
    HAND,
    TX90HAND,
    UR5
  };

#ifndef INFINITY
#define INFINITY 1.0e40
#endif

    typedef std::map<std::string, KthReal> HASH_S_K;
    typedef std::map<std::string, std::string> HASH_S_S;

	
	//!	Structure  that save the object position and orientation in the WSpace.	
	/*!	This structure save  temporarily the most important information about 
	*		the object position and orientation in the 3D physical space regardless
	*		If the problem is or not 3D.*/
	struct DATA{
		DATA(){
			for(int i=0;i<3;i++)
				pos[i]=ori[i]= (KthReal)0.0;
			ori[3]= (KthReal)0.0;
		}
		KthReal pos[3];
		KthReal ori[4];
	};
}

#endif //_KAUTHAMDEFS_H

