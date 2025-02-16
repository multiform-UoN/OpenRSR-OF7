template<class Type>
void Foam::coupledSolver::insertField  
(
  GeometricField<Type, fvPatchField, volMesh>& field
)
{
  // Check if already set
  if (isSet)
  {
    FatalErrorIn
    (
        "Foam::coupledSolver::addField"
    )   << nl << nl << "Cannot insert fields after inserting equation. "
        << exit(FatalError);
  }
  
  // Check if var already exists
  if (varInfo.size() != 0)
  {
    label fieldID(findField(field.name(), field.mesh().name()));
 
    if ( fieldID != -1 )
    {
       FatalErrorInFunction
       << "Field with name " << field.name() << " already exists in the coupled matrix "
       << name_ << "."
       << exit(FatalError);
    }
  } 
  
  // Add var to the list
  varInfo.append(varInfoS());

  typename pTraits<Type>::labelType validComponents
  (
    field.mesh().template validComponents<Type>()
  );
 
  int i = 0;
  for (direction cmpt=0; cmpt<pTraits<Type>::nComponents; cmpt++)
  {
    if (component(validComponents, cmpt) == -1) continue; 
    
    varNamesCmpList.append(word(field.name()+pTraits<Type>::componentNames[cmpt]));
    i++;
  }
  
  // Find meshID
  label mID(-1);
  forAll(meshList, j)
  {
    if (field.mesh().name() == meshList[j].mesh->name())
      mID = j;
  }
  
  meshList[mID].nValidCmp += i;
  
  label ncells = field.size(); 
  reduce(ncells, sumOp<int>());
  
  // Set the position for the first component of the NEXT variable to be added.
  // A Type variable may add up to N blocks because it has
  // up to N solvable components
  if (varInfo.size() == 1)
  {
    // For the first variable, its first component will
    // always correspond to block 0, independent of his type 
    varInfo[0].firstCmp = 0;  
    varInfo[0].firstElem = 0;  
  }
  else
  {
    label pen(varInfo.size()-2); // Before last position 
    
    varInfo.last().firstCmp = varInfo[pen].firstCmp + varInfo[pen].nValidCmp;
    varInfo.last().firstElem = varInfo[pen].firstElem + varInfo[pen].nValidCmp*varInfo[pen].nCells;
  }
  
  // Element of type Type
  Type type(pTraits<Type>::zero); 
 
  varNames += field.name() + ", ";
  
  // Append field to the list of its class
  varTypeList(type).append(&field);
  
  // Finish to complete the remaining fields in varInfo
  varInfo.last().localID = varTypeList(type).size()-1;
  varInfo.last().typeV = ftType(type);
  varInfo.last().nCells = ncells;
  varInfo.last().meshID = mID;
  varInfo.last().nValidCmp = i;
  varInfo.last().name = field.name();  
}

// Note: in the way the function is defined, the matrix and the 
// fields must belong to the same mesh.
template<class Type>
void Foam::coupledSolver::insertEquation
(
  word rowField,
  word colField,
  fvMatrix<Type>& matrix
)
{
 createSystem();
  
 // The index retrieved is the position in varInfo where the field lies
 int bRow = findField(rowField, matrix.psi().mesh().name());
 int bCol = findField(colField, matrix.psi().mesh().name());
 
 label sizeRow = varInfo[bRow].nCells;  
 label sizeCol = varInfo[bCol].nCells;  
 
 // Note: in general rowField and colField may belong to different meshes. Here
 // we will assume they are the same.
 label mID = varInfo[bRow].meshID;  
 typename pTraits<Type>::labelType validComponents
 (
    (*meshList[mID].mesh).template validComponents<Type>()
 );
 
 int i = 0;
 for (direction cmpt=0; cmpt<pTraits<Type>::nComponents; cmpt++)
 {
   if (component(validComponents, cmpt) == -1) continue; 
   
   int rowBias = varInfo[bRow].firstElem+(i*sizeRow);
   int colBias = varInfo[bCol].firstElem+(i*sizeCol);  // TODO: will only work if rowField = colField or if both fields are of scalar type.
                                                       // In general one would have i for bRow and j for bCol. (See insertEquation(LMatrix)) 

   if (initTimeFlag || updateA_ || !saveSystem_)
   {
     assemblePetscAb
     (
       A,
       b,
       x,
       matrix,
       cmpt,  
       rowBias,
       colBias,
       bRow
     );
   }
   else
   {
     assemblePetscb
     (
       b,
       matrix,
       cmpt,  
       rowBias,
       bRow
     );
   }
   
   i++;
 } 
 
 isSet = true;
}


// Note: in the way the function is defined, the matrix and the 
// fields must belong to the same mesh.
template<class Type>
void Foam::coupledSolver::insertEquation
(
  word rowField,
  word colField,
  const tmp<fvMatrix<Type>>& tmatrix
)
{
  insertEquation(rowField, colField, const_cast<fvMatrix<Type>&>(tmatrix()));
  tmatrix.clear();
}
 
 
template<class Type>
void Foam::coupledSolver::insertEquation
(
  word rowField,
  word colField,
  LMatrix<Type>& matrix
)
{
 createSystem();
  
 // The index retrieved is the position in varInfo where the field lies
 int bRow = findField(rowField, matrix.mesh().name());
 int bCol = findField(colField, matrix.mesh().name());
 
 int fRow = varInfo[bRow].firstElem;
 int fCol = varInfo[bCol].firstElem;
 
 label sizeRow = varInfo[bRow].nCells;  
 label sizeCol = varInfo[bCol].nCells;  
 
 List<labelList>& rel = matrix.rowColList();
 
 forAll(rel, i)
 {   
   int rowBias = fRow + (rel[i][0]*sizeRow);
   int colBias = fCol + (rel[i][1]*sizeCol);
   direction cmpt = rel[i][2];
   
   if (initTimeFlag || updateA_ || !saveSystem_)
   {
     assemblePetscAb
     (
       A,
       b,
       x,
       matrix,
       cmpt,  
       rowBias,
       colBias,
       bRow
     );
   }
   else
   {
     assemblePetscb
     (
       b,
       matrix,
       cmpt,  
       rowBias,
       bRow
     );
   }
 } 
 
 isSet = true;
}


template<class Type>
void Foam::coupledSolver::insertEquation
(
  word rowField,
  word colField,
  const tmp<LMatrix<Type>>& tLMatrix
)
{
  insertEquation(rowField, colField, const_cast<LMatrix<Type>&>(tLMatrix()));
  tLMatrix.clear();
}

template<class Type>
void Foam::coupledSolver::getSolution()  
{
 Type type(pTraits<Type>::zero); 
   
 List<GeometricField<Type, fvPatchField, volMesh>*>& varList(varTypeList(type));

 forAll(varList, i)
 {
   int g(-1);  
   forAll(varInfo, j)
   {
     if (varInfo[j].localID == i && varInfo[j].typeV == ftType(type))
       g = j;
   }
   
   label fPos = varInfo[g].firstElem;
   label sizeField = varInfo[g].nCells;
   label mID = varInfo[g].meshID;  
   
   typename pTraits<Type>::labelType validComponents
   (
     varList[i]->mesh().template validComponents<Type>()
   );
   
   // varList[i]->correctBoundaryConditions();
   
   int j = 0;
   for (direction cmpt=0; cmpt<pTraits<Type>::nComponents; cmpt++)
   {
     if (component(validComponents, cmpt) == -1) continue; 
     
     int rowBias = fPos+(j*sizeField);
     transferPetscSolution(x, *varList[i], cmpt, rowBias, mID);  
     
     j++;
   }
   
   varList[i]->correctBoundaryConditions();
   
  // Info << min(*varList[i])<< tab << max (*varList[i]) << endl;  
 }
}

template<class Type>
void Foam::coupledSolver::transferPetscSolution
(
  Vec& x,
  Foam::GeometricField<Type, fvPatchField, volMesh>& T,
  int cmpI,
  int rowBias,
  int mID
)
{
 int ilower = this->sharedData[meshList[mID].ID].ilower;
 int n = T.size();
 
 // If run in parallel, we cannot VecGetValues() from processors different
 // from the one we are. Therefore we need to scatter and gather the values
 // we need to our processor. 
 if (Pstream::parRun())
 {
   Vec xloc;  
   VecScatter scatter;  
   IS from, to;  
   PetscScalar *values;

   scalarField tt(n, 0.);
   std::vector<int> idx_from(n);
   std::vector<int> idx_to(n);
   for (int i = 0; i < n; i++)
   {
    idx_from[i] = ilower + i + rowBias;
    idx_to[i] = i;
   }

   VecCreateSeq(PETSC_COMM_SELF,n,&xloc);

   ISCreateGeneral(PETSC_COMM_SELF,n,&idx_from[0],PETSC_COPY_VALUES,&from);
   ISCreateGeneral(PETSC_COMM_SELF,n,&idx_to[0],PETSC_COPY_VALUES,&to);

   VecScatterCreate(x,from,xloc,to,&scatter);
   VecScatterBegin(scatter,x,xloc,INSERT_VALUES,SCATTER_FORWARD);
   VecScatterEnd(scatter,x,xloc,INSERT_VALUES,SCATTER_FORWARD);

   VecGetArray(xloc,&values);
   for (int i = 0; i < n; i++)
    tt[i] = values[i];

   T.primitiveFieldRef().replace(cmpI, tt);

   ISDestroy(&from);
   ISDestroy(&to);
   VecScatterDestroy(&scatter);
   VecDestroy(&xloc);
 }
 else
 { 
   std::vector<int> rows(n);
        
   for (int i = 0; i < n; i++)
    rows[i] = ilower + i + rowBias;
    
   scalarField tt(n, 0.);
   VecGetValues(x, n, &rows[0], tt.begin());  
   
   T.primitiveFieldRef().replace(cmpI, tt);
 }
} 

