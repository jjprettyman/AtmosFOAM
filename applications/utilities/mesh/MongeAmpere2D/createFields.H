// Create the mesh displacement potential, Phi
volScalarField Phi
(
    IOobject("Phi",runTime.timeName(),mesh,
             IOobject::MUST_READ, IOobject::AUTO_WRITE),
    mesh
);
Phi.oldTime();

// The gradient of Phi at cell centres
volVectorField gradPhi
(
    IOobject("gradPhi", runTime.timeName(), mesh,
             IOobject::NO_READ, IOobject::AUTO_WRITE),
    fvc::reconstruct(fvc::snGrad(Phi)*mesh.magSf())
);
gradPhi.boundaryField()
    == (static_cast<volVectorField>(fvc::grad(Phi))).boundaryField();

// The gradient of Phi on faces
surfaceVectorField gradPhif
(
    IOobject("gradPhif", runTime.timeName(), mesh, 
             IOobject::NO_READ, IOobject::AUTO_WRITE),
    fvc::interpolate(gradPhi),
    "calculated"
);
gradPhif += (fvc::snGrad(Phi) - (gradPhif & mesh.Sf())/mesh.magSf())
            *mesh.Sf()/mesh.magSf();

// Hessian of Phi and its determinant
volTensorField Hessian("Hessian", fvc::grad(gradPhif));
volScalarField detHess
(
    IOobject("detHess", runTime.timeName(), mesh,
             IOobject::NO_READ, IOobject::AUTO_WRITE),
    mesh,
    dimensionedScalar("", dimless, scalar(1))
);
// laplacian of Phi
volScalarField del2Phi
(
    IOobject("del2Phi", runTime.timeName(), mesh,
             IOobject::NO_READ, IOobject::AUTO_WRITE),
    fvc::laplacian(Phi)
);

// the volume ratio between old and new meshes (the geometric Hessian)
volScalarField volRatio
(
    IOobject("volRatio", runTime.timeName(), mesh,
             IOobject::NO_READ, IOobject::AUTO_WRITE),
    mesh, dimless, wordList(5,"zeroGradient")
);
volRatio.internalField() = 1;
volRatio.correctBoundaryConditions();
volRatio.write();

// The original monitor function on the old mesh
const volScalarField monitor
(
    IOobject("monitor", runTime.timeName(), mesh, IOobject::READ_IF_PRESENT),
    mesh,
    dimensionedScalar("monitor", dimensionSet(0,-2,0,0,0), scalar(1)),
    wordList(5,"zeroGradient")
);

// The monitor function on the old mesh calculated from the new mesh
volScalarField monitorNew("monitorNew", monitor);

// The monitor function on the new mesh
volScalarField monitorR
(
    IOobject("monitorR", runTime.timeName(), rMesh,
             IOobject::NO_READ, IOobject::AUTO_WRITE),
    monitorFunc().map(rMesh, monitor)
);
monitorNew.internalField() = monitorR.internalField();

// The mean equidistribution, c
dimensionedScalar equiDistMean = fvc::domainIntegrate(detHess)
                                 /fvc::domainIntegrate(1/monitorNew);

// Diagnositic of equidistribution
volScalarField equidist
(
    IOobject("equidist", runTime.timeName(), mesh,
             IOobject::NO_READ, IOobject::AUTO_WRITE),
    detHess - equiDistMean/monitor
);

// Equidistribution w.r.t. the changes in cell volume (another diagnostic)
volScalarField equidistVol
(
    IOobject("equidistVol", runTime.timeName(), mesh,
             IOobject::NO_READ, IOobject::AUTO_WRITE),
    volRatio - equiDistMean/monitor
);

// Source term for post processing
volScalarField source
(
    IOobject("source", runTime.timeName(), rMesh,
             IOobject::NO_READ, IOobject::AUTO_WRITE),
    detHess - equiDistMean/monitorNew
);
