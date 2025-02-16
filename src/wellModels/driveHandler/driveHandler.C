/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2011-2019 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "driveHandler.H"

// * * * * * * * * * * * *  Static Member Functions  * * * * * * * * * * * * //

template<class RockType, int nPhases>
Foam::autoPtr<Foam::driveHandler<RockType, nPhases>>
Foam::driveHandler<RockType, nPhases>::New
(
    const word& name,
    const dictionary& driveDict,
    HashTable<autoPtr<wellSource<RockType, nPhases>>>& sources,
    sourceProperties& srcProps,
    HashPtrTable<fvScalarMatrix>& matrices
)
{
    const word modelType = driveDict.dictName();

    Info<< "Selecting well imposed drive handler type " << modelType << endl;

    typename dictionaryConstructorTable::iterator cstrIter =
        dictionaryConstructorTablePtr_->find(modelType);

    if (cstrIter == dictionaryConstructorTablePtr_->end())
    {
        FatalErrorInFunction
            << "Unknown Well Drive Handler type "
            << modelType << nl << nl
            << "Valid well drive handler types:" << endl
            << dictionaryConstructorTablePtr_->sortedToc()
            << exit(FatalError);
    }

    return autoPtr<driveHandler>
    ( cstrIter()(name, driveDict, sources, srcProps, matrices) );
}

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class RockType, int nPhases>
Foam::driveHandler<RockType, nPhases>::driveHandler
(
    const word& name,
    const dictionary& driveDict,
    HashTable<autoPtr<wellSource<RockType, nPhases>>>& sources,
    sourceProperties& srcProps,
    HashPtrTable<fvScalarMatrix>& matrices
)
:
    name_(name),
    driveDict_(driveDict),
    wellSources_(sources),
    srcProps_(srcProps),
    matrices_(matrices),
    cells_(srcProps.cells()),
    driveSeries_
    (
        basicInterpolationTable<scalar>::New(driveDict_)
    ),
    coeffs_(3, scalarList(cells_.size(), 0.0))
{
    if (sources.toc().size() != nPhases)
    {
        FatalErrorInFunction
            << "driveHandler " << name << " expects " << nPhases
            << " phases, but was constructed with " << sources.size()
            << exit(FatalError);
    }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

template<class RockType, int nPhases>
Foam::driveHandler<RockType, nPhases>::~driveHandler()
{}

// ************************************************************************* //
