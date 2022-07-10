#include "Scene.h"

std::string getNodeName(const Scene& scene, int node)
{
	int strID = scene.nodeIDToNameID.contains(node) ? scene.nodeIDToNameID.at(node) : -1;
	return (strID > -1) ? scene.names[strID] : std::string();
}

int addNode(Scene& scene, int parent, int level)
{
	// the current size of hierarchy array is the new node's ID
	int node = (int)scene.hierarchy.size();

	// new identity transfoms are added to the new node
	scene.localTransform.push_back(glm::mat4(1.0f));
	scene.globalTransform.push_back(glm::mat4(1.0f));

	// the new node only consists of the parent reference
	scene.hierarchy.push_back({.parent = parent, .lastSibling = -1});

	// if we have a parent
	if (parent > -1)
	{
		int s = scene.hierarchy[parent].firstChild;
		// if the parent node has no children
		if (s == -1)
		{
			// directly set its firstChild
			scene.hierarchy[parent].firstChild = node;
			scene.hierarchy[node].lastSibling  = node;
		}
		// if parent node has children:
		else
		{
			// run over the siblings of the node to find out where to add it
			int dest = scene.hierarchy[s].lastSibling;
			if (dest <= -1)
			{
				// no cached lastSibling, iterate nextSibling indices
				for (dest = s; scene.hierarchy[dest].nextSibling != -1; dest = scene.hierarchy[dest].nextSibling);
			}
			scene.hierarchy[dest].nextSibling = node;
			scene.hierarchy[s].lastSibling    = node;
		}
	}
	scene.hierarchy[node].level = level;
	// store negative indices for the newly added nodes
	scene.hierarchy[node].nextSibling = -1;
	scene.hierarchy[node].firstChild  = -1;
	return node;
}

void loadScene(const char* fileName, Scene& scene)
{
	FILE* f = fopen(fileName, "rb");

	if (!f)
	{
		printf("Cannot open scene file '%s'. Please run SceneConverterTool  and/or MergeMeshes.", fileName);
		return;
	}

	uint32_t nodeCount = 0;
	fread(&nodeCount, sizeof(nodeCount), 1, f);

	scene.hierarchy.resize(nodeCount);
	scene.globalTransform.resize(nodeCount);
	scene.localTransform.resize(nodeCount);
	// TODO: check > -1
	// TODO: recalculate changedAtThisLevel() - find max depth of a node [or save scene.maxLevel]
	fread(scene.localTransform.data(), sizeof(glm::mat4), nodeCount, f);
	fread(scene.globalTransform.data(), sizeof(glm::mat4), nodeCount, f);
	fread(scene.hierarchy.data(), sizeof(Hierarchy), nodeCount, f);

	// Mesh for node [index to some list of buffers]
	loadMap(f, scene.nodeIDToMaterialID);
	loadMap(f, scene.nodeIDToMeshID);

	if (!feof(f))
	{
		loadMap(f, scene.nodeIDToNameID);
		loadStringList(f, scene.names);
		loadStringList(f, scene.materialNames);
	}

	fclose(f);
}

void saveScene(const char* fileName, const Scene& scene)
{
	FILE* f = fopen(fileName, "wb");

	// write the count of scene nodes
	const uint32_t nodeCount = (uint32_t)scene.hierarchy.size();
	fwrite(&nodeCount, sizeof(nodeCount), 1, f);


	fwrite(scene.localTransform.data(), sizeof(glm::mat4), nodeCount, f);
	fwrite(scene.globalTransform.data(), sizeof(glm::mat4), nodeCount, f);
	fwrite(scene.hierarchy.data(), sizeof(Hierarchy), nodeCount, f);

	// Mesh for node [index to some list of buffers]
	saveMap(f, scene.nodeIDToMaterialID);
	saveMap(f, scene.nodeIDToMeshID);

	if (!scene.names.empty() && !scene.nodeIDToNameID.empty())
	{
		saveMap(f, scene.nodeIDToNameID);
		saveStringList(f, scene.names);
		saveStringList(f, scene.materialNames);
	}
	fclose(f);
}

void saveMap(FILE* f, const std::unordered_map<uint32_t, uint32_t>& map)
{
	std::vector<uint32_t> ms;
	ms.reserve(map.size() * 2);
	for (const auto& m : map)
	{
		ms.push_back(m.first);
		ms.push_back(m.second);
	}
	const uint32_t sz = static_cast<uint32_t>(ms.size());
	fwrite(&sz, sizeof(sz), 1, f);
	fwrite(ms.data(), sizeof(int), ms.size(), f);
}

void saveStringList(FILE* f, const std::vector<std::string>& lines)
{
	uint32_t sz = (uint32_t)lines.size();
	fwrite(&sz, sizeof(uint32_t), 1, f);
	for (const auto& s : lines)
	{
		sz = (uint32_t)s.length();
		fwrite(&sz, sizeof(uint32_t), 1, f);
		fwrite(s.c_str(), sz + 1, 1, f);
	}
}

void loadStringList(FILE* f, std::vector<std::string>& lines)
{
	{
		uint32_t sz = 0;
		fread(&sz, sizeof(uint32_t), 1, f);
		lines.resize(sz);
	}
	std::vector<char> inBytes;
	for (auto& s : lines)
	{
		uint32_t sz = 0;
		fread(&sz, sizeof(uint32_t), 1, f);
		inBytes.resize(sz + 1);
		fread(inBytes.data(), sz + 1, 1, f);
		s = std::string(inBytes.data());
	}
}

void loadMap(FILE* f, std::unordered_map<uint32_t, uint32_t>& map)
{
	std::vector<uint32_t> ms;

	uint32_t sz = 0;
	fread(&sz, 1, sizeof(sz), f);

	ms.resize(sz);
	fread(ms.data(), sizeof(int), sz, f);
	for (size_t i          = 0; i < (sz / 2); i++)
		map[ms[i * 2 + 0]] = ms[i * 2 + 1];
}

// mark this node whose transforms have changed in this frame and its children as changed
void markAsChanged(Scene& scene, int node)
{
	// mark this node
	int level = scene.hierarchy[node].level;
	scene.changedAtThisFrame[level].push_back(node);

	// mark its children 
	// TODO: use non-recursive iteration with aux stack
	for (int s = scene.hierarchy[node].firstChild; s != - 1; s = scene.hierarchy[s].nextSibling)
	{
		markAsChanged(scene, s);
	}
}

// CPU version of global transform update []
// TODO: implement a GPU version using compute shaders 
void recalculateGlobalTransforms(Scene& scene)
{
	// start from the root layer, check if any nodes are changed
	if (!scene.changedAtThisFrame[0].empty())
	{
		// assume we have only one root node
		int nodeIndex = scene.changedAtThisFrame[0][0];
		// root node global transforms coincide with their local transforms
		scene.globalTransform[nodeIndex] = scene.localTransform[nodeIndex];
		scene.changedAtThisFrame[0].clear();
	}

	// check all the lower levels
	for (int i = 1; i < MAX_NODE_LEVEL && (!scene.changedAtThisFrame[i].empty()); i++)
	{
		for (const int& c : scene.changedAtThisFrame[i])
		{
			int p = scene.hierarchy[c].parent;
			// NO RECURSION. MAGIC! 
			scene.globalTransform[c] = scene.globalTransform[p] * scene.localTransform[c];
		}
		scene.changedAtThisFrame[i].clear();
	}
}
