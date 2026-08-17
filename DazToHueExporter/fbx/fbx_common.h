#pragma once

#include <QTCore/qlist.h>
#include <fbxsdk.h>

#define Y_TO_Z(a) FbxVector4(a[0], a[2], a[1])
#define Y_TO_NEGZ(a) FbxVector4(a[0], a[2], -a[1])
#define NEGY_TO_NEGZ(a) FbxVector4(a[0], -a[2], -a[1])
#define NEGZ(a) FbxVector4(a[0], a[1], -a[2])

class DzProgress;

class FbxCommon
{
public:
	static double getLength(double a, double b);

	static double getLength(double a, double b, double c);

	static double getDistance(FbxVector2 a, FbxVector2 b);

	static double getDistance(FbxVector4 a, FbxVector4 b);

	static double determinant_3x3(FbxVector4* matrix);

	static FbxVector4* CalculateBoundingVolume(QList<FbxVector4>& pointCloud);

	static FbxVector4* CalculateBoundingVolume(FbxMesh* mesh);

	static FbxVector4* CalculateBoundingVolume(FbxMesh* mesh, QList<int>* vertexIndexes);

	static void MultiplyMatrix_InPlace(FbxAMatrix& matrix, double value);

	static void AddToScaleOfMatrix_InPlace(FbxAMatrix& matrix, double value);

	static void AddMatrix_InPlace(FbxAMatrix& destinationMatrix, const FbxAMatrix& sourceMatrix);

	static FbxAMatrix GetPoseMatrix(FbxPose* pose, int nodeIndex);

	static FbxAMatrix GetAffineMatrix(FbxPose* pose, int itemIndex, bool returnLocalSpace = false, FbxTime fbxTime = FBXSDK_TIME_INFINITE);

	static FbxAMatrix GetAffineMatrix(FbxPose* pose, FbxNode* node, bool returnLocalSpace = false, FbxTime fbxTime = FBXSDK_TIME_INFINITE);

	static FbxAMatrix GetGeometricAffineMatrix(FbxNode* node);

	static bool CalculateClusterDeformationMatrix(FbxAMatrix& clusterDeformationMatrix, FbxCluster* cluster, FbxAMatrix* globalOffsetMatrix, FbxPose* pose, const FbxMesh* mesh, FbxTime fbxTime = FBXSDK_TIME_INFINITE);

	static bool BakePoseToVertexBuffer_LinearPathway(FbxVector4* vertexBuffer, FbxAMatrix* globalOffsetMatrix, FbxPose* pose, const FbxMesh* mesh, FbxTime fbxTime = FBXSDK_TIME_INFINITE);

	static bool BakePoseToVertexBuffer_DualQuaternionPathway(FbxVector4* vertexBuffer, FbxAMatrix* globalOffsetMatrix, FbxPose* pose, const FbxMesh* mesh, FbxTime fbxTime = FBXSDK_TIME_INFINITE);

	static bool BakePoseToVertexBuffer(FbxVector4* vertexBuffer, FbxAMatrix* globalOffsetMatrix, FbxPose* pose, const FbxMesh* mesh, FbxTime time = FBXSDK_TIME_INFINITE);

	static FbxAMatrix FindPoseMatrixOrIdentity(FbxPose* pose, FbxNode* node);

	static FbxAMatrix FindPoseMatrixOrGlobal(FbxPose* pose, FbxNode* node);

	static void RemoveBindPoses(FbxScene* scene);

	static FbxPose* SaveBindMatrixToPose(FbxScene* scene, const char* poseName, FbxNode* meshNode = nullptr, bool addPose = false);

	static void ApplyBindPose(FbxScene* scene, FbxPose* pose, FbxNode* node = nullptr, bool recurse = true, bool clampJoints = false);

	static FbxNode* GetRootBone(FbxScene* scene, bool renameRootBone = false, FbxNode* previousBone = nullptr);

	static void DetachGeometry(FbxScene* scene);

	static bool BakePoseToBindMatrix(FbxMesh* mesh, FbxPose* pose);

	static bool SyncDuplicateBones(FbxScene* currentScene);

	static bool LoadAndPoseBelowHeadOnly(QString poseFilePath, FbxScene* currentScene, DzProgress* progress = nullptr, bool convertToZUp = false);

	static bool LoadAndPose(QString poseFilePath, FbxScene* currentScene, DzProgress* progress = nullptr, bool convertToZUp = false);

	static int ConvertToZUp(FbxMesh* mesh, FbxNode* rootNode);

	static bool FlipAndBakeVertexBuffer(FbxMesh* mesh, FbxNode* rootNode, FbxVector4* vertexBuffer);

	static FbxCluster* FindClusterFromNode(FbxNode* node);

	static FbxVector4 CalculatePointCloudAverage(FbxMesh* mesh, QList<int>* vertexIndexes);

	static FbxVector4 CalculatePointCloudCenter(FbxMesh* mesh, QList<int>* vertexIndexes, bool centerWeight = false);

	static void removeMorphExportPrefixFromNode(FbxNode* node, const char* prefix);
	static void removeMorphExportPrefixFromBlendShapeChannel(FbxBlendShapeChannel* channel, const char* prefix);

	static bool MultiplyMatrixToVertexBuffer(FbxAMatrix* matrix, FbxVector4* vertexBuffer, int numVerts);

	static FbxVector4 CalculatePointCloudCenter(FbxVector4* vertexBuffer, int numVertices, bool centerWeight = false);

	static bool GetAllMeshes(FbxNode* node, QList<FbxNode*>& nodeList);
	static bool HasNodeAncestor(FbxNode* node, const QString ancestorName, Qt::CaseSensitivity cs = Qt::CaseSensitive);

	static void FixClusterTranformLinks(FbxScene* scene, FbxNode* rootNode, bool correctFix = true);
	static void RemovePrePostRotations(FbxNode* node);
};
