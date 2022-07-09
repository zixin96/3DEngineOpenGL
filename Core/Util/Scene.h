#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
using std::vector;
using glm::mat4;
using std::unordered_map;
using std::string;

constexpr int MAX_NODE_LEVEL = 16;

struct Hierarchy
{
	// parent for this node (or -1 for root)
	int parent;
	// first child for a node (or -1)
	int firstChild;
	// next sibling for a node (or -1)
	int nextSibling;
	// last added node (or -1)
	int lastSibling;
	// stores the cached depth of the node from the top of the scene graph
	int level;
};

// In a data-oriented scene graph, each scene node is represented implicitly by
// integer indices in the arrays inside the Scene structure
struct Scene
{
	// the local and global transforms are stored in separate arrays
	vector<mat4> localTransform;
	vector<mat4> globalTransform;

	// list of nodes whose global transform must be recalculated
	vector<int> changedAtThisFrame[MAX_NODE_LEVEL];

	vector<Hierarchy> hierarchy;

	// hash tables to store node-to-mesh, node-to-material, node-to-name mappings
	// absence of such mappings indicates that a node doesn't have such property

	// (Node -> Mesh)
	unordered_map<uint32_t, uint32_t> nodeIDToMeshID;
	// (Node -> Material)
	unordered_map<uint32_t, uint32_t> nodeIDToMaterialID;
	// (Node -> Name)
	unordered_map<uint32_t, uint32_t> nodeIDToNameID;

	// collection of debug node names and material names
	vector<string> names;
	vector<string> materialNames;
};

int  addNode(Scene& scene, int parent, int level);
void loadScene(const char* fileName, Scene& scene);
void loadStringList(FILE* f, std::vector<std::string>& lines);
void loadMap(FILE* f, std::unordered_map<uint32_t, uint32_t>& map);
void saveScene(const char* fileName, const Scene& scene);
void saveMap(FILE* f, const std::unordered_map<uint32_t, uint32_t>& map);
void saveStringList(FILE* f, const std::vector<std::string>& lines);

void markAsChanged(Scene& scene, int node);
void recalculateGlobalTransforms(Scene& scene);