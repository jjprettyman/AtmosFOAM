/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 1991-2009 OpenCFD Ltd.
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

Application
    setExnerBalancedH

Description
    Find discretely balanced Exner profile given theta

\*---------------------------------------------------------------------------*/

#include "Hops.H"
#include "fvCFD.H"
#include "ExnerTheta.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    #include "setRootCase.H"
    #include "createTime.H"
    #include "createMesh.H"
    #include "orthogonalBoundaries.H"
    #include "readEnvironmentalProperties.H"
    #include "readThermoProperties.H"
    Hops H(mesh);
    surfaceScalarField gd("gd", g & H.delta());
    #include "createFields.H"
      
    const dictionary& itsDict = mesh.solutionDict().subDict("initialisation");
    const int maxIters = itsDict.lookupOrDefault<int>("maxIters", 100);
    const int BCiters  = itsDict.lookupOrDefault<int>("BCiters", 10);
    const scalar BCtol = itsDict.lookupOrDefault<scalar>("BCtol", SMALL);
    const scalar boundaryRelaxation
         = itsDict.lookupOrDefault<scalar>("boundaryRelaxation", 0.5);
   
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    bool innerConverged = false;
    bool outerConverged = false;
    scalar topBCval = Exner.boundaryField()[topBC][0];
    for(label iter = 0; iter < maxIters && !(innerConverged && outerConverged); iter++)
    {
        Info << "Outer iteration " << iter << endl;
        
        // Inner iterations with fixed top boundary
        innerConverged = false;
        for(label BCiter = 0; BCiter < BCiters && !innerConverged; BCiter++)
        {
//            rho = pRef/(R*theta)*pow(Exner, (1-kappa)/kappa);
//            rhof = fvc::interpolate(rho);
            //U = H.ddirToFlux(gd)
            //  - H.ddirToFluxOffDiag(Cp*thetaf*H.magd()*fvc::snGrad(Exner));
            U = H.Hdiag()*gd;
            fvScalarMatrix ExnerEqn
            (
                fvc::div(U)
              - fvm::laplacian
                (
                    H.Hdiag()*Cp*thetaf*H.magd()/mesh.magSf(),
                    Exner
                )
            );
            //ExnerEqn.relax(0.85);
            innerConverged = ExnerEqn.solve(mesh.solver(Exner.name())).nIterations() == 0;
            //Exner = mag(Exner);

    	}
    	scalar maxGroundExner = max(Exner.boundaryField()[groundBC]);
        outerConverged = (mag(1-maxGroundExner)< BCtol);
    	
        // modify the top boundary condition
        Info << "Exner ground value = " << maxGroundExner
             << "  ground value minus one = "
             << maxGroundExner-1
             << " Exner top BC going from  = " << topBCval << " to ";

        topBCval = (1-boundaryRelaxation)*topBCval
                 + boundaryRelaxation*Exner.boundaryField()[topBC][0]
                       /maxGroundExner;
        topBCval = min(max(topBCval, scalar(0)), scalar(1));
        Info << topBCval << endl;
        Exner.boundaryField()[topBC] == topBCval;
    	
        //Exner.write();
	}

	// Change the top boundary type to be fixedFluxBuoyantExner
	wordList ExnerBCs = Exner.boundaryField().types();
	ExnerBCs[topBC] = "fixedFluxBuoyantExner";
	volScalarField ExnerNew
	(
	    IOobject("Exner", runTime.timeName(), mesh, IOobject::NO_READ),
	    Exner,
	    ExnerBCs
	);
	ExnerNew.correctBoundaryConditions();
	ExnerNew.write();

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //

