
#include "dzprogress.h"
#include "open_fbx_interface.h"
#include "fbx_common.h"

#include <fbxsdk.h>
#include <QTCore/qlist.h>
#include <QTCore/qmap.h>
#include <dzapp.h>

double FbxCommon::getLength(double a, double b, double c)
{
	double distance = 0;
	double a2 = a * a;
	double b2 = b * b;
	double c2 = c * c;
	distance = sqrt(a2 + b2 + c2);
	return distance;
}

double FbxCommon::getLength(double a, double b)
{
	double distance = 0;
	double a2 = a * a;
	double b2 = b * b;
	distance = sqrt(a2 + b2);
	return distance;
}

double FbxCommon::getDistance(FbxVector4 a, FbxVector4 b)
{
	FbxVector4 ab = b - a;
	double distance = getLength(ab[0], ab[1], ab[2]);
	return distance;
}

double FbxCommon::getDistance(FbxVector2 a, FbxVector2 b)
{
	FbxVector2 ab = b - a;
	double distance = getLength(ab[0], ab[1]);
	return distance;
}

double FbxCommon::determinant_3x3(FbxVector4* matrix)
{
	double return_value = 0.0;

	double mat1 = matrix[0][0] * matrix[1][1] * matrix[2][2];
	double mat2 = matrix[0][1] * matrix[1][2] * matrix[2][0];
	double mat3 = matrix[0][2] * matrix[1][0] * matrix[2][1];

	double mat4 = matrix[0][2] * matrix[1][1] * matrix[2][0];
	double mat5 = matrix[0][1] * matrix[1][0] * matrix[2][2];
	double mat6 = matrix[0][0] * matrix[1][2] * matrix[2][1];

	return_value = mat1 + mat2 + mat3 - mat4 - mat5 - mat6;

	return return_value;
}

FbxVector4* FbxCommon::CalculateBoundingVolume(QList<FbxVector4>& pointCloud)
{
	FbxVector4* result = new FbxVector4[3];

	if (pointCloud.isEmpty())
	{
		return result;
	}

	FbxVector4 cloudCenter;

	FbxVector4 maxBounds = pointCloud[0];
	FbxVector4 minBounds = pointCloud[0];
	FbxVector4 sum(0, 0, 0);
	for (FbxVector4 currentPoint : pointCloud)
	{
		for (int i = 0; i < 3; i++)
		{
			if (maxBounds[i] < currentPoint[i])
			{
				maxBounds[i] = currentPoint[i];
			}
			if (minBounds[i] > currentPoint[i])
			{
				minBounds[i] = currentPoint[i];
			}
		}
		sum += currentPoint;
	}

	FbxVector4 cloudAverage = sum / pointCloud.count();

	cloudCenter[0] = (maxBounds[0] + minBounds[0]) / 2;
	cloudCenter[1] = (maxBounds[1] + minBounds[1]) / 2;
	cloudCenter[2] = (maxBounds[2] + minBounds[2]) / 2;

	FbxVector4 cloudSize;
	cloudSize[0] = abs(maxBounds[0] - minBounds[0]);
	cloudSize[1] = abs(maxBounds[1] - minBounds[1]);
	cloudSize[2] = abs(maxBounds[2] - minBounds[2]);

	result[0] = cloudSize;
	result[1] = cloudCenter;
	result[2] = cloudAverage;

	return result;
}

FbxVector4* FbxCommon::CalculateBoundingVolume(FbxMesh* mesh)
{
	FbxVector4* result = new FbxVector4[3];

	FbxVector4 cloudCenter;
	FbxVector4 minBounds;
	FbxVector4 maxBounds;
	FbxVector4 sum(0, 0, 0);

	int numPoints = mesh->GetControlPointsCount();
	for (int vertex_index = 0; vertex_index < numPoints; vertex_index++)
	{
		FbxVector4 currentPoint = mesh->GetControlPointAt(vertex_index);
		sum += currentPoint;
		if (vertex_index == 0)
		{
			minBounds = maxBounds = currentPoint;
			continue;
		}
		for (int axis_index = 0; axis_index < 3; axis_index++)
		{
			if (currentPoint[axis_index] < minBounds[axis_index])
				minBounds[axis_index] = currentPoint[axis_index];
			if (currentPoint[axis_index] > maxBounds[axis_index])
				maxBounds[axis_index] = currentPoint[axis_index];
		}
	}

	FbxVector4 cloudAverage = sum / numPoints;

	cloudCenter[0] = (maxBounds[0] + minBounds[0]) / 2;
	cloudCenter[1] = (maxBounds[1] + minBounds[1]) / 2;
	cloudCenter[2] = (maxBounds[2] + minBounds[2]) / 2;

	FbxVector4 cloudSize;
	cloudSize[0] = abs(maxBounds[0] - minBounds[0]);
	cloudSize[1] = abs(maxBounds[1] - minBounds[1]);
	cloudSize[2] = abs(maxBounds[2] - minBounds[2]);

	result[0] = cloudSize;
	result[1] = cloudCenter;
	result[2] = cloudAverage;

	return result;
}

FbxVector4* FbxCommon::CalculateBoundingVolume(FbxMesh* mesh, QList<int>* vertexIndexes)
{
	FbxVector4* result = new FbxVector4[3];
	FbxVector4 cloudAverage;
	FbxVector4 cloudCenter;
	FbxVector4 minBounds;
	FbxVector4 maxBounds;

	bool bFirstElement = true;
	double totalWeights = 0.0;
	for (int vertex_index : (*vertexIndexes))
	{
		FbxVector4 currentPoint = mesh->GetControlPointAt(vertex_index);
		if (bFirstElement)
		{
			bFirstElement = false;
			minBounds = maxBounds = currentPoint;
			cloudAverage = currentPoint;
			totalWeights = 1.0;
			continue;
		}
		for (int axis_index = 0; axis_index < 3; axis_index++)
		{
			cloudAverage += currentPoint;
			totalWeights += 1.0;
			if (currentPoint[axis_index] < minBounds[axis_index])
				minBounds[axis_index] = currentPoint[axis_index];
			if (currentPoint[axis_index] > maxBounds[axis_index])
				maxBounds[axis_index] = currentPoint[axis_index];
		}
	}

	cloudAverage = cloudAverage / totalWeights;

	cloudCenter[0] = (maxBounds[0] + minBounds[0]) / 2;
	cloudCenter[1] = (maxBounds[1] + minBounds[1]) / 2;
	cloudCenter[2] = (maxBounds[2] + minBounds[2]) / 2;

	FbxVector4 cloudSize;
	cloudSize[0] = abs(maxBounds[0] - minBounds[0]);
	cloudSize[1] = abs(maxBounds[1] - minBounds[1]);
	cloudSize[2] = abs(maxBounds[2] - minBounds[2]);

	result[0] = cloudSize;
	result[1] = cloudCenter;
	result[2] = cloudAverage;

	return result;
}

FbxVector4 FbxCommon::CalculatePointCloudAverage(FbxMesh* mesh, QList<int>* vertexIndexes)
{
	if (mesh == nullptr || vertexIndexes == nullptr || vertexIndexes->count() <= 0)
	{
		dzApp->warning("ERROR: CalculatePointCloudCenter recieved invalid inputs");
		return nullptr;
	}

	FbxVector4 cloudCenter = FbxVector4(0, 0, 0);
	double totalWeights = 0.0;
	for (int vertex_index : (*vertexIndexes))
	{
		FbxVector4 currentPoint = mesh->GetControlPointAt(vertex_index);
		double currentWeight = 1.0;
		cloudCenter[0] += currentPoint[0];
		cloudCenter[1] += currentPoint[1];
		cloudCenter[2] += currentPoint[2];
		totalWeights += currentWeight;
	}
	cloudCenter[0] = cloudCenter[0] / totalWeights;
	cloudCenter[1] = cloudCenter[1] / totalWeights;
	cloudCenter[2] = cloudCenter[2] / totalWeights;

	return cloudCenter;
}

FbxVector4 FbxCommon::CalculatePointCloudCenter(FbxMesh* mesh, QList<int>* vertexIndexes, bool centerWeight)
{

	if (mesh == nullptr || vertexIndexes == nullptr || vertexIndexes->count() <= 0)
	{
		dzApp->warning("ERROR: CalculatePointCloudCenter recieved invalid inputs");
		return nullptr;
	}

	FbxVector4 cloudCenter = mesh->GetControlPointAt(vertexIndexes->first());
	FbxVector4 min_bounds = cloudCenter;
	FbxVector4 max_bounds = cloudCenter;
	for (int vertex_index : (*vertexIndexes))
	{
		FbxVector4 currentPoint = mesh->GetControlPointAt(vertex_index);
		for (int i = 0; i < 3; i++)
		{
			if (currentPoint[i] < min_bounds[i]) min_bounds[i] = currentPoint[i];
			if (currentPoint[i] > max_bounds[i]) max_bounds[i] = currentPoint[i];
		}
	}
	double center_weight = 0;
	if (abs(max_bounds[0]) < abs(min_bounds[0]))
		center_weight = max_bounds[0];
	else
		center_weight = min_bounds[0];

	if (centerWeight)
		//		cloudCenter[0] = (max_bounds[0] + min_bounds[0] + center_weight) / 3;
		cloudCenter[0] = center_weight;
	else
		cloudCenter[0] = (max_bounds[0] + min_bounds[0]) / 2;
	cloudCenter[1] = (max_bounds[1] + min_bounds[1]) / 2;
	cloudCenter[2] = (max_bounds[2] + min_bounds[2]) / 2;

	FbxVector4 cloudAverage = CalculatePointCloudAverage(mesh, vertexIndexes);

	return cloudCenter;

}


////////////////////////////////////////////
/// FBX CLUSTER DEFORM FUNCTIONS

// Scale all the elements of a matrix.
void FbxCommon::MultiplyMatrix_InPlace(FbxAMatrix& matrix, double value)
{

	int i, j;

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			matrix[i][j] *= value;
		}
	}

}

// Add a value to all the elements in the diagonal of the matrix.
void FbxCommon::AddToScaleOfMatrix_InPlace(FbxAMatrix& matrix, double value)
{

	for (int i = 0; i < 4; i++)
	{
		matrix[i][i] += value;
	}

}

// Sum two matrices element by element
void FbxCommon::AddMatrix_InPlace(FbxAMatrix& destinationMatrix, const FbxAMatrix& sourceMatrix)
{

	int i, j;

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			destinationMatrix[i][j] += sourceMatrix[i][j];
		}
	}

}

// Get the matrix of the given pose
FbxAMatrix FbxCommon::GetPoseMatrix(FbxPose* pose, int nodeIndex)
{

	FbxAMatrix lPoseMatrix;
	FbxMatrix lMatrix = pose->GetMatrix(nodeIndex);

	memcpy((double*)lPoseMatrix, (double*)lMatrix, sizeof(lMatrix.mData));

	return lPoseMatrix;

}

FbxAMatrix FbxCommon::GetAffineMatrix(FbxPose* pose, int itemIndex, bool returnLocalSpace, FbxTime fbxTime)
{

	/////////////////////
	// DEFAULT CASES: Return global or local pose matrix (with matching returnLocalSpace)
	////////////////////////
	FbxAMatrix returnMatrix;
	FbxMatrix tempMatrix = pose->GetMatrix(itemIndex);
	memcpy(&returnMatrix, &tempMatrix, sizeof(tempMatrix.mData));

	/////////////////////////
	// OTHER CONDITIONS
	/////////////////////////
	if (pose->IsLocalMatrix(itemIndex) == true && returnLocalSpace == false)
	{
		FbxNode* pParentNode = pose->GetNode(itemIndex)->GetParent();
		if (pParentNode)
		{
			FbxAMatrix parentMatrix;
			int nParentIndex = pose->Find(pParentNode);
			if (nParentIndex > -1)
			{
				parentMatrix = GetAffineMatrix(pose, nParentIndex, returnLocalSpace, fbxTime);
			}
			else
			{
				parentMatrix = pParentNode->EvaluateGlobalTransform(fbxTime);
			}
			FbxAMatrix tempMatrix2 = parentMatrix * returnMatrix;
			returnMatrix = tempMatrix2;
		}
	}
	else if (pose->IsLocalMatrix(itemIndex) == false && returnLocalSpace == true)
	{
		FbxNode* pParentNode = pose->GetNode(itemIndex)->GetParent();
		if (pParentNode)
		{
			FbxAMatrix parentMatrix;
			int nParentIndex = pose->Find(pParentNode);
			if (nParentIndex > -1)
			{
				parentMatrix = GetAffineMatrix(pose, nParentIndex, returnLocalSpace, fbxTime);
			}
			else
			{
				parentMatrix = pParentNode->EvaluateGlobalTransform(fbxTime);
			}
			FbxAMatrix tempMatrix2 = parentMatrix.Inverse() * returnMatrix;
			returnMatrix = tempMatrix2;
		}
	}
	////////
	// NOTE: DEFAULT CASES ALREADY ASSIGNED ABOVE
	////////

	return returnMatrix;

}

// Return matrix of node, using pose if it is present, using WS matrix by default, using time infinite by default
FbxAMatrix FbxCommon::GetAffineMatrix(FbxPose* pose, FbxNode* node, bool returnLocalSpace, FbxTime fbxTime)
{

	FbxAMatrix returnMatrix;

	if (pose != nullptr)
	{
		int nodeIndex = pose->Find(node);
		if (nodeIndex == -1)
		{
			QString sActiveNodeName(node->GetName());
			for (int i = 0; i < pose->GetCount(); i++)
			{
				FbxNode* current_node = pose->GetNode(i);
				QString sCurrentNodeName(current_node->GetName());
				if (sCurrentNodeName.contains(sActiveNodeName) == true)
				{
					nodeIndex = i;
					break;
				}
			}
		}

		if (nodeIndex > -1)
		{
			returnMatrix = FbxCommon::GetAffineMatrix(pose, nodeIndex, returnLocalSpace);
			return returnMatrix;
		}
	}
	if (returnLocalSpace == false)
	{
		returnMatrix = node->EvaluateGlobalTransform(fbxTime);
	}
	else
	{
		returnMatrix = node->EvaluateLocalTransform(fbxTime);
	}

	return returnMatrix;

}

FbxAMatrix FbxCommon::GetGeometricAffineMatrix(FbxNode* node)
{

	FbxVector4 t = node->GetGeometricTranslation(FbxNode::eSourcePivot);
	FbxVector4 r = node->GetGeometricRotation(FbxNode::eSourcePivot);
	FbxVector4 s = node->GetGeometricScaling(FbxNode::eSourcePivot);

	FbxAMatrix returnMatrix(t, r, s);

	return returnMatrix;

}

bool FbxCommon::CalculateClusterDeformationMatrix(FbxAMatrix& clusterDeformationMatrix, FbxCluster* cluster, FbxAMatrix* globalOffsetMatrix, FbxPose* pose, const FbxMesh* mesh, FbxTime fbxTime)
{

	bool bResult = false;
	// if cluster link mode is eAdditive
	if (cluster->GetLinkMode() == FbxCluster::eAdditive)
	{
		FbxAMatrix clusterBindMatrix_x_Geo;
		cluster->GetTransformMatrix(clusterBindMatrix_x_Geo);
		FbxAMatrix meshGeoMatrix = GetGeometricAffineMatrix(mesh->GetNode());
		clusterBindMatrix_x_Geo *= meshGeoMatrix;

		// associate matrix
		FbxAMatrix associateModelMatrix;
		cluster->GetTransformAssociateModelMatrix(associateModelMatrix);

		FbxNode* pAssociateMesh = cluster->GetAssociateModel();
		FbxAMatrix associateGeoMatrix = GetGeometricAffineMatrix(pAssociateMesh);
		FbxAMatrix associateModelPosedMatrix = GetAffineMatrix(pose, cluster->GetAssociateModel(), false, fbxTime);

		FbxAMatrix clusterPosedMatrix = GetAffineMatrix(pose, cluster->GetLink(), false, fbxTime);

		FbxAMatrix clusterLinkBindMatrix_x_Geo;
		cluster->GetTransformLinkMatrix(clusterLinkBindMatrix_x_Geo);
		FbxAMatrix clusterLinkGeoMatrix = GetGeometricAffineMatrix(cluster->GetLink());
		clusterLinkBindMatrix_x_Geo *= clusterLinkGeoMatrix;

		/////// Compute the shift of the link relative to the reference.
		// reference_inverse * associate * associate_geo_inverse * link_geo * link_geo_inverse * reference
		clusterDeformationMatrix = clusterBindMatrix_x_Geo.Inverse() * associateModelMatrix * associateModelPosedMatrix.Inverse() *
			clusterPosedMatrix * clusterLinkBindMatrix_x_Geo.Inverse() * clusterBindMatrix_x_Geo;
		bResult = true;
	}
	else
	{
		FbxAMatrix clusterPosedMatrix = GetAffineMatrix(pose, cluster->GetLink(), false, fbxTime);

		FbxAMatrix clusterLinkBindMatrix_x_Geo;
		cluster->GetTransformLinkMatrix(clusterLinkBindMatrix_x_Geo);

		FbxAMatrix clusterBindMatrix_x_Geo;
		cluster->GetTransformMatrix(clusterBindMatrix_x_Geo);
		FbxAMatrix meshGeoMatrix = GetGeometricAffineMatrix(mesh->GetNode());
		clusterBindMatrix_x_Geo *= meshGeoMatrix;

		// relative_current_inverse * relative_initial
		clusterDeformationMatrix = globalOffsetMatrix->Inverse() * clusterPosedMatrix *
			clusterLinkBindMatrix_x_Geo.Inverse() * clusterBindMatrix_x_Geo;
		bResult = true;
	}

	return bResult;

}

bool FbxCommon::BakePoseToVertexBuffer_LinearPathway(FbxVector4* vertexBuffer, FbxAMatrix* globalOffsetMatrix, FbxPose* pose, const FbxMesh* mesh, FbxTime fbxTime)
{

	bool bResult = false;
	// get cluster link mode
	FbxSkin* pSkinDeformer = (FbxSkin*)mesh->GetDeformer(0, FbxDeformer::eSkin);
	FbxCluster* cluster = pSkinDeformer->GetCluster(0);
	FbxCluster::ELinkMode clusterMode = cluster->GetLinkMode();

	int numVerts = mesh->GetControlPointsCount();
	// prepare cluster matrix buffer (one matrix per vertex)
	FbxAMatrix* pMatrixBuffer = new FbxAMatrix[numVerts];
	memset(pMatrixBuffer, 0, numVerts * sizeof(FbxAMatrix));
	// prepare cluster weight buffer (one weight per vertex)
	double* pWeightBuffer = new double[numVerts];
	memset(pWeightBuffer, 0, numVerts * sizeof(double));
	// if addtive cluster mode, set each matrix in matrix buffer to identity
	if (clusterMode == FbxCluster::eAdditive)
	{
		for (int matrixIndex = 0; matrixIndex < numVerts; matrixIndex++)
		{
			pMatrixBuffer[matrixIndex].SetIdentity();
		}
	}

	// for each cluster in each skindeformer of mesh, calc matrix transform and weights per vertex
	int numSkinDeformers = mesh->GetDeformerCount(FbxSkin::eSkin);
	for (int skinIndex = 0; skinIndex < numSkinDeformers; skinIndex++)
	{
		FbxSkin* pCurrentSkinDeformer = (FbxSkin*)mesh->GetDeformer(skinIndex, FbxSkin::eSkin);
		int numClusters = pCurrentSkinDeformer->GetClusterCount();
		for (int clusterIndex = 0; clusterIndex < numClusters; clusterIndex++)
		{
			if (pCurrentSkinDeformer->GetCluster(clusterIndex)->GetLink() == nullptr)
			{
				//printf("DEBUG: cluster is not linked to any bone, skipping cluster[%i]", clusterIndex);
				continue;
			}
			FbxCluster* pCurrentCluster = pCurrentSkinDeformer->GetCluster(clusterIndex);

			FbxAMatrix clusterTransformMatrix;
			if (CalculateClusterDeformationMatrix(clusterTransformMatrix, pCurrentCluster, globalOffsetMatrix, pose, mesh, fbxTime) == false)
			{
				//printf("ERROR: unable to calculate cluster deformation matrix, skipping cluster[%i]", clusterIndex);
				continue;
			}
			// each cluster has a list of indexes into the global vertex index buffer
			// localIndex == offset into each cluster's buffer of vertex indexes
			// globalIndex == offset into the global vertex buffer
			int numLocalIndexes = pCurrentCluster->GetControlPointIndicesCount();
			for (int localIndex = 0; localIndex < numLocalIndexes; localIndex++)
			{
				int globalIndex = pCurrentCluster->GetControlPointIndices()[localIndex];
				if (globalIndex >= numVerts)
				{
					//printf("ERROR: global vertex index is out of range of global vertex buffer: globalIndex=[%i]", globalIndex);
					continue;
				}
				double fWeightOfVertex = pCurrentCluster->GetControlPointWeights()[localIndex];
				FbxAMatrix weightedTransformMatrix = clusterTransformMatrix;
				MultiplyMatrix_InPlace(weightedTransformMatrix, fWeightOfVertex);
				if (clusterMode == FbxCluster::eAdditive)
				{
					AddToScaleOfMatrix_InPlace(weightedTransformMatrix, 1 - fWeightOfVertex);
					pMatrixBuffer[globalIndex] = weightedTransformMatrix * pMatrixBuffer[globalIndex];
					pWeightBuffer[globalIndex] = 1.0;
				}
				else
				{
					AddMatrix_InPlace(pMatrixBuffer[globalIndex], weightedTransformMatrix);
					pWeightBuffer[globalIndex] += fWeightOfVertex;
				}
			}

		}
	}

	// apply weight * matrix transform to each vertex
	for (int globalIndex = 0; globalIndex < numVerts; globalIndex++)
	{
		FbxVector4 sourceVertex = vertexBuffer[globalIndex];
		FbxVector4 finalTargetVertex;
		double fVertexWeight = pWeightBuffer[globalIndex];
		if (fVertexWeight != 0.0)
		{
			FbxVector4 intermediateVertexValue = pMatrixBuffer[globalIndex].MultT(sourceVertex);
			if (clusterMode == FbxCluster::eNormalize)
			{
				finalTargetVertex = intermediateVertexValue / fVertexWeight;
			}
			else if (clusterMode == FbxCluster::eTotalOne)
			{
				finalTargetVertex = intermediateVertexValue + sourceVertex * (1 - fVertexWeight);
			}
			else
			{
				finalTargetVertex = intermediateVertexValue;
			}
			vertexBuffer[globalIndex] = finalTargetVertex;
		}
	}
	bResult = true;

	// cleanup buffers
	delete[] pMatrixBuffer;
	delete[] pWeightBuffer;

	return bResult;

}

bool FbxCommon::BakePoseToVertexBuffer_DualQuaternionPathway(FbxVector4* vertexBuffer, FbxAMatrix* globalOffsetMatrix, FbxPose* pose, const FbxMesh* mesh, FbxTime fbxTime)
{
	bool bResult = false;
	// get cluster link mode
	FbxSkin* pSkinDeformer = (FbxSkin*)mesh->GetDeformer(0, FbxDeformer::eSkin);
	if (pSkinDeformer->GetClusterCount() < 1)
	{
		dzApp->log("WARNING: FbxCommon::BakePoseToVertexBuffer_DualQuaternionPathway(): SkinDeformer node has zero clusters");
		return false;
	}
	FbxCluster* cluster = pSkinDeformer->GetCluster(0);
	FbxCluster::ELinkMode clusterMode = cluster->GetLinkMode();

	int numVerts = mesh->GetControlPointsCount();
	// prepare dual-quaternion buffer (one DQ per vertex)
	FbxDualQuaternion* pDualQuaternionBuffer = new FbxDualQuaternion[numVerts];
	memset(pDualQuaternionBuffer, 0, numVerts * sizeof(FbxDualQuaternion));
	// prepare cluster weight buffer (one weight per vertex)
	double* pWeightBuffer = new double[numVerts];
	memset(pWeightBuffer, 0, numVerts * sizeof(double));

	// for each cluster of each skindeformer of mesh
	int numSkinDeformers = mesh->GetDeformerCount(FbxSkin::eSkin);
	for (int skinIndex = 0; skinIndex < numSkinDeformers; skinIndex++)
	{
		FbxSkin* pCurrentSkinDeformer = (FbxSkin*)mesh->GetDeformer(skinIndex, FbxSkin::eSkin);
		int numClusters = pCurrentSkinDeformer->GetClusterCount();
		for (int clusterIndex = 0; clusterIndex < numClusters; clusterIndex++)
		{
			if (pCurrentSkinDeformer->GetCluster(clusterIndex)->GetLink() == nullptr)
			{
				//printf("DEBUG: cluster is not linked to any bone, skipping cluster[%i]", clusterIndex);
				continue;
			}
			FbxCluster* pCurrentCluster = pCurrentSkinDeformer->GetCluster(clusterIndex);

			FbxAMatrix clusterTransformMatrix;
			if (CalculateClusterDeformationMatrix(clusterTransformMatrix, pCurrentCluster, globalOffsetMatrix, pose, mesh, fbxTime) == false)
			{
				//printf("ERROR: unable to calculate cluster deformation matrix, skipping cluster[%i]", clusterIndex);
				continue;
			}
			// compute DQ deformation and weight for each vertex
			FbxQuaternion componentQuaternion = clusterTransformMatrix.GetQ();
			FbxVector4 componentTranslation = clusterTransformMatrix.GetT();
			FbxDualQuaternion clusterDualQuaternion(componentQuaternion, componentTranslation);

			// each cluster has a list of indexes into the global vertex index buffer
			// localIndex == offset into each cluster's buffer of vertex indexes
			// globalIndex == offset into the global vertex buffer
			int numLocalIndexes = pCurrentCluster->GetControlPointIndicesCount();
			for (int localIndex = 0; localIndex < numLocalIndexes; localIndex++)
			{
				int globalIndex = pCurrentCluster->GetControlPointIndices()[localIndex];
				if (globalIndex >= numVerts)
				{
					//printf("ERROR: global vertex index is out of range of global vertex buffer: globalIndex=[%i]", globalIndex);
					continue;
				}
				double fWeightOfVertex = pCurrentCluster->GetControlPointWeights()[localIndex];
				if (fWeightOfVertex != 0.0)
				{
					FbxDualQuaternion weightedDualQuaternion = clusterDualQuaternion * fWeightOfVertex;
					if (clusterMode == FbxCluster::eAdditive)
					{
						pDualQuaternionBuffer[globalIndex] = weightedDualQuaternion;
						pWeightBuffer[globalIndex] = 1.0;
					}
					else
					{
						pWeightBuffer[globalIndex] += fWeightOfVertex;
						if (clusterIndex == 0)
						{
							pDualQuaternionBuffer[globalIndex] = weightedDualQuaternion;
						}
						else
						{
							FbxQuaternion quaternionA = pDualQuaternionBuffer[globalIndex].GetFirstQuaternion();
							FbxQuaternion quaternionB = weightedDualQuaternion.GetFirstQuaternion();
							double fSign = quaternionA.DotProduct(quaternionB);
							if (fSign >= 0.0)
							{
								pDualQuaternionBuffer[globalIndex] += weightedDualQuaternion;
							}
							else
							{
								pDualQuaternionBuffer[globalIndex] -= weightedDualQuaternion;
							}
						}
					}

				}

			}


		}
	}

	// apply weighted DQ deformation, based on cluster link mode
	for (int globalIndex = 0; globalIndex < numVerts; globalIndex++)
	{
		FbxVector4 sourceVertex = vertexBuffer[globalIndex];
		FbxVector4 finalTargetVertex = sourceVertex;
		double fVertexWeight = pWeightBuffer[globalIndex];
		if (fVertexWeight != 0.0)
		{
			pDualQuaternionBuffer[globalIndex].Normalize();
			FbxVector4 intermediateVertexValue = pDualQuaternionBuffer[globalIndex].Deform(finalTargetVertex);
			if (clusterMode == FbxCluster::eNormalize)
			{
				finalTargetVertex = intermediateVertexValue / fVertexWeight;
			}
			else if (clusterMode == FbxCluster::eTotalOne)
			{
				finalTargetVertex = intermediateVertexValue + sourceVertex * (1.0 - fVertexWeight);
			}
			else
			{
				finalTargetVertex = intermediateVertexValue;
			}

			vertexBuffer[globalIndex] = finalTargetVertex;
		}
	}
	bResult = true;

	// cleanup buffers
	delete[] pDualQuaternionBuffer;
	delete[] pWeightBuffer;

	return bResult;
}

bool FbxCommon::BakePoseToVertexBuffer(FbxVector4* vertexBuffer, FbxAMatrix* globalOffsetMatrix, FbxPose* pose, const FbxMesh* mesh, FbxTime time)
{
	bool bResult = false;
	// get skin deformer for mesh
	FbxSkin* pSkinDeformer = (FbxSkin*)mesh->GetDeformer(0, FbxDeformer::eSkin);
	if (!pSkinDeformer)
	{
		// do unskinned bake
		bResult = MultiplyMatrixToVertexBuffer(globalOffsetMatrix, vertexBuffer, mesh->GetControlPointsCount());
		return bResult;
	}
	FbxSkin::EType skinningType = pSkinDeformer->GetSkinningType();

	// choose linear, dual-quaternion or blend pathways
	switch (skinningType)
	{
	case FbxSkin::eLinear:
	case FbxSkin::eRigid:
		bResult = BakePoseToVertexBuffer_LinearPathway(vertexBuffer, globalOffsetMatrix, pose, mesh, time);
		break;
	case FbxSkin::eDualQuaternion:
		bResult = BakePoseToVertexBuffer_DualQuaternionPathway(vertexBuffer, globalOffsetMatrix, pose, mesh, time);
		break;
	case FbxSkin::eBlend:
		// create temp vertex buffers to compute linear & quaternion pathways
		// linear
		int numVerts = mesh->GetControlPointsCount();
		FbxVector4* pVertexBuffer_Linear = new FbxVector4[numVerts];
		memcpy(pVertexBuffer_Linear, mesh->GetControlPoints(), numVerts * sizeof(FbxVector4));
		BakePoseToVertexBuffer_LinearPathway(pVertexBuffer_Linear, globalOffsetMatrix, pose, mesh, time);
		// dual-quaternion
		FbxVector4* pVertexBuffer_DQ = new FbxVector4[numVerts];
		memcpy(pVertexBuffer_DQ, mesh->GetControlPoints(), numVerts * sizeof(FbxVector4));
		BakePoseToVertexBuffer_DualQuaternionPathway(pVertexBuffer_DQ, globalOffsetMatrix, pose, mesh, time);
		// linear-interpolate between the two buffer results
		int numBlendWeights = pSkinDeformer->GetControlPointIndicesCount();
		double* pBlendWeightBuffer = pSkinDeformer->GetControlPointBlendWeights();
		for (int nVertexIndex = 0; nVertexIndex < numBlendWeights; nVertexIndex++)
		{
			double fBlendWeight = pBlendWeightBuffer[nVertexIndex];
			FbxVector4 linearResult = pVertexBuffer_Linear[nVertexIndex];
			FbxVector4 dqResult = pVertexBuffer_DQ[nVertexIndex];
			vertexBuffer[nVertexIndex] = (linearResult * fBlendWeight) + (dqResult * (1 - fBlendWeight));
		}
		// cleanup buffers
		delete[] pVertexBuffer_Linear;
		delete[] pVertexBuffer_DQ;
		bResult = true;
		break;
	}

	return bResult;
}

////////////////////////////////////////////
/// FBX POSE FUNCTIONS
FbxAMatrix FbxCommon::FindPoseMatrixOrIdentity(FbxPose* pose, FbxNode* node)
{
	FbxAMatrix returnMatrix;

	int nodeIndex = pose->Find(node);
	if (nodeIndex > -1)
	{
		returnMatrix = FbxCommon::GetAffineMatrix(pose, nodeIndex);
	}
	else
	{
		returnMatrix.SetIdentity();
	}

	return returnMatrix;
}

FbxAMatrix FbxCommon::FindPoseMatrixOrGlobal(FbxPose* pose, FbxNode* node)
{
	FbxAMatrix returnMatrix;

	int nodeIndex = pose->Find(node);
	if (nodeIndex > -1)
	{
		returnMatrix = FbxCommon::GetAffineMatrix(pose, nodeIndex);
	}
	else
	{
		returnMatrix = node->EvaluateGlobalTransform(FBXSDK_TIME_INFINITE);
	}

	return returnMatrix;
}

void FbxCommon::RemoveBindPoses(FbxScene* scene)
{
	QList<int> poseIndexesToDelete;
	int numPoses = scene->GetPoseCount();
	for (int PoseIndex = numPoses - 1; PoseIndex >= 0; --PoseIndex)
	{
		FbxPose* pose = scene->GetPose(PoseIndex);
		if (pose->IsBindPose())
		{
			//			ApplyPose(scene, pose);
			for (int nGeoIndex = 0; nGeoIndex < scene->GetGeometryCount(); nGeoIndex++)
			{
				FbxMesh* mesh = (FbxMesh*)scene->GetGeometry(0);
				FbxVector4* vertexBuffer = mesh->GetControlPoints();
				//				ComputeSkinDeformation(GetGlobalPosition(mesh->GetNode(), FbxTime(0), pose), mesh, FbxTime(0), vertexBuffer, NULL);
			}
			poseIndexesToDelete.append(PoseIndex);

		}
	}

	for (int i : poseIndexesToDelete)
	{
		scene->RemovePose(i);
	}

}

FbxPose* FbxCommon::SaveBindMatrixToPose(FbxScene* scene, const char* poseName, FbxNode* meshNode, bool addPose)
{
	FbxPose* pNewBindPose = FbxPose::Create(scene->GetFbxManager(), poseName);

	QList<FbxNode*> todoList;
	FbxNode* pRootNode = scene->GetRootNode();
	if (meshNode != nullptr)
	{
		pRootNode = meshNode;
	}
	todoList.append(pRootNode);

	while (todoList.isEmpty() == false)
	{
		FbxNode* pCurrentMeshNode = todoList.front();
		todoList.pop_front();
		const char* lpCurrentMeshNodeName = pCurrentMeshNode->GetName();
		FbxGeometry* pGeometry = static_cast<FbxGeometry*>(pCurrentMeshNode->GetMesh());
		if (pGeometry)
		{
			for (int nDeformerIndex = 0; nDeformerIndex < pGeometry->GetDeformerCount(); ++nDeformerIndex)
			{
				FbxSkin* pSkin = static_cast<FbxSkin*>(pGeometry->GetDeformer(nDeformerIndex));
				if (pSkin)
				{
					for (int nClusterIndex = 0; nClusterIndex < pSkin->GetClusterCount(); ++nClusterIndex)
					{
						FbxCluster* cluster = pSkin->GetCluster(nClusterIndex);
						FbxNode* pClusterBone = cluster->GetLink();
						const char* pBoneName = pClusterBone->GetName();
						FbxAMatrix bindMatrix;
						cluster->GetTransformLinkMatrix(bindMatrix);
						pNewBindPose->Add(pClusterBone, bindMatrix, false);

						if (QString(pBoneName).contains("lShldrBend", Qt::CaseInsensitive))
						{
							FbxVector4 rotation = bindMatrix.GetR();
							//printf("nop");
						}
						if (QString(pBoneName).contains("lForearmBend", Qt::CaseInsensitive))
						{
							FbxVector4 rotation = bindMatrix.GetR();
							//printf("nop");
						}

					}
				}
			}
		}
		for (int nChildIndex = 0; nChildIndex < pCurrentMeshNode->GetChildCount(); ++nChildIndex)
		{
			FbxNode* pChildBone = pCurrentMeshNode->GetChild(nChildIndex);
			todoList.push_back(pChildBone);
		}
	}
	if (addPose)
	{
		scene->AddPose(pNewBindPose);
	}
	return pNewBindPose;
}

void FbxCommon::ApplyBindPose(FbxScene* scene, FbxPose* pose, FbxNode* node, bool recurse, bool clampJoints)
{
	// loop and perform for each node starting with root node
	if (node == nullptr)
	{
		node = scene->GetRootNode();
	}
	const char* lpNodeName = node->GetName();

	FbxAMatrix poseMatrix;
	FbxAMatrix parentMatrix;
	FbxAMatrix localMatrix;
	// find node in main scene
	FbxNode* pParentNode = node->GetParent();
	if (pParentNode == NULL)
	{
		localMatrix = FindPoseMatrixOrGlobal(pose, node);
	}
	else
	{
		const char* lpParentNodeName = pParentNode->GetName();
		parentMatrix = FindPoseMatrixOrGlobal(pose, pParentNode);

		poseMatrix = FindPoseMatrixOrGlobal(pose, node);

		localMatrix = parentMatrix.Inverse() * poseMatrix;
	}

	if (clampJoints)
	{
		//		ClampTransform(node, &localMatrix);
	}

	//// rotation order
	FbxVector4 correctRotation = localMatrix.GetR();
	FbxRotationOrder rotationOrderFixer(node->RotationOrder.Get());
	rotationOrderFixer.M2V(correctRotation, localMatrix);

	node->SetPreRotation(FbxNode::EPivotSet::eSourcePivot, FbxVector4(0, 0, 0));
	node->SetPostRotation(FbxNode::EPivotSet::eSourcePivot, FbxVector4(0, 0, 0));
	node->SetRotationOffset(FbxNode::EPivotSet::eSourcePivot, FbxVector4(0, 0, 0));

	node->LclTranslation.Set(localMatrix.GetT());
	node->LclRotation.Set(correctRotation);
	node->LclScaling.Set(localMatrix.GetS());


	if (QString(lpNodeName).contains("lForearmBend", Qt::CaseInsensitive))
	{
		FbxVector4 localRot = localMatrix.GetR();
		FbxVector4 poseRot = poseMatrix.GetR();
		FbxVector4 parentRot = parentMatrix.GetR();
		const char* lpParentName = pParentNode->GetName();

		//printf("nop");
	}

	if (recurse == false)
		return;

	// apply to all children
	for (int childIndex = 0; childIndex < node->GetChildCount(); childIndex++)
	{
		FbxNode* pChildNode = node->GetChild(childIndex);
		ApplyBindPose(scene, pose, pChildNode, recurse, clampJoints);
	}

}

bool FbxCommon::BakePoseToBindMatrix(FbxMesh* mesh, FbxPose* pose)
{
	// for each cluster,
	// get link node
	// look up link node in pose
	// apply pose matrix to bindmatrix with SetTransformLinkMatrix

	int numSkinDeformers = mesh->GetDeformerCount(FbxSkin::eSkin);
	for (int skinIndex = 0; skinIndex < numSkinDeformers; skinIndex++)
	{
		FbxSkin* pCurrentSkinDeformer = (FbxSkin*)mesh->GetDeformer(skinIndex, FbxSkin::eSkin);
		int numClusters = pCurrentSkinDeformer->GetClusterCount();
		for (int clusterIndex = 0; clusterIndex < numClusters; clusterIndex++)
		{
			FbxCluster* pCurrentCluster = pCurrentSkinDeformer->GetCluster(clusterIndex);
			FbxNode* clusterBone = pCurrentCluster->GetLink();
			if (clusterBone == nullptr)
			{
				//printf("DEBUG: cluster is not linked to any bone, skipping cluster[%i]", clusterIndex);
				continue;
			}

			bool bNoPoseBone = false;
			if (pose != nullptr)
			{
				const char* lpBoneName = clusterBone->GetName();
				QString sSearchName(lpBoneName);
				int poseNodeIndex = -1;
				for (int i = 0; i < pose->GetCount(); i++)
				{
					FbxNode* current_node = pose->GetNode(i);
					QString sCurrentNodeName(current_node->GetName());
					if (sCurrentNodeName == sSearchName)
					{
						poseNodeIndex = i;
						break;
					}
				}
				if (poseNodeIndex != -1)
				{
					assert(pose->IsLocalMatrix(poseNodeIndex) == false);
					FbxAMatrix poseMatrix = GetPoseMatrix(pose, poseNodeIndex);
					pCurrentCluster->SetTransformLinkMatrix(poseMatrix);
				}
				else
				{
					dzApp->log(QString("ERROR: BakePoseToBindMatrix() could not find cluster bone: %1 in pose").arg(sSearchName));
					bNoPoseBone = true;
				}
			}

			if (pose == nullptr || bNoPoseBone == true)
			{
				FbxAMatrix poseMatrix = FbxCommon::GetAffineMatrix(nullptr, clusterBone);
				pCurrentCluster->SetTransformLinkMatrix(poseMatrix);
			}

		}
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////
/// FBX SCENE FUNCTIONS
FbxNode* FbxCommon::GetRootBone(FbxScene* scene, bool renameRootBone, FbxNode* previousBone)
{
	QList<FbxNode*> todoList;

	// Find the root bone.  There should only be one bone off the scene root
	FbxNode* pRootNode = scene->GetRootNode();
	FbxNode* rootBone = nullptr;
	int rootChildCount = pRootNode->GetChildCount();
	int rootBoneCount = 0;
	bool bFoundPrevious = false;
	for (int nChildIndex = 0; nChildIndex < rootChildCount; ++nChildIndex)
	{
		FbxNode* pChildNode = pRootNode->GetChild(nChildIndex);
		if (previousBone != nullptr && bFoundPrevious == false)
		{
			if (pChildNode == previousBone)
				bFoundPrevious = true;
			continue;
		}
		FbxNodeAttribute* pAttr = pChildNode->GetNodeAttribute();
		if (pAttr && pAttr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			rootBoneCount++;
			rootBone = pChildNode;
			const char* lpRootBoneName = rootBone->GetName();
			if (renameRootBone)
			{
				rootBone->SetName("root");
				pAttr->SetName("root");
			}
			break;
		}
		todoList.append(pChildNode);
	}

	// if first layer failed, search each successive layer
	if (rootBone == nullptr)
	{
		while (todoList.isEmpty() == false)
		{
			if (rootBone)
			{
				break;
			}
			FbxNode* node = todoList.front();
			todoList.pop_front();
			int nChildCount = node->GetChildCount();
			for (int nChildIndex = 0; nChildIndex < nChildCount; nChildIndex++)
			{
				FbxNode* pChildNode = node->GetChild(nChildIndex);
				if (previousBone != nullptr && bFoundPrevious == false)
				{
					if (pChildNode == previousBone)
						bFoundPrevious = true;
					continue;
				}
				FbxNodeAttribute* pAttr = pChildNode->GetNodeAttribute();
				if (pAttr && pAttr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
				{
					rootBoneCount++;
					rootBone = pChildNode;
					const char* lpRootBoneName = rootBone->GetName();
					if (renameRootBone)
					{
						rootBone->SetName("root");
						pAttr->SetName("root");
					}
					break;
				}
				todoList.append(pChildNode);
			}
		}
	}

	return rootBone;
}

void FbxCommon::DetachGeometry(FbxScene* scene)
{
	FbxNode* rootNode = scene->GetRootNode();

	// Detach geometry from the skeleton
	for (int NodeIndex = 0; NodeIndex < scene->GetNodeCount(); ++NodeIndex)
	{
		FbxNode* SceneNode = scene->GetNode(NodeIndex);
		if (SceneNode == nullptr)
		{
			continue;
		}
		FbxGeometry* NodeGeometry = static_cast<FbxGeometry*>(SceneNode->GetMesh());
		if (NodeGeometry)
		{
			if (SceneNode->GetParent() &&
				SceneNode->GetParent()->GetNodeAttribute() &&
				SceneNode->GetParent()->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
			{
				SceneNode->GetParent()->RemoveChild(SceneNode);
				rootNode->AddChild(SceneNode);
			}
		}
	}
}

bool FbxCommon::SyncDuplicateBones(FbxScene* currentScene)
{
	// for each bone with .001, sync with original bone
	for (int i = 0; i < currentScene->GetNodeCount(); i++)
	{
		FbxNode* pBone = currentScene->GetNode(i);
		FbxNodeAttribute* Attr = pBone->GetNodeAttribute();
		if (Attr && Attr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			const char* lpBoneName = pBone->GetName();
			QString sBoneName(lpBoneName);
			if (sBoneName.contains(".001"))
			{
				QString sOrigBoneName = QString(sBoneName).replace(".001", "");
				FbxNode* pOrigBone = currentScene->FindNodeByName(sOrigBoneName.toUtf8().constData());
				if (pOrigBone)
				{
					//pBone->Copy(*pOrigBone);
					pBone->LclRotation.Set(pOrigBone->LclRotation.Get());
					pBone->LclScaling.Set(pOrigBone->LclScaling.Get());
					pBone->LclTranslation.Set(pOrigBone->LclTranslation.Get());
					pBone->PreRotation.Set(pOrigBone->PreRotation.Get());
					pBone->PostRotation.Set(pOrigBone->PostRotation.Get());

				}
				else
				{
					dzApp->log(QString("ERROR: SyncDuplicateBones(): OrigBone not found for: %1").arg(sBoneName));
				}
			}
		}
	}


	return true;
}

bool FbxCommon::LoadAndPoseBelowHeadOnly(QString poseFilePath, FbxScene* currentScene, DzProgress* progress, bool convertToZUp)
{
	OpenFBXInterface* openFBX = OpenFBXInterface::GetInterface();

	FbxScene* pPoseScene = openFBX->createScene("My scene");
	if (openFBX->loadScene(pPoseScene, poseFilePath.toUtf8().data()) == false)
	{
		return false;
	}
	//if (progress) progress->step();

	// make nodename lookup table
	QMap<QString, FbxNode*> lookupTable;
	int numPoseNodes = pPoseScene->GetNodeCount();
	for (int i = 0; i < numPoseNodes; i++)
	{
		FbxNode* node = pPoseScene->GetNode(i);
		const char* lpNodeName = node->GetName();
		QString sNodeName(lpNodeName);
		lookupTable.insert(sNodeName, node);
	}
	//if (progress) progress->step();

	// Convert Pose scene to Zup
	if (convertToZUp)
	{
		FbxMesh* mesh = (FbxMesh*)pPoseScene->GetGeometry(0);
		if (mesh)
		{
			ConvertToZUp(mesh, lookupTable["root"]);
		}
	}

	int numMainNodes = currentScene->GetNodeCount();
	for (int i = 0; i < numMainNodes; i++)
	{
		FbxNode* node = currentScene->GetNode(i);
		FbxNodeAttribute* Attr = node->GetNodeAttribute();
		if (Attr && Attr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			const char* lpNodeName = node->GetName();
			QString sNodeName(lpNodeName);
			if (sNodeName.contains("head"))
			{
				break;
			}
			if (lookupTable.find(sNodeName) != lookupTable.end())
			{
				FbxNode* pPoseNode = lookupTable[sNodeName];
				//node->Copy(*pPoseNode);
				node->LclRotation.Set(pPoseNode->LclRotation.Get());
				node->LclScaling.Set(pPoseNode->LclScaling.Get());
				node->LclTranslation.Set(pPoseNode->LclTranslation.Get());
				node->PreRotation.Set(pPoseNode->PreRotation.Get());
				node->PostRotation.Set(pPoseNode->PostRotation.Get());
			}
		}
	}
	//if (progress) progress->step();

	// close pose scene
	pPoseScene->Destroy();

	return true;
}

bool FbxCommon::LoadAndPose(QString poseFilePath, FbxScene* currentScene, DzProgress* progress, bool convertToZUp)
{
	OpenFBXInterface* openFBX = OpenFBXInterface::GetInterface();

	FbxScene* pPoseScene = openFBX->createScene("My scene");
	if (openFBX->loadScene(pPoseScene, poseFilePath.toUtf8().data()) == false)
	{
		return false;
	}
	//if (progress) progress->step();

	// make nodename lookup table
	QMap<QString, FbxNode*> lookupTable;
	int numPoseNodes = pPoseScene->GetNodeCount();
	for (int i = 0; i < numPoseNodes; i++)
	{
		FbxNode* node = pPoseScene->GetNode(i);
		const char* lpNodeName = node->GetName();
		QString sNodeName(lpNodeName);
		lookupTable.insert(sNodeName, node);
	}
	//if (progress) progress->step();

	// Convert Pose scene to Zup
	if (convertToZUp)
	{
		FbxMesh* mesh = (FbxMesh*)pPoseScene->GetGeometry(0);
		if (mesh)
		{
			ConvertToZUp(mesh, lookupTable["root"]);
		}
	}

	int numMainNodes = currentScene->GetNodeCount();
	for (int i = 0; i < numMainNodes; i++)
	{
		FbxNode* node = currentScene->GetNode(i);
		FbxNodeAttribute* Attr = node->GetNodeAttribute();
		if (Attr && Attr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			const char* lpNodeName = node->GetName();
			QString sNodeName(lpNodeName);
			if (sNodeName == "rootNode")
				continue;
			if (lookupTable.find(sNodeName) != lookupTable.end())
			{
				FbxNode* pPoseNode = lookupTable[sNodeName];
				node->Copy(*pPoseNode);
				//node->LclRotation.Set(pPoseNode->LclRotation.Get());
				//node->LclScaling.Set(pPoseNode->LclScaling.Get());
				//node->LclTranslation.Set(pPoseNode->LclTranslation.Get());
				//node->PreRotation.Set(pPoseNode->PreRotation.Get());
				//node->PostRotation.Set(pPoseNode->PostRotation.Get());
			}
		}
	}
	//if (progress) progress->step();

	// close pose scene
	pPoseScene->Destroy();

	return true;
}

int FbxCommon::ConvertToZUp(FbxMesh* mesh, FbxNode* rootNode)
{
	int correction = 0;
	FbxVector4 eulerRotation;
	bool bRotate = false;
	if (bRotate == false)
	{
		// 1. Find Bounding Box
		FbxVector4* result = CalculateBoundingVolume(mesh);
		// 2. Check longest axis
		FbxVector4 cloudSize = result[0];
		FbxVector4 cloudCenter = result[1];
		if (cloudSize[1] > cloudSize[2])
		{
			// 3. If longest axis is not Y, then flip
			bRotate = true;
			// check Y value to figure out which direction to flip
			if (cloudCenter[1] > 0)
			{
				// mesh is +Yup
				correction = 90;
				eulerRotation = FbxVector4(correction, 0, 0);
			}
			else
			{
				correction = -90;
				eulerRotation = FbxVector4(correction, 0, 0);
			}
		}
		delete[] result;
	}
	//FbxNode* rootNode = lookupTable[QString("root")];
	if (rootNode && bRotate)
	{
		// HARD-CODED 90-deg X-axis rotation of root node....
		// TODO: detect and apply global axis correction as needed
		rootNode->LclRotation.Set(eulerRotation + rootNode->LclRotation.Get());
	}

	return correction;
}

bool FbxCommon::FlipAndBakeVertexBuffer(FbxMesh* mesh, FbxNode* rootNode, FbxVector4* vertexBuffer)
{
	if (ConvertToZUp(mesh, rootNode) == false)
		return false;
	FbxAMatrix matrix = FbxCommon::GetAffineMatrix(NULL, mesh->GetNode());
	BakePoseToVertexBuffer(vertexBuffer, &matrix, nullptr, mesh);

	return true;
}

FbxCluster* FbxCommon::FindClusterFromNode(FbxNode* node)
{
	// debug
	int numDstConnections = node->GetDstObjectCount();
	int numSrcConnections = node->GetSrcObjectCount();
	const char* lpNdoeName = node->GetName();

	FbxCluster* pCluster1 = (FbxCluster*)node->GetSrcObject(FbxCriteria::ObjectType(FbxCluster::ClassId));
	FbxCluster* pCluster2 = (FbxCluster*)node->GetDstObject(FbxCriteria::ObjectType(FbxCluster::ClassId));
	FbxCluster* cluster = nullptr;

	if (pCluster1)
	{
		cluster = pCluster1;
	}
	else if (pCluster2)
	{
		cluster = pCluster2;
	}

	return cluster;

}

void FbxCommon::removeMorphExportPrefixFromBlendShapeChannel(FbxBlendShapeChannel* channel, const char* prefix)
{
	// DB 2025-04-08: rename deformer (blendshapechannel)
	FbxString newChannelName = channel->GetName();
	newChannelName.FindAndReplace(prefix, "", 0);
	channel->SetName(newChannelName.Buffer());

	int shapeCount = channel->GetTargetShapeCount();
	for (int shapeIndex = 0; shapeIndex < shapeCount; ++shapeIndex)
	{
		FbxShape* shape = channel->GetTargetShape(shapeIndex);
		if (shape)
		{
			FbxString newName = shape->GetName();
			newName.FindAndReplace(prefix, "", 0);
			shape->SetName(newName.Buffer());
		}
	}
}

void FbxCommon::removeMorphExportPrefixFromNode(FbxNode* node, const char* prefix)
{
	if (node)
	{
		// Check if the node has a mesh
		FbxMesh* mesh = node->GetMesh();

		// Rename Shapes
		if (mesh)
		{
			int deformerCount = mesh->GetDeformerCount(FbxDeformer::eBlendShape);
			for (int deformerIndex = 0; deformerIndex < deformerCount; ++deformerIndex)
			{
				FbxBlendShape* blendShape = static_cast<FbxBlendShape*>(mesh->GetDeformer(deformerIndex, FbxDeformer::eBlendShape));

				int blendShapeChannelCount = blendShape->GetBlendShapeChannelCount();
				for (int channelIndex = 0; channelIndex < blendShapeChannelCount; ++channelIndex)
				{
					FbxBlendShapeChannel* channel = blendShape->GetBlendShapeChannel(channelIndex);
					if (channel)
					{
						// Rename the shapes associated with this channel
						removeMorphExportPrefixFromBlendShapeChannel(channel, prefix);
					}
				}
			}
		}

		// Recursively process children nodes
		for (int j = 0; j < node->GetChildCount(); j++)
		{
			removeMorphExportPrefixFromNode(node->GetChild(j), prefix);
		}
	}
}

bool FbxCommon::MultiplyMatrixToVertexBuffer(FbxAMatrix* matrix, FbxVector4* vertexBuffer, int numVerts)
{
	// apply weight * matrix transform to each vertex
	for (int i = 0; i < numVerts; i++)
	{
		FbxVector4 sourceVertex = vertexBuffer[i];
		FbxVector4 finalTargetVertex = matrix->MultT(sourceVertex);
		vertexBuffer[i] = finalTargetVertex;
	}
	return true;
}

FbxVector4 FbxCommon::CalculatePointCloudCenter(FbxVector4* vertexBuffer, int numVertices, bool centerWeight)
{

	if (vertexBuffer == nullptr || numVertices <= 0)
	{
		dzApp->warning("ERROR: CalculatePointCloudCenter recieved invalid inputs");
		return nullptr;
	}

	FbxVector4 cloudCenter = vertexBuffer[0];
	FbxVector4 min_bounds = cloudCenter;
	FbxVector4 max_bounds = cloudCenter;
	for (int vertIndex = 0; vertIndex < numVertices; vertIndex++)
	{
		FbxVector4 currentPoint = vertexBuffer[vertIndex];
		for (int i = 0; i < 3; i++)
		{
			if (currentPoint[i] < min_bounds[i]) min_bounds[i] = currentPoint[i];
			if (currentPoint[i] > max_bounds[i]) max_bounds[i] = currentPoint[i];
		}
	}
	double center_weight = 0;
	if (abs(max_bounds[0]) < abs(min_bounds[0]))
		center_weight = max_bounds[0];
	else
		center_weight = min_bounds[0];

	if (centerWeight)
		//		cloudCenter[0] = (max_bounds[0] + min_bounds[0] + center_weight) / 3;
		cloudCenter[0] = center_weight;
	else
		cloudCenter[0] = (max_bounds[0] + min_bounds[0]) / 2;
	cloudCenter[1] = (max_bounds[1] + min_bounds[1]) / 2;
	cloudCenter[2] = (max_bounds[2] + min_bounds[2]) / 2;

	//	FbxVector4 cloudAverage = CalculatePointCloudAverage(mesh, vertexIndexes);

	return cloudCenter;

}

bool FbxCommon::GetAllMeshes(FbxNode* node, QList<FbxNode*>& nodeList)
{
	if (node == NULL) return false;

	auto attribute = node->GetNodeAttribute();
	if (attribute)
	{
		auto attributeType = attribute->GetAttributeType();
		if (attributeType == FbxNodeAttribute::eMesh)
		{
			nodeList.append(node);
		}
	}

	for (int i = 0; i < node->GetChildCount(); i++)
	{
		FbxNode* pChild = node->GetChild(i);
		GetAllMeshes(pChild, nodeList);
	}

	return true;
}

bool FbxCommon::HasNodeAncestor(FbxNode* node, const QString ancestorName, Qt::CaseSensitivity cs)
{

	FbxNode* pParentNode = node->GetParent();
	if (pParentNode == NULL) return false;

	QString sParentName = pParentNode->GetName();
	if (sParentName.compare(ancestorName, cs) == 0)
	{
		return true;
	}
	return HasNodeAncestor(pParentNode, ancestorName, cs);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEV TESTING
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FbxCommon::FixClusterTranformLinks(FbxScene* scene, FbxNode* rootNode, bool correctFix)
{
	FbxGeometry* NodeGeometry = static_cast<FbxGeometry*>(rootNode->GetMesh());

	// Create missing weights
	if (NodeGeometry)
	{

		for (int DeformerIndex = 0; DeformerIndex < NodeGeometry->GetDeformerCount(); ++DeformerIndex)
		{
			FbxSkin* Skin = static_cast<FbxSkin*>(NodeGeometry->GetDeformer(DeformerIndex));
			if (Skin)
			{
				for (int ClusterIndex = 0; ClusterIndex < Skin->GetClusterCount(); ++ClusterIndex)
				{
					// Get the current tranform
					FbxAMatrix Matrix;
					FbxCluster* Cluster = Skin->GetCluster(ClusterIndex);
					Cluster->GetTransformLinkMatrix(Matrix);

					QString sBoneName(Cluster->GetLink()->GetName());

					// Update the rotation
					FbxDouble3 Rotation = Cluster->GetLink()->PostRotation.Get();
					if (correctFix)
					{
						Matrix.MultRM(Rotation);
						if (sBoneName.contains("_r"))
						{
							Matrix.MultRM(FbxVector4(90, 0, 0));
						}
						else
						{
							Matrix.MultRM(FbxVector4(-90, 0, 0));
						}
						if (sBoneName.contains("ball_"))
						{
							Matrix.MultRM(FbxVector4(90, 0, 0));
						}
						else if (sBoneName.contains("thumb_"))
						{
							Matrix.MultRM(FbxVector4(0, 0, 0));
						}
						else if (sBoneName.contains("hand_") ||
							HasNodeAncestor(Cluster->GetLink(), "hand_r", Qt::CaseInsensitive) ||
							HasNodeAncestor(Cluster->GetLink(), "hand_l", Qt::CaseInsensitive))
						{
							Matrix.MultRM(FbxVector4(-90, 0, 0));
						}
						if (sBoneName.contains("_l") || sBoneName.contains("_r"))
						{
							if (HasNodeAncestor(Cluster->GetLink(), "spine_01", Qt::CaseInsensitive))
							{
								Matrix.MultRM(FbxVector4(0, 0, 0));
							}
							else
							{
								Matrix.MultRM(FbxVector4(0, -90, 0));
							}
						}
						else
						{
							Matrix.MultRM(FbxVector4(0, -90, 0));
						}

					}
					else
					{
						Matrix.SetR(Rotation);
					}
					Cluster->SetTransformLinkMatrix(Matrix);
				}
			}
		}
	}

	for (int ChildIndex = 0; ChildIndex < rootNode->GetChildCount(); ++ChildIndex)
	{
		FbxNode* ChildNode = rootNode->GetChild(ChildIndex);
		FixClusterTranformLinks(scene, ChildNode, correctFix);
	}
}

void FbxCommon::RemovePrePostRotations(FbxNode* node)
{
	QString sNodeName = node->GetName();
	for (int nChildIndex = 0; nChildIndex < node->GetChildCount(); nChildIndex++)
	{
		FbxNode* pChildBone = node->GetChild(nChildIndex);
		RemovePrePostRotations(pChildBone);
	}
	if (sNodeName.contains("twist", Qt::CaseInsensitive) == false)
	{
		node->SetPreRotation(FbxNode::EPivotSet::eSourcePivot, FbxVector4(0, 0, 0));
		node->SetPostRotation(FbxNode::EPivotSet::eSourcePivot, FbxVector4(0, 0, 0));
	}
}

#define TCHAR_TO_UTF8(a) a
#define TEXT(a) a