template<class eqType>
void Foam::coupledSolver::assemblePetscAb
(
  Mat& A,
  Vec& b,
  Vec& x,
  eqType& eqn,
  int cmpI,
  int rowBias,
  int colBias,
  int rowVarID
)
{
  
 // Note: in general rowField and colField may belong to different meshes. Here
 // we will assume they are the same.
 label mID = varInfo[rowVarID].meshID;  
 
 int ilower = this->sharedData[meshList[mID].ID].ilower;
 
 // Start filling the matrix/vector
 
 //- Off diagonal elements   
 const lduAddressing& lduA = eqn.lduAddr();
 int col; int row;
 int nFaces = lduA.lowerAddr().size();
 
 // Symmetric matrices only have upper(). No need to force creation of lower().
 if (eqn.symmetric())
 {
   scalarField upper(eqn.upper().component(cmpI));
   for (register label face=0; face<nFaces; face++)
   {    
     // ij Off-diagonal
     row = ilower + lduA.upperAddr()[face] + rowBias;
     col = ilower + lduA.lowerAddr()[face] + colBias;
     ierr = MatSetValues(A,1,&row,1,&col,&upper[face],ADD_VALUES);CHKERRV(ierr);
     
     // ji Off-diagonal
     row = ilower + lduA.lowerAddr()[face] + rowBias;
     col = ilower + lduA.upperAddr()[face] + colBias;
     ierr = MatSetValues(A,1,&row,1,&col,&upper[face],ADD_VALUES);CHKERRV(ierr);   
   }
 }
 else
 {
   scalarField lower(eqn.lower().component(cmpI));
   scalarField upper(eqn.upper().component(cmpI)); 
   for (register label face=0; face<nFaces; face++)
   {   
     // ij Off-diagonal
     row = ilower + lduA.upperAddr()[face] + rowBias;
     col = ilower + lduA.lowerAddr()[face] + colBias;
     ierr = MatSetValues(A,1,&row,1,&col,&lower[face],ADD_VALUES);CHKERRV(ierr);
     
     // ji Off-diagonal
     row = ilower + lduA.lowerAddr()[face] + rowBias;
     col = ilower + lduA.upperAddr()[face] + colBias;
     ierr = MatSetValues(A,1,&row,1,&col,&upper[face],ADD_VALUES);CHKERRV(ierr);   
   }
 }
 
 // Diagonal and source  
 scalarField source(eqn.source().component(cmpI));  
 scalarField diag(eqn.diag().component(cmpI));
 int n = source.size(); 
 for (int cellI=0; cellI<n; cellI++) 
 {  
   // Diagonal
   row = ilower + cellI + rowBias; 
   col = ilower + cellI + colBias; 
   ierr = MatSetValues(A,1,&row,1,&col,&diag[cellI],ADD_VALUES);CHKERRV(ierr);
   
   // Source vector   
   ierr = VecSetValues(b,1,&row,&source[cellI],ADD_VALUES);CHKERRV(ierr);
 }
 
 const fvBoundaryMesh& bMesh(meshList[mID].mesh->boundary());
  
 //- Contribution from BCs
 forAll(bMesh, patchI)
 {
   scalarField bC(eqn.boundaryCoeffs()[patchI].component(cmpI));  
   scalarField iC(eqn.internalCoeffs()[patchI].component(cmpI));  
   const labelUList& addr = lduA.patchAddr(patchI);
   const fvPatch& pfvPatch = bMesh[patchI];      
   
   // Check special (non-general) boundary conditions
   #include "specialBoundariesA.H" 
 
   // Non-coupled
   if (!bMesh[patchI].coupled())
   {
     forAll(addr, facei)
     {
       // Matrix of coefs - Diagonal
       row = ilower + addr[facei] + rowBias;  
       col = ilower + addr[facei] + colBias;  
       ierr = MatSetValues(A,1,&row,1,&col,&iC[facei],ADD_VALUES);CHKERRV(ierr);
       
       // Source vector
       ierr = VecSetValues(b,1,&row,&bC[facei],ADD_VALUES);CHKERRV(ierr);       
     }
   }
   // Processor or processorCyclic.
   // Note: from exchange ops point of view both patch types are equivalent.
   // Only difference is when building the sharedData struct, where the names
   // used to find neig patch in neig proc are different.
   else if (   isType<processorFvPatch>(bMesh[patchI])
            || isType<processorCyclicFvPatch>(bMesh[patchI])
           )
   { 
     // Matrix of coefs - Diagonal
     forAll(addr, facei)
     {      
       row = ilower + addr[facei] + rowBias;
       col = ilower + addr[facei] + colBias;   
       ierr = MatSetValues(A,1,&row,1,&col,&iC[facei],ADD_VALUES);CHKERRV(ierr);       
     } 
     
     // Matrix of coefs - off-diagonal (row -> this processor; col -> other processors)
     // Unique face contribution to off-diag (set instead of add). If use add, then the
     // inter-processor off-diag coefs need to be reset to 0 before.
     forAll(this->sharedData[meshList[mID].ID].procInfo, pI)
     {
       if (this->sharedData[meshList[mID].ID].procInfo[pI][2] == patchI)
       {
         forAll(bC, facei)
         {        
           double v = -bC[facei];
           int row = this->sharedData[meshList[mID].ID].fCo[pI][facei] + rowBias;  
           int col = this->sharedData[meshList[mID].ID].fCn[pI][facei] + colBias; 
           ierr = MatSetValues
           (
             A,
             1,
             &row,
             1,
             &col,
             &v,
             ADD_VALUES
           ); 
           CHKERRV(ierr);     
         } 
       }
     }  
   }
   // Cyclic. 
   // Note: cyclic patches (if not empty after decomposition) always contain
   // both own and neig cells in the same processor. Therefore, no data exchange 
   // is needed, and biasing with ilower is enough to account for global indexing.  
   else if (isType<cyclicFvPatch>(bMesh[patchI]))
   { 
     // Row
     const labelList& owfC = meshList[mID].mesh->boundaryMesh()[patchI].faceCells();
     
     const fvPatch& cyclicPatch = bMesh[patchI];
    
     const fvPatch& nbrPatch = refCast<const cyclicFvPatch>
        (
            cyclicPatch
        ).neighbFvPatch();
      
     // Col   
     const labelList& nbFC = nbrPatch.patch().faceCells();
     
     forAll(owfC, facei)
     {                  
       // Matrix of coefs - Diagonal  
       row = ilower + owfC[facei] + rowBias;  
       col = ilower + owfC[facei] + colBias; 
       ierr = MatSetValues(A,1,&row,1,&col,&iC[facei],ADD_VALUES);CHKERRV(ierr);  
       
       // Matrix of coefs - off-diagonal
       col = ilower + nbFC[facei] + colBias; 
       double v = -bC[facei];
       ierr = MatSetValues(A,1,&row,1,&col,&v,ADD_VALUES);CHKERRV(ierr);       
     } 
       
   }
   // cyclicAMIFvPatch
   // Note: contrary to cyclic, cyclicAMI patches do not get transformed in processorCyclicAMI
   // when they are split between processors. The AMI machinery allows mapping between 
   // processors. 
   else if (isType<cyclicAMIFvPatch>(pfvPatch))
   {        
     // camipp and neicampi always lay in the same processor, but one of them can be empty
     // if the other half of a given AMI was sent to another processor. In pratice, nothing
     // happens bellow if camipp is zero-sized.
     const cyclicAMIPolyPatch& camipp = refCast<const cyclicAMIFvPatch>(pfvPatch).cyclicAMIPatch();
     const cyclicAMIPolyPatch& neicamipp = camipp.nbrPatch();
     
     const labelList& ownFC = camipp.faceCells();   
     const labelList& neiFC = neicamipp.faceCells();
     
     // The neiFC faceCells already get the proc's ilower, but still need correction by colBias
     labelList neiFCproc(neiFC+ilower);
     
     // AMIs are shared by at least 2 patches, but the AMI interpolator is only accessible
     // from one of them, which is considered the "owner" patch.
     if (camipp.owner())
     {      
       forAll(camipp.AMIs(), i)
       {
         // Distribute faceCells of neighbour (tgt) patch if parallel. We get
         // the tgt faceCells transfered to the proc where the src patch (camipp) is.
         if (camipp.AMIs()[i].singlePatchProc() == -1) 
           camipp.AMIs()[i].tgtMap().distribute(neiFCproc);   
     
         const labelListList& srcAd = camipp.AMIs()[i].srcAddress();
         const scalarListList& srcW = camipp.AMIs()[i].srcWeights();
         forAll(srcAd, k)
         {             
           // Matrix of coefs - Diagonal  
           row = ilower + ownFC[k] + rowBias; 
           col = ilower + ownFC[k] + colBias;  
           ierr = MatSetValues(A,1,&row,1,&col,&iC[k],ADD_VALUES);CHKERRV(ierr); 
         
           // If the applyLowWeightCorrection option is enabled, the zero-gradient
           // condition must be applied when the weightSum is less than a treshold.  
           if (camipp.AMIs()[i].applyLowWeightCorrection())
           {
             // Apply implicit zero-gradient
             if (camipp.AMIs()[i].srcWeightsSum()[k] < camipp.AMIs()[i].lowWeightCorrection())
             {
               col = ilower + ownFC[k] + colBias;
               double v = -bC[k];
               ierr = MatSetValues(A,1,&row,1,&col,&v,ADD_VALUES);CHKERRV(ierr); 
             }
             // Distribute weighted coefficients 
             else
             {
               // Matrix of coefs - off-diagonal
               forAll(srcAd[k], kk)
               {                    
                col = neiFCproc[srcAd[k][kk]] + colBias;  
                double v = -bC[k]*srcW[k][kk];
                ierr = MatSetValues(A,1,&row,1,&col,&v,ADD_VALUES);CHKERRV(ierr);          
               }
             }                                           
           }
           else
           {
              // Matrix of coefs - off-diagonal
              forAll(srcAd[k], kk)
              {                        
               col = neiFCproc[srcAd[k][kk]] + colBias; 
               double v = -bC[k]*srcW[k][kk];
               ierr = MatSetValues(A,1,&row,1,&col,&v,ADD_VALUES);CHKERRV(ierr);          
              }   
           }
         }
         
       }
  
     }
     else
     { 
      forAll(neicamipp.AMIs(), i)
       {
         // Distribute faceCells of neighbour (src) patch if parallel. We get
         // the src faceCells transfered to the proc where the tgt patch (camipp) is
         if (neicamipp.AMIs()[i].singlePatchProc() == -1) 
           neicamipp.AMIs()[i].srcMap().distribute(neiFCproc);   
       
         const labelListList& tgtAd = neicamipp.AMIs()[i].tgtAddress();
         const scalarListList& tgtW = neicamipp.AMIs()[i].tgtWeights();
         forAll(tgtAd, k)
         {                         
           // Matrix of coefs - Diagonal 
           row = ilower + ownFC[k] + rowBias; 
           col = ilower + ownFC[k] + colBias;     
           ierr = MatSetValues(A,1,&row,1,&col,&iC[k],ADD_VALUES);CHKERRV(ierr);            
                     
           // If the applyLowWeightCorrection option is enabled, the zero-gradient
           // condition must be applied when the weightSum is less than a treshold.  
           if (neicamipp.AMIs()[i].applyLowWeightCorrection())
           {
             // Apply implicit zero-gradient
             if (neicamipp.AMIs()[i].tgtWeightsSum()[k] < neicamipp.AMIs()[i].lowWeightCorrection())
             {
               col = ilower + ownFC[k] + colBias;
               double v = -bC[k];
               ierr = MatSetValues(A,1,&row,1,&col,&v,ADD_VALUES);CHKERRV(ierr); 
             }
             // Distribute weighted coefficients 
             else
             {
               // Matrix of coefs - off-diagonal
               forAll(tgtAd[k], kk)
               {                
                col = neiFCproc[tgtAd[k][kk]] + colBias; 
                double v = -bC[k]*tgtW[k][kk];
                ierr = MatSetValues(A,1,&row,1,&col,&v,ADD_VALUES);CHKERRV(ierr);          
               }  
             }                                           
           }
           else
           {
              // Matrix of coefs - off-diagonal
              forAll(tgtAd[k], kk)
              {                
               col = neiFCproc[tgtAd[k][kk]] + colBias; 
               double v = -bC[k]*tgtW[k][kk];
               ierr = MatSetValues(A,1,&row,1,&col,&v,ADD_VALUES);CHKERRV(ierr);          
              }   
           }
              
         }
       }
    }
    
   } 
   // NOT IMPLEMENTED 
   else
   {   
     FatalErrorInFunction
     << "Sorry, but Petsc interface cannot support patches of type: "
     << pfvPatch.type() << ", which is currently set for "
     << "patch " << pfvPatch.name() << "."
     << exit(FatalError);    
   } 
 }  
}  

template<class eqType>
void Foam::coupledSolver::assemblePetscb
(
  Vec& b,
  eqType& eqn,
  int cmpI,
  int rowBias,
  int rowVarID
)
{
 // Note: in general rowField and colField may belong to different meshes. Here
 // we will assume they are the same.
 label mID = varInfo[rowVarID].meshID; 
 
 int ilower = this->sharedData[meshList[mID].ID].ilower;
 
 // Start filling the vector
 
 const lduAddressing& lduA = eqn.lduAddr();
 int row;
 
 // Diagonal and source  
 scalarField source(eqn.source().component(cmpI));  
 int n = source.size(); 
 for (int cellI=0; cellI<n; cellI++) 
 {  
   // Diagonal
   row = ilower + cellI + rowBias; 
      
   // Source vector   
   ierr = VecSetValues(b,1,&row,&source[cellI],ADD_VALUES);CHKERRV(ierr);
 }
 
 const fvBoundaryMesh& bMesh(meshList[mID].mesh->boundary());
 
 //- Contribution from BCs
 forAll(bMesh, patchI)
 {  
   // Check special (non-general) boundary conditions
   #include "specialBoundariesB.H" 
   
   // Non-coupled
   if (!bMesh[patchI].coupled())
   {
     scalarField bC(eqn.boundaryCoeffs()[patchI].component(cmpI));   
     const labelUList& addr = lduA.patchAddr(patchI); 
     
     forAll(addr, facei)
     {
       row = ilower + addr[facei] + rowBias;  
              
       // Source vector
       ierr = VecSetValues(b,1,&row,&bC[facei],ADD_VALUES);CHKERRV(ierr);       
     }
   }    
 }  
} 
 